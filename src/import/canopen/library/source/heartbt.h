/*
 * heartbt - defines for heartbeat services
 *
 * Copyright (c) 2001-2017 port GmbH Halle/Saale
 *------------------------------------------------------------------
 */

/*
DESCRIPTION

The file contains definitions of structures and complex data types
for heartbeat services

*/

#include <co_def.h>
#include <co_hb.h>
#include <co_timer.h>
#include <co_nmt.h>
#include <co_stru.h>

#ifndef PCO_HEARTBT_H__
# define PCO_HEARTBT_H__


typedef struct {
	TIMER_EVENT_T   timer;  /* timer structure (must be at first position)*/
	COB_T		*pGuard_COB;	/* pointer to COB */
	NODE_STATE_T	eState;		/* node state */
	FLAG_T		mflags;		/* flag (see nmterr.h) */
	UNSIGNED8	nodeId;
#ifdef CONFIG_REDUNDANCY_SUPPORT
	UNSIGNED8	line;		/* default or reduncy line */
	UNSIGNED8	hbCnt;
# ifdef CONFIG_MARITIME_SUPPORT
	BOOL_T		redcyNode;	/* node is redundancy device */
	BOOL_T		redcyLine;	/* not redundant, only connected to redcy line*/
# endif /* CONFIG_MARITIME_SUPPORT */
#endif /* CONFIG_REDUNDANCY_SUPPORT */
	BOOL_T		eStateChanged;	/* node state was changed */
} HB_CONS_T;


/* external variable declarations */
# ifdef CONFIG_DYN_MEM_ALLOC
extern HB_CONS_T	*p_hbConsList[];
extern HB_CONS_T	*p_redcyHbConsList[];
extern UNSIGNED16	co_maxHbConsCnt;
extern UNSIGNED8	*p_hbIdxList[];
# else /* CONFIG_DYN_MEM_ALLOC */
extern HB_CONS_T	hbConsList[];
extern HB_CONS_T	redcyHbConsList[];
# endif /* CONFIG_DYN_MEM_ALLOC */
extern UNSIGNED16	co_hbConsLineOffs[];
extern UNSIGNED8	co_hbConsLineCnts[];

#endif /* PCO_HEARTBT_H__ */

#ifdef CONFIG_WITHOUT_PROTOTYPES
#else /* CONFIG_WITHOUT_PROTOTYPES */

# ifndef PCO_HEARTBT__PROTOTYPES_H__
#  define PCO_HEARTBT__PROTOTYPES_H__

/* function prototypes */

RET_T	setHeartBeatConsumerTime(UNSIGNED32 hbEntry, UNSIGNED8 subIndex,
		BOOL_T sortHbList CO_COMMA_LINE_PARA_DECL);
UNSIGNED8 getHeartBeatIndex(UNSIGNED8 bNodeId CO_COMMA_LINE_PARA_DECL);
void	NMT_HB_Cons_TimerPulse(TIMER_EVENT_T *pTimer CO_COMMA_LINE_PARA_DECL);
RET_T	setHbNodeState(UNSIGNED8 nodeNr, NODE_STATE_T newState CO_COMMA_LINE_PARA_DECL);
void	hbMsgReceived(UNSIGNED8 idx, UNSIGNED8 state CO_COMMA_REDCY_PARA_DECL);
NODE_STATE_T getHbNodeState(UNSIGNED8 nodeNr CO_COMMA_REDCY_PARA_DECL);
void	initHeartBeatVars(CO_LINE_PARA_DECL);
RET_T	setHeartBeatSignaling(UNSIGNED8	nodeNr CO_COMMA_LINE_PARA_DECL);
void	setHbBootupState(UNSIGNED8 nodeId CO_COMMA_REDCY_PARA_DECL);

# ifdef CONFIG_REDUNDANCY_SUPPORT
BOOL_T	checkRedcyHBConsumer(CO_GLOBVARS_PARA_DECL);
# endif /* CONFIG_REDUNDANCY_SUPPORT */

# endif /* PCO_HEARTBT__PROTOTYPES_H__ */
#endif /* CONFIG_WITHOUT_PROTOTYPES */


/* end of source */
