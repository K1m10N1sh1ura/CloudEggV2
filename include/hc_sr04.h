// hc_sr04.h

#ifndef HC_SR04_H
#define HC_SR04_H

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "gpio_pins.h"
#include "driver/gpio.h"
#include "esp_clk_tree.h"
#include "esp_log.h"
#include "math.h"
#include "tasks.h"
#include <sys/time.h>


// Enums zur Darstellung des Sensorzustandes
typedef enum {
    NO_ECHO,
    MEASUREMENT_RUNNING,
    TRIGGER_SENT,
    ECHO_RECEIVED
} HC_SR04_ECHO;

typedef enum {
    IDEAL_POSITION,                 // Optimal position des Sensors
    OBJECT_INTERFERENCE,            // Zu viele Objekte stören den Sensorbereich
    CLOSE_OBJECT_INTERFERENCE,      // Sensor steht zu nah an einem Objekt
    SUBOPTIMAL_POSITION,            // Semi-optimale Position des Sensors (kein Objekt im Bereich)
    UNKNOWN
} SENSOR_POS_QUALITY;

// Structs
typedef struct {
    float avg_distance_to_next_object; // 
    float distance_std_dev; // high standard deviation means lots of different objects seem to be in the range of the sensor which is bad
    float measurements_per_second; //
    int errorDetections;
    SENSOR_POS_QUALITY sensor_pos_Quality;
} CloudEgg_Position_Manager;

// manage HC-SR04 state
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

// Global vars
extern HC_SR04_Manager hc_sr04_manager;

// Initialisierung der Struktur mit Default-Werten
void HC_SR04_init(HC_SR04_Manager *manager);
// Beispiel einer Funktion, die den Sensorzustand verwendet
void HC_SR04_trigger(HC_SR04_Manager *manager);
// start single shot distance measurement
float getDist(HC_SR04_Manager *manager);

void CloudEgg_position_manager_init(CloudEgg_Position_Manager *manager);
void CloudEgg_validate_sensor_pos(CloudEgg_Position_Manager *manager);
float floatAvg(float *array, int length);
float floatStdDev(float *array, int length);

#endif // HC_SR04_H
