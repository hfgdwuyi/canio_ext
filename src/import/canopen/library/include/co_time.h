/*
 * co_time - public defines for time usage
 *
 * Copyright (c) 2001-2017 port GmbH Halle/Saale
 *------------------------------------------------------------------
 */

/**
*  \file co_time.h
*++ Defines for the time stamp service
*-- Definitionen für den Timestamp-Dienst
*  \author port GmbH Halle (Saale)
*
*++ The file contains definitions of structures and data types for time usage
*-- Diese Datei enthält Definitionen von Strukturen und Datentypen
*-- zur Verwendung des Timestamp-Dienstes
*
*/

#ifndef __CO_TIME_H
# define __CO_TIME_H

#include <co_def.h>
#include <co_cobid.h>		/* include cobid definition */

#define TIME_CONSUMER_BIT       0x80000000UL
#define TIME_PRODUCER_BIT       0x40000000UL


typedef struct
{
    UNSIGNED32 time;    /**< time in ms after midnight */
    UNSIGNED16 days;    /**< number of day since January 1, 1984 */
} TIME_OF_DAY_T;


/* external data declarations */
#endif		/*  __CO_TIME_H */

#ifdef CONFIG_WITHOUT_PROTOTYPES
#else /* CONFIG_WITHOUT_PROTOTYPES */

# ifndef __CO_TIME_PROTOTYPES_H
#  define __CO_TIME_PROTOTYPES_H

/* function prototypes */

RET_T 	defineTime(CO_USER_T CO_COMMA_LINE_PARA_DECL);
RET_T 	writeTimeReq( UNSIGNED32, UNSIGNED16 CO_COMMA_LINE_PARA_DECL);
RET_T 	readTimeReq(CO_LINE_PARA_DECL);

void 	timeInd(TIME_OF_DAY_T * CO_COMMA_LINE_PARA_DECL);

# endif /* __CO_TIME_PROTOTYPES_H */
#endif /* CONFIG_WITHOUT_PROTOTYPES */


/* end of source */

