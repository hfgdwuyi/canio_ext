/*!
 * Copyright Siemens Healthcare GmbH 2022, All Rights Reserved
 *
 * Project: Building Block Low End MCU
 *
 * @file
 * @brief Header file for prog_ctrl.c
 */
/*----------------------------------------------------------------------------*/
#ifndef PROG_CTRL_H
#define PROG_CTRL_H

#include <stdbool.h>

/*----------------------------------------------------------------------------*/
/*!
@name           Program control signatures
@{
*/
/*----------------------------------------------------------------------------*/
/*! Default value */
#define PROG_CTRL_START_DEFAULT (0x00000000UL)
/*! Start application */
#define PROG_CTRL_START_APPLICATION (0x12345678UL)
/*! Start default image */
#define PROG_CTRL_START_DEFAULT_IMAGE (0x56781234UL)
/*! Start bootloader */
#define PROG_CTRL_START_BOOTLOADER (0x1111BFBFUL)

#define PROG_COMMAND_BOOTLOADER    0
#define PROG_COMMAND_APPLICATION   1


#define PROG_APP_START_LOCATION    (0x55555555)
extern bool needToReset;

void     progCtrlSetSignature(uint32_t signature);
uint32_t progCtrlGetSignature(void);

/*!
@}
*/

#endif
