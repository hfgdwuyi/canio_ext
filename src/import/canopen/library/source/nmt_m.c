/*
 *++ nmt_m - Network Management for Minimum Boot Up Master (Management Control)
 *-- nmt_m - Network Management für Minimum Boot Up Master (Management Control)
 *
 * Copyright (c) 2001-2017 port GmbH Halle (Saale)
 *------------------------------------------------------------------
 */


/****************************************************************************/
/**
*  \file nmt_m.c
*++ Network Management for Minimum Boot Up Master (Management Control)
*-- Network Management für Minimum Boot Up Master (Management Control)
*  \author port GmbH Halle (Saale)
*
*++ This module contains the functions for the Network Management Control
*++ for a master device.
*++ It contains only the functions for CANopen Minimum Boot Up
*++ master devices.
*-- Dieses Modul beinhaltet Funktionen des Netzwerk Management Protokolls
*-- für einen Master.
*-- Es ist beschränkt auf Funktionen, die für das CANopen Minimum Boot Up
*-- Master benötigt werden.
*
*++ All defined functions are only valid on a master device.
*-- Alle definierten Funktionen sind nur für Masterapplikationen gültig.
*/


/* header of standard C - libraries */

#include <string.h>
#include <stdio.h>

/* header of project specific types */

#include <cal_conf.h>
#include <co_odidx.h>
#include <co_cobid.h>
#include <co_guard.h>
#include <co_def.h>
#include "nmt_m.h"
#include "access.h"
#include "nmterr.h"
#include "heartbt.h"
#include "drv.h"
#include "utility.h"

#ifdef CONFIG_FLYING_MASTER
# include "flyma.h"
#endif /* CONFIG_FLYING_MASTER */

#ifdef CONFIG_REDUNDANCY_SUPPORT
# include "reduncy.h"
#endif /* CONFIG_REDUNDANCY_SUPPORT */

#ifdef CONFIG_NMT_STARTUP_MANAGER
# include "nmtstart.h"
#endif /* CONFIG_NMT_STARTUP_MANAGER */

/* constant definitions
---------------------------------------------------------------------------*/
#ifdef CONFIG_DYN_MEM_ALLOC
# define NMT_SLAVE_CNT		co_maxNmtSlaves
#else /* CONFIG_DYN_MEM_ALLOC */
# define NMT_SLAVE_CNT		CONFIG_NMT_SLAVE_CNT
#endif /* CONFIG_DYN_MEM_ALLOC */


/* local defined data types
---------------------------------------------------------------------------*/

/* list of external used functions, if not in headers
---------------------------------------------------------------------------*/

/* list of global defined functions
---------------------------------------------------------------------------*/

/* list of local defined functions
---------------------------------------------------------------------------*/
#ifdef CONFIG_MASTER
static RET_T NMT_Node_req(UNSIGNED8 remoteNodeId, NODE_STATE_T newState
		CO_COMMA_REDCY_PARA_DECL);
#endif /* CONFIG_MASTER */

/* external variables
---------------------------------------------------------------------------*/

/* global variables
---------------------------------------------------------------------------*/

/* local defined variables
---------------------------------------------------------------------------*/
#ifdef CONFIG_RCS_IDENT
CO_LIB_INIT_VAR static char _rcsid[] = "$Id: nmt_m.c,v 2.50 2016/09/26 11:16:08 rli Exp $";
#endif /* CONFIG_RCS_IDENT */

#ifdef CONFIG_NO_GLOBAL_VARS
#else /* CONFIG_NO_GLOBAL_VARS */

# if defined(CONFIG_MASTER)
#  ifdef CONFIG_NMT_SLAVE_CNT
#   ifdef CONFIG_DYN_MEM_ALLOC
CO_LIB_UNINIT_VAR REMOTE_NODE_T	*p_nmtSlaveList[1];
CO_LIB_UNINIT_VAR UNSIGNED16	co_maxNmtSlaves;
#    ifdef CONFIG_FAST_SORT
CO_LIB_UNINIT_VAR UNSIGNED8	*p_nmtSlaveIdxList[1];
#    endif /* CONFIG_FAST_SORT */
#   else /* CONFIG_DYN_MEM_ALLOC */
CO_LIB_UNINIT_VAR REMOTE_NODE_T	nmtSlaveList[CONFIG_NMT_SLAVE_CNT];
#    ifdef CONFIG_FAST_SORT
CO_LIB_UNINIT_VAR static UNSIGNED8	nmtSlaveIdxList[CONFIG_NMT_SLAVE_CNT];
#    endif /* CONFIG_FAST_SORT */
#   endif /* CONFIG_DYN_MEM_ALLOC */

#   ifdef CONFIG_MULT_LINES
		/* nmt slave line counters */
#    ifdef CONFIG_DYN_MEM_ALLOC
CO_LIB_UNINIT_VAR UNSIGNED8	co_nmtSlaveLineCnts[CO_MAX_CAN_LINES];
#    else /* CONFIG_DYN_MEM_ALLOC */
CO_LIB_CONST_VAR UNSIGNED8	co_nmtSlaveLineCnts[CO_MAX_CAN_LINES] =
			    { CONFIG_NMT_SLAVE_LINECFG };
#    endif /* CONFIG_DYN_MEM_ALLOC */
		/* nmt slave line offsets */
CO_LIB_UNINIT_VAR UNSIGNED16	co_nmtSlaveLineOffs CO_LINE_PARA_ARRAY_DEF;
#   endif /* CONFIG_MULT_LINES */
#  endif /* CONFIG_NMT_SLAVE_CNT */
# endif /* defined(CONFIG_MASTER) */
#endif /* CONFIG_NO_GLOBAL_VARS */


#if defined(CONFIG_MASTER)
# ifdef CONFIG_NMT_SLAVE_CNT
#  ifdef CONFIG_MULT_LINES
#define		CO_NMT_SLAVE_LINE_CNTS	GL_ARRAY(co_nmtSlaveLineCnts)
#  else /* CONFIG_MULT_LINES */
#define		CO_NMT_SLAVE_LINE_CNTS	NMT_SLAVE_CNT
#  endif /* CONFIG_MULT_LINES */
# endif /* CONFIG_NMT_SLAVE_CNT */
#endif /* defined(CONFIG_MASTER) */


#if defined(CONFIG_MASTER)

/****************************************************************************/
/**
*++ \brief createNetworkReq - request the local service Create Network
*-- \brief createNetworkReq - fordert den lokalen Dienst Create Network an
*
*++ The NMT master needs internal structures for the management
*++ of the remote nodes.
*++ These structures contain monitoring times for guarding and heatbeat
*++ and the current node state.
*-- Der NMT-Master benötigt für das selektive Umschalten der Remoteknoten
*-- interne Verwaltungs-Strukturen,
*-- in denen der aktuelle Knotenstatus hinterlegt ist.
*++ The initialization must be done by calling this function.
*-- Die Initialisierung für das Anlegen dieser Strukturen
*-- muss einmalig beim Start mit dieser Funktion erfolgen.
*++ Each node can be added by using the function
*-- Jeder Knoten kann dann mit der Funktion
* addRemoteNodeReq()
*-- eingetragen werden.
*
*++ The monitoring functions for node guarding and heartbeat
*++ can be used independent from the NMT nodes
*++ initialised by addRemoteNodeReq().
*-- Die Überwachungsfunktionen für Nodeguarding oder Heartbeat
*-- können unabhängig von den eingetragenen NMT Knoten
*-- genutzt werden.
*
* \retval CO_OK
*++ success
*-- Erfolg
* \retval CO_E_MEM
*++ not enough memory
*-- Nicht genug dyn. Speicher vorhanden
* \retval CO_E_ALREADY_EXIST
*++ network was already defined
*-- Netzwerk wurde bereits definiert, oder eigener Knoten ist nicht angelegt
*
*/

RET_T createNetworkReq(
	CO_LINE_PARA_DECL	/**< number of CAN line 0..CO_MAX_CAN_LINES-1 */
    )
{
# if defined(CONFIG_MULT_LINES) || defined(CONFIG_NO_GLOBAL_VARS)
    /* prevent compiler warnings */
    CO_LINE_PARA = CO_LINE_PARA;
# endif /* defined(CONFIG_MULT_LINES) || defined(CONFIG_NO_GLOBVARS) */

# ifdef CONFIG_SLAVE
    /* if we are not the master, return */
    if ((GL_ARRAY(co_Node).flags & NMTERRFLAG_MASTER) == 0)  {
	return(CO_E_BAD_SERVICE);
    }
# endif /*  CONFIG_SLAVE */

    return(CO_OK);
}


/****************************************************************************/
/**
*++ \brief deleteNetworkReq - request the local service Delete Network
*-- \brief deleteNetworkReq - fordert den lokalen Dienst Delete Network an
*
*++ The NMT-Master deletes the network object.
*++ Before this service can be used all nodes must be deleted.
*-- Der NMT-Master beseitigt das Netzwerk Objekt.
*-- Vor Aufruf dieses Dienstes müssen alle Knoten aus dem Netzwerk
*-- entfernt sein.
*
* \return
*++ nothing
*-- nichts
*
*/

void deleteNetworkReq(
	CO_LINE_PARA_DECL	/**< number of CAN line 0..CO_MAX_CAN_LINES-1 */
    )
{
# ifdef CONFIG_NMT_SLAVE_CNT
UNSIGNED8	idx = 0;
# endif /* CONFIG_NMT_SLAVE_CNT */


# if defined(CONFIG_MULT_LINES) || defined(CONFIG_NO_GLOBAL_VARS)
    /* prevent compiler warnings */
    CO_LINE_PARA = CO_LINE_PARA;
# endif /* defined(CONFIG_MULT_LINES) || defined(CONFIG_NO_GLOBAL_VARS) */

# ifdef CONFIG_NMT_SLAVE_CNT
    while (idx < CO_NMT_SLAVE_LINE_CNTS)  {
	/* set node = 0 */
	GL_PVAR(nmtSlaveList)[idx
#  ifdef CONFIG_MULT_LINES
	    + GL_ARRAY(co_nmtSlaveLineOffs)
#  endif /* CONFIG_MULT_LINES */
	    ].nodeId = 0;
	idx++;
    }
# endif /* CONFIG_NMT_SLAVE_CNT */
}


/****************************************************************************/
/**
*++ \brief addRemoteNodeReq - request the service Add Remote Node.
*-- \brief addRemoteNodeReq - fordert den Dienst Add Remote Node an.
*
*++ This function registers the given node at the NMT-Master
*++ with the given Error Control Protocol.
*-- Mit dieser Funktion wird ein Knoten für die Verwaltung
*-- durch den NMT-Master und für die Knotenüberwachung eingetragen.
*++ As current node state
*-- Als aktueller Knotenstatus wird immer
* PRE-OPRATIONAL
*++ is used.
*-- eingetragen.
*++ Additionally the error control mechanism for this node
*++ can be added by this function.
*-- Gleichzeitig kann mit dieser Funktion
*-- die Knotenüberwachung initialisiert werden.
*++ The parameter
*-- Die Parameter
* useHeartBeat
*-- or
*++ bzw.
* useGuarding
*++ starts automatically the intialization of the given monitoring mode.
*-- erlauben das automatische Initialisieren
*-- des entsprechenden Knotenüberwachungsmechanismus.
*
*++ It is not necessary to add the nodes here
*++ for the usage of error control mechanism (heartbeat or nodeguarding).
*++ This can be used independently of the NMT services.
*++ This function is only necessary for nodes,
*++ to switch the NMT state machine selectively from this node.
*-- Für die Nutzung der Knotenüberwachung müssen die Knoten
*-- nicht mit dieser Funktion in der Netzwerkstruktur eingetragen werden.
*-- Das kann auch unabhüngig hiervon erfolgen.
*-- Das Eintragen der Knoten ist nur notwendig,
*-- wenn ein selektives Umschalten der NMT-State Maschinen
*-- auf einzelnen Knoten erfolgen soll.
*
*
* \retval CO_OK
*++ success
*-- Erfolg
* \retval CO_E_NO_NETWORK
*++ no network object defined
*-- kein Netzwerkobjekt vorhanden
* \retval CO_E_MEM
*++ not enough memory
*-- nicht genug dyn. Speicher vorhanden
* \retval CO_E_RANGE
*++ not allowed module number 0
*-- Modulnummer 0 wurde angegeben
* \retval CO_E_ALREADY_EXIST
*++ Remote Node Object with this number was already defined
*-- Remote Node Objekt mit dieser Nummer existiert bereits
* \retval CO_E_NONEXIST_SUBINDEX
*-- Heartbeat Eintrag nicht möglich
*++ heartbeat not possible; subindex does not exist
* \retval CO_E_TRANS_TYPE
*++ bad transmission type
*-- falscher transmission type
*/

RET_T addRemoteNodeReq(
     UNSIGNED8  bNodeId,	/**< Node ID 1..127 (CANopen) */
# ifdef CONFIG_REDUNDANCY_SUPPORT
     UNSIGNED32 wGuardTime,     /**< guarding/heartbeat consumer time,
                                     format as in heartbeatconsumer */
# else /* CONFIG_REDUNDANCY_SUPPORT */
     UNSIGNED16 wGuardTime,     /**< guarding/heartbeat consumer time in ms */
# endif /* CONFIG_REDUNDANCY_SUPPORT */
     UNSIGNED8  bLifeTimeFactor,/**< life time factor */
     BOOL_T	useHeartBeat,	/**< use Heartbeat for this node */
     BOOL_T	useGuarding	/**< use NodeGuarding for this node */
     CO_COMMA_LINE_PARA_DECL	/**< number of CAN line 0..CO_MAX_CAN_LINES-1 */
     )
{
# ifdef CONFIG_NMT_SLAVE_CNT
UNSIGNED8	idx;
# endif /* CONFIG_NMT_SLAVE_CNT */
RET_T		retVal;

# ifdef CONFIG_SLAVE
    /* if we are not the master, return */
    if ((GL_ARRAY(co_Node).flags & NMTERRFLAG_MASTER) == 0)  {
	return(CO_E_BAD_SERVICE);
    }
# endif /*  CONFIG_SLAVE */

    /* if the own node, return */
    if (bNodeId == GL_ARRAY(coNodeId))  {
	return(CO_OK);
    }

    /* node id 0 isn't allowed normally */
    if (bNodeId == 0)  {
	return(CO_E_RANGE);
    }

# ifdef CONFIG_NMT_SLAVE_CNT
# else /* CONFIG_NMT_SLAVE_CNT */
    /* no slaves defined */
    return(CO_E_MEM);
# endif /* CONFIG_NMT_SLAVE_CNT */

    /* if node already exist, abort function */
    if (getNmtSlaveIndex(bNodeId CO_COMMA_LINE_PARA) != 0xff)  {
	/* node found, return  */
	return(CO_E_ALREADY_EXIST);
    }

# ifdef CONFIG_NMT_SLAVE_CNT
    /* look for free index */
    idx = 0;
    while (idx < CO_NMT_SLAVE_LINE_CNTS)  {
	/* node = 0 */
	if (GL_PVAR(nmtSlaveList)[idx
# ifdef CONFIG_MULT_LINES
		+ GL_ARRAY(co_nmtSlaveLineOffs)
# endif /* CONFIG_MULT_LINES */
		].nodeId == 0)  {
	    break;
	}
	idx++;
    }

    /* list full ? */
    if (idx == CO_NMT_SLAVE_LINE_CNTS)  {
	return(CO_E_MEM);
    }
# endif /* CONFIG_NMT_SLAVE_CNT */

# ifdef CONFIG_NMT_SLAVE_CNT
    /* save node id */
    GL_PVAR(nmtSlaveList)[idx
#  ifdef CONFIG_MULT_LINES
		+ GL_ARRAY(co_nmtSlaveLineOffs)
#  endif /* CONFIG_MULT_LINES */
		].nodeId = bNodeId;
# endif /* CONFIG_NMT_SLAVE_CNT */

# ifdef CONFIG_NMT_SLAVE_CNT
#  ifdef CONFIG_FAST_SORT
    /* sort node-id list */
    sortNodeIdList(
#   ifdef CONFIG_MULT_LINES
	&GL_PVAR(nmtSlaveIdxList)[GL_ARRAY(co_nmtSlaveLineOffs)],
	&GL_PVAR(nmtSlaveList)[GL_ARRAY(co_nmtSlaveLineOffs)].nodeId,
#   else /* CONFIG_MULT_LINES */
	GL_PVAR(nmtSlaveIdxList),
	&GL_PVAR(nmtSlaveList)[0].nodeId,
#   endif /* CONFIG_MULT_LINES */
	sizeof(REMOTE_NODE_T), CO_NMT_SLAVE_LINE_CNTS);
#  endif /* CONFIG_FAST_SORT */
# endif /* CONFIG_NMT_SLAVE_CNT */

    /* request heartbeat monitoring ? */
# ifdef CONFIG_HEARTBEAT_CONSUMER
    if (useHeartBeat == CO_TRUE)  {
	retVal = setHeartBeatTime(bNodeId, wGuardTime CO_COMMA_LINE_PARA);
	if (retVal != CO_OK)  {
	    return(retVal);
	}
    }
# else /* CONFIG_HEARTBEAT_CONSUMER */
    /* to avoid compiler warnings */
    CO_INTERNAL_NOT_USED(useHeartBeat);
# endif /* CONFIG_HEARTBEAT_CONSUMER */

# ifdef CONFIG_NODE_GUARDING
    if (useGuarding == CO_TRUE)  {
	retVal = addGuardingSlave(bNodeId, wGuardTime, bLifeTimeFactor
		CO_COMMA_LINE_PARA);
	if (retVal != CO_OK)  {
	    return(retVal);
	}
    }
# else /* CONFIG_NODE_GUARDING */
    /* to avoid compiler warnings */
    CO_INTERNAL_NOT_USED(useGuarding);
    CO_INTERNAL_NOT_USED(bLifeTimeFactor);
# endif /* CONFIG_NODE_GUARDING */

    return(CO_OK);
}


/****************************************************************************/
/**
*++ \brief removeRemoteNodeReq - request the service Remove Remote Node.
*-- \brief removeRemoteNodeReq - fordert den Dienst Remove Remote Node an.
*
*++ The NMT Master removes the Remote Node Object with the specified
*++ number (Node Id) from the list of the defined remote node objects.
*-- Der NMT-Master entfernt das Remote Node Object
*-- mit der angegebenen Node-Id
*-- aus der Liste der Remote Node Objects.
*
* \retval CO_OK
*++ success
*-- Erfolg
* \retval CO_E_NOT_EXIST
*++ Remote Node Object with this number does not exist
*-- Remote Node Object mit dieser Nummer bzw. diesem Namen existiert nicht
* \retval CO_E_STATE
*-- Remote Node Object nicht im Zustand PRE_OPERAIONAL
*++ Remote Node object is not PRE_OPERATIONAL
*
*/

RET_T removeRemoteNodeReq(
	UNSIGNED8 nodeId	/**< Node ID 0..127 */
	CO_COMMA_LINE_PARA_DECL	/**< number of CAN line 0..CO_MAX_CAN_LINES-1 */
     )
{
UNSIGNED8	idx = 0xff;

    idx = getNmtSlaveIndex(nodeId CO_COMMA_LINE_PARA);
    if (idx == 0xff)  {
	return(CO_E_NOT_EXIST);
    }

# ifdef CONFIG_NMT_SLAVE_CNT
    GL_PVAR(nmtSlaveList)[idx
#  ifdef CONFIG_MULT_LINES
		+ GL_ARRAY(co_nmtSlaveLineOffs)
#  endif /* CONFIG_MULT_LINES */
		].nodeId = 0;
# endif /* CONFIG_NMT_SLAVE_CNT */

# ifdef CONFIG_NMT_SLAVE_CNT
#  ifdef CONFIG_FAST_SORT
    /* sort node-id list */
    sortNodeIdList(
#   ifdef CONFIG_MULT_LINES
        &GL_PVAR(nmtSlaveIdxList)[GL_ARRAY(co_nmtSlaveLineOffs)],
        &GL_PVAR(nmtSlaveList)[GL_ARRAY(co_nmtSlaveLineOffs)].nodeId,
#   else /* CONFIG_MULT_LINES */
        GL_PVAR(nmtSlaveIdxList),
        &GL_PVAR(nmtSlaveList)[0].nodeId,
#   endif /* CONFIG_MULT_LINES */
        sizeof(REMOTE_NODE_T), CO_NMT_SLAVE_LINE_CNTS);
#  endif /* CONFIG_FAST_SORT */
# endif /* CONFIG_NMT_SLAVE_CNT */

    return(CO_OK);
}


/****************************************************************************/
/**
*++ \brief getRemoteNodePtr - get pointer to remote node structure data
*-- \brief getRemoteNodePtr - liefert einen Pointer zur Remote Node Struktur
*
*++ This function returns a pointer to the REMOTE_NODE structure of
*++ the adressed remote node.
*++ If the node isn't found the function returns NULL.
*
*-- Diese Funktion liefert einen Pointer auf die REMOTE_NODE Struktur
*-- des adressierten Remote Knoten.
*-- Falls der Knoten nicht in der Netzwerkstruktur gefunden wird,
*-- liefert die Funktion NULL zurück.
*
* \retval REMOTE_NODE_T *
*++ success
*-- Erfolg
* \retval NULL
*++ the node with the choosen ID doesn't exist
*-- Der gewählte Knoten existiert nicht
*/
REMOTE_NODE_T *getRemoteNodePtr(
	UNSIGNED8  nodeId	/**< remote node id */
	CO_COMMA_REDCY_PARA_DECL
    )
{
REMOTE_NODE_T *pRN = NULL;
UNSIGNED8 idx;

	/* if node in network list ? */
	idx = getNmtSlaveIndex(nodeId CO_COMMA_LINE_PARA);
	if (idx == 0xff) {
	    if ((nodeId & 0x80) == 0) {
		return(NULL);
	    }
	} else {

#ifdef CONFIG_NMT_SLAVE_CNT
	    /* set new node state for this node */
	    pRN = &GL_PVAR(nmtSlaveList)[idx
#  ifdef CONFIG_MULT_LINES
			+ GL_ARRAY(co_nmtSlaveLineOffs)
#  endif /* CONFIG_MULT_LINES */
			];
#endif /* CONFIG_NMT_SLAVE_CNT */
	}

    return pRN;
}


/*******************************************************************
*
* getNmtSlaveNodeState - returns the actual node state for a NMT slave node
*
* \internal
*
* This function returns the actual state of an node,
* If the node was not found, UNKNOWN is returned
*
* \retval
*	NODE_STATE_T	- is node exists
*	UNKNOWN		- node not found
*
*/
NODE_STATE_T getNmtSlaveNodeState(
	UNSIGNED8	nodeNr		/* Node Id */
	CO_COMMA_LINE_PARA_DECL	/* number of CAN line 0..CO_MAX_CAN_LINES-1 */
    )
{
UNSIGNED8 idx;

    idx = getNmtSlaveIndex(nodeNr CO_COMMA_LINE_PARA);
    if (idx == 0xff)  {
	return(UNKNOWN);
    }

# ifdef CONFIG_NMT_SLAVE_CNT
    return(GL_PVAR(nmtSlaveList)[idx
#  ifdef CONFIG_MULT_LINES
			+ GL_ARRAY(co_nmtSlaveLineOffs)
#  endif /* CONFIG_MULT_LINES */
			].eState);
# else /* CONFIG_NMT_SLAVE_CNT */
    return(UNKNOWN);
# endif /* CONFIG_NMT_SLAVE_CNT */
}


/*******************************************************************
*
* getNmtSlaveIndex - searches for NMT Slave entry
*
* \internal
*
* This function checks the NMT Slave list
* and returns the index at the list.
* if the node isn't at the list, 0xff is returned
*
* RETURNS
* \retval subIndex
*	subindex for the given node
* \retval 0xff
*	no entry found and no more entries free
*
*/

UNSIGNED8 getNmtSlaveIndex(
	UNSIGNED8 nodeId	/* node id */
	CO_COMMA_LINE_PARA_DECL	/* number of CAN line 0..CO_MAX_CAN_LINES-1 */
    )
{
# ifdef CONFIG_NMT_SLAVE_CNT
REMOTE_NODE_T	*pSlaveList;

#  ifdef CONFIG_FAST_SORT
INTEGER8 	found = 0;
INTEGER16	low, mid = 0, high;
UNSIGNED8	*pIdxList;
#  else /* CONFIG_FAST_SORT */
UNSIGNED8	i;		/* loop variable */
#  endif /* CONFIG_FAST_SORT */


#  ifdef CONFIG_MULT_LINES
    pSlaveList = &GL_PVAR(nmtSlaveList)[GL_ARRAY(co_nmtSlaveLineOffs)];
#  else /* CONFIG_MULT_LINES */
    pSlaveList = &GL_PVAR(nmtSlaveList)[0];
#  endif /* CONFIG_MULT_LINES */

    /* ignore node 0 */
    if (nodeId == 0)  {
	return(0xff);
    }

#  ifdef CONFIG_FAST_SORT

    high = CO_NMT_SLAVE_LINE_CNTS - 1;
    low = 0;
#   ifdef CONFIG_MULT_LINES
    pIdxList = &GL_PVAR(nmtSlaveIdxList)[GL_ARRAY(co_nmtSlaveLineOffs)];
#   else /* CONFIG_MULT_LINES */
    pIdxList = &GL_PVAR(nmtSlaveIdxList[0]);
#   endif /* CONFIG_MULT_LINES */

    while (found == 0)  {
	if (high >= low) {
	    mid = (high + low) / 2;
	    if (pSlaveList[pIdxList[mid]].nodeId == nodeId)  {
		found = 1;
	    } else {
		if (pSlaveList[pIdxList[mid]].nodeId > nodeId) {
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
#  else /* CONFIG_FAST_SORT */

    for (i = 0; i < CO_NMT_SLAVE_LINE_CNTS; i++)  {
	/* get the entry */
	if (pSlaveList[i].nodeId == nodeId)  {
	    return(i);
	}
    }
    return(0xff);
#  endif /* CONFIG_FAST_SORT */

# else /* CONFIG_NMT_SLAVE_CNT */

    /* avoid compiler warnings */
    nodeId = nodeId;
#  if defined(CONFIG_MULT_LINES) || defined(CONFIG_NO_GLOBAL_VARS)
    CO_LINE_PARA = CO_LINE_PARA;
#  endif /* defined(CONFIG_MULT_LINES) || defined(CONFIG_NO_GLOBAL_VARS) */

    return(0xff);
# endif /* CONFIG_NMT_SLAVE_CNT */
}

#endif /* defined(CONFIG_MASTER) */


#if defined(CONFIG_MASTER)

/****************************************************************************/
/*
*++ \brief NMT_Node_req - request one NMT service
*-- \brief NMT_Node_req - fordert einen NMT Dienst an.
*
* \internal
*
*++ The NMT-Slave referenced by the Node-ID
*++ 1..127, 0 for all slaves or 128 for all slaves without the own node
*++ will be forced to the requested state
*++ If the first bit in the Node-Id is set
*++ e.g. ((remoteNodeId & 0x80) != 0)
*++ the NMT message is put onto the bus even if no slave with this Node-Id is
*++ registered/known.
*-- Der NMT-Master versetzt den durch die Node-ID
*-- 1..127, 0 für alle oder 128 für alle ausser dem eigenen Node
*-- angegebenen NMT-Slave
*-- und das dazugehörige Remote Node Object
*-- in den geforderten Zustand.
*-- Wenn das erste bit der NodeId gesetzt wurde
*-- d.h. ((remoteNodeId & 0x80) != 0)
*-- wird die NMT Nachricht auf den Bus gesendet, auch wenn kein Slave mit dieser NodeId
*-- registriert wurde/bekannt ist.
*
* \retval CO_OK
*++ success
*-- Erfolg
* \retval CO_E_NOT_EXIST
*++ Remote Node Object with specified number doesn't exist
*++ (for module nummer > 0) or
*++ Not any Remote Node Object does exist (for module nummer = 0)
*-- Remote Node Object mit dieser Nummer existiert nicht (für Modulnummer > 0)
*-- beziehungsweise es existiert kein Remote Node Object (für Modulnummer = 0)
* \retval CO_E_STATE
*++ The Remote Node Object is not in the state
*++ PRE_OPERATIONAL for module number > 0.
*++ There is a Remote Node Object which is not in the state
*++ PRE_OPERATIONAL for module number = 0.
*-- Das Remote Node Object ist nicht im Zustand
*-- PRE_OPERATIONAL für Modulnummer > 0,
*-- bzw. ein Remote Node Object ist nicht Zustand
*-- PRE_OPERATIONAL für Modulnummer = 0.
* \retval CO_E_NO_INITIATE
*++ Node is not in NMT master mode
*-- Knoten ist nicht im NMT master Mode
*
* INTERNAL
* changed globals: co_pNode->eState, pNodeInUse->eState
*/

static RET_T NMT_Node_req(
	UNSIGNED8	remoteNodeId,	/* Node ID 0..127 (CANopen)*/
	NODE_STATE_T	newState	/* requested state */
	CO_COMMA_REDCY_PARA_DECL
     )
{
CAN_MSG_T	canMsg;			/* CAN Message structure */
NODE_STATE_T	nextState;		/* next state */
RET_T		retVal;
UNSIGNED8	idx;
# ifdef CONFIG_NMT_SLAVE_CNT
UNSIGNED8	i;
# endif /* CONFIG_NMT_SLAVE_CNT */

    /* if not as master initialized, return */
    if ((GL_ARRAY(co_Node).flags & NMTERRFLAG_MASTER) == 0)  {
	return(CO_E_NO_INITIATE);
    }

    switch (newState)  {
	case OPERATIONAL:
	    canMsg.pData[0] = CS_START_REMOTE_NODE;
	    nextState = OPERATIONAL;
	    break;
	case PRE_OPERATIONAL:
	    canMsg.pData[0] = CS_ENTER_PRE_OP_STATE;
	    nextState = PRE_OPERATIONAL;
	    break;
	case STOPPED:
	    canMsg.pData[0] = CS_STOP_REMOTE_NODE;
	    nextState = STOPPED;
	    break;
	case RESET_APPLICATION:
	    canMsg.pData[0] = CS_RESET_APPLICATION;
	    nextState = UNKNOWN;
	    break;
	case RESET_COMM:
	    canMsg.pData[0] = CS_RESET_COMM;
	    nextState = UNKNOWN;
	    break;
        /* for static check: add all possible enums */
        case UNKNOWN:
        case INITIALISING:
	default:
	    return(CO_E_STATE);
    }

    /* local node ? */
    if ((GL_ARRAY(coNodeId) == remoteNodeId) || ((GL_ARRAY(coNodeId) | 0x80 ) == remoteNodeId))
    {
	/* yes, local node */
	setNodeState(newState CO_COMMA_REDCY_PARA);

	return(CO_OK);
    }

    canMsg.pData[1] = remoteNodeId & 0x7f;

    /* selective mode - only one node ? */
    if ((remoteNodeId & 0x7f) != 0) {

	/* if node in network list ? */
	idx = getNmtSlaveIndex(remoteNodeId CO_COMMA_LINE_PARA);
	if (idx == 0xff) {
	    if ((remoteNodeId & 0x80) == 0) {
		return(CO_E_NOT_EXIST);
	    }
	} else {

#ifdef CONFIG_NMT_SLAVE_CNT
	    /* set new node state for this node */
	    GL_PVAR(nmtSlaveList)[idx
#  ifdef CONFIG_MULT_LINES
			+ GL_ARRAY(co_nmtSlaveLineOffs)
#  endif /* CONFIG_MULT_LINES */
			].eState = nextState;
#endif /* CONFIG_NMT_SLAVE_CNT */
	}

#ifdef CONFIG_HEARTBEAT_CONSUMER
	/* set heartbeat state */
	(void) setHbNodeState(remoteNodeId & 0x7f, nextState CO_COMMA_LINE_PARA);
#endif /* CONFIG_HEARTBEAT_CONSUMER */

#ifdef CONFIG_MASTER
# ifdef CONFIG_NODE_GUARDING
	/* set nodeguarding state */
	(void) setGuardNodeState(remoteNodeId & 0x7f, nextState CO_COMMA_LINE_PARA);
# endif /* CONFIG_NODE_GUARDING */
#endif /* CONFIG_MASTER */
    } else {

	/* network mode */

#ifdef CONFIG_NMT_SLAVE_CNT
	/* for all remote nodes */
	for (i = 0; i < CO_NMT_SLAVE_LINE_CNTS; i++)  {
	    idx = getNmtSlaveIndex(i CO_COMMA_LINE_PARA);
	    if (idx != 0xff)  {
		/* set new node state for this node */
		GL_PVAR(nmtSlaveList)[idx
# ifdef CONFIG_MULT_LINES
			    + GL_ARRAY(co_nmtSlaveLineOffs)
# endif /* CONFIG_MULT_LINES */
			    ].eState = nextState;
	    }
	}
#endif /* CONFIG_NMT_SLAVE_CNT */

# ifdef CONFIG_HEARTBEAT_CONSUMER
	/* set new state for this hb node */
	(void) setHbNodeState(0, nextState CO_COMMA_LINE_PARA);
# endif /* CONFIG_HEARTBEAT_CONSUMER */

# ifdef CONFIG_NODE_GUARDING
	/* set new state for this guarding node */
	(void) setGuardNodeState(0, nextState CO_COMMA_LINE_PARA);
# endif /* CONFIG_NODE_GUARDING */

	/* for local node */
	if (remoteNodeId == 0) {
	    setNodeState(newState CO_COMMA_REDCY_PARA);
	}
    }

    /* send NMT command */
#ifdef CONFIG_REDUNDANCY_SUPPORT
    GL_VAR(co_redcyNmtLine) = canLine;
#endif /* CONFIG_REDUNDANCY_SUPPORT */

    retVal = TRANSMIT_COB(GL_ARRAY(co_pNMT_COB), &canMsg.pData[0]);

    return(retVal);
}


/****************************************************************************/
/**
*++ \brief startRemoteNodeReq - request the service Start Remote Node.
*-- \brief startRemoteNodeReq - fordert den Dienst Start Remote Node an.
*
*++ The NMT-Slave referenced by the Node-ID
*++ 1..127, 0 for all slaves or 128 for all slaves except the own node
*++ will be forced to the state OPERATIONAL.
*++ The state of the slaves have be PRE_OPERATIONAL or STOPPED.
*++ In the case of Node-ID = 0 all slaves will be forced to OPERATIONAL.
*
*-- Der NMT-Master versetzt den durch die Node-ID
*-- 1..127 (CANopen), 0 für alle oder 128 für alle ausser dem eigenen Node
*-- angegebenen NMT-Slave
*-- in den Zustand OPERATIONAL.
*-- Ist die Node-ID gleich 0,
*-- so werden alle Knoten,
*-- die sich im Zustand STOPPED oder
*-- PRE_OPERATIONAL befinden
*-- in den Zustand OPERATIONAL versetzt (gestartet).
*
* \retval CO_OK
*++ success
*-- Erfolg
* \retval CO_E_NO_NETWORK
*++ Network Object not defined
*-- Kein Netzwerkobjekt vorhanden
* \retval CO_E_NOT_EXIST
*++ Remote Node Object with specified number doesn't exist
*++ (for module nummer > 0) or
*++ Not any Remote Node Object does exist (for module nummer = 0)
*-- Remote Node Object mit dieser Nummer existiert nicht (für Modulnummer > 0)
*-- beziehungsweise es existiert kein Remote Node Object (für Modulnummer = 0).
* \retval CO_E_STATE
*++ The Remote Node Object is not in the state PREPARED or
*++ PRE_OPERATIONAL (only CANopen) for module number > 0.
*++ There is no Remote Node Object which is in the state PREPARED or
*++ PRE_OPERATIONAL (only CANopen) for module number = 0.
*-- Das Remote Node Object ist nicht im Zustand PREPARED
*-- oder PRE_OPERATIONAL (nur CANopen) für Modulnummer > 0,
*-- bzw. kein Remote Node Object ist ist Zustand PREPARED
*-- oder PRE_OPERATIONAL (nur CANopen) für Modulnummer = 0.
* \retval CO_E_NO_INITIATE
*++ Node is not in NMT master mode
*-- Knoten ist nicht im NMT master Mode
*
* changed globals: co_pNode->eState, pNodeInUse->eState
*/

RET_T startRemoteNodeReq(
	UNSIGNED8 bMod_ID    /**< Node ID 0..127 (CANopen)*/
	CO_COMMA_REDCY_PARA_DECL
     )
{
    return(NMT_Node_req(bMod_ID, OPERATIONAL CO_COMMA_REDCY_PARA));
}


/****************************************************************************/
/**
*++ \brief stopRemoteNodeReq - request the service Stop Remote Node.
*-- \brief stopRemoteNodeReq - fordert den Dienst Stop Remote Node an.
*
*++ The NMT-Slave referenced by the Node-ID
*++ 1..127, 0 for all slaves or 128 for all slaves without the own node
*++ will be forced to the state STOPPED.
*++ The state of the slaves have to be OPERATIONAL or
*++ PRE_OPERATIONAL (only CANopen).
*++ In the case of Node-ID = 0 all slaves will be forced to the state
*++ STOPPED.
*
*-- Der NMT-Master versetzt den durch die Node-ID
*-- 1..127, 0 für alle oder 128 für alle ausser dem eigenen Node
*-- angegebenen NMT-Slave
*-- in den Zustand STOPPED.
*-- Ist die Node-ID gleich 0,
*-- so werden alle Knoten,
*-- die sich im Zustand OPERATIONAL bzw. PRE_OPERATIONAL
*-- befinden in den Zustand STOPPED versetzt.
*
* \retval CO_OK
*++ success
*-- Erfolg
* \retval CO_E_NOT_EXIST
*++ Remote Node Object with specified number doesn't exist
*++ (for module nummer > 0) or
*++ no Remote Node Object doesn't exist (for module nummer = 0)
*-- Remote Node Object mit dieser Nummer existiert nicht (für Modulnummer > 0)
*-- beziehungsweise es existiert kein Remote Node Object (für Modulnummer = 0)
* \retval CO_E_STATE
*++ The Remote Node Object is not in the state OPERATIONAL or
*++ PRE_OPERATIONAL (only CANopen) for module number > 0.
*++ There is no Remote Node Object which is in the state OPERATIONAL or
*++ PRE_OPERATIONAL (only CANopen) for module number = 0.
*-- Das Remote Node Object ist nicht im Zustand OPERATIONAL
*-- oder PRE_OPERATIONAL (nur CANopen) für Modulnummer > 0,
*-- bzw. ein Remote Node Object ist nicht Zustand OPERATIONAL
*-- oder PRE_OPERATIONAL (nur CANopen) für Modulnummer = 0.
* \retval CO_E_NO_INITIATE
*++ Node is not in NMT master mode
*-- Knoten ist nicht im NMT master Mode
*
* changed globals: co_pNode->eState, pNodeInUse->eState
*/

RET_T stopRemoteNodeReq(
	UNSIGNED8 bMod_ID    /**< Node ID 0..127 */
	CO_COMMA_REDCY_PARA_DECL
    )
{
    return(NMT_Node_req(bMod_ID, STOPPED CO_COMMA_REDCY_PARA));
}



/****************************************************************************/
/**
*++ \brief enterPreOpStateReq - set NMT-Slave in PRE_OPERATIONAL state
*-- \brief enterPreOpStateReq - setzt NMT-Slave in den Zustand PRE_OPERATIONAL
*
*++ The NMT-Master sets the NMT-Slave with ID \em nodeId
*++ 1..127, 0 for all slaves or 128 for all slaves without the own node
*++ in the state PRE_OPERATIONAL.
*++ The service is unconfirmed
*++ and mandatory for devices which support dynamic PDO configuration.
*++ In the state PRE_OPERATIONAL the node can communicate only via SDOs.
*-- Der NMT-Master setzt den NMT-Slave mit der ID \em nodeId
*-- 1..127, 0 für alle oder 128 für alle ausser dem eigenen Node
*-- in den Zustand PRE_OPERATIONAL.
*-- Im Zustand PRE_OPERATIONAL können die Knoten nur über SDO
*-- kommunizieren.
* \retval CO_E_NO_INITIATE
*++ Node is not in NMT master mode
*-- Knoten ist nicht im NMT master Mode
*
* \retval CO_OK
*++ success
*-- Erfolg
* \retval CO_E_NOT_EXIST
*++ the node with the choosen ID doesn't exist
*-- Der gewählte Knoten mit dieser ID existiert nicht
* \retval CO_E_STATE
*++ the choosen node is in the wrong state
*-- Der gewählte Knoten ist im falschen Zustand
* \retval CO_E_NO_INITIATE
*++ Node is not in NMT master mode
*-- Knoten ist nicht im NMT master Mode
*/

RET_T enterPreOpStateReq(
	UNSIGNED8 nodeId	/**< Identification number of the node */
	CO_COMMA_REDCY_PARA_DECL
      )
{
    return(NMT_Node_req(nodeId, PRE_OPERATIONAL CO_COMMA_REDCY_PARA));
}


/****************************************************************************/
/**
*++ \brief resetCommReq - set NMT-Slave in RESET_COMM state
*-- \brief resetCommReq - setzt den NMT-Slave in den RESET_COMM Zustand
*
*++ The NMT-Master sets the NMT-Slave with ID \em nodeId
*++ 1..127, 0 for all slaves or 128 for all slaves without the own node
*++ in the state RESET_COMM.
*++ In this state the parameters in the communication area of the object
*++ dictionary are set to their default values.
*++ The state is temporary only.
*++ That means the reached state will change automatically to
*++ PRE_OPERATIONAL afterwards.
*++ This service is unconfirmed and mandatory for all devices.
*-- Der NMT-Master setzt den NMT-Slave mit der ID \em nodeId
*-- 1..127, 0 für alle oder 128 für alle ausser dem eigenen Node
*-- in den Zustand RESET_COMM.
*-- In diesem Zustand werden die Kommunikationsparameter des Knotens
*-- auf die Standardwerte zurückgesetzt.
*-- Der Zustand ist nur temporär, d.h. der Knoten geht automatisch
*-- in den Zustand PRE_OPERATIONAL über.
*-- Dieser Dienst wird nicht bestätigt und ist vorgeschrieben für
*-- alle Geräte.
* \retval CO_E_NO_INITIATE
*++ Node is not in NMT master mode
*-- Knoten ist nicht im NMT master Mode
*
* \retval CO_OK
*++ success
*-- Erfolg
* \retval CO_E_NOT_EXIST
*++ the node with the choosen ID doesn't exist
*-- der gewählte Knoten existiert nicht
*/

RET_T resetCommReq(
	UNSIGNED8  nodeId  /**< Identificationsnumber of the node */
	CO_COMMA_REDCY_PARA_DECL
      )
{
    return(NMT_Node_req(nodeId, RESET_COMM CO_COMMA_REDCY_PARA));
}


/****************************************************************************/
/**
*++ \brief resetNodeReq - reset the application of the NMT-Slave
*-- \brief resetNodeReq - setzt die Applikation des NMT-Slave zurück
*
*++ The NMT-Master sets the NMT-Slave with ID \em nodeId
*++ 1..127, 0 for all slaves or 128 for all slaves without the own node
*++ in the state RESET_APPLICATION.
*++ In this state a reset of the node application will be performed.
*++ The state is temporary only.
*++ That means the reached state will change automatically to
*++ PRE_OPERATIONAL afterwards.
*++ The service is unconfirmed and mandatory for all devices.
*-- Der NMT-Master setzt den NMT-Slave mit der ID \em nodeId
*-- 1..127 (CANopen), 0 für alle oder 128 für alle ausser dem eigenen Node
*-- in den Zustand RESET_APPLICATION.
*-- In diesem Zustand wird die Applikation des Knotens zurückgesetzt.
*-- Der Zustand ist nur temporär, d.h. der Knoten geht in den Zustand
*-- PRE_OPERATIONAL automatisch über.
*-- Dieser Dienst wird nicht bestätigt und ist vorgeschrieben für
*-- alle Geräte.
* \retval CO_E_NO_INITIATE
*++ Node is not in NMT master mode
*-- Knoten ist nicht im NMT master Mode
*
* \retval CO_OK
*++ success
*-- Erfolg
* \retval CO_E_NOT_EXIST
*++ the node with the choosen ID doesn't exist
*-- der gewählte Knoten existiert nicht
*/

RET_T resetNodeReq(
	UNSIGNED8  nodeId  /**< Identificationsnumber of the node */
	CO_COMMA_REDCY_PARA_DECL
    )
{
    return(NMT_Node_req(nodeId, RESET_APPLICATION CO_COMMA_REDCY_PARA));
}


/*******************************************************************
*
* initNmtMasterVars - init all NMT Master variables
*
* \internal
*
* RETURNS
* \retval nothing
*
*/

void initNmtMasterVars(
	CO_LINE_PARA_DECL
    )
{
# ifdef CONFIG_MULT_LINES
UNSIGNED8	l;
UNSIGNED16	offs;
# endif /* CONFIG_MULT_LINES */

    /* clear global variables (some compilers doesn't clear global variables */
# ifdef CONFIG_CLEAR_CO_GLOBAL_VARS
#  ifdef CONFIG_NMT_SLAVE_CNT
    memset(&GL_PVAR(nmtSlaveList)[0], (int)0,
	(size_t)(sizeof(REMOTE_NODE_T) * NMT_SLAVE_CNT));
#   ifdef CONFIG_FAST_SORT
    memset(&GL_PVAR(nmtSlaveIdxList)[0], (int)0,
	(size_t)(sizeof(UNSIGNED8) * NMT_SLAVE_CNT));
#   endif /* CONFIG_FAST_SORT */
#  endif /* CONFIG_NMT_SLAVE_CNT */
# endif /* CONFIG_CLEAR_CO_GLOBAL_VARS */

# ifdef CONFIG_MULT_LINES
    /* calculate line offsets */
    l = canLine;
    offs = 0;
    while (l > 0)  {
	l--;
	offs += co_nmtSlaveLineCnts[l];
    }
    GL_ARRAY(co_nmtSlaveLineOffs) = offs;
# endif /* CONFIG_MULT_LINES */
}
#endif  /* CONFIG_MASTER */

/*______________________________________________________________________EOF_*/
