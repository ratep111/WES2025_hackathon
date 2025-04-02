/**
 * @file buzzer.h
 * 
 * @brief Public API for buzzer driver.
 * 
 */

#ifndef BUZZER_H
#define BUZZER_H

#ifdef __cplusplus
extern "C" {
#endif

//--------------------------------- INCLUDES ----------------------------------
#include "esp_err.h"

//---------------------------------- MACROS -----------------------------------

//-------------------------------- DATA TYPES ---------------------------------

//---------------------- PUBLIC FUNCTION PROTOTYPES --------------------------

/**
 * @brief Initialize the buzzer using LEDC PWM.
 *
 * Configures the LEDC timer and channel for the buzzer GPIO.
 *
 * @return ESP_OK on success, or appropriate error code.
 */
esp_err_t buzzer_init(void);

/**
 * @brief Set buzzer duty cycle (volume/tone intensity).
 *
 * @param duty Duty cycle (0 to 2^PWM_RESOLUTION).
 * @return ESP_OK on success, or appropriate error code.
 */
esp_err_t buzzer_set_duty(uint32_t duty);

#ifdef __cplusplus
}
#endif

#endif // BUZZER_H