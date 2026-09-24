/*!
 * Copyright � Siemens Healthcare GmbH 2023, All Rights Reserved
 *
 * Project: Building Block Low End MCU
 *
 * @file
 * @brief Header for versions definitions
 */
/*----------------------------------------------------------------------------*/
#ifndef VERSION_H
#define VERSION_H

#define APP_SW_VERSION    1
#define APP_SW_SUBVERSION 0
#define APP_SW_REVISION   0

/* Firmware build tag. Used to distinguish two firmware images during upgrade
 * verification. Change it per image (e.g. "A" / "B") or override it on the
 * compiler command line with -DAPP_SW_BUILD_TAG=\"B\".
 * The running image reports this tag through object 0x100A (CANopen SDO) and
 * through the "version" terminal command. */
#ifndef APP_SW_BUILD_TAG
#define APP_SW_BUILD_TAG  "A"
#endif

#define BL_SW_VERSION     2
#define BL_SW_SUBVERSION  0
#define BL_SW_REVISION    0


#endif    // VERSION_H
