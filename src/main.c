/*
 * File: main.c
 * Author: Kimio Nishiura
 * Description: Init and setup functions for CloudEggV2 project (GPIOs, ISRs, RTOS, wifi, webserver).
 *              
 * Created on: January 1, 2024
 */
#include "main.h"

void app_main(void) {
    // Set up GPIO
    gpio_setup();

    // Install ISR service and add handler for echo pin
    gpio_install_isr_service(ESP_INTR_FLAG_IRAM);
    gpio_isr_handler_add(GPIO_OUTPUT_IO_ECHO, hcsr04_gpio_isr_handler, (void*)&hc_sr04_manager);

    // Create RTOS tasks
    xTaskCreate(vAcousticBarrierTask, "AcousticBarrierTask", 2048, NULL, 1, &xAcousticBarrierTaskHandle);
    xTaskCreate(vCliTask, "CliTask", 2048, NULL, 2, &xCliTaskHandle);

    // Initialize WiFi
    wifi_init();

    // Initialize server
    server_init();

    while (1) {
        vTaskDelay(1e6 / portTICK_PERIOD_MS); 
    }
}
