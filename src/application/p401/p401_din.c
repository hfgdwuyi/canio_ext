/*!
 * Copyright � Siemens Healthcare GmbH 2022, All Rights Reserved
 *
 * Project: Building Block Low End MCU
 *
 * @file
 * @brief Digital inputs implementation in regard for p401 profile
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
#include "bsp_dio.h"
#include "p401_din.h"

/* Number of DigitalInput elements saved*/
#define P401_DIN_BYTES ((sizeof(p410DIRead8Bit) / sizeof(p410DIRead8Bit[0])) - 1)

/*----------------------------------------------------------------------------*/
/*!
 * @brief   Reads DINs states and put them to object 0x6000
 *          Must be called periodically. Polarity settings applied here as well
 *
 */
/*----------------------------------------------------------------------------*/
void p401ReadDI(void)
{
    for (uint8_t i = 0; i < P401_DIN_BYTES; i++)
    {
        // Get din state
        uint8_t din = bspDinRead(i);
        // Apply write value and write mask
        din &= (uint8_t)~manDIWriteMask[i + 1U];
        din |= manDIWriteValue[i + 1U] & manDIWriteMask[i + 1U];
        // Apply polarity settings
        din ^= p401DIPolarity8Bit[i + 1U];
        // Set object value
        p410DIRead8Bit[i + 1U] = din;
    }
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief   Cheecks if TPDO should be issued
 *          Must be called periodically.
 *
 * @return  true if succeded, false otherwise
 */
/*----------------------------------------------------------------------------*/
bool p401CheckForPdo(void)
{
    static uint8_t prevDINValues[P401_DIN_BYTES];
    bool           bret = false;
    // Check if DI interrupts are enabled
    if (p401DIInterruptEn != CO_FALSE)
    {
        for (uint8_t i = 0; i < P401_DIN_BYTES; i++)
        {
            // Detecting "any change" trigger conditions
            uint8_t resAnyChange = (prevDINValues[i] ^ p410DIRead8Bit[i + 1U]) & p401DIMaskAnyChange[i + 1U];
            // Detecting "rising edge" trigger conditions
            uint8_t resRisingEdge = ((uint8_t)~prevDINValues[i] & p410DIRead8Bit[i + 1U]) & p401DIMaskRising[i + 1U];
            // Detecting "falling edge" trigger conditions
            uint8_t resFallingEdge = (prevDINValues[i] & (uint8_t)~p410DIRead8Bit[i + 1U]) & p401DIMaskFalling[i + 1U];
            // Check if one of the conditions met
            if ((resAnyChange | resRisingEdge | resFallingEdge) != 0)
            {
                bret = true;
            }
        }
    }
    else
    {
        // Any state change generates PDO
        // Compare data
        if (memcmp(prevDINValues, &p410DIRead8Bit[1], P401_DIN_BYTES) != 0)
        {
            bret = true;
        }
    }
    // Save previous values
    memcpy(prevDINValues, &p410DIRead8Bit[1], P401_DIN_BYTES);
    return bret;
}
