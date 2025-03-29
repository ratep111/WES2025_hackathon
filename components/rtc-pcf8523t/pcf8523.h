#include <time.h>
#include "driver/i2c.h"

#define PCF8523_I2C_ADDR 0x68
#define I2C_FREQ_HZ      400000

// Status flags
typedef enum {
    PCF8523_OK                 = 0,
    PCF8523_OSCILLATOR_STOPPED = 1,
    PCF8523_ERROR              = 2
} pcf8523_status_t;

esp_err_t pcf8523_init(i2c_port_t port, gpio_num_t sda_gpio, gpio_num_t scl_gpio);
esp_err_t pcf8523_set_time(const struct tm *time);
esp_err_t pcf8523_get_time(struct tm *time);
esp_err_t pcf8523_check_status(pcf8523_status_t *status);