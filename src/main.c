#include "stdio.h"
#include "string.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/uart.h"
#include "esp_system.h"
#include "esp_log.h"
#include "esp_clk_tree.h"
#include "gpio_pins.h"
#include "gpio_config.h"
#include "hc_sr04.h"
#include "tasks.h"

// Global Variables
HC_SR04_Manager sensor_manager;

// Function Prototypes
void vMainTask( void * pvParameters );
static void gpio_isr_handler(void* arg);
float getDist();

// RTOS Handles
TaskHandle_t xMainTaskHandle = NULL;


void app_main(void) {
    HC_SR04_init(&sensor_manager); 
    gpio_setup();

    // init ISR
    gpio_install_isr_service(ESP_INTR_FLAG_IRAM);
    gpio_isr_handler_add(GPIO_OUTPUT_IO_ECHO, gpio_isr_handler, (void*)&sensor_manager);

    // RTOS Config
    xTaskCreate(vMainTask, "MainTask", 2048, NULL, 1, &xMainTaskHandle);

    // Attempt to get the CPU clock frequency
    esp_err_t result = esp_clk_tree_src_get_freq_hz(SOC_MOD_CLK_CPU, ESP_CLK_TREE_SRC_FREQ_PRECISION_EXACT, &sensor_manager.frequency);

    if (result == ESP_OK) {
        // Successfully retrieved the frequency
        ESP_LOGI("CPU Freq", "CPU Frequency: %lu Hz", sensor_manager.frequency);
    } else {
        // Failed to retrieve the frequency
        ESP_LOGE("CPU Freq", "Failed to get CPU frequency, error: %d", result);
    }

    char cli_command[80];

    while(1) {
        if (scanf("%79s", cli_command) == 1) {  // Check if scanf successfully read a string
            if (strcmp(cli_command, "m") == 0) {
                printf("Sound barrier on! \n");
                if(xMainTaskHandle != NULL) {
                    vTaskResume(xMainTaskHandle); // Ensure xMainTaskHandle is initialized and points to a valid task
                }
            }
            else {
                printf("Invalid command\n");
            }
        }
        vTaskDelay(200 / portTICK_PERIOD_MS);
    } 
}

void vMainTask( void * pvParameters ) {

    gpio_set_level(GPIO_OUTPUT_IO_TRIGGER, 1); 
    float distance;
    vTaskSuspend(NULL);

    while(1)
    {
        do {
            distance = getDist();
        }
        while (distance > 1.0 || distance < 0.0001);
        printf("TRIGGER\n");
        vTaskSuspend(NULL);
    }
}

float getDist() {
    HC_SR04_trigger(&sensor_manager);
    if (sensor_manager.state == TRIGGER_SENT) {
            if (xSemaphoreTake(sensor_manager.semaphore,50) == pdTRUE) {
                int time_ticks;
                if (sensor_manager.time_stop < sensor_manager.time_start) {
                    // cpu cycle count overflow case protection
                    time_ticks = 4294967296 - 1 + sensor_manager.time_stop - sensor_manager.time_start; // 2^32 = 4294967296
                }
                else {
                    time_ticks = sensor_manager.time_stop - sensor_manager.time_start;
                }
                float time_sec = (float) time_ticks / sensor_manager.frequency;
                float distance = 343.0/2 * time_sec;
                return distance;
            }
            else {return 0.0;}
    }
    else {return 0.0;}
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