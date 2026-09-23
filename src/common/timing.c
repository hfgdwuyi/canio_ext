/*!
 * Copyright © Siemens Healthcare GmbH 2022, All Rights Reserved
 *
 * Project: Building Block Low End MCU
 *
 * @file
 * @brief Implementation of timeout and delay functions
 *
 */
/*----------------------------------------------------------------------------*/

// C Standard includes
#include <stdlib.h>

// HAL includes
#include <hal.h>

// Application includes
#include "timing.h"

/*! Ticks per second rate */
#define TICKRATE_HZ1 (1000)

/*! Timer for time delay imlementation */
static timingTimer delayTimer = { .type = TIMING_TIMER_DELAY };

/*! Head of timers linked list */
static timingTimer *timingListHead = &delayTimer;

/*----------------------------------------------------------------------------*/
/*!
 * @brief          Remove timer from the list
 *
 * @param[in]      pTimer pointer to timer to be deleted
 *
 */
/*----------------------------------------------------------------------------*/
void timingRemoveTimer(const timingTimer *pTimer)
{
    timingTimer *pPrev     = timingListHead;
    timingTimer *pIterator = pPrev->pNext;

    while (pIterator != NULL)
    {
        if (pIterator == pTimer)
        {
            pPrev->pNext = pIterator->pNext;
            break;
        }
        pPrev     = pIterator;
        pIterator = pIterator->pNext;
    }
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief          Add cyclic timer to timer's linked list
 *
 * @param[in]      pTimer pointer to timer to be added
 * @param[in]      type timers type
 * @param[in]      timeout time to change timers state after
 * @param[in]      callback  the function which calls to start after time is up
 *
 */
/*----------------------------------------------------------------------------*/
void timingAddTimer(timingTimer *pTimer, uint8_t type, uint32_t timeout, timingCallback callback)
{
    if (pTimer == NULL)
    {
        return;
    }

    timingRemoveTimer(pTimer);
    timingTimer *pIterator = timingListHead;

    // Search for the end of the list
    while (pIterator->pNext != NULL)
    {
        pIterator = pIterator->pNext;
    }

    // Initialize the timer
    pTimer->timeout  = timeout;
    pTimer->counter  = timeout;
    pTimer->callback = callback;
    pTimer->type     = type;
    pTimer->pNext    = NULL;


    // Add the timer to the end of the list
    pIterator->pNext = pTimer;

    return;
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief          Check timers list
 *
 */
/*----------------------------------------------------------------------------*/
void timingExecute(void)
{
    // Go through timers list and check if actions are necessary
    for (timingTimer *pIterator = timingListHead; pIterator != NULL; pIterator = pIterator->pNext)
    {
        // If timer's timeout expired
        if (pIterator->counter == 0)
        {
            if (pIterator->type == TIMING_TIMER_CYCLIC)
            {
                // Re-initialize timer
                pIterator->counter = pIterator->timeout;
            }
            else
            {
                timingRemoveTimer(pIterator);
            }

            // Execute handler if it is not NULL
            if (pIterator->callback != NULL)
            {
                // Call handler
                pIterator->callback();
            }
        }
    }
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief          Check if timer expired
 *
 * @param[in]      pTimer pointer to timer to be added
 * @return         True if timeout expires, false otherwise
 */
/*----------------------------------------------------------------------------*/
bool timingCheckTimeout(const timingTimer *pTimer)
{
    timingTimer *pIterator = timingListHead;

    while (pIterator != NULL)
    {
        if (pIterator == pTimer)
        {
            return pIterator->counter == 0 ? true : false;
        }
        pIterator = pIterator->pNext;
    }
    // Timer not found -> timeout was not set or expired
    return true;
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief          Time delay in milliseconds
 *
 * @param[in]      delay_ms duration of delay
 *
 */
/*----------------------------------------------------------------------------*/
void timingDelay_ms(uint32_t delay_ms)
{
    delayTimer.counter = delay_ms;
    while (delayTimer.counter != 0)
    {
        // Wait until timeout expires
    }
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief	Handle interrupt from SysTick timer
 *
 */
/*----------------------------------------------------------------------------*/
void timingTick(void)
{
    for (timingTimer *pIterator = timingListHead; pIterator != NULL; pIterator = pIterator->pNext)
    {
        if (pIterator->counter != 0)
        {
            pIterator->counter--;
        }
    }
}

//--------------------------------- End Of File -------------------------------/
