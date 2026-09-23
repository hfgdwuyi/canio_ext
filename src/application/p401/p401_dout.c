/*!
 * Copyright � Siemens Healthcare GmbH 2022, All Rights Reserved
 *
 * Project: Building Block Low End MCU
 *
 * @file
 * @brief Digital outputs implementation in regard for p401 profile
 */
/*----------------------------------------------------------------------------*/
// Standard includes
#include <stdbool.h>
#include <limits.h>
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
#include "bsp_board.h"
#include "bsp_dio.h"
#include "p401_dout.h"


/* Number of DigitalOutput elements saved*/
#define P401_DOUT_BYTES (sizeof(p401DOWrite8Bit) / sizeof(p401DOWrite8Bit[0]) - 1)
#define P401_TLC_LEFT_BYTE 0U
#define P401_TLC_RIGHT_BYTE 1U

/*Saved parameters to compare with*/
static uint8_t writeDout[P401_DOUT_BYTES];
static uint8_t polarity[P401_DOUT_BYTES];

/*----------------------------------------------------------------------------*/
/*!
 * @brief          Changes the state of pin values
 *
 * @param          byte - array index to set
 * @param          value - value to set
 *
 */
/*----------------------------------------------------------------------------*/
void p401WriteDOUT(uint8_t byte, uint8_t value)
{
    value ^= p401DOPolarity8Bit[byte + 1];

    if (byte == P401_TLC_LEFT_BYTE)
    {
        (void)tlc591xSetValue(tlcLedDriverL, value);
        return;
    }

    if (byte == P401_TLC_RIGHT_BYTE)
    {
        (void)tlc591xSetValue(tlcLedDriverR, value);
        return;
    }

    for (uint8_t i = 0; i < CHAR_BIT; i++)
    {
        bool initState = (value & (1U << i)) == 0 ? false : true;
        bool filter    = (p401DOFilterMask[byte + 1] & (1U << i)) == 0 ? false : true;
        // Check filter state
        if (filter)
        {
            // Set every output pin including polarity
            bspDoutSet(i + (byte * CHAR_BIT), initState);
        }
    }
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief          Sets error values for output pins and object 0x6200
 *
 */
/*----------------------------------------------------------------------------*/
static void p401SetDoutErrValue(void)
{
    for (uint8_t i = 0; i <= P401_DOUT_BYTES * CHAR_BIT; i++)
    {
        uint8_t bit               = i % CHAR_BIT;
        uint8_t byte              = i / CHAR_BIT;
        uint8_t errValue          = p401DOErrorMode[byte] & p401DOErrorVal[byte];
        p401DOWrite8Bit[byte + 1] = errValue;
        bool pinErrState          = (p401DOWrite8Bit[byte + 1] & (1U << bit)) == 0 ? false : true;
        bspDoutSet(i, pinErrState);
    }
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief          DOUT Handler
 *
 */
/*----------------------------------------------------------------------------*/
void doutHandler(void)
{
    uint32_t mask= ERR_CANOPEN_BUS_OFF | ERR_CANOPEN_CHGSTATE_STOPPED;
    // Check if error occurs
    if ((manErrorDesc[ERR_DESC_SUBINDEX_CANOPEN] & mask) != 0)
    {
        p401SetDoutErrValue();
    }
    else
    {
        if (memcmp(writeDout, &p401DOWrite8Bit[1], sizeof(writeDout)) != 0)
        {
            for (uint8_t i = 0; i < P401_DOUT_BYTES; i++)
            {
                p401WriteDOUT(i, p401DOWrite8Bit[i + 1]);
            }
            memcpy(writeDout, &p401DOWrite8Bit[1], sizeof(writeDout));
        }
        else if (memcmp(polarity, &p401DOPolarity8Bit[1], sizeof(polarity)) != 0)
        {
            for (uint8_t i = 0; i < P401_DOUT_BYTES; i++)
            {
                p401WriteDOUT(i, p401DOWrite8Bit[i + 1]);
            }
            memcpy(polarity, &p401DOPolarity8Bit[1], sizeof(polarity));
        }
    }
}
