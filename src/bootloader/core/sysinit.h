/*!
* Copyright © Siemens Healthcare GmbH 2022, All Rights Reserved
*
* Project: Building Block Low End MCU
* 
* @file
* @brief Header file for sysinit.c
*/
/*----------------------------------------------------------------------------*/
#ifndef SYSINIT_H
#define SYSINIT_H

void SystemClock_Config(void);
void SystemCache_Enable(void);
void SystemCache_Disable(void);

#endif // SYSINIT_H

