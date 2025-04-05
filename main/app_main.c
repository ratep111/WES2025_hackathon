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

/*******************************************************************************/
/*                                   MACROS                                     */
/*******************************************************************************/

#define TAG                  "MAIN"
#define TEMP_TASK_STACK_SIZE 2048
#define TEMP_TASK_PRIORITY   5
#define TEMP_READ_DELAY_MS   2000

// TCRT5000 configuration
#define TCRT5000_DIGITAL_PIN GPIO_NUM_35
#define TCRT5000_ADC_CHANNEL ADC1_CHANNEL_6
#define TCRT5000_THRESHOLD   2000 // Threshold in mV

// HC-SR04 configuration
// #define ULTRASONIC_TRIGGER_PIN GPIO_NUM_27
// #define ULTRASONIC_ECHO_PIN    GPIO_NUM_26
// #define MAX_DISTANCE_CM        400 // Maximum distance in cm

/*******************************************************************************/
/*                                 DATA TYPES                                  */
/*******************************************************************************/

/*******************************************************************************/
/*                         PRIVATE FUNCTION PROTOTYPES                         */
/*******************************************************************************/
static void accelerometer_task(void *args);
static void temp_sens_task(void *args);
static void rtc_task(void *args);
static void button_task(void *args);
static void eeprom_task(void *args);
static void joystick_task(void *args);
static void buzzer_task(void *args);
static void led_task(void *args);
static void tcrt5000_task(void *arg);
static void veml7700_task(void *arg);
static void ultrasonic_task(void *arg);
// static void ultrasonic_i2c_task(void *arg);

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
    .master.clk_speed = 50000,
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

    vTaskDelay(1000 / portTICK_PERIOD_MS);

    ESP_LOGI(TAG, "Creating temperature sensor task...");
    xTaskCreate(temp_sens_task, "temp_sens_task", TEMP_TASK_STACK_SIZE, NULL, TEMP_TASK_PRIORITY, NULL);
    xTaskCreate(rtc_task, "rtc_task", TEMP_TASK_STACK_SIZE, NULL, TEMP_TASK_PRIORITY, NULL);
    xTaskCreate(button_task, "button_task", TEMP_TASK_STACK_SIZE, NULL, TEMP_TASK_PRIORITY, NULL);
    xTaskCreate(eeprom_task, "eeprom_task", 2 * TEMP_TASK_STACK_SIZE, NULL, TEMP_TASK_PRIORITY, NULL);
    //xTaskCreate(joystick_task, "joystick_task", TEMP_TASK_STACK_SIZE, NULL, TEMP_TASK_PRIORITY, NULL);
    xTaskCreate(buzzer_task, "buzzer_task", TEMP_TASK_STACK_SIZE, NULL, TEMP_TASK_PRIORITY, NULL);
    //xTaskCreate(led_task, "led_task", TEMP_TASK_STACK_SIZE, NULL, TEMP_TASK_PRIORITY, NULL);
    //xTaskCreate(accelerometer_task, "accelerometer_task", 4096, NULL, 5, NULL);
    // xTaskCreate(tcrt5000_task, "tcrt5000_task", 4096, NULL, 5, NULL);
    // xTaskCreate(veml7700_task, "veml7700_task", 4096, NULL, 5, NULL);
    // xTaskCreate(ultrasonic_task, "ultrasonic_task", 4096, NULL, 5, NULL);

    xTaskCreate(speed_estimator_task, "speed_estimator", 4096, NULL, 5, NULL);
    xTaskCreate(parking_sensor_task, "parking_sensor", 4096, NULL, 5, NULL);
    xTaskCreate(day_night_task, "day_night_sensor", 4096, NULL, 5, NULL);
    xTaskCreate(door_detector_task, "door_detector", 4096, NULL, 5, NULL);

    ESP_LOGI(TAG, "All sensor tasks started");
}

/*******************************************************************************/
/*                             PRIVATE FUNCTIONS                               */
/*******************************************************************************/

static void accelerometer_task(void *args) {
    LIS2DH12TR_accelerations acc = { 0 };

    // This delay ensures gui_init is done before calling LIS..._init()
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    LIS2DH12TR_init();

    while(1) {
        LIS2DH12TR_read_acc(&acc);
        ESP_LOGI(TAG, "x: %f, y: %f, z: %f", acc.x_acc, acc.y_acc, acc.z_acc);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}

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

    vTaskDelete(NULL);
}

static void led_task(void *args) {

    led_t leds[] = { LED_RED, LED_GREEN, LED_BLUE };

    for(int i = 0; i < LED_COUNT; i++) {
        if(led_init(leds[i]) != ESP_OK) {
            ESP_LOGE(TAG, "Failed to initialize LED %d", i);
            vTaskDelete(NULL);
        }
    }

    ESP_LOGI(TAG, "Initalized LEDs!");

    while(1) {

        led_on(LED_RED);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        led_off(LED_RED);

        led_on(LED_GREEN);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        led_off(LED_GREEN);

        led_on(LED_BLUE);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        led_off(LED_BLUE);
    }
}

static void tcrt5000_task(void *arg) {
    // Configure TCRT5000 with digital output
    tcrt5000_config_t digital_config = {
        .use_digital   = true,
        .digital_pin   = TCRT5000_DIGITAL_PIN,
        .invert_output = false // Try inverting this to true if detection logic is reversed
    };

    tcrt5000_handle_t digital_sensor;

    esp_err_t ret = tcrt5000_init(&digital_config, &digital_sensor);
    if(ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize TCRT5000 digital sensor: %s", esp_err_to_name(ret));
        vTaskDelete(NULL);
    }

    ESP_LOGI(TAG, "TCRT5000 sensors initialized successfully");

    // Add debug to check the actual GPIO level
    ESP_LOGI(TAG, "Initial GPIO level: %d", gpio_get_level(TCRT5000_DIGITAL_PIN));

    while(1) {
        // Read digital sensor
        bool digital_detected;
        ESP_ERROR_CHECK(tcrt5000_read_digital(&digital_sensor, &digital_detected));

        // Also read raw GPIO level for debugging
        int gpio_level = gpio_get_level(TCRT5000_DIGITAL_PIN);

        ESP_LOGI(TAG, "TCRT5000: Digital=%s (raw GPIO=%d)", digital_detected ? "Detected" : "Not detected", gpio_level);

        vTaskDelay(2000 / portTICK_PERIOD_MS);
    }
}
/**
 * @brief Task to handle VEML7700 sensor readings
 */
static void veml7700_task(void *arg) {
    veml7700_handle_t dev;

    esp_err_t ret = veml7700_initialize(&dev, I2C_MASTER_NUM);
    if(ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize VEML7700 sensor: %s", esp_err_to_name(ret));
        vTaskDelete(NULL);
    }

    ESP_LOGI(TAG, "VEML7700 sensor initialized successfully");

    // Allow some time for the sensor to initialize
    vTaskDelay(100 / portTICK_PERIOD_MS);

    while(1) {
        double light_lux;

        // Read ambient light with auto scaling
        ret = veml7700_read_als_lux_auto(dev, &light_lux);
        if(ret == ESP_OK) {
            ESP_LOGI(TAG, "VEML7700: Light = %.2f lux", light_lux);
        } else {
            ESP_LOGE(TAG, "Failed to read VEML7700 sensor: %s", esp_err_to_name(ret));
        }

        vTaskDelay(2000 / portTICK_PERIOD_MS);
    }
}

// static void ultrasonic_i2c_task(void *arg) {
//     esp_err_t ret = ultrasonic_i2c_init(I2C_MASTER_NUM);
//     if(ret != ESP_OK) {
//         ESP_LOGE(TAG, "Failed to initialize HC-SR04 I2C sensor: %s", esp_err_to_name(ret));
//         vTaskDelete(NULL);
//     }

//     ESP_LOGI(TAG, "HC-SR04 I2C sensor initialized successfully");

//     while(1) {
//         uint16_t distance = 0;

//         ret = ultrasonic_i2c_measure_cm(I2C_MASTER_NUM, &distance);
//         if(ret == ESP_OK) {
//             ESP_LOGI(TAG, "HC-SR04 I2C: Distance = %u cm", distance);
//         } else {
//             ESP_LOGW(TAG, "Failed to read HC-SR04 I2C sensor: %s", esp_err_to_name(ret));
//         }

//         vTaskDelay(1000 / portTICK_PERIOD_MS);
//     }
// }

// static void ultrasonic_task(void *arg) {
//     ultrasonic_sensor_t sensor = { .trigger_pin = ULTRASONIC_TRIGGER_PIN, .echo_pin = ULTRASONIC_ECHO_PIN };

//     esp_err_t ret = ultrasonic_init(&sensor);
//     if(ret != ESP_OK) {
//         ESP_LOGE(TAG, "Failed to initialize HC-SR04 sensor: %s", esp_err_to_name(ret));
//         vTaskDelete(NULL);
//     }

//     ESP_LOGI(TAG, "HC-SR04 sensor initialized successfully");

//     while(1) {
//         uint32_t distance;
//         uint32_t time_us;

//         // Try measuring raw time first
//         ret = ultrasonic_measure_raw(&sensor, MAX_DISTANCE_CM * 58, &time_us);
//         if(ret == ESP_OK) {
//             ESP_LOGI(TAG, "HC-SR04: Raw time = %lu us", time_us);
//         } else {
//             ESP_LOGW(TAG, "Failed to read HC-SR04 raw timing: %s (error code: %d)", esp_err_to_name(ret), ret);
//         }

//         vTaskDelay(100 / portTICK_PERIOD_MS); // Longer delay between measurements

//         // Then try distance calculation
//         ret = ultrasonic_measure_cm(&sensor, MAX_DISTANCE_CM, &distance);
//         if(ret == ESP_OK) {
//             ESP_LOGI(TAG, "HC-SR04: Distance = %lu cm", distance);
//         } else {
//             ESP_LOGW(TAG, "Failed to read HC-SR04 sensor: %s (error code: %d)", esp_err_to_name(ret), ret);

//             // Check for specific errors
//             if(ret == ESP_ERR_ULTRASONIC_PING) {
//                 ESP_LOGW(TAG, "Ping error - sensor might be busy or incorrectly connected");
//             } else if(ret == ESP_ERR_ULTRASONIC_PING_TIMEOUT) {
//                 ESP_LOGW(TAG, "Ping timeout - no trigger pulse detected");
//             } else if(ret == ESP_ERR_ULTRASONIC_ECHO_TIMEOUT) {
//                 ESP_LOGW(TAG, "Echo timeout - no echo received, check connections and obstacles");
//             }
//         }

//         vTaskDelay(3000 / portTICK_PERIOD_MS);
//     }
// }

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