#ifndef HEADER_FILE_H
#define HEADER_FILE_H

/*******************************************************************************/
/*                                  INCLUDES                                    */
/*******************************************************************************/

/*******************************************************************************/
/*                                   MACROS                                    */
/*******************************************************************************/

/*******************************************************************************/
/*                                 DATA TYPES                                  */
/*******************************************************************************/

typedef enum
{
    LIS2DH12TR_SPI_ERROR,
    LIS2DH12TR_ID_MISMATCH,
    LIS2DH12TR_OK    
} LIS2DH12TR_init_status;

typedef enum
{
    LIS2DH12TR_READING_ERROR,
    LIS2DH12TR_READING_EMPTY,
    LIS2DH12TR_READING_OK
} LIS2DH12TR_reading_status;

typedef struct
{
    float x_acc; // Acceleration in X axis [G]
    float y_acc; // Acceleration in Y axis [G]
    float z_acc; // Acceleration in Z axis [G]
} LIS2DH12TR_accelerations;


/*******************************************************************************/
/*                             PUBLIC DEFINITIONS                              */
/*******************************************************************************/

/**
 * @brief 
 * Initialisator function of the LIS2DH12TR sensor
 * 
 * @return 
 * Status of the initalisator function
 */
LIS2DH12TR_init_status LIS2DH12TR_init();

/**
 * @brief 
 * Read acceleration values from the LIS2DH12TR sensor
 * 
 * @param acc_output Reference to a structure with the x,y,z accelerations stored in terms of G-s
 * @return Status of the reading process 
 */
LIS2DH12TR_reading_status LIS2DH12TR_read_acc(LIS2DH12TR_accelerations *acc_output);

/*******************************************************************************/
/*                          PUBLIC FUNCTION PROTOTYPES                         */
/*******************************************************************************/

#endif /* HEADER_FILE_H */