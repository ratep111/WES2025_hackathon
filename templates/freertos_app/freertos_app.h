/**
 * @file freertos_app.h
 *
 * @brief Public interface for the FreeRTOS app template.
 */

#ifndef FREERTOS_APP_H
#define FREERTOS_APP_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

typedef enum {
    APP_EVENT_EXAMPLE = 0,
    // Add more events here
} app_event_t;

/**
  * @brief Initializes the FreeRTOS app task and queue.
  */
void freertos_app_init(void);

/**
  * @brief Sends an event to the app task.
  */
void freertos_app_send_event(app_event_t event);

#ifdef __cplusplus
}
#endif

#endif // FREERTOS_APP_H
