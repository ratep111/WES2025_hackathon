/**
* @file button_gpio.h

* @brief See the source file.

* @par
*
* COPYRIGHT NOTICE: (c) 2022 Byte Lab Grupa d.o.o.
* All rights reserved.
*/

#ifndef __BUTTON_GPIO_H__
#define __BUTTON_GPIO_H__

#ifdef __cplusplus
extern "C" {
#endif

//--------------------------------- INCLUDES ----------------------------------
#include <stdio.h>
#include <stdbool.h>
//---------------------------------- MACROS -----------------------------------

//-------------------------------- DATA TYPES ---------------------------------

typedef void (*btn_gpio_pressed_t)(void *);

// Struct is hidden on purpose from an user.
struct _button_gpio_t;
typedef struct _button_gpio_t button_gpio_t;

//---------------------- PUBLIC FUNCTION PROTOTYPES --------------------------

/**
 * @brief It creates a button object and initializes it.
 *
 * @param [in] pin The GPIO pin number that the button is connected to.
 * @param [in] b_is_active_on_high_level If true, the button is active when the GPIO is high. If false, the
 * button is active when the GPIO is low.
 * @param [in] p_button_pressed_cb This is the callback function that will be called when the button is
 * pressed.
 *
 * @return A pointer to a button_gpio_t struct.
 */
button_gpio_t *button_gpio_create(uint8_t pin, bool b_is_active_on_high_level, btn_gpio_pressed_t p_button_pressed_cb);

/**
 * @brief This function frees the memory allocated for the button_gpio_t structure.
 *
 * @param [in] p_btn A pointer to the button_gpio_t structure that was created by the button_gpio_create()
 * function.
 */
void button_gpio_delete(button_gpio_t *p_btn);

/**
 * @brief This function checks if the button is pressed.
 * 
 * @param [in] p_btn A pointer to the button_gpio_t structure that was created in the previous step.
 * 
 * @return True if the button is pressed, false otherwise.
 */
bool button_gpio_is_pressed(button_gpio_t *p_btn);

#ifdef __cplusplus
}
#endif

#endif // __BUTTON_GPIO_H__
