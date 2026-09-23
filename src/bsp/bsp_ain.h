/*!
 * Copyright © Siemens Healthcare GmbH 2022, All Rights Reserved
 *
 * Project: Building Block Low End MCU
 *
 * @file
 * @brief Header file for bsp_ain.c
 */
/*----------------------------------------------------------------------------*/
#ifndef BSP_AIN
#define BSP_AIN

void     bspAinInit(void);
uint32_t bspAinGetRawValue(uint8_t channel);

extern uint8_t AIN_NUMBER;

#endif
