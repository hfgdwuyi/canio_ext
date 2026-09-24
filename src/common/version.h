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
 * compiler command line with -DAPP_SW_BUILD_TAG=\"B\". */
#ifndef APP_SW_BUILD_TAG
#define APP_SW_BUILD_TAG  "A"
#endif

/* Composed firmware version string. It is compiled in as a single string
 * literal so it can be checked before flashing with:
 *     strings application.bin | grep "1.0.0-A Application"
 * and is reported through object 0x100A (CANopen SDO), the boot log and the
 * "version" terminal command. */
#define APP_SW_STR_(x)  #x
#define APP_SW_STR(x)   APP_SW_STR_(x)
#define APP_SW_VERSION_STR                                     \
    APP_SW_STR(APP_SW_VERSION) "." APP_SW_STR(APP_SW_SUBVERSION) "." \
    APP_SW_STR(APP_SW_REVISION) "-" APP_SW_BUILD_TAG " Application"

#define BL_SW_VERSION     2
#define BL_SW_SUBVERSION  0
#define BL_SW_REVISION    0


#endif    // VERSION_H
