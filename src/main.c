#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include <driver/uart.h>

// Defines
#define GPIO_OUTPUT_IO_2    2

// RTOS Handles
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

void app_main(void) {
    // Configure GPIOs
    ESP_ERROR_CHECK(gpio_config(&led_gpio));
    // Configure UART parameters
    ESP_ERROR_CHECK(uart_param_config(uart_num, &uart_config));
    // Set UART pins(TX: IO17 (UART2 default), RX: IO16 (UART2 default), RTS: IO18, CTS: IO19)
    ESP_ERROR_CHECK(uart_set_pin(UART_NUM_2, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE, 18, 19));

    // RTOS Config
    xTaskCreate(vMainTask, "MainTask", 2048, NULL, 1, NULL);

    while(1) {
        // Setze den GPIO-Pin auf HIGH
        gpio_set_level(GPIO_NUM_2, 0);

        // Beispielhaft kann hier eine Pause von einigen Sekunden eingefügt werden
        vTaskDelay(1000 / portTICK_PERIOD_MS);

        // Setze den GPIO-Pin auf LOW
        gpio_set_level(GPIO_NUM_2, 1);

        vTaskDelay(1000 / portTICK_PERIOD_MS);

        printf("[HELLO]\n");
    } 
}

void vMainTask( void * pvParameters ) {

    while(1) {

        vTaskDelay(10000000 / portTICK_PERIOD_MS);
    }

}


