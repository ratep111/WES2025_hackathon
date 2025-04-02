/**
* @file button_gpio_driver.c

* @brief Button driver for ESP32 connected to its GPIO interface.

* @par Button driver for ESP32 connected to its GPIO interface.
*
* COPYRIGHT NOTICE: (c) 2022 Byte Lab Grupa d.o.o.
* All rights reserved.
*/

//--------------------------------- INCLUDES ----------------------------------
#include "button_gpio.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"
//---------------------------------- MACROS -----------------------------------
#define ESP_INTR_FLAG_DEFAULT (0)

//-------------------------------- DATA TYPES ---------------------------------
typedef struct _button_gpio_t {
    uint8_t pin;
    bool b_is_active_on_high_level;
    bool is_button_pressed;
    btn_gpio_pressed_t p_btn_pressed_cb;
    TimerHandle_t debounce_timer;
} _button_gpio_t;

//---------------------- PRIVATE FUNCTION PROTOTYPES --------------------------
/**
 * @brief It configures the GPIO pin as an input, sets the interrupt type, installs the ISR service, and
 * adds the ISR handler.
 *
 * @param [in] p_btn a pointer to the button_gpio_t structure.
 *
 * @return Status of creation.
 */
static esp_err_t _button_create(button_gpio_t *p_btn);

/**
 * @brief The function checks if the button is pressed.
 *
 * @param [in] p_btn a pointer to the button_gpio_t structure that was passed to the button_gpio_create()
 * function.
 *
 * @return True if the button is pressed, false otherwise.
 */
static bool _is_button_pressed(button_gpio_t *p_btn);

/**
 * @brief Allocate memory for a button_gpio_t structure and return a pointer to it
 *
 * @return A pointer to a button_gpio_t struct.
 */
static button_gpio_t *_button_alloc(void);

/**
 * @brief This function frees the memory allocated for the button_gpio_t structure.
 *
 * @param [in] p_btn A pointer to the button_gpio_t structure that was created by the _button_create()
 * function.
 */
static void _button_free(button_gpio_t *p_btn);

/**
 * @brief Debounce timer callback function.
 *
 * @param p_timer Pointer to timer handle.
 */
static void debounce_timer_callback(TimerHandle_t timer);

/**
 * @brief Gpio interrupt handler.
 *
 * @param arg Args.
 */
static void IRAM_ATTR gpio_isr_handler(void *arg);

//------------------------- STATIC DATA & CONSTANTS ---------------------------

static bool gpio_isr_installed = false;

//------------------------------- GLOBAL DATA ---------------------------------

//------------------------------ PUBLIC FUNCTIONS -----------------------------

button_gpio_t *button_gpio_create(uint8_t pin, bool b_is_active_on_high_level, btn_gpio_pressed_t p_button_pressed_cb) {
    if(NULL == p_button_pressed_cb) {
        return NULL;
    }

    button_gpio_t *p_btn = _button_alloc();

    if(NULL == p_btn) {
        return NULL;
    }

    p_btn->pin                       = pin;
    p_btn->b_is_active_on_high_level = b_is_active_on_high_level;
    p_btn->is_button_pressed         = false;
    p_btn->p_btn_pressed_cb          = p_button_pressed_cb;

    if(ESP_OK != _button_create(p_btn)) {
        // Delete button.
        _button_free(p_btn);
        return NULL;
    }

    return p_btn;
}

void button_gpio_delete(button_gpio_t *p_btn) {
    if(NULL != p_btn) {
        _button_free(p_btn);
    }
}

bool button_gpio_is_pressed(button_gpio_t *p_btn) {
    if(NULL != p_btn) {
        return p_btn->is_button_pressed;
    } else {
        return false;
    }
}

//---------------------------- PRIVATE FUNCTIONS ------------------------------

static esp_err_t _button_create(button_gpio_t *p_btn) {
    // Configure GPIO.
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << p_btn->pin),
        .mode         = GPIO_MODE_INPUT,
        .pull_up_en   = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = (p_btn->b_is_active_on_high_level ? GPIO_INTR_POSEDGE : GPIO_INTR_NEGEDGE),
    };

    esp_err_t esp_err = gpio_config(&io_conf);

    if(ESP_OK == esp_err) {
        // Change gpio interrupt type for a pin.
        esp_err = gpio_set_intr_type(io_conf.pin_bit_mask, io_conf.intr_type);
    }

    if(ESP_OK == esp_err) {
        // Install gpio isr service.
        if(!gpio_isr_installed) {
            ESP_ERROR_CHECK(gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT));
            gpio_isr_installed = true;
        }

        // Hook isr handler for specific gpio pin.
        esp_err = gpio_isr_handler_add(p_btn->pin, gpio_isr_handler, (void *) p_btn);

        p_btn->debounce_timer =
                xTimerCreate("debTim", 50 / portTICK_PERIOD_MS, pdFALSE, (void *) p_btn, debounce_timer_callback);
        if(p_btn->debounce_timer == NULL) {
            esp_err = ESP_FAIL;
        }
    }

    return esp_err;
}

static bool _is_button_pressed(button_gpio_t *p_btn) {
    bool b_is_high_level = (0 != gpio_get_level(p_btn->pin));

    return (p_btn->b_is_active_on_high_level ? b_is_high_level : !b_is_high_level);
}

static button_gpio_t *_button_alloc(void) {
    return (button_gpio_t *) malloc(sizeof(button_gpio_t));
}

static void _button_free(button_gpio_t *p_btn) {
    free(p_btn);
}

static void debounce_timer_callback(TimerHandle_t timer) {
    button_gpio_t *p_btn = (button_gpio_t *) pvTimerGetTimerID(timer);
    if(_is_button_pressed(p_btn)) {
        p_btn->is_button_pressed = true;
        if(p_btn->p_btn_pressed_cb != NULL) {
            p_btn->p_btn_pressed_cb((void *) p_btn);
        }
    }
}

//---------------------------- INTERRUPT HANDLERS -----------------------------

static void IRAM_ATTR gpio_isr_handler(void *arg) {
    button_gpio_t *p_btn = (button_gpio_t *) arg;

    // Start or reset the debounce timer for 50 ms.
    if(p_btn->debounce_timer != NULL) {
        xTimerResetFromISR(p_btn->debounce_timer, NULL);
    }
}