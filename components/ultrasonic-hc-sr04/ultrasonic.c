/**
 * @file ultrasonic.c
 *
 * ESP-IDF driver for ultrasonic range meters, e.g. ultrasonic, HY-SRF05 and the like
 *
 * Ported from esp-open-rtos
 *
 * Copyright (c) 2016 Ruslan V. Uss <unclerus@gmail.com>
 *
 * BSD Licensed as described in the file LICENSE
 */

//--------------------------------- INCLUDES ----------------------------------
#include "ultrasonic.h"
#include <esp_idf_lib_helpers.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_timer.h>
#include <ets_sys.h>

//---------------------------------- MACROS -----------------------------------
#define TRIGGER_LOW_DELAY  4
#define TRIGGER_HIGH_DELAY 10
#define PING_TIMEOUT       6000
#define ROUNDTRIP_M        5800.0f
#define ROUNDTRIP_CM       58

#define PORT_ENTER_CRITICAL         portENTER_CRITICAL(&mux)
#define PORT_EXIT_CRITICAL          portEXIT_CRITICAL(&mux)
#define timeout_expired(start, len) ((esp_timer_get_time() - (start)) >= (len))

#define CHECK_ARG(VAL)                  \
    do {                                \
        if(!(VAL))                      \
            return ESP_ERR_INVALID_ARG; \
    } while(0)

#define CHECK(x)               \
    do {                       \
        esp_err_t __;          \
        if((__ = x) != ESP_OK) \
            return __;         \
    } while(0)

#define RETURN_CRITICAL(RES) \
    do {                     \
        PORT_EXIT_CRITICAL;  \
        return RES;          \
    } while(0)


//-------------------------------- DATA TYPES ---------------------------------

//------------------------- STATIC DATA & CONSTANTS ---------------------------
static portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;

//------------------------------- GLOBAL DATA ---------------------------------

//------------------------------ PUBLIC FUNCTIONS -----------------------------

esp_err_t ultrasonic_init(const ultrasonic_sensor_t *dev) {
    CHECK_ARG(dev);

    CHECK(gpio_set_direction(dev->trigger_pin, GPIO_MODE_OUTPUT));
    CHECK(gpio_set_direction(dev->echo_pin, GPIO_MODE_INPUT));

    return gpio_set_level(dev->trigger_pin, 0);
}


esp_err_t ultrasonic_measure_raw(const ultrasonic_sensor_t *dev, uint32_t max_time_us, uint32_t *time_us) {
    CHECK_ARG(dev && time_us);

    PORT_ENTER_CRITICAL;

    // Ping: Low for 2..4 us, then high 10 us
    CHECK(gpio_set_level(dev->trigger_pin, 0));
    ets_delay_us(TRIGGER_LOW_DELAY);
    CHECK(gpio_set_level(dev->trigger_pin, 1));
    ets_delay_us(TRIGGER_HIGH_DELAY);
    CHECK(gpio_set_level(dev->trigger_pin, 0));

    // Previous ping isn't ended
    if(gpio_get_level(dev->echo_pin))
        RETURN_CRITICAL(ESP_ERR_ULTRASONIC_PING);

    // Wait for echo
    int64_t start = esp_timer_get_time();
    while(!gpio_get_level(dev->echo_pin)) {
        if(timeout_expired(start, PING_TIMEOUT))
            RETURN_CRITICAL(ESP_ERR_ULTRASONIC_PING_TIMEOUT);
    }

    // got echo, measuring
    int64_t echo_start = esp_timer_get_time();
    int64_t time       = echo_start;
    while(gpio_get_level(dev->echo_pin)) {
        time = esp_timer_get_time();
        if(timeout_expired(echo_start, max_time_us))
            RETURN_CRITICAL(ESP_ERR_ULTRASONIC_ECHO_TIMEOUT);
    }
    PORT_EXIT_CRITICAL;

    *time_us = time - echo_start;

    return ESP_OK;
}

esp_err_t ultrasonic_measure(const ultrasonic_sensor_t *dev, float max_distance, float *distance) {
    CHECK_ARG(dev && distance);

    uint32_t time_us;
    CHECK(ultrasonic_measure_raw(dev, max_distance * ROUNDTRIP_M, &time_us));
    *distance = time_us / ROUNDTRIP_M;

    return ESP_OK;
}

esp_err_t ultrasonic_measure_cm(const ultrasonic_sensor_t *dev, uint32_t max_distance, uint32_t *distance) {
    CHECK_ARG(dev && distance);

    uint32_t time_us;
    CHECK(ultrasonic_measure_raw(dev, max_distance * ROUNDTRIP_CM, &time_us));
    *distance = time_us / ROUNDTRIP_CM;

    return ESP_OK;
}
//---------------------------- PRIVATE FUNCTIONS ------------------------------

//---------------------------- INTERRUPT HANDLERS -----------------------------
