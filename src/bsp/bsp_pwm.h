/*!
 * Copyright Siemens Healthcare GmbH 2022, All Rights Reserved
 *
 * Project: Building Block Low End MCU
 *
 * @file
 * @brief Header file for bsp_pwm.c
 */
/*----------------------------------------------------------------------------*/
#ifndef BSP_PWM_H
#define BSP_PWM_H

void bspPwmInit(void);
void bspPwmStart(uint8_t pwmNum);
void bspPwmStop(uint8_t pwmNum);
void bspPwmSetCarrierFreq(uint8_t pwmNum, uint32_t frequency);
void bspPwmSetDutyCycle(uint8_t pwmNum, uint32_t value);


#endif

//--------------------------------- End Of File -------------------------------/