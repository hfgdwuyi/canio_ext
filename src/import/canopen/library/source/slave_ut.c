/*
 *++ slave_ut - slave utilities for optional master features
 *-- slave_ut - slave utilities für optionale Masterfunktionalität
 *
 * Copyright (c) 1998-2017 port GmbH Halle (Saale)
 *------------------------------------------------------------------
 */


/****************************************************************************/
/**
*  \file slave_ut.c
*++ slave utilities for optional master features
*-- Slave utilities für optionale Masterfunktionalität
*  \author port GmbH Halle (Saale)
*
*++ This file contains utilities for CANopen slaves to substitute
*++ a CANopen network master minimum functionality.
*++ It contains features for
*++ start/stop of other slaves.
*-- Diese Datei beinhaltet Zusatzfunktionen für CANopen-Slaves, die
*-- die Minimalfunktionalität des CANopen-Master ersetzt.
*-- Es werden Mechanismen für zusätzliche Netzwerkdienste wie
*-- das Starten/Stoppen anderer Slaves bereitgestellt.
*/

/* header of standard C - libraries */
# include <stdio.h>
# include <stdlib.h>

/* project headers */
# include <cal_conf.h>
# include <co_cobid.h>
# include <co_splus.h>
# include "nmt.h"
# include "nmt_s.h"
# include "drv.h"

#if defined(CONFIG_SRDO_CONSUMER) || defined(CONFIG_SRDO_PRODUCER)
# include "srdo.h"
#endif /* defined(CONFIG_SRDO_CONSUMER) || defined(CONFIG_SRDO_PRODUCER) */

#ifdef CONFIG_REDUNDANCY_SUPPORT
# include "reduncy.h"
#endif /* CONFIG_REDUNDANCY_SUPPORT */


/* local header */

/* constant definitions
---------------------------------------------------------------------------*/
#ifndef WAIT_FOR_TRANSMIT_RDY
# define WAIT_FOR_TRANSMIT_RDY
#endif /* WAIT_FOR_TRANSMIT_RDY */

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
/* flag for switching to master */
#ifdef CONFIG_SLAVE_PLUS /* module is only valid for this configuration */

/* local defined variables
---------------------------------------------------------------------------*/
#ifdef CONFIG_RCS_IDENT
CO_LIB_INIT_VAR static char _rcsid[] = "$Id: slave_ut.c,v 2.25 2016/09/26 11:16:09 rli Exp $";
#endif /* CONFIG_RCS_IDENT */


/****************************************************************************/
/**
*++ \brief newLocalStateReq - force the own slave into the selected state
*-- \brief newLocalStateReq - überführt den eigenen Slave in den gewählten Zustand
*
*++ This function forces the own slave to the requested state.
*++ There is no transmitting of any message.
*++ Possible communication states are:
*-- Diese Funktion überführt den eigenen Slave in den gewählten Zustand.
*-- Dabei erfolgt kein Senden am Bus.
*-- Mögliche Zustände sind:
*
* \code
* OPERATIONAL
* PRE_OPERATIONAL
* STOPPED
* \endcode
* \par
*++ All other states are not supported by this function.
*-- Alle anderen Zustände werden durch diese Funktion nicht unterstützt.
*
* \code
* // start of network
* newRemoteStateReq(OPERATIONAL);
* // stop of network
* newRemoteStateReq(STOPPED);
* // force network to pre-operational
* newRemoteStateReq(PRE_OPERATIONAL);
* \endcode
*
* \retval CO_OK
*++ success
*-- Erfolg
* \retval CO_E_STATE
*-- Node is already in this state
*++ Knoten befindet sich schon im vorgegebenen Status
* \retval CO_E_TYPE
*++ parameter \em newstate has wrong value
*-- Parameter \em newstate besitzt ungültigen Wert
*
*/

RET_T newLocalStateReq(
	NODE_STATE_T newState	/**< new communication state */
	CO_COMMA_REDCY_PARA_DECL/**< number of CAN line 0..CO_MAX_CAN_LINES-1 */
    )
{
LOCAL_NODE_T	*pNode;

# ifdef CONFIG_REDUNDANCY_SUPPORT
    if (canLine == 0)  {
	pNode = &GL_VAR(co_Node);
    } else {
	pNode = &GL_VAR(co_redcyNode);
    }
# else /* CONFIG_REDUNDANCY_SUPPORT */
    pNode = &GL_ARRAY(co_Node);
# endif /* CONFIG_REDUNDANCY_SUPPORT */

    /* check for NULL pointer */
    switch (newState) {
	case OPERATIONAL:
	case PRE_OPERATIONAL:
	case STOPPED:
	     break;

	default:
	    return(CO_E_TYPE);
    }

# if defined(CONFIG_SRDO_CONSUMER) || defined(CONFIG_SRDO_PRODUCER)
    if (newState == OPERATIONAL)  {
	if (srdoGoOperational() == CO_OK)  {
	    if (newStateInd(newState CO_COMMA_LINE_PARA) == CO_TRUE)  {
		pNode->eState = newState;
	    }
	}
    } else {
	/* not OPERATIONAL - delete all srdo timer events */
	srdoGoPreop();
	newStateInd(newState CO_COMMA_LINE_PARA);
	pNode->eState = newState;
    }
# else /* defined(CONFIG_SRDO_CONSUMER) || defined(CONFIG_SRDO_PRODUCER) */

    if (newStateInd(newState CO_COMMA_REDCY_PARA) == CO_FALSE)  {

	/* don't change to OPERATIONAL */
	if (newState != OPERATIONAL)  {
	    pNode->eState = newState;
	}
    } else {
	pNode->eState = newState;
    }
# endif /* defined(CONFIG_SRDO_CONSUMER) || defined(CONFIG_SRDO_PRODUCER) */

# ifdef CONFIG_CO_RUN_LED
    updateNMTState_led(CO_REDCY_PARA);
# endif /* CONFIG_CO_RUN_LED */

    return(CO_OK);
}


/****************************************************************************/
/**
*++ \brief newRemoteStateReq - force all slaves into the selected state
*-- \brief newRemoteStateReq - überführt alle Slaves in den gewählten Zustand
*
*++ This function forces all other slaves to the requested state.
*++ Possible communication states are:
*-- Diese Funktion überführt alle anderen Slaves in den gewählten Zustand.
*-- Mögliche Zustände sind:
*
* \code
* OPERATIONAL
* PRE_OPERATIONAL
* STOPPED
* \endcode
* \par
*++ Other states are not supported by this function.
*-- Andere Zustände werden durch diese Funktion nicht unterstützt.
*
* \code
* // start of network
* newRemoteStateReq(OPERATIONAL);
* // stop of network
* newRemoteStateReq(STOPPED);
* // force network to pre-operational
* newRemoteStateReq(PRE_OPERATIONAL);
* \endcode
*
* \retval CO_OK
*++ success
*-- Erfolg
* \retval CO_E_STATE
*-- Node is already in this state
*++ Knoten befindet sich schon im vorgegebenen Status
* \retval CO_E_TYPE
*++ parameter \em newstate has wrong value
*-- Parameter \em newstate besitzt ungültigen Wert
*
*/

RET_T newRemoteStateReq(
	NODE_STATE_T newState	/**< new communication state */
	CO_COMMA_REDCY_PARA_DECL/**< number of CAN line 0..CO_MAX_CAN_LINES-1 */
    )
{
RET_T	retVal;
UNSIGNED8 msg[2]={0,0};  /* NMT code
			    byte 0: state
			    byte 1: always 0 for broadcasting
			  */
LOCAL_NODE_T	*pNode;

# ifdef CONFIG_REDUNDANCY_SUPPORT
    if (canLine == 0)  {
	pNode = &GL_VAR(co_Node);
    } else {
	pNode = &GL_VAR(co_redcyNode);
    }
# else /* CONFIG_REDUNDANCY_SUPPORT */
    pNode = &GL_ARRAY(co_Node);
# endif /* CONFIG_REDUNDANCY_SUPPORT */

    /* check for NULL pointer */
    switch (newState) {
	case OPERATIONAL:
	     msg[0] = CS_START_REMOTE_NODE;
	     break;

	case PRE_OPERATIONAL:
	     msg[0] = CS_ENTER_PRE_OP_STATE;
	     break;

	case STOPPED:
	     msg[0] = CS_STOP_REMOTE_NODE;
	     break;

	default:
	    return(CO_E_TYPE);
    }

    /* switch COB-type to transmit */
    retVal = SET_COB_ID(GL_ARRAY(co_pNMT_COB), CO_COBID_NMT, CO_COB_NMT_MASTER);
    if (retVal != CO_OK)  {
	return(retVal);
    }

    /* send NMT command */
#ifdef CONFIG_REDUNDANCY_SUPPORT
    GL_VAR(co_redcyNmtLine) = canLine;
#endif /* CONFIG_REDUNDANCY_SUPPORT */

    /* send to all remote nodes */
    retVal = TRANSMIT_COB(GL_ARRAY(co_pNMT_COB), &msg[0]);
    if (retVal != CO_OK)  {
	return(retVal);
    }

    /* don't change cob-type back until transmission is finished */
    /* only necessary for some CAN controllers ... */
    WAIT_FOR_TRANSMIT_RDY

    /* switch COB-type back */
    retVal = SET_COB_ID(GL_ARRAY(co_pNMT_COB), CO_COBID_NMT, CO_COB_NMT_SLAVE);
    if (retVal != CO_OK)  {
	return(retVal);
    }

    /* are we already in this new state ? */
    if (pNode->eState == newState)  {
	return(CO_OK);
    }

    retVal = newLocalStateReq(newState CO_COMMA_REDCY_PARA);

    return(retVal);
}

#endif /* CONFIG_SLAVE_PLUS */
/*______________________________________________________________________EOF_*/
