/*******************************************************************************/
/*                                  INCLUDES                                   */
/*******************************************************************************/

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2c.h"

#include "gui/gui.h"
#include "LIS2DH12TR.h"
#include "sht3x.h"
#include "pcf8523.h"
#include "button.h"
#include "at24cx_i2c.h"
#include "joystick.h"
#include "led.h"
#include "buzzer.h"
#include "esp32_perfmon.h"
#include "tcrt5000.h"
#include "veml7700.h"
#include "ultrasonic.h"

#include "parking_sensor.h"
#include "day_night_detector.h"
#include "door_detector.h"
#include "speed_estimator.h"
#include "crash_detector.h"

#include "i2cdev.h"
#include "pcf8574.h"
#include "speaker.h"
#include "my_mqtt.h"

/*******************************************************************************/
/*                                   MACROS                                     */
/*******************************************************************************/

#define EXPANDER_I2C_ADDR 0x20 // From schematic
#define I2C_PORT          I2C_NUM_0
#define SDA_GPIO          GPIO_NUM_22 // Check schematic if different
#define SCL_GPIO          GPIO_NUM_21

i2c_dev_t expander;
uint8_t expander_state;

#define TAG                  "MAIN"
#define TEMP_TASK_STACK_SIZE 2048
#define TEMP_TASK_PRIORITY   5
#define TEMP_READ_DELAY_MS   2000

// TCRT5000 configuration
#define TCRT5000_DIGITAL_PIN GPIO_NUM_35
#define TCRT5000_ADC_CHANNEL ADC1_CHANNEL_6
#define TCRT5000_THRESHOLD   2000 // Threshold in mV

/*******************************************************************************/
/*                                 DATA TYPES                                  */
/*******************************************************************************/

/*******************************************************************************/
/*                         PRIVATE FUNCTION PROTOTYPES                         */
/*******************************************************************************/

/*******************************************************************************/
/*                          STATIC DATA & CONSTANTS                            */
/*******************************************************************************/

static const i2c_config_t _i2c_config = {
    .mode             = I2C_MODE_MASTER,
    .scl_io_num       = GPIO_NUM_21,
    .sda_io_num       = GPIO_NUM_22,
    .sda_pullup_en    = GPIO_PULLUP_ENABLE,
    .scl_pullup_en    = GPIO_PULLUP_ENABLE,
    .master.clk_speed = 50000,
    .clk_flags        = 0,
};

/*******************************************************************************/
/*                                 GLOBAL DATA                                 */
/*******************************************************************************/

/*******************************************************************************/
/*                              PUBLIC FUNCTIONS                               */
/*******************************************************************************/

void initialization_peripheral_creator() {
    ESP_LOGI(TAG, "System boot...");


    // --- Init i2cdev mutex system (MUST come before any pcf8574 or other i2cdev use) ---
    i2cdev_init();

    // --- I2C Master Init ---
    ESP_LOGI(TAG, "Initializing I2C master...");
    esp_err_t i2c_ret;

    i2c_ret = i2c_param_config(I2C_MASTER_NUM, &_i2c_config);
    if(i2c_ret != ESP_OK) {
        ESP_LOGE(TAG, "I2C param config failed: %s", esp_err_to_name(i2c_ret));
        return;
    }

    i2c_ret = i2c_driver_install(I2C_MASTER_NUM, _i2c_config.mode, I2C_MASTER_RX_BUF_DISABLE, I2C_MASTER_TX_BUF_DISABLE, 0);
    if(i2c_ret != ESP_OK) {
        ESP_LOGE(TAG, "I2C driver install failed: %s", esp_err_to_name(i2c_ret));
        return;
    }

    // --- PCF8574 I/O Expander Init ---
    esp_err_t err = pcf8574_init_desc(&expander, EXPANDER_I2C_ADDR, I2C_PORT, SDA_GPIO, SCL_GPIO);
    if(err != ESP_OK) {
        ESP_LOGE("EXPANDER", "Failed to init PCF8574 descriptor: %s", esp_err_to_name(err));
        return;
    }

    expander_state = 0x00;
    err            = pcf8574_port_write(&expander, expander_state); // Set all I/O expander pins LOW
    if(err != ESP_OK) {
        ESP_LOGE("EXPANDER", "Failed to write to PCF8574: %s", esp_err_to_name(err));
        return;
    }

    ESP_LOGI("EXPANDER", "PCF8574 initialized and all pins set LOW");

    // --- Start I2C Temperature/Humidity Sensor ---
    if(sht3x_start_periodic_measurement() != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start SHT3x periodic measurement!");
        return;
    }

    ESP_ERROR_CHECK(mqtt_client_init());

    // --- Initialize UI + perf monitor ---
    gui_init();
    perfmon_start();
    i2s_dac_init();

    vTaskDelay(pdMS_TO_TICKS(1000));
}

void initailization_task_creator() {

    ESP_LOGI(TAG, "Launching sensor tasks...");


    xTaskCreatePinnedToCore(parking_sensor_task, "parking_sensor", 4096, NULL, 5, NULL, 0);
    xTaskCreatePinnedToCore(day_night_task, "day_night_sensor", 4096, NULL, 5, NULL, 0);
    xTaskCreatePinnedToCore(door_detector_task, "door_detector", 4096, NULL, 5, NULL, 0);

    vTaskDelay(pdMS_TO_TICKS(2000));

    // --- Crash detector ---
    // ESP_ERROR_CHECK(crash_detector_init());
    // xTaskCreatePinnedToCore(crash_detector_task, "crash_detector", 4096, NULL, 5, NULL, 0);
    xTaskCreatePinnedToCore(audio_task, "audioTask", 4096, NULL, 5, NULL, 0);
    // xTaskCreatePinnedToCore(speed_estimator_task, "speedEstimator", 4096, NULL, 5, NULL, 0);

    ESP_LOGI(TAG, "All sensor tasks started.");
}

/*******************************************************************************/
/*                             PRIVATE FUNCTIONS                               */
/*******************************************************************************/

/*******************************************************************************/
/*                             INTERRUPT HANDLERS                              */
/*******************************************************************************/
