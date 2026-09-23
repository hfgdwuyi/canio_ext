/*!
 * Copyright Siemens Healthcare GmbH 2022, All Rights Reserved
 *
 * Project: Building Block Low End MCU
 *
 * @file
 * @brief Header for selftest.c
 */
/*----------------------------------------------------------------------------*/
#ifndef SELFTEST_H
#define SELFTEST_H

bool                           selftestFlashApp(uint32_t address);
bool                           selftestRescueSwitchRead(void);
__attribute__((noreturn)) void selftestErrorState(void);

#endif