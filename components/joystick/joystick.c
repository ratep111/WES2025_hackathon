/**
 * @file joystick.c
 *
 * @brief This file controls the joystick.
 *
 */

//--------------------------------- INCLUDES ----------------------------------
#include <stdio.h>
#include <string.h>
#include "esp_log.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "joystick.h"

//---------------------------------- MACROS -----------------------------------
#define TAG "JOYSTICK"

#if CONFIG_IDF_TARGET_ESP32
#define JOY_X_CHAN ADC_CHANNEL_6
#define JOY_Y_CHAN ADC_CHANNEL_7
#define JOY_ATTEN  ADC_ATTEN_DB_11
#endif

//-------------------------------- DATA TYPES ---------------------------------

//---------------------- PRIVATE FUNCTION PROTOTYPES --------------------------

/**
 * @brief Logs the joystick position as a string.
 *
 * @param input The current joystick position.
 */
static void _joystick_log_position(int input);

/**
 * @brief Calibrates the ADC for the given channel.
 *
 * @param unit ADC unit.
 * @param channel ADC channel.
 * @param atten Attenuation level.
 * @param out_handle Pointer to store the calibration handle.
 * @return true if calibration succeeded, false otherwise.
 */
static bool _joystick_adc_calibrate(adc_unit_t unit, adc_channel_t channel, adc_atten_t atten, adc_cali_handle_t *out_handle);

/**
 * @brief Deinitializes the ADC calibration.
 *
 * @param handle Calibration handle to delete.
 */
static void _joystick_adc_calibrate_deinit(adc_cali_handle_t handle);

//------------------------- STATIC DATA & CONSTANTS ---------------------------
static int last_input = INPUT_CENTER;
static adc_oneshot_unit_handle_t adc1_handle;
static joystick_callback_t callback = NULL;

//------------------------------- GLOBAL DATA ---------------------------------

//------------------------------ PUBLIC FUNCTIONS -----------------------------
esp_err_t joystick_init(void) {
    adc_oneshot_unit_init_cfg_t unit_cfg = { .unit_id = ADC_UNIT_1 };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&unit_cfg, &adc1_handle));

    adc_oneshot_chan_cfg_t chan_cfg = {
        .bitwidth = ADC_BITWIDTH_DEFAULT,
        .atten    = JOY_ATTEN,
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, JOY_X_CHAN, &chan_cfg));
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, JOY_Y_CHAN, &chan_cfg));

    adc_cali_handle_t cal_x = NULL, cal_y = NULL;
    _joystick_adc_calibrate(ADC_UNIT_1, JOY_X_CHAN, JOY_ATTEN, &cal_x);
    _joystick_adc_calibrate(ADC_UNIT_1, JOY_Y_CHAN, JOY_ATTEN, &cal_y);

    return ESP_OK;
}

enum joystick_pos_t joystick_get_position(void) {
    int raw_x = 0, raw_y = 0;

    ESP_ERROR_CHECK(adc_oneshot_read(adc1_handle, JOY_X_CHAN, &raw_x));
    ESP_ERROR_CHECK(adc_oneshot_read(adc1_handle, JOY_Y_CHAN, &raw_y));

    int current_input = INPUT_CENTER;
    if(raw_x <= 1000)
        current_input = INPUT_RIGHT;
    else if(raw_x >= 3500)
        current_input = INPUT_LEFT;
    else if(raw_y <= 500)
        current_input = INPUT_UP;
    else if(raw_y >= 4000)
        current_input = INPUT_DOWN;

    if(current_input != last_input) {
        if(callback) {
            callback(current_input);
        }

#if CONFIG_JOYSTICK_ENABLE_LOGGING
        _joystick_log_position(current_input);
#endif
        last_input = current_input;
    }

    return last_input;
}

void joystick_register_callback(joystick_callback_t cb) {
    callback = cb;
}

//---------------------------- PRIVATE FUNCTIONS ------------------------------
static void _joystick_log_position(int input) {
    switch(input) {
        case INPUT_UP:
            ESP_LOGI(TAG, "↑ UP");
            break;
        case INPUT_DOWN:
            ESP_LOGI(TAG, "↓ DOWN");
            break;
        case INPUT_LEFT:
            ESP_LOGI(TAG, "← LEFT");
            break;
        case INPUT_RIGHT:
            ESP_LOGI(TAG, "→ RIGHT");
            break;
        case INPUT_CENTER:
            ESP_LOGI(TAG, "= CENTER");
            break;
        default:
            break;
    }
}

/* ADC Calibration */
static bool _joystick_adc_calibrate(adc_unit_t unit, adc_channel_t channel, adc_atten_t atten, adc_cali_handle_t *out_handle) {
    adc_cali_handle_t handle = NULL;
    esp_err_t ret            = ESP_FAIL;
    bool calibrated          = false;

#if ADC_CALI_SCHEME_CURVE_FITTING_SUPPORTED
    adc_cali_curve_fitting_config_t cfg = {
        .unit_id  = unit,
        .chan     = channel,
        .atten    = atten,
        .bitwidth = ADC_BITWIDTH_DEFAULT,
    };
    ret = adc_cali_create_scheme_curve_fitting(&cfg, &handle);
    if(ret == ESP_OK)
        calibrated = true;
#endif

#if ADC_CALI_SCHEME_LINE_FITTING_SUPPORTED
    if(!calibrated) {
        adc_cali_line_fitting_config_t cfg = {
            .unit_id  = unit,
            .atten    = atten,
            .bitwidth = ADC_BITWIDTH_DEFAULT,
        };
        ret = adc_cali_create_scheme_line_fitting(&cfg, &handle);
        if(ret == ESP_OK)
            calibrated = true;
    }
#endif

    *out_handle = handle;
    if(ret == ESP_OK) {
        ESP_LOGI(TAG, "ADC Calibration Success");
    } else {
        ESP_LOGW(TAG, "Calibration not supported or failed");
    }

    return calibrated;
}

static void _joystick_adc_calibrate_deinit(adc_cali_handle_t handle) {
#if ADC_CALI_SCHEME_CURVE_FITTING_SUPPORTED
    ESP_ERROR_CHECK(adc_cali_delete_scheme_curve_fitting(handle));
#elif ADC_CALI_SCHEME_LINE_FITTING_SUPPORTED
    ESP_ERROR_CHECK(adc_cali_delete_scheme_line_fitting(handle));
#endif
}

//---------------------------- INTERRUPT HANDLERS -----------------------------