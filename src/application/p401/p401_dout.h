/*!
 * Copyright © Siemens Healthcare GmbH 2022, All Rights Reserved
 *
 * Project: Building Block Low End MCU
 *
 * @file
 * @brief    Header file for p401_dout.c
 */
/*----------------------------------------------------------------------------*/
#ifndef P401_DOUT_H
#define P401_DOUT_H

void p401WriteDOUT(uint8_t byte, uint8_t value);
void doutHandler(void);

#endif

//--------------------------------- End Of File -------------------------------/
