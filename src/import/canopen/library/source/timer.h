/*
 * timer - defines for timer usage
 *
 * Copyright (c) 2001-2017 port GmbH Halle/Saale
 *------------------------------------------------------------------
 */

/*
DESCRIPTION

The file contains definitions of structures and data types for timer usage

*/


#ifndef PCO_TIMER_H__
# define PCO_TIMER_H__

#include <co_timer.h>

struct INHIBIT_EVENT {
	struct INHIBIT_EVENT	*pNext;
	UNSIGNED16		ticks;		/* timer ticks */
# ifdef CO_CONFIG_PDO_INHIBITTIME_RESEND
        UNSIGNED8 *pPdo;
# endif /* CO_CONFIG_PDO_INHIBITTIME_RESEND */
};

typedef struct INHIBIT_EVENT INHIBIT_EVENT_T;

/* external data declarations */

extern TIMER_EVENT_T	*co_timerList CO_LINE_PARA_ARRAY_DEF;
extern INHIBIT_EVENT_T	*co_inhibitList CO_LINE_PARA_ARRAY_DEF;




/* function prototypes */
#ifdef CONFIG_WITHOUT_PROTOTYPES
#else /* CONFIG_WITHOUT_PROTOTYPES */

# ifndef PCO_TIMER_PROTOTYPES_H__
#  define PCO_TIMER_PROTOTYPES_H__

void checkTimerEvent(CO_LINE_PARA_DECL);
void startInhibitTimer(INHIBIT_EVENT_T *pInhibit, UNSIGNED16 timerVal CO_COMMA_LINE_PARA_DECL);
void stopInhibitTimer(INHIBIT_EVENT_T *pInhibit CO_COMMA_LINE_PARA_DECL);

# endif /* PCO_TIMER_PROTOTYPES_H__ */
#endif /* CONFIG_WITHOUT_PROTOTYPES */

#endif		/*  PCO_TIMER_H__ */
/* end of source */
