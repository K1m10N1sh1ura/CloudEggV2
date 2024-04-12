// main.h

#ifndef MAIN_H
#define MAIN_H

#include "string.h"
#include "freertos/FreeRTOS.h"
#include "driver/gpio.h"
#include "gpio_pins.h"
#include "gpio_config.h"
#include "hc_sr04.h"
#include "tasks.h"
#include "wifi.h"
#include "server.h"

// Task Function Prototypes
void vAcousticBarrierTask( void * pvParameters );
void vCliTask( void * pvParameters );
static void gpio_isr_handler(void* arg);
void cli_menu();

#endif // MAIN_H