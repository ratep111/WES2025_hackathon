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

#define GUI_PROX_ARC_NUM   (3)
#define GUI_PROX_FRONT_VAL (0)
#define GUI_PROX_NONE_VAL  (50)
#define GUI_PROX_BACK_VAL  (100)

#define GUI_FUEL_MAX (60)


//-------------------------------- DATA TYPES ---------------------------------
typedef enum {
    GUI_PROX_FRONT_CLOSE,
    GUI_PROX_FRONT_MID,
    GUI_PROX_FRONT_FAR,
    GUI_PROX_BACK_CLOSE,
    GUI_PROX_BACK_MID,
    GUI_PROX_BACK_FAR,
    GUI_PROX_NUM
} gui_proximity_t;

typedef enum {
    front_right,
    front_left,
    back_right,
    back_left,
    trunk,
    gui_num_of_doors
} gui_doors_t;
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

/**
 * @brief Sets proximity bars around car icon
 * 
 * @param prox variable that holds which prox bar should be activated, closer bars automatically activate farther ones
 */
void gui_proximity_set(gui_proximity_t prox);


#ifdef __cplusplus
}
#endif

#endif // __GUI_H__
