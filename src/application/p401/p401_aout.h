/*!
 * Copyright © Siemens Healthcare GmbH 2022, All Rights Reserved
 *
 * Project: Building Block Low End MCU
 *
 * @file
 * @brief Header file for p401_aout.c
 */
/*----------------------------------------------------------------------------*/
#ifndef P401_AOUT_H
#define P401_AOUT_H

void p401WriteDACValue(const int16_t *value);
void p401SetDefaultDACValues(void);

#endif
