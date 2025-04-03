/**
 * @file wifi_provisioning.h
 *
 * @brief Wi-Fi provisioning interface with BLE support.
 *
 */

#ifndef WIFI_PROV_H
#define WIFI_PROV_H

#ifdef __cplusplus
extern "C" {
#endif

#include "esp_err.h"
#include <stdbool.h>

/**
  * @brief Initializes Wi-Fi and provisioning system.
  *
  * Must be called before any other Wi-Fi provisioning functions.
  *
  * @return ESP_OK on success, appropriate error otherwise.
  */
esp_err_t wifi_provisioning_init(void);

/**
  * @brief Starts BLE provisioning or connects if already provisioned.
  *
  * If credentials are already stored, provisioning is skipped and Wi-Fi connects automatically.
  * This function does not block; use `wifi_provisioning_wait()` to block until connected.
  *
  * @return ESP_OK on success, appropriate error otherwise.
  */
esp_err_t wifi_provisioning_start(void);

/**
  * @brief Waits until Wi-Fi is connected.
  *
  * Blocks the calling task until a successful IP connection is established.
  */
void wifi_provisioning_wait(void);

/**
  * @brief Returns whether Wi-Fi is currently connected.
  *
  * @return true if connected, false otherwise.
  */
bool wifi_provisioning_is_connected(void);

#ifdef __cplusplus
}
#endif

#endif // WIFI_PROV_H
