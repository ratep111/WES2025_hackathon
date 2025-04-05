/**
 * @file parking_sensor.h
 * @brief Parking sensor logic using HC-SR04 ultrasonic sensor
 */
#ifndef PARKING_SENSOR_H
#define PARKING_SENSOR_H

#include "esp_err.h"
#include <stdbool.h>

/** @brief Distance thresholds in cm */
#define DISTANCE_DANGER  30
#define DISTANCE_WARNING 80
#define DISTANCE_SAFE    150
#define MAX_DISTANCE     400

/**
 * @brief Initialize the parking sensor hardware
 * 
 * @return esp_err_t ESP_OK on success
 */
esp_err_t parking_sensor_init(void);

/**
 * @brief Task function for parking sensor operation
 * 
 * @param pvParameters FreeRTOS task parameters (not used)
 */
void parking_sensor_task(void *pvParameters);

/**
 * @brief Get the current distance reading in cm
 * 
 * @param distance Pointer to store distance value
 * @return esp_err_t ESP_OK on success
 */
esp_err_t parking_sensor_get_distance(uint32_t *distance);

/**
 * @brief Check if object is in danger zone
 * 
 * @return bool true if in danger zone (<30cm)
 */
bool parking_sensor_is_danger(void);

/**
 * @brief Check if object is in warning zone
 * 
 * @return bool true if in warning zone (30-80cm)
 */
bool parking_sensor_is_warning(void);

/**
 * @brief Check if object is in safe zone
 * 
 * @return bool true if in safe zone (>80cm)
 */
bool parking_sensor_is_safe(void);

#endif /* PARKING_SENSOR_H */