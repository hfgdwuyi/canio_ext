/*
 * nmterr - defines for nmt error control usage
 *
 * Copyright (c) 2001-2017 port GmbH Halle/Saale
 *------------------------------------------------------------------
 */

/*
DESCRIPTION

The file contains definitions of structures and data types for
 nmt error control usage

*/

#ifndef PCO_NMTERROR_H__
# define PCO_NMTERROR_H__

#include <co_stru.h>
#include "timer.h"


/* flags for local node */
#define NMTERRFLAG_NG_POSSIBLE	2u	/* Guarding possible */
#define NMTERRFLAG_LG_ISSET	8u	/* LifeGuarding possible */
#define NMTERRFLAG_LG_ACTIVE	0x10u	/* lifetime is enabled */
#define NMTERRFLAG_HB_ACTIVE	0x20u	/* Heartbeat active */
#define NMTERRFLAG_HB_POSSIBLE	0x40u	/* Heartbeat possible */
#define NMTERRFLAG_MASTER	0x80u	/* node works as master */


/* flags for remote nodes */
#define GUARDFLAG_NG_RECEIVED	1u	/* Guarding Telegramm received */
#define GUARDFLAG_NG_POSSIBLE	2u	/* Guarding possible */
#define GUARDFLAG_NG_ACTIVE	4u	/* Guarding is active */
#define GUARDFLAG_NG_NOTE	8u	/* Guarding start signaled */
#define GUARDFLAG_SIGNAL	0x10u	/* signal next nmterr msg */
#define GUARDFLAG_HB_ACTIVE	0x20u	/* Heartbeat active */
#define GUARDFLAG_HB_POSSIBLE	0x40u	/* Heartbeat possible */
#define GUARDFLAG_HB_NOTE	0x80u	/* Heartbeat start signaled */



#define NMTERROR_STATE_UNCONFIG	0	/* nmterror unconfigured */
#define NMTERROR_STATE_CONFIG	1	/* nmterror configured */
#define NMTERROR_STATE_STARTED	2	/* nmterror started */
#define NMTERROR_STATE_FAILED	3	/* nmterror failed */
#define NMTERROR_STATE_3HB_OK	4	/* nmterror 3 hb ok */

#define NMTERR_MAX_INDEX	(CO_MAX_NODE >> 3)

/* external data declarations */
extern UNSIGNED8	nmtErrFailed[] CO_REDCY_PARA_ARRAY_DEF;
extern UNSIGNED8	nmtErrStarted[] CO_REDCY_PARA_ARRAY_DEF;
extern UNSIGNED8	nmtErrConfig[] CO_REDCY_PARA_ARRAY_DEF;
# ifdef CONFIG_REDUNDANCY_SUPPORT
extern UNSIGNED8	nmtErr3HBok[];
#  ifdef CONFIG_MARITIME_SUPPORT
extern UNSIGNED8	nmtErrRedundancy[];
#  endif /* CONFIG_MARITIME_SUPPORT */
# endif /* CONFIG_REDUNDANCY_SUPPORT */

/* function prototypes */

void	NMT_M_NodeGuardingMsg (CAN_MSG_T *canMsg CO_COMMA_REDCY_PARA_DECL);
void	NMT_NodeGuardingMsg (CO_LINE_PARA_DECL);
void	NMT_HB_TimerPulse (CO_LINE_PARA_DECL);
void	NMT_NG_TimerPulse (CO_LINE_PARA_DECL);
RET_T	setLifeTimePara(UNSIGNED16 lifeTime, UNSIGNED8 faktor
		CO_COMMA_LINE_PARA_DECL);
RET_T	setHeartBeatProducerTime(UNSIGNED16 hbTime CO_COMMA_LINE_PARA_DECL);
void	nmtErrNodeFailed(UNSIGNED8 nodeId, UNSIGNED8 failed
		CO_COMMA_REDCY_PARA_DECL);
void	initNmtErrVars(CO_LINE_PARA_DECL);
RET_T	initNmtErr(CO_LINE_PARA_DECL);


#endif		/*  PCO_NMTERROR_H__ */

/* end of source */

