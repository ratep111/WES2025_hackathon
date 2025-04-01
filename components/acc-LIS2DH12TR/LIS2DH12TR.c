/*******************************************************************************/
/*                                  INCLUDES                                   */
/*******************************************************************************/

#include "LIS2DH12TR.h"
#include "LIS2DH12TR_core.h"

#include <stdint.h>
#include <string.h>

#include "esp_err.h"
#include "esp_log.h"
#include "driver/spi_master.h"

/*******************************************************************************/
/*                                   MACROS                                     */
/*******************************************************************************/

#define LIS2DH12TR_LOG_TAG ("LIS2DH12TR")

#define LIS2DH12TR_SPI_freqeuncy  (1000000)
#define LIS2DH12TR_SPI_IO_NUM     (13)
#define LIS2DH12TR_SPI_QUEUE_SIZE (1)

/*******************************************************************************/
/*                                 DATA TYPES                                  */
/*******************************************************************************/

/*******************************************************************************/
/*                         PRIVATE FUNCTION PROTOTYPES                         */
/*******************************************************************************/

/**
 * @brief
 * Internal function that connects abstract Core driver funcionality.
 * This function is an abstract connector for the SPI writing API
 *  
 * @param handle Handle of the SPI device
 * @param Reg Registry to which data needs to be written onto
 * @param Bufp Buffer that is being written
 * @param len Length of the buffer
 * @return Internal status of writing opperation (1 - error | 0 - ok)
 */
int32_t _lsi2dh12_core_write(void *handle, uint8_t Reg, const uint8_t *Bufp, uint16_t len);

/**
 * @brief
 * Internal function that connects abstract Core driver functionality
 * This function is an abstract connector for SPI reading API
 * 
 * @param handle Handle of the SPI device
 * @param Reg Registry from which data is being read from
 * @param Bufp Buffer that is being read into
 * @param len Length of the buffer
 * @return Internal status of reading opperation (1 - error | 0 - ok)
 */
int32_t _lsi2dh12_core_read(void *handle, uint8_t Reg, uint8_t *Bufp, uint16_t len);

/*******************************************************************************/
/*                          STATIC DATA & CONSTANTS                            */
/*******************************************************************************/

/**
 * @brief 
 * Variable that stores context of the LIS2DH12TR sensor. 
 * Core driver needs it to operate correctly.
 */
static stmdev_ctx_t _lsi2dh12_core_ctx;

/**
 * @brief 
 * Variable that consists all needed parameters for the SPI bus
 * to function proprerlly while doing read/write operations
 */
static spi_device_handle_t _esp_spi_hdev;

/*******************************************************************************/
/*                                 GLOBAL DATA                                 */
/*******************************************************************************/

/*******************************************************************************/
/*                              PUBLIC FUNCTIONS                               */
/*******************************************************************************/

LIS2DH12TR_init_status LIS2DH12TR_init()
{
    spi_device_interface_config_t spi_device_config = {
        .clock_speed_hz = LIS2DH12TR_SPI_freqeuncy,
        .mode = 0,
        .spics_io_num = LIS2DH12TR_SPI_IO_NUM,
        .queue_size = LIS2DH12TR_SPI_QUEUE_SIZE,
        .flags = 0,
        .pre_cb = NULL,
        .post_cb = NULL,
        .address_bits = 8
    };

    esp_err_t _err_value = spi_bus_add_device(VSPI_HOST, &spi_device_config, &_esp_spi_hdev);
    
    if (_err_value != ESP_OK) 
    {
        ESP_LOGE(
            LIS2DH12TR_LOG_TAG, 
            "SPI bus couldn't be initialized, error cause: %s",
            esp_err_to_name(_err_value)
        );
        return LIS2DH12TR_SPI_ERROR;
    }

    _lsi2dh12_core_ctx.write_reg = _lsi2dh12_core_write;
    _lsi2dh12_core_ctx.read_reg  = _lsi2dh12_core_read;
    _lsi2dh12_core_ctx.handle = &_esp_spi_hdev;

    uint8_t dev_id;
    lis2dh12_device_id_get(&_lsi2dh12_core_ctx, &dev_id);

    if (dev_id != LIS2DH12_ID) 
    {
        ESP_LOGE(
            LIS2DH12TR_LOG_TAG,
            "Received an unexpected ID from the device"
        );
        return LIS2DH12TR_ID_MISMATCH;
    }

    ESP_LOGI(LIS2DH12TR_LOG_TAG, "Sensor ID: %hhu\n", dev_id);

    // Asigning a specific setup parameters

    lis2dh12_block_data_update_set(&_lsi2dh12_core_ctx, PROPERTY_ENABLE);
    lis2dh12_data_rate_set(&_lsi2dh12_core_ctx, LIS2DH12_ODR_1Hz);
    lis2dh12_full_scale_set(&_lsi2dh12_core_ctx, LIS2DH12_8g);
    lis2dh12_operating_mode_set(&_lsi2dh12_core_ctx, LIS2DH12_HR_12bit);

    return LIS2DH12TR_OK;
}

LIS2DH12TR_reading_status LIS2DH12TR_read_acc(LIS2DH12TR_accelerations *acc_output)
{
    int16_t data_raw_acceleration[3];

    lis2dh12_reg_t reg;
    if (lis2dh12_xl_data_ready_get(&_lsi2dh12_core_ctx, &reg.byte) != 0) 
    {
        ESP_LOGE(
            LIS2DH12TR_LOG_TAG,
            "Error while obtaining raw data from the device..."
        );
        return LIS2DH12TR_READING_ERROR;
    }

    if (reg.byte) 
    {
        memset(data_raw_acceleration, 0x00, 3 * sizeof(int16_t));
        lis2dh12_acceleration_raw_get(&_lsi2dh12_core_ctx, data_raw_acceleration);
        
        acc_output->x_acc = lis2dh12_from_fs8_hr_to_mg(data_raw_acceleration[0]) / 1000.f;
        acc_output->y_acc = lis2dh12_from_fs8_hr_to_mg(data_raw_acceleration[1]) / 1000.f;
        acc_output->z_acc = lis2dh12_from_fs8_hr_to_mg(data_raw_acceleration[2]) / 1000.f;

        return LIS2DH12TR_READING_OK;
    }
    else 
    {
        ESP_LOGE(
            LIS2DH12TR_LOG_TAG,
            "Received data was empty"
        );
        return LIS2DH12TR_READING_EMPTY;
    }
}

/*******************************************************************************/
/*                             PRIVATE FUNCTIONS                               */
/*******************************************************************************/

int32_t _lsi2dh12_core_write(void *handle, uint8_t Reg, const uint8_t *Bufp, uint16_t len)
{
    spi_transaction_t spi_tranaction = {
        .addr = Reg | 0x60,
        .tx_buffer = Bufp,
        .length = 8 * len
    };

    esp_err_t _err_value = spi_device_polling_transmit(*(spi_device_handle_t*) handle, &spi_tranaction);

    if (_err_value != ESP_OK) {
        ESP_LOGE(
            LIS2DH12TR_LOG_TAG, 
            "SPI transmition failed while writing to a device register, error cause: %s",
            esp_err_to_name(_err_value)
        );
        return 1;
    }
    
    return 0;
}

int32_t _lsi2dh12_core_read(void *handle, uint8_t Reg, uint8_t *Bufp, uint16_t len)
{
    spi_transaction_t spi_tranaction = {
        .addr = Reg | 0xC0,
        .rx_buffer = Bufp,
        .rxlength = 0,
        .length = 8 * len
    };

    esp_err_t _err_value = spi_device_polling_transmit(*(spi_device_handle_t*) handle, &spi_tranaction);

    if (_err_value != ESP_OK) {
        ESP_LOGE(
            LIS2DH12TR_LOG_TAG, 
            "SPI transmition failed while reading from the device register, error cause: %s",
            esp_err_to_name(_err_value)
        );
        return 1;
    }
    
    return 0;
}

/*******************************************************************************/
/*                             INTERRUPT HANDLERS                              */
/*******************************************************************************/
