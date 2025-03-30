/**
* @file button.c

* @brief Button implementation for Byte Lab Development Kit.

* @par
*
* COPYRIGHT NOTICE: (c) 2022 Byte Lab Grupa d.o.o.
* All rights reserved.
*/

//--------------------------------- INCLUDES ----------------------------------
#include "button.h"

//---------------------------------- MACROS -----------------------------------

//-------------------------------- DATA TYPES ---------------------------------
typedef struct {
    uint8_t pin;
    bool b_is_active_on_high_level;

} _button_config_t;

//---------------------- PRIVATE FUNCTION PROTOTYPES --------------------------

//------------------------- STATIC DATA & CONSTANTS ---------------------------
static const _button_config_t _button_info[BUTTON_COUNT] = {
    // GPIO BUTTONS
    { .pin = 36U, .b_is_active_on_high_level = true }, /* BUTTON_1 */
    { .pin = 32U, .b_is_active_on_high_level = true }, /* BUTTON_2 */
    { .pin = 33U, .b_is_active_on_high_level = true }, /* BUTTON_3 */
    { .pin = 25U, .b_is_active_on_high_level = true }, /* BUTTON_4 */
};
//------------------------------- GLOBAL DATA ---------------------------------

//------------------------------ PUBLIC FUNCTIONS -----------------------------

button_err_t button_create(button_id_t btn_id, button_pressed_t p_btn_cb) {
    /* Validate button name */
    if(BUTTON_COUNT <= btn_id) {
        return BUTTON_ERR_UNKNOWN_BUTTON;
    }

    button_gpio_t *p_button =
            button_gpio_create(_button_info[btn_id].pin, _button_info[btn_id].b_is_active_on_high_level, p_btn_cb);

    if(NULL == p_button) {
        return BUTTON_ERR_CREATE;
    }

    return BUTTON_ERR_NONE;
}
//---------------------------- PRIVATE FUNCTIONS ------------------------------

//---------------------------- INTERRUPT HANDLERS -----------------------------
