/**
 * @file led.h
 *
 * @brief See the source file.
 * 
 */

#ifndef LED_H
#define LED_H

#ifdef __cplusplus
extern "C" {
#endif

//--------------------------------- INCLUDES ----------------------------------
#include "esp_err.h"

//---------------------------------- MACROS -----------------------------------

//-------------------------------- DATA TYPES ---------------------------------
/**
  * @brief Enums hold all board LEDs.
  *
  */
typedef enum {
    LED_BLUE,
    LED_RED,
    LED_GREEN,

    LED_COUNT
} led_t;

//---------------------- PUBLIC FUNCTION PROTOTYPES --------------------------
/**
  * @brief The function initializes LED.
  *
  * @param [in] led LED instance (e.g. LED_BLUE).
  *
  * @return esp_err_t ESP_OK on success, fail otherwise.
  */
esp_err_t led_init(led_t led);

/**
  * @brief The function turns LED on.
  *
  * @param [in] led LED instance (e.g. LED_BLUE).
  *
  * @return esp_err_t ESP_OK on success, fail otherwise.
  */
esp_err_t led_on(led_t led);

/**
  * @brief The function turns LED off.
  *
  * @param [in] led LED instance (e.g. LED_BLUE).
  *
  * @return esp_err_t ESP_OK on success, fail otherwise.
  */
esp_err_t led_off(led_t led);

#ifdef __cplusplus
}
#endif

#endif // LED_H