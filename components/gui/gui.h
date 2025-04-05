/**
* @file gui.h

* @brief See the source file.
* 
* COPYRIGHT NOTICE: (c) 2022 Byte Lab Grupa d.o.o.
* All rights reserved.
*/

#ifndef __GUI_H__
#define __GUI_H__

#ifdef __cplusplus
extern "C" {
#endif

//--------------------------------- INCLUDES ----------------------------------
#include <stdint.h>
//---------------------------------- MACROS -----------------------------------
#define GUI_SPEED_BUFF_SIZE (4)
#define GUI_SPEED_LOW       (50)
#define GUI_SPEED_MID       (100)
#define GUI_SPEED_HIGH      (120)

//-------------------------------- DATA TYPES ---------------------------------

//---------------------- PUBLIC FUNCTION PROTOTYPES --------------------------

/**
 * @brief Initializes LVGL, TFT drivers and input drivers and starts task needed for GUI operation.
 * 
 */
void gui_init(void);

/**
 * @brief Sets speed bar object and speed bar label to new value
 * 
 * @param new_speed new value for speed
 */
void gui_speed_bar_set(int32_t new_speed);

#ifdef __cplusplus
}
#endif

#endif // __GUI_H__
