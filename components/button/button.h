/**
* @file button.h

* @brief See the source file.

* @par
*
* COPYRIGHT NOTICE: (c) 2022 Byte Lab Grupa d.o.o.
* All rights reserved.
*/

#ifndef __BUTTON_H__
#define __BUTTON_H__

#ifdef __cplusplus
extern "C" {
#endif

//--------------------------------- INCLUDES ----------------------------------
#include <stdio.h>
#include "platform/inc/button_gpio.h"
//---------------------------------- MACROS -----------------------------------

//-------------------------------- DATA TYPES ---------------------------------
typedef void (*button_pressed_t)(void *);

typedef enum {
    BUTTON_1,
    BUTTON_2,
    BUTTON_3,
    BUTTON_4,

    BUTTON_COUNT
} button_id_t;

typedef enum {
    BUTTON_ERR_NONE = 0,

    BUTTON_ERR                = -1,
    BUTTON_ERR_CREATE         = -2,
    BUTTON_ERR_UNKNOWN_BUTTON = -3,
} button_err_t;
//---------------------- PUBLIC FUNCTION PROTOTYPES --------------------------

/**
 * @brief It creates a button object based on the button type and initializes it.
 * 
 * @param [in] btn_id  The id of the button to create.
 * @param [in] p_btn_cb This is the callback function that will be called when the button is pressed.
 * 
 * @return Status of creation.
 */
button_err_t button_create(button_id_t btn_id, button_pressed_t p_btn_cb);

#ifdef __cplusplus
}
#endif

#endif // __BUTTON_H__
