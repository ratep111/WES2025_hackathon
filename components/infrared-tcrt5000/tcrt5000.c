/**
 * @file tcrt5000.c
 * @brief Implementation of TCRT5000 IR reflective sensor driver
 */
#include "tcrt5000.h"
#include "esp_log.h"

static const char *TAG = "TCRT5000";

esp_err_t tcrt5000_init(const tcrt5000_config_t *config, tcrt5000_handle_t *handle) {
    if(config == NULL || handle == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    // Copy configuration
    handle->config = *config;

    if(config->use_digital) {
        // Configure GPIO for digital input
        gpio_config_t io_conf = { .pin_bit_mask = (1ULL << config->digital_pin),
            .mode                               = GPIO_MODE_INPUT,
            .pull_up_en                         = GPIO_PULLUP_DISABLE,
            .pull_down_en                       = GPIO_PULLDOWN_DISABLE,
            .intr_type                          = GPIO_INTR_DISABLE };

        esp_err_t ret = gpio_config(&io_conf);
        if(ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to configure GPIO: %d", ret);
            return ret;
        }
    } else {
        // Configure ADC for analog input
        esp_err_t ret = adc1_config_width(ADC_WIDTH_BIT_12);
        if(ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to configure ADC width: %d", ret);
            return ret;
        }

        ret = adc1_config_channel_atten(config->adc_channel, ADC_ATTEN_DB_11);
        if(ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to configure ADC attenuation: %d", ret);
            return ret;
        }

        // Characterize ADC for more accurate readings
        esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN_DB_11, ADC_WIDTH_BIT_12, 0, &handle->adc_chars);
    }

    ESP_LOGI(TAG, "TCRT5000 sensor initialized successfully");
    return ESP_OK;
}

esp_err_t tcrt5000_read_raw(tcrt5000_handle_t *handle, uint32_t *value) {
    if(handle == NULL || value == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    if(handle->config.use_digital) {
        // If using digital input, just return the digital level multiplied by 4095
        // to convert to same range as analog reading
        *value = gpio_get_level(handle->config.digital_pin) ? 4095 : 0;
    } else {
        // Read raw ADC value
        int adc_raw = adc1_get_raw(handle->config.adc_channel);
        if(adc_raw < 0) {
            ESP_LOGE(TAG, "Failed to read ADC value");
            return ESP_FAIL;
        }

        // Convert ADC value to voltage in mV
        *value = esp_adc_cal_raw_to_voltage(adc_raw, &handle->adc_chars);
    }

    return ESP_OK;
}

esp_err_t tcrt5000_read_digital(tcrt5000_handle_t *handle, bool *detected) {
    if(handle == NULL || detected == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    if(handle->config.use_digital) {
        // Direct digital reading
        int level = gpio_get_level(handle->config.digital_pin);
        *detected = handle->config.invert_output ? !level : level;
    } else {
        // Read raw ADC value and compare with threshold
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
