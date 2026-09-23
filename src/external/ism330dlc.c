/*!
 * Copyright � Siemens Healthcare GmbH 2022, All Rights Reserved
 *
 * Project: Building Block Low End MCU
 *
 * @file
 * @brief Provide functionality to communicate with ISM330 inertial module
 */
/*----------------------------------------------------------------------------*/
// HAL includes
#include <hal.h>
// Project includes
#include "bsp_board.h"
#include "ism330dlc.h"

/* Read/Write commands to ism330 peripheral*/
#define ISM330_WRITE 0
#define ISM330_READ  1

/*! Structure for write data over SPI */
typedef struct
{
    uint16_t data : 8;    ///!< Data to be written
    uint16_t reg : 7;     ///!< Register to read/write
    uint16_t read : 1;    ///!< Read/write flag
} ism330dlcWrite;

/*! Structure for read data over SPI */
typedef struct
{
    uint16_t data : 8;        ///!< Read data
    uint16_t reserved : 8;    ///!< Not used
} ism330dlcRead;

// Static functions declaration
static ism330dlcRet ism330dlCheckRegister(const ism330dlcWrite *operationInfo);
static bool         ismSPIDataTransfer(const uint8_t spiNum, ism330dlcWrite *txBuf, ism330dlcRead *rxBuf, uint32_t length);

/* ---------------------------------------------------------------------------- */
/*!
 @brief  Check whether the register is viable for writing/reading

 @param[in]     operationInfo SPI number to communicate
 @return none
*/
/* ---------------------------------------------------------------------------- */
static ism330dlcRet ism330dlCheckRegister(const ism330dlcWrite *operationInfo)
{
    switch (operationInfo->reg)
    {
        case REG_ISM330_FUNC_CFG_ACCESS:
        case REG_ISM330_SENSOR_SYNC_TIME_FRAME:
        case REG_ISM330_SENSOR_SYNC_RES_RATIO:
        case REG_ISM330_FIFO_CTRL1:
        case REG_ISM330_FIFO_CTRL2:
        case REG_ISM330_FIFO_CTRL3:
        case REG_ISM330_FIFO_CTRL4:
        case REG_ISM330_FIFO_CTRL5:
        case REG_ISM330_DRDY_PULSE_CFG:
        case REG_ISM330_INT1_CTRL:
        case REG_ISM330_INT2_CTRL:
        case REG_ISM330_CTRL3_C:
        case REG_ISM330_CTRL4_C:
        case REG_ISM330_CTRL5_C:
        case REG_ISM330_CTRL6_C:
        case REG_ISM330_CTRL7_C:
        case REG_ISM330_CTRL8_C:
        case REG_ISM330_CTRL9_C:
        case REG_ISM330_CTRL10_C:
        case REG_ISM330_MASTER_CONFIG:
        case REG_ISM330_TAP_CFG:
        case REG_ISM330_TAP_THS_6D:
        case REG_ISM330_INT_DUR2:
        case REG_ISM330_WAKE_UP_THS:
        case REG_ISM330_WAKE_UP_DUR:
        case REG_ISM330_FREE_FALL:
        case REG_ISM330_MD1_CFG:
        case REG_ISM330_MD2_CFG:
        case REG_ISM330_MASTER_CMD_CODE:
        case REG_ISM330_SENS_SYNC_SPI_ERR:
        case REG_ISM330_INT_OIS:
        case REG_ISM330_CTRL1_OIS:
        case REG_ISM330_CTRL2_OIS:
        case REG_ISM330_CTRL3_OIS:
        case REG_ISM330_X_OFS_USR:
        case REG_ISM330_Y_OFS_USR:
        case REG_ISM330_Z_OFS_USR:
        case REG_ISM330_CTRL1_XL:
        case REG_ISM330_CTRL2_G: return RET_ISM330_OK;

        case REG_ISM330_WAKE_UP_SRC:
        case REG_ISM330_TAP_SRC:
        case REG_ISM330_D6D_SRC:
        case REG_ISM330_SENSORHUB1:
        case REG_ISM330_SENSORHUB2:
        case REG_ISM330_SENSORHUB3:
        case REG_ISM330_SENSORHUB4:
        case REG_ISM330_SENSORHUB5:
        case REG_ISM330_SENSORHUB6:
        case REG_ISM330_SENSORHUB7:
        case REG_ISM330_SENSORHUB8:
        case REG_ISM330_SENSORHUB9:
        case REG_ISM330_SENSORHUB10:
        case REG_ISM330_SENSORHUB11:
        case REG_ISM330_SENSORHUB12:
        case REG_ISM330_SENSORHUB13:
        case REG_ISM330_SENSORHUB14:
        case REG_ISM330_SENSORHUB15:
        case REG_ISM330_SENSORHUB16:
        case REG_ISM330_SENSORHUB17:
        case REG_ISM330_SENSORHUB18:
        case REG_ISM330_FUNC_SRC2:
        case REG_ISM330_FUNC_SRC1:
        case REG_ISM330_FIFO_STATUS1:
        case REG_ISM330_FIFO_STATUS2:
        case REG_ISM330_FIFO_STATUS3:
        case REG_ISM330_FIFO_STATUS4:
        case REG_ISM330_FIFO_DATA_OUT_L:
        case REG_ISM330_FIFO_DATA_OUT_H:
        case REG_ISM330_TIMESTAMP0:
        case REG_ISM330_TIMESTAMP1:
        case REG_ISM330_TIMESTAMP2:
        case REG_ISM330_STATUS:
        case REG_ISM330_WHOAMI:
        case REG_ISM330_OUT_TEMP_L:
        case REG_ISM330_OUT_TEMP_H:
        case REG_ISM330_OUTX_L_G:
        case REG_ISM330_OUTX_H_G:
        case REG_ISM330_OUTY_L_G:
        case REG_ISM330_OUTY_H_G:
        case REG_ISM330_OUTZ_L_G:
        case REG_ISM330_OUTZ_H_G:
        case REG_ISM330_OUTX_L_XL:
        case REG_ISM330_OUTX_H_XL:
        case REG_ISM330_OUTY_L_XL:
        case REG_ISM330_OUTY_H_XL:
        case REG_ISM330_OUTZ_L_XL:
        case REG_ISM330_OUTZ_H_XL:
        case REG_ISM330_OUT_MAG_RAW_X_L:
        case REG_ISM330_OUT_MAG_RAW_X_H:
        case REG_ISM330_OUT_MAG_RAW_Y_L:
        case REG_ISM330_OUT_MAG_RAW_Y_H:
        case REG_ISM330_OUT_MAG_RAW_Z_L:
        case REG_ISM330_OUT_MAG_RAW_Z_H:
            if (operationInfo->read == ISM330_READ)
            {
                return RET_ISM330_OK;
            }
            return RET_ISM330_READONLY;
        default: return RET_ISM330_NOT_ACCESSIBLE;
    }
}

/* ---------------------------------------------------------------------------- */
/*!
 @brief  Read 8 bit register

 @param[in]     spi SPI number to communicate
 @param[in]     reg Register number
 @param[out]    data Pinter to data container
 @return none
*/
/* ---------------------------------------------------------------------------- */
ism330dlcRet ism330dlcReadRegister8bit(uint8_t spi, ism330dlcReg reg, uint8_t *data)
{
    ism330dlcWrite write = { .read = ISM330_READ, .reg = (uint16_t)reg };
    ism330dlcRead  read;
    ism330dlcRet   check = ism330dlCheckRegister(&write);
    if (check != RET_ISM330_OK)
    {
        return check;
    }
    if (!ismSPIDataTransfer(spi, &write, &read, 1))
    {
        return RET_ISM330_ESPI;
    }

    *data = read.data;

    return RET_ISM330_OK;
}

/* ---------------------------------------------------------------------------- */
/*!
 @brief  Read 16 bit register

 @param[in]     spi SPI number to communicate
 @param[in]     reg Register number
 @param[out]    data Pinter to data container
 @return none
*/
/* ---------------------------------------------------------------------------- */
ism330dlcRet ism330dlcReadRegister16bit(uint8_t spi, ism330dlcReg reg, uint16_t *data)
{
    ism330dlcWrite write = { .read = ISM330_READ, .reg = (uint16_t)reg };
    ism330dlcRead  read;
    ism330dlcRet   check = ism330dlCheckRegister(&write);
    if (check != RET_ISM330_OK)
    {
        return check;
    }
    // Read first register
    if (!ismSPIDataTransfer(spi, &write, &read, 1))
    {
        return RET_ISM330_ESPI;
    }

    *data = read.data;

    // Read second register
    write.reg++;

    if (!ismSPIDataTransfer(spi, &write, &read, 1))
    {
        return RET_ISM330_ESPI;
    }
    *data |= ((uint32_t)read.data << 8);

    return RET_ISM330_OK;
}

/* ---------------------------------------------------------------------------- */
/*!
 @brief  Write 8 bit register

 @param[in]     spi SPI number to communicate
 @param[in]     reg Register number
 @param[out]    data Data to write
 @return none
*/
/* ---------------------------------------------------------------------------- */
ism330dlcRet ism330dlcWriteRegister8bit(uint8_t spi, ism330dlcReg reg, uint8_t data)
{
    ism330dlcWrite write = { .read = ISM330_WRITE, .reg = (uint16_t)reg, .data = data };
    ism330dlcRead  read;
    ism330dlcRet   check = ism330dlCheckRegister(&write);
    if (check != RET_ISM330_OK)
    {
        return check;
    }
    if (!ismSPIDataTransfer(spi, &write, &read, 1))
    {
        return RET_ISM330_ESPI;
    }


    return RET_ISM330_OK;
}

/*----------------------------------------------------------------------------*/
/*!
  @brief        Transer the data via SPI line

  @param[in]    spiNum  number of spi interface used
  @param[in]    txBuf   pointer to write buffer
  @param[in]    rxBuf   pointer to read buffer
  @param[in]    length  data length
  @return       true if transfer successful, false otherwise
*/
/*----------------------------------------------------------------------------*/
static bool ismSPIDataTransfer(const uint8_t spiNum, ism330dlcWrite *txBuf, ism330dlcRead *rxBuf, uint32_t length)
{
    boardSpiXfer xfer = {
        .spi     = spiNum,
        .wrBuf   = txBuf,
        .rdBuf   = rxBuf,
        .size    = (uint16_t)length,
        .timeout = 200,
    };

    return boardSpiTransfer(&xfer);
}
