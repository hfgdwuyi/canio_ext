/*
 * co_hb - public defines for heart beat
 *
 * Copyright (c) 2001-2017 port GmbH Halle/Saale
 *------------------------------------------------------------------
 */

/**
*  \file co_hb.h
*++ Defines for heartbeat usage
*-- Definitionen für die Verwendung des Heartbeat-Dienstes
*  \author port GmbH Halle (Saale)
*
*++ The file contains definitions of structures and data types for
*++ heartbeat usage
*-- Diese Datei enthält Definitionen von Strukturen und Datentypen
*-- zur Verwendung des Hearbeat-Dienstes
*/

#ifndef __CO_HB_H
# define __CO_HB_H


#include <co_def.h>		/* include canopen definition */

/** Defines the legth of a valid heartbeat-msg */
#ifndef PCO_VALID_MESSAGE_LENGTH_HBC
# define PCO_VALID_MESSAGE_LENGTH_HBC 1
#endif /* PCO_VALID_MESSAGE_LENGTH_HBC */

/* hearbeat consumer entries */
#define HB_CONS_TIME_MASK	0x0000ffff
#define HB_CONS_NODEID_MASK	0x00ff0000
#define HB_CONS_REDCY_MASK	0x03000000
#define HB_CONS_REDCY_NODE	0x02000000

#ifdef CONFIG_REDUNDANCY_SUPPORT
# define HB_CONS_RESERVED_MASK	(0xff000000 ^ HB_CONS_REDCY_MASK)
#else /* CONFIG_REDUNDANCY_SUPPORT */
# define HB_CONS_RESERVED_MASK	0xff000000
#endif /* CONFIG_REDUNDANCY_SUPPORT */

/* external data declarations */

#endif		/*  __CO_HB_H */


#ifdef CONFIG_WITHOUT_PROTOTYPES
#else /* CONFIG_WITHOUT_PROTOTYPES */

# ifndef __CO_HB_PROTOTYPES_H
#  define __CO_HB_PROTOTYPES_H

/* function prototypes */

RET_T	defineHeartbeatConsumer(CO_LINE_PARA_DECL);
# ifdef CONFIG_REDUNDANCY_SUPPORT
RET_T   setHeartBeatTime(UNSIGNED8, UNSIGNED32 CO_COMMA_LINE_PARA_DECL);
# else /* CONFIG_REDUNDANCY_SUPPORT */
RET_T	setHeartBeatTime(UNSIGNED8, UNSIGNED16 CO_COMMA_LINE_PARA_DECL);
# endif /* CONFIG_REDUNDANCY_SUPPORT */
RET_T	startHeartBeatReq(UNSIGNED8 CO_COMMA_LINE_PARA_DECL);
RET_T	stopHeartBeatReq(UNSIGNED8 CO_COMMA_LINE_PARA_DECL);
BOOL_T	checkAllHBConsumer(BOOL_T allCfgNodes CO_COMMA_LINE_PARA_DECL);

# endif /* __CO_HB_PROTOTYPES_H */
#endif /* CONFIG_WITHOUT_PROTOTYPES */


/* end of source */

