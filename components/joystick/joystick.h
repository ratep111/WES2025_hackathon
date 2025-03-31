#ifndef JOYSTICK_H
#define JOYSTICK_H

#include "esp_err.h"

enum joystick_pos_t {
    INPUT_PUSH_BUTTON,
    INPUT_UP,
    INPUT_DOWN,
    INPUT_RIGHT,
    INPUT_LEFT,
    INPUT_CENTER
};

esp_err_t joystick_init(void);
enum joystick_pos_t get_joystick_position(void);

#endif