/**
 * @file template_component.h
 *
 * @brief Template header file for an ESP-IDF component.
 */

#ifndef TEMPLATE_COMPONENT_H
#define TEMPLATE_COMPONENT_H

#ifdef __cplusplus
extern "C" {
#endif

#include "esp_err.h"

/**
  * @brief Initialize the template component.
  *
  * @return ESP_OK on success, appropriate error otherwise.
  */
esp_err_t component_template_init(void);

#ifdef __cplusplus
}
#endif

#endif // TEMPLATE_COMPONENT_H
