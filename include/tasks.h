// tasks.h

#ifndef TASKS_H
#define TASKS_H

#include "main.h"

#define MAX_DISTANCE_TO_OBJECT 1.0 // in meters

// Structs
typedef struct {
    int64_t current_time_us;
    bool running_measurement;
} Meastask_Manager;

// Global vars
extern Meastask_Manager meastask_manager;
extern HC_SR04_Manager hc_sr04_manager;

// Task Function Prototypes
void vAcousticBarrierTask( void * pvParameters );
void vCliTask( void * pvParameters );
void gpio_isr_handler(void* arg);
void cli_menu();

#endif // TASKS_H