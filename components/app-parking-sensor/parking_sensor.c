/**
 * @file parking_sensor.c
 * @brief Parking sensor logic using HC-SR04 ultrasonic sensor
 */
#include "../ultrasonic-hc-sr04/ultrasonic.h"
#include "parking_sensor.h"
#include "../buzzer/buzzer.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <inttypes.h>

static const char *TAG = "PARKING_SENSOR";

#define ULTRASONIC_TRIGGER_PIN GPIO_NUM_27
#define ULTRASONIC_ECHO_PIN    GPIO_NUM_34

static ultrasonic_sensor_t sensor = { .trigger_pin = ULTRASONIC_TRIGGER_PIN, .echo_pin = ULTRASONIC_ECHO_PIN };

static uint32_t current_distance = MAX_DISTANCE;

esp_err_t parking_sensor_init(void) {
    esp_err_t ret;

    ret = ultrasonic_init(&sensor);
    if(ret != ESP_OK) {
        ESP_LOGE(TAG, "Ultrasonic sensor init failed: %s", esp_err_to_name(ret));
        return ret;
    }

    ret = buzzer_init();
    if(ret != ESP_OK) {
        ESP_LOGE(TAG, "Buzzer init failed: %s", esp_err_to_name(ret));
        return ret;
    }

    ESP_LOGI(TAG, "Parking sensor initialized");
    return ESP_OK;
}

esp_err_t parking_sensor_get_distance(uint32_t *distance) {
    if(!distance)
        return ESP_ERR_INVALID_ARG;

    *distance = current_distance;
    return ESP_OK;
}

bool parking_sensor_is_danger(void) {
    return current_distance < DISTANCE_DANGER;
}

bool parking_sensor_is_warning(void) {
    return current_distance >= DISTANCE_DANGER && current_distance < DISTANCE_WARNING;
}

bool parking_sensor_is_safe(void) {
    return current_distance >= DISTANCE_WARNING;
}

void parking_sensor_task(void *pvParameters) {
    if(parking_sensor_init() != ESP_OK) {
        vTaskDelete(NULL);
        return;
    }

    TickType_t delay_ms = 500;

    while(1) {
        esp_err_t ret = ultrasonic_measure_cm(&sensor, MAX_DISTANCE, &current_distance);

        if(ret == ESP_OK) {
            // ESP_LOGI(TAG, "Distance: %u cm", current_distance);
            ESP_LOGI(TAG, "Distance: %" PRIu32 " cm", current_distance);

            if(current_distance < DISTANCE_DANGER) {
                delay_ms = 100;
                ESP_LOGW(TAG, "DANGER ZONE!");
            } else if(current_distance < DISTANCE_WARNING) {
                delay_ms = 300;
                ESP_LOGI(TAG, "Warning zone");
            } else if(current_distance < DISTANCE_SAFE) {
                delay_ms = 700;
                ESP_LOGI(TAG, "Safe zone");
            } else {
                delay_ms = 0;
                ESP_LOGI(TAG, "Out of range");
            }

            // Beep if within range
            if(delay_ms > 0) {
                buzzer_set_duty(500); // Set moderate intensity (depends on LEDC resolution)
                vTaskDelay(pdMS_TO_TICKS(50));
                buzzer_set_duty(0);
                vTaskDelay(pdMS_TO_TICKS(delay_ms));
            } else {
                buzzer_set_duty(0);
                vTaskDelay(pdMS_TO_TICKS(500));
            }

        } else {
            ESP_LOGE(TAG, "Distance read failed: %s", esp_err_to_name(ret));
            current_distance = MAX_DISTANCE;
            buzzer_set_duty(0);
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
    }
}
