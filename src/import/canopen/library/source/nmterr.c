/*
 *++ nmterr - NMT routines for network and node error handling (Slave)
 *-- nmterr - NMT Routinen zur Netzwerk- und Knotenüberwachung (Slave)
 *
 * Copyright (c) 1995-2017 port GmbH Halle (Saale)
 *------------------------------------------------------------------
 */

/*
*  \file nmterr.c
*++ NMT routines for network and node error handling (Slave)
*-- NMT Routinen zur Netzwerk- und Knotenüberwachung (Slave)
*  \author port GmbH Halle (Saale)
*
*++ This file contains the NMT Network Error Mangement functionalities.
*++ One of the two protocols
*++ \b "node guarding"
*++ or
*++ \b "heart beat"
*++ has to be supported by a slave.
*++ For a CANopen Minimum Capability Device
*++ the define \c CONFIG_NODE_GUARDING or \c CONFIG_HEARTBEAT_PRODUCER has to
*++ be set to enable this code for the compiler.
*-- Diese Datei enthält Routinen für die Heartbeat-Generierung
*-- und die Life-Guarding Überwachung des CANopen Masters.
*-- Jeder Knoten muß einen der beiden Überwachungsdienste
*-- \b Nodeguarding
*-- oder
*-- \b Heartbeat.
*-- bereitstellen.
*
*++ All of the functions are only called from within the library
*++ and not from the library user.
*++ Therefore there are no manual entries of the functions available.
*-- Alle hier enthaltenen Funktionen werden nur innerhalb der Library
*-- aufgerufen.
*-- Daher sind keine Funktionsbeschreibungen verfügbar.
*
*/

/* header of standard C - libraries */

#include <string.h>
#include <stdio.h>

/* header of project specific types */

#include <cal_conf.h>
#include <co_cobid.h>
#include <co_odidx.h>
#include <co_acces.h>
#include "nmterr.h"
#include "nmt.h"
#include "drv.h"
#include "timer.h"
#ifdef CONFIG_MASTER
# include "nmt_m.h"
#endif
#ifdef CONFIG_FLYING_MASTER
# include "flyma.h"
#endif /* CONFIG_FLYING_MASTER */
#ifdef CONFIG_HEARTBEAT_CONSUMER
# include "heartbt.h"
# include <co_hb.h>
#endif /* CONFIG_HEARTBEAT_CONSUMER */
#ifdef CONFIG_CO_LED
# include "led.h"
#endif /* CONFIG_CO_LED */
#ifdef CONFIG_NMT_STARTUP_MANAGER
# include "nmtstart.h"
#endif /* CONFIG_NMT_STARTUP_MANAGER */
#ifdef CONFIG_REDUNDANCY_SUPPORT
# include <co_redcy.h>
#endif /* CONFIG_REDUNDANCY_SUPPORT */
#ifdef CONFIG_NO_GLOBAL_VARS
# include <co_timer.h>
#endif /*CONFIG_NO_GLOBAL_VARS*/


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
#ifdef CONFIG_NO_GLOBAL_VARS
#else /* CONFIG_NO_GLOBAL_VARS */
# if defined(CONFIG_HEARTBEAT_CONSUMER) || (defined(CONFIG_MASTER) && defined(CONFIG_NODE_GUARDING))
				/* failed hb consumer (bitcoded) */
CO_LIB_UNINIT_VAR UNSIGNED8	nmtErrFailed[NMTERR_MAX_INDEX] CO_REDCY_PARA_ARRAY_DEF;
CO_LIB_UNINIT_VAR UNSIGNED8	nmtErrStarted[NMTERR_MAX_INDEX] CO_REDCY_PARA_ARRAY_DEF;
CO_LIB_UNINIT_VAR UNSIGNED8	nmtErrConfig[NMTERR_MAX_INDEX] CO_REDCY_PARA_ARRAY_DEF;

#  ifdef CONFIG_REDUNDANCY_SUPPORT
CO_LIB_UNINIT_VAR UNSIGNED8	nmtErr3HBok[NMTERR_MAX_INDEX];

#   ifdef CONFIG_MARITIME_SUPPORT
CO_LIB_UNINIT_VAR UNSIGNED8	nmtErrRedundancy[NMTERR_MAX_INDEX];
#   endif /* CONFIG_MARITIME_SUPPORT */
#  endif /* CONFIG_REDUNDANCY_SUPPORT */
# endif /* defined(CONFIG_HEARTBEAT_CONSUMER) || (defined(CONFIG_MASTER) && defined(CONFIG_NODE_GUARDING)) */
#endif /* CONFIG_NO_GLOBAL_VARS */


/* local defined variables
---------------------------------------------------------------------------*/
#ifdef CONFIG_RCS_IDENT
CO_LIB_INIT_VAR static char _rcsid[] = "$Id: nmterr.c,v 2.50 2016/09/26 11:16:07 rli Exp $";
#endif /* CONFIG_RCS_IDENT */


/*******************************************************************
*
* initNmtErr - init NMT err control mechanism
*
* \internal
*
*
* It is called at startup and at resetComm
*
* \retval
* nothing
*/
RET_T initNmtErr(
	CO_LINE_PARA_DECL
    )
{
UNSIGNED16	tmpU16;		/* temp u16 val */
UNSIGNED32	size;		/* size of object */
#ifdef CONFIG_NODE_GUARDING
# ifdef CONFIG_SLAVE
UNSIGNED8	tmpU8;		/* temp u8 val */
# endif /* CONFIG_SLAVE */
#endif /* CONFIG_NODE_GUARDING */
#ifdef CONFIG_DS301_V30
UNSIGNED32	tmpU32;		/* temp u32 variable */
#endif /* CONFIG_DS301_V30 */
RET_T		retVal;


#if defined(CONFIG_NODE_GUARDING) || defined(CONFIG_HEARTBEAT_PRODUCER)
    retVal = SET_COB_ID(GL_ARRAY(co_Node).pGuard_COB,
	(UNSIGNED16)(CO_COBID_NMTERR + (UNSIGNED16)GL_ARRAY(coNodeId)),
	(COB_KIND_T)((UNSIGNED32)GL_ARRAY(co_Node).pGuard_COB->eType & ~(UNSIGNED32)(CO_COB_DISABLED)));

    if (retVal != CO_OK)  {
	return(retVal);
    }

# ifdef CONFIG_FULLCAN
    /* preset guarding channel */
    pData[0] = (UNSIGNED8) GL_ARRAY(co_Node).eState;
    UPDATE_COB(GL_ARRAY(co_Node).pGuard_COB, &pData[0]);
# endif /* CONFIG_FULLCAN */

# if defined(CONFIG_NODE_GUARDING)
    if ((GL_ARRAY(co_Node).flags & NMTERRFLAG_MASTER) == 0)  {
	/* Guarding Slave */

	GL_ARRAY(co_Node).flags &= (FLAG_T)~(GUARDFLAG_NG_ACTIVE + GUARDFLAG_NG_RECEIVED);

	/* toggled to 1 after first guarding */
	GL_ARRAY(co_Node).bGuardToggle = 0;
	GL_ARRAY(co_Node).bSuspendedGuardings = 0;

	/* get Guarding Time from od */
	/* get value from object dictionary */
#  ifdef CONFIG_DS301_V30
	/* use 32bit entries from DS301 V30 */
	if (getObjEntry(GUARD_TIME_INDEX, 0, (UNSIGNED8 *)&tmpU32, &size,
		CO_TRUE CO_COMMA_LINE_PARA) != CO_OK) {
	    return CO_E_NO_ACCESS;
	}
	tmpU16 = (UNSIGNED16)tmpU32;
#  else /* CONFIG_DS301_V30 */
	if (getObjEntry(GUARD_TIME_INDEX, 0, (UNSIGNED8 *)&tmpU16, &size,
		    CO_TRUE CO_COMMA_LINE_PARA) != CO_OK) {
	    return CO_E_NO_ACCESS;
	}
#  endif /* CONFIG_DS301_V30 */

#  ifdef CONFIG_SLAVE
	/* get Lifetime Factor from od */
#   ifdef CONFIG_DS301_V30
	/* use 32bit entries from DS301 V30 */
	if (getObjEntry(LIFE_TIME_FAC_INDEX, 0, (UNSIGNED8 *)&tmpU32, &size,
		CO_TRUE CO_COMMA_LINE_PARA) != CO_OK) {
	    return CO_E_NO_ACCESS;
	}
	tmpU8 = (UNSIGNED8)tmpU32;
#   else /* CONFIG_DS301_V30 */
	if (getObjEntry(LIFE_TIME_FAC_INDEX, 0,
		    &GL_ARRAY(co_Node).bLifeTimeFactor, &size,
		    CO_TRUE CO_COMMA_LINE_PARA) != CO_OK) {
	    return CO_E_NO_ACCESS;
	}
	tmpU8 = GL_ARRAY(co_Node).bLifeTimeFactor;
#   endif /* CONFIG_DS301_V30 */

	(void) setLifeTimePara(tmpU16, tmpU8 CO_COMMA_LINE_PARA);
#  endif /* CONFIG_SLAVE */
    }
# endif /* CONFIG_NODE_GUARDING */


# ifdef CONFIG_HEARTBEAT_PRODUCER
    if ((GL_ARRAY(co_Node).flags & NMTERRFLAG_HB_POSSIBLE) != 0u)  {
	/* get Heartbeat Time from dictionary */
	if (getObjEntry(HEARTBEAT_PROD_INDEX, 0u, (UNSIGNED8 *)&tmpU16,
		 &size, CO_TRUE CO_COMMA_LINE_PARA) != CO_OK) {
	    return CO_E_NO_ACCESS;
	}
	/* set heartbeat time */
	(void)setHeartBeatProducerTime(tmpU16 CO_COMMA_LINE_PARA);
    }
# endif  /* CONFIG_HEARTBEAT_PRODUCER */

# endif /* defined(CONFIG_NODE_GUARDING) || defined(CONFIG_HEARTBEAT_PRODUCER)*/

    return(CO_OK);
}

#if defined(CONFIG_MASTER) || defined(CONFIG_HEARTBEAT_CONSUMER)
/***************************************************************************
*
* NMT_M_NodeGuardingMsg - function analyses the guarding message
*
* \internal
*
* This function analyses the guarding message answer from any slave on the
*
* \returns
* nothing
*
*/

void NMT_M_NodeGuardingMsg(
	CAN_MSG_T *canMsg	/* Pointer to CAN Message */
	CO_COMMA_REDCY_PARA_DECL
    )
{
UNSIGNED8 idx;

    /* test for bootup message */
    if ((canMsg->pData[0]) == 0) {
	/* yes, it's bootup message */

# ifdef CONFIG_FLYING_MASTER
	/* send master id */
	if (GL_ARRAY(co_activeMaster) == GL_ARRAY(coNodeId))  {
#  ifdef CONFIG_REDUNDANCY_SUPPORT
	    GL_VAR(co_redcyFlymaLine) = canLine;
#  endif /* CONFIG_REDUNDANCY_SUPPORT */
	    activeMaster_Resp(CO_LINE_PARA);
	}
# endif /* CONFIG_FLYING_MASTER */


#ifdef CONFIG_HEARTBEAT_CONSUMER
	/* set HB states for bootup */
	setHbBootupState(canMsg->cobId & 0x7f CO_COMMA_REDCY_PARA);
#endif /* CONFIG_HEARTBEAT_CONSUMER */

#ifdef CONFIG_MASTER
# ifdef CONFIG_NODE_GUARDING
	/* set nodeguarding state */
	(void) setGuardNodeState(canMsg->cobId & 0x7f, PRE_OPERATIONAL
		CO_COMMA_LINE_PARA);
# endif /* CONFIG_NODE_GUARDING */
#endif /* CONFIG_MASTER */

	mGuardErrorInd(canMsg->cobId & 0x7f, CO_BOOT_UP CO_COMMA_REDCY_PARA);

# ifdef CONFIG_NMT_STARTUP_MANAGER
	nmtsEventHandler(NMT_ERRCTRL_BOOTUP_RECEIVED, canMsg->cobId & 0x7f
	    CO_COMMA_REDCY_PARA);
# endif /* CONFIG_NMT_STARTUP_MANAGER */

	return;
    }

    /* no bootup message */

# ifdef CONFIG_NODE_GUARDING
    /* check first for node guarding */
#  ifdef CONFIG_MASTER
    idx = getGuardSlaveIndex((canMsg->cobId & 0x7f) CO_COMMA_LINE_PARA);
    /* index found and guarding active ? */
    if (idx != 0xFF) {
	GUARDING_T	*pSlave;

	pSlave = &GL_PVAR(guardSlaveList)[idx
#ifdef CONFIG_MULT_LINES
		    + GL_ARRAY(co_guardSlaveLineOffs)
#endif /* CONFIG_MULT_LINES */
		    ];
	if ((pSlave->flags & GUARDFLAG_NG_ACTIVE) != 0) {

	    /* Guarding found */
	    guardMsgReceived(idx
#ifdef CONFIG_MULT_LINES
		+ GL_ARRAY(co_guardSlaveLineOffs)
#endif /* CONFIG_MULT_LINES */
		    , canMsg->pData CO_COMMA_LINE_PARA);
	} else {
	    idx = 0xFF;
	}
    }

    if (idx == 0xFF)
#  endif /* CONFIG_MASTER */
# endif /* CONFIG_NODE_GUARDING */

    {
# ifdef CONFIG_HEARTBEAT_CONSUMER
    /* check for known heartbeat consumer */
        idx = getHeartBeatIndex((canMsg->cobId & 0x7f) CO_COMMA_LINE_PARA);
        if (idx != 0xff) {
	    /* HB entry found */
            /* Now check if this a valid heartbeat-msg */
#  ifdef CO_CONFIG_WRONG_MSG_IND_HBC
            if (canMsg->length != PCO_VALID_MESSAGE_LENGTH_HBC ) {
                RET_T ret = CO_OK;
                /* Wrong length of a heartbeat-msg */
                ret = coProtocolErrorInd(CO_PROT_ERR_HBC_LEN, canMsg CO_COMMA_REDCY_PARA);
                if ( ret != CO_OK ) {
                    /* Just do the default behavior */
	            hbMsgReceived(idx, canMsg->pData[0] CO_COMMA_REDCY_PARA);
                }
            } else
#  endif /* CO_CONFIG_WRONG_MSG_IND_HBC */
            {
	        hbMsgReceived(idx, canMsg->pData[0] CO_COMMA_REDCY_PARA);
            }
        }
# endif /* CONFIG_HEARTBEAT_CONSUMER */
    }

    /* nothing found */
    return;

}
#endif /* defined(CONFIG_MASTER) || defined(CONFIG_HEARTBEAT_CONSUMER) */


#if defined(CONFIG_HEARTBEAT_PRODUCER)
/***************************************************************************
*
*++ NMT_HB_TimerPulse - function counts timer pulses for heartbeat
*-- NMT_HB_TimerPulse - Funktion zählt Timerpulse für Heartbeat
*
* \internal
*
*-- Diese Funktion sendet das Heartbeat für einen Heartbeat Producer.
*
* \returns
* nothing
*
*/

void NMT_HB_TimerPulse(
	CO_LINE_PARA_DECL	/* number of CAN line 0..CO_MAX_CAN_LINES-1 */
    )
{
UNSIGNED8 pData[1];		/* temp. transmit buffer */

    /* if heartbeat not enabled, return */
    if ((GL_ARRAY(co_Node).flags & NMTERRFLAG_HB_ACTIVE) == 0u) {
	return;
    }

#ifdef CONFIG_LSS_SLAVE
    /* don't send if state intialiasing */
    if (GL_ARRAY(co_Node).eState == INITIALISING)  {
	return;
    }
#endif /* CONFIG_LSS_SLAVE */

    /* setup new messages */
    pData[0] = (UNSIGNED8)GL_ARRAY(co_Node).eState;

# ifdef CONFIG_FULLCAN
    UPDATE_COB(GL_ARRAY(co_Node).pGuard_COB, pData);
# endif /* CONFIG_FULLCAN */
    (void)TRANSMIT_COB(GL_ARRAY(co_Node).pGuard_COB, pData);

# ifdef CONFIG_REDUNDANCY_SUPPORT
    pData[0] = (UNSIGNED8)GL_VAR(co_redcyNode).eState;

#  ifdef CONFIG_FULLCAN
    UPDATE_COB(GL_VAR(co_redcyNode).pGuard_COB, pData);
#  endif /* CONFIG_FULLCAN */
    (void)TRANSMIT_COB(GL_VAR(co_redcyNode).pGuard_COB, pData);
# endif /* CONFIG_REDUNDANCY_SUPPORT */
}


/***************************************************************************
*
* setHeartbeatProducerTime - set the heartbeat producer time
*
* \internal
*
* This function set up the internal values for heartbeat producer
* It reads the value from od and starts or stops the
* heartbeat producer timer.
*
* If the heartbeat time is > 0 and nodeguarding is enabled,
* nodeguarding will be stopped and heartbeat is started.
*
*
* \returns
* nothing
*
*/

RET_T setHeartBeatProducerTime(
	UNSIGNED16	hbTime	/**< heartbeat time */
	CO_COMMA_LINE_PARA_DECL	/* number of CAN line 0..CO_MAX_CAN_LINES-1 */
    )
{
UNSIGNED32	timerVal;	/* timer value */

    /* if heartbeat isn't initialized, return */
    if ((GL_ARRAY(co_Node).flags & NMTERRFLAG_HB_POSSIBLE) == 0u) {
	return(CO_E_NO_INITIATE);
    }

    /* check, if LG is off */
    if ((GL_ARRAY(co_Node).flags & NMTERRFLAG_LG_ISSET) != 0u)
    {
#  ifdef CONFIG_SLAVE
#   ifdef CONFIG_NODE_GUARDING
        /* for conformance test */
        if (hbTime > 0)
        {
            (void) setLifeTimePara(0, 0 CO_COMMA_LINE_PARA);
        }
        else
#   endif /* CONFIG_NODE_GUARDING */
#  endif /* CONFIG_SLAVE */
        {
            return(CO_OK);
        }
    }

    timerVal = (UNSIGNED32)hbTime * 10u;

    /* if time > 0 enable heartbeat */
    if (timerVal > 0u)  {

	GL_ARRAY(co_Node).flags |= NMTERRFLAG_HB_ACTIVE;

# ifdef CONFIG_REDUNDANCY_SUPPORT
	GL_VAR(co_redcyNode).flags |= NMTERRFLAG_HB_ACTIVE;
# endif /* CONFIG_REDUNDANCY_SUPPORT */

	/* start heartbeat timer */
	(void)changeTimerEvent(&GL_ARRAY(co_Node).timer, timerVal,
		CO_TIMER_TYPE_HB_PROD | CO_TIMER_TYPE_CYCLIC
		CO_COMMA_LINE_PARA);
    } else {

	/* disable heartbeat */
	removeTimerEvent(&GL_ARRAY(co_Node).timer CO_COMMA_LINE_PARA);

	GL_ARRAY(co_Node).flags &= (FLAG_T)~NMTERRFLAG_HB_ACTIVE;

# ifdef CONFIG_REDUNDANCY_SUPPORT
	GL_VAR(co_redcyNode).flags &= (FLAG_T)~NMTERRFLAG_HB_ACTIVE;
# endif /* CONFIG_REDUNDANCY_SUPPORT */
    }

    return(CO_OK);
}
#endif /* CONFIG_HEARTBEAT_PRODUCER) */


#ifdef CONFIG_SLAVE
# ifdef CONFIG_NODE_GUARDING
/***************************************************************************
*
*++ NMT_TimerPulse - function counts timer pulses for node guarding
*-- NMT_TimerPulse - Funktion zählt Timerpulse für Nodeguarding
*
* \internal
*
*++ This function counts timer pulses for node guarding on a slave device.
*++ It must be included in the Timer ISR or in a time triggered
*++ process. The parameter is the timer interval in ms.
*-- Diese Funktion zählt die Timerpulse für das Nodeguarding auf Slave-Geräten.
*
* \returns
* nothing
*
*/

void NMT_NG_TimerPulse(
	CO_LINE_PARA_DECL	/* number of CAN line 0..CO_MAX_CAN_LINES-1 */
    )
{
LOCAL_NODE_T	*pNode;		/* local pointer to node structure */
#  if defined(CONFIG_FULLCAN)
UNSIGNED8	pData[1];	/* temp. transmit buffer */
#  endif /* defined(CONFIG_HEARTBEAT_PRODUCER) || defined(CONFIG_FULLCAN) */

    pNode = &GL_ARRAY(co_Node);

    /* no request from master was received */
    pNode->bSuspendedGuardings++;
    if (pNode->bSuspendedGuardings >= pNode->bLifeTimeFactor) {

#  ifdef CONFIG_NO_ERROR_BEHAVIOR
#  else /* CONFIG_NO_ERROR_BEHAVIOR */
	execCommErrorBehavior(CO_REDCY_PARA);
#  endif /* CONFIG_NO_ERROR_BEHAVIOR */

	/* lifetime is over, disable life guarding */
	if (sGuardErrorInd(CO_LOST_CONNECTION CO_COMMA_LINE_PARA) == 1) {
#  ifdef CONFIG_CO_ERR_LED
	    setCoErrLedState(CO_ERR_LED_NMT_ERROR CO_COMMA_LINE_PARA);
#  endif /* CONFIG_CO_ERR_LED */
	    if (pNode->eState == OPERATIONAL)  {
		setNodeState(PRE_OPERATIONAL CO_COMMA_LINE_PARA);
	    }
	    GL_ARRAY(co_Node).bGuardToggle = 0;
	}

#  ifdef CONFIG_FULLCAN
	pData[0] = (UNSIGNED8)GL_ARRAY(co_pNode)->bGuardToggle
		       | (UNSIGNED8)GL_ARRAY(co_pNode)->eState;
	UPDATE_COB(GL_ARRAY(co_pNode)->pGuard_COB, pData);
#  else /* CONFIG_FULLCAN */
#  endif /* CONFIG_FULLCAN */

	/* disable guarding */
	pNode->flags &= (FLAG_T)~NMTERRFLAG_LG_ACTIVE;
	/* remove timer */
	pNode->timer.timerType &= ~CO_TIMER_TYPE_CYCLIC;

	return;
    }

/* one suspended guarding is generated by the timer resolution
 only the seconded missed guarding will be showed to prevent such an error */
    if (pNode->bSuspendedGuardings > 0) {
	if (sGuardErrorInd(CO_LOST_GUARDING_MSG CO_COMMA_LINE_PARA) == 1) {
#  ifdef CONFIG_CO_ERR_LED
	    setCoErrLedState(CO_ERR_LED_NMT_ERROR CO_COMMA_LINE_PARA);
#  endif /* CONFIG_CO_ERR_LED */
	    if (pNode->eState == OPERATIONAL)  {
		setNodeState(PRE_OPERATIONAL CO_COMMA_LINE_PARA);
	    }
	}
    }
}


/***************************************************************************
*
* setLifeTimePara - set the guarding and life time
*
* \internal
*
* This function set up the internal values for life time monitoring.
* It reads the value from od and starts or stops the
* life time monitoring
*
* If the nodeguard time and life time factor != 0
* and heartbeat is disabled,
* lifeguarding is started
* else is it disabled
*
*
* \returns CO_OK
*	successfull
* \returns CO_E_SRD_NO_RESSOURCE
*	node guardin not possible because heartbeat is active
*	or node is master - no lifetime monitoring allowed
*
*/

RET_T setLifeTimePara(
	UNSIGNED16	lifeTime,	/**< life time */
	UNSIGNED8	factor		/**< life time factor */
	CO_COMMA_LINE_PARA_DECL	/* number of CAN line 0..CO_MAX_CAN_LINES-1 */
    )
{
    /*printf("setLifeTimePara %d %d\n",lifeTime,factor);*/
    /* if lifeguarding isn't initialized, return */
    if ((GL_ARRAY(co_Node).flags & NMTERRFLAG_NG_POSSIBLE) == 0) {
	return(CO_E_NO_INITIATE);
    }

    /* if heartbeat is active, don't anything */
    /*printf("HB active = %d\n",(GL_ARRAY(co_Node).flags & NMTERRFLAG_HB_ACTIVE));*/
    if ((GL_ARRAY(co_Node).flags & NMTERRFLAG_HB_ACTIVE) != 0) {
	return(CO_E_BAD_ERROR_CTRL);
    }

# if defined(CONFIG_MASTER)
    /* if the node is master, no lifetime monitoring is possible */
    if ((GL_ARRAY(co_Node).flags & NMTERRFLAG_MASTER) != 0)  {
	return(CO_E_SRD_NO_RESSOURCE);
    }
# endif /* defined(CONFIG_MASTER) */

    /* disable if lifetime or lifetime factor is zero */
    if ((lifeTime == 0) || (factor == 0)) {
	/* disable guarding */
	GL_ARRAY(co_Node).flags &=
		(FLAG_T)~(NMTERRFLAG_LG_ISSET | NMTERRFLAG_LG_ACTIVE);

	/* stop timer */
	removeTimerEvent(&GL_ARRAY(co_Node).timer CO_COMMA_LINE_PARA);

# ifdef CONFIG_CO_ERR_LED
	/* delete error led event */
	resetCoErrLedState(CO_ERR_LED_NMT_ERROR CO_COMMA_LINE_PARA);
# endif /* CONFIG_CO_ERR_LED */
    } else {
	/* lifetime and guardtime are not 0 */

	/* start life guarding, if its not active */
	GL_ARRAY(co_Node).flags |= NMTERRFLAG_LG_ISSET;

	GL_ARRAY(co_Node).timer.timerVal = (UNSIGNED32)lifeTime * 10;
	GL_ARRAY(co_Node).bLifeTimeFactor = factor;
    }

    return(CO_OK);
}
# endif /* defined(CONFIG_NODE_GUARDING) */
#endif /* CONFIG_SLAVE */


#if (defined(CONFIG_NODE_GUARDING) && defined(CONFIG_SLAVE))	\
  || defined(CONFIG_HEARTBEAT_PRODUCER)
/***************************************************************************
*
* NMT_NodeGuardingMsg - reaction to node guarding message from master
*
* \internal
*
* transmit function was inserted because node guarding didn't work.
*
* First node guard COB must have a toggle bit set to 0.
* The default settings toke place in function
* NMT_CreateNode(). Node guarding COB has to be transferred first
* then you can change the toggle bit. The next problem is to catch
* the status before you send the guarding COB.
* I decide to solve this problem with the changing of the init data of the node.
* init value of bGuardToggle == 1 (ts).
*
* \returns
* nothing
*
*/

void NMT_NodeGuardingMsg(
	CO_LINE_PARA_DECL	/* number of CAN line 0..CO_MAX_CAN_LINES-1 */
     )
{
UNSIGNED8 pData[1];             /* transmit buffer */

    /* if guarding and heartbeat is active - heartbeat has priority */
    if ((GL_ARRAY(co_Node).flags
		& (NMTERRFLAG_NG_POSSIBLE | NMTERRFLAG_HB_ACTIVE))
		!= NMTERRFLAG_NG_POSSIBLE)  {
	/* nodeguarding is not initialized or hb is active */

	/* workaround for conformance test */
	pData[0] = (UNSIGNED8)GL_ARRAY(co_Node).eState;
	(void)TRANSMIT_COB(GL_ARRAY(co_Node).pGuard_COB, pData);

	return;
    }

# if (defined(CONFIG_NODE_GUARDING) && defined(CONFIG_SLAVE))
    GL_ARRAY(co_Node).bSuspendedGuardings = 0;

    /* inform application about start of guarding */
    if ((GL_ARRAY(co_Node).flags & NMTERRFLAG_LG_ACTIVE) == 0) {
	/* if lifeguarding is enabled */
	if ((GL_ARRAY(co_Node).flags & NMTERRFLAG_LG_ISSET) != 0) {
	    (void) sGuardErrorInd(CO_GUARDING_STARTED CO_COMMA_LINE_PARA);

	    /* first node guarding request set guarding active */
	    GL_ARRAY(co_Node).flags |= NMTERRFLAG_LG_ACTIVE;
	}

#  ifdef CONFIG_CO_ERR_LED
	resetCoErrLedState(CO_ERR_LED_NMT_ERROR CO_COMMA_LINE_PARA);
#  endif /* CONFIG_CO_ERR_LED */
    }

    /* if lifeguarding is enabled */
    if ((GL_ARRAY(co_Node).flags & NMTERRFLAG_LG_ISSET) != 0) {
	/* restart timer */
	(void)addTimerEvent(&GL_ARRAY(co_Node).timer,
	    GL_ARRAY(co_Node).timer.timerVal,
	    CO_TIMER_TYPE_NG_SLAVE | CO_TIMER_TYPE_CYCLIC
	    CO_COMMA_LINE_PARA);
    }

    pData[0] = GL_ARRAY(co_Node).bGuardToggle
	       | (UNSIGNED8)GL_ARRAY(co_Node).eState;
    GL_ARRAY(co_Node).bGuardToggle = GL_ARRAY(co_Node).bGuardToggle ^ 0x80;

#  ifdef CONFIG_FULLCAN
    UPDATE_COB(GL_ARRAY(co_pNode)->pGuard_COB, pData);
#  else /* CONFIG_FULLCAN */
    (void)TRANSMIT_COB(GL_ARRAY(co_Node).pGuard_COB, pData);
#  endif /* CONFIG_FULLCAN */
# endif /* defined(CONFIG_NODE_GUARDING) */
}
#endif /* (defined(CONFIG_NODE_GUARDING) && defined(CONFIG_SLAVE))	*/


#if defined(CONFIG_HEARTBEAT_CONSUMER) || (defined(CONFIG_MASTER) && defined(CONFIG_NODE_GUARDING))
/***************************************************************************
*
* nmtErrNodeFailed - handle nmterr node states
*
* \internal
* This function saves the state of all nodes,
* they are monitored and they are nmterr is active
*
* Parameter state means:
*	0 - nmterror configured
*	1 - nmterror started
*	2 - nmterror failed
*	3 - nmterror 3 heartbeats received
*/

void nmtErrNodeFailed(
	UNSIGNED8	nodeId,		/* node id */
	UNSIGNED8	state		/* failed, monitoring...*/
	CO_COMMA_REDCY_PARA_DECL
    )
{
UNSIGNED8	index;
UNSIGNED8	bitpos;
#ifdef CONFIG_CO_ERR_LED
UNSIGNED8	start;
#endif /* CONFIG_CO_ERR_LED */

    if (nodeId == 0u)  {
	return;
    }

    index = (nodeId - 1u) >> 3u;
    bitpos = (UNSIGNED8)((nodeId - 1u) % 8u);

    /* node not longer monitored ? */
    if (state == NMTERROR_STATE_UNCONFIG)  {
#ifdef CONFIG_REDUNDANCY_SUPPORT
	GL_VAR(nmtErrConfig)[index][canLine] &= ~(1u << bitpos);
	GL_VAR(nmtErrFailed)[index][canLine] &= ~(1u << bitpos);
	GL_VAR(nmtErrStarted)[index][canLine] &= ~(1u << bitpos);
#else /* CONFIG_REDUNDANCY_SUPPORT */
	GL_ARRAY(nmtErrConfig[index]) &= ~(1u << bitpos);
	GL_ARRAY(nmtErrFailed[index]) &= ~(1u << bitpos);
	GL_ARRAY(nmtErrStarted[index]) &= ~(1u << bitpos);
#endif /* CONFIG_REDUNDANCY_SUPPORT */
    } else {
#ifdef CONFIG_REDUNDANCY_SUPPORT
	GL_ARRAY(nmtErrConfig[index])[canLine] |= (1u << bitpos);
#else /* CONFIG_REDUNDANCY_SUPPORT */
	GL_ARRAY(nmtErrConfig[index]) |= (1u << bitpos);
#endif /* CONFIG_REDUNDANCY_SUPPORT */
    }

    if (state == NMTERROR_STATE_CONFIG)  {
#ifdef CONFIG_REDUNDANCY_SUPPORT
	GL_VAR(nmtErrStarted)[index][canLine] &= ~(1u << bitpos);
	GL_VAR(nmtErrFailed)[index][canLine] &= ~(1u << bitpos);
#else /* CONFIG_REDUNDANCY_SUPPORT */
	GL_ARRAY(nmtErrStarted)[index] &= ~(1u << bitpos);
	GL_ARRAY(nmtErrFailed[index]) &= ~(1u << bitpos);
#endif /* CONFIG_REDUNDANCY_SUPPORT */
    }
    if (state == NMTERROR_STATE_FAILED)  {
#ifdef CONFIG_REDUNDANCY_SUPPORT
	GL_VAR(nmtErrFailed)[index][canLine] |= (1u << bitpos);
	GL_ARRAY(nmtErr3HBok)[index] &= ~(1u << bitpos);
#else /* CONFIG_REDUNDANCY_SUPPORT */
	GL_ARRAY(nmtErrFailed)[index] |= (1u << bitpos);
#endif /* CONFIG_REDUNDANCY_SUPPORT */
    }
    if (state == NMTERROR_STATE_STARTED)  {
#ifdef CONFIG_REDUNDANCY_SUPPORT
	GL_VAR(nmtErrFailed)[index][canLine] &= ~(1u << bitpos);
	GL_VAR(nmtErrStarted)[index][canLine] |= (1u << bitpos);
	GL_ARRAY(nmtErr3HBok)[index] &= ~(1u << bitpos);
#else /* CONFIG_REDUNDANCY_SUPPORT */
	GL_ARRAY(nmtErrFailed)[index] &= ~(1u << bitpos);
	GL_ARRAY(nmtErrStarted)[index] |= (1u << bitpos);
#endif /* CONFIG_REDUNDANCY_SUPPORT */
    }
#ifdef CONFIG_REDUNDANCY_SUPPORT
    if (state == NMTERROR_STATE_3HB_OK)  {
	GL_ARRAY(nmtErr3HBok)[index] |= (1u << bitpos);
    }
#endif /* CONFIG_REDUNDANCY_SUPPORT */

#ifdef CONFIG_CO_ERR_LED
    /* check for new state and set led */
    bitpos = 0;
    for (start = 0; start < NMTERR_MAX_INDEX; start ++)  {
# ifdef CONFIG_REDUNDANCY_SUPPORT
	if (GL_VAR(nmtErrFailed)[start][canLine] != 0)  {
# else /* CONFIG_REDUNDANCY_SUPPORT */
	if (GL_ARRAY(nmtErrFailed)[start] != 0)  {
# endif /* CONFIG_REDUNDANCY_SUPPORT */
	    bitpos++;
	}
    }

    /* set led */
    if (bitpos != 0)  {
	setCoErrLedState(CO_ERR_LED_NMT_ERROR CO_COMMA_REDCY_PARA);
    } else {
	resetCoErrLedState(CO_ERR_LED_NMT_ERROR CO_COMMA_REDCY_PARA);
    }
#endif /* CONFIG_CO_ERR_LED */
}


/*******************************************************************
*
* initNmterrVars - init all nmterr variables
*
* \internal
*
* RETURNS
* \retval nthing
*
*/

void initNmtErrVars(
	CO_LINE_PARA_DECL
    )
{
UNSIGNED8	i;

    /* clear global variables (some compilers doesn't clear global variables */
#ifdef CONFIG_CLEAR_CO_GLOBAL_VARS
#endif /* CONFIG_CLEAR_CO_GLOBAL_VARS */


    /* reset failed nodes */
    for (i = 0; i < NMTERR_MAX_INDEX; i++)  {
# ifdef CONFIG_REDUNDANCY_SUPPORT
	GL_VAR(nmtErrFailed)[i][0] = 0;
	GL_VAR(nmtErrStarted)[i][0] = 0;
	GL_VAR(nmtErrConfig)[i][0] = 0;
	GL_VAR(nmtErrFailed)[i][1] = 0;
	GL_VAR(nmtErrStarted)[i][1] = 0;
	GL_VAR(nmtErrConfig)[i][1] = 0;
	GL_VAR(nmtErr3HBok)[i] = 0;
#  ifdef CONFIG_MARITIME_SUPPORT
	GL_VAR(nmtErrRedundancy)[i] = 0;
#  endif /* CONFIG_MARITIME_SUPPORT */
# else /* CONFIG_REDUNDANCY_SUPPORT */
	GL_ARRAY(nmtErrFailed[i]) = 0;
	GL_ARRAY(nmtErrStarted[i]) = 0;
	GL_ARRAY(nmtErrConfig[i]) = 0;
# endif /* CONFIG_REDUNDANCY_SUPPORT */
    }

}
# endif /* defined(CONFIG_HEARTBEAT_CONSUMER) || (defined(CONFIG_MASTER) && defined(CONFIG_NODE_GUARDING)) */
/*______________________________________________________________________EOF_*/
