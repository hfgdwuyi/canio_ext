/*!
 * Copyright © Siemens Healthcare GmbH 2022, All Rights Reserved
 *
 * Project: Building Block Low End MCU
 *
 * @file
 * @brief Analog outputs implementation in regard for p401 profile
 *
 */
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
#include "sys_config.h"
#include "bsp_aout.h"
#include "error.h"
#include "p401_aout.h"

#define P401_AOUT_NUMBER (sizeof(p401AnalogOutput16bit) / sizeof(p401AnalogOutput16bit[0]) - 1)

/*----------------------------------------------------------------------------*/
/*!
 * @brief          Calculates value to DAC output
 * @param[in]      value - pointer to array of CANOpen DAC output values
 *
 */
/*----------------------------------------------------------------------------*/
void p401WriteDACValue(const int16_t *value)
{
    int16_t dacValue = 0;
    for (uint8_t i = 0; i < P401_AOUT_NUMBER; i++)
    {
        dacValue = (int16_t)((value[i + 1] + p401AOOffset[i + 1]) * p401AOScaling[i + 1]);
        bspAoutWrite(i, dacValue);
    }
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief          Sets default DAC values in case of CANOpen error
 *
 */
/*----------------------------------------------------------------------------*/
void p401SetDefaultDACValues(void)
{
    for (uint8_t i = 0; i < P401_AOUT_NUMBER; i++)
    {
        uint8_t errorState = (p401AOErrorMode[1] & (1U << i)) == 0 ? false : true;
        if (errorState)
        {
            bspAoutWrite(i, (int16_t)p401AOErrorValue[i + 1]);
        }
    }
}
