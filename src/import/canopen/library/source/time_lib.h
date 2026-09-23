/*
 * time - defines for time usage
 *
 * Copyright (c) 2001-2017 port GmbH Halle/Saale
 *------------------------------------------------------------------
 */

/*
DESCRIPTION

The file contains definitions of structures and data types for time usage

*/

#ifndef PCO_TIME_H__
# define PCO_TIME_H__

#include "co_stru.h"
#include "co_time.h"

/* defines for flags */
#define TIMEFLAG_INITIALIZED	1
#define TIMEFLAG_CONSUMER	2
#define TIMEFLAG_PRODUCER	4


struct CO_TIME {
	COB_T		*pCOB;		/* COB for Request/Response */
	UNSIGNED16	cobId;
	FLAG_T   	flags;		/* Time-flags: init... */
};

typedef struct CO_TIME CO_TIME_T;

/* external data declarations */
extern CO_TIME_T	co_Time CO_LINE_PARA_ARRAY_DEF;

#endif		/*  PCO_TIME_H__ */



#ifdef CONFIG_WITHOUT_PROTOTYPES
#else /* CONFIG_WITHOUT_PROTOTYPES */

# ifndef __TIME_PROTOTYPES_H
#  define __TIME_PROTOTYPES_H

/* function prototypes */

void	timeMsgReceived(CAN_MSG_T *canMsg CO_COMMA_LINE_PARA_DECL);
RET_T	setTimeCobId(UNSIGNED32 cobId CO_COMMA_LINE_PARA_DECL);
void	initTimeVars(CO_LINE_PARA_DECL);

# endif /* __TIME_PROTOTYPES_H */
#endif /* CONFIG_WITHOUT_PROTOTYPES */

/* end of source */

