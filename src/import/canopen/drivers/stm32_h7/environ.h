/*
 * environ.h - CPU/compiler settings depending on the used environment
 *
 * Copyright (c) 2020 port GmbH Halle/Saale
 *------------------------------------------------------------------
 */

/**
* \file environ.h
* \author port GmbH
*
*/

#ifndef ENVIRON_H
#  define ENVIRON_H

/* STM32 HAL layer */
#  define USE_HAL_DRIVER	1
#  include <stm32h7xx.h>
#  include <stm32h7xx_hal_conf.h>

extern FDCAN_HandleTypeDef hfdcan;
extern void Timer_int(void);

#endif /* ENVIRON_H */
