#ifndef AT24CX_I2C
#define AT24CX_I2C

#ifdef __cplusplus
extern "C" {
#endif

#include "at24cx_i2c_hal.h"

typedef struct {
    uint8_t data;
    uint8_t data_multi[128];
    uint16_t address;
} at24cx_writedata_t;

typedef struct {
    uint8_t status : 1;
    uint32_t byte_size;
    uint16_t page_write_size;
    uint8_t i2c_addres;
    uint16_t dev_chip;
} at24cx_dev_t;

/**
 * @brief AT24CX device address.
 * @details AT24CX I2C slave address.
 */
#define I2C_ADDRESS_AT24CX 0x50

/**
 * @brief AT24CX device write delay.
 * @details AT24CX self-time write cycle.
 */
#define AT24CX_WRITE_CYCLE_DELAY 5

/**
 * @brief Register device.
 * @details Register device based on specification.
*/
void at24cx_i2c_device_register(uint16_t _dev_chip, uint8_t _i2c_addres);

/**
 * @brief Write word to device.
 * @details Read word to AT24CX.
*/
at24cx_err_t at24cx_i2c_byte_write(at24cx_writedata_t dt);

/**
 * @brief Write multi word to device.
 * @details Write 128 bytes to AT24CX.
*/
at24cx_err_t at24cx_i2c_page_write(at24cx_writedata_t dt);

/**
 * @brief Read word from device.
 * @details Read word from AT24CX.
*/
at24cx_err_t at24cx_i2c_byte_read(at24cx_writedata_t *dt);

/**
 * @brief Read from device.
 * @details Read word from current address of AT24CX.
*/
at24cx_err_t at24cx_i2c_current_address_read(at24cx_dev_t dev, at24cx_writedata_t *dt);

void write_to_eeprom(uint8_t *data, uint8_t size);

#ifdef __cplusplus
}
#endif

#endif /* AT24CX_I2C */
