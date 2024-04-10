#include "stdio.h"
#include "string.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/uart.h"
#include "esp_system.h"
#include "esp_log.h"
#include "esp_clk_tree.h"

// Defines
#define GPIO_OUTPUT_IO_2     2
#define GPIO_OUTPUT_IO_ECHO    33
#define GPIO_OUTPUT_IO_TRIGGER    25

// Enums
typedef enum {
    NO_ECHO,
    MEASUREMENT_RUNNING,
    TRIGGER_SENT,
    ECHO
} HC_SR04_ECHO;

// Global Variables
volatile int HC_SR04_done = 0;
volatile int32_t time_start;
volatile int32_t time_stop;
volatile bool running_measurement = false;
volatile bool echo_started = false;
volatile bool echo_stoped  = false;
volatile uint32_t freq_value;

// Function Prototypes
static void IRAM_ATTR gpio_isr_handler(void* arg);
HC_SR04_ECHO trigger_HC_SR04();
float getDist();

// RTOS Handles
SemaphoreHandle_t xSemaphoreISR = NULL;
TaskHandle_t xMainTaskHandle = NULL;

void vMainTask( void * pvParameters );

// GPIO Configuration
gpio_config_t led_gpio = {
    // Trigger Pin Output
    .intr_type = GPIO_INTR_DISABLE,             // disable interrupt
	.mode = GPIO_MODE_OUTPUT,                   // set as output mode
	.pin_bit_mask = (1ULL << GPIO_OUTPUT_IO_2), // GPIO 2
	.pull_down_en = GPIO_PULLDOWN_DISABLE,      // disable pull-down mode
	.pull_up_en = GPIO_PULLUP_DISABLE,          // disable pull-up mode
};
gpio_config_t io_conf_trigger = {
    // Trigger Pin Output
    .intr_type = GPIO_INTR_DISABLE,         // disable interrupt
	.mode = GPIO_MODE_OUTPUT,               // set as output mode
	.pin_bit_mask = (1ULL<<GPIO_OUTPUT_IO_TRIGGER),             // GPIO D33
	.pull_down_en = GPIO_PULLDOWN_DISABLE,  // disable pull-down mode
	.pull_up_en = GPIO_PULLUP_DISABLE,      // disable pull-up mode
};
gpio_config_t io_conf_echo = {
    // Echo Pin Input
    .intr_type = GPIO_INTR_ANYEDGE,          // enable interrupt
	.mode = GPIO_MODE_INPUT,                 // set as input mode
	.pin_bit_mask = (1ULL<<GPIO_OUTPUT_IO_ECHO),               // GPIO D25
	.pull_down_en = GPIO_PULLDOWN_ENABLE,    // disable pull-down mode
	.pull_up_en = GPIO_PULLUP_DISABLE,       // enable pull-up mode
};

void app_main(void) {
    // Configure GPIOs
    ESP_ERROR_CHECK(gpio_config(&led_gpio));
    ESP_ERROR_CHECK(gpio_config(&io_conf_trigger));
    ESP_ERROR_CHECK(gpio_config(&io_conf_echo));
    // init ISR
    gpio_install_isr_service(0);
    gpio_isr_handler_add(GPIO_OUTPUT_IO_ECHO, gpio_isr_handler, (void*) GPIO_OUTPUT_IO_ECHO);
    
    // RTOS Config
    xTaskCreate(vMainTask, "MainTask", 2048, NULL, 1, &xMainTaskHandle);
    xSemaphoreISR = xSemaphoreCreateBinary();

    // Attempt to get the CPU clock frequency
    esp_err_t result = esp_clk_tree_src_get_freq_hz(SOC_MOD_CLK_CPU, ESP_CLK_TREE_SRC_FREQ_PRECISION_EXACT, &freq_value);

    if (result == ESP_OK) {
        // Successfully retrieved the frequency
        ESP_LOGI("CPU Freq", "CPU Frequency: %lu Hz", freq_value);
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

HC_SR04_ECHO trigger_HC_SR04() {
    if (running_measurement == true) {
        return MEASUREMENT_RUNNING;
    }
    else {
        gpio_set_level(GPIO_OUTPUT_IO_TRIGGER, 0); 
        esp_rom_delay_us(10); 
        gpio_set_level(GPIO_OUTPUT_IO_TRIGGER, 1);
        return TRIGGER_SENT;
    }
}

float getDist() {
    if (trigger_HC_SR04() == TRIGGER_SENT) {
            if (xSemaphoreTake(xSemaphoreISR,20) == pdTRUE) {
                int time_ticks;
                if (time_stop < time_start) {
                    // cpu cycle count overflow case protection
                    time_ticks = 4294967296 - 1 + time_stop - time_start; // 2^32 = 4294967296
                }
                else {
                    time_ticks = time_stop - time_start;
                }
                float time_sec = (float) time_ticks / freq_value;
                float distance = 343.0/2 * time_sec;
                return distance;
            }
            else {return 0.0;}
    }
    else {return 0.0;}
}

static void IRAM_ATTR gpio_isr_handler(void* arg) {   
    // HC_SR04 Driver
    if (gpio_get_level(GPIO_OUTPUT_IO_ECHO) == 1 && !running_measurement && !echo_started) {
        time_start = esp_cpu_get_cycle_count();
        running_measurement = true;
        echo_started = true;
    }
    else if (gpio_get_level(GPIO_OUTPUT_IO_ECHO) == 0 && running_measurement == true && echo_started == true) {
        time_stop = esp_cpu_get_cycle_count();
        static BaseType_t xHigherPriorityTaskWoken = pdFALSE;
        running_measurement = false;
        echo_started = false;
        echo_stoped = true;
        xSemaphoreGiveFromISR(xSemaphoreISR,&xHigherPriorityTaskWoken);
    }
    else {
        running_measurement = false;
        echo_stoped = false;
        echo_started = false;
        time_start = 0;
        time_stop = 0;
    }
}