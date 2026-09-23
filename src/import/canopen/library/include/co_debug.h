/****************************************************************************
 *
 * co_debug -	 Makros for library debugging
 *
 * Copyright (c) 2000-2017 port GmbH Halle
 *------------------------------------------------------------------
 */

/**
*  \file co_debug.h
*++ Defines for CANopen library debugging
*-- Definitionen für die Fehlersuche in der CANopen library
*  \author port GmbH Halle (Saale)
*
*++ This file contains macros for debug the CANopen library.
*++ Debug messages can be enabled/disabled at runtime
*++ over the variable co_debug.
*++ This variable is bitcoded.
*-- Diese Datei enthält Makros für die Fehlersuche in der CANopen
*-- Bibliothek.
*-- Debug Telegramme können zur Laufzeit über die Variable
*-- co_debug aktiviert und deaktiviert werden.
*-- Diese Variable ist bitcodiert.
*/

#ifndef __CO_DEBUG_H
# define __CO_DEBUG_H

#ifndef CO_CONST
# define CO_CONST const
#endif

# ifdef CONFIG_RCS_IDENT
static CO_CONST char _rcs_debug_h[] = "$Id: co_debug.h,v 2.19 2016/09/26 11:15:31 rli Exp $";
# endif

extern int co_debug;

# ifndef _STDIO_H_
#  include <stdio.h>
# endif

# ifndef PRINTF
#  ifdef TARGET_LINUX
#   define PRINTF fprintf(stderr,
#  else /* TARGET_LINUX */
#   define PRINTF printf
#  endif /* else TARGET_LINUX */
# endif /* ifndef PRINTF */


# ifdef CONFIG_CO_DEBUG
#  ifndef BDEBUG

#if defined TARGET_LINUX
#  define BDEBUG(type, fmt, str...) \
	fprintf(stderr, "%s:%s (%i) - %d: " fmt, \
	__FILE__, __func__, __LINE__, type, ##str)
#else
#   define BDEBUG		debugprint
#endif

#  endif /* ifndef BDEBUG */
# else /* CONFIG_CO_DEBUG */
#  ifndef BDEBUG
#   if defined(TARGET_LINUX) || defined(TARGET_APC)
#     define BDEBUG(args...)
#   elif defined(TARGET_FUJITSU_90540)
#     define BDEBUG		//
#   else /* TARGET_LINUX */
#     define BDEBUG //
#   endif /* else TARGET_LINUX */
#  endif /*  BDEBUG */
# endif /* else CONFIG_CO_DEBUG */

/* Debuglevels */
#define CO_DEBUG_CPU		(1 << 0)
#define CO_DEBUG_CAN		(1 << 1)
#define CO_DEBUG_SDO		(1 << 2)
#define CO_DEBUG_SDOBLOCK	(1 << 3)
#define CO_DEBUG_SDOMANAGER	(1 << 4)
#define CO_DEBUG_PDO		(1 << 5)
#define CO_DEBUG_LSS		(1 << 6)

/* Funktionen zur Meldungsausgabe aus co_debug.c */
void debugprint(int level, char *fmt, ...);

#endif /* __CO_DEBUG_H */

