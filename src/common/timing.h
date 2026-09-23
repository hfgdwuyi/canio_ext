/*!
 * Copyright © Siemens Healthcare GmbH 2022, All Rights Reserved
 *
 * Project: Building Block Low End MCU
 *
 * @file
 * @brief Header file for timing.c
 */
/*----------------------------------------------------------------------------*/
#ifndef TIMING_H
#define TIMING_H

#include <stdbool.h>

/*! Timer used to implement time delays */
#define TIMING_TIMER_DELAY 0
/*! Timer to be run one time */
#define TIMING_TIMER_ONE_SHOT 1
/*! Periodic timer */
#define TIMING_TIMER_CYCLIC 2

/* Typedef for handler to be called after a timer is expired */
typedef void (*timingCallback)(void);

/* Structure for timers to be added to linked list */
typedef struct timing_Timer
{
    struct timing_Timer *pNext;    /*!< Pointer to the next timer in the list */
    uint32_t             timeout;  /*!< Timer's timeout */
    volatile uint32_t    counter;  /*!< Timer's current counter */
    uint8_t              type;     /*!< Timer's type */
    timingCallback       callback; /*!< Callback */
} timingTimer;

static inline void timingDelay_us(uint32_t delay_us)
{
    while (delay_us-- > 0)
    {
        for (uint32_t i = 0; i < (SystemCoreClock / 10000000); i++)
        {
            __NOP();
        }
    }
}

void timingInit(void);
void timingRemoveTimer(const timingTimer *pTimer);
void timingAddTimer(timingTimer *pTimer, uint8_t type, uint32_t timeout, timingCallback callback);
void timingExecute(void);
bool timingCheckTimeout(const timingTimer *pTimer);
void timingTick(void);
void timingDelay_ms(uint32_t delay_ms);

#endif

//--------------------------------- End Of File -------------------------------/
