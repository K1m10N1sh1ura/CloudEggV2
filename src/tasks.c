/*
 * File: tasks.c
 * Author: Kimio Nishiura
 * Description: contains the RTOS tasks and ISRs:
 *      1. vCliTask: 
 *      2. vAcousticBarrierTask:
 *      3. hcsr04_gpio_isr_handler:
 *              
 * Created on: January 1, 2024
 */

#include "tasks.h"

// Global Variables
Meastask_Manager meastask_manager;

// RTOS Handles
TaskHandle_t xAcousticBarrierTaskHandle = NULL;
TaskHandle_t xCliTaskHandle = NULL;

void vCliTask( void * pvParameters) {

    char cli_command[8];
    cli_menu();

    while(1) {
        if (scanf("%7s", cli_command) == 1) {  // Check if scanf successfully read a string
            if (strcmp(cli_command, "1") == 0) {
                if (!meastask_manager.running_measurement) {
                    if(xAcousticBarrierTaskHandle != NULL) {
                        vTaskResume(xAcousticBarrierTaskHandle); // Resume the acoustic barrier task
                    }
                }
                else {
                    printf("Error, last measurement not finished yet!\n");
                }
            }
            else if (strcmp(cli_command, "2") == 0) {
                printf("\nDistance = %.2f \n", getDist(&hc_sr04_manager));
            }
            else if (strcmp(cli_command, "3") == 0) {
                CloudEgg_validate_sensor_pos(&pos_manager);
                printSensorValidationResult(&pos_manager);
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
    within a range of 0 to MAX_DISTANCE_TO_OBJECT meters.
*/
void vAcousticBarrierTask(void *pvParameters) {
    // Initialize the ultrasonic sensor
    HC_SR04_init(&hc_sr04_manager);
    // Initialize the GPIO trigger
    meastask_manager.current_time_us = 0;
    meastask_manager.running_measurement = false;
    float distance;
    int32_t num_meas = 0;
    int64_t elapsed_time_us;
    int64_t time_start_us;
    int64_t time_stop_us;
    struct timeval tv_start, tv_stop;
    // Suspend the task initially
    vTaskSuspend(NULL);

    while (1) {
        meastask_manager.running_measurement = true;
        // Record the start time
        gettimeofday(&tv_start, NULL);
        time_start_us = (int64_t)tv_start.tv_sec * 1000000L + (int64_t)tv_start.tv_usec;
        printf("Soundbarrier ready!\n");
        // Continuously measure distance until an object is detected within the range (MAX_DISTANCE_TO_OBJECT)
        do {
            distance = getDist(&hc_sr04_manager);
            num_meas++;
            // Record the stop time
            gettimeofday(&tv_stop, NULL);
            time_stop_us = (int64_t)tv_stop.tv_sec * 1000000L + (int64_t)tv_stop.tv_usec;
            elapsed_time_us = (time_stop_us - time_start_us);
            meastask_manager.current_time_us = elapsed_time_us;
        } while (distance > MAX_DISTANCE_TO_OBJECT || distance < 0.0);
        
        float elapsed_time_s = elapsed_time_us/1e6;
        printf("\nTriggered after %.2f s at distance: %.2f m with %.2f measurements per second\n", elapsed_time_s, distance, (float) num_meas/elapsed_time_s);
        num_meas = 0;
        meastask_manager.running_measurement = false;
        // Suspend the task until the next trigger
        vTaskSuspend(NULL);
    }
}

void cli_menu() {
    printf("\nMenu:\n");
    printf(" 1 - Turn on sound barrier\n");
    printf(" 2 - Display distance\n");
    printf(" 3 - Validate CloudEgg positioning\n");
    printf("Enter command: \n");
}

void validateCloudEggPosition() {

}

void hcsr04_gpio_isr_handler(void* arg) {  
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

