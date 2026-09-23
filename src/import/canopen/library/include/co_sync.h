/*
 * co_sync - public defines for sync usage
 *
 * Copyright (c) 2001-2017 port GmbH Halle/Saale
 *------------------------------------------------------------------
 */

/**
*  \file co_sync.h
*++ Defines for the SYNC service
*-- Definitionen für die Verwendung des SYNC-Dienstes
*  \author port GmbH Halle (Saale)
*
*++ The file contains definitions of structures and data types for
*++ sync
*-- Diese Datei enthält Definitionen von Strukturen und Datentypen
*-- für die Verwendung des SYNC-Dienstes
*/

#ifndef __CO_SYNC_H
# define __CO_SYNC_H

# include <co_def.h>		/* include canopen definition */

#define SYNC_PRODUCER_BIT	0x40000000UL


/* external data declarations */
#ifdef CONFIG_REDUNDANCY_SUPPORT
extern UNSIGNED8	co_syncCnt;	/* SYNC counter */
#else /* CONFIG_REDUNDANCY_SUPPORT */
extern UNSIGNED8	co_syncCnt CO_LINE_PARA_ARRAY_DEF; /* SYNC counter */
#endif /* CONFIG_REDUNDANCY_SUPPORT */

#endif		/*  __CO_SYNC_H */


#ifdef CONFIG_WITHOUT_PROTOTYPES
#else /* CONFIG_WITHOUT_PROTOTYPES */

# ifndef __CO_SYNC__PROTOTYPES_H
#  define __CO_SYNC__PROTOTYPES_H

/* function prototypes */

RET_T	defineSync(CO_USER_T CO_COMMA_LINE_PARA_DECL);
RET_T	startSyncReq(CO_LINE_PARA_DECL);
RET_T	stopSyncReq(CO_LINE_PARA_DECL);

void 	syncCommand(CO_LINE_PARA_DECL);
void 	syncPreCommand(CO_LINE_PARA_DECL);

#ifdef CO_CONFIG_SYNC_SEND_IND
void sendSyncInd(UNSIGNED8 syncCnt, RET_T returnCode
        CO_COMMA_LINE_PARA_DECL);
#endif /*CO_CONFIG_SYNC_SEND_IND*/


# endif /* __CO_SYNC__PROTOTYPES_H */
#endif /* CONFIG_WITHOUT_PROTOTYPES */


/* end of source */

