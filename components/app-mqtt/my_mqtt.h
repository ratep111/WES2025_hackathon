/**
 * @file my_mqtt_client.h
 * @brief Public API for MQTT module.
 */

#ifndef MQTT_CLIENT_H
#define MQTT_CLIENT_H

//--------------------------------- INCLUDES ----------------------------------
#include "esp_err.h"
#include <stdbool.h>


#ifdef __cplusplus
extern "C" {
#endif

//---------------------------------- MACROS -----------------------------------

//-------------------------------- DATA TYPES ---------------------------------
typedef struct {
    float temperature;
    float humidity;
} TempHumData;

//---------------------- PUBLIC FUNCTION PROTOTYPES --------------------------
/**
   * @brief Initialize MQTT and start publishing task.
   */
esp_err_t mqtt_client_init(void);
/**
   * @brief Returns MQTT connection status.
   */
bool mqtt_client_is_connected(void);

#ifdef __cplusplus
}
#endif

#endif // MQTT_CLIENT_H