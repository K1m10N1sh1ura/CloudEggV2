#include "hc_sr04.h"


void HC_SR04_init(HC_SR04_Manager *manager) {
    manager->time_start = 0;
    manager->time_stop = 0;
    manager->running_measurement = false;
    manager->echo_started = false;
    manager->echo_stopped = false;
    manager->state = NO_ECHO;
    manager->semaphore = xSemaphoreCreateBinary();
    manager->frequency = 0;
}

void HC_SR04_trigger(HC_SR04_Manager *manager) {
    
    if (!manager->running_measurement) {
        gpio_set_level(GPIO_OUTPUT_IO_TRIGGER, 0);
        esp_rom_delay_us(10);
        gpio_set_level(GPIO_OUTPUT_IO_TRIGGER, 1);
        manager->state = TRIGGER_SENT;
    }
    else {
        manager->state = MEASUREMENT_RUNNING;
    }
}

float getDist(HC_SR04_Manager *manager) {
    HC_SR04_trigger(manager);
    if (manager->state == TRIGGER_SENT) {
            if (xSemaphoreTake(manager->semaphore,50) == pdTRUE) {
                int time_ticks;
                if (manager->time_stop < manager->time_start) {
                    // cpu cycle count overflow case protection
                    time_ticks = 4294967296 - 1 + manager->time_stop - manager->time_start; // 2^32 = 4294967296
                }
                else {
                    time_ticks = manager->time_stop - manager->time_start;
                }
                float time_sec = (float) time_ticks / manager->frequency;
                float distance = 343.0/2 * time_sec;
                return distance;
            }
            else {return 0.0;}
    }
    else {return 0.0;}
}