/*!
 * Copyright © Siemens Healthcare GmbH 2022, All Rights Reserved
 *
 * Project: Building Block Low End MCU
 *
 * @file
 * @brief LED control functions
 */
/*----------------------------------------------------------------------------*/

// Standard includes
#include <stdbool.h>

// HAL includes
#include <hal.h>

// project includes
#include "bsp_led.h"

/*! @struct Structure for LED desription */
static const struct
{
    GPIO_TypeDef    *port;
    GPIO_InitTypeDef pin;
    bool             active;
} ledPinCfg[] = {
#ifndef VDX8
    { GPIOJ, { GPIO_PIN_0, GPIO_MODE_OUTPUT_PP, GPIO_PULLUP, GPIO_SPEED_FREQ_HIGH, 0 }, true },    // SYS_ERR_LED
    { GPIOJ, { GPIO_PIN_1, GPIO_MODE_OUTPUT_PP, GPIO_PULLUP, GPIO_SPEED_FREQ_HIGH, 0 }, true },    // SYS_OK_LED
    { GPIOJ, { GPIO_PIN_2, GPIO_MODE_OUTPUT_PP, GPIO_PULLUP, GPIO_SPEED_FREQ_HIGH, 0 }, true },    // CAN_ERR
    { GPIOJ, { GPIO_PIN_3, GPIO_MODE_OUTPUT_PP, GPIO_PULLUP, GPIO_SPEED_FREQ_HIGH, 0 }, true },    // CAN_RUN
#else
    { GPIOI, { GPIO_PIN_2,  GPIO_MODE_OUTPUT_PP, GPIO_NOPULL, GPIO_SPEED_FREQ_HIGH, 0 }, true },        // STAT_RED10(SYS_ERROR_LED)
    { GPIOC, { GPIO_PIN_9,  GPIO_MODE_OUTPUT_PP, GPIO_NOPULL, GPIO_SPEED_FREQ_HIGH, 0 }, true },        // STAT_GRN12(SYS_OK_LED)
    { GPIOH, { GPIO_PIN_13, GPIO_MODE_OUTPUT_PP, GPIO_NOPULL, GPIO_SPEED_FREQ_HIGH, 0 }, true },        // STAT_GRN11(USB_CONNECTED_LED)
#endif
};

/*! Number of LEDs available */
const uint8_t LED_PIN_NUM = sizeof(ledPinCfg) / sizeof(ledPinCfg[0]);

/*----------------------------------------------------------------------------*/
/*!
 * @brief          LEDs initialization
 *
 */
/*----------------------------------------------------------------------------*/
void ledInit(void)
{
    for (uint8_t i = 0; i < LED_PIN_NUM; i++)
    {
        // Set initial pin state
        HAL_GPIO_WritePin(ledPinCfg[i].port, (uint16_t)ledPinCfg[i].pin.Pin, ledPinCfg[i].active ? GPIO_PIN_RESET : GPIO_PIN_SET);

        // Initialize pin
        GPIO_InitTypeDef pin = ledPinCfg[i].pin;
        HAL_GPIO_Init(ledPinCfg[i].port, &pin);
    }
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief          Switch on the specified LED
 *
 * @param[in]      ledNumber - Index of LED to switch on
 *
 */
/*----------------------------------------------------------------------------*/
void ledSwitchOn(uint8_t ledNumber)
{
    if (ledNumber < LED_PIN_NUM)
    {
        HAL_GPIO_WritePin(
            ledPinCfg[ledNumber].port, (uint16_t)ledPinCfg[ledNumber].pin.Pin, ledPinCfg[ledNumber].active ? GPIO_PIN_SET : GPIO_PIN_RESET);
    }
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief          Switch off the specified LED
 *
 * @param[in]      ledNumber - Index of LED to switch off
 *
 */
/*----------------------------------------------------------------------------*/
void ledSwitchOff(uint8_t ledNumber)
{
    if (ledNumber < LED_PIN_NUM)
    {
        HAL_GPIO_WritePin(
            ledPinCfg[ledNumber].port, (uint16_t)ledPinCfg[ledNumber].pin.Pin, ledPinCfg[ledNumber].active ? GPIO_PIN_RESET : GPIO_PIN_SET);
    }
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief          Toggle the specified LED
 *
 * @param[in]      ledNumber - Index of LED to switch off
 *
 */
/*----------------------------------------------------------------------------*/
void ledToggle(uint8_t ledNumber)
{
    if (ledNumber < LED_PIN_NUM)
    {
        HAL_GPIO_TogglePin(ledPinCfg[ledNumber].port, (uint16_t)ledPinCfg[ledNumber].pin.Pin);
    }
}

//--------------------------------- End Of File -------------------------------/
