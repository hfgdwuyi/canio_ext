/*
 * sync - defines for sync usage
 *
 * Copyright (c) 2001-2017 port GmbH Halle/Saale
 *------------------------------------------------------------------
 */

/*
DESCRIPTION

The file contains definitions of structures and data types for sync

*/


# include <co_stru.h>
# include <co_sync.h>

# include "timer.h"

#ifndef PCO_SYNC_H__
# define PCO_SYNC_H__

/* defines for sync flags */

#define CO_SYNC_FLAG_INIT	1	/* sync is initialized */
#define CO_SYNC_FLAG_ENABLED	2	/* sync producer is enabled,not active*/
#define CO_SYNC_FLAG_ACTIVE	4	/* sync producer is active */

#define ERRCODE_BAD_SYNCLEN	0x8240	/* bad length */

/* structure of a SYNC */

struct SYNC_OBJ {
	TIMER_EVENT_T	timer;		/* tdimer structure */
	COB_T		*pCOB;		/* COB structure */
	UNSIGNED8	maxCounter;	/* max sync counter */
	FLAG_T		flags;		/* sync flags */
};

typedef struct SYNC_OBJ SYNC_T;


/* external data declarations */

extern UNSIGNED32 coInternSyncPeriod CO_LINE_PARA_ARRAY_DEF;  /* internal time for sync */
extern UNSIGNED32 coLastSyncTime CO_LINE_PARA_ARRAY_DEF;  /* last sync time */
extern SYNC_T	co_Sync CO_LINE_PARA_ARRAY_DEF;		/* SYNC Object */
#endif		/*  PCO_SYNC_H__ */


#ifdef CONFIG_WITHOUT_PROTOTYPES
#else /* CONFIG_WITHOUT_PROTOTYPES */

# ifndef __SYNC_PROTOTYPES_H
#  define __SYNC_PROTOTYPES_H

/* function prototypes */

void 	writeSyncReq(CO_LINE_PARA_DECL);
RET_T	setSyncCobId(UNSIGNED32 cobId CO_COMMA_LINE_PARA_DECL);
RET_T	setSyncTimePara(UNSIGNED32 timeVal CO_COMMA_LINE_PARA_DECL);
RET_T	setSyncCounter(UNSIGNED8 syncCnt CO_COMMA_LINE_PARA_DECL);
void	initSyncVars(CO_LINE_PARA_DECL);
void	resetSyncCounter(CO_REDCY_PARA_DECL);
RET_T	initSync(CO_LINE_PARA_DECL);

# endif /* __SYNC_PROTOTYPES_H */
#endif /* CONFIG_WITHOUT_PROTOTYPES */

/* end of source */

