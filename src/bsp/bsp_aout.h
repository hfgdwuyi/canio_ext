/*!
 * Copyright © Siemens Healthcare GmbH 2022, All Rights Reserved
 *
 * Project: Building Block Low End MCU
 *
 * @file
 * @brief Header file for bsp_ain.c
 */
/*----------------------------------------------------------------------------*/
#ifndef BSP_AOUT
#define BSP_AOUT

void bspAoutInit(void);
void bspAoutWrite(uint8_t channel, int16_t writeValue);

#endif
