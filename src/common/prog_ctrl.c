/*!
 * Copyright © Siemens Healthcare GmbH 2022, All Rights Reserved
 *
 * Project: Building Block Low End MCU
 *
 * @file
 * @brief Program behavior control
 *
 */
/*----------------------------------------------------------------------------*/

// HAL includes
#include <hal.h>
// Project includes
#include "prog_ctrl.h"

/*! RTC register to save signature */
#define RTC_SIGN_REG RTC->BKP1R

/*! Need to reset flag */
bool needToReset = false;

/*----------------------------------------------------------------------------*/
/*!
 * @brief  This function sets signature of starting procedure in RTC register.
 *
 * @param  signature combination of numbers specified the procedure to start
 *
 */
/*----------------------------------------------------------------------------*/
void progCtrlSetSignature(uint32_t signature)
{
    __HAL_RCC_RTC_ENABLE();
    HAL_PWR_EnableBkUpAccess();
    WRITE_REG(RTC_SIGN_REG, signature);
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief  This function reads signature of starting procedure from RTC register.
 *
 * @return signature
 */
/*----------------------------------------------------------------------------*/
uint32_t progCtrlGetSignature(void)
{
    return READ_REG(RTC_SIGN_REG);
}
