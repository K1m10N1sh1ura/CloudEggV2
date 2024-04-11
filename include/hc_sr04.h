// hc_sr04.h

#ifndef HC_SR04_H
#define HC_SR04_H

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

// Enums zur Darstellung des Sensorzustandes
typedef enum {
    NO_ECHO,
    MEASUREMENT_RUNNING,
    TRIGGER_SENT,
    ECHO_RECEIVED
} HC_SR04_ECHO;

// Struktur zur Verwaltung des HC-SR04 Zustandes
typedef struct {
    int32_t time_start;
    int32_t time_stop;
    bool running_measurement;
    bool echo_started;
    bool echo_stopped;
    HC_SR04_ECHO state;
    SemaphoreHandle_t semaphore;
    uint32_t frequency;
} HC_SR04_Manager;

// Initialisierung der Struktur mit Default-Werten
void HC_SR04_init(HC_SR04_Manager *manager);
// Beispiel einer Funktion, die den Sensorzustand verwendet
void HC_SR04_trigger(HC_SR04_Manager *manager);

#endif // HC_SR04_H
