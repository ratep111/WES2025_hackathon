#ifndef MAIN_AT24CX_I2C_HAL
#define MAIN_AT24CX_I2C_HAL

#ifdef __cplusplus
extern "C" {
#endif

#include "stdint.h"

typedef enum {
    AT24CX_ERR = -1,
    AT24CX_OK,
    AT24CX_NOT_DETECTED,
    AT24CX_INVALID_ADDRESS,
    AT24CX_INVALID_PAGEWRITE_ADDRESS,
} at24cx_err_t;


/**
 * @brief User implementation for I2C initialization.
 * @details To be implemented by user based on hardware platform.
 */
at24cx_err_t at24cx_i2c_hal_init();

/**
 * @brief User implementation for I2C read.
 * @details To be implemented by user based on hardware platform.
 */
at24cx_err_t at24cx_i2c_hal_read(uint8_t address, uint8_t *reg, uint16_t reg_count, uint8_t *data, uint16_t data_count);

/**
 * @brief User implementation for I2C write.
 * @details To be implemented by user based on hardware platform.
 */
at24cx_err_t at24cx_i2c_hal_write(uint8_t address, uint8_t *data, uint16_t count);

/**
 * @brief User implementation for I2C address test.
 * @details To be implemented by user based on hardware platform.
 */
at24cx_err_t at24cx_i2c_hal_test(uint8_t address);

/**
 * @brief User implementation for milliseconds delay.
 * @details To be implemented by user based on hardware platform.
 */
void at24cx_i2c_hal_ms_delay(uint32_t ms);

#ifdef __cplusplus
}
#endif

#endif /* MAIN_AT24CX_I2C_HAL */
