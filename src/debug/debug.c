/*!
 * Copyright © Siemens Healthcare GmbH 2022, All Rights Reserved
 *
 * Project: Building Block Low End MCU
 *
 * @file
 * @brief Debug output implementation
 */
/*----------------------------------------------------------------------------*/
// Standard includes
#include <stdio.h>
#include <stdarg.h>
// HAL includes
#include <hal.h>
// Project includes
#include "timing.h"
#include "debug.h"

/*! Size of debug buffer */
#define DEBUG_BUFFER_SIZE 1024

/*! Debug data */
static struct
{
    uint32_t wrCounter;
    uint32_t rdCounter;
    uint8_t  buffer[DEBUG_BUFFER_SIZE];
} debugData;

/*----------------------------------------------------------------------------*/
/*!
    @brief         Output debug data

*/
/*----------------------------------------------------------------------------*/
static void debugOut(void)
{
    while (debugData.rdCounter < debugData.wrCounter)
    {
        putchar(debugData.buffer[debugData.rdCounter]);
        debugData.rdCounter++;
    }
}

/*----------------------------------------------------------------------------*/
/*!
    @brief          Debug initialization

*/
/*----------------------------------------------------------------------------*/
__attribute__((constructor)) void debugInit(void)
{
    static timingTimer debugTimer;
    timingAddTimer(&debugTimer, TIMING_TIMER_CYCLIC, 1000, debugOut);
}

/*----------------------------------------------------------------------------*/
/*!
    @brief          Copy debug data to the buffer
*/
/*----------------------------------------------------------------------------*/
static void debugPutToBuffer(const char *str, uint32_t size)
{
    uint32_t index = debugData.wrCounter % DEBUG_BUFFER_SIZE;
    if (index + size > DEBUG_BUFFER_SIZE)
    {
        // Put data until the end of an array
        uint32_t amount = size - (DEBUG_BUFFER_SIZE - index);
        snprintf((char *)&debugData.buffer[index], amount, "%s", str);
        // Put rest of the data
        snprintf((char *)debugData.buffer, size - amount, "%s", &str[amount]);
    }
    else
    {
        // Just copy the string to the buffer
        snprintf((char *)&debugData.buffer[index], size, "%s", str);
    }
    //
    debugData.wrCounter = (debugData.wrCounter + size) % DEBUG_BUFFER_SIZE - 1;
}

/*----------------------------------------------------------------------------*/
/*!
    @brief          Parse debug data and put them to buffer
*/
/*----------------------------------------------------------------------------*/
void debugPrint(uint8_t level, const char *func, const char *frm, ...)
{
    static char debugBuffer[DEBUG_BUFFER_SIZE];
    if (level > LOG_LEVEL)
    {
        return;
    }
    va_list args;
    va_start(args, frm);

    // Print function's name
    uint32_t size = snprintf(debugBuffer, sizeof(debugBuffer), "%s : ", func);
    // Print data
    size += vsprintf(&debugBuffer[size], frm, args);
    debugPutToBuffer(debugBuffer, size + 1);
}
