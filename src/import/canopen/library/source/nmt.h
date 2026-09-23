/*
 * nmt - defines for nmt services
 *
 * Copyright (c) 2001-2017 port GmbH Halle/Saale
 *------------------------------------------------------------------
 */

/*
DESCRIPTION

The file contains definitions of structures and complex data types
for nmt services

*/

#include <co_stru.h>
#include <co_nmt.h>
#include "timer.h"
#ifdef CONFIG_CO_LED
# include "led.h"
#endif /* CONFIG_CO_LED */
#ifdef CONFIG_NMT_STARTUP_MANAGER
# include "nmtstart.h"
#endif /* CONFIG_NMT_STARTUP_MANAGER */


#ifndef PCO_NMT_H__
# define PCO_NMT_H__



/* structure of a node object */

struct LOCAL_NODE {
	TIMER_EVENT_T	timer;	/* timer structure (must be at first position)*/
	COB_T	    *pGuard_COB;	/* pointer to guarding COB */
#if defined(CONFIG_NODE_GUARDING)
	UNSIGNED8   bGuardToggle;	/* toggle bit for Node Guarding */
	UNSIGNED8   bLifeTimeFactor;	/* wGuardTime*bLifeTimeFactor = LifeTime*/
	UNSIGNED8   bSuspendedGuardings;/* suspended Life Guardings */
#endif
	NODE_STATE_T	eState;		/* node state */

	FLAG_T	   flags;		/* flag (see nmterr.h) */
};

struct REMOTE_NODE {
#ifdef CONFIG_REDUNDANCY_SUPPORT
struct	NODE	    *pRedcy;		/* pointer to redundant data */
#endif /* CONFIG_REDUNDANCY_SUPPORT */
	FLAG_T	    flags;		/* flag (see nmterr.h) */
	UNSIGNED8   nodeId;		/* Node-ID (1..127) */
	NODE_STATE_T  eState;		/* node state */
#ifdef CONFIG_REDUNDANCY_SUPPORT
	UNSIGNED8   hbCnt;
	BOOL_T	    redcyNode;		/* node is redundancy device */
#endif /* CONFIG_REDUNDANCY_SUPPORT */
};


struct GUARDING {
	TIMER_EVENT_T	timer;	/* timer structure (must be at first position)*/
	UNSIGNED8   bGuardToggle;	/* toggle bit for Node Guarding */
	UNSIGNED8   bLifeTimeFactor;	/* wGuardTime*bLifeTimeFactor = LifeTime*/
	UNSIGNED8   bSuspendedGuardings;/* suspended Life Guardings */
	COB_T	    *pGuard_COB;	/* pointer to guarding COB */
	NODE_STATE_T  eState;		/* node state */

	FLAG_T      flags;		/* flags only for master(see nmterr.h)*/
	UNSIGNED8   nodeId;		/* Node-ID (1..127) */
};

typedef struct LOCAL_NODE 	LOCAL_NODE_T;
typedef struct REMOTE_NODE 	REMOTE_NODE_T;
typedef struct GUARDING 	GUARDING_T;


/* NMT Codes */
#define CS_START_REMOTE_NODE	   1u
#define CS_STOP_REMOTE_NODE	   2u
#define CS_ENTER_PRE_OP_STATE	 128u
#define CS_RESET_APPLICATION 	 129u
#define CS_RESET_COMM        	 130u

/* error behavior defines */
#define EB_CHANGE_TO_PREOP	0u
#define EB_CHANGE_NONE		1u
#define EB_CHANGE_TO_STOP	2u

/* external variable declarations */

extern UNSIGNED8	coNodeId CO_LINE_PARA_ARRAY_DEF;/* CANopen Node Id */
extern UNSIGNED8	commErrorBehavior CO_LINE_PARA_ARRAY_DEF;

#if defined(CONFIG_MASTER) || defined(CONFIG_SLAVE_PLUS) || defined(CO_CONFIG_SELFSTARTING_SLAVE)
extern UNSIGNED32	co_nmtStartUp	CO_LINE_PARA_ARRAY_DEF;
#endif /* defined(CONFIG_MASTER) || defined(CONFIG_SLAVE_PLUS) || defined(CO_CONFIG_SELFSTARTING_SLAVE) */
#if defined(CONFIG_MASTER) || defined(CONFIG_SLAVE_PLUS)
extern COB_T		*co_pNMT_COB CO_LINE_PARA_ARRAY_DEF;
#endif /* defined(CONFIG_MASTER) || defined(CONFIG_SLAVE_PLUS) */

#ifdef CONFIG_REDUNDANCY_SUPPORT
extern LOCAL_NODE_T	co_Node;
extern LOCAL_NODE_T	co_redcyNode;
#else /* CONFIG_REDUNDANCY_SUPPORT */
extern LOCAL_NODE_T	co_Node CO_LINE_PARA_ARRAY_DEF;
#endif /* CONFIG_REDUNDANCY_SUPPORT */

#endif /* PCO_NMT_H__ */

#ifdef CONFIG_WITHOUT_PROTOTYPES
#else /* CONFIG_WITHOUT_PROTOTYPES */

# ifndef PCO_NMT_PROTOTYPES_H__
#  define PCO_NMT_PROTOTYPES_H__

/* function prototypes */

void	NMT_NodeStartStopMsg(CAN_MSG_T *canMsg CO_COMMA_REDCY_PARA_DECL);
void	setNodeState(NODE_STATE_T newState CO_COMMA_REDCY_PARA_DECL);
RET_T	initNmtState(CO_REDCY_PARA_DECL);
void	setupCommErrorBehavior(CO_LINE_PARA_DECL);
void	execCommErrorBehavior(CO_REDCY_PARA_DECL);

# ifdef CONFIG_CO_RUN_LED
void	updateNMTState_led(CO_REDCY_PARA_DECL);
# endif /* CONFIG_CO_RUN_LED */

# endif /* PCO_NMT_PROTOTYPES_H__ */
#endif /* CONFIG_WITHOUT_PROTOTYPES */


/* end of source */

