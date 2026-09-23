/*!
 * Copyright © Siemens Healthcare GmbH 2022, All Rights Reserved
 *
 * Project: Building Block Low End MCU
 *
 * @file
 * @brief Header file for p401_ain.c
 */
/*----------------------------------------------------------------------------*/
#ifndef P401_AIN_H
#define P401_AIN_H


void      p401ReadAI(void);
uint32_t  p401AIInterruptsCalculate(void);
void      p401AIFilterCalculate(void);
INTEGER16 p401AinNTCCalc(UNSIGNED16 adcTemp);
INTEGER16 p401AinPumpCurrentCalc(UNSIGNED16 adcVal);

#endif
