/**
 * @file door_detector.h
 * @brief Door open/closed detection using TCRT5000 IR reflective sensor
 */
#ifndef DOOR_DETECTOR_H
#define DOOR_DETECTOR_H

#include "esp_err.h"
#include <stdbool.h>
#include "freertos/FreeRTOS.h"

/**
 * @brief Door state enumeration
 */
typedef enum {
    DOOR_STATE_UNKNOWN,
    DOOR_STATE_OPEN,
    DOOR_STATE_CLOSED
} door_state_t;

/**
 * @brief Door event structure
 */
typedef struct {
    door_state_t state;
    uint32_t timestamp;
} door_event_t;

/**
 * @brief Initialize the door detector
 * 
 * @return esp_err_t ESP_OK on success
 */
esp_err_t door_detector_init(void);

/**
 * @brief Task function for door detector operation
 * 
 * @param pvParameters FreeRTOS task parameters (not used)
 */
void door_detector_task(void *pvParameters);

/**
 * @brief Check if the door is currently open
 * 
 * @return bool true if door is open
 */
bool is_door_open(void);

/**
 * @brief Check if the door is currently closed
 * 
 * @return bool true if door is closed
 */
bool is_door_closed(void);

/**
 * @brief Get the current door state
 * 
 * @return door_state_t Current door state
 */
door_state_t get_door_state(void);

/**
 * @brief Get door state change event from queue
 * 
 * @param event Pointer to store the event
 * @param wait_ticks How long to wait for an event
 * @return bool true if event was retrieved
 */
bool get_door_event(door_event_t *event, TickType_t wait_ticks);

/**
 * @brief Register callback for door state changes
 * 
 * @param callback Function to call when door state changes
 */
void door_register_callback(void (*callback)(door_state_t state));

#endif /* DOOR_DETECTOR_H */