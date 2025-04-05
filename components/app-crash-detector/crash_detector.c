/**
 * @file crash_detector.c
 * @brief Impact/crash detection using accelerometer
 */
#include "crash_detector.h"
#include "LIS2DH12TR.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include <string.h>
#include <time.h>
#include <math.h>
#include "pcf8574.h"

#define TAG "CRASH_DETECTOR"

// Sampling interval (ms)
#define SAMPLE_INTERVAL_MS 50

// PCF8574 I/O expander pin used for crash detection signal
extern i2c_dev_t expander;            // Declared in app_main
static uint8_t expander_state = 0xFF; // Default all HIGH (idle)
#define CRASH_DET_PIN 0               // P0 on PCF8574

// Config and state
static float crash_threshold                        = CRASH_ACCEL_THRESHOLD;
static bool crash_detected                          = false;
static crash_event_t last_crash_event               = { 0 };
static TimerHandle_t reset_timer                    = NULL;
static void (*crash_callback)(crash_event_t *event) = NULL;

// Mock time for demo
static time_t mock_timestamp = 1712342400;

static time_t get_mock_time(void) {
    mock_timestamp += (SAMPLE_INTERVAL_MS / 1000);
    return mock_timestamp;
}

static void format_timestamp(time_t ts, char *buf, size_t size) {
    struct tm timeinfo;
    localtime_r(&ts, &timeinfo);
    snprintf(buf,
            size,
            "%04d-%02d-%02d %02d_%02d_%02d",
            timeinfo.tm_year + 1900,
            timeinfo.tm_mon + 1,
            timeinfo.tm_mday,
            timeinfo.tm_hour,
            timeinfo.tm_min,
            timeinfo.tm_sec);
}

// === NEW: PCF8574 GPIO helper ===
static void pcf8574_set_pin(uint8_t pin, bool high) {
    if(high)
        expander_state |= (1 << pin); // release line (idle)
    else
        expander_state &= ~(1 << pin); // pull low (signal)

    pcf8574_port_write(&expander, expander_state);
}

/**
 * @brief Send crash notification (via I/O expander pin)
 */
static void send_crash_notification(crash_event_t *event) {
    pcf8574_set_pin(CRASH_DET_PIN, false); // Pull LOW to signal
    ESP_LOGW(TAG, "Crash signal sent via PCF8574 P%d (LOW)", CRASH_DET_PIN);
}

/**
 * @brief Timer callback to auto-reset crash state
 */
static void reset_timer_callback(TimerHandle_t xTimer) {
    crash_detected = false;
    pcf8574_set_pin(CRASH_DET_PIN, true); // Release pin (HIGH)
    ESP_LOGW(TAG, "Crash reset: pin released (HIGH)");
}

esp_err_t crash_detector_init(void) {
    // Reset state
    crash_detected = false;
    pcf8574_set_pin(CRASH_DET_PIN, true); // Idle state: HIGH

    reset_timer = xTimerCreate("crash_reset_timer", pdMS_TO_TICKS(CRASH_RESET_TIMEOUT_MS), pdFALSE, 0, reset_timer_callback);

    if(!reset_timer) {
        ESP_LOGE(TAG, "Failed to create crash reset timer");
        return ESP_FAIL;
    }

    ESP_LOGE(TAG, "Crash detector initialized with threshold: %.2fg", crash_threshold);
    return ESP_OK;
}

void crash_detector_task(void *pvParameters) {
    LIS2DH12TR_accelerations acc = { 0 };
    float impact_magnitude;
    LIS2DH12TR_init();

    ESP_LOGE(TAG, "Crash detector task started");

    while(1) {
        if(LIS2DH12TR_read_acc(&acc) == LIS2DH12TR_READING_OK) {
            impact_magnitude         = sqrtf(acc.x_acc * acc.x_acc + acc.y_acc * acc.y_acc + acc.z_acc * acc.z_acc);
            float adjusted_magnitude = impact_magnitude > 1.0f ? impact_magnitude - 1.0f : 0.0f;

            if(adjusted_magnitude > crash_threshold && !crash_detected) {
                crash_detected                = true;
                last_crash_event.impact_force = adjusted_magnitude;
                last_crash_event.timestamp    = get_mock_time();
                format_timestamp(last_crash_event.timestamp,
                        last_crash_event.timestamp_str,
                        sizeof(last_crash_event.timestamp_str));

                send_crash_notification(&last_crash_event);

                if(crash_callback)
                    crash_callback(&last_crash_event);
                xTimerStart(reset_timer, 0);

                ESP_LOGE(TAG, "Crash detected! Force: %.2fg", adjusted_magnitude);
            }

            if(adjusted_magnitude > crash_threshold / 2) {
                ESP_LOGD(TAG, "High impact detected: %.2fg", adjusted_magnitude);
            }
        }

        vTaskDelay(pdMS_TO_TICKS(SAMPLE_INTERVAL_MS));
    }
}

bool crash_detector_is_crashed(void) {
    return crash_detected;
}

bool crash_detector_get_last_event(crash_event_t *event) {
    if(!crash_detected || !event)
        return false;
    memcpy(event, &last_crash_event, sizeof(crash_event_t));
    return true;
}

void crash_detector_reset(void) {
    crash_detected = false;
    pcf8574_set_pin(CRASH_DET_PIN, true); // release
    ESP_LOGE(TAG, "Crash state manually reset");
}

void crash_detector_set_threshold(float threshold) {
    if(threshold > 0) {
        crash_threshold = threshold;
        ESP_LOGE(TAG, "Crash threshold updated to %.2fg", threshold);
    }
}

void crash_detector_register_callback(void (*callback)(crash_event_t *event)) {
    crash_callback = callback;
}
