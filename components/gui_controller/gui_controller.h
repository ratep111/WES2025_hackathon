/**
 * @file gui_controller.h
 * @brief Connects sensor modules to the GUI frontend
 * 
 * This module handles the connection between sensor data and GUI display,
 * registering callbacks for sensor events and updating the UI accordingly.
 */

#ifndef GUI_CONTROLLER_H
#define GUI_CONTROLLER_H

#include "esp_err.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
  * @brief Initialize the GUI controller
  * 
  * This function initializes all sensor modules and registers callbacks
  * for sensor events to update the GUI
  * 
  * @return esp_err_t ESP_OK on success
  */
esp_err_t gui_controller_init(void);

/**
  * @brief Deinitialize the GUI controller
  * 
  * @return esp_err_t ESP_OK on success
  */
esp_err_t gui_controller_deinit(void);

/**
  * @brief Simulate a fuel level change (for demo purposes)
  * 
  * @param percentage New fuel level (0-100)
  */
void gui_controller_set_fuel(int percentage);

#ifdef __cplusplus
}
#endif

#endif /* GUI_CONTROLLER_H */