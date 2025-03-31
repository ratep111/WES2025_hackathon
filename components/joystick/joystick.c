#include <stdio.h>
#include <string.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"

#include "joystick.h"

#define TAG "JOYSTICK"

#if CONFIG_IDF_TARGET_ESP32
#define JOY_X_CHAN ADC_CHANNEL_6
#define JOY_Y_CHAN ADC_CHANNEL_7
#define JOY_ATTEN  ADC_ATTEN_DB_11
#endif

static int last_input = INPUT_CENTER;
static adc_oneshot_unit_handle_t adc1_handle;
static TaskHandle_t joystick_task_handle;

static bool adc_calibrate(adc_unit_t unit, adc_channel_t channel, adc_atten_t atten, adc_cali_handle_t *out_handle);
static void adc_calibrate_deinit(adc_cali_handle_t handle);
static void joystick_task(void *arg);

static void handle_input(int input) {
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
    adc_calibrate(ADC_UNIT_1, JOY_X_CHAN, JOY_ATTEN, &cal_x);
    adc_calibrate(ADC_UNIT_1, JOY_Y_CHAN, JOY_ATTEN, &cal_y);

    if(!joystick_task_handle) {
        xTaskCreate(joystick_task, "joystick_task", 2048, NULL, 5, &joystick_task_handle);
    }

    return ESP_OK;
}

enum joystick_pos_t get_joystick_position(void) {
    return last_input;
}

static void joystick_task(void *arg) {
    int raw_x = 0, raw_y = 0;

    while(1) {
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
            handle_input(current_input);
            last_input = current_input;
        }

        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

/* ADC Calibration */
static bool adc_calibrate(adc_unit_t unit, adc_channel_t channel, adc_atten_t atten, adc_cali_handle_t *out_handle) {
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

static void adc_calibrate_deinit(adc_cali_handle_t handle) {
#if ADC_CALI_SCHEME_CURVE_FITTING_SUPPORTED
    ESP_ERROR_CHECK(adc_cali_delete_scheme_curve_fitting(handle));
#elif ADC_CALI_SCHEME_LINE_FITTING_SUPPORTED
    ESP_ERROR_CHECK(adc_cali_delete_scheme_line_fitting(handle));
#endif
}
