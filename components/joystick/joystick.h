/**
 * @file joystick.h
 *
 * @brief See the source file.
 *
 */

#ifndef JOYSTICK_H
#define JOYSTICK_H

#ifdef __cplusplus
extern "C" {
#endif

//--------------------------------- INCLUDES ----------------------------------
#include "esp_err.h"

//---------------------------------- MACROS -----------------------------------

//-------------------------------- DATA TYPES ---------------------------------

/**
 * @brief Joystick directional input states.
 */
enum joystick_pos_t {
    INPUT_PUSH_BUTTON, /**< Center button press (if available). */
    INPUT_UP,          /**< Joystick moved up. */
    INPUT_DOWN,        /**< Joystick moved down. */
    INPUT_RIGHT,       /**< Joystick moved right. */
    INPUT_LEFT,        /**< Joystick moved left. */
    INPUT_CENTER       /**< Joystick in neutral/center position. */
};

/**
 * @brief Callback type for joystick position changes.
 *
 * This function will be called whenever the joystick position changes,
 * if a callback is registered via joystick_register_callback().
 *
 * @param pos The new joystick position.
 */
typedef void (*joystick_callback_t)(enum joystick_pos_t pos);

//---------------------- PUBLIC FUNCTION PROTOTYPES --------------------------

/**
 * @brief Initializes the joystick driver.
 *
 * Configures ADC channels and performs calibration.
 *
 * @return ESP_OK on success.
 */
esp_err_t joystick_init(void);

/**
 * @brief Gets the current joystick position.
 *
 * If the position changes, the registered callback is invoked (if any),
 * and logging is optionally performed.
 *
 * @return Current joystick position.
 */
enum joystick_pos_t joystick_get_position(void);

/**
 * @brief Registers a callback to be called on position change.
 *
 * @param cb Callback function of type joystick_callback_t.
 */
void joystick_register_callback(joystick_callback_t cb);

#ifdef __cplusplus
}
#endif

#endif // JOYSTICK_H