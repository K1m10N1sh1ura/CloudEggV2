#include "stdio.h"
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

// Function Prototypes
static void IRAM_ATTR gpio_isr_handler(void* arg);
HC_SR04_ECHO trigger_HC_SR04();
float getDist();

// RTOS Handles
SemaphoreHandle_t xSemaphoreISR = NULL;
void vMainTask( void * pvParameters );

// UART Configuration
const uart_port_t uart_num = UART_NUM_2;
uart_config_t uart_config = {
    .baud_rate = 115200,
    .data_bits = UART_DATA_8_BITS,
    .parity = UART_PARITY_DISABLE,
    .stop_bits = UART_STOP_BITS_1,
    .flow_ctrl = UART_HW_FLOWCTRL_CTS_RTS,
    .rx_flow_ctrl_thresh = 122,
};
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

    // Configure UART parameters
    ESP_ERROR_CHECK(uart_param_config(uart_num, &uart_config));
    // Set UART pins(TX: IO17 (UART2 default), RX: IO16 (UART2 default), RTS: IO18, CTS: IO19)
    ESP_ERROR_CHECK(uart_set_pin(UART_NUM_2, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE, 18, 19));
    // init ISR
    gpio_install_isr_service(0);
    gpio_isr_handler_add(GPIO_OUTPUT_IO_ECHO, gpio_isr_handler, (void*) GPIO_OUTPUT_IO_ECHO);
    
    // RTOS Config
    xTaskCreate(vMainTask, "MainTask", 2048, NULL, 1, NULL);
    xSemaphoreISR = xSemaphoreCreateBinary();

    while(1) {
        // Setze den GPIO-Pin auf HIGH
        gpio_set_level(GPIO_NUM_2, 0);

        // Beispielhaft kann hier eine Pause von einigen Sekunden eingefügt werden
        vTaskDelay(1000 / portTICK_PERIOD_MS);

        // Setze den GPIO-Pin auf LOW
        gpio_set_level(GPIO_NUM_2, 1);

        vTaskDelay(1000 / portTICK_PERIOD_MS);
    } 
}

void vMainTask( void * pvParameters ) {

    // Set GPIO_OUTPUT_IO_TRIGGER high
    gpio_set_level(GPIO_OUTPUT_IO_TRIGGER, 1); 
    float distance;

    uint32_t freq_value;
    // Attempt to get the CPU clock frequency
    esp_err_t result = esp_clk_tree_src_get_freq_hz(SOC_MOD_CLK_CPU, ESP_CLK_TREE_SRC_FREQ_PRECISION_EXACT, &freq_value);

    if (result == ESP_OK) {
        // Successfully retrieved the frequency
        ESP_LOGI("CPU Freq", "CPU Frequency: %lu Hz", freq_value);
    } else {
        // Failed to retrieve the frequency
        ESP_LOGE("CPU Freq", "Failed to get CPU frequency, error: %d", result);
    }

    while(1)
    {
        distance = getDist(freq_value);
        if (distance > 0.0)
        {
            printf("Distanz = %f\n", distance);
        }
        else {
            printf("ÄH = %f\n", distance);
        }

        vTaskDelay(1000 / portTICK_PERIOD_MS);
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

float getDist(uint32_t freq_value) {
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