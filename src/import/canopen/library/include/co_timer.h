/*
 * co_timer - public defines for timer usage
 *
 * Copyright (c) 2001-2017 port GmbH Halle/Saale
 *------------------------------------------------------------------
 */

/**
*  \file co_timer.h
*++ Defines for timer usage
*-- Definitionen für die Verwendung der Timerfunktionen
*  \author port GmbH Halle (Saale)
*
*++ The file contains definitions of structures and data types for timer usage
*-- Diese Datei enthält Definitionen, Strukturen und Datentypen für
*-- die Verwendung der Timerfunktionen
*/

#ifndef __CO_TIMER_H
# define __CO_TIMER_H

#include <co_def.h>

/* structure of a timer */

struct TIMER_EVENT {
	struct TIMER_EVENT  *pNext;	/**< pointer to next timer event */
	UNSIGNED32	    timerVal;	/**< timerVal in 1/10 of msec */
#ifdef CONFIG_LARGE_TIMER
	UNSIGNED32	    endTime;	/**< endtime in timerticks */
#else /* CONFIG_LARGE_TIMER */
	UNSIGNED16	    endTime;	/**< endtime in timerticks */
#endif /* CONFIG_LARGE_TIMER */
	UNSIGNED16	    restTime;	/**< rest time in 1/10 of msec */
	UNSIGNED8	    timerType;	/**< type of timer event */
};

typedef struct TIMER_EVENT TIMER_EVENT_T;


/* timer types for execution */
#define CO_TIMER_TYPE_CYCLIC	0x80u	/* for cyclic timers */
#define CO_TIMER_TYPE_AGAIN	0x40u	/* for start timer again */
#define CO_TIMER_TYPE_REMAIN	0x20u	/* use remain time */
#define CO_TIMER_TYPE_SYNC	1u	/* for sync transmit */
#define CO_TIMER_TYPE_HB_PROD	2u	/* for heartbeat producer transmit */
#define CO_TIMER_TYPE_HB_CONS	3u	/* for heartbeat consumer */
#define CO_TIMER_TYPE_NG_MSTR	4u	/* for Nodeguarding master */
#define CO_TIMER_TYPE_NG_SLAVE	5u	/* for Nodeguarding slave */
#define CO_TIMER_TYPE_EVENTRPDO	6u	/* for event rec pdos */
#define CO_TIMER_TYPE_EVENTTPDO	7u	/* for event trans pdos */
#define CO_TIMER_TYPE_FLYMA	8u	/* for flying master */
#define CO_TIMER_TYPE_REDCY	12u	/* for redundancy timer */
#define CO_TIMER_TYPE_SRDO_PROD	13u	/* for srdo producer */
#define CO_TIMER_TYPE_SRDO_CON	14u	/* for srdo consumer */
#define CO_TIMER_TYPE_LSS_SL	15u	/* for lss slave timer */
#define CO_TIMER_TYPE_LSS_MSTR	16u	/* for lss ,master timer */
#define CO_TIMER_TYPE_LED	17u	/* for led timer */
#define CO_TIMER_TYPE_SDO	18u	/* for sdo timeout monitoring */
#define CO_TIMER_TYPE_NMT_BOOT_TIME	19u	/* for NMT startup boot time */
#define CO_TIMER_TYPE_USERSPEC	20u	/**< for user specific timers */

/* external data declarations */

#endif		/*  __CO_TIMER_H */


/* function prototypes */
#ifdef CONFIG_WITHOUT_PROTOTYPES
#else /* CONFIG_WITHOUT_PROTOTYPES */

# ifndef __CO_TIMER_PROTOTYP_H
#  define __CO_TIMER_PROTOTYP_H

void	removeTimerEvent(TIMER_EVENT_T *pTimer CO_COMMA_LINE_PARA_DECL);
UNSIGNED8 addTimerEvent(TIMER_EVENT_T *pTimer, UNSIGNED32 timerVal,
	UNSIGNED8 timerType CO_COMMA_LINE_PARA_DECL);
UNSIGNED8 changeTimerEvent(TIMER_EVENT_T *pTimer, UNSIGNED32 timerVal,
	UNSIGNED8 timerType CO_COMMA_LINE_PARA_DECL);
void	userTimerEvent(TIMER_EVENT_T * CO_COMMA_LINE_PARA_DECL);
BOOL_T	checkActiveTimer(TIMER_EVENT_T	*pTimer CO_COMMA_LINE_PARA_DECL);
void	showTimerList(CO_LINE_PARA_DECL);
UNSIGNED32 co_getNextInternalEventTime(CO_LINE_PARA_DECL);

#ifdef CO_CONFIG_PDO_INHIBITTIME_RESEND
RET_T   coUserPdoInhibittimeInd(UNSIGNED16, UNSIGNED8 CO_COMMA_LINE_PARA_DECL);
#endif /* CO_CONFIG_PDO_INHIBITTIME_RESEND */

# endif /* __CO_TIMER_PROTOTYP_H */
#endif /* CONFIG_WITHOUT_PROTOTYPES */

/* end of source */
