/**
* @file gui.c

* @brief This file is an example for how to use the LVGL library.
*
* COPYRIGHT NOTICE: (c) 2022 Byte Lab Grupa d.o.o.
* All rights reserved.
*/

//--------------------------------- INCLUDES ----------------------------------
#include "gui.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_freertos_hooks.h"
#include "freertos/semphr.h"
#include "esp_log.h"
#include "esp_timer.h"

/* Littlevgl specific */
#include "lvgl.h"
#include "lvgl_helpers.h"

#include "squareline/project/ui.h"

//---------------------------------- MACROS -----------------------------------
#define LV_TICK_PERIOD_MS (1U)

//-------------------------------- DATA TYPES ---------------------------------

//---------------------- PRIVATE FUNCTION PROTOTYPES --------------------------
/**
 * @brief Initializes GUI application.
 */
static void _gui_application_init(void);

/**
 * @brief Lv's timer callback function.
 *
 * @param [in] p_arg The argument of the timer.
 */
static void _lv_tick_timer(void *p_arg);

/**
 * @brief Starts GUI task.
 *
 * @param [in] p_parameter Parameter that is passed to the task.
 */
static void _gui_task(void *p_parameter);

/**
 * @brief Returns color based on gradient from green to red with respect to variable speed
 * 
 * @param speed speed from witch to generate color
 * @return lv_color_t generated color
 */
static lv_color_t _get_speed_color(uint32_t speed);


//------------------------- STATIC DATA & CONSTANTS ---------------------------
static SemaphoreHandle_t p_gui_semaphore;
static const char *TAG = "GUI";

//------------------------------- GLOBAL DATA ---------------------------------
static const int _proximity_setup[GUI_PROX_NUM][GUI_PROX_ARC_NUM] = {
    /* red      orange      green   */
    { GUI_PROX_FRONT_VAL, GUI_PROX_FRONT_VAL, GUI_PROX_FRONT_VAL }, // GUI_PROX_FRONT_CLOSE
    { GUI_PROX_NONE_VAL, GUI_PROX_FRONT_VAL, GUI_PROX_FRONT_VAL },  // GUI_PROX_FRONT_MID
    { GUI_PROX_NONE_VAL, GUI_PROX_NONE_VAL, GUI_PROX_FRONT_VAL },   // GUI_PROX_FRONT_FAR
    { GUI_PROX_BACK_VAL, GUI_PROX_BACK_VAL, GUI_PROX_BACK_VAL },    // GUI_PROX_BACK_CLOSE
    { GUI_PROX_NONE_VAL, GUI_PROX_BACK_VAL, GUI_PROX_BACK_VAL },    // GUI_PROX_BACK_MID
    { GUI_PROX_NONE_VAL, GUI_PROX_NONE_VAL, GUI_PROX_BACK_VAL }     // GUI_PROX_BACK_FAR
};

//------------------------------ PUBLIC FUNCTIONS -----------------------------
void gui_init() {
    /* The ESP32 MCU has got two cores - Core 0 and Core 1, each capable of running tasks independently.
    We want the GUI to run smoothly, without Wi-Fi, Bluetooth and any other task taking its time and therefor
    slowing it down. That's why we need to "pin" the GUI task to it's own core, Core 1.
    Doing so, we reduce the risk of resource conflicts, race conditions and other potential issues.
    * NOTE: When not using Wi-Fi nor Bluetooth, you can pin the GUI task to Core 0.*/
    xTaskCreatePinnedToCore(_gui_task, "gui", 4096 * 2, NULL, 0, NULL, 1);
}


void gui_speed_bar_set(int32_t new_speed) {

    if(ui_speed_bar == NULL) {
        ESP_LOGE(TAG, "Speed bar not initialized!");
        return;
    }
    char buffer[GUI_SPEED_BUFF_SIZE];

    int speed_low  = GUI_SPEED_LOW;
    int speed_mid  = GUI_SPEED_MID;
    int speed_high = lv_bar_get_max_value(ui_speed_bar);

    lv_bar_set_value(ui_speed_bar, new_speed, LV_ANIM_ON);

    if(ui_speed_num_lbl != NULL) {
        sprintf(buffer, "%ld", new_speed);
        lv_label_set_text(ui_speed_num_lbl, buffer);
    }
    if(ui_speed_panel != NULL) {

        int border_width = 2;
        int shadow_width = 0;

        if(new_speed < speed_low) {
            border_width = 2;
        } else if(new_speed < speed_mid) {
            border_width = 2;
        } else {
            border_width = 3;
            if(new_speed > speed_high) {
                shadow_width = 10; // Add glow effect
            }
        }

        lv_color_t border_color = _get_speed_color(new_speed);

        // Apply styles to speed panel
        lv_obj_set_style_border_color(ui_speed_panel, border_color, LV_PART_MAIN);
        lv_obj_set_style_border_width(ui_speed_panel, border_width, LV_PART_MAIN);
        lv_obj_set_style_border_opa(ui_speed_panel, LV_OPA_COVER, LV_PART_MAIN);
        lv_obj_set_style_outline_color(ui_speed_panel, border_color, LV_PART_MAIN);
        lv_obj_set_style_outline_width(ui_speed_panel, border_width, LV_PART_MAIN);
        lv_obj_set_style_outline_opa(ui_speed_panel, LV_OPA_COVER, LV_PART_MAIN);

        // Apply style to bar indicator
        lv_obj_set_style_bg_color(ui_speed_bar, border_color, LV_PART_INDICATOR);
        lv_obj_set_style_bg_opa(ui_speed_bar, LV_OPA_COVER, LV_PART_INDICATOR);

        lv_obj_set_style_bg_color(ui_top_panel, border_color, LV_PART_MAIN);

        // Optional shadow
        lv_obj_set_style_shadow_width(ui_speed_panel, shadow_width, LV_PART_MAIN);
        lv_obj_set_style_shadow_color(ui_speed_panel, border_color, LV_PART_MAIN);
        lv_obj_set_style_shadow_opa(ui_speed_panel, LV_OPA_50, LV_PART_MAIN);
    }
}

void gui_proximity_set(gui_proximity_t prox) {
    if(ui_red_proxim_arc == NULL || ui_green_proxim_arc == NULL || ui_orange_proxim_arc == NULL) {
        ESP_LOGE(TAG, "Proximity arcs not initialized");
        return;
    }
    lv_obj_t *_proximity_arcs[] = { ui_red_proxim_arc, ui_orange_proxim_arc, ui_green_proxim_arc };

    for(int i = 0; i < GUI_PROX_ARC_NUM; i++) {
        lv_arc_set_value(_proximity_arcs[i], _proximity_setup[prox][i]);
    }
}


//---------------------------- PRIVATE FUNCTIONS ------------------------------

static lv_color_t _get_speed_color(uint32_t speed) {

    uint8_t r = 0, g = 0;

    int speed_low = GUI_SPEED_LOW;
    int speed_mid = GUI_SPEED_MID;

    if(speed < speed_low) {
        // Green to Orange (0–75): #00FF00 → #FFA500
        float ratio = speed / (float) speed_low;
        r           = (uint8_t) (255 * ratio); // Red: 0 → 255
        g           = 255;                     // Green stays 255
    } else if(speed < speed_mid) {
        // Orange to Red (75–150): #FFA500 → #FF0000
        float ratio = (speed_mid - speed) / (float) speed_mid;
        r           = 255;                             // Red stays 255
        g           = (uint8_t) (255 * (ratio - 1.0)); // Green: 165 → 0
    } else {
        r = 255;
    }

    return lv_color_make(r, g, 20); // Blue is 0 throughout
}


static void _gui_application_init(void) {
    ui_init();
}

static void _lv_tick_timer(void *p_arg) {
    (void) p_arg;

    lv_tick_inc(LV_TICK_PERIOD_MS);
}

static void _gui_task(void *p_parameter) {

    (void) p_parameter;
    p_gui_semaphore = xSemaphoreCreateMutex();

    lv_init();

    /* Initialize SPI or I2C bus used by the drivers */
    lvgl_driver_init();

    lv_color_t *p_buf1 = heap_caps_malloc(DISP_BUF_SIZE * sizeof(lv_color_t), MALLOC_CAP_DMA);
    assert(NULL != p_buf1);

    /* Use double buffered when not working with monochrome displays */
    lv_color_t *p_buf2 = heap_caps_malloc(DISP_BUF_SIZE * sizeof(lv_color_t), MALLOC_CAP_DMA);
    assert(NULL != p_buf2);
    static lv_disp_draw_buf_t disp_draw_buf;
    uint32_t size_in_px = DISP_BUF_SIZE;

    /* Initialize the working buffer */
    lv_disp_draw_buf_init(&disp_draw_buf, p_buf1, p_buf2, size_in_px);

    static lv_disp_drv_t disp_drv;
    disp_drv.hor_res = LV_HOR_RES_MAX;
    disp_drv.ver_res = LV_VER_RES_MAX;
    lv_disp_drv_init(&disp_drv);
    disp_drv.flush_cb = disp_driver_flush;

    disp_drv.draw_buf = &disp_draw_buf;
    lv_disp_drv_register(&disp_drv);

    /* Register an input device */
    static lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.read_cb = touch_driver_read;
    indev_drv.type    = LV_INDEV_TYPE_POINTER;
    lv_indev_drv_register(&indev_drv);

    /* Create and start a periodic timer interrupt to call lv_tick_inc */
    const esp_timer_create_args_t periodic_timer_args = { .callback = &_lv_tick_timer, .name = "periodic_gui" };

    esp_timer_handle_t periodic_timer;
    ESP_ERROR_CHECK(esp_timer_create(&periodic_timer_args, &periodic_timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(periodic_timer, LV_TICK_PERIOD_MS * 1000));

    /* Create the demo application */
    _gui_application_init();

    for(;;) {
        /* Delay 1 tick (assumes FreeRTOS tick is 10ms */
        vTaskDelay(pdMS_TO_TICKS(10));

        /* Try to take the semaphore, call lvgl related function on success */
        if(pdTRUE == xSemaphoreTake(p_gui_semaphore, portMAX_DELAY)) {
            lv_task_handler();
            xSemaphoreGive(p_gui_semaphore);
        }
    }

    /* A task should NEVER return */
    free(p_buf1);
    free(p_buf2);
    vTaskDelete(NULL);
}

//---------------------------- INTERRUPT HANDLERS -----------------------------
