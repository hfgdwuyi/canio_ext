/*
 *++ heartbt - additional heartbeat routines for a CANopen master
 *-- heartbt - Zusätzliche Heartbeat Routinen für einen CANopen Master
 *
 * Copyright (c) 2001-2017 port GmbH Halle (Saale)
 *------------------------------------------------------------------
 */


/****************************************************************************/
/**
*  \file heartbt.c
*++ Heartbeat routines for a CANopen master
*-- Heartbeat Routinen für einen CANopen Master
*  \author port GmbH Halle (Saale)
*
*++ This module contains functions for monitoring heartbeat messages
*++ of CANopen nodes. It can be used by master and slave applications.
*-- Dieses Modul enthält die Heartbeat-Überwachungsfunktionen.
*-- Sie können von Master als auch von Slave Knoten genutzt werden.
*
*++ All monitored nodes have to be entered into the heartbeat consumer
*++ list at index 0x1016. Additional nodes can be entered with the
*++ function
*-- Alle zu überwachenden Knoten müssen in der Heartbeat-Consumer-Liste
*-- auf dem Index 0x1016 eingetragen sein. Weitere Knoten können
*-- mit der Funktion
* setHeartBeatTime()
*-- eingetragen werden.
*
*++ Heartbeat monitoring starts automatically with the first received
*++ heartbeat message of the corresponding node.
*++ If the guarding time has exceeded the indication function
*-- Die Heartbeatüberwachung startet automatisch mit dem Eintreffen
*-- des ersten Heartbeats des jeweiligen Knotens.
*-- Bei der Überschreitung der Überwachungszeit wird die Indikationfunktion
* mGuardErrInd()
*++ is called and guarding of this node is stopped.
*-- aufgerufen und die Überwachung eingestellt.
*
*++ Heartbeat monitoring can be started and stopped with the functions
*-- Die Überwachung kann auch über die Funktionen
* startHeartBeatReq()
*-- bzw.
*++ and
* stopHeartBeatReq()
*-- ein- bzw. ausgeschalten werden.
*++ respectively.
*
*/

/* header of standard C - libraries */

#include <stdio.h>
#include <string.h>

/* header of project specific types */
#include <cal_conf.h>
#include <co_odidx.h>
#include <co_cobid.h>
#include <co_nmt.h>
#include <co_def.h>
#include "heartbt.h"
#include "nmt.h"
#include "nmterr.h"
#include "access.h"
#include "drv.h"
#include "utility.h"

#ifdef CONFIG_FLYING_MASTER
# include "flyma.h"
# include "co_nmt_m.h"
#endif /* CONFIG_FLYING_MASTER */

# ifdef CONFIG_REDUNDANCY_SUPPORT
# include "reduncy.h"
# endif /* CONFIG_REDUNDANCY_SUPPORT */

#ifdef CONFIG_NMT_STARTUP_MANAGER
# include "nmtstart.h"
#endif /* CONFIG_NMT_STARTUP_MANAGER */

/* constant definitions
---------------------------------------------------------------------------*/
#ifdef CONFIG_DYN_MEM_ALLOC
# define HB_CONSUMER_CNT	co_maxHbConsCnt
#else /* CONFIG_DYN_MEM_ALLOC */
# define HB_CONSUMER_CNT	CONFIG_HEARTBEAT_CONSUMER
#endif /* CONFIG_DYN_MEM_ALLOC */

/* local defined data types
---------------------------------------------------------------------------*/

/* list of external used functions, if not in headers
---------------------------------------------------------------------------*/

/* list of global defined functions
---------------------------------------------------------------------------*/

/* list of local defined functions
---------------------------------------------------------------------------*/
#ifdef CONFIG_HEARTBEAT_CONSUMER
# ifdef CONFIG_FAST_SORT
static void sortHbConsList(CO_LINE_PARA_DECL);
# endif /* CONFIG_FAST_SORT */
#endif /* CONFIG_HEARTBEAT_CONSUMER */

/* external variables
---------------------------------------------------------------------------*/

/* global variables
---------------------------------------------------------------------------*/
#ifdef EXPERIMENTAL_HB_LIST
CO_LIB_INIT_VAR UNSIGNED16 co_hbNodeIdConsArray[127] CO_LINE_PARA_ARRAY_DEF;
#endif /* EXPERIMENTAL_HB_LIST */

#ifdef CONFIG_NO_GLOBAL_VARS
#else /* CONFIG_NO_GLOBAL_VARS */

# ifdef CONFIG_HEARTBEAT_CONSUMER
#  ifdef CONFIG_MULT_LINES
			/* heartbeat server line counters */
CO_LIB_INIT_VAR UNSIGNED8	co_hbConsLineCnts[CO_MAX_CAN_LINES] =
			    { CONFIG_HB_CONSUMER_LINECFG };
			/* heartbeat server line offsets */
CO_LIB_UNINIT_VAR UNSIGNED16	co_hbConsLineOffs CO_LINE_PARA_ARRAY_DEF;
#  endif /* CONFIG_MULT_LINES */
# endif /* CONFIG_HEARTBEAT_CONSUMER */

# ifdef CONFIG_HEARTBEAT_CONSUMER
#  ifdef CONFIG_DYN_MEM_ALLOC
CO_LIB_UNINIT_VAR HB_CONS_T	*p_hbConsList[1];
CO_LIB_UNINIT_VAR UNSIGNED16	co_maxHbConsCnt;
#   ifdef CONFIG_REDUNDANCY_SUPPORT
CO_LIB_UNINIT_VAR HB_CONS_T	*p_redcyHbConsList[1];
#   endif /* CONFIG_REDUNDANCY_SUPPORT */
#  else /* CONFIG_DYN_MEM_ALLOC */
CO_LIB_UNINIT_VAR CO_LIB_UNINIT_VAR HB_CONS_T	hbConsList[CONFIG_HEARTBEAT_CONSUMER];
#   ifdef CONFIG_REDUNDANCY_SUPPORT
CO_LIB_UNINIT_VAR HB_CONS_T	redcyHbConsList[CONFIG_HEARTBEAT_CONSUMER];
#   endif /* CONFIG_REDUNDANCY_SUPPORT */
#  endif /* CONFIG_DYN_MEM_ALLOC */
# endif /* CONFIG_HEARTBEAT_CONSUMER */
#endif /* CONFIG_NO_GLOBAL_VARS */


/* local defined variables
---------------------------------------------------------------------------*/
#ifdef CONFIG_RCS_IDENT
CO_LIB_INIT_VAR static char _rcsid[] = "$Id: heartbt.c,v 2.60 2016/09/19 17:28:33 rli Exp $";
#endif /* CONFIG_RCS_IDENT */

#ifdef CONFIG_NO_GLOBAL_VARS
#else /* CONFIG_NO_GLOBAL_VARS */

# ifdef CONFIG_HEARTBEAT_CONSUMER
#  ifdef CONFIG_FAST_SORT
#   ifdef CONFIG_DYN_MEM_ALLOC
CO_LIB_UNINIT_VAR UNSIGNED8	*p_hbIdxList[1];
#   else /* CONFIG_DYN_MEM_ALLOC */
CO_LIB_UNINIT_VAR static UNSIGNED8	hbIdxList[CONFIG_HEARTBEAT_CONSUMER];
#   endif /* CONFIG_DYN_MEM_ALLOC */
#  endif /* CONFIG_FAST_SORT */
# endif /* CONFIG_HEARTBEAT_CONSUMER */
#endif /* CONFIG_NO_GLOBAL_VARS */

#ifdef CONFIG_HEARTBEAT_CONSUMER
# ifdef CONFIG_MULT_LINES
#  define CO_HB_CONS_LINE_CNTS	GL_ARRAY(co_hbConsLineCnts)
# else /* CONFIG_MULT_LINES */
#  define CO_HB_CONS_LINE_CNTS	HB_CONSUMER_CNT
# endif /* CONFIG_MULT_LINES */
#endif /* CONFIG_HEARTBEAT_CONSUMER */


#ifdef CONFIG_HEARTBEAT_CONSUMER
/****************************************************************************/
/**
*++ \brief defineHeartbeatConsumer - init heartbeat consumer
*-- \brief defineHeartbeatConsumer - Initialisierung der Heartbeat Consumer
*
*++ This function initializes the heartbeat monitoring
*++ for all nodes, that are saved at HB-Consumer list (index 1016).
*++ Additional nodes can be added by calling
*-- Diese Funktion initialisiert die Heartbeatüberwachung
*-- für alle Knoten, die in der HB-Consumer Liste (Index 1016)
*-- eingetragen sind.
*-- Weitere Knoten können mit der Funktion
* setHeartBeatTime()
*-- für das Monitoring eingetragen werden.
*++ Monitoring starts automatically after the first heartbeat was received.
*-- Die Überwachung startet mit dem ersten eintreffenden Heartbeat
*-- des entsprechenden Knotens.
*
* \retval CO_OK
*++ success
*-- Erfolg
* \retval CO_E_NOT_EXIST
*++ node or consumer communication object doesn't exist
*-- der gewählte Knoten existiert nicht oder kein Consumer Eintrag frei
* \retval CO_E_NONEXIST_OBJECT
*++ object doesnt exist
*-- Objekt existiert nicht
* \retval CO_E_NO_WRITE_PERM
*++ object not writable
*-- Objekt nicht schreibbar
*
*/
RET_T defineHeartbeatConsumer(
	CO_LINE_PARA_DECL	/**< number of CAN line 0..CO_MAX_CAN_LINES-1 */
    )
{
LIST_ELEMENT_T *curObj = NULL; 		/* pointer to current object */
RET_T		ret = CO_OK;		/* return value */
UNSIGNED8	hbCnt;			/* HB consumer count */
UNSIGNED32	chbt;			/* HB consumer entry */
UNSIGNED32	size;			/* object size */
UNSIGNED8	i;			/* loop counter */
HB_CONS_T	*pHbCons;		/* pointer to HB list */
# ifdef CONFIG_REDUNDANCY_SUPPORT
HB_CONS_T	*pRedcyHbCons;		/* pointer to HB list */
# endif /* CONFIG_REDUNDANCY_SUPPORT */

    /* all heartbeat consumer entries from od are added automatically */
    /* get number of heartbeat consumer entries from od */
    curObj = searchObj(HEARTBEAT_CON_INDEX CO_COMMA_LINE_PARA);

    ret = getObjPtrEntry( curObj, HEARTBEAT_CON_INDEX, 0, &hbCnt, &size, CO_TRUE
		CO_COMMA_LINE_PARA);
    if (ret != CO_OK)  {
        return(ret);
    }

# ifdef CONFIG_MULT_LINES
    pHbCons = &GL_PVAR(hbConsList)[GL_ARRAY(co_hbConsLineOffs)];
# else /* CONFIG_MULT_LINES */
    pHbCons = &GL_PVAR(hbConsList)[0];
#  ifdef CONFIG_REDUNDANCY_SUPPORT
    pRedcyHbCons = &GL_PVAR(redcyHbConsList)[0];
#  endif /* CONFIG_REDUNDANCY_SUPPORT */
# endif /* CONFIG_MULT_LINES */

    /* foreach entry */
    for (i = 1; i <= hbCnt; i++)  {
        /* init structure and COB-Entry */
        pHbCons[i - 1].eState = UNKNOWN;
        pHbCons[i - 1].eStateChanged = CO_FALSE;
        pHbCons[i - 1].mflags = GUARDFLAG_HB_POSSIBLE;
        pHbCons[i - 1].nodeId = 0;
        if (pHbCons[i - 1].pGuard_COB == NULL)  {
            pHbCons[i - 1].pGuard_COB =
            DEFINE_COB(CO_COB_HB_CONS, 1 CO_COMMA_LINE_PARA);
        }

        if (pHbCons[i - 1].pGuard_COB == NULL)  {
            return(CO_E_MEM);
        }

# ifdef CONFIG_REDUNDANCY_SUPPORT
        pHbCons[i - 1].line = CAN_DEFAULT_LINE;
        pRedcyHbCons[i - 1].line = CAN_REDCY_LINE;
        pRedcyHbCons[i - 1].eState = UNKNOWN;
        pRedcyHbCons[i - 1].eStateChanged = CO_FALSE;
        pRedcyHbCons[i - 1].mflags = GUARDFLAG_HB_POSSIBLE;
        pRedcyHbCons[i - 1].nodeId = 0;
# endif /* CONFIG_REDUNDANCY_SUPPORT */

        /* get node id and heartbeat time */
        ret = getObjPtrEntry( curObj, HEARTBEAT_CON_INDEX, i, (UNSIGNED8 *)&chbt,
        	&size, CO_TRUE CO_COMMA_LINE_PARA);
        if (ret != CO_OK)  {
            return(ret);
        }

        /* set the heartbeat consumer time */
        ret = setHeartBeatConsumerTime(chbt, i, CO_FALSE CO_COMMA_LINE_PARA);
        if (ret != CO_OK)  {
            return(ret);
        }
    }

# ifdef CONFIG_FAST_SORT
    if (hbCnt != 0)  {
        sortHbConsList(CO_LINE_PARA);
    }
# endif /* CONFIG_FAST_SORT */

    return(ret);
}


/****************************************************************************/
/**
*++ \brief setHeartBeatTime -  set the heartbeat monitoring time
*-- \brief setHeartBeatTime -  setzt die Heartbeat Überwachungszeit
*
*++ This function sets the heartbeat monitoring time
*++ for a guarded slave device.
*++ This time is the basic time period which
*++ the node checks the connection to the device.
*-- Diese Funktion setzt das Zeitintervall für die Heartbeat Überwachung
*-- Diese Zeit ist die Spanne in der der Knoten den jeweiligen
*-- Master/Slave überwacht.
*-- Gleichzeitig wird auch der entsprechende Eintrag im OV aktualisiert.
*
*++ This function can also be used to enter further heartbeat nodes.
*++ If no entry is found in the heartbeat consumer list then
*++ a new entry is added to the list, if there is enough memory
*++ available.
*-- Diese Funktion kann auch zum Eintragen weiterer HB Knoten genutzt werden.
*-- Wenn kein Eintrag in der HB-Consumer Liste (0x1016) gefunden wird,
*-- wird automatisch ein neuer Eintrag hinzugefügt,
*-- wenn noch Platz in der HB Consumer-Liste vorhanden ist.
*
* \retval CO_OK
*++ success
*-- Erfolg
* \retval CO_E_NOT_EXIST
*++ node or consumer communication object doesn't exist
*-- der gewählte Knoten existiert nicht oder kein Consumer Eintrag frei
* \retval CO_E_NONEXIST_OBJECT
*++ object doesnt exist
*-- Objekt existiert nicht
* \retval CO_E_NO_WRITE_PERM
*++ object not writable
*-- Objekt nicht schreibbar
*
*/

RET_T setHeartBeatTime(
        UNSIGNED8  nodeId,	/**< node ID of heartbeat slave */
# ifdef CONFIG_REDUNDANCY_SUPPORT
        UNSIGNED32 hbTime	/**< heartbeat time in ms */
# else
        UNSIGNED16 hbTime       /**< heartbeat time in ms */
# endif /* CONFIG_REDUNDANCY_SUPPORT */
        CO_COMMA_LINE_PARA_DECL	/**< number of CAN line 0..CO_MAX_CAN_LINES-1 */
)
{
UNSIGNED32	tmpU32;		/* temp u32 val */
RET_T		retVal;		/* return value */
UNSIGNED8	idx;		/* subindex */
HB_CONS_T	*pHbCons;	/* pointer to HB list */

    /* node not saved ? */
    idx = getHeartBeatIndex(nodeId CO_COMMA_LINE_PARA);
    if (idx == 0xff)
    {
        /* no, look for free subindex */
        idx = 0;

# ifdef CONFIG_MULT_LINES
	pHbCons = &GL_PVAR(hbConsList)[GL_ARRAY(co_hbConsLineOffs)];
# else /* CONFIG_MULT_LINES */
	pHbCons = &GL_PVAR(hbConsList)[0];
# endif /* CONFIG_MULT_LINES */

	while (idx < CO_HB_CONS_LINE_CNTS)  {
	    /* time = 0 or node = 0 */
	    if ((pHbCons[idx].nodeId == 0)
	     || (pHbCons[idx].timer.timerVal == 0)) {
		/* idx = i; */
		break;
	    }
	    idx++;
	}
    }

    if (idx == CO_HB_CONS_LINE_CNTS)  {
	return(CO_E_NOT_EXIST);
    }

    /* write the value into object-dictionary */
    tmpU32 = ((UNSIGNED32)nodeId << 16) | hbTime;

    retVal = setHeartBeatConsumerTime(tmpU32, idx + 1, CO_TRUE
		CO_COMMA_LINE_PARA);
    if (retVal == CO_OK)  {
        /* save at od */
        retVal = putObj(HEARTBEAT_CON_INDEX, idx + 1, (UNSIGNED8 *)&tmpU32,
        	4, CO_TRUE CO_COMMA_LINE_PARA);
    }

    return(retVal);
}


/****************************************************************************/
/**
*
*++ \brief startHeartBeatReq -  start the heartbeat monitoring
*-- \brief startHeartBeatReq -  startet die Heartbeat Überwachung
*
*++ This function starts the heartbeat monitoring for the given node.
*++ Normally this happens automatically with reception of the
*++ first heartbeat message of the node. However it can be start with
*++ this function manually.
*-- Diese Funktion startet die Heartbeat Überwachung für den
*-- übergebenen Knoten.
*-- Dies erfolgt normalerweise automatisch beim Eintreffen
*-- der ersten Heartbeat Nachricht,
*-- kann aber auch mit dieser Funktion eingeschalten werden.
*
*++ If the monitoring time exceeded and no heartbeat message
*++ was received the indication function
*-- Wenn bis zum Ablauf der Überwachungszeit kein Heartbeat empfangen wurde,
*-- wird die Indikationfunktion
* mGuarErrInd()
*++ is called and monitoring is started again.
*-- aufgerufen und die Überwachung wieder eingestellt.
*
*
* \retval CO_OK
*++ success
*-- Erfolg
* \retval CO_E_NOT_EXIST
*++ node doesn't exist
*-- Der gewählte Knoten existiert nicht
*
*/

RET_T startHeartBeatReq(
	UNSIGNED8 nodeId    /**< node ID of heartbeat producers */
	CO_COMMA_LINE_PARA_DECL	/**< number of CAN line 0..CO_MAX_CAN_LINES-1 */
    )
{
UNSIGNED8	idx;		/* sub index */
HB_CONS_T	*pNode;		/* pointer to node struct */
# ifdef CONFIG_REDUNDANCY_SUPPORT
HB_CONS_T	*pRedcyNode;	/* pointer to node struct */
# endif /* CONFIG_REDUNDANCY_SUPPORT */

    idx = getHeartBeatIndex(nodeId CO_COMMA_LINE_PARA);
    /* returns, if not subindex available */
    if (idx == 0xff)  {
        return(CO_E_NOT_EXIST);
    }

    pNode = &GL_PVAR(hbConsList)[idx
# ifdef CONFIG_MULT_LINES
			+ GL_ARRAY(co_hbConsLineOffs)
# endif /* CONFIG_MULT_LINES */
			];
# ifdef CONFIG_REDUNDANCY_SUPPORT
    pRedcyNode = &GL_PVAR(redcyHbConsList)[idx];
# endif /* CONFIG_REDUNDANCY_SUPPORT */

    if (addTimerEvent(&pNode->timer, pNode->timer.timerVal,
		CO_TIMER_TYPE_HB_CONS CO_COMMA_LINE_PARA)
	    != 0)  {
	return(CO_E_RANGE);
    }

    pNode->mflags |= GUARDFLAG_HB_ACTIVE;
    pNode->mflags &= (FLAG_T)~GUARDFLAG_NG_RECEIVED;
    pNode->mflags &= (FLAG_T)~GUARDFLAG_HB_NOTE;

# ifdef CONFIG_REDUNDANCY_SUPPORT
    if (addTimerEvent(&pRedcyNode->timer, pRedcyNode->timer.timerVal,
		CO_TIMER_TYPE_HB_CONS CO_COMMA_LINE_PARA)
	    != 0)  {
	return(CO_E_RANGE);
    }

    pRedcyNode->mflags |= GUARDFLAG_HB_ACTIVE;
    pRedcyNode->mflags &= (FLAG_T)~GUARDFLAG_NG_RECEIVED;
    pRedcyNode->mflags &= (FLAG_T)~GUARDFLAG_HB_NOTE;
# endif /* CONFIG_REDUNDANCY_SUPPORT */

    /* set heartbeat consumer as not failed */
# ifdef CONFIG_REDUNDANCY_SUPPORT
    nmtErrNodeFailed(nodeId, NMTERROR_STATE_CONFIG, CAN_DEFAULT_LINE
	CO_COMMA_GLOBVARS_PARA);
    nmtErrNodeFailed(nodeId, NMTERROR_STATE_CONFIG, CAN_REDCY_LINE
	CO_COMMA_GLOBVARS_PARA);
# else /* CONFIG_REDUNDANCY_SUPPORT */
    nmtErrNodeFailed(nodeId, NMTERROR_STATE_CONFIG CO_COMMA_LINE_PARA);
# endif /* CONFIG_REDUNDANCY_SUPPORT */

    return(CO_OK);
}


/****************************************************************************/
/**
*
*++ \brief stopHeartBeatReq -  stop the heartbeat monitoring
*-- \brief stopHeartBeatReq -  beendet die Heartbeat Überwachung
*
*++ This function stops heartbeat monitoring
*++ for the specified heartbeat slave.
*-- Diese Funktion beendet die Heartbeat Überwachung für den
*-- übergebenen Knoten.
*++ The indication function
*-- Die Indikationfunktion
* mGuarErrInd()
*++ will not be called.
*-- wird nicht aufgerufen.
*
*++ With the reception of the next heartbeat message
*++ monitoring is activated again.
*-- Mit dem Eintreffen der nächsten Heartbeat-Nachricht
*-- wird die Überwachung wieder neu gestartet.
*
*
* \retval CO_OK
*++ success
*-- Erfolg
* \retval CO_E_NOT_EXIST
*++ node doesn't exist
*-- Der gewählte Knoten existiert nicht
*
*/

RET_T stopHeartBeatReq(
	UNSIGNED8 nodeId    /**< node ID of heartbeat producers */
	CO_COMMA_LINE_PARA_DECL	/**< number of CAN line 0..CO_MAX_CAN_LINES-1 */
    )
{
UNSIGNED8	idx;		/* sub index */
HB_CONS_T	*pNode;		/* pointer to node struct */
# ifdef CONFIG_REDUNDANCY_SUPPORT
HB_CONS_T	*pRedcyNode;	/* pointer to node struct */
# endif /* CONFIG_REDUNDANCY_SUPPORT */

    idx = getHeartBeatIndex(nodeId CO_COMMA_LINE_PARA);
    /* returns, if not subindex available */
    if (idx == 0xff)  {
        return(CO_E_NOT_EXIST);
    }

    pNode = &GL_PVAR(hbConsList)[idx
# ifdef CONFIG_MULT_LINES
			+ GL_ARRAY(co_hbConsLineOffs)
# endif /* CONFIG_MULT_LINES */
			];
# ifdef CONFIG_REDUNDANCY_SUPPORT
    pRedcyNode = &GL_PVAR(redcyHbConsList)[idx];
# endif /* CONFIG_REDUNDANCY_SUPPORT */

    pNode->mflags &= (FLAG_T)~GUARDFLAG_HB_ACTIVE;
    pNode->mflags &= (FLAG_T)~GUARDFLAG_HB_NOTE;

    removeTimerEvent(&pNode->timer CO_COMMA_LINE_PARA);

# ifdef CONFIG_REDUNDANCY_SUPPORT
    pRedcyNode->mflags &= (FLAG_T)~GUARDFLAG_HB_ACTIVE;
    pRedcyNode->mflags &= (FLAG_T)~GUARDFLAG_HB_NOTE;

    removeTimerEvent(&pRedcyNode->timer CO_COMMA_LINE_PARA);
# endif /* CONFIG_REDUNDANCY_SUPPORT */

    return(CO_OK);
}


/*******************************************************************
*
* getHbNodeState - returns the actual node state for a heartbeat node
*
* \internal
*
* This function returns the actual state of an node,
* This function only returns values for active heartbeat monitoring nodes.
* If the node was not found, or heartbeat is inactive, UNKNOWN is returned
*
* \retval
*	NODE_STATE_T	- if node exists
*	UNKNOWN		- node not found
*
*/
NODE_STATE_T getHbNodeState(
	UNSIGNED8	nodeNr		/* Node Id */
	CO_COMMA_REDCY_PARA_DECL
    )
{
UNSIGNED8 idx;
HB_CONS_T	*pHbCons;

    idx = getHeartBeatIndex(nodeNr CO_COMMA_LINE_PARA);
    if (idx == 0xff)  {
	return(UNKNOWN);
    }

    pHbCons = &GL_PVAR(hbConsList)[idx
# ifdef CONFIG_MULT_LINES
		    + GL_ARRAY(co_hbConsLineOffs)
# endif /* CONFIG_MULT_LINES */
		    ];

# ifdef CONFIG_REDUNDANCY_SUPPORT
    if (canLine != CAN_DEFAULT_LINE)  {
	pHbCons = &GL_PVAR(redcyHbConsList)[idx];
    }
# endif /* CONFIG_REDUNDANCY_SUPPORT */

    return(pHbCons->eState);
}


/*******************************************************************
*
* setHbNodeState - set the new node state for a heartbeat node
*
* \internal
*
* This function sets the actual state of an node,
* This function is only called for heartbeat monitoring nodes.
* If the node was not found, UNKNOWN is returned
*
* If the parameter nodeNr is zero, all node states are set to the new state
*
* \retval
*	CO_OK		- node exists
*	CO_E_NOT_EXIST	- node not found
*
*/
RET_T setHbNodeState(
	UNSIGNED8	nodeNr,		/* Node Id */
	NODE_STATE_T	newState	/* new node state */
	CO_COMMA_LINE_PARA_DECL	/* number of CAN line 0..CO_MAX_CAN_LINES-1 */
    )
{
UNSIGNED8 idx;

    /* set state for all nodes? */
    if (nodeNr == 0)  {
	for (idx = 0; idx < CO_HB_CONS_LINE_CNTS; idx++) {
	    GL_PVAR(hbConsList)[idx
# ifdef CONFIG_MULT_LINES
			+ GL_ARRAY(co_hbConsLineOffs)
# endif /* CONFIG_MULT_LINES */
			].eState = newState;
	    GL_PVAR(hbConsList)[idx
# ifdef CONFIG_MULT_LINES
			+ GL_ARRAY(co_hbConsLineOffs)
# endif /* CONFIG_MULT_LINES */
			].eStateChanged = CO_TRUE;
	}
    } else {
	/* node specific */
	idx = getHeartBeatIndex(nodeNr CO_COMMA_LINE_PARA);
	if (idx == 0xff)  {
	    return(CO_E_NOT_EXIST);
	}
	GL_PVAR(hbConsList)[idx
# ifdef CONFIG_MULT_LINES
		    + GL_ARRAY(co_hbConsLineOffs)
# endif /* CONFIG_MULT_LINES */
		    ].eState = newState;
	GL_PVAR(hbConsList)[idx
# ifdef CONFIG_MULT_LINES
		    + GL_ARRAY(co_hbConsLineOffs)
# endif /* CONFIG_MULT_LINES */
		    ].eStateChanged = CO_TRUE;
    }

    return(CO_OK);
}


/*******************************************************************
*
* setHbSignaling - signal the next receive heartbeat
*
* \internal
*
* Set a flag to inform the startup manager about next receive heartbeat
*
* \retval
*	CO_OK		- node exists
*	CO_E_NOT_EXIST	- node not found
*
*/
RET_T setHeartBeatSignaling(
	UNSIGNED8	nodeNr		/* Node Id */
	CO_COMMA_LINE_PARA_DECL	/* number of CAN line 0..CO_MAX_CAN_LINES-1 */
    )
{
HB_CONS_T	*pHBCons;		/* pointer to node struct */
UNSIGNED8	idx;

    idx = getHeartBeatIndex(nodeNr CO_COMMA_LINE_PARA);
    if (idx == 0xff)  {
	return(CO_E_NOT_EXIST);
    }

    pHBCons = &GL_PVAR(hbConsList)[idx
# ifdef CONFIG_MULT_LINES
			+ GL_ARRAY(co_hbConsLineOffs)
# endif /* CONFIG_MULT_LINES */
			];

    pHBCons->mflags |= GUARDFLAG_SIGNAL;

    return(CO_OK);
}


/*******************************************************************/
/*
* setHeartBeatConsumerTime - set heartbeat consumer time
*
* \internal
*
* This service change an entry from heartbeat consumer list
* and updates the internal structures
*
* \retval
* RET_T
*
*/
RET_T setHeartBeatConsumerTime(
	UNSIGNED32	hbEntry,	/* entry from od heartbeat-consumer list */
	UNSIGNED8	subIndex,	/* subindex for this entry */
	BOOL_T		sortHbLists	/* sort heartbeat lists */
	CO_COMMA_LINE_PARA_DECL	/* number of CAN line 0..CO_MAX_CAN_LINES-1 */
    )
{
UNSIGNED8	nodeId, oldNodeId;	/* node id */
HB_CONS_T	*pHbEntry;
# ifdef CONFIG_REDUNDANCY_SUPPORT
HB_CONS_T	*pRedcyHbEntry;
# endif /* CONFIG_REDUNDANCY_SUPPORT */
UNSIGNED16	hbTime;			/* heartbeat time */
RET_T		retVal;

    hbTime = (UNSIGNED16)(hbEntry & HB_CONS_TIME_MASK);
    nodeId = (UNSIGNED8)((hbEntry & HB_CONS_NODEID_MASK) >> 16);
    oldNodeId = nodeId;

    /* check for own node-id */
    if (nodeId == GL_ARRAY(coNodeId))  {
	return(CO_E_PARA_INCOMP);
    }

    /* check node id */
    if (nodeId > 127)  {
	return(CO_E_PARA_INCOMP);
    }
    /* check reserved area */
    if ((hbEntry & HB_CONS_RESERVED_MASK) != 0)  {
	return(CO_E_PARA_INCOMP);
    }
    /* check subIndex */
    if (subIndex > CO_HB_CONS_LINE_CNTS)
    {
        return(CO_E_PARA_INCOMP);
    }

    /* get the correspondenting network structure */
    pHbEntry = &GL_PVAR(hbConsList)[(subIndex - 1)
# ifdef CONFIG_MULT_LINES
    	+ GL_ARRAY(co_hbConsLineOffs)
# endif /* CONFIG_MULT_LINES */
 	];

# ifdef CONFIG_REDUNDANCY_SUPPORT
    pRedcyHbEntry = &GL_PVAR(redcyHbConsList)[subIndex - 1];
# endif /* CONFIG_REDUNDANCY_SUPPORT */

    /* is not the same node saved at this position ? */
    if (nodeId != pHbEntry->nodeId)  {
	/* check, if the node saved at other index ? */
	if (getHeartBeatIndex(nodeId CO_COMMA_LINE_PARA) != 0xff)  {
	    /* yes, error 0x06040043 */
	    return(CO_E_PARA_INCOMP);
	}

	oldNodeId = pHbEntry->nodeId;

	/* set old heartbeat consumer as unconfigured */
# ifdef CONFIG_REDUNDANCY_SUPPORT
	nmtErrNodeFailed(oldNodeId, NMTERROR_STATE_UNCONFIG, CAN_DEFAULT_LINE
		CO_COMMA_GLOBVARS_PARA);
	nmtErrNodeFailed(oldNodeId, NMTERROR_STATE_UNCONFIG, CAN_REDCY_LINE
		CO_COMMA_GLOBVARS_PARA);
# else /* CONFIG_REDUNDANCY_SUPPORT */
	nmtErrNodeFailed(oldNodeId, NMTERROR_STATE_UNCONFIG CO_COMMA_LINE_PARA);
# endif /* CONFIG_REDUNDANCY_SUPPORT */

	/* save new node-id and update COB-Id */
	pHbEntry->nodeId = nodeId;

#ifdef CONFIG_REDUNDANCY_SUPPORT
	pRedcyHbEntry->nodeId = nodeId;
#endif /* CONFIG_REDUNDANCY_SUPPORT */

	/* now we have to change the node-id */
	retVal = SET_COB_ID(pHbEntry->pGuard_COB,
		CO_COBID_NMTERR + (UNSIGNED32)nodeId, CO_COB_HB_CONS);
	if (retVal != CO_OK)  {
	    return(retVal);
	}
    }

#ifdef CONFIG_REDUNDANCY_SUPPORT
# ifdef CONFIG_MARITIME_SUPPORT
    /* redundancy entry ? */
    if ((hbEntry & HB_CONS_REDCY_MASK) == HB_CONS_REDCY_NODE)  {
#define SET_NODE_BIT(var,node) {	\
	var[(node - 1) >> 3] |= (UNSIGNED8)(1 << ((node - 1) % 8));	\
    }
#define RESET_NODE_BIT(var,node) {	\
	var[(node - 1) >> 3] &= ~((UNSIGNED8)(1 << ((node - 1) % 8)));	\
    }

	pHbEntry->redcyNode = CO_TRUE;
	SET_NODE_BIT(GL_ARRAY(nmtErrRedundancy), nodeId);
	RESET_NODE_BIT(GL_ARRAY(nmtErr3HBok), nodeId);
    } else {
	pHbEntry->redcyNode = CO_FALSE;
	RESET_NODE_BIT(GL_ARRAY(nmtErrRedundancy), nodeId);

	/* connected on which line */
	if ((hbEntry & HB_CONS_REDCY_MASK) != 0)  {
	    pHbEntry->redcyLine = CO_TRUE;
	} else {
	    pHbEntry->redcyLine = CO_FALSE;
	}
    }
# endif /* CONFIG_MARITIME_SUPPORT */
#endif /* CONFIG_REDUNDANCY_SUPPORT */

    /* stop the timer */
    removeTimerEvent(&pHbEntry->timer CO_COMMA_LINE_PARA);
# ifdef CONFIG_REDUNDANCY_SUPPORT
    removeTimerEvent(&pRedcyHbEntry->timer CO_COMMA_LINE_PARA);
# endif /* CONFIG_REDUNDANCY_SUPPORT */

    /* if heartbeat time = 0 or node = 0 set it to invalid */
    if ((nodeId == 0) || (hbTime == 0))  {
	/* actualize the heartbeat monitoring time */
	pHbEntry->timer.timerVal = (UNSIGNED32)hbTime * 10;
# ifdef CONFIG_REDUNDANCY_SUPPORT
	pRedcyHbEntry->timer.timerVal = (UNSIGNED32)hbTime * 10;
# endif /* CONFIG_REDUNDANCY_SUPPORT */

	/* remove active flag */
	pHbEntry->mflags &= (FLAG_T)~GUARDFLAG_HB_ACTIVE;
	pHbEntry->mflags &= (FLAG_T)~GUARDFLAG_HB_NOTE;
# ifdef CONFIG_REDUNDANCY_SUPPORT
	pRedcyHbEntry->mflags &= (FLAG_T)~GUARDFLAG_HB_ACTIVE;
	pRedcyHbEntry->mflags &= (FLAG_T)~GUARDFLAG_HB_NOTE;
# endif /* CONFIG_REDUNDANCY_SUPPORT */

	/* set heartbeat consumer as unconfigured */
# ifdef CONFIG_REDUNDANCY_SUPPORT
	nmtErrNodeFailed(oldNodeId, NMTERROR_STATE_UNCONFIG, CAN_DEFAULT_LINE
		CO_COMMA_GLOBVARS_PARA);
	nmtErrNodeFailed(oldNodeId, NMTERROR_STATE_UNCONFIG, CAN_REDCY_LINE
		CO_COMMA_GLOBVARS_PARA);
# else /* CONFIG_REDUNDANCY_SUPPORT */
	nmtErrNodeFailed(oldNodeId, NMTERROR_STATE_UNCONFIG CO_COMMA_LINE_PARA);
# endif /* CONFIG_REDUNDANCY_SUPPORT */

    } else {

	/* set the new heartbeat monitoring time */
	pHbEntry->timer.timerVal = (UNSIGNED32)hbTime * 10;
# ifdef CONFIG_REDUNDANCY_SUPPORT
	pRedcyHbEntry->timer.timerVal = (UNSIGNED32)hbTime * 10;
# endif /* CONFIG_REDUNDANCY_SUPPORT */

	/* set heartbeat consumer as failed until first heartbeat was received */
# ifdef CONFIG_REDUNDANCY_SUPPORT
	nmtErrNodeFailed(nodeId, NMTERROR_STATE_CONFIG, CAN_DEFAULT_LINE
		CO_COMMA_GLOBVARS_PARA);
	nmtErrNodeFailed(nodeId, NMTERROR_STATE_CONFIG, CAN_REDCY_LINE
		CO_COMMA_GLOBVARS_PARA);

#  ifdef CONFIG_MARITIME_SUPPORT
	/* non redundant node ? */
	if (pHbEntry->redcyNode == CO_FALSE)  {
	    /* connected to redundant line ? */
	    if (pHbEntry->redcyLine == CO_TRUE)  {
		nmtErrNodeFailed(nodeId, NMTERROR_STATE_UNCONFIG,
			CAN_DEFAULT_LINE
			CO_COMMA_GLOBVARS_PARA);
	    } else {
		nmtErrNodeFailed(nodeId, NMTERROR_STATE_UNCONFIG,
			CAN_REDCY_LINE
			CO_COMMA_GLOBVARS_PARA);
	    }
	}
#  endif /* CONFIG_MARITIME_SUPPORT */
	/* check, if all HB consumers are ok */
	/* problem: which state has a node,
	 * before we have received his first HB ?
	 * Should we assume, it is not available
	 * and change to the redundant line
	 * or should we start the HB monitoring and change then to the
	 * redundant line ? */
	/* redcyCheckNodeAvailable(CAN_DEFAULT_LINE); */
	/* if (GL_VAR(co_redcyActiveLine) == CAN_DEFAULT_LINE)  { */
	    /* yes, then start heartbeat monitoring for this node */
	    /* startHeartBeatReq(nodeId CO_COMMA_LINE_PARA); */
	/* } */
# else /* CONFIG_REDUNDANCY_SUPPORT */
	nmtErrNodeFailed(nodeId, NMTERROR_STATE_CONFIG CO_COMMA_LINE_PARA);
# endif /* CONFIG_REDUNDANCY_SUPPORT */
    }

# ifdef CONFIG_FAST_SORT
    if (sortHbLists == CO_TRUE)  {
	sortHbConsList(CO_LINE_PARA);
    }
# else /* CONFIG_FAST_SORT */
    CO_INTERNAL_NOT_USED(sortHbLists);
# endif /* CONFIG_FAST_SORT */

    return(CO_OK);
}


/*******************************************************************/
/*
* setHbBootupState - set heartbeat consumer bootup states
*
* \internal
*
* This service set the states for a heartbeat consumer after a bootup was received
*
* \retval
* RET_T
*
*/
void setHbBootupState(
	UNSIGNED8	nodeId		/* node id */
	CO_COMMA_REDCY_PARA_DECL
    )
{
UNSIGNED8	idx;
HB_CONS_T	*pNode;		/* pointer to node struct */

    idx = getHeartBeatIndex(nodeId CO_COMMA_LINE_PARA);

# ifdef CONFIG_REDUNDANCY_SUPPORT
    /* erase hbCounter */
    if (idx != 0xff)  {
	if (canLine == CAN_DEFAULT_LINE) {
	    GL_PVAR(hbConsList)[idx].hbCnt = 0;/* number of received heartbeats*/
	}
    }
# endif /* CONFIG_REDUNDANCY_SUPPORT */

    /* set heartbeat state */
    (void) setHbNodeState(nodeId, PRE_OPERATIONAL CO_COMMA_LINE_PARA);

    if (idx != 0xff)  {
	pNode = &GL_PVAR(hbConsList)[idx
# ifdef CONFIG_MULT_LINES
		    + GL_ARRAY(co_hbConsLineOffs)
# endif /* CONFIG_MULT_LINES */
		    ];
	pNode->mflags &= (FLAG_T)~GUARDFLAG_HB_ACTIVE;
	pNode->mflags &= (FLAG_T)~GUARDFLAG_NG_RECEIVED;
	pNode->mflags &= (FLAG_T)~GUARDFLAG_HB_NOTE;
	pNode->eStateChanged = CO_FALSE;
    }
}


# ifdef CONFIG_FAST_SORT
/*******************************************************************
*
* sortHbConsList - sort heartbeat list
*
* \internal
*
* This function sorts the heartbeat node-id list
*
* \retval
*	nothing
*
*/
static void sortHbConsList(
	CO_LINE_PARA_DECL
    )
{
#  ifdef EXPERIMENTAL_HB_LIST
UNSIGNED8 u8_i = 0u, nodeId = 0u;
    for (u8_i = 0u; u8_i < 127u; u8_i++ ) {
       GL_ARRAY(co_hbNodeIdConsArray[u8_i]) = 0xFF;
    }
    for ( u8_i = 0u; u8_i < CO_HB_CONS_LINE_CNTS; u8_i++ ) {
        nodeId = GL_PVAR(hbConsList)[
#   ifdef CONFIG_MULT_LINES
        GL_ARRAY(co_hbConsLineOffs) +
#   endif /* CONFIG_MULT_LINES */
        u8_i].nodeId;
        if ( (nodeId < 128u) && (nodeId > 0u) ) {
            GL_ARRAY(co_hbNodeIdConsArray[nodeId - 1u]) = u8_i;
        }
    }
#  else /* EXPERIMENTAL_HB_LIST */
    sortNodeIdList(
#   ifdef CONFIG_MULT_LINES
	&GL_PVAR(hbIdxList)[GL_ARRAY(co_hbConsLineOffs)],
	&GL_PVAR(hbConsList)[GL_ARRAY(co_hbConsLineOffs)].nodeId,
#   else /* CONFIG_MULT_LINES */
	&GL_PVAR(hbIdxList)[0],
	&GL_PVAR(hbConsList)[0].nodeId,
#   endif /* CONFIG_MULT_LINES */
	sizeof(HB_CONS_T),
	CO_HB_CONS_LINE_CNTS);
#  endif /* EXPERIMENTAL_HB_LIST */

}
# endif /* CONFIG_FAST_SORT */


/*******************************************************************
*
* void hbMsgReceived - heartbeat message was received
*
* \internal
*
* This function is called after a heartbeat message was received.
* Depending on the recieved state and the internal saved state
* the user indication is called.
*
* \retval
*	nothing
*
*/
void hbMsgReceived(
    UNSIGNED8   idx,            /* index at heartbeat consumer list */
    UNSIGNED8   state           /* heartbeat state */
    CO_COMMA_REDCY_PARA_DECL
    )
{
HB_CONS_T       *pHBCons;               /* pointer to node struct */
BOOL_T          wrongState = CO_FALSE;

    if (idx >= CO_HB_CONS_LINE_CNTS)
        return;

    pHBCons = &GL_PVAR(hbConsList)[idx
# ifdef CONFIG_MULT_LINES
			+ GL_ARRAY(co_hbConsLineOffs)
# endif /* CONFIG_MULT_LINES */
			];

# ifdef CONFIG_REDUNDANCY_SUPPORT
#  ifdef CONFIG_MARITIME_SUPPORT
    /* heartbeat for this node on this line configured */
    if (pHBCons->redcyNode == CO_FALSE)  {
	/* no redundancy node, check connected line */
	if (((pHBCons->redcyLine == CO_TRUE) && (canLine == CAN_DEFAULT_LINE))
	 || ((pHBCons->redcyLine == CO_FALSE) && (canLine == CAN_REDCY_LINE))) {
	    return;
	}
    }
#  else /* CONFIG_MARITIME_SUPPORT */
#  endif /* CONFIG_MARITIME_SUPPORT */

    if (canLine != CAN_DEFAULT_LINE)  {
	pHBCons = &GL_PVAR(redcyHbConsList)[idx];
    }
# endif /* CONFIG_REDUNDANCY_SUPPORT */

    /* if heartbeat time is disabled, return */
    if (pHBCons->timer.timerVal == 0)  {
	return;
    }

    /* user indication for the first occurence of the heartbeat */
    if ((pHBCons->mflags & GUARDFLAG_HB_NOTE) == 0) {
	/* test for wrong state */
	if ((state & 0x7f) != (UNSIGNED8)(pHBCons->eState)) {
	    if (pHBCons->eState == UNKNOWN)  {
		pHBCons->eState = (NODE_STATE_T)(state & 0x7f);
	    } else {
		/* was the state change before than ignore it */
		if (pHBCons->eStateChanged != CO_TRUE)  {
		    pHBCons->eState = (NODE_STATE_T)(state & 0x7f);
		    wrongState = CO_TRUE;
		}
	    }
	}

	pHBCons->mflags |= GUARDFLAG_HB_ACTIVE;
	pHBCons->mflags |= GUARDFLAG_HB_NOTE;

	/* set heartbeat consumer as not failed */
	nmtErrNodeFailed(pHBCons->nodeId, NMTERROR_STATE_STARTED
		CO_COMMA_REDCY_PARA);

	mGuardErrorInd(pHBCons->nodeId, CO_HB_STARTED
		CO_COMMA_REDCY_PARA);

# ifdef CONFIG_NMT_STARTUP_MANAGER
	nmtsEventHandler(NMT_ERRCTRL_HB_STARTED, pHBCons->nodeId
		CO_COMMA_REDCY_PARA);
	pHBCons->mflags &= (FLAG_T)~GUARDFLAG_SIGNAL;
# endif /* CONFIG_NMT_STARTUP_MANAGER */

    } else {

	/* test for wrong state */
	if ((state & 0x7f) != (UNSIGNED8)(pHBCons->eState)) {
	    /* was the state change before than ignore it */
	    if (pHBCons->eStateChanged != CO_TRUE)  {
		wrongState = CO_TRUE;
		pHBCons->eState = (NODE_STATE_T)(state & 0x7f);
	    }
	}
    }

    /* reset statechanged mode */
    pHBCons->eStateChanged = CO_FALSE;

    if (wrongState == CO_TRUE)  {
	mGuardErrorInd(pHBCons->nodeId, CO_NODE_STATE CO_COMMA_REDCY_PARA);

# ifdef CONFIG_NMT_STARTUP_MANAGER
	nmtsEventHandler(NMT_ERRCTRL_NODE_STATE, pHBCons->nodeId
		CO_COMMA_REDCY_PARA);
	pHBCons->mflags &= (FLAG_T)~GUARDFLAG_SIGNAL;
# endif /* CONFIG_NMT_STARTUP_MANAGER */
    }

# ifdef CO_CONFIG_REPORT_ANY_HB
    coUserHbReceived(pHBCons->nodeId, pHBCons->eState CO_COMMA_REDCY_PARA);
# endif /* CO_CONFIG_REPORT_ANY_HB */

# ifdef CONFIG_NMT_STARTUP_MANAGER
    if ((pHBCons->mflags & GUARDFLAG_SIGNAL) != 0) {
	nmtsEventHandler(NMT_ERRCTRL_RECEIVED, pHBCons->nodeId
		CO_COMMA_REDCY_PARA);
	pHBCons->mflags &= (FLAG_T)~GUARDFLAG_SIGNAL;
    }
# endif /* CONFIG_NMT_STARTUP_MANAGER */

    pHBCons->mflags |= GUARDFLAG_NG_RECEIVED;

    /* start the timer (again) */
    (void) addTimerEvent(&pHBCons->timer, pHBCons->timer.timerVal,
		CO_TIMER_TYPE_HB_CONS CO_COMMA_LINE_PARA);

# ifdef CONFIG_REDUNDANCY_SUPPORT
    /* check for 3 heartbeats on default line for
     * startup or
     * defaultline is not active */
    if ((canLine == CAN_DEFAULT_LINE)
     && (((GL_VAR(co_redcyFlags) & CO_REDCY_FLAG_DETECT_CANLINE) != 0)
       || (GL_VAR(co_redcyActiveLine) != CAN_DEFAULT_LINE)))  {
	if (pHBCons->hbCnt < 4) {
	    /* incr. number of received heartbeats*/
	    pHBCons->hbCnt++;
	}
	/* 3rd heartbeat received ? */
	if (pHBCons->hbCnt == 3)  {
	    nmtErrNodeFailed(pHBCons->nodeId, NMTERROR_STATE_3HB_OK, canLine
		CO_COMMA_GLOBVARS_PARA);
	    /* check, if all redcy nodes are available */
	    redcyCheckNodeAvailable(CO_REDCY_PARA);
	}
    }
# endif /* CONFIG_REDUNDANCY_SUPPORT */
}


/*******************************************************************
*
* NMT_HB_Cons_TimerPulse - heartbeat timer is elapsed
*
* \internal
*
* This function is called if a heartbeat timer is elapsed.
* The heartbeat monitoring is stopped and the user indication is called.
*
* \retval
*	nothing
*
*/
void NMT_HB_Cons_TimerPulse(
	TIMER_EVENT_T *pTimer	/* pointer to timer event structure */
	CO_COMMA_LINE_PARA_DECL	/* number of CAN line 0..CO_MAX_CAN_LINES-1 */
    )
{
HB_CONS_T *pNodeInUse;     /* pointer to current node structure */
# ifdef CONFIG_REDUNDANCY_SUPPORT
UNSIGNED8 canLine;
# endif /* CONFIG_REDUNDANCY_SUPPORT */

    /* the timer event structure is the first entry at the node structure
     * therefore the pointer is equal to the start of the node structure
     */
    pNodeInUse = (HB_CONS_T *)pTimer;

    if ((pNodeInUse->mflags & GUARDFLAG_HB_ACTIVE) == 0)  {
	return;
    }

# ifdef CONFIG_REDUNDANCY_SUPPORT
    canLine = pNodeInUse->line;
# endif /* CONFIG_REDUNDANCY_SUPPORT */

    /* set actual state to unknown */
    pNodeInUse->eState = UNKNOWN;

    /* stop heartbeat monitoring */
    pNodeInUse->mflags &= (FLAG_T)~GUARDFLAG_HB_ACTIVE;
    pNodeInUse->mflags &= (FLAG_T)~GUARDFLAG_HB_NOTE;

    /* set heartbeat consumer as failed */
    nmtErrNodeFailed(pNodeInUse->nodeId, NMTERROR_STATE_FAILED
	CO_COMMA_REDCY_PARA);

# ifdef CONFIG_DYN_SDO_CONNECTION_MANAGER
    lostConnection(pNodeInUse->nodeId CO_COMMA_LINE_PARA);
# endif /* CONFIG_DYN_SDO_CONNECTION_MANAGER */

# ifdef CONFIG_NO_ERROR_BEHAVIOR
# else /* CONFIG_NO_ERROR_BEHAVIOR */
    execCommErrorBehavior(CO_REDCY_PARA);
# endif /* CONFIG_NO_ERROR_BEHAVIOR */

    /* user indication */
    mGuardErrorInd(pNodeInUse->nodeId, CO_LOST_HEARTBEAT CO_COMMA_REDCY_PARA);

# ifdef CONFIG_NMT_STARTUP_MANAGER
    nmtsEventHandler(NMT_ERRCTRL_HB_LOST, pNodeInUse->nodeId
	CO_COMMA_REDCY_PARA);
    pNodeInUse->mflags &= (FLAG_T)~GUARDFLAG_SIGNAL;
# endif /* CONFIG_NMT_STARTUP_MANAGER */

# ifdef CONFIG_FLYING_MASTER
    /* if this the actual master node.*/
    if (GL_ARRAY(co_activeMaster) == pNodeInUse->nodeId)  {

#  ifdef CONFIG_REDUNDANCY_SUPPORT
	/* heartbeat failed on both lines ? */
	UNSIGNED8	idx;

	/* search node struct */
	idx = getHeartBeatIndex(pNodeInUse->nodeId CO_COMMA_LINE_PARA);
	/* returns, if not subindex available */
	if (idx != 0xff)  {

	    /* if (((pTmpNode->mflags | pTmpNode->pRedcy->mflags) */
	    if (((GL_PVAR(hbConsList)[idx].mflags | GL_PVAR(redcyHbConsList)[idx].mflags)
		    & GUARDFLAG_HB_ACTIVE) == 0) {
#  endif /* CONFIG_REDUNDANCY_SUPPORT */

		/* start new master negoitation ? */
		if (flyingMasterInd(0, FLYMA_MASTER_HB_FAILED
				CO_COMMA_LINE_PARA)
			== CO_TRUE)  {
		    /* start new master nego. - set temporary master flag */
		    (void) setNmtMasterMode(CO_LINE_PARA);
		    /* forceCommResetReq(CO_LINE_PARA); */
#  ifdef CONFIG_REDUNDANCY_SUPPORT
	    	    resetCommReq(0x80, GL_VAR(co_redcyActiveLine)
				CO_COMMA_LINE_PARA);
		    resetCommReq(GL_ARRAY(coNodeId), GL_VAR(co_redcyActiveLine)
				CO_COMMA_LINE_PARA);
#  else /* CONFIG_REDUNDANCY_SUPPORT */
		    (void) resetCommReq(0x80 CO_COMMA_LINE_PARA);
		    (void) resetCommReq(GL_ARRAY(coNodeId) CO_COMMA_LINE_PARA);
#  endif /* CONFIG_REDUNDANCY_SUPPORT */
		}
#  ifdef CONFIG_REDUNDANCY_SUPPORT
	    }
	}
#  endif /* CONFIG_REDUNDANCY_SUPPORT */
    }
# endif /* CONFIG_FLYING_MASTER */

# ifdef CONFIG_REDUNDANCY_SUPPORT
    pNodeInUse->hbCnt = 0;
    /* switch to redcy line only for events on default line
     * and timer is off */
    if ((pNodeInUse->line == CAN_DEFAULT_LINE)
     && ((GL_VAR(co_redcyFlags) & CO_REDCY_FLAG_DETECT_CANLINE) == 0)
#  ifdef CONFIG_MARITIME_SUPPORT
	/* if maritime is enabled, only for maritime nodes */
     && (pNodeInUse->redcyNode == CO_TRUE)
#  endif /* CONFIG_MARITIME_SUPPORT */
	)  {

	if (redundancyInd(REDUNCY_HB_ERROR CO_COMMA_LINE_PARA) == CO_TRUE)  {
#  ifdef CONFIG_REDUNDANCY_DEF_SUPPORT
	    redcySetActiveLine(CAN_REDCY_LINE CO_COMMA_LINE_PARA);
#  else /* CONFIG_REDUNDANCY_DEF_SUPPORT */
	    reduncySwitchLine(CAN_REDCY_LINE CO_COMMA_LINE_PARA);
#  endif /* CONFIG_REDUNDANCY_DEF_SUPPORT */
	}
    }
# endif /* CONFIG_REDUNDANCY_SUPPORT */

}


/****************************************************************************/
/**
*
*++ \brief checkAllHBConsumer - checks for all heartbeat consumer exist
*-- \brief checkAllHBConsumer - testet Vorhandensein aller Heartbeat Consumer
*
*++ This function checks if all heartbeat consumer are alive
*++ and the heartbeat were received.
*++ All nodes from the heartbeat-consumer list are tested.
*-- Diese Funktion testet, ob alle HB Consumer am Leben sind
*-- und die Heartbeats empfangen wurden.
*++ If the parameter
*-- Der Parameter
* allCfgNodes
*++ is CO_TRUE then all configured nodes are tested.
*++ Otherwise only the active nodes
*++ (they have sent already a BOOTUP or a heartbeat)
*++ are used.
*-- gibt an,
*-- ob alle konfigurierten Knoten getestet werden
*-- oder nur die Knoten, die sich bisher mit einem Bootup
*-- oder einem Heartbeat gemeldet haben.
*
* \retval CO_TRUE
*++ all HB node are available
*-- alle HB Knoten sind verfügbar
* \retval CO_FALSE
*++ one or more HB consumer nodes are not available
*-- ein oder mehrere HB consumer sind nicht verfügbar
*
*/

BOOL_T checkAllHBConsumer(
	BOOL_T	   allCfgNodes	/**< all configured nodes */
	CO_COMMA_LINE_PARA_DECL	/**< number of CAN line 0..CO_MAX_CAN_LINES-1 */
    )
{
UNSIGNED8	failed, start;

    failed = 0;
    for (start = 0; start < NMTERR_MAX_INDEX; start ++)  {
	if (allCfgNodes == CO_TRUE)  {
# ifdef CONFIG_REDUNDANCY_SUPPORT
	    if ((GL_ARRAY(nmtErrFailed)[start][GL_VAR(co_redcyActiveLine)] != 0)
	     || ((GL_ARRAY(nmtErrConfig)[start][GL_VAR(co_redcyActiveLine)]
	       ^ GL_ARRAY(nmtErrStarted)[start][GL_VAR(co_redcyActiveLine)]) != 0))  {
# else /* CONFIG_REDUNDANCY_SUPPORT */
	    if ((GL_ARRAY(nmtErrFailed[start]) != 0)
	     || ((GL_ARRAY(nmtErrConfig[start])
	       ^ GL_ARRAY(nmtErrStarted[start])) != 0))  {
# endif /* CONFIG_REDUNDANCY_SUPPORT */
	    /* printf("start: %d failed: %d\n", start, nmtErrFailed[start]); */
		failed++;
	    }
	} else {
# ifdef CONFIG_REDUNDANCY_SUPPORT
	    if (GL_ARRAY(nmtErrFailed[start])[GL_VAR(co_redcyActiveLine)] != 0)  {
# else /* CONFIG_REDUNDANCY_SUPPORT */
	    if (GL_ARRAY(nmtErrFailed[start]) != 0)  {
# endif /* CONFIG_REDUNDANCY_SUPPORT */
		/* printf("start: %d failed: %d\n", start, nmtErrFailed[start]); */
		failed++;
	    }
	}
    }

    if (failed != 0)  {
	return(CO_FALSE);
    } else {
	return(CO_TRUE);
    }
}


# ifdef CONFIG_REDUNDANCY_SUPPORT
/****************************************************************************/
/**
*++ \brief checkredcyHBConsumer - checks for all heartbeat consumer exist
*-- \brief checkredcyHBConsumer - testet Vorhandensein aller Heartbeat Consumer
*
*++ This function checks if all redundancy heartbeat consumer are alive
*++ and the heartbeat were 3 times received.
*-- Diese Funktion testet, ob alle Redundancy HB Consumer am Leben sind
*-- und jeweils 3 Heartbeats empfangen wurden.
*++ The parameter
*
* \retval CO_TRUE
*++ all redcy HB node are available
*-- alle redcy HB Knoten sind verfügbar
* \retval CO_FALSE
*++ one or more redcy HB consumer nodes are not available
*-- ein oder mehrere redcy HB consumer sind nicht verfügbar
*
*/

BOOL_T checkRedcyHBConsumer(
	CO_LINE_PARA_DECL
    )
{
UNSIGNED8	failed, start;

    failed = 0;
    for (start = 0; start < NMTERR_MAX_INDEX; start ++)  {
	if (((GL_VAR(nmtErrConfig)[start][0] ^ GL_ARRAY(nmtErr3HBok)[start])
#  ifdef CONFIG_MARITIME_SUPPORT
	    & GL_ARRAY(nmtErrRedundancy)[start]
#  endif /* CONFIG_MARITIME_SUPPORT */
		) != 0)  {
	    failed++;
	}
    }

    if (failed != 0)  {
	return(CO_FALSE);
    } else {
	return(CO_TRUE);
    }
}
# endif /* CONFIG_REDUNDANCY_SUPPORT */


/*******************************************************************
*
* getHeartBeatIndex - searches for heartbeat entry
*
* \internal
*
* This function checks the heartbeat-consumer list
* and returns the index at the list.
* if the consumer isn't at the list, 0xff is returned
*
* RETURNS
* \retval subIndex
*	subindex for the given node
* \retval 0xff
*	no entry found and no more entries free
*
*/

UNSIGNED8 getHeartBeatIndex(
	UNSIGNED8 nodeId	/**< node id */
	CO_COMMA_LINE_PARA_DECL	/**< number of CAN line 0..CO_MAX_CAN_LINES-1 */
    )
{
#ifdef EXPERIMENTAL_HB_LIST
    /* ignore node 0 */
    if (nodeId == 0)  {
	return(0xff);
    }

    return GL_ARRAY(co_hbNodeIdConsArray[nodeId - 1]);
#else /* EXPERIMENTAL_HB_LIST */

HB_CONS_T	*pHbCons;

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
    pHbCons = &GL_PVAR(hbConsList)[GL_ARRAY(co_hbConsLineOffs)];
# else /* CONFIG_MULT_LINES */
    pHbCons = &GL_PVAR(hbConsList)[0];
# endif /* CONFIG_MULT_LINES */

# ifdef CONFIG_FAST_SORT

    low = 0;
    high = CO_HB_CONS_LINE_CNTS - 1;
#  ifdef CONFIG_MULT_LINES
    pIdxList = &GL_PVAR(hbIdxList)[GL_ARRAY(co_hbConsLineOffs)];
#  else /* CONFIG_MULT_LINES */
    pIdxList = &GL_PVAR(hbIdxList)[0];
#  endif /* CONFIG_MULT_LINES */

    while (found == 0)  {
	if (high >= low) {
	    mid = (high + low) / 2;
	    if (pHbCons[pIdxList[mid]].nodeId == nodeId)  {
		found = 1;
	    } else {
		if (pHbCons[pIdxList[mid]].nodeId > nodeId) {
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

    for (i = 0; i < CO_HB_CONS_LINE_CNTS; i++)  {
	/* get the entry */
	if (pHbCons[i].nodeId == nodeId)  {
	    return(i);
	}
    }
    return(0xff);
# endif /* CONFIG_FAST_SORT */
#endif /* EXPERIMENTAL_HB_LIST */

}


/*******************************************************************
*
* initHeartbeatVars - init all heartbeat variables
*
* \internal
*
* RETURNS
* \retval nthing
*
*/

void initHeartBeatVars(
	CO_LINE_PARA_DECL
    )
{
# ifdef CONFIG_MULT_LINES
UNSIGNED8	l;
UNSIGNED16	offs;
# endif /* CONFIG_MULT_LINES */

    /* clear global variables (some compilers doesn't clear global variables */
# ifdef CONFIG_CLEAR_CO_GLOBAL_VARS
    memset(&GL_PVAR(hbConsList)[0], (int)0,
	(size_t)(sizeof(HB_CONS_T) * HB_CONSUMER_CNT));

#  ifdef CONFIG_REDUNDANCY_SUPPORT
    memset(&GL_PVAR(redcyHbConsList)[0], (int)0,
	(size_t)(sizeof(HB_CONS_T) * HB_CONSUMER_CNT));
#  endif /* CONFIG_REDUNDANCY_SUPPORT */
# endif /* CONFIG_CLEAR_CO_GLOBAL_VARS */

# ifdef CONFIG_FAST_SORT
    memset(&GL_PVAR(hbIdxList)[0], (int)0, (size_t)sizeof(UNSIGNED8) * HB_CONSUMER_CNT);
# endif /* CONFIG_FAST_SORT */

# ifdef CONFIG_MULT_LINES
    /* calculate heartbeat line offsets */
    l = canLine;
    offs = 0;
    while (l > 0)  {
	l--;
	offs += co_hbConsLineCnts[l];
    }
    GL_ARRAY(co_hbConsLineOffs) = offs;

# endif /* CONFIG_MULT_LINES */
}

#endif /* CONFIG_HEARTBEAT_CONSUMER */

/*______________________________________________________________________EOF_*/


