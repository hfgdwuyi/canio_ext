/*!
 * Copyright � Siemens Healthcare GmbH 2022, All Rights Reserved
 *
 * Project: Building Block Low End MCU
 *
 * @file
 * @brief          Header file for ism330dlc.c
 */
/*----------------------------------------------------------------------------*/
#ifndef ISM330DLC_H
#define ISM330DLC_H

typedef enum
{
    CONF_ISM330_XL_ODR_PW_DOWN  = 0x00, 
    CONF_ISM330_XL_ODR_1_6_HZ   = 0xB0, //12.5 Hz in high performance mode
    CONF_ISM330_XL_ODR_12_5_HZ  = 0x10,
    CONF_ISM330_XL_ODR_26_HZ    = 0x20,
    CONF_ISM330_XL_ODR_52_HZ    = 0x30,
    CONF_ISM330_XL_ODR_104_HZ   = 0x40,
    CONF_ISM330_XL_ODR_208_HZ   = 0x50,
    CONF_ISM330_XL_ODR_416_HZ   = 0x60,
    CONF_ISM330_XL_ODR_833_HZ   = 0x70,
    CONF_ISM330_XL_ODR_1_66_KHZ = 0x80,
    CONF_ISM330_XL_ODR_3_33_KHZ = 0x90,
    CONF_ISM330_XL_ODR_6_66_KHZ = 0xA0,

    CONF_ISM330_XL_SCALE_2G  = 0x00,
    CONF_ISM330_XL_SCALE_4G  = 0x08,
    CONF_ISM330_XL_SCALE_8G  = 0x0C,
    CONF_ISM330_XL_SCALE_16G = 0x04,
} ism330dlcConf;

/*! ISM330 registers */
typedef enum
{
    REG_ISM330_FUNC_CFG_ACCESS        = 0x01,
    REG_ISM330_SENSOR_SYNC_TIME_FRAME = 0x04,
    REG_ISM330_SENSOR_SYNC_RES_RATIO  = 0x05,
    REG_ISM330_FIFO_CTRL1             = 0x06,
    REG_ISM330_FIFO_CTRL2             = 0x07,
    REG_ISM330_FIFO_CTRL3             = 0x08,
    REG_ISM330_FIFO_CTRL4             = 0x09,
    REG_ISM330_FIFO_CTRL5             = 0x0A,
    REG_ISM330_DRDY_PULSE_CFG         = 0x0B,
    REG_ISM330_INT1_CTRL              = 0x0D,
    REG_ISM330_INT2_CTRL              = 0x0E,
    REG_ISM330_WHOAMI                 = 0x0F,
    REG_ISM330_CTRL1_XL               = 0x10,
    REG_ISM330_CTRL2_G                = 0x11,
    REG_ISM330_CTRL3_C                = 0x12,
    REG_ISM330_CTRL4_C                = 0x13,
    REG_ISM330_CTRL5_C                = 0x14,
    REG_ISM330_CTRL6_C                = 0x15,
    REG_ISM330_CTRL7_C                = 0x16,
    REG_ISM330_CTRL8_C                = 0x17,
    REG_ISM330_CTRL9_C                = 0x18,
    REG_ISM330_CTRL10_C               = 0x19,
    REG_ISM330_MASTER_CONFIG          = 0x1A,
    REG_ISM330_WAKE_UP_SRC            = 0x1B,
    REG_ISM330_TAP_SRC                = 0x1C,
    REG_ISM330_D6D_SRC                = 0x1D,
    REG_ISM330_STATUS                 = 0x1E,
    REG_ISM330_OUT_TEMP_L             = 0x20,
    REG_ISM330_OUT_TEMP_H             = 0x21,
    REG_ISM330_OUTX_L_G               = 0x22,
    REG_ISM330_OUTX_H_G               = 0x23,
    REG_ISM330_OUTY_L_G               = 0x24,
    REG_ISM330_OUTY_H_G               = 0x25,
    REG_ISM330_OUTZ_L_G               = 0x26,
    REG_ISM330_OUTZ_H_G               = 0x27,
    REG_ISM330_OUTX_L_XL              = 0x28,
    REG_ISM330_OUTX_H_XL              = 0x29,
    REG_ISM330_OUTY_L_XL              = 0x2A,
    REG_ISM330_OUTY_H_XL              = 0x2B,
    REG_ISM330_OUTZ_L_XL              = 0x2C,
    REG_ISM330_OUTZ_H_XL              = 0x2D,
    REG_ISM330_SENSORHUB1             = 0x2E,
    REG_ISM330_SENSORHUB2             = 0x2F,
    REG_ISM330_SENSORHUB3             = 0x30,
    REG_ISM330_SENSORHUB4             = 0x31,
    REG_ISM330_SENSORHUB5             = 0x32,
    REG_ISM330_SENSORHUB6             = 0x33,
    REG_ISM330_SENSORHUB7             = 0x34,
    REG_ISM330_SENSORHUB8             = 0x35,
    REG_ISM330_SENSORHUB9             = 0x36,
    REG_ISM330_SENSORHUB10            = 0x37,
    REG_ISM330_SENSORHUB11            = 0x38,
    REG_ISM330_SENSORHUB12            = 0x39,
    REG_ISM330_FIFO_STATUS1           = 0x3A,
    REG_ISM330_FIFO_STATUS2           = 0x3B,
    REG_ISM330_FIFO_STATUS3           = 0x3C,
    REG_ISM330_FIFO_STATUS4           = 0x3D,
    REG_ISM330_FIFO_DATA_OUT_L        = 0x3E,
    REG_ISM330_FIFO_DATA_OUT_H        = 0x3F,
    REG_ISM330_TIMESTAMP0             = 0x40,
    REG_ISM330_TIMESTAMP1             = 0x41,
    REG_ISM330_TIMESTAMP2             = 0x42,
    REG_ISM330_SENSORHUB13            = 0x4D,
    REG_ISM330_SENSORHUB14            = 0x4E,
    REG_ISM330_SENSORHUB15            = 0x4F,
    REG_ISM330_SENSORHUB16            = 0x50,
    REG_ISM330_SENSORHUB17            = 0x51,
    REG_ISM330_SENSORHUB18            = 0x52,
    REG_ISM330_FUNC_SRC1              = 0x53,
    REG_ISM330_FUNC_SRC2              = 0x54,
    REG_ISM330_TAP_CFG                = 0x58,
    REG_ISM330_TAP_THS_6D             = 0x59,
    REG_ISM330_INT_DUR2               = 0x5A,
    REG_ISM330_WAKE_UP_THS            = 0x5B,
    REG_ISM330_WAKE_UP_DUR            = 0x5C,
    REG_ISM330_FREE_FALL              = 0x5D,
    REG_ISM330_MD1_CFG                = 0x5E,
    REG_ISM330_MD2_CFG                = 0x5F,
    REG_ISM330_MASTER_CMD_CODE        = 0x60,
    REG_ISM330_SENS_SYNC_SPI_ERR      = 0x61,
    REG_ISM330_OUT_MAG_RAW_X_L        = 0x66,
    REG_ISM330_OUT_MAG_RAW_X_H        = 0x67,
    REG_ISM330_OUT_MAG_RAW_Y_L        = 0x68,
    REG_ISM330_OUT_MAG_RAW_Y_H        = 0x69,
    REG_ISM330_OUT_MAG_RAW_Z_L        = 0x6A,
    REG_ISM330_OUT_MAG_RAW_Z_H        = 0x6B,
    REG_ISM330_INT_OIS                = 0x6F,
    REG_ISM330_CTRL1_OIS              = 0x70,
    REG_ISM330_CTRL2_OIS              = 0x71,
    REG_ISM330_CTRL3_OIS              = 0x72,
    REG_ISM330_X_OFS_USR              = 0x73,
    REG_ISM330_Y_OFS_USR              = 0x74,
    REG_ISM330_Z_OFS_USR              = 0x75,
} ism330dlcReg;

/*! Return codes */
typedef enum
{
    RET_ISM330_OK = 0,
    RET_ISM330_ESPI,
    RET_ISM330_NOT_ACCESSIBLE,
    RET_ISM330_READONLY
} ism330dlcRet;

ism330dlcRet ism330dlcReadRegister8bit(uint8_t spi, ism330dlcReg reg, uint8_t *data);
ism330dlcRet ism330dlcReadRegister16bit(uint8_t spi, ism330dlcReg reg, uint16_t *data);
ism330dlcRet ism330dlcWriteRegister8bit(uint8_t spi, ism330dlcReg reg, uint8_t data);


#endif    // ISM330DLC_H
