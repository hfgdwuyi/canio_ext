/*!
 * Copyright Siemens Healthcare GmbH 2022, All Rights Reserved
 *
 * Project: Building Block Low End MCU
 *
 * @file
 * @brief HAL includes
 */
/*----------------------------------------------------------------------------*/
#ifndef HAL_H
#define HAL_H

#include <stm32h7xx_hal.h>

/* @typedef Structure for digital input/output desription */

typedef struct
{
    GPIO_TypeDef    *port;
    GPIO_InitTypeDef pin;
} pinCfg;

extern QSPI_HandleTypeDef hqspi;
extern ADC_HandleTypeDef  hadc1;
extern ADC_HandleTypeDef  hadc3;
extern I2C_HandleTypeDef  hi2c4;
extern DMA_HandleTypeDef  hdma1;
extern DMA_HandleTypeDef  hdma3;
extern I2C_HandleTypeDef  hi2c1;
extern SPI_HandleTypeDef  hspi6;
extern UART_HandleTypeDef serialUart;
extern PCD_HandleTypeDef  hpcd;
extern IWDG_HandleTypeDef hiwdg1;
extern uint8_t            file_buffer[];
extern char               TxBuf[400];
#define SYS_SPI_ISM330_NUMBER 5    //!< SPI number for ISM330 sensor
#endif
