/*
 *++ guard - additional node-guarding routines for a CANopen master
 *-- guard - Zusätzliche Node Guarding Routinen für einen CANopen Master
 *
 * Copyright (c) 2001-2017 port GmbH Halle (Saale)
 *------------------------------------------------------------------
 */


/****************************************************************************/
/**
*  \file guard.c
*++ Additional node-guarding routines for a CANopen master
*-- Zusätzliche Node Guarding Routinen für einen CANopen Master
*  \author port GmbH Halle (Saale)
*
*++ Additional services for Node Guarding
*++ are defined in this module.
*++ They are necessary for the CANopen Communication Profile DS 301.
*++ It contains functions for
*++ parametrization, start and stop of Node Guarding for a Minimum Capability
*++ Master Device.
*-- In diesem Modul sind die Nodeguarding Überwachungs-Dienste definiert.
*-- Dazu gehören Funktionen zur Parametrierung,
*-- sowie zum Start und Stop des Node Guardings.
*
* \par
*++ All functions of this module are only valid for master applications.
*-- Alle Funktionen dieses Moduls können nur für Masterapplikationen
*-- genutzt werden.
*/

/* header of standard C - libraries */

#include <stdio.h>
#include <string.h>

/* header of project specific types */

#include <cal_conf.h>
#include <co_guard.h>
#include <co_cobid.h>
#include "drv.h"
#include "nmterr.h"
#include "utility.h"
#if defined(CONFIG_MASTER)
# include "nmt_m.h"
# include <co_guard.h>
#endif /* defined(CONFIG_MASTER) */
#ifdef CONFIG_HEARTBEAT_CONSUMER
# include "heartbt.h"
#endif /* CONFIG_HEARTBEAT_CONSUMER */

#ifdef CONFIG_NMT_STARTUP_MANAGER
# include "nmtstart.h"
#endif /* CONFIG_NMT_STARTUP_MANAGER */


/* constant definitions
---------------------------------------------------------------------------*/

/* local defined data types
---------------------------------------------------------------------------*/

/* list of external used functions, if not in headers
---------------------------------------------------------------------------*/

/* list of global defined functions
---------------------------------------------------------------------------*/

/* list of local defined functions
---------------------------------------------------------------------------*/

/* external variables
---------------------------------------------------------------------------*/

/* global variables
---------------------------------------------------------------------------*/

/* local defined variables
---------------------------------------------------------------------------*/
#ifdef CONFIG_RCS_IDENT
CO_LIB_INIT_VAR static char _rcsid[] = "$Id: guard.c,v 2.33 2016/09/26 11:16:08 rli Exp $";
#endif /* CONFIG_RCS_IDENT */

#ifdef CONFIG_NO_GLOBAL_VARS
#else /* CONFIG_NO_GLOBAL_VARS */

# ifdef CONFIG_MASTER
#  ifdef CONFIG_NODE_GUARDING
#   ifdef CONFIG_DYN_MEM_ALLOC
CO_LIB_UNINIT_VAR GUARDING_T	*p_guardSlaveList[1];
CO_LIB_UNINIT_VAR UNSIGNED16	co_maxGuardSlaves;
#   else /* CONFIG_DYN_MEM_ALLOC */
CO_LIB_UNINIT_VAR GUARDING_T	guardSlaveList[CONFIG_GUARD_SLAVE_CNT];
#   endif /* CONFIG_DYN_MEM_ALLOC */
#   ifdef CONFIG_FAST_SORT
#    ifdef CONFIG_DYN_MEM_ALLOC
CO_LIB_UNINIT_VAR UNSIGNED8	*p_guardSlaveIdxList[1];
#    else /* CONFIG_DYN_MEM_ALLOC */
CO_LIB_UNINIT_VAR static UNSIGNED8	guardSlaveIdxList[CONFIG_GUARD_SLAVE_CNT];
#    endif /* CONFIG_DYN_MEM_ALLOC */
#   endif /* CONFIG_FAST_SORT */

#   ifdef CONFIG_MULT_LINES
		/* line counters */
#    ifdef CONFIG_DYN_MEM_ALLOC
CO_LIB_UNINIT_VAR UNSIGNED8	co_guardSlaveLineCnts[CO_MAX_CAN_LINES];
#    else /* CONFIG_DYN_MEM_ALLOC */
CO_LIB_INIT_VAR UNSIGNED8	co_guardSlaveLineCnts[CO_MAX_CAN_LINES] =
			    { CONFIG_GUARD_SLAVE_LINECFG };
#    endif /* CONFIG_DYN_MEM_ALLOC */
		/* line offsets */
CO_LIB_UNINIT_VAR UNSIGNED16	co_guardSlaveLineOffs CO_LINE_PARA_ARRAY_DEF;
#   endif /* CONFIG_MULT_LINES */
#  endif /* CONFIG_NODE_GUARDING */
# endif /* CONFIG_MASTER */
#endif /* CONFIG_NO_GLOBAL_VARS */


#ifdef CONFIG_MASTER
# ifdef CONFIG_NODE_GUARDING
#  ifdef CONFIG_MULT_LINES
#    define CO_GUARD_SLAVE_LINE_CNTS	GL_ARRAY(co_guardSlaveLineCnts)
#  else /* CONFIG_MULT_LINES */
#    define CO_GUARD_SLAVE_LINE_CNTS	CONFIG_GUARD_SLAVE_CNT
#  endif /* CONFIG_MULT_LINES */
# endif /* CONFIG_NODE_GUARDING */
#endif /* CONFIG_MASTER */


#ifdef CONFIG_MASTER
# ifdef CONFIG_NODE_GUARDING

/****************************************************************************/
/**
*++ startNodeGuardReq -  starts the Node Guarding
*-- startNodeGuardReq -  startet das Node Guarding
*
*++ This function starts the Node Guarding of a master,
*++ which is a minimum boot up master, for the specified
*++ node guarding slave.
*++ The information for guarding (guarding time, lifetime factor)
*++ have to be set with the function setGuardTimePara().
*-- Diese Funktion startet das Node Guarding auf einem Master
*-- für den übergebenen Slave.
*-- Die Überwachungsdaten (Überwachungszeit, Lifetime Faktor)
*-- sind vorher mit der Funktion
*-- setGuardTimePara() einzutragen.
*
* \par
*++ Nodeguarding is done automatically by the master.
*++ Only if the master detects a guarding error at one or more nodes
*++ the indication function mGuardErrorInd() is called.
*-- Die Knotenüberwachung wird automatisch vom Guardingmaster
*-- durchgeführt.
*-- Sobald der Master bei einem Knoten einen Guarding-Fehler feststellt,
*-- wird die Indikationfunktion
*-- mGuardError() aufgerufen.
*
* \retval CO_OK
*++ success
*-- Erfolg
* \retval CO_E_NOT_EXIST
*++ node doesn't exist
*-- der gewählte Knoten existiert nicht
*
*/

RET_T startNodeGuardReq(
	UNSIGNED8 nodeId	/**< node ID of guarding slave */
	CO_COMMA_LINE_PARA_DECL	/**< number of CAN line 0..CO_MAX_CAN_LINES-1 */
    )
{
GUARDING_T	*pGuard;	/* pointer to actual nodes */
UNSIGNED8	idx;

#  ifdef CONFIG_SLAVE
    /* if we are not the master, return */
    if ((GL_ARRAY(co_Node).flags & NMTERRFLAG_MASTER) == 0)  {
	return(CO_E_BAD_SERVICE);
    }
#  endif /*  CONFIG_SLAVE */

    /* get index at list */
    idx = getGuardSlaveIndex(nodeId CO_COMMA_LINE_PARA);
    if (idx == 0xff)  {
	return CO_E_NOT_EXIST;
    }

    pGuard = &GL_PVAR(guardSlaveList)[idx
#ifdef CONFIG_MULT_LINES
		+ GL_ARRAY(co_guardSlaveLineOffs)
#endif /* CONFIG_MULT_LINES */
		];

    /* Start/Stop guarding */
    pGuard->flags |= GUARDFLAG_NG_ACTIVE;
    pGuard->flags &= (FLAG_T)~GUARDFLAG_NG_NOTE;
    /* start timer */
    if (addTimerEvent(&pGuard->timer, pGuard->timer.timerVal,
	    CO_TIMER_TYPE_NG_MSTR | CO_TIMER_TYPE_CYCLIC CO_COMMA_LINE_PARA)
		!= 0)  {
	return(CO_E_RANGE);
    }

    /* for first timer event */
    pGuard->flags |= GUARDFLAG_NG_RECEIVED;
    pGuard->bSuspendedGuardings = 0;
    pGuard->bGuardToggle = 0x80;

    /* set node as not failed */
    nmtErrNodeFailed(nodeId, NMTERROR_STATE_STARTED CO_COMMA_LINE_PARA);

    return(CO_OK);
}


/****************************************************************************/
/**
*++ stopNodeGuardReq -  stops the Node Guarding
*-- stopNodeGuardReq -  stoppt das Node Guarding
*
*++ This function stops the Node Guarding of a master,
*++ for the specified
*++ node guarding slave.
*-- Diese Funktion stoppt das Node Guarding auf einem Master
*-- für den übergebenen Slave.
*
* \retval CO_OK
*++ success
*-- Erfolg
* \retval CO_E_NOT_EXIST
*++ node doesn't exist
*-- der gewählte Knoten existiert nicht
*
*/

RET_T stopNodeGuardReq(
	UNSIGNED8 nodeId	/**< node ID of guarding slave */
	CO_COMMA_LINE_PARA_DECL	/**< number of CAN line 0..CO_MAX_CAN_LINES-1 */
    )
{
UNSIGNED8	idx;
GUARDING_T	*pGuard;	/* pointer to actual nodes */

#  ifdef CONFIG_SLAVE
    /* if we are not the master, return */
    if ((GL_ARRAY(co_Node).flags & NMTERRFLAG_MASTER) == 0)  {
	return(CO_E_BAD_SERVICE);
    }
#  endif /*  CONFIG_SLAVE */

    /* get index at list */
    idx = getGuardSlaveIndex(nodeId CO_COMMA_LINE_PARA);
    if (idx == 0xff)  {
	return CO_E_NOT_EXIST;
    }

    pGuard = &GL_PVAR(guardSlaveList)[idx
#ifdef CONFIG_MULT_LINES
		+ GL_ARRAY(co_guardSlaveLineOffs)
#endif /* CONFIG_MULT_LINES */
		];

    /* Stop guarding */
    pGuard->flags &= (FLAG_T)~GUARDFLAG_NG_ACTIVE;
    removeTimerEvent(&pGuard->timer CO_COMMA_LINE_PARA);

    /* set node as not failed */
    nmtErrNodeFailed(nodeId, NMTERROR_STATE_UNCONFIG CO_COMMA_LINE_PARA);

    return(CO_OK);
}


/****************************************************************************/
/**
*++ setGuardTimePara - set the Guarding Para for the master
*-- setGuardTimePara - setzt die Node Guarding Parameter beim Master
*
*++ This function sets the Guarding Time and the lifetime
*++ for the nodeguarding master.
*++ For monitoring a node by nodeguarding
*++ it has to be added by the function
*-- Diese Funktion setzt das Überwachungsintervall und die Lebenszeitfaktor
*-- für das Node Guarding für den übergebenen Slave-Knoten.
*-- Dazu muss jeder Knoten, der mit Nodeguarding überwacht werden soll,
*-- vorher mit der Funktion
* addGuardingSlave()
*++ before.
*-- in die Nodeguarding Liste eingetragen werden.
*++ The monitoring can be started by the function
*-- Der Start des Node-Guarding kann dann mit der Funktion
* startNodeGuardReq()
*-- erfolgen.
*
* \retval CO_OK
*++ success
*-- Erfolg
* \retval CO_E_NOT_EXIST
*++ node doesn't exist
*-- der gewählte Knoten existiert nicht
*
*/

RET_T setGuardTimePara(
	UNSIGNED8  nodeId,	/**< node ID of guarding slave */
	UNSIGNED16 guardTime,	/**< guarding time in ms */
	UNSIGNED8 lifeTimeFac	/**< life time factor */
	CO_COMMA_LINE_PARA_DECL	/**< number of CAN line 0..CO_MAX_CAN_LINES-1 */
    )
{
RET_T retVal = CO_OK;
UNSIGNED8	idx;
GUARDING_T	*pGuard;	/* pointer to actual nodes */

    /* get index at list */
    idx = getGuardSlaveIndex(nodeId CO_COMMA_LINE_PARA);
    if (idx == 0xff)  {
	retVal = CO_E_NOT_EXIST;
    } else {

        pGuard = &GL_PVAR(guardSlaveList)[idx
#ifdef CONFIG_MULT_LINES
		+ GL_ARRAY(co_guardSlaveLineOffs)
#endif /* CONFIG_MULT_LINES */
		];

        pGuard->timer.timerVal = (UNSIGNED32)guardTime * 10;
        pGuard->bLifeTimeFactor = lifeTimeFac;
    }
    return(retVal);
}


/****************************************************************************/
/**
*
*++ addGuardingSlave - add Node Guarding Slave to monitoring list
*-- addGuardingSlave - Node Guarding Slave in Überwachung aufnehmen
*
*++ This function adds a slave to the nodeguarding monitoring list.
*-- Diese Funktion fügt einen Slave zur Überwachung mit Nodeguarding
*-- zur Überwachungsliste hinzu.
*++ The values for the guard time and the lifetime can also be set
*++ later by using the function
*-- Die Werte für GuardingTime und LifeTime
*-- können auch zu einem späteren Zeitpunkt mit der Funktion
* setGuardTimePara()
*-- modifiziert werden.
*++ The nodeguarding must be started by the function
*-- Das Nodeguarding für jeden Knoten muss explizit mit der Funktion
* startNodeGuardReq()
*++ and can be finished by the function
*-- gestartetet bzw. mit der Funktion
* stopNodeGuardReq()
*-- beendet werden.
*
* \retval CO_OK
*++ success
*-- Erfolg
* \retval CO_E_NOT_EXIST
*++ node doesn't exist
*-- der gewählte Knoten existiert nicht
*
*/
RET_T addGuardingSlave(
	UNSIGNED8	nodeId,		/* Knoten nummer */
	UNSIGNED16	guardTime,	/* Guarding Time */
	UNSIGNED8	lifeTime	/* life time */
	CO_COMMA_LINE_PARA_DECL	/* number of CAN line 0..CO_MAX_CAN_LINES-1 */
    )
{
UNSIGNED8	idx;
GUARDING_T	*pSlave;
RET_T		retVal;

    /* already added ? */
    idx = getGuardSlaveIndex(nodeId CO_COMMA_LINE_PARA);
    if (idx != 0xff)  {
	return CO_E_ALREADY_EXIST;
    }

    /* search next free entry */
    idx = 0;
    while (idx < CO_GUARD_SLAVE_LINE_CNTS)  {
	/* node = 0 */
	if (GL_PVAR(guardSlaveList)[idx
#ifdef CONFIG_MULT_LINES
			    + GL_ARRAY(co_guardSlaveLineOffs)
#endif /* CONFIG_MULT_LINES */
			    ].nodeId == 0)  {
	    break;
	}
	idx++;
    }

    if (idx == CO_GUARD_SLAVE_LINE_CNTS)  {
	return(CO_E_MEM);
    }

    pSlave = &GL_PVAR(guardSlaveList)[idx
#ifdef CONFIG_MULT_LINES
		+ GL_ARRAY(co_guardSlaveLineOffs)
#endif /* CONFIG_MULT_LINES */
		];

    /* save node number */
    pSlave->nodeId = nodeId;
    pSlave->flags = GUARDFLAG_NG_POSSIBLE;
    pSlave->eState = PRE_OPERATIONAL;
    pSlave->pGuard_COB = DEFINE_COB(CO_COB_GUARD_MASTER, 1 CO_COMMA_LINE_PARA);

    if (pSlave->pGuard_COB == NULL)  {
	return(CO_E_MEM);
    }

    retVal = SET_COB_ID(pSlave->pGuard_COB,
	(UNSIGNED32)(CO_COBID_NMTERR + nodeId), CO_COB_GUARD_MASTER);
    if (retVal != CO_OK)  {
	return(retVal);
    }

#ifdef CONFIG_FAST_SORT
    sortNodeIdList(
# ifdef CONFIG_MULT_LINES
	&GL_PVAR(guardSlaveIdxList)[GL_ARRAY(co_guardSlaveLineOffs)],
	&GL_PVAR(guardSlaveList)[GL_ARRAY(co_guardSlaveLineOffs)].nodeId,
# else /* CONFIG_MULT_LINES */
	GL_PVAR(guardSlaveIdxList),
	&GL_PVAR(guardSlaveList)[0].nodeId,
# endif /* CONFIG_MULT_LINES */
	sizeof(GUARDING_T),
	CO_GUARD_SLAVE_LINE_CNTS);
#endif /* CONFIG_FAST_SORT */

    retVal = setGuardTimePara(nodeId, guardTime, lifeTime CO_COMMA_LINE_PARA);
    return(retVal);
}


/*******************************************************************
*
* getGuardNodeState - returns the actual node state for a guarding node
*
* \internal
*
* This function returns the actual state of an node,
* This function only returns values for active guarding nodes.
* If the node was not found, or guarding is inactive, UNKNOWN is returned
*
* \retval
*	NODE_SATTE_T	- is node exists
*	UNKNOWN		- node not found
*
*/
NODE_STATE_T getGuardNodeState(
	UNSIGNED8	nodeNr		/* Node Id */
	CO_COMMA_LINE_PARA_DECL	/* number of CAN line 0..CO_MAX_CAN_LINES-1 */
    )
{
UNSIGNED8 idx;
GUARDING_T	*pGuard;	/* pointer to actual nodes */

    /* get index at list */
    idx = getGuardSlaveIndex(nodeNr CO_COMMA_LINE_PARA);
    if (idx == 0xff)  {
	return(UNKNOWN);
    }

    pGuard = &GL_PVAR(guardSlaveList)[idx
#ifdef CONFIG_MULT_LINES
		+ GL_ARRAY(co_guardSlaveLineOffs)
#endif /* CONFIG_MULT_LINES */
		];

    /* return state only for active nodes */
    if ((pGuard->flags & GUARDFLAG_NG_ACTIVE) == 0)  {
	return(UNKNOWN);
    }

    return(pGuard->eState);
}



/*******************************************************************
*
* setGuardNodeState - set node state for a guarding node
*
* \internal
*
* This function set the given state for an node,
* If the node was not found, or guarding is inactive, UNKNOWN is returned
*
* If the paramter nodeNr is zero, all node states are set to the new state
*
* \retval
*	NODE_STATE_T	- is node exists
*	CO_E_UNKNOWN_NODE - unknown device
*
*/
RET_T setGuardNodeState(
	UNSIGNED8	nodeNr,		/* Node Id */
	NODE_STATE_T	newState	/* new node state */
	CO_COMMA_LINE_PARA_DECL	/* number of CAN line 0..CO_MAX_CAN_LINES-1 */
    )
{
UNSIGNED8 idx;
GUARDING_T	*pGuard;	/* pointer to actual nodes */

    /* set state for all nodes? */
    if (nodeNr == 0)  {
	for (idx = 0; idx < CO_GUARD_SLAVE_LINE_CNTS; idx++) {
	    GL_PVAR(guardSlaveList)[idx
#ifdef CONFIG_MULT_LINES
		    + GL_ARRAY(co_guardSlaveLineOffs)
#endif /* CONFIG_MULT_LINES */
		    ].eState = newState;
	}

    } else {
	/* get index at list */
	idx = getGuardSlaveIndex(nodeNr CO_COMMA_LINE_PARA);
	if (idx == 0xff)  {
	    return(CO_E_UNKNOWN_NODE);
	}

	pGuard = &GL_PVAR(guardSlaveList)[idx
#ifdef CONFIG_MULT_LINES
		+ GL_ARRAY(co_guardSlaveLineOffs)
#endif /* CONFIG_MULT_LINES */
		];

	pGuard->eState = newState;
    }

    return(CO_OK);
}


/*******************************************************************
*
* guardMsgReceived- node guarding message received
*
* \internal
*
* This function checks the received node guarding message
*
* RETURNS
* \retval
*	nothing
*
*/
void guardMsgReceived(
	UNSIGNED16	idx,		/* guarding index */
	UNSIGNED8	*canData	/* ptr to can data */
	CO_COMMA_LINE_PARA_DECL	/* number of CAN line 0..CO_MAX_CAN_LINES-1 */
    )
{
GUARDING_T	*pSlave;

    pSlave = &GL_PVAR(guardSlaveList)[idx];

    if ((pSlave->flags & GUARDFLAG_NG_ACTIVE) != 0)  {
	/* test of toggle bit */
	if ((canData[0] & 0x80) != pSlave->bGuardToggle) {
	    pSlave->bGuardToggle = canData[0] & 0x80;
	    pSlave->bSuspendedGuardings = 0;
	    pSlave->flags |= GUARDFLAG_NG_RECEIVED;
	}
    }

# ifdef CONFIG_NMT_STARTUP_MANAGER
    /* guarding start signaled ? */
    if ((pSlave->flags & GUARDFLAG_NG_NOTE) == 0)  {
	pSlave->flags |= GUARDFLAG_NG_NOTE;
	nmtsEventHandler(NMT_ERRCTRL_GUARD_RECEIVED, pSlave->nodeId
	    CO_COMMA_LINE_PARA);
    }
# endif /* CONFIG_NMT_STARTUP_MANAGER */

    /* test for wrong state */
    if ((canData[0] & 0x7f) != (UNSIGNED8)(pSlave->eState)) {
	/* set new NMT state */
	pSlave->eState = (NODE_STATE_T)(canData[0] & 0x7f);

#ifdef CONFIG_HEARTBEAT_CONSUMER
	/* set heartbeat state */
	(void) setHbNodeState(pSlave->nodeId, pSlave->eState CO_COMMA_LINE_PARA);
#endif /* CONFIG_HEARTBEAT_CONSUMER */

	mGuardErrorInd(pSlave->nodeId, CO_NODE_STATE CO_COMMA_LINE_PARA);

#ifdef CONFIG_NMT_STARTUP_MANAGER
	nmtsEventHandler(NMT_ERRCTRL_NODE_STATE, pSlave->nodeId CO_COMMA_LINE_PARA);
#endif /* CONFIG_NMT_STARTUP_MANAGER */
    }
}


/***************************************************************************
*
*++ NMT_M_TimerPulse - function counts timer pulses for node guarding
*-- NMT_M_TimerPulse - Funktion zählt Timerpulse für Nodeguarding
*
* \internal
*
*++ This function counts timer pulses for node guarding on a master device.
*-- Diese Funktion zählt die Timerpulse für das Nodeguarding auf Master-Geräten.
*
* \retval
*	nothing
*
*/

void NMT_M_TimerPulse(
	TIMER_EVENT_T *pTimer	/* pointer to timer event structure */
	CO_COMMA_LINE_PARA_DECL	/* number of CAN line 0..CO_MAX_CAN_LINES-1 */
    )
{
GUARDING_T	*pSlave;
    /* the timer event structure is the first entry at the node structure
     * therefore the pointer is equal to the start of the node structure
     */
    pSlave = (GUARDING_T *)pTimer;

    /* Guarding active ? */
    if ((pSlave->flags & GUARDFLAG_NG_ACTIVE) == 0)  {
	/* remove the timer event */
	/* at the incdication function its better
	 * we only remove the flag */
	/* removeTimerEvent(&pNodeInUse->timer); */
	pSlave->timer.timerType &= ~CO_TIMER_TYPE_CYCLIC;
	return;
    }

    /* Nodeguarding active */

    /* GuardTime elapsed but no Guarding Answer received */
    if ((pSlave->flags & GUARDFLAG_NG_RECEIVED) == 0) {

	pSlave->bSuspendedGuardings++;

	/* life time for this module is elapsed */
	if (pSlave->bSuspendedGuardings == pSlave->bLifeTimeFactor) {
	    /* disable guarding */
	    pSlave->flags &= (FLAG_T)~GUARDFLAG_NG_ACTIVE;

#  ifdef CONFIG_DYN_SDO_CONNECTION_MANAGER
	    lostConnection(pSlave->bNode_ID CO_COMMA_LINE_PARA);
#  endif /* CONFIG_DYN_SDO_CONNECTION_MANAGER */

	    mGuardErrorInd(pSlave->nodeId, CO_LOST_CONNECTION
			CO_COMMA_LINE_PARA);

#  ifdef CONFIG_NMT_STARTUP_MANAGER
	    nmtsEventHandler(NMT_ERRCTRL_LOST_GUARDING, pSlave->nodeId
		CO_COMMA_LINE_PARA);
#  endif /* CONFIG_NMT_STARTUP_MANAGER */

	    /* set node as failed */
	    nmtErrNodeFailed(pSlave->nodeId, NMTERROR_STATE_FAILED
		CO_COMMA_LINE_PARA);

	    /* remove the timer event */
	    /* at the incdication function its better
	     * we only remove the flag */
	    /* removeTimerEvent(&pNodeInUse->timer); */
	    pSlave->timer.timerType &= ~CO_TIMER_TYPE_CYCLIC;

	    return;

	} else {
	    /* life time isn't elapsed */
	    mGuardErrorInd(pSlave->nodeId, CO_LOST_GUARDING_MSG
			CO_COMMA_LINE_PARA);
	}
    }

    /* guarding was received */
    pSlave->flags &= (FLAG_T)~GUARDFLAG_NG_RECEIVED;
    (void)TRANSMIT_COB(pSlave->pGuard_COB, NULL);
}


/*******************************************************************
*
* getGuardSlaveIndex - searches for Guarding Slave entry
*
* \internal
*
* This function checks the Guarding Slave list
* and returns the index at the list.
* if the node isn't at the list, 0xff is returned
*
* \retval subIndex
*	subindex for the given node
* \retval 0xff
*	no entry found and no more entries free
*
*/

UNSIGNED8 getGuardSlaveIndex(
	UNSIGNED8 nodeId	/* node id */
	CO_COMMA_LINE_PARA_DECL	/* number of CAN line 0..CO_MAX_CAN_LINES-1 */
    )
{
GUARDING_T	*pGuardList;

# ifdef CONFIG_FAST_SORT
INTEGER8	found = 0;
INTEGER16	low, mid = 0, high;
UNSIGNED8	*pIdxList;
# else /* CONFIG_FAST_SORT */
UNSIGNED8	i;		/* loop variable */
# endif /* CONFIG_FAST_SORT */

    /* ignore node 0 */
    if (nodeId == 0)  {
	return(0xff);
    }

# ifdef CONFIG_MULT_LINES
    pGuardList = &GL_PVAR(guardSlaveList)[GL_ARRAY(co_guardSlaveLineOffs)];
# else /* CONFIG_MULT_LINES */
    pGuardList = &GL_PVAR(guardSlaveList)[0];
# endif /* CONFIG_MULT_LINES */

# ifdef CONFIG_FAST_SORT

    low = 0;
    high = CO_GUARD_SLAVE_LINE_CNTS - 1;
#  ifdef CONFIG_MULT_LINES
    pIdxList = &GL_PVAR(guardSlaveIdxList)[GL_ARRAY(co_guardSlaveLineOffs)];
#  else /* CONFIG_MULT_LINES */
    pIdxList = &GL_PVAR(guardSlaveIdxList)[0];
#  endif /* CONFIG_MULT_LINES */

    while (found == 0)  {
	if (high >= low) {
	    mid = (high + low) / 2;
	    if (pGuardList[pIdxList[mid]].nodeId == nodeId)  {
		found = 1;
	    } else {
		if (pGuardList[pIdxList[mid]].nodeId > nodeId) {
		    high = mid - 1;
		} else  {
		    low = mid + 1;
		}
	    }
	} else {
	    found = -1;
	}
    }
    if (found < 0)  {
	return(0xff);
    } else {
	return(pIdxList[mid]);
    }
# else /* CONFIG_FAST_SORT */

    for (i = 0; i < CO_GUARD_SLAVE_LINE_CNTS; i++)  {
	/* get the entry */
	if (pGuardList[i].nodeId == nodeId)  {
	    return(i);
	}
    }
    return(0xff);
# endif /* CONFIG_FAST_SORT */
}


/*******************************************************************
*
* initGuardVars - init all Guarding variables
*
* \internal
*
* RETURNS
* \retval nothing
*
*/

void initGuardVars(
	CO_LINE_PARA_DECL
    )
{
# ifdef CONFIG_MULT_LINES
UNSIGNED8	l;
UNSIGNED16	offs;
# endif /* CONFIG_MULT_LINES */

    /* clear global variables (some compilers doesn't clear global variables */
# ifdef CONFIG_CLEAR_CO_GLOBAL_VARS
    memset(&GL_PVAR(guardSlaveList)[0], (int)0,
	(size_t)(sizeof(GUARDING_T) * CO_GUARD_SLAVE_LINE_CNTS));

#  ifdef CONFIG_FAST_SORT
    memset(&GL_PVAR(guardSlaveIdxList)[0], (int)0,
	(size_t)(sizeof(UNSIGNED8) * CO_GUARD_SLAVE_LINE_CNTS));
#  endif /* CONFIG_FAST_SORT */
# endif /* CONFIG_FAST_SORT */


# ifdef CONFIG_MULT_LINES
    /* calculate line offsets */
    l = canLine;
    offs = 0;
    while (l > 0)  {
	l--;
	offs += co_guardSlaveLineCnts[l];
    }
    GL_ARRAY(co_guardSlaveLineOffs) = offs;
# endif /* CONFIG_MULT_LINES */

}

# endif /* CONFIG_NODE_GUARDING */
#endif /* CONFIG_MASTER */

/*______________________________________________________________________EOF_*/

