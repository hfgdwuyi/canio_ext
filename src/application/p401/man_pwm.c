/*!
 * Copyright � Siemens Healthcare GmbH 2022, All Rights Reserved
 *
 * Project: Building Block Low End MCU
 *
 * @file
 * @brief PWM outputs implementation in regard for p401 profile
 */
/*----------------------------------------------------------------------------*/

// Standard includes
#include <stdbool.h>
// HAL includes
#include <hal.h>
// CANOpen includes
#define DEF_HW_PART
#include <cal_conf.h>

#include <co_acces.h>
#include <co_sdo.h>
#include <co_pdo.h>
#include <co_drv.h>
#include <co_lme.h>
#include <co_nmt.h>
#include <co_init.h>
#include <objects.h>
// Project includes
#include "error_defines.h"
#include "bsp_pwm.h"

/* Maximum number of PWM used by runtime drivers */
#define PWM_CHANNEL (sizeof(manPWMControl) / sizeof(manPWMControl[0]) - 1)

static uint8_t  pwmDutyCycle[PWM_CHANNEL];
static uint32_t pwmFrequency;
static bool     pwmState[PWM_CHANNEL];

/*----------------------------------------------------------------------------*/
/*!
 @brief          Starts or stops PWM on specific channel

 @param          pwmNum - number of PWM to change state
 @param          pwmState - specifies start or stop pwm signalling
 @return         None

*/
/*----------------------------------------------------------------------------*/
static void manPwmControl(uint8_t pwmNum, bool state)
{
    if (state)
    {
        bspPwmStart(pwmNum);
    }
    else
    {
        bspPwmStop(pwmNum);
    }
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief          Sets duty cycle of pwmChannel predefined in object 0x5405
 *
 * @param[in]      None
 * @return         None
 *
 */
/*----------------------------------------------------------------------------*/
static void manPwmSetErrState(void)
{
    for (uint8_t i = 1; i <= PWM_CHANNEL; i++)
    {
        if (manPWMErrMode[i])
        {
            bspPwmSetDutyCycle(i - 1, manPWMErrValue[i]);
        }
    }
}

/*----------------------------------------------------------------------------*/
/*!
 @brief          DOUT Handler

*/
/*----------------------------------------------------------------------------*/
void manPwmHandler(void)
{
    uint32_t mask= ERR_CANOPEN_BUS_OFF | ERR_CANOPEN_CHGSTATE_STOPPED;
    // Check if error occurs
    if ((manErrorDesc[ERR_DESC_SUBINDEX_CANOPEN] & mask) != 0)
    {
        manPwmSetErrState();
    }
    else if (pwmFrequency != manPWMFrequency)
    {
        for (uint8_t i = 0; i < PWM_CHANNEL; i++)
        {
            bspPwmSetCarrierFreq(i, manPWMFrequency);
            bspPwmSetDutyCycle(i, manPWMDutyCycle[i + 1]);
        }
        pwmFrequency = manPWMFrequency;
        memcpy(pwmDutyCycle, &manPWMDutyCycle[1], (sizeof(pwmDutyCycle) / sizeof(pwmDutyCycle[0])));
    }
    else if (memcmp(pwmDutyCycle, &manPWMDutyCycle[1], (sizeof(pwmDutyCycle) / sizeof(pwmDutyCycle[0]))) != 0)
    {
        for (uint8_t i = 0; i < PWM_CHANNEL; i++)
        {
            bspPwmSetDutyCycle(i, manPWMDutyCycle[i + 1]);
        }
        memcpy(pwmDutyCycle, &manPWMDutyCycle[1], (sizeof(pwmDutyCycle) / sizeof(pwmDutyCycle[0])));
    }
    else if (memcmp(pwmState, &manPWMControl[1], (sizeof(pwmState) / sizeof(pwmState[0]))) != 0)
    {
        for (uint8_t i = 0; i < PWM_CHANNEL; i++)
        {
            if (pwmState[i] != manPWMControl[i + 1])
            {
                manPwmControl(i, manPWMControl[i + 1]);
            }
        }
        memcpy(pwmState, &manPWMControl[1], (sizeof(pwmState) / sizeof(pwmState[0])));
    }
}
