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

//---------------------------------- MACROS -----------------------------------

//-------------------------------- DATA TYPES ---------------------------------

//---------------------- PUBLIC FUNCTION PROTOTYPES --------------------------

/**
 * @brief Initializes LVGL, TFT drivers and input drivers and starts task needed for GUI operation.
 * 
 */
void gui_init(void);

#ifdef __cplusplus
}
#endif

#endif // __GUI_H__
