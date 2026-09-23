/*!
 * Copyright © Siemens Healthcare GmbH 2022, All Rights Reserved
 *
 * Project: Building Block Low End MCU
 *
 * @file
 * @brief          Implementation of basic control functions for watchdog
 */
/*----------------------------------------------------------------------------*/
// HAL includes
#include <hal.h>
// Project includes
#include "terminal.h"
#include "bsp_wtdg.h"
#include "error.h"


#ifdef WATCHDOG_TEST
#include <stdio.h>
#include <string.h>
#include "terminal.h"
#endif

#ifdef WTDG_DISABLED
#warning Watchdog triggering is disabled!
#else
/*! WTDGpin pin assignment */
static const pinCfg wtdgPinCfg = { GPIOH, { GPIO_PIN_15, GPIO_MODE_OUTPUT_PP, GPIO_PULLUP, GPIO_SPEED_FREQ_VERY_HIGH, 0 } };
/*! Stop watchdog flag. When set to true watchdog should be stopped */
#endif
static bool wtdgStopFlag;

/*----------------------------------------------------------------------------*/
/*!
 *  @brief          Watchdog initialization
 *                  To calculate values for watchdog initialization use:
 *                  Reload = ((Time_needed * 32000)/(Prescaler * 1000)) -1
 *
 */
/*----------------------------------------------------------------------------*/
void WTDG_Init(void)
{
#ifndef WTDG_DISABLED
    // Configure GPIO pin
    GPIO_InitTypeDef pin = wtdgPinCfg.pin;
    (void)HAL_GPIO_Init(wtdgPinCfg.port, &pin);

    // Set the initial state for WTDG pin
    HAL_GPIO_WritePin(wtdgPinCfg.port, (uint16_t)wtdgPinCfg.pin.Pin, GPIO_PIN_RESET);

    // Set the timeout to 1.0 s
    hiwdg1.Init.Prescaler = IWDG_PRESCALER_8;
    hiwdg1.Init.Window    = 0xFFF;    // disable IWDG window value
    hiwdg1.Init.Reload    = 3999;
    if (HAL_IWDG_Init(&hiwdg1) != HAL_OK)
    {
        set_error(ERR_MAN_HW_INIT, ERR_INIT_WTDG);
    }
#endif
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief          Feed the watchdog
 *
 */
/*----------------------------------------------------------------------------*/
void WTDG_Feed(void)
{
#ifndef WTDG_DISABLED
    if (!wtdgStopFlag)
    {
        HAL_GPIO_TogglePin(wtdgPinCfg.port, (uint16_t)wtdgPinCfg.pin.Pin);
        HAL_IWDG_Refresh(&hiwdg1);
    }
#endif
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief          Stops watchdog triggering
 *
 */
/*----------------------------------------------------------------------------*/
void wtdgStop(void)
{
    wtdgStopFlag = true;
}

#ifdef WATCHDOG_TEST

/*----------------------------------------------------------------------------*/
/*!
 * @brief          Stops watchdog triggering
 *
 */
/*----------------------------------------------------------------------------*/
static terminalRet terminalWatchdogTest(uint8_t argc, char **argv)
{
    if (argc < 2)
    {
        return SHELL_EARGC;
    }
    if (strcmp(argv[1], "stop") == 0)
    {
        wtdgStop();
    }
    else
    {
        return SHELL_EARG;
    }
    return SHELL_OK;
}

/* ---------------------------------------------------------------------------*/
/*!
 * @brief        Adds watchdog command to the list of terminal commands
 *
 */
/*----------------------------------------------------------------------------*/
__attribute__((constructor)) void terminalWatchdogTestInit(void)
{
    static terminalItem testItem = { .name     = "wtdg",
                                     .desc     = "Watchdog test",
                                     .help     = "Usage: \n\n "
                                                 "wtdg stop  - Stops watchdog triggering \n",
                                     .callback = terminalWatchdogTest,
                                     .next     = NULL };
    terminalAddItem(&testItem);
}
#endif
//--------------------------------- End Of File -------------------------------/
