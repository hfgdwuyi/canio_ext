/**
 * Copyright (C) Siemens Healthcare GmbH 2022, All Rights Reserved
 *
 * Project: Building Block Low End MCU
 *
 * @file
 * @brief     This module contains callback functions to the CANopen LEDs
 *            specified in the CiA standard CiA-303-3.
 *            These callback functions are called by the CANopen Library
 *            when an event occurs which shall be indicated by the CANopen LEDs.
 */
/*----------------------------------------------------------------------------*/

// CANOpen includes
#include <environ.h>
#include <cal_conf.h>
#include <co_led.h>

// Project includes
#include "bsp_led.h"

#ifdef CONFIG_CO_LED
/*----------------------------------------------------------------------------*/
/*!
 @brief ledInd - switch CANopen LED

 This function switches the CANopen LEDs on or off in the used hardware.
 It is called from the Library according to the
 NMT-state and error state, respectively.
 The parameter led determines which of the two LEDs to act on.
 @code
    CO_ERR_LED	Error LED
    CO_RUN_LED	NMT LED
 @endcode

 The state for this LED is handed over
 @code
    CO_LED_ON	turn LED on
    CO_LED_OFF	turn LED off
 @endcode
 within the parameter action.

 @param[in] led     kind of CANopen LED
 @param[in] action  turn LED on or off

 @return nothing
*/
/*----------------------------------------------------------------------------*/
void ledInd(UNSIGNED8 led, UNSIGNED8 action CO_COMMA_REDCY_PARA_DECL)
{
    if (led == CO_ERR_LED)
    {
        if (action == CO_LED_ON)
        {
            ledSwitchOn(CAN_ERROR_LED_NUM);
        }
        else
        {
            ledSwitchOff(CAN_ERROR_LED_NUM);
        }
    }
    if (led == CO_RUN_LED)
    {
        if (action == CO_LED_ON)
        {
            ledSwitchOn(CAN_STATUS_LED_NUM);
        }
        else
        {
            ledSwitchOff(CAN_STATUS_LED_NUM);
        }
    }
}
#endif /* CONFIG_CO_LED */

/*______________________________________________________________________EOF_*/
