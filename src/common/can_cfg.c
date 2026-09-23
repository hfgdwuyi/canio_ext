/*!
 * Copyright � Siemens Healthcare GmbH 2022, All Rights Reserved
 *
 * Project: Building Block Low End MCU
 *
 * @file
 * @brief CAN configuration
 */
/*----------------------------------------------------------------------------*/
// Standard includes
#include <stdbool.h>

// HAL includes
#include <hal.h>

// CANOpen includes
#include <cal_conf.h>
#include <co_acces.h>
#include <co_pdo.h>
#include <co_stru.h>
#include <co_stor.h>
#include <objects.h>

// Project includes
#include "timing.h"
#include "storage.h"
#include "can_cfg.h"


/* Default value of NodeID for CAN configuration */
#define CAN_NODE_ID_DEFAULT 0x34
/* Default value of Baud rate for CAN configuration */
#define CAN_BAUDRATE_DEFAULT 500

/*! CANOpen nodeID */
UNSIGNED8 nodeId;
/*! CAN baudrate */
UNSIGNED16 bitRate;
/*! CAN config pins state */
static UNSIGNED8 canPinsState;

/*! CAN configuration pins */
static const pinCfg canPinCfg[] = {
    { GPIOJ, { GPIO_PIN_4, GPIO_MODE_INPUT, GPIO_PULLUP, GPIO_SPEED_FREQ_LOW, 0 } },
    { GPIOJ, { GPIO_PIN_5, GPIO_MODE_INPUT, GPIO_PULLUP, GPIO_SPEED_FREQ_LOW, 0 } },
};

/*----------------------------------------------------------------------------*/
/*!
 @brief Set up CANOpen nodeID and baudrate

         The function reads state of pins which define CANOpen settings, then reads
         nodeID and baudrate tables from a non-vloatile storage and assigns nodeID and
         baudrate from the tables according to the read value.

 @return True is values were assigned, false if default settings were set

*/
/*----------------------------------------------------------------------------*/
bool canCfgGetConfig(void)
{
    // Read config pins
    canCfgSetCode();

    // Read data from non-volatile storage
    if (!storageLoadSegment(STORAGE_SEG_CAN))
    {
        // Read error, default settings are set
        nodeId  = CAN_NODE_ID_DEFAULT;
        bitRate = CAN_BAUDRATE_DEFAULT;
        return false;
    }

    // Choose data from the tables
    nodeId  = manNodeIDTable[canPinsState + 1];
    bitRate = manBaudrateTable[canPinsState + 1];
    return true;
}

/*----------------------------------------------------------------------------*/
/*!
 @brief Set CANOpen variable with CAN confog pins state

          Must be called after CANOpen library is initialized otherwise the value
          will be overwritten with the default one
*/
/*----------------------------------------------------------------------------*/
void canCfgSetCode(void)
{
    // Initialize pins
    for (uint8_t i = 0; i < sizeof(canPinCfg) / sizeof(canPinCfg[0]); i++)
    {
        GPIO_InitTypeDef pin = canPinCfg[i].pin;
        HAL_GPIO_Init(canPinCfg[i].port, &pin);
    }

    // Short delay to config be applied
    timingDelay_us(10);

    // Read pins state
    for (uint8_t i = 0; i < (sizeof(canPinCfg) / sizeof(canPinCfg[0])); i++)
    {
        if (HAL_GPIO_ReadPin(canPinCfg[i].port, (uint16_t)canPinCfg[i].pin.Pin) == GPIO_PIN_SET)
        {
            canPinsState |= (uint8_t)(1U << i);
        }
    }
    // Set CANOpen object with pins values
    manCANConfig = canPinsState;
}
