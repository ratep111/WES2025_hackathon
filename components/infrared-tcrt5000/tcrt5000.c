/**
 * @file tcrt5000.c
 * @brief Implementation of TCRT5000 IR reflective sensor driver (ESP32 + optional PCF8574 I/O expander)
 */

#include "tcrt5000.h"
#include "esp_log.h"
#include "pcf8574.h"
#include <stdbool.h>

static const char *TAG = "TCRT5000";

extern i2c_dev_t expander;
extern uint8_t expander_state; // shared state for expander

esp_err_t tcrt5000_init(const tcrt5000_config_t *config, tcrt5000_handle_t *handle) {
    if(config == NULL || handle == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    // Copy configuration
    handle->config = *config;

    if(config->use_expander) {
        // Set expander pin high to enable input mode
        expander_state |= (1 << config->digital_pin);
        esp_err_t ret   = pcf8574_port_write(&expander, expander_state);
        if(ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to set expander pin %d as input: %s", config->digital_pin, esp_err_to_name(ret));
            return ret;
        }
    } else if(config->use_digital) {
        // Configure GPIO for digital input
        gpio_config_t io_conf = { .pin_bit_mask = (1ULL << config->digital_pin),
            .mode                               = GPIO_MODE_INPUT,
            .pull_up_en                         = GPIO_PULLUP_DISABLE,
            .pull_down_en                       = GPIO_PULLDOWN_DISABLE,
            .intr_type                          = GPIO_INTR_DISABLE };

        esp_err_t ret = gpio_config(&io_conf);
        if(ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to configure GPIO: %s", esp_err_to_name(ret));
            return ret;
        }
    } else {
        // Configure ADC for analog input
        esp_err_t ret = adc1_config_width(ADC_WIDTH_BIT_12);
        if(ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to configure ADC width: %s", esp_err_to_name(ret));
            return ret;
        }

        ret = adc1_config_channel_atten(config->adc_channel, ADC_ATTEN_DB_11);
        if(ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to configure ADC attenuation: %s", esp_err_to_name(ret));
            return ret;
        }
    }

    ESP_LOGI(TAG, "TCRT5000 sensor initialized successfully");
    return ESP_OK;
}

esp_err_t tcrt5000_read_raw(tcrt5000_handle_t *handle, uint32_t *value) {
    if(handle == NULL || value == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    if(handle->config.use_digital || handle->config.use_expander) {
        // For digital, return 4095 if high, 0 if low
        bool detected = false;
        esp_err_t ret = tcrt5000_read_digital(handle, &detected);
        *value        = detected ? 4095 : 0;
        return ret;
    } else {
        // Read raw ADC value
        int adc_raw = adc1_get_raw(handle->config.adc_channel);
        if(adc_raw < 0) {
            ESP_LOGE(TAG, "Failed to read ADC value");
            return ESP_FAIL;
        }
        *value = (uint32_t) adc_raw;
        return ESP_OK;
    }
}

esp_err_t tcrt5000_read_digital(tcrt5000_handle_t *handle, bool *detected) {
    if(handle == NULL || detected == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    if(handle->config.use_expander) {
        uint8_t port_val = 0;
        esp_err_t ret    = pcf8574_port_read(&expander, &port_val);
        if(ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to read from expander: %s", esp_err_to_name(ret));
            return ret;
        }

        bool level = port_val & (1 << handle->config.digital_pin);
        *detected  = handle->config.invert_output ? !level : level;
    } else if(handle->config.use_digital) {
        int level = gpio_get_level(handle->config.digital_pin);
        *detected = handle->config.invert_output ? !level : level;
    } else {
        uint32_t value;
        esp_err_t ret = tcrt5000_read_raw(handle, &value);
        if(ret != ESP_OK) {
            return ret;
        }
        bool detection = (value > handle->config.threshold);
        *detected      = handle->config.invert_output ? !detection : detection;
    }

    return ESP_OK;
}