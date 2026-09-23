/*!
 * Copyright © Siemens Healthcare GmbH 2022, All Rights Reserved
 *
 * Project: Building Block Low End MCU
 *
 * @file
 * @brief Pins and interfaces initialization
 */
/*----------------------------------------------------------------------------*/
#ifndef CRC_CALC_H
#define CRC_CALC_H

#include <stdbool.h>

void     crcInit(void);
void     crcReset(void);
uint8_t  crcCalc8(void *data, uint32_t length);
uint32_t crcCalc32(void *data, uint32_t length);


#endif
