#include "main.h"

// Global Variables
HC_SR04_Manager hc_sr04_manager;

// RTOS Handles
TaskHandle_t xAcousticBarrierTaskHandle = NULL;
TaskHandle_t xCliTaskHandle = NULL;

void app_main(void) {
    // Initialize the ultrasonic sensor
    HC_SR04_init(&hc_sr04_manager);

    // Set up GPIO
    gpio_setup();

    // Install ISR service and add handler for echo pin
    gpio_install_isr_service(ESP_INTR_FLAG_IRAM);
    gpio_isr_handler_add(GPIO_OUTPUT_IO_ECHO, gpio_isr_handler, (void*)&hc_sr04_manager);

    // Create RTOS tasks
    xTaskCreate(vAcousticBarrierTask, "AcousticBarrierTask", 2048, NULL, 1, &xAcousticBarrierTaskHandle);
    xTaskCreate(vCliTask, "CliTask", 2048, NULL, 2, &xCliTaskHandle);

    // Initialize WiFi
    wifi_init();

    // Initialize server
    server_init();
}

void vCliTask( void * pvParameters) {
    char cli_command[8];
    cli_menu();

    while(1) {
        if (scanf("%79s", cli_command) == 1) {  // Check if scanf successfully read a string
            if (strcmp(cli_command, "1") == 0) {
                printf("Sound barrier on! \n");
                if(xAcousticBarrierTaskHandle != NULL) {
                    vTaskResume(xAcousticBarrierTaskHandle); // Resume the acoustic barrier task
                }
            }
            else if (strcmp(cli_command, "2") == 0) {
                printf("\nDistance = %.2f \n", getDist(&hc_sr04_manager));
            }
            else if (strcmp(cli_command, "3") == 0) {
                struct timeval tv_now;
                gettimeofday(&tv_now, NULL);
                int64_t time_us = (int64_t)tv_now.tv_sec * 1000000L + (int64_t)tv_now.tv_usec;
                printf("Time: %lld\n", time_us);
            }
            else {
                printf("Invalid command\n\n");
                cli_menu();
            }
        }
        vTaskDelay(500 / portTICK_PERIOD_MS); // Delay to prevent CPU hogging and debounce input
    }
}

/*
    This task continuously measures distance and logs the time when an object is detected
    within a range of 0 to 1.5 meters.
*/
void vAcousticBarrierTask(void *pvParameters) {
    // Initialize the GPIO trigger
    gpio_set_level(GPIO_OUTPUT_IO_TRIGGER, 1);

    float distance;
    struct timeval tv_start, tv_stop;

    // Suspend the task initially
    vTaskSuspend(NULL);

    while (1) {
        // Record the start time
        gettimeofday(&tv_start, NULL);

        // Continuously measure distance until an object is detected within the range
        do {
            distance = getDist(&hc_sr04_manager);
        } while (distance > 1.5 || distance < 0);

        // Record the stop time
        gettimeofday(&tv_stop, NULL);

        // Calculate the elapsed time in milliseconds
        int64_t time_start_us = (int64_t)tv_start.tv_sec * 1000000L + (int64_t)tv_start.tv_usec;
        int64_t time_stop_us = (int64_t)tv_stop.tv_sec * 1000000L + (int64_t)tv_stop.tv_usec;
        int64_t elapsed_time_ms = (time_stop_us - time_start_us) / 1000;

        // Output the result
        printf("\nTriggered after %lld ms at distance: %.2f m\n", elapsed_time_ms, distance);

        // Suspend the task until the next trigger
        vTaskSuspend(NULL);
    }
}

void cli_menu() {
    printf("\nMenu:\n");
    printf(" 1 - Turn on sound barrier\n");
    printf(" 2 - Display distance\n");
    printf(" 3 - Show current time\n");
    printf("Enter command: \n");
}

static void IRAM_ATTR gpio_isr_handler(void* arg) {  
    /*
        Interrupt handler for HC-SR04 sensor echoes.
        Tracks the time between the rising and falling edges of the echo signal to calculate the travel time of an ultrasound pulse.

        Arguments:
        arg - Pointer to the HC_SR04_Manager which manages state for the ultrasonic sensor.
    */

    HC_SR04_Manager *manager = (HC_SR04_Manager *)arg;
    int currentLevel = gpio_get_level(GPIO_OUTPUT_IO_ECHO);

    if (currentLevel == 1 && !manager->running_measurement && !manager->echo_started) {
        // Rising edge detected, save start time stamp
        manager->time_start = esp_cpu_get_cycle_count();
        manager->running_measurement = true;
        manager->echo_started = true;
    } else if (currentLevel == 0 && manager->running_measurement && manager->echo_started) {
        // Falling edge detected, save stop time stamp
        manager->time_stop = esp_cpu_get_cycle_count();
        manager->running_measurement = false;
        manager->echo_started = false;
        manager->echo_stopped = true;
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;
        xSemaphoreGiveFromISR(manager->semaphore, &xHigherPriorityTaskWoken);
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }
    else {
        // In case of noise or spurious triggers, reset the measurement state
        manager->running_measurement = false;
        manager->echo_stopped = false;
        manager->echo_started = false;
        manager->time_start = 0;
        manager->time_stop = 0;
    }
}

