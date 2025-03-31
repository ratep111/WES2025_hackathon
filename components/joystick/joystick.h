#ifndef JOYSTICK_H
#define JOYSTICK_H

//--------------------------------- INCLUDES ----------------------------------
#include "esp_err.h"

//---------------------------------- MACROS -----------------------------------

//-------------------------------- DATA TYPES ---------------------------------
enum joystick_pos_t {
    INPUT_PUSH_BUTTON,
    INPUT_UP,
    INPUT_DOWN,
    INPUT_RIGHT,
    INPUT_LEFT,
    INPUT_CENTER
};

typedef void (*joystick_callback_t)(enum joystick_pos_t pos);

//---------------------- PUBLIC FUNCTION PROTOTYPES --------------------------

esp_err_t joystick_init(void);
enum joystick_pos_t joystick_get_position(void);
void joystick_register_callback(joystick_callback_t cb);

#endif // JOYSTICK_H