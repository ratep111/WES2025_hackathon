/**
 * @file day_night_detector.c
 * @brief Day/night detection using VEML7700 ambient light sensor
 */
#include "day_night_detector.h"
#include "../als-veml7700/veml7700.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "driver/i2c.h"

#define TAG "DAY_NIGHT"

// Hysteresis to prevent rapid switching
#define HYSTERESIS_FACTOR 1.5

static EventGroupHandle_t day_night_event_group     = NULL;
static light_state_t current_state                  = LIGHT_STATE_TRANSITION;
static veml7700_handle_t sensor_handle              = NULL;
static void (*state_change_callback)(light_state_t) = NULL;
static double current_lux                           = 0.0;

esp_err_t day_night_init(void) {
    // Create the event group
    if(!day_night_event_group) {
        day_night_event_group = xEventGroupCreate();
        if(day_night_event_group == NULL) {
            ESP_LOGE(TAG, "Failed to create event group");
            return ESP_FAIL;
        }
    }

    ESP_LOGI(TAG, "Day/night detector initialized");
    return ESP_OK;
}

bool is_day_mode(void) {
    if(!day_night_event_group)
        return true;
    EventBits_t bits = xEventGroupGetBits(day_night_event_group);
    return (bits & DAY_MODE_BIT) != 0;
}

bool is_night_mode(void) {
    if(!day_night_event_group)
        return false;
    EventBits_t bits = xEventGroupGetBits(day_night_event_group);
    return (bits & NIGHT_MODE_BIT) != 0;
}

esp_err_t get_light_level(double *lux) {
    if(!lux)
        return ESP_ERR_INVALID_ARG;
    *lux = current_lux;
    return ESP_OK;
}

light_state_t get_light_state(void) {
    return current_state;
}

void light_register_callback(void (*callback)(light_state_t state)) {
    state_change_callback = callback;
}

static void update_light_state(light_state_t new_state) {
    if(current_state == new_state)
        return;

    current_state = new_state;

    if(new_state == LIGHT_STATE_DAY) {
        xEventGroupClearBits(day_night_event_group, NIGHT_MODE_BIT);
        xEventGroupSetBits(day_night_event_group, DAY_MODE_BIT);
    } else if(new_state == LIGHT_STATE_NIGHT) {
        xEventGroupClearBits(day_night_event_group, DAY_MODE_BIT);
        xEventGroupSetBits(day_night_event_group, NIGHT_MODE_BIT);
    }

    ESP_LOGI(TAG,
            "Light state changed: %s",
            new_state == LIGHT_STATE_DAY ? "DAY"
            : new_state == LIGHT_STATE_NIGHT
                    ? "NIGHT"
                    : "UNKNOWN");

    if(state_change_callback) {
        state_change_callback(new_state);
    }
}

void day_night_task(void *pvParameters) {
    esp_err_t ret = veml7700_initialize(&sensor_handle, 0);
    if(ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize VEML7700 sensor: %s", esp_err_to_name(ret));
        vTaskDelete(NULL);
    }

    ESP_LOGI(TAG, "VEML7700 sensor initialized");
    if(day_night_init() != ESP_OK) {
        vTaskDelete(NULL);
    }

    vTaskDelay(pdMS_TO_TICKS(1000)); // Sensor stabilization delay

    double samples[5] = { 0 };
    int sample_index  = 0;

    while(1) {
        ret = veml7700_read_als_lux_auto(sensor_handle, &current_lux);
        if(ret != ESP_OK) {
            ESP_LOGE(TAG, "Sensor read failed: %s", esp_err_to_name(ret));
            vTaskDelay(pdMS_TO_TICKS(2000));
            continue;
        }

        samples[sample_index] = current_lux;
        sample_index          = (sample_index + 1) % 5;

        double avg = 0;
        for(int i = 0; i < 5; ++i) {
            avg += samples[i];
        }
        avg /= 5;

        ESP_LOGI(TAG, "Lux: %.2f (avg: %.2f)", current_lux, avg);

        switch(current_state) {
            case LIGHT_STATE_DAY:
                if(avg < NIGHT_THRESHOLD) {
                    ESP_LOGI(TAG, "Transition to NIGHT");
                    update_light_state(LIGHT_STATE_NIGHT);
                }
                break;

            case LIGHT_STATE_NIGHT:
                if(avg > DAY_THRESHOLD * HYSTERESIS_FACTOR) {
                    ESP_LOGI(TAG, "Transition to DAY");
                    update_light_state(LIGHT_STATE_DAY);
                }
                break;

            case LIGHT_STATE_TRANSITION:
            case LIGHT_STATE_UNKNOWN:
            default:
                if(avg < NIGHT_THRESHOLD) {
                    update_light_state(LIGHT_STATE_NIGHT);
                } else {
                    update_light_state(LIGHT_STATE_DAY);
                }
                break;
        }

        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}
