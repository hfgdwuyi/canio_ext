/*
 * nmt_m - defines for nmt master services
 *
 * Copyright (c) 2001-2017 port GmbH Halle/Saale
 *------------------------------------------------------------------
 */

/*
DESCRIPTION

The file contains definitions of structures and complex data types
for nmt master services

*/

#ifndef PCO_NMT_M_H__
# define PCO_NMT_M_H__


#include "nmt.h"
#include <co_nmt_m.h>


/* defines for object 0x1f80 */
#define NMT_STARTUP_MASTER_BIT			0x00000001
#define NMT_STARTUP_START_ALL_NODE_BIT		0x00000002
#define NMT_STARTUP_MASTER_START_BIT		0x00000004
#define NMT_STARTUP_NOT_START_NODE_BIT		0x00000008
#define NMT_STARTUP_RESET_ALL_NODES_BIT		0x00000010
#define NMT_STARTUP_FLYING_MASTER_BIT		0x00000020
#define NMT_STARTUP_STOP_ALL_NODES_BIT		0x00000040

#define NMT_STARTUP_FLYING_MSTR	(NMT_STARTUP_MASTER_BIT | NMT_STARTUP_FLYING_MASTER_BIT)


/* external variable declarations */

#ifdef CONFIG_MULT_LINES
extern UNSIGNED16	co_nmtSlaveLineOffs[];
extern UNSIGNED8	co_guardSlaveLineCnts[];
extern UNSIGNED16	co_guardSlaveLineOffs[];
#  ifdef CONFIG_DYN_MEM_ALLOC
extern UNSIGNED8	co_nmtSlaveLineCnts[];
#  else /* CONFIG_DYN_MEM_ALLOC */
extern CO_CONST UNSIGNED8 co_nmtSlaveLineCnts[];
#  endif /* CONFIG_DYN_MEM_ALLOC */
# endif /* CONFIG_MULT_LINES */

#  ifdef CONFIG_DYN_MEM_ALLOC
extern REMOTE_NODE_T	*p_nmtSlaveList[];
extern GUARDING_T	*p_guardSlaveList[];
extern UNSIGNED16	co_maxNmtSlaves;
extern UNSIGNED16	co_maxGuardSlaves;
extern UNSIGNED8	*p_nmtSlaveIdxList[1];
extern UNSIGNED8	*p_guardSlaveIdxList[1];
#  else /* CONFIG_DYN_MEM_ALLOC */
extern REMOTE_NODE_T	nmtSlaveList[];
extern GUARDING_T	guardSlaveList[];
#  endif /* CONFIG_DYN_MEM_ALLOC */

/* function prototypes */
NODE_STATE_T	getNmtSlaveNodeState(UNSIGNED8 nodeNr CO_COMMA_LINE_PARA_DECL);
NODE_STATE_T	getGuardNodeState(UNSIGNED8 nodeNr CO_COMMA_LINE_PARA_DECL);
UNSIGNED8	getGuardSlaveIndex(UNSIGNED8 nodeId CO_COMMA_LINE_PARA_DECL);
void		guardMsgReceived(UNSIGNED16 idx, UNSIGNED8 *canData
			CO_COMMA_LINE_PARA_DECL);
RET_T		setGuardNodeState(UNSIGNED8 nodeNr, NODE_STATE_T newState
			CO_COMMA_LINE_PARA_DECL);
void		NMT_M_TimerPulse(TIMER_EVENT_T *pTimer CO_COMMA_LINE_PARA_DECL);
UNSIGNED8	getNmtSlaveIndex(UNSIGNED8 nodeId CO_COMMA_LINE_PARA_DECL);
void		initNmtMasterVars(CO_LINE_PARA_DECL);
void		initGuardVars(CO_LINE_PARA_DECL);
REMOTE_NODE_T   *getRemoteNodePtr(UNSIGNED8  nodeId CO_COMMA_REDCY_PARA_DECL);

#endif /* PCO_NMT_M_H__ */

/* end of source */

