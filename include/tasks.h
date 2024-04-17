// tasks.h

#ifndef TASKS_H
#define TASKS_H

#include "freertos/FreeRTOS.h"
#include "hc_sr04.h"
#include "string.h"


#define MAX_DISTANCE_TO_OBJECT 1.0 // in meters

typedef struct {
    int64_t current_time_us;
    bool running_measurement;
} Meastask_Manager;

// Global vars
extern Meastask_Manager meastask_manager;

// RTOS Handles
extern TaskHandle_t xAcousticBarrierTaskHandle;
extern TaskHandle_t xCliTaskHandle;

// Task Function Prototypes
void vAcousticBarrierTask( void * pvParameters );
void vCliTask( void * pvParameters );
void gpio_isr_handler(void* arg);
void cli_menu();
void validateCloudEggPosition();

#endif // TASKS_H