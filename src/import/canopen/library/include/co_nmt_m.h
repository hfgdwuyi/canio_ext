/*
 * co_nmt_m - public defines for nmt master usage
 *
 * Copyright (c) 2001-2017 port GmbH Halle/Saale
 *------------------------------------------------------------------
 */

/**
*  \file co_nmt_m.h
*++ Defines for NMT master usage
*-- Definitionen für die Verwendung eines NMT-Master
*  \author port GmbH Halle (Saale)
*
*++ The file contains definitions of structures and data types for
*++ NMT master usage.
*-- Diese Datei enthält Definitionen von Strukturen und Datentypen
*-- zur Verwendung eines NMT-Master.
*/

#ifndef __CO_NMT_M_H
# define __CO_NMT_M_H

# include <co_def.h>		/* include canopen definition */


/* external data declarations */

/* function prototypes */

RET_T	createNetworkReq (CO_LINE_PARA_DECL);
#ifdef CONFIG_REDUNDANCY_SUPPORT
RET_T	addRemoteNodeReq (UNSIGNED8, UNSIGNED32, UNSIGNED8, BOOL_T, BOOL_T
                          CO_COMMA_LINE_PARA_DECL);
#else /* CONFIG_REDUNDANCY_SUPPORT */
RET_T   addRemoteNodeReq (UNSIGNED8, UNSIGNED16, UNSIGNED8, BOOL_T, BOOL_T
                          CO_COMMA_LINE_PARA_DECL);
#endif /* CONFIG_REDUNDANCY_SUPPORT */
void 	deleteNetworkReq (CO_LINE_PARA_DECL);
RET_T	removeRemoteNodeReq(UNSIGNED8 CO_COMMA_LINE_PARA_DECL);

RET_T	startRemoteNodeReq (UNSIGNED8 CO_COMMA_REDCY_PARA_DECL);
RET_T	stopRemoteNodeReq  (UNSIGNED8 CO_COMMA_REDCY_PARA_DECL);
RET_T 	enterPreOpStateReq(UNSIGNED8 CO_COMMA_REDCY_PARA_DECL);
RET_T 	resetNodeReq(UNSIGNED8 CO_COMMA_REDCY_PARA_DECL);
RET_T 	resetCommReq(UNSIGNED8 CO_COMMA_REDCY_PARA_DECL);


#endif		/*  __CO_NMT_M_H */

/* end of source */

