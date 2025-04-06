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
    GUI_PROX_NOTHING_NEAR,
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

/**
 * @brief Set the current time text on UI labels.
 *
 * Updates both the main and top time labels with the given time string.
 *
 * @param time The formatted time string to display (e.g., "14:30").
 */
void gui_time_set(const char *time);

/**
 * @brief Set the current date text on UI labels.
 *
 * Updates both the main and top date labels with the given date string.
 *
 * @param date The formatted date string to display (e.g., "Sat, 5 May").
 */
void gui_date_set(const char *date);

/**
 * @brief Convert hours and minutes into a time string.
 *
 * Formats a time string from integer hour and minute values.
 *
 * @param buffer The buffer to store the formatted time string (should be at least 6 bytes).
 * @param hours The hour component (0–23).
 * @param minutes The minute component (0–59).
 */
void gui_time_convert(char *buffer, int hours, int minutes);

/**
 * @brief Set the current weather condition text on the UI.
 *
 * Updates the weather information label with the given string.
 *
 * @param weather A string describing the current weather (e.g., "Sunny", "Rainy").
 */
void gui_weather_set(const char *weather);

/**
 * @brief Set the current temperature value on the UI.
 *
 * Updates the temperature label with the given string.
 *
 * @param temp A string representing the temperature (e.g., "23°C").
 */
void gui_sntp_temp_set(const char *temp);

/**
 * @brief Convert a temperature value into a formatted string with a degree symbol.
 *
 * @param buffer The buffer to store the formatted temperature string (e.g., "23°C").
 * @param temp The temperature in Celsius.
 */
void gui_temp_convert(char *buffer, int temp);

/**
 * @brief Set the fuel percentage value on the UI fuel arc.
 *
 * Validates and updates the fuel indicator arc with a percentage from 0 to 100.
 *
 * @param fuel_percentage An integer between 0 and 100 representing fuel level.
 */
void gui_fuel_percentage_set(int fuel_percentage);

/**
 * @brief Set the given door indicator to "open" state (hides the bar).
 *
 * Hides the corresponding UI element indicating the door is open.
 *
 * @param door The door enum value (e.g., GUI_DOOR_FRONT_LEFT).
 */
void gui_set_door_open(gui_doors_t door);

/**
 * @brief Set the given door indicator to "closed" state (shows the bar).
 *
 * Clears the hidden flag for the door's UI element, marking it as closed.
 *
 * @param door The door enum value (e.g., GUI_DOOR_TRUNK).
 */
void gui_set_door_closed(gui_doors_t door);

/**
 * @brief Set the current temperature value on the UI.
 *
 * Updates the temperature label with the given string.
 *
 * @param temp A string representing the temperature (e.g., "23°C").
 */
void gui_local_temp_set(const char *temp);

/**
 * @brief Set the current temperature value on the UI.
 *
 * Updates the temperature label with the given string.
 *
 * @param temp A string representing the temperature (e.g., "23°C").
 */
void gui_hum_temp_set(const char *hum);


#ifdef __cplusplus
}
#endif

#endif // __GUI_H__
