#ifndef SPEED_ESTIMATOR_H
#define SPEED_ESTIMATOR_H

#include "esp_err.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Movement direction enumeration
 */
typedef enum {
    DIRECTION_UNKNOWN,
    DIRECTION_FORWARD,
    DIRECTION_BACKWARD,
    DIRECTION_LEFT,
    DIRECTION_RIGHT
} movement_direction_t;

/**
 * @brief Initialize the speed estimator.
 *
 * @return esp_err_t ESP_OK on success
 */
esp_err_t speed_estimator_init(void);

/**
 * @brief Task function that runs speed estimation from accelerometer.
 */
void speed_estimator_task(void *args);

/**
 * @brief Get the latest speed estimate in m/s
 *
 * @return float current speed estimate
 */
float speed_estimator_get_speed_mps(void);

/**
 * @brief Get the latest speed estimate in km/h
 *
 * @return float current speed estimate
 */
float speed_estimator_get_speed_kmh(void);

/**
 * @brief Get the current movement direction
 *
 * @return movement_direction_t The detected movement direction
 */
movement_direction_t speed_estimator_get_direction(void);

/**
 * @brief Check if movement is in forward direction
 *
 * @return bool true if moving forward
 */
bool speed_estimator_is_moving_forward(void);

/**
 * @brief Check if movement is in backward direction
 *
 * @return bool true if moving backward
 */
bool speed_estimator_is_moving_backward(void);

/**
 * @brief Get direction as string for display/logging
 *
 * @return const char* Direction string ("Forward", "Backward", etc.)
 */
const char *speed_estimator_get_direction_string(void);

#ifdef __cplusplus
}
#endif

#endif // SPEED_ESTIMATOR_H