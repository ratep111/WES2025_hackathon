#ifndef INITIALIZATION_H
#define INITIALIZATION_H

/**
 * @brief Initialize all peripheral devices
 * 
 * Initializes I2C, sensors, and other hardware peripherals
 */
void initialization_peripheral_creator();

/**
 * @brief Create and start all sensor tasks
 * 
 * Launches all the FreeRTOS tasks for the sensor modules
 */
void initailization_task_creator();

/**
 * @brief Initialize GUI controller that connects sensors to GUI
 * 
 * This function should be called after peripheral and task initialization
 */
void initialization_gui_controller();

#endif /* INITIALIZATION_H */