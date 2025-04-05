/**
 * @file tcrt5000.h
 * @brief Minimal driver for TCRT5000 IR reflective sensor
 */
#ifndef TCRT5000_H
#define TCRT5000_H

#include "esp_err.h"
#include "driver/gpio.h"
#include "driver/adc.h"
// #include "esp_adc_cal.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

/**
 * @brief TCRT5000 sensor configuration
 */
typedef struct {
    bool use_digital;           /*!< true: Use digital input, false: Use analog input */
    gpio_num_t digital_pin;     /*!< GPIO pin number for digital input */
    adc1_channel_t adc_channel; /*!< ADC channel for analog input */
    uint16_t threshold;         /*!< Threshold value for analog to digital conversion */
    bool invert_output;         /*!< Invert the output (true: detected = 0, false: detected = 1) */
} tcrt5000_config_t;

/**
 * @brief TCRT5000 sensor handle
 */
typedef struct {
    tcrt5000_config_t config; /*!< Sensor configuration */
    // esp_adc_cal_characteristics_t adc_chars; /*!< ADC characteristics for voltage conversion */
} tcrt5000_handle_t;

/**
 * @brief Initialize the TCRT5000 sensor
 * 
 * @param config Pointer to sensor configuration
 * @param handle Pointer to sensor handle to be initialized
 * @return esp_err_t ESP_OK on success
 */
esp_err_t tcrt5000_init(const tcrt5000_config_t *config, tcrt5000_handle_t *handle);

/**
 * @brief Read raw ADC value from TCRT5000 sensor
 * 
 * @param handle Pointer to sensor handle
 * @param value Pointer to store the raw ADC value
 * @return esp_err_t ESP_OK on success
 */
esp_err_t tcrt5000_read_raw(tcrt5000_handle_t *handle, uint32_t *value);

/**
 * @brief Read digital value from TCRT5000 sensor
 * 
 * @param handle Pointer to sensor handle
 * @param detected Pointer to store the detection result (1: detected, 0: not detected)
 * @return esp_err_t ESP_OK on success
 */
esp_err_t tcrt5000_read_digital(tcrt5000_handle_t *handle, bool *detected);

#endif /* TCRT5000_H */
