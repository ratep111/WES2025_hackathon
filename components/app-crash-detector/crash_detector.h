/**
 * @file crash_detector.h
 * @brief Impact/crash detection using accelerometer
 */
#ifndef CRASH_DETECTOR_H
#define CRASH_DETECTOR_H

#include "esp_err.h"
#include <stdbool.h>
#include <time.h>

/** @brief Impact threshold in g-forces */
#define CRASH_ACCEL_THRESHOLD  4.0f // Default: 4g force
#define CRASH_RESET_TIMEOUT_MS 5000 // Auto reset crash state after 5 seconds

/**
 * @brief Crash event data structure
 */
typedef struct {
    float impact_force;     // Force of impact in g
    time_t timestamp;       // Time of crash detection
    char timestamp_str[32]; // Human-readable timestamp
} crash_event_t;

/**
 * @brief Initialize the crash detector
 * 
 * @return esp_err_t ESP_OK on success
 */
esp_err_t crash_detector_init(void);

/**
 * @brief Task function for crash detection
 * 
 * @param pvParameters FreeRTOS task parameters (not used)
 */
void crash_detector_task(void *pvParameters);

/**
 * @brief Check if a crash has been detected
 * 
 * @return bool true if a crash was detected
 */
bool crash_detector_is_crashed(void);

/**
 * @brief Get the most recent crash event data
 * 
 * @param event Pointer to store crash event data
 * @return bool true if valid crash data is available
 */
bool crash_detector_get_last_event(crash_event_t *event);

/**
 * @brief Reset crash detection state
 */
void crash_detector_reset(void);

/**
 * @brief Set crash detection threshold
 * 
 * @param threshold Threshold in g-forces above which a crash is detected
 */
void crash_detector_set_threshold(float threshold);

/**
 * @brief Register callback for crash events
 * 
 * @param callback Function to call when crash is detected
 */
void crash_detector_register_callback(void (*callback)(crash_event_t *event));

#endif /* CRASH_DETECTOR_H */