/*!
 * Copyright © Siemens Healthcare GmbH 2022, All Rights Reserved
 *
 * Project: Building Block Low End MCU
 *
 * @file
 * @brief Header file for debug.c
 */
/*----------------------------------------------------------------------------*/
#ifndef DEBUG_H
#define DEBUG_H

#define LOG_ERROR   1U
#define LOG_WARNING 2U
#define LOG_INFO    3U
#define LOG_DEBUG   4U

#define LOG_LEVEL   LOG_INFO

#ifdef DEBUG_ENABLED

#define DEBUGOUT(level, frm, ...) debugPrint(level, __func__, frm, ##__VA_ARGS__)
void debugPrint(uint8_t level, const char *func, const char *frm, ...);

#else

#define DEBUGOUT(...)
#endif


#endif
