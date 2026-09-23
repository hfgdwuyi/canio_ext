/*
 *++ nmt - Network Management for Minimum Boot Up (Module Control)
 *-- nmt - Network Management für Minimum Boot Up (Module Control)
 *
 * Copyright (c) 2001-2017 port GmbH Halle (Saale)
 *------------------------------------------------------------------
 */


/****************************************************************************/
/**
*  \file nmt.c
*++ Network Management for Minimum Boot Up (Module Control)
*-- Network Management für Minimum Boot Up (Module Control)
*  \author port GmbH Halle (Saale)
*
*++ This module contains the functions for the Network Management Control protocol.
*++ It contains the functions for CANopen Minimum Boot Up.
*-- Dieses Modul beinhaltet Funktionen des Network Management Protokolls.
*-- Es ist beschränkt auf Funktionen,
*-- die für das CANopen Minimum Boot Up
*-- benötigt werden.
*
*-- Folgende Kombinationen sind möglich
*++ The following combinations are possible.
*
*\code
* 		Slave	Master
* NMT_Master		x
* Nodeguarding		o
* Lifeguarding	o
* HB Producer	o	o
* HB Consumer	o	o
*
*-- x - zwingend
*++ x - mandatory
* o - optional
*\endcode
*
*/


/* header of standard C - libraries */

#include <string.h>
#include <stdio.h>

/* header of project specific types */

#include <cal_conf.h>
#include <co_odidx.h>
#include <co_cobid.h>
#include "nmt.h"
#include "nmt_s.h"
#include "nmterr.h"
#include "access.h"
#include "drv.h"
#include <co_usr.h>
#if defined(CONFIG_MASTER)
# include "nmt_m.h"
#endif /* defined(CONFIG_MASTER) */

#ifdef CONFIG_HEARTBEAT_CONSUMER
#include "heartbt.h"
#endif /* CONFIG_HEARTBEAT_CONSUMER */

#ifdef CONFIG_FLYING_MASTER
#include "flyma.h"
#endif /* CONFIG_FLYING_MASTER */

#if defined(CONFIG_SRDO_CONSUMER) || defined(CONFIG_SRDO_PRODUCER)
#include "srdo.h"
#endif /* defined(CONFIG_SRDO_CONSUMER) || defined(CONFIG_SRDO_PRODUCER) */

#ifdef CONFIG_LSS_SLAVE
# include "lss.h"
#endif /* CONFIG_LSS_SLAVE */

#ifdef CONFIG_CO_LED
# include "led.h"
#endif /* CONFIG_CO_RUN_LED */

#ifdef CONFIG_REDUNDANCY_SUPPORT
# include "reduncy.h"
#endif /* CONFIG_REDUNDANCY_SUPPORT */

#ifdef CONFIG_SYNC_PRODUCER
# include "sync.h"
#endif /* CONFIG_SYNC_PRODUCER */

#if defined(CONFIG_PDO_CONSUMER) || defined(CONFIG_PDO_PRODUCER)
# include "pdo.h"
#endif /* defined(CONFIG_PDO_CONSUMER) || defined(CONFIG_PDO_PRODUCER) */

/* constant definitions
---------------------------------------------------------------------------*/

/* local defined data types
---------------------------------------------------------------------------*/

/* list of external used functions, if not in headers
---------------------------------------------------------------------------*/
#ifdef CO_CONFIG_USER_NMT_MSG_IND
    RET_T coUserNmtMsgInd( UNSIGNED8 newState CO_COMMA_LINE_PARA_DECL );
#endif /* CO_CONFIG_USER_NMT_MSG_IND */

/* list of global defined functions
---------------------------------------------------------------------------*/

/* list of local defined functions
---------------------------------------------------------------------------*/

/* external variables
---------------------------------------------------------------------------*/

/* global variables
---------------------------------------------------------------------------*/
#ifdef CONFIG_NO_GLOBAL_VARS
#else /* CONFIG_NO_GLOBAL_VARS */

/* pointer to local node structure */
CO_LIB_UNINIT_VAR LOCAL_NODE_T	co_Node	CO_LINE_PARA_ARRAY_DEF;
# ifdef CONFIG_REDUNDANCY_SUPPORT
CO_LIB_UNINIT_VAR LOCAL_NODE_T	co_redcyNode;
# endif /* CONFIG_REDUNDANCY_SUPPORT */

					/* NMT COB (master or slave) */
CO_LIB_UNINIT_VAR COB_T		*co_pNMT_COB	CO_LINE_PARA_ARRAY_DEF;
CO_LIB_UNINIT_VAR UNSIGNED32	co_nmtStartUp	CO_LINE_PARA_ARRAY_DEF;

#endif /* CONFIG_NO_GLOBAL_VARS */

/* local defined variables
---------------------------------------------------------------------------*/
#ifdef CONFIG_RCS_IDENT
CO_LIB_INIT_VAR static char _rcsid[] = "$Id: nmt.c,v 2.56 2016/08/29 14:00:04 rli Exp $";
#endif /* CONFIG_RCS_IDENT */



/*******************************************************************
*
* H_NMT_NodeStartStopMsg - eval the NMT messages at the node
*
* \internal
*
* nothing
*
*/
void NMT_NodeStartStopMsg(
	CAN_MSG_T *canMsg		/* Pointer to CAN Message */
	CO_COMMA_REDCY_PARA_DECL	/* number of CAN line 0..CO_MAX_CAN_LINES-1 */
    )
{
NODE_STATE_T	newState = STOPPED;		/* temporary state */
# ifdef CO_CONFIG_WRONG_MSG_IND_NMT
RET_T           ret = CO_OK;
# endif /* CO_CONFIG_WRONG_MSG_IND_NMT */
# ifdef CO_CONFIG_USER_NMT_MSG_IND
RET_T           retVal = CO_OK;
# endif /* CO_CONFIG_USER_NMT_MSG_IND */

    /* Has the message the length of a valid nmt-message */
    if( canMsg->length == PCO_VALID_MESSAGE_LENGTH_NMT ) {
            /* NMT message for all nodes or only for this node ? */
        if ((canMsg->pData[1] != 0u) && (canMsg->pData[1] != GL_ARRAY(coNodeId)))  {
	    /* Do nothing yet */
        } else {

        switch (canMsg->pData[0]) {

	    case CS_START_REMOTE_NODE :
	        newState = OPERATIONAL;
# ifdef CO_CONFIG_USER_NMT_MSG_IND
                retVal = coUserNmtMsgInd( newState CO_COMMA_LINE_PARA );
                if ( retVal == CO_OK )
# endif /* CO_CONFIG_USER_NMT_MSG_IND */
                {
                    setNodeState(newState CO_COMMA_REDCY_PARA);
                }
	        break;

	    case CS_ENTER_PRE_OP_STATE :
	        newState = PRE_OPERATIONAL;
# ifdef CO_CONFIG_USER_NMT_MSG_IND
                retVal = coUserNmtMsgInd( newState CO_COMMA_LINE_PARA );
                if ( retVal == CO_OK )
# endif /* CO_CONFIG_USER_NMT_MSG_IND */
                {
# ifdef CONFIG_LSS_SLAVE
	            setLssState(LSS_SWITCH_MODE_WAIT_C CO_COMMA_LINE_PARA);
# endif /* CONFIG_LSS_SLAVE */
                    setNodeState(newState CO_COMMA_REDCY_PARA);
                }
	        break;

	    case CS_RESET_APPLICATION :
	        newState = RESET_APPLICATION;
# ifdef CO_CONFIG_USER_NMT_MSG_IND
                retVal = coUserNmtMsgInd( newState CO_COMMA_LINE_PARA );
                if ( retVal == CO_OK )
# endif /* CO_CONFIG_USER_NMT_MSG_IND */
                {
                    setNodeState(newState CO_COMMA_REDCY_PARA);
                }
	        break;

	    case CS_RESET_COMM :
	        newState = RESET_COMM;
# ifdef CO_CONFIG_USER_NMT_MSG_IND
                retVal = coUserNmtMsgInd( newState CO_COMMA_LINE_PARA );
                if ( retVal == CO_OK )
# endif /* CO_CONFIG_USER_NMT_MSG_IND */
                {
                    setNodeState(newState CO_COMMA_REDCY_PARA);
                }
	        break;

	    case CS_STOP_REMOTE_NODE :
                newState = STOPPED;
# ifdef CO_CONFIG_USER_NMT_MSG_IND
                retVal = coUserNmtMsgInd( newState CO_COMMA_LINE_PARA );
                if ( retVal == CO_OK )
# endif /* CO_CONFIG_USER_NMT_MSG_IND */
                {
# ifdef CONFIG_LSS_SLAVE
	            setLssState(LSS_SWITCH_MODE_WAIT_C CO_COMMA_LINE_PARA);
# endif /* CONFIG_LSS_SLAVE */
                    setNodeState(newState CO_COMMA_REDCY_PARA);
                }
                break;

	    default:
	        newState = STOPPED;
# ifdef CO_CONFIG_WRONG_MSG_IND_NMT
                ret = coProtocolErrorInd( CO_PROT_ERR_NMT_CMD , canMsg CO_COMMA_REDCY_PARA );
                if ( ret != CO_OK )
# endif /* CO_CONFIG_WRONG_MSG_IND_NMT */
                {
                    setNodeState(newState CO_COMMA_REDCY_PARA);
                }
	        break;
            }
        }
    } else {
# ifdef CO_CONFIG_WRONG_MSG_IND_NMT
        ret = coProtocolErrorInd( CO_PROT_ERR_NMT_LEN , canMsg CO_COMMA_REDCY_PARA );
        if ( ret != CO_OK )
# endif /* CO_CONFIG_WRONG_MSG_IND_NMT */
        {
            setNodeState(newState CO_COMMA_REDCY_PARA);
        }
    }
}


/*******************************************************************
*
* setNodeState - set the requested NMT state for this node
*
* \internal
*
* This function sets the requested NMT state for this node.
* and calls the necessary functions and the user indication.
*
* \retval
*	nothing
*
*/
void setNodeState(
	NODE_STATE_T	newState	/* new NMT state */
	CO_COMMA_REDCY_PARA_DECL
     )
{
LOCAL_NODE_T	*pNode;
#if defined(CONFIG_FULLCAN)
UNSIGNED8	pData[1];		/* temporary transmit buffer */
#endif /* CONFIG_FULLCAN */
#ifdef CONFIG_SYNC_PRODUCER
# ifdef CONFIG_SYNC_COUNTER
NODE_STATE_T	oldState;
# endif /* CONFIG_SYNC_COUNTER */
#endif /* CONFIG_SYNC_PRODUCER */

#ifdef CONFIG_LSS_SLAVE
    /* don't change the state, if we don't have a valid node-id */
    if (GL_ARRAY(coNodeId) == 255)  {
	return;
    }
#endif /* CONFIG_LSS_SLAVE */

#ifdef CONFIG_REDUNDANCY_SUPPORT
    if (canLine == 0)  {
	pNode = &GL_VAR(co_Node);
    } else {
	pNode = &GL_VAR(co_redcyNode);
    }
#else /* CONFIG_REDUNDANCY_SUPPORT */
    pNode = &GL_ARRAY(co_Node);
#endif /* CONFIG_REDUNDANCY_SUPPORT */

    if (newState == RESET_APPLICATION)  {
	pNode->eState = RESET_APPLICATION;
	resetNodeMsg(CO_REDCY_PARA);
	newState = pNode->eState;
    }

    if (newState == RESET_COMM)  {
	pNode->eState = RESET_COMM;
	resetCommMsg(CO_REDCY_PARA);
	newState = pNode->eState;

#ifdef CONFIG_FLYING_MASTER
	/* start trigger timeslot */
# ifdef CONFIG_REDUNDANCY_SUPPORT
	if (canLine == GL_VAR(co_redcyActiveLine))  {
# else /* CONFIG_REDUNDANCY_SUPPORT */
	{
# endif /* CONFIG_REDUNDANCY_SUPPORT */
	    (void) flyMa_WaitNegotiationTime(CO_LINE_PARA);
	}
#endif /* CONFIG_FLYING_MASTER */
    }

    /* if new state */
    if (newState != pNode->eState)  {

#ifdef CONFIG_SYNC_PRODUCER
# ifdef CONFIG_SYNC_COUNTER
	oldState = pNode->eState;
# endif /* CONFIG_SYNC_COUNTER */
#endif /* CONFIG_SYNC_PRODUCER */

#if defined(CONFIG_SRDO_CONSUMER) || defined(CONFIG_SRDO_PRODUCER)
	if (newState == OPERATIONAL)  {
	    if (srdoGoOperational(CO_LINE_PARA) == CO_OK)  {
		if (CO_TRUE == newStateInd(newState CO_COMMA_REDCY_PARA))  {
		    pNode->eState = newState;
		}
	    }
	} else {
	    /* not OPERATIONAL - delete all srdo timer events */
	    srdoGoPreop(CO_LINE_PARA);
	    newStateInd(newState CO_COMMA_REDCY_PARA);
	    pNode->eState = newState;
	}
#else /* defined(CONFIG_SRDO_CONSUMER) || defined(CONFIG_SRDO_PRODUCER) */

	if (newStateInd(newState CO_COMMA_REDCY_PARA) == CO_FALSE)  {
	    /* don't change to OPERATIONAL */
	    if (newState != OPERATIONAL)  {
		pNode->eState = newState;
	    }
	} else {
	    pNode->eState = newState;
	}
#endif /* defined(CONFIG_SRDO_CONSUMER) || defined(CONFIG_SRDO_PRODUCER) */

#ifdef CONFIG_SYNC_PRODUCER
# ifdef CONFIG_SYNC_COUNTER
	/* changed from STOPPED to PRE-OP or OPER */
	if ((oldState == STOPPED) && (oldState != pNode->eState))  {
	    /* reset sync counter */
	    resetSyncCounter(CO_REDCY_PARA);
	}
# endif /* CONFIG_SYNC_COUNTER */
#endif /* CONFIG_SYNC_PRODUCER */

#if defined(CONFIG_PDO_CONSUMER) || defined(CONFIG_PDO_PRODUCER)
	/* was switched to OPERATIONAL ? */
	if (pNode->eState == OPERATIONAL)  {

# ifdef CONFIG_REDUNDANCY_SUPPORT
	/* sync start value shall only indicated on active interface */
	if (canLine == GL_VAR(co_redcyActiveLine))
# endif /* CONFIG_REDUNDANCY_SUPPORT */
	    {
# ifdef CONFIG_PDO_SYNC_START_VALUE
		updatePdoSyncStartValues(CO_LINE_PARA);
# endif /* CONFIG_PDO_SYNC_START_VALUE */
	    }
	}
#endif /* defined(CONFIG_PDO_CONSUMER) || defined(CONFIG_PDO_PRODUCER) */

    }

#if defined(CONFIG_FULLCAN)
    pData[0] = (UNSIGNED8) pNode->eState
#  if defined(CONFIG_NODE_GUARDING)
	/* toogle bit is already set to next state, invert it here */
	| ((UNSIGNED8)(pNode->bGuardToggle) ^ 0x80)
#  endif /* defined(CONFIG_NODE_GUARDING) */
	;
    UPDATE_COB(pNode->pGuard_COB, pData);
#endif /* CONFIG_FULLCAN */

#ifdef CONFIG_CO_RUN_LED
    updateNMTState_led(CO_REDCY_PARA);
#endif /* CONFIG_CO_RUN_LED */
}


/****************************************************************************/
/**
*++ \brief createNodeReq - request the service Create Node.
*-- \brief createNodeReq - fordert den Dienst Service Create Node an.
*
*++ This function creates the internal structures for a CANopen node
*++ with the demanded error control services.
*++ At minimum 1 service is mandatory.
*-- Diese Funktion legt die internen Strukturen für einen CANopen-Knoten
*-- mit den geforderten Überwachungsdiensten an.
*-- Mindestens 1 Überwachungsdienst muss eingerichtet werden.
*
*-- Weiterhin wird das NMT object mit der COB-ID 0 angelegt.
*-- Ob der Knoten als NMT-Master oder NMT-Slave arbeiten soll,
*-- kann über den Parameter
*++ Furthermore the NMT object with node-id 0 is created.
*++ The operating mode (master or slave) is set with the parameter
* master
*-- festgelegt werden.
*++ Depending on this parameter the NMT object is created as
*++ receive or transmit object.
*-- Dementsprechend wird das NMT-Objekt auch als Sende- oder Empfangsobjekt
*-- angelegt.
*-- Der Parameter ist nur für MASTER, SLAVE_PLUS oder Multi-Line
*-- Knoten notwendig.
*++ This parameter is only necessary for MASTER, SLAVE_PLUS or Multi-Line nodes.
*
*-- Am Ende dieser Funktion befindet sich der Knoten im Zustand
*-- PRE_OPERATIONAL,
*-- falls er eine gültige Knotennummer bekommen hat.
*++ The node state is PRE_OPERATIONAL after calling this function.
*
* \retval CO_OK
*++ success
*-- Erfolg
* \retval CO_E_MEM
*++ not enough memory
*-- nicht genug dyn. Speicher vorhanden
* \retval CO_E_ALREADY_EXIST
*++ Remote Node already exists
*-- Remote Node existiert bereits
* \retval CO_E_NO_ACCESS
*++ no access to the Object Dictionary (node ID and node-guarding parameters)
*-- kein Zugriff auf das Objektverzeichnis (node ID und Nodeguardingparameter)
* \retval CO_E_NO_INITIATE
*++ no error control service requested (node-guarding or heartbeat is mandatory)
*-- kein Error Control Service angefordert (Nodeguarding oder Heartbeat
*-- ist erforderlich)
*
*/

RET_T createNodeReq(
	BOOL_T	defNodeguarding,	/**< init node-guarding */
	BOOL_T	defHeartbeat		/**< init Heart-Beat */
#if defined(CONFIG_MASTER)
	,BOOL_T	master			/**< is master */
#endif /* defined(CONFIG_MASTER) */
	CO_COMMA_LINE_PARA_DECL	/**< number of CAN line 0..CO_MAX_CAN_LINES-1 */
    )
{
COB_KIND_T	cobKind;
RET_T		retVal;

    /* test if at least one error service is requested */
    if ((defNodeguarding == CO_FALSE) && (defHeartbeat == CO_FALSE))  {
	return(CO_E_NO_INITIATE);
    }

    GL_ARRAY(co_Node).flags = 0u;

#ifdef CONFIG_REDUNDANCY_SUPPORT
    GL_VAR(co_redcyNode).flags = 0u;
#endif /* CONFIG_REDUNDANCY_SUPPORT */

    /*---- initialize cobs for nmt -----------------------------------*/
#if defined(CONFIG_MASTER)
    if (master == CO_TRUE) {
	/* save master flag */
	GL_ARRAY(co_Node).flags |= NMTERRFLAG_MASTER;
	cobKind = CO_COB_NMT_MASTER;

# ifdef CONFIG_REDUNDANCY_SUPPORT
	GL_VAR(co_redcyNode).flags |= NMTERRFLAG_MASTER;
# endif /* CONFIG_REDUNDANCY_SUPPORT */

    } else
#endif /* defined(CONFIG_MASTER) */
    {
	cobKind = CO_COB_NMT_SLAVE;
    }

    GL_ARRAY(co_pNMT_COB) = DEFINE_COB(cobKind, 2u CO_COMMA_LINE_PARA);
    if (GL_ARRAY(co_pNMT_COB) == NULL)  {
	return(CO_E_NO_DATABASE);
    }

    retVal = SET_COB_ID(GL_ARRAY(co_pNMT_COB), CO_COBID_NMT, cobKind);
    if (retVal != CO_OK)  {
	return(retVal);
    }

    /*---- initialize cobs for nmterr -----------------------------------*/
#if defined(CONFIG_NODE_GUARDING)
    if ((defNodeguarding == CO_TRUE)
# ifdef CONFIG_MASTER
	&& (master != CO_TRUE)
# endif /* CONFIG_MASTER */
    ) {
	cobKind = CO_COB_GUARD_SLAVE;
    } else
#endif /* CONFIG_NODE_GUARDING */
    {
	cobKind = CO_COB_HB_PROD;
    }

    GL_ARRAY(co_Node).pGuard_COB = DEFINE_COB(cobKind, 1u CO_COMMA_LINE_PARA);
    if (GL_ARRAY(co_Node).pGuard_COB == NULL)  {
	return(CO_E_NO_DATABASE);
    }

#ifdef CONFIG_REDUNDANCY_SUPPORT
    GL_VAR(co_redcyNode).pGuard_COB = GL_VAR(co_Node).pGuard_COB->pNextLine;
#endif /* CONFIG_REDUNDANCY_SUPPORT */

    if (defNodeguarding == CO_TRUE)  {

#ifdef CONFIG_NODE_GUARDING
# if defined(CONFIG_MASTER)
	/* define cob only if we are not the nodeguarding master */
	if (master == CO_FALSE)
# endif /* defined(CONFIG_MASTER) */
	{
	    /* Nodeguarding is possible */
	    GL_ARRAY(co_Node).flags |= NMTERRFLAG_NG_POSSIBLE;
	}
#else /* CONFIG_NODE_GUARDING */
	/* Nodeguarding requested but not compiled */
	return(CO_E_NO_INITIATE);
#endif /* defined(CONFIG_NODE_GUARDING) */
    }

    if (defHeartbeat == CO_TRUE)  {

#if defined(CONFIG_HEARTBEAT_PRODUCER)
	GL_ARRAY(co_Node).flags |= NMTERRFLAG_HB_POSSIBLE;

# ifdef CONFIG_REDUNDANCY_SUPPORT
	GL_VAR(co_redcyNode).flags |= NMTERRFLAG_HB_POSSIBLE;
# endif /* CONFIG_REDUNDANCY_SUPPORT */

#else /* defined(CONFIG_HEARTBEAT_PRODUCER) */
	/* Heartbeat requested but not compiled */
	return(CO_E_NO_INITIATE);
#endif /* defined(CONFIG_HEARTBEAT_PRODUCER) || defined(HEARTBEAT_CONSUMER) */
    }

    /* init NMT error variables */
    retVal = initNmtErr(CO_LINE_PARA);
    if (retVal != CO_OK)  {
	return(retVal);
    }

#ifndef CO_CONFIG_DONT_AUTOSEND_BOOTUP
# ifdef CONFIG_REDUNDANCY_SUPPORT
    retVal = initNmtState(CAN_DEFAULT_LINE CO_COMMA_LINE_PARA);
    if (retVal == CO_OK)  {
	retVal = initNmtState(CAN_REDCY_LINE CO_COMMA_LINE_PARA);
    }
# else /* CONFIG_REDUNDANCY_SUPPORT */

    retVal = initNmtState(CO_LINE_PARA);
# endif /* CONFIG_REDUNDANCY_SUPPORT */
#endif /* CO_CONFIG_DONT_AUTOSEND_BOOTUP */

    return(retVal);
}


/*******************************************************************
*
* initNmtState - init NMT state maschine
*
* \internal
*
* This function setup the NMT state machine for the requested line
* send the bootup and set the leds
*
* \retval
*	RET_T
*
*/

RET_T initNmtState(
	CO_REDCY_PARA_DECL
    )
{
RET_T		retVal = CO_OK;
LOCAL_NODE_T	*pNode;
#ifndef CO_CONFIG_SUPPRESS_BOOTUP
UNSIGNED8	pData[1];	/* temporary variable */
#endif /* CO_CONFIG_SUPPRESS_BOOTUP */
#if defined(CONFIG_MASTER) || defined(CONFIG_SLAVE_PLUS) || defined(CO_CONFIG_SELFSTARTING_SLAVE)
UNSIGNED32	size;		/* size of object */
#endif /* defined(CONFIG_MASTER) || defined(CONFIG_SLAVE_PLUS) */

#ifdef CONFIG_REDUNDANCY_SUPPORT
    if (canLine == CAN_DEFAULT_LINE)  {
	pNode = &GL_ARRAY(co_Node);
    } else {
	pNode = &GL_ARRAY(co_redcyNode);
    }
#else /* CONFIG_REDUNDANCY_SUPPORT */
    pNode = &GL_ARRAY(co_Node);
#endif /* CONFIG_REDUNDANCY_SUPPORT */

#ifdef CONFIG_LSS_SLAVE
    /* if the node unconfigured */
    if (GL_ARRAY(coNodeId) == 255)  {
	pNode->eState = INITIALISING;

	/* enter lss mode wait */
	setLssState(LSS_SWITCH_MODE_WAIT CO_COMMA_LINE_PARA);

    } else
#endif /* CONFIG_LSS_SLAVE */
    {
# ifndef CO_CONFIG_DONT_AUTOSEND_BOOTUP
        if (  (GL_ARRAY(coNodeId) < 1u ) ||
              (GL_ARRAY(coNodeId) > 127u ) ) {
            return CO_E_BAD_NODEID;
        }
# endif /* CO_CONFIG_DONT_AUTOSEND_BOOTUP */
	pNode->eState = PRE_OPERATIONAL;
# ifndef CO_CONFIG_SUPPRESS_BOOTUP
	pData[0] = 0u;
	retVal = TRANSMIT_COB(pNode->pGuard_COB, pData);
# endif /* CO_CONFIG_SUPPRESS_BOOTUP */
    }

#if defined(CONFIG_SLAVE) || defined(CONFIG_SLAVE_PLUS)
# ifdef CO_CONFIG_SELFSTARTING_SLAVE
    if (getObjEntry(NMT_MASTER_INDEX, 0,
        (UNSIGNED8 *)&GL_ARRAY(co_nmtStartUp), &size,
        CO_TRUE CO_COMMA_LINE_PARA) != CO_OK)
    {
        /* object doesn't exist - set default value */
        GL_ARRAY(co_nmtStartUp) = 0;
    }
    else
    {
        if (GL_ARRAY(co_nmtStartUp) == 0x8)
        {
            coSetNodeOPERATIONAL(CO_REDCY_PARA);
        }
    }
# endif /* CO_CONFIG_SELFSTARTING_SLAVE */
#endif /* definded(CONFIG_SLAVE) || (CONFIG_SLAVE_PLUS) */

#ifdef CONFIG_CO_RUN_LED
    updateNMTState_led(CO_REDCY_PARA);
#endif /* CONFIG_CO_RUN_LED */

#if defined(CONFIG_MASTER) || defined(CONFIG_SLAVE_PLUS)
    /* read flying master bit at 0x1f80 */
    if (getObjEntry(NMT_MASTER_INDEX, 0,
		(UNSIGNED8 *)&GL_ARRAY(co_nmtStartUp), &size,
		CO_TRUE CO_COMMA_LINE_PARA) != CO_OK)  {
	/* object doesn't exist - set default value */
	GL_ARRAY(co_nmtStartUp) = 0;
    }
#endif /* defined(CONFIG_MASTER) || defined(CONFIG_SLAVE_PLUS) */

    return(retVal);
}


/****************************************************************************/
/**
*++ \brief deleteNodeReq - request the service Delete Node.
*-- \brief deleteNodeReq - fordert den Dienst Delete Node an.
*
*++ The NMT-Slave deletes its own Node Object.
*++ Allocated ressources are freed.
*
*-- Der NMT-Slave löscht das bei ihm vorhandene
*-- Node Object.
*-- Belegte Ressourcen werden freigegeben.
*
* \retval CO_OK
*++ success
*-- Erfolg
* \retval CO_E_STATE
*++ The Node Object isn't in the state DISCONNECTED or
*++ PRE_OPERATIONAL or PREPARED for
*++ Minimum Capability Device (Minimum Boot Up).
*-- Das Node Object ist nicht im Zustand DISCONNECTED bzw.
*-- PRE_OPERATIONAL oder PREPARED für
*-- Minimum Capability Device (Minimum Boot Up).
*
*/

RET_T deleteNodeReq(
	CO_LINE_PARA_DECL	/**< number of CAN line 0..CO_MAX_CAN_LINES-1 */
    )
{
RET_T retVal = CO_OK;

    if (GL_ARRAY(co_Node).eState == OPERATIONAL ) {
	retVal = CO_E_STATE;
    }

#ifdef CONFIG_REDUNDANCY_SUPPORT
    if (retVal == CO_OK) {
        if (GL_VAR(co_redcyNode).eState == OPERATIONAL)  {
	    retVal = CO_E_STATE;
        }
    }
#endif /* CONFIG_REDUNDANCY_SUPPORT */

    return(retVal);
}


#if defined(CONFIG_SLAVE) || defined(CONFIG_MASTER_PLUS) || !defined(CONFIG_NO_ERROR_BEHAVIOR)
/****************************************************************************/
/**
*++ \brief getNodeState - provide the local node state
*-- \brief getNodeState - liefert den Zustand des lokalen Netzknotens
*
*++ This function provides the communication state of the local node.
*-- Diese Funktion liefert den Kommunikationszustand
*-- des lokalen Knotens zurück.
*
* \retval 0
*++ error, node doesn't exist
*-- Fehler, der gewählte Knoten existiert nicht
* \retval STOPPED
* \retval OPERATIONAL
* \retval PRE_OPERATIONAL
* \retval RESET_APPLICATION
* \retval RESET_COMM
*
*/

NODE_STATE_T getNodeState(
	CO_REDCY_PARA_DECL
    )
{
#ifdef CONFIG_REDUNDANCY_SUPPORT
    if (canLine == 0)  {
	return(GL_VAR(co_Node).eState);
    } else {
	return(GL_VAR(co_redcyNode).eState);
    }
#else /* CONFIG_REDUNDANCY_SUPPORT */
    return(GL_ARRAY(co_Node).eState);
#endif /* CONFIG_REDUNDANCY_SUPPORT */
}


/****************************************************************************/
/**
*++ \brief setNodePREOP - set the local node state to PRE-OPERATIONAL
*-- \brief setNodePREOP - setzt den Zustand des lokalen Netzknotens
*
*++ This function changes the communication state of the local node.
*++ It is only allowed to call this function in state OPERATIONAL.
*-- Diese Funktion setzt den Kommunikationszustand
*-- des lokalen Knotens zu PRE-OPERATIONAL.
*-- Sie kann nur im Zustand OPERATIONAL aufgerufen werden.
*
* \retval CO_OK
*++ ok
*-- ok
* \retval CO_E_DEVICE_STATE
*++ error, bad device state
*-- Fehler, falscher Knotenstatus (ungleich OPERATIONAL)
*
*/

RET_T setNodePREOP(
	CO_REDCY_PARA_DECL
    )
{
RET_T retVal = CO_OK;
#ifdef CONFIG_REDUNDANCY_SUPPORT
    if (getNodeState(canLine CO_COMMA_LINE_PARA) != OPERATIONAL) {
#else /* CONFIG_REDUNDANCY_SUPPORT */
    if (GL_ARRAY(co_Node).eState != OPERATIONAL) {
#endif /* CONFIG_REDUNDANCY_SUPPORT */
	retVal = CO_E_DEVICE_STATE;
    }

    if ( retVal == CO_OK ) {
        setNodeState(PRE_OPERATIONAL CO_COMMA_REDCY_PARA);
    }

    return(retVal);
}


/****************************************************************************/
/**
*++ \brief setNodeSTOPPED - set the local node state to STOPPED
*-- \brief setNodeSTOPPED - setzt den Zustand des lokalen Netzknotens zu STOPPED
*
*++ This function changes the communication state of the local node to
*-- Diese Funktion setzt den Kommunikationszustand
*-- des lokalen Knotens zu
* STOPPED.
*
* \retval CO_OK
*++ ok
*-- ok
*
*/

RET_T setNodeSTOPPED(
	CO_REDCY_PARA_DECL
    )
{
    setNodeState(STOPPED CO_COMMA_REDCY_PARA);

    return(CO_OK);
}

/****************************************************************************/
/**
*++ \brief coSetNodeOPERATIONAL - set the local node state to OPERATIONAL
*-- \brief coSetNodeOPERATIONAL - setzt den Zustand des lokalen Netzknotens zu OPERATIONAL
*
*++ This function changes the communication state of the local node to
*-- Diese Funktion setzt den Kommunikationszustand
*-- des lokalen Knotens zu
* OPERATIONAL.
*
* \retval CO_OK
*++ ok
*-- ok
*
*/

RET_T coSetNodeOPERATIONAL(
        CO_REDCY_PARA_DECL
    )
{
    setNodeState(OPERATIONAL CO_COMMA_REDCY_PARA);

    return(CO_OK);
}

#endif /* CONFIG_SLAVE || CONFIG_MASTER_PLUS */



#if defined(CONFIG_HEARTBEAT_CONSUMER) || defined(CONFIG_MASTER)
/****************************************************************************/
/**
*++ \brief getRemoteNodeState - get last received node state
*-- \brief getRemoteNodeState - liefert den letzten empfangenen Knotenstatus
*
*++ This function returns the node state of remote nodes
*++ from the last received heartbeat or nodegurading message.
*++ This function works only for nodes,
*++ that are monitored by the application itself.
*++ If the node isn't found the function returns 0.
*-- Diese Funktion liefert den aktuellen Knotenstatus
*-- von Remote Knoten,
*-- für die eine Knotenüberwachung auf dem lokalen Knoten eingerichtet wurde.
*-- Zurückgeliefert wird der Zustand des letzten empfangenen Heartbeats
*-- bzw. Nodeguardings.
*-- Falls der Knoten nicht in der Netzwerkstruktur gefunden wird,
*-- liefert die Funktion eine 0 zurück.
*
* \retval NODE_STATE_T
*++ success
*-- Erfolg
* \retval UNKNOWN
*++ the node with the choosen ID doesn't exist
*-- Der gewählte Knoten existiert nicht
*/
NODE_STATE_T getRemoteNodeState(
	UNSIGNED8  nodeNr	/**< node number */
	CO_COMMA_REDCY_PARA_DECL
    )
{
#if defined(CONFIG_HEARTBEAT_CONSUMER) || defined(CONFIG_MASTER)
NODE_STATE_T	state;
#endif /* CONFIG_HEARTBEAT_CONSUMER */

#ifdef CONFIG_HEARTBEAT_CONSUMER
    /* actual state from heartbeat monitoring available ? */
    state = getHbNodeState(nodeNr CO_COMMA_REDCY_PARA);
    if (state != UNKNOWN)  {
	return(state);
    }
#endif /* CONFIG_HEARTBEAT_CONSUMER */

#ifdef CONFIG_MASTER
# ifdef CONFIG_NODE_GUARDING
    state = getGuardNodeState(nodeNr CO_COMMA_LINE_PARA);
    if (state != UNKNOWN)  {
	return(state);
    }
# endif /* CONFIG_NODE_GUARDING */

    state = getNmtSlaveNodeState(nodeNr CO_COMMA_LINE_PARA);
    if (state != UNKNOWN)  {
	return(state);
    }
#endif /* CONFIG_MASTER */

    /* Node not found */
    return(UNKNOWN);
}
#endif /* defined(CONFIG_HEARTBEAT_CONSUMER) || defined(CONFIG_MASTER) */


#ifdef CONFIG_CO_RUN_LED
/*******************************************************************
*
* updateNMTState_led - update run led with actual nmt state
*
* \internal
*
* nothing
*
*/
void updateNMTState_led(
	CO_REDCY_PARA_DECL	/**< number of CAN line 0..CO_MAX_CAN_LINES-1 */
    )
{
LOCAL_NODE_T	*pNode;

#ifdef CONFIG_REDUNDANCY_SUPPORT
    if (canLine == 0)  {
	pNode = &GL_VAR(co_Node);
    } else {
	pNode = &GL_VAR(co_redcyNode);
    }
#else /* CONFIG_REDUNDANCY_SUPPORT */
    pNode = &GL_ARRAY(co_Node);
#endif /* CONFIG_REDUNDANCY_SUPPORT */

    if (pNode->eState == OPERATIONAL)  {
	setCoRunLedState(CO_RUN_LED_OPERATIONAL CO_COMMA_REDCY_PARA);
    } else
    if (pNode->eState == PRE_OPERATIONAL)  {
	setCoRunLedState(CO_RUN_LED_PREOP CO_COMMA_REDCY_PARA);
    } else
    if (pNode->eState == STOPPED)  {
	setCoRunLedState(CO_RUN_LED_STOPPED CO_COMMA_REDCY_PARA);
    } else
    if (pNode->eState == RESET_COMM)  {
	setCoRunLedState(CO_RUN_LED_OFF CO_COMMA_REDCY_PARA);
    } else
    if (pNode->eState == RESET_APPLICATION)  {
        setCoRunLedState(CO_RUN_LED_OFF CO_COMMA_REDCY_PARA);
    } else
    if (pNode->eState == INITIALISING)  {
        setCoRunLedState(CO_RUN_LED_LSS CO_COMMA_REDCY_PARA);
    }
}
#endif /* CONFIG_CO_RUN_LED */

/*______________________________________________________________________EOF_*/
