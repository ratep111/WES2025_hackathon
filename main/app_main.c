/*******************************************************************************/
/*                                  INCLUDES                                   */
/*******************************************************************************/

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2c.h"

#include "gui/gui.h"
#include "sht3x.h"
#include "pcf8523.h"
#include "button.h"
#include "at24cx_i2c.h"
#include "joystick.h"
#include "buzzer.h"
#include "esp32_perfmon.h"

/*******************************************************************************/
/*                                   MACROS                                     */
/*******************************************************************************/

#define TAG                  "MAIN"
#define TEMP_TASK_STACK_SIZE 2048
#define TEMP_TASK_PRIORITY   5
#define TEMP_READ_DELAY_MS   2000

/*******************************************************************************/
/*                                 DATA TYPES                                  */
/*******************************************************************************/

/*******************************************************************************/
/*                         PRIVATE FUNCTION PROTOTYPES                         */
/*******************************************************************************/

static void temp_sens_task(void *args);
static void rtc_task(void *args);
static void button_task(void *args);
static void eeprom_task(void *args);
static void joystick_task(void *args);
static void buzzer_task(void *args);

static void button1_pressed(void *args);
static void button2_pressed(void *args);
static void button3_pressed(void *args);
static void button4_pressed(void *args);

/*******************************************************************************/
/*                          STATIC DATA & CONSTANTS                            */
/*******************************************************************************/

static const i2c_config_t _i2c_config = {
    .mode             = I2C_MODE_MASTER,
    .scl_io_num       = GPIO_NUM_21,
    .sda_io_num       = GPIO_NUM_22,
    .sda_pullup_en    = GPIO_PULLUP_ENABLE,
    .scl_pullup_en    = GPIO_PULLUP_ENABLE,
    .master.clk_speed = 200000,
    .clk_flags        = 0,
};

/*******************************************************************************/
/*                                 GLOBAL DATA                                 */
/*******************************************************************************/

/*******************************************************************************/
/*                              PUBLIC FUNCTIONS                               */
/*******************************************************************************/
void app_main() {
    ESP_LOGI(TAG, "Initializing I2C master...");
    ESP_ERROR_CHECK(i2c_param_config(I2C_MASTER_NUM, &_i2c_config));
    ESP_ERROR_CHECK(
            i2c_driver_install(I2C_MASTER_NUM, _i2c_config.mode, I2C_MASTER_RX_BUF_DISABLE, I2C_MASTER_TX_BUF_DISABLE, 0));

    if(sht3x_start_periodic_measurement() != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start SHT3x periodic measurement!");
        return;
    }

    gui_init();
    perfmon_start();

    ESP_LOGI(TAG, "Creating temperature sensor task...");
    xTaskCreate(temp_sens_task, "temp_sens_task", TEMP_TASK_STACK_SIZE, NULL, TEMP_TASK_PRIORITY, NULL);
    xTaskCreate(rtc_task, "rtc_task", TEMP_TASK_STACK_SIZE, NULL, TEMP_TASK_PRIORITY, NULL);
    xTaskCreate(button_task, "button_task", TEMP_TASK_STACK_SIZE, NULL, TEMP_TASK_PRIORITY, NULL);
    xTaskCreate(eeprom_task, "eeprom_task", 2 * TEMP_TASK_STACK_SIZE, NULL, TEMP_TASK_PRIORITY, NULL);
    xTaskCreate(joystick_task, "joystick_task", TEMP_TASK_STACK_SIZE, NULL, TEMP_TASK_PRIORITY, NULL);
    xTaskCreate(buzzer_task, "buzzer_task", TEMP_TASK_STACK_SIZE, NULL, TEMP_TASK_PRIORITY, NULL);
}

/*******************************************************************************/
/*                             PRIVATE FUNCTIONS                               */
/*******************************************************************************/

static void temp_sens_task(void *args) {
    sht3x_sensors_values_t sensor_values = { 0 };
    TickType_t last_wake_time            = xTaskGetTickCount();

    ESP_LOGI(TAG, "Start temp task.");
    for(;;) {
        esp_err_t ret = sht3x_read_measurement(&sensor_values);

        if(ret == ESP_OK) {
            ESP_LOGI(TAG, "Temperature: %.1f°C - Humidity: %.1f%%", sensor_values.temperature, sensor_values.humidity);

        } else {
            ESP_LOGE(TAG, "Failed to read sensor measurement! Error: %d", ret);
        }

        vTaskDelayUntil(&last_wake_time, TEMP_READ_DELAY_MS / portTICK_PERIOD_MS);
    }

    vTaskDelete(NULL);
}

static void rtc_task(void *args) {
    struct tm time            = { 0 };
    TickType_t last_wake_time = xTaskGetTickCount();

    esp_err_t ret = pcf8523_init(I2C_NUM_0, GPIO_NUM_22, GPIO_NUM_21);
    if(ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize PCF8523 RTC: %s", esp_err_to_name(ret));
        vTaskDelete(NULL);
    }

    pcf8523_set_time(&time);

    ESP_LOGI(TAG, "RTC task started successfully");
    for(;;) {
        ret = pcf8523_get_time(&time);
        if(ret == ESP_OK) {
            ESP_LOGI(TAG,
                    "Current time: %02d:%02d:%02d %02d-%02d-%04d",
                    time.tm_hour,
                    time.tm_min,
                    time.tm_sec,
                    time.tm_mday,
                    time.tm_mon + 1,
                    time.tm_year + 1900);
        } else {
            ESP_LOGE(TAG, "Failed to get RTC time: %s", esp_err_to_name(ret));
        }

        vTaskDelayUntil(&last_wake_time, 2000 / portTICK_PERIOD_MS);
    }

    vTaskDelete(NULL);
}

static void button_task(void *args) {
    button_create(BUTTON_1, button1_pressed);
    button_create(BUTTON_2, button2_pressed);
    button_create(BUTTON_3, button3_pressed);
    button_create(BUTTON_4, button4_pressed);

    vTaskDelete(NULL);
}

static void eeprom_task(void *args) {
#define AT24CX_SIZE 256

    at24cx_dev_t eeprom_1;

    at24cx_i2c_hal_init();

    ESP_LOGI(TAG, "Initializing AT24CX. . .");

    //Register device
    at24cx_i2c_device_register(&eeprom_1, AT24CX_SIZE, I2C_ADDRESS_AT24CX);

    //Check if eeprom_1 is active
    ESP_LOGI(TAG, "eeprom_1 is %s", eeprom_1.status ? "detected" : "not detected");

    ESP_LOGI(TAG, "Write byte demo: This will write value 0 at address 0x00, 1 at address 0x01 and so on");
    for(int i = 0; i < 10; i++) {
        at24cx_writedata_t dt = { .address = i, .data = i };

        if(at24cx_i2c_byte_write(eeprom_1, dt) == AT24CX_OK)
            ESP_LOGI(TAG, "Writing at address 0x%02X: %d", dt.address, dt.data);
        else
            ESP_LOGE(TAG, "Device write error!");
    }

    ESP_LOGI(TAG, "Read byte demo: Obtain values from addresses 0x00 to 0x09, values should be from 0 to 9 respectively");
    for(int i = 0; i < 10; i++) {
        at24cx_writedata_t dt = {
            .address = i,
        };
        if(at24cx_i2c_byte_read(eeprom_1, &dt) == AT24CX_OK)
            ESP_LOGI(TAG, "Reading at address 0x%02X: %d", dt.address, dt.data);
        else
            ESP_LOGE(TAG, "Device read error!");
    }

    vTaskDelete(NULL);
}

static void joystick_task(void *args) {
    if(joystick_init() != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize joystick");
        vTaskDelete(NULL);
    }

    ESP_LOGI(TAG, "Joystick task started");

    while(1) {
        enum joystick_pos_t pos = joystick_get_position();
        (void) pos;
        // Position is already logged by the driver if logging is enabled
        vTaskDelay(2000 / portTICK_PERIOD_MS);
    }
}

static void buzzer_task(void *args) {

    buzzer_init();

    // This is weird behaviour from LEDC driver. Current fix is to wait some time after initialization
    // so buzzer won't be triggered twice.
    vTaskDelay(500 / portTICK_PERIOD_MS);

    buzzer_set_duty(1000);

    vTaskDelay(500 / portTICK_PERIOD_MS);

    buzzer_set_duty(0);

    vTaskDelete(NULL);
}

/*******************************************************************************/
/*                             INTERRUPT HANDLERS                              */
/*******************************************************************************/


static void button1_pressed(void *args) {
    ESP_LOGI(TAG, "button 1 pressed");
}
static void button2_pressed(void *args) {
    ESP_LOGI(TAG, "button 2 pressed");
}

static void button3_pressed(void *args) {
    ESP_LOGI(TAG, "button 3 pressed");
}

static void button4_pressed(void *args) {
    ESP_LOGI(TAG, "button 4 pressed");
}