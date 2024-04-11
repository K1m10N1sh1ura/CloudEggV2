#include "string.h"
#include "freertos/FreeRTOS.h"
#include "driver/gpio.h"
#include "gpio_pins.h"
#include "gpio_config.h"
#include "hc_sr04.h"
#include "tasks.h"

// Global Variables
HC_SR04_Manager sensor_manager;

// Function Prototypes
void vAcousticBarrierTask( void * pvParameters );
void vCliTask( void * pvParameters );
static void gpio_isr_handler(void* arg);
float getDist(HC_SR04_Manager *manager);

// RTOS Handles
TaskHandle_t xAcousticBarrierTaskHandle = NULL;
TaskHandle_t xCliTaskHandle = NULL;


void app_main(void) {
    HC_SR04_init(&sensor_manager); 
    gpio_setup();

    // init ISR
    gpio_install_isr_service(ESP_INTR_FLAG_IRAM);
    gpio_isr_handler_add(GPIO_OUTPUT_IO_ECHO, gpio_isr_handler, (void*)&sensor_manager);

    // RTOS Config
    xTaskCreate(vAcousticBarrierTask, "AcousticBarrierTask", 2048, NULL, 1, &xAcousticBarrierTaskHandle);
    xTaskCreate(vCliTask, "CliTask", 2048, NULL, 2, &xCliTaskHandle);

}

void vCliTask( void * pvparameters) {

    char cli_command[80];

    while(1) {
        if (scanf("%79s", cli_command) == 1) {  // Check if scanf successfully read a string
            if (strcmp(cli_command, "m") == 0) {
                printf("Sound barrier on! \n");
                if(xAcousticBarrierTaskHandle != NULL) {
                    vTaskResume(xAcousticBarrierTaskHandle); // Ensure xMainTaskHandle is initialized and points to a valid task
                }
            }
            else {
                printf("Invalid command\n");
            }
        }
        vTaskDelay(500 / portTICK_PERIOD_MS);
    }
}

void vAcousticBarrierTask( void * pvParameters ) {

    gpio_set_level(GPIO_OUTPUT_IO_TRIGGER, 1); 
    float distance;
    vTaskSuspend(NULL);

    while(1)
    {
        do {
            distance = getDist(&sensor_manager);
        }
        while (distance > 1.0 || distance < 0.0001);
        printf("TRIGGER\n");
        vTaskSuspend(NULL);
    }
}

static void IRAM_ATTR gpio_isr_handler(void* arg) {  

    HC_SR04_Manager *manager = (HC_SR04_Manager *)arg;
    int currentLevel = gpio_get_level(GPIO_OUTPUT_IO_ECHO);

    if (currentLevel == 1 && !manager->running_measurement && !manager->echo_started) {
        manager->time_start = esp_cpu_get_cycle_count();
        manager->running_measurement = true;
        manager->echo_started = true;
    } else if (currentLevel == 0 && manager->running_measurement && manager->echo_started) {
        manager->time_stop = esp_cpu_get_cycle_count();
        manager->running_measurement = false;
        manager->echo_started = false;
        manager->echo_stopped = true;
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;
        xSemaphoreGiveFromISR(manager->semaphore, &xHigherPriorityTaskWoken);
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }
    else {
        manager->running_measurement = false;
        manager->echo_stopped = false;
        manager->echo_started = false;
        manager->time_start = 0;
        manager->time_stop = 0;
    }
}