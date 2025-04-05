/**
 * @file day_night_detector.h
 * @brief Day/night detection using VEML7700 ambient light sensor
 */
#ifndef DAY_NIGHT_DETECTOR_H
#define DAY_NIGHT_DETECTOR_H

#include "esp_err.h"
#include <stdbool.h>

/** @brief Light thresholds in lux */
#define NIGHT_THRESHOLD 10.0
#define DAY_THRESHOLD   50.0

/** @brief Event group bits */
#define DAY_MODE_BIT   BIT0
#define NIGHT_MODE_BIT BIT1

/**
 * @brief Light state enumeration
 */
typedef enum {
    LIGHT_STATE_UNKNOWN,
    LIGHT_STATE_DAY,
    LIGHT_STATE_NIGHT,
    LIGHT_STATE_TRANSITION
} light_state_t;

/**
 * @brief Initialize the day/night detector
 * 
 * @return esp_err_t ESP_OK on success
 */
esp_err_t day_night_init(void);

/**
 * @brief Task function for day/night detector operation
 * 
 * @param pvParameters FreeRTOS task parameters (not used)
 */
void day_night_task(void *pvParameters);

/**
 * @brief Check if currently in day mode
 * 
 * @return bool true if in day mode
 */
bool is_day_mode(void);

/**
 * @brief Check if currently in night mode
 * 
 * @return bool true if in night mode
 */
bool is_night_mode(void);

/**
 * @brief Get current light level in lux
 * 
 * @param lux Pointer to store lux value
 * @return esp_err_t ESP_OK on success
 */
esp_err_t get_light_level(double *lux);

/**
 * @brief Get current light state
 * 
 * @return light_state_t Current light state
 */
light_state_t get_light_state(void);

/**
 * @brief Register callback for light state changes
 * 
 * @param callback Function to call when light state changes
 */
void light_register_callback(void (*callback)(light_state_t state));

#endif /* DAY_NIGHT_DETECTOR_H */