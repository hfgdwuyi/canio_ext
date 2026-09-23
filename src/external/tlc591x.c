/*!
 * Copyright � Siemens Healthcare GmbH 2022, All Rights Reserved
 *
 * Project: Building Block Low End MCU
 *
 *  @file
 *  @brief     Provide functionality to communicate with ISM330DLC
 *             8-Channel Constant-Current LED Sink Drivers
 */
/*----------------------------------------------------------------------------*/
// LPCOpen includes
#include <hal.h>
// Project includes
#include "sys_config.h"
#include "bsp_board.h"
#include "timing.h"
#include "error.h"

#define DELAY_TOGGLE_PIN 10    // delay toggle pin in microseconds

// Configuration for the left tlc591x driver
tlc591xInstance tlcLedDriverL = {
    .spi = 6,
    .le  = { SPI6_CS1_PORT, { SPI6_CS1_PIN, GPIO_MODE_OUTPUT_PP, GPIO_NOPULL, GPIO_SPEED_FREQ_VERY_HIGH } },
    .oe  = { GPIOG, { GPIO_PIN_0, GPIO_MODE_OUTPUT_PP, GPIO_NOPULL, GPIO_SPEED_FREQ_MEDIUM } },
};

// Configuration for the right tlc591x driver
tlc591xInstance tlcLedDriverR = {
    .spi = 6,
    .le  = { SPI6_CS2_PORT, { SPI6_CS2_PIN, GPIO_MODE_OUTPUT_PP, GPIO_NOPULL, GPIO_SPEED_FREQ_VERY_HIGH } },
    .oe  = { GPIOJ, { GPIO_PIN_14, GPIO_MODE_OUTPUT_PP, GPIO_NOPULL, GPIO_SPEED_FREQ_MEDIUM } },
};

/*----------------------------------------------------------------------------*/
/*!
 * @brief      Initializes tlc591x
 *
 * @param[in]  inst tlc591x instance
 *
 */
/*----------------------------------------------------------------------------*/
void tlc591xInit(tlc591xInstance inst)
{
    // Initialize LE pin
    GPIO_InitTypeDef pin = inst.le.pin;
    (void)HAL_GPIO_Init(inst.le.port, &pin);
    HAL_GPIO_WritePin(inst.le.port, (uint16_t)inst.le.pin.Pin, GPIO_PIN_RESET);

    // Initialize OE pin
    pin = inst.oe.pin;
    (void)HAL_GPIO_Init(inst.oe.port, &pin);
    HAL_GPIO_WritePin(inst.oe.port, (uint16_t)inst.oe.pin.Pin, GPIO_PIN_SET);
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief      Write data to tlc591x
 *
 * @param[in]  inst  tlc591x instance
 * @param[in]  value value to write
 * @return     operation result
 */
/*----------------------------------------------------------------------------*/
tlc591xRet tlc591xSetValue(tlc591xInstance inst, uint8_t value)
{
    return tlc591xSetValues(inst, &value, 1U);
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief      Write one or more bytes to tlc591x daisy chain
 *
 * @param[in]  inst  tlc591x instance
 * @param[in]  values bytes to write
 * @param[in]  size number of bytes to write
 * @return     operation result
 */
/*----------------------------------------------------------------------------*/
tlc591xRet tlc591xSetValues(tlc591xInstance inst, const uint8_t *values, uint16_t size)
{
    uint8_t      read[2] = { 0 };
    boardSpiXfer xfer = {
        .spi     = inst.spi,
        .wrBuf   = (void *)values,
        .rdBuf   = read,
        .size    = size,
        .timeout = 2000,
        .skipCs  = true,
    };

    if ((values == NULL) || (size == 0U) || (size > sizeof(read)))
    {
        return RET_TLC591X_ESPI;
    }

    if (!boardSpiTransfer(&xfer))
    {
        return RET_TLC591X_ESPI;
    }

    // Latch data
    HAL_GPIO_WritePin(inst.le.port, (uint16_t)inst.le.pin.Pin, GPIO_PIN_SET);
    timingDelay_us(DELAY_TOGGLE_PIN);
    HAL_GPIO_WritePin(inst.le.port, (uint16_t)inst.le.pin.Pin, GPIO_PIN_RESET);

    return RET_TLC591X_OK;
}
