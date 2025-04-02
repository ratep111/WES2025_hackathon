/**
 * @file buzzer.c
 * 
 * @brief Buzzer driver using LEDC PWM for tone control.
 * 
 */

//--------------------------------- INCLUDES ----------------------------------
#include <stdio.h>
#include "esp_err.h"
#include "driver/ledc.h"
#include "buzzer.h"

//---------------------------------- MACROS -----------------------------------
#define TAG            "BUZZER"
#define BUZZER_GPIO    2
#define PWM_CHANNEL    LEDC_CHANNEL_0
#define PWM_MODE       LEDC_HIGH_SPEED_MODE
#define PWM_FREQ_HZ    1000
#define PWM_RESOLUTION LEDC_TIMER_13_BIT
#define PWM_TIMER      LEDC_TIMER_0

//-------------------------------- DATA TYPES ---------------------------------

//---------------------- PRIVATE FUNCTION PROTOTYPES --------------------------

//------------------------- STATIC DATA & CONSTANTS ---------------------------

//------------------------------- GLOBAL DATA ---------------------------------

//------------------------------ PUBLIC FUNCTIONS -----------------------------

esp_err_t buzzer_init(void) {
    ledc_timer_config_t timer_cfg = { .speed_mode = PWM_MODE,
        .timer_num                                = PWM_TIMER,
        .duty_resolution                          = PWM_RESOLUTION,
        .freq_hz                                  = PWM_FREQ_HZ,
        .clk_cfg                                  = LEDC_AUTO_CLK };

    ESP_ERROR_CHECK(ledc_timer_config(&timer_cfg));

    ledc_channel_config_t channel_cfg = { .channel = PWM_CHANNEL,
        .duty                                      = 0,
        .gpio_num                                  = BUZZER_GPIO,
        .speed_mode                                = PWM_MODE,
        .intr_type                                 = LEDC_INTR_DISABLE,
        .hpoint                                    = 0,
        .timer_sel                                 = PWM_TIMER };

    return ledc_channel_config(&channel_cfg);
}

esp_err_t buzzer_set_duty(uint32_t duty) {

    ESP_ERROR_CHECK(ledc_set_duty(PWM_MODE, PWM_CHANNEL, duty));

    return ledc_update_duty(PWM_MODE, PWM_CHANNEL);
}

//---------------------------- PRIVATE FUNCTIONS ------------------------------

//---------------------------- INTERRUPT HANDLERS -----------------------------