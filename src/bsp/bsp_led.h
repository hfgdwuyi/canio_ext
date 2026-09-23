/*!
 * Copyright © Siemens Healthcare GmbH 2022, All Rights Reserved
 *
 * Project: Building Block Low End MCU
 *
 * @file
 * @brief Header file for led_ctr.c
 */
/*----------------------------------------------------------------------------*/
#ifndef LED_CTRL_H
#define LED_CTRL_H


/*! System ERROR LED number */
#define SYSTEM_ERROR_LED_NUM (0)
/*! System OK LED number */
#define SYSTEM_OK_LED_NUM (1)
/*! CAN error LED number */
#define CAN_ERROR_LED_NUM (2)
/*! CAN status LED number */
#define CAN_STATUS_LED_NUM (3)

void ledInit(void);
void ledSwitchOn(uint8_t ledNumber);
void ledSwitchOff(uint8_t ledNumber);
void ledToggle(uint8_t ledNumber);

#endif

//--------------------------------- End Of File -------------------------------/
