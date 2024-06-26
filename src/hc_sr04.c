#include "hc_sr04.h"

#define TRIGGER_LOW_DELAY 4
#define TRIGGER_HIGH_DELAY 10

HC_SR04_Manager hc_sr04_manager;
CloudEgg_Position_Manager pos_manager;

void HC_SR04_init(HC_SR04_Manager *manager) {
    manager->time_start = 0;
    manager->time_stop = 0;
    manager->running_measurement = false;
    manager->echo_started = false;
    manager->echo_stopped = false;
    manager->state = NO_ECHO;
    manager->semaphore = xSemaphoreCreateBinary();

    // Attempt to get the CPU clock frequency
    esp_err_t result = esp_clk_tree_src_get_freq_hz(SOC_MOD_CLK_CPU, ESP_CLK_TREE_SRC_FREQ_PRECISION_EXACT, &manager->frequency);

    if (result == ESP_OK) {
        // Successfully retrieved the frequency
        ESP_LOGI("CPU Freq", "CPU Frequency: %lu Hz", (unsigned long) manager->frequency);
    } else {
        // Failed to retrieve the frequency
        ESP_LOGE("CPU Freq", "Failed to get CPU frequency, error: %d", result);
    }
}

void HC_SR04_trigger(HC_SR04_Manager *manager) {
    // measurement is triggered by pulling down trigger pin for 10 microseconds
    if (!manager->running_measurement) {
        // Ping: Low for 2..4 us, then high 10 us
        gpio_set_level(GPIO_OUTPUT_IO_TRIGGER, 0);
        esp_rom_delay_us(TRIGGER_LOW_DELAY);
        gpio_set_level(GPIO_OUTPUT_IO_TRIGGER, 1);
        esp_rom_delay_us(TRIGGER_HIGH_DELAY);
        gpio_set_level(GPIO_OUTPUT_IO_TRIGGER, 0);
        manager->state = TRIGGER_SENT;
    }
    else {
        manager->state = MEASUREMENT_RUNNING;
    }
}

float getDist(HC_SR04_Manager *manager) {
    HC_SR04_trigger(manager);
    if (manager->state == TRIGGER_SENT) {
            if (xSemaphoreTake(manager->semaphore,10) == pdTRUE) {
                // wait for signal from ISR (via semaphore) that indicates a finished measurement 
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
            else {return -1.0;} // no echo received in max waiting time of 10 ms
    }
    else {return -2.0;} // some kind of error
}

void CloudEgg_position_manager_init(CloudEgg_Position_Manager *manager) {
    manager->avg_distance_to_next_object = 0;
    manager->distance_std_dev = 0;
    manager->measurements_per_second = 0;
    manager->errorDetections = 0;
    manager->sensor_pos_Quality = UNKNOWN;
}

void CloudEgg_validate_sensor_pos(CloudEgg_Position_Manager *manager) {
    
    if (!meastask_manager.running_measurement) {
        CloudEgg_position_manager_init(manager);
        meastask_manager.running_measurement = true;
        const int num_measurements = 100;
        float elapsed_time_s;
        int64_t time_start_us;
        int64_t time_stop_us;
        struct timeval tv_start, tv_stop;
        // Allocate memory for an array of 100 floats
        float *distances = (float*)malloc(100 * sizeof(float));
    
        printf("Start validation of Sensorposition!\n");

        if (distances == NULL) {
            printf("Memory allocation failed.\n");
            return;
        }

        // Record the start time
        gettimeofday(&tv_start, NULL);
        time_start_us = (int64_t)tv_start.tv_sec * 1000000L + (int64_t)tv_start.tv_usec;
        for (int i = 0; i < num_measurements; i++) {
            distances[i] = getDist(&hc_sr04_manager);
        }
        gettimeofday(&tv_stop, NULL);
        time_stop_us = (int64_t)tv_stop.tv_sec * 1000000L + (int64_t)tv_stop.tv_usec;
        elapsed_time_s = (float) (time_stop_us - time_start_us) / (float) 1e6;
        manager->avg_distance_to_next_object = floatAvg(distances, num_measurements);
        manager->measurements_per_second = (float) num_measurements / elapsed_time_s;
        manager->distance_std_dev = floatStdDev(distances, num_measurements);
        for (int i = 0; i < num_measurements; i++) {
            if (distances[i] < 0) {
                manager->errorDetections++;
            }
        }

        if (manager->avg_distance_to_next_object < 5.0 &&
                manager->avg_distance_to_next_object > 1.2 && 
                manager->measurements_per_second > 30.0 &&
                manager->distance_std_dev < 0.3 && 
                manager->errorDetections < 2) {

            manager->sensor_pos_Quality = IDEAL_POSITION;
        }
        else if (manager->avg_distance_to_next_object < 4.0 && 
                    manager->distance_std_dev > 0.3) {
            manager->sensor_pos_Quality = OBJECT_INTERFERENCE;
        }
        else if(manager->avg_distance_to_next_object < 1.1) {
            manager->sensor_pos_Quality = CLOSE_OBJECT_INTERFERENCE;
        }
        else {
            manager->sensor_pos_Quality = SUBOPTIMAL_POSITION;
        }

        meastask_manager.running_measurement = false;
        free(distances);

    }
    else {
        printf("Position validation failed, sensor in use!\n");
    }
}

float floatAvg(float *array, int length) {
    float sum = 0.0;
    for (int i = 0; i < length; i++) {
        sum += array[i];
    }
    
    // Calculate the average
    return sum / (float) length;
}

float floatStdDev(float *array, int length) {
    float mean = floatAvg(array, length);
    // Calculate the squared differences between each value and the mean
    float squaredDifferencesSum = 0.0;
    for (int i = 0; i < length; i++) {
        float difference = array[i] - mean;
        squaredDifferencesSum += difference * difference;
    }
    
    // Calculate the mean of the squared differences
    float meanSquaredDifferences = squaredDifferencesSum / (float) length;

    // Calculate the standard deviation
    return sqrt(meanSquaredDifferences);
}

const char* sensorPosQualityToString(SENSOR_POS_QUALITY sensor_pos_quality) {
    switch(sensor_pos_quality) {
        case IDEAL_POSITION: return "Ideal postion";
        case OBJECT_INTERFERENCE: return "Object interference";
        case CLOSE_OBJECT_INTERFERENCE: return "Close object interference";
        case SUBOPTIMAL_POSITION: return "Suboptimal position";
        default: return "UNKNOWN"; // Handle unknown enum values
    }
}

void printSensorValidationResult(CloudEgg_Position_Manager *manager) {
        printf("AvgDistance: %.2f,\n"
            "MeasPerSec: %.2f,\n"
            "StdDev: %.2f,\n"
            "ErrDetections: %d,\n"
            "PosQual: %s\n", 
            manager->avg_distance_to_next_object,
            manager->measurements_per_second, 
            manager->distance_std_dev, 
            manager->errorDetections,
            sensorPosQualityToString(manager->sensor_pos_Quality));
}