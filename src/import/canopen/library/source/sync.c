/*
 *++ sync - functions for the Synchronisation Object (SYNC) handling
 *-- sync - Funktionen für das Synchronisation Object (SYNC)
 *
 * Copyright (c) 1997-2017 port GmbH Halle (Saale)
 *------------------------------------------------------------------
 */


/****************************************************************************/
/**
*  \file sync.c
*++ Functions for the Synchronisation Object (SYNC) handling
*-- Funktionen für das Synchronisation Object (SYNC)
*  \author port GmbH Halle (Saale)
*
*++ This module contains the functions for SYNC Object handling for
*++ a SYNC producer or consumer.
*-- Diese Modul beinhaltet Funktionen für einen SYNC Producer
*-- oder Consumer.
*
*/


/* header of standard C - libraries */

#include <stdio.h>
#include <string.h>

/* project headers */

#include <cal_conf.h>
#include <co_odidx.h>
#include <co_cobid.h>
#include <co_flag.h>
#include "sync.h"
#include "access.h"
#include "nmt.h"
#include "drv.h"
#include <co_util.h>

#if defined(CONFIG_PDO_CONSUMER) || defined(CONFIG_PDO_PRODUCER)
# ifdef CONFIG_PDO_SYNC_START_VALUE
#include "pdo.h"
# endif /* CONFIG_PDO_SYNC_START_VALUE */
#endif /* defined(CONFIG_PDO_CONSUMER) || defined(CONFIG_PDO_PRODUCER) */

#ifdef CONFIG_REDUNDANCY_SUPPORT
#include "reduncy.h"
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
#if defined(CONFIG_SYNC_PRODUCER)
static RET_T activateSync(CO_LINE_PARA_DECL);
static void deActivateSync(CO_LINE_PARA_DECL);
#endif /* defined(CONFIG_SYNC_PRODUCER) */

/* external variables
---------------------------------------------------------------------------*/

/* global variables
---------------------------------------------------------------------------*/
#ifdef CONFIG_NO_GLOBAL_VARS
#else /* CONFIG_NO_GLOBAL_VARS */
# if defined(CONFIG_SYNC_PRODUCER) || defined(CONFIG_SYNC_CONSUMER)
CO_LIB_UNINIT_VAR SYNC_T		co_Sync CO_LINE_PARA_ARRAY_DEF;		/* SYNC Object */
CO_LIB_UNINIT_VAR UNSIGNED8	co_syncCnt CO_LINE_PARA_ARRAY_DEF;	/* SYNC counter */
# endif /* defined(CONFIG_SYNC_PRODUCER) || defined(CONFIG_SYNC_CONSUMER) */
#endif /* CONFIG_NO_GLOBAL_VARS */


/* local defined variables
---------------------------------------------------------------------------*/
#ifdef CONFIG_RCS_IDENT
CO_LIB_INIT_VAR static char _rcsid[] = "$Id: sync.c,v 2.43 2016/09/26 11:16:09 rli Exp $";
#endif /* CONFIG_RCS_IDENT */

#if defined(CONFIG_SYNC_PRODUCER) || defined(CONFIG_SYNC_CONSUMER)
/****************************************************************************/
/**
*++ \brief defineSync - define a SYNC Object
*-- \brief defineSync - definiert ein SYNC - Objekt
*
*++ This function defines a Synchronization Object (SYNC)
*++ for transmitting (PRODUCER) or receiving (CONSUMER).
*++ according to the DS 301.
*++ The SYNC service can be used to exchange synchronous PDOs
*++ or start other processes at the network nodes.
*-- Diese Funktion initialisiert das Synchronisationsobjekt (SYNC)
*-- als Sende- oder Empfangsobjekt entsprechend dem DS 301.
*-- Mit dem SYNC-Dienst können PDOs synchron übertragen,
*-- aber auch zyklische Vorgänge auf den Netzwerkteilnehmern
*-- gestartet werden.
*
*++ This function is actualisizing the COB-ID SYNC entry (index 0x1005)
*++ at the object dictionary depending the given mode (Producer/Consumer).
*++ Switching to the other mode is always possible
*++ by setting Cob-Id entry (1005h, Bit 30) at the object dictionary.
*-- Diese Funktion aktualisiert auch den Eintrag für die COB-ID SYNC
*-- (index 1005h) im Objektverzeichnis
*-- entsprechend dem angeforderten Mode (Producer/Consumer).
*-- Ein Umschalten in den anderen Mode ist jederzeit
*-- über das Setzen der COB-Id (Objekt 1005h, Bit 30) möglich.
*++ The SYNC producer mode can only be activated
*++ if the entry
*-- Der SYNC-Producer Mode kann nur aktiviert werden,
*-- wenn auch im Eintrag
* Communication Cycle Period (index 1006h)
*++ is set greater than 0.
*-- eine Zeit > 0 eingetragen ist.
*++ The COB-Id can be changed any time at the SYNC consumer mode,
*++ at the producer mode it must be inactive.
*-- Die COB-Id kann im Consumer Mode jederzeit,
*-- im Producer-Mode nur bei inaktivem Producer
*-- modifiziert werden.
*
*++ The SYNC Producer can be also enabled/disabled
*++ by the functions startSyncReq() or StopSyncReq().
*-- Das Ein- bzw. Ausschalten des SYNC-Producers
*-- kann auch die Funktion
*-- startSyncReq() bzw. StopSyncReq()
*-- genutzt werden.
*
*++ \b Example
*-- \b Beispiel
* \code
* RET_T retVal;
*
* // consumer
* retVal = defineSync(CONSUMER);
*
* // producer
* retVal = defineSync(PRODUCER);
* \endcode
*
* \retval CO_OK
*++ success
*-- Erfolg
* \retval CO_E_MEM
*++ memory allocation fault
*-- Speicherallozierungsfehler
* \retval CO_E_NO_ACCESS
*++ no access to object dictionary at index 0x1005
*-- Kein Zugriff auf Objektverzeichnis mit Index 0x1005
*
*/

RET_T defineSync(
	CO_USER_T kindOfUse   /**< kind of using (PRODUCER/CONSUMER) */
	CO_COMMA_LINE_PARA_DECL	/**< number of CAN line 0..CO_MAX_CAN_LINES-1 */
    )
{
UNSIGNED32 	*pCobId;			/* pointer to COB-ID */
COB_KIND_T      kind;				/* kind of COB RX/TX */
RET_T		retVal;				/* return value */
UNSIGNED32      size;				/* size of object */
UNSIGNED8	*pU8;

    /* get sync cobid */
    if (getObjAddr(SYNC_COB_ID_INDEX, 0, &pU8, &size CO_COMMA_LINE_PARA)
		!= CO_OK) {
	return CO_E_NO_ACCESS;
    }
    pCobId = (UNSIGNED32 *)pU8;

    if (kindOfUse == PRODUCER) {
# ifdef CONFIG_SYNC_PRODUCER
	kind = CO_COB_SYNC_PROD;
# else /* CONFIG_SYNC_PRODUCER */
	return(CO_E_TRANS_TYPE);
# endif /* CONFIG_SYNC_PRODUCER */
    } else {
	kind = CO_COB_SYNC_CONS;
	*pCobId &= ~SYNC_PRODUCER_BIT;
    }

    /* define COB */
    if ((GL_ARRAY(co_Sync).flags & CO_SYNC_FLAG_INIT) == 0)  {
	GL_ARRAY(co_Sync).pCOB = DEFINE_COB(kind, 0 CO_COMMA_LINE_PARA);

	if (GL_ARRAY(co_Sync).pCOB == NULL) {
	    return(CO_E_NO_DATABASE);
	}
    }

    retVal = initSync(CO_LINE_PARA);

    return(retVal);
}


/****************************************************************************/
/**
*++ initSync - init a SYNC Object
*-- initSync - init ein SYNC - Objekt
*
*++ This function defines a Synchronization Object (SYNC)
*++ for transmitting (PRODUCER) or receiving (CONSUMER).
*++ according to the DS 301.
*++ This function is called at defineSync and resetComm
*-- Diese Funktion initialisiert das Synchronisationsobjekt (SYNC)
*-- als Sende- oder Empfangsobjekt entsprechend dem DS 301.
*-- Diese Funktion wird bei defineSync und ResetComm aufgerufen
*
* \retval CO_OK
*++ success
*-- Erfolg
* \retval CO_E_MEM
*++ memory allocation fault
*-- Speicherallozierungsfehler
* \retval CO_E_NO_ACCESS
*++ no access to object dictionary at index 0x1005
*-- Kein Zugriff auf Objektverzeichnis mit Index 0x1005
*
*/

RET_T initSync(
	CO_LINE_PARA_DECL	/* number of CAN line 0..CO_MAX_CAN_LINES-1 */
    )
{
UNSIGNED32 	cobId;			/* COB-ID */
RET_T		retVal;				/* return value */
UNSIGNED32      size;				/* size of object */
# ifdef CONFIG_SYNC_PRODUCER
UNSIGNED32  	syncTime;			/* temp buffer */
# endif /* CONFIG_SYNC_PRODUCER */
# ifdef CONFIG_SYNC_COUNTER
UNSIGNED8	syncCnt = 0;
# endif /* CONFIG_SYNC_COUNTER */

    /* get sync cobid */
    if (getObjEntry(SYNC_COB_ID_INDEX, 0, (UNSIGNED8 *)&cobId, &size, CO_TRUE
		CO_COMMA_LINE_PARA) != CO_OK) {
	return CO_E_NO_ACCESS;
    }

    if (GL_ARRAY(co_Sync).pCOB == NULL) {
	return(CO_E_NO_DATABASE);
    }

    GL_ARRAY(co_Sync).flags = CO_SYNC_FLAG_INIT;
    GL_ARRAY(co_syncCnt) = 1;	/* SYNC counter */

# ifdef CONFIG_SYNC_COUNTER
    /* optional sync counter */
    if (getObjEntry(SYNC_COUNTER_INDEX, 0, (UNSIGNED8 *)&syncCnt, &size,
		CO_TRUE CO_COMMA_LINE_PARA) != CO_OK) {
	syncCnt = 0;
    }
    /* ensure that timer is off */
    GL_ARRAY(co_Sync).timer.timerVal = 0;
    retVal = setSyncCounter(syncCnt CO_COMMA_LINE_PARA);
    if (retVal != CO_OK) {
	return(retVal);
    }
# endif /* CONFIG_SYNC_COUNTER */

    /* sync not active */
# if defined(CONFIG_SYNC_PRODUCER)
    /* get sync cycle periode */
    if (getObjEntry(COMM_CYCLE_INDEX, 0, (UNSIGNED8 *)&syncTime, &size,
		CO_TRUE CO_COMMA_LINE_PARA) != CO_OK) {
	GL_ARRAY(co_Sync).flags &= (FLAG_T)~CO_SYNC_FLAG_INIT;
	return CO_E_NO_ACCESS;
    }

    /* if (kindOfUse == PRODUCER) { */
    /* if ((cobId & SYNC_PRODUCER_BIT) != 0)  { */

    retVal = setSyncTimePara(syncTime CO_COMMA_LINE_PARA);
    if (retVal != CO_OK)
    {
        GL_ARRAY(co_Sync).flags &= (FLAG_T)~CO_SYNC_FLAG_INIT;
        return(retVal);
    }

    GL_ARRAY(co_Sync).flags |= CO_SYNC_FLAG_ENABLED;

    /* } */
# endif /* defined(CONFIG_SYNC_PRODUCER) */

    retVal = setSyncCobId(cobId CO_COMMA_LINE_PARA);

# if defined(CONFIG_SYNC_PRODUCER)
    if ((cobId & SYNC_PRODUCER_BIT) == 0)  {
	(void) stopSyncReq(CO_LINE_PARA);
    }
# endif /* defined(CONFIG_SYNC_PRODUCER) */

    return(retVal);
}


/****************************************************************************/
/*
*++ \brief setSyncCobId - set the sync cob-id for producer
*-- \brief setSyncCobId - setzt die Sync COB-Id für Producer
*
* \internal
*
*++ This function sets the sync cob-id.
*++ It checks before the allowed states for sync
*++ (Sync consumers can't produce sync...)
*
* \retval CO_OK
*++ success
*-- Erfolg
* \retval CO_E_TRANSTYPE
*++ bad transmission mode requested
*-- nicht erlaubter Transmission Mode angefordert
*
*/
RET_T setSyncCobId(
	UNSIGNED32	cobId		/* pointer to cob-id */
	CO_COMMA_LINE_PARA_DECL	/* number of CAN line 0..CO_MAX_CAN_LINES-1 */
    )
{
RET_T	retVal = CO_OK;

    if ((GL_ARRAY(co_Sync).flags & CO_SYNC_FLAG_INIT) == 0)  {
	return(CO_E_NOT_EXIST);
    }

    /* check for sync producer */
    if ((cobId & SYNC_PRODUCER_BIT) != 0)  {
# ifdef CONFIG_SYNC_PRODUCER
	/* change cob-id is only allowed if producer isn't set */
	if ((GL_ARRAY(co_Sync).flags & CO_SYNC_FLAG_ACTIVE)
		!= 0) {
	    /* sync producer is active */
	    return(CO_E_TRANS_TYPE);
	}

	/* start sync */
	if (startSyncReq(CO_LINE_PARA) != CO_OK)  {
	    return(CO_E_TRANS_TYPE);
	}
# else /* CONFIG_SYNC_PRODUCER */
	return(CO_E_TRANS_TYPE);
# endif /* CONFIG_SYNC_PRODUCER */

    } else {
# ifdef CONFIG_SYNC_PRODUCER

	/* stop sync */
	if (stopSyncReq(CO_LINE_PARA) != CO_OK)  {
	    return(CO_E_TRANS_TYPE);
	}
# else /* CONFIG_SYNC_PRODUCER */
#  ifndef CO_CONFIG_DONT_CHECK_RESTRICTED_COBID
        retVal = coCheckRestrictedCobId(0x0000, cobId CO_COMMA_LINE_PARA); /* SYNC checks all restricted cobIds thus index does not matter */
        if (retVal != CO_OK)  {
            return(retVal);
        }
#  endif /* CO_CONFIG_DONT_CHECK_RESTRICTED_COBID */

	retVal = SET_COB_ID(GL_ARRAY(co_Sync).pCOB, (cobId & CAN_BIT_ID_MASK),
		CO_COB_SYNC_CONS);
	if (retVal != CO_OK)  {
	    return(retVal);
	}
# endif /* CONFIG_SYNC_PRODUCER */
    }

    return(retVal);
}

#endif /* SYNC CONSUMER + PRODUCER */


#if defined(CONFIG_SYNC_PRODUCER)

/****************************************************************************/
/**
*++ \brief startSyncReq - starts the transmission of SYNC to the consumer(s)
*-- \brief startSyncReq - startet das Senden der SYNC zu den SYNC-Consumern
*
*++ This function enables SYNC messages to be sent
*++ and sets the PRODUCER-Bit at the index 1005h at the object dictionary.
*++ If the producer time at the index 0x1006h is zero,
*++ the function doesn't start the SYNC producing
*++ and returns an error.
*-- Diese Funktion schaltet das Senden von SYNC Nachrichten ein.
*-- Dabei wird auch das Producer-Bit im Objekt COB-ID SYNC (Index 1005h)
*-- im Objektverzeichnis gesetzt.
*-- Wenn die eingestellte Producer-Zeit (index 1006h) null ist,
*-- kann der SYNC-Producer nicht aktiviert werden
*-- und die Funktion kehrt mit einem Fehler zurück.
*
*
* \retval CO_OK
*++ success
*-- Erfolg
* \retval CO_E_NO_ACCESS
*++ no access to object dictionary at index 0x1005
*-- Kein Zugriff auf Objektverzeichnis mit Index 0x1005
*
*/
RET_T startSyncReq(
	CO_LINE_PARA_DECL	/**< number of CAN line 0..CO_MAX_CAN_LINES-1 */
    )
{
UNSIGNED32	*pCobId;		/* pointer to cob-id */
UNSIGNED32	size;			/* temporary variable */
RET_T		retVal;			/* return value */
UNSIGNED8	*pU8;

    /* check for initialization */
    if ((GL_ARRAY(co_Sync).flags & CO_SYNC_FLAG_INIT) == 0)  {
	return(CO_E_NOT_EXIST);
    }

    /* get sync cob-id address */
    if (getObjAddr(SYNC_COB_ID_INDEX, 0u, &pU8, &size CO_COMMA_LINE_PARA)
		!= CO_OK)  {
	return CO_E_NO_ACCESS;
    }
    pCobId = (UNSIGNED32 *)pU8;

    retVal = activateSync(CO_LINE_PARA);
    if (retVal != CO_OK)  {
	return(retVal);
    }

    /* set cob as producer and cob-id */
    retVal = SET_COB_ID(GL_ARRAY(co_Sync).pCOB, (*pCobId & CAN_BIT_ID_MASK),
	    CO_COB_SYNC_PROD);
    if (retVal != CO_OK)  {
	deActivateSync(CO_LINE_PARA);
	return(retVal);
    }

    /* set producer bit */
    *pCobId |= SYNC_PRODUCER_BIT;
    GL_ARRAY(co_Sync).flags |= CO_SYNC_FLAG_ENABLED;
    GL_ARRAY(co_syncCnt) = 1;	/* reset SYNC counter */

    return(retVal);
}


/****************************************************************************/
/**
*++ \brief stopSyncReq - stop the transmission of SYNC to the consumer(s)
*-- \brief stopSyncReq - stopt das Senden des SYNC
*
*++ This function disables the transmission of SYNC messages
*++ and resets the PRODUCER-Bit at the index 1005h at the object dictionary.
*-- Diese Funktion stellt das Senden des SYNC Telegrammes ein.
*-- Dabei wird auch das Producer-Bit im Objekt COB-ID SYNC (Index 1005h)
*-- im Objektverzeichnis gelöscht.
*
* \retval CO_OK
*++ success
*-- Erfolg
*
*/

RET_T stopSyncReq(
	CO_LINE_PARA_DECL	/**< number of CAN line 0..CO_MAX_CAN_LINES-1 */
    )
{
UNSIGNED32	*pCobId;		/* pointer to cob-id */
UNSIGNED32	size;			/* object size */
RET_T		retVal;
UNSIGNED8	*pU8;

    /* check for initialization */
    if ((GL_ARRAY(co_Sync).flags & CO_SYNC_FLAG_INIT) == 0)  {
	return(CO_E_NOT_EXIST);
    }

    /* check, if sync producer is enabled */
    if (getObjAddr(SYNC_COB_ID_INDEX, 0u, &pU8, &size CO_COMMA_LINE_PARA)
		!= CO_OK)  {
	return CO_E_NO_ACCESS;
    }
    pCobId = (UNSIGNED32 *)pU8;

    retVal = SET_COB_ID(GL_ARRAY(co_Sync).pCOB, (*pCobId & CAN_BIT_ID_MASK),
		CO_COB_SYNC_CONS);
    if (retVal != CO_OK)  {
	return(retVal);
    }

    /* reset producer bit */
    *pCobId &= ~SYNC_PRODUCER_BIT;

    GL_ARRAY(co_Sync).flags &= (FLAG_T)~CO_SYNC_FLAG_ENABLED;

    deActivateSync(CO_LINE_PARA);
    return(CO_OK);

}


/****************************************************************************/
/*
*++ \brief setSyncTimePara - set the sync mer paraid
*-- \brief setSyncTimePara - setzt die Sync Timer Wert
*
* \internal
*
*++ This function sets the sync timer value.
*++ if the sync is enabled, it starts it immediately.
*
* \retval CO_OK
*++ success
*-- Erfolg
* \retval CO_E_TRANSTYPE
*++ bad transmission mode requested
*-- nicht erlaubter Transmission Mode angefordert
*
*/
RET_T setSyncTimePara(
	UNSIGNED32	timeVal		/* new time value */
	CO_COMMA_LINE_PARA_DECL	/* number of CAN line 0..CO_MAX_CAN_LINES-1 */
    )
{
RET_T ret = CO_OK;

    /* save the new value */
    GL_ARRAY(co_Sync).timer.timerVal = timeVal / 100;

    /* if sync initialised */
    if ((GL_ARRAY(co_Sync).flags & CO_SYNC_FLAG_INIT) == 0) {
	return(CO_OK);
    }

    /* if sync producer is enabled, set the new value and start sync */
    if ((GL_ARRAY(co_Sync).flags & CO_SYNC_FLAG_ENABLED) != 0) {
	if (timeVal != 0)  {
	    /* start sync */
	    ret = activateSync(CO_LINE_PARA);
	} else {
	    /* stop sync */
	    deActivateSync(CO_LINE_PARA);
	}
    }

    return(ret);
}


/*******************************************************************/
/*
*++ activateSync - activate SYNC transmission
*-- activateSync - aktiviert SYNC Senden
*
* \internal
*
*++ This function activate the SYNC message transmission.
*-- Diese Funktion aktiviert das Senden der Synchronisationsnachricht (SYNC).
*
* \return
*++ RET_T
*-- RET_T
*
*/

static RET_T activateSync(
	CO_LINE_PARA_DECL	/**< number of CAN line 0..CO_MAX_CAN_LINES-1 */
    )
{
RET_T retVal = CO_OK;
# ifdef CONFIG_REDUNDANCY_SUPPORT
UNSIGNED8	canLine;

    canLine = GL_VAR(co_redcyActiveLine);
# endif /* CONFIG_REDUNDANCY_SUPPORT */

    if (addTimerEvent(&GL_ARRAY(co_Sync).timer, GL_ARRAY(co_Sync).timer.timerVal,
	CO_TIMER_TYPE_SYNC | CO_TIMER_TYPE_CYCLIC CO_COMMA_LINE_PARA)
	    != 0)  {
	retVal = CO_E_RANGE;
    } else {

        GL_ARRAY(co_Sync).flags |= CO_SYNC_FLAG_ACTIVE;

        RESET_COLIB_FLAG(COFLAG_SYNC_RECEIVED);
    }
    return(retVal);
}


/*******************************************************************/
/*
*++ deActivateSync - deactivate SYNC transmission
*-- deActivateSync - deaktiviert SYNC Senden
*
* \internal
*
*++ This function deactivate the SYNC message transmission.
*-- Diese Funktion deaktiviert das Senden der Synchronisationsnachricht (SYNC).
*
* \return
*++ RET_T
*-- RET_T
*
*/

static void deActivateSync(
	CO_LINE_PARA_DECL	/**< number of CAN line 0..CO_MAX_CAN_LINES-1 */
    )
{
# ifdef CONFIG_REDUNDANCY_SUPPORT
UNSIGNED8	canLine;
# endif /* CONFIG_REDUNDANCY_SUPPORT */


    removeTimerEvent(&GL_ARRAY(co_Sync).timer CO_COMMA_LINE_PARA);
    GL_ARRAY(co_Sync).flags &= (FLAG_T)~CO_SYNC_FLAG_ACTIVE;

# ifdef CONFIG_REDUNDANCY_SUPPORT
    canLine = CAN_LINE0;
    RESET_COLIB_FLAG(COFLAG_SYNC_RECEIVED);
    canLine = CAN_LINE1;
    RESET_COLIB_FLAG(COFLAG_SYNC_RECEIVED);
# else /* CONFIG_REDUNDANCY_SUPPORT */
    RESET_COLIB_FLAG(COFLAG_SYNC_RECEIVED);
# endif /* CONFIG_REDUNDANCY_SUPPORT */
}


# ifdef CONFIG_SYNC_COUNTER
/*******************************************************************/
/*
*++ resetSyncCounter - reset the optional sync counter
*-- resetSyncCounter - setzt den optionalen SYNC-Counter zurück
*
* \internal
*
*++ This function resets the optional sync counter
*
* \return
*++ RET_T
*-- RET_T
*
*/
void resetSyncCounter(
	CO_REDCY_PARA_DECL	/* number of CAN line 0..CO_MAX_CAN_LINES-1 */
    )
{
#  ifdef CONFIG_REDUNDANCY_SUPPORT
    /* reset sync counter only for active line allowed */
    if (canLine != GL_VAR(co_redcyActiveLine))  {
	return;
    }
#  endif /* CONFIG_REDUNDANCY_SUPPORT */

    GL_ARRAY(co_syncCnt) = 1;

    return;
}
# endif /* CONFIG_SYNC_COUNTER */


/*******************************************************************/
/*
*++ writeSyncReq - sends a SYNC to the consumer(s)
*-- writeSyncReq - sendet ein SYNC zu dem(n) Konsumenten(n)
*
* \internal
*
*++ This function generates the SYNC message.
*-- Diese Funktion generiert eine Synchronisationsnachricht (SYNC).
*
* \return
*++ nothing
*-- nichts
*
*/

void writeSyncReq(
	CO_LINE_PARA_DECL	/* number of CAN line 0..CO_MAX_CAN_LINES-1 */
    )
{
# ifdef CO_CONFIG_SYNC_SEND_IND
RET_T retVal = CO_OK;
# endif /* CO_CONFIG_SYNC_SEND_IND */
# ifdef CONFIG_REDUNDANCY_SUPPORT
UNSIGNED8	canLine;
# else /* CONFIG_REDUNDANCY_SUPPORT */
NODE_STATE_T	actState;
# endif /* CONFIG_REDUNDANCY_SUPPORT */

    /* check for PREOP/OPER */
#ifdef CONFIG_REDUNDANCY_SUPPORT
#else /* CONFIG_REDUNDANCY_SUPPORT */
    actState = GL_ARRAY(co_Node).eState;

    if ((actState != OPERATIONAL) && (actState != PRE_OPERATIONAL)) {
	return;
    }
#endif /* CONFIG_REDUNDANCY_SUPPORT */

# ifdef CO_CONFIG_SYNC_SEND_IND
    retVal = TRANSMIT_COB(GL_ARRAY(co_Sync).pCOB, &GL_ARRAY(co_syncCnt));
    sendSyncInd( GL_ARRAY(co_syncCnt), retVal CO_COMMA_LINE_PARA);
# else /*CO_CONFIG_SYNC_SEND_IND*/
    (void)TRANSMIT_COB(GL_ARRAY(co_Sync).pCOB, &GL_ARRAY(co_syncCnt));
# endif /*CO_CONFIG_SYNC_SEND_IND*/

# ifdef CONFIG_SYNC_COUNTER
    /* increment counter */
    GL_ARRAY(co_syncCnt) ++;
    if (GL_ARRAY(co_syncCnt) > GL_ARRAY(co_Sync).maxCounter)  {
	GL_ARRAY(co_syncCnt) = 1;
    }
# endif /* CONFIG_SYNC_COUNTER */

# ifdef CONFIG_REDUNDANCY_SUPPORT
    canLine = GL_VAR(co_redcyActiveLine);
# endif /* CONFIG_REDUNDANCY_SUPPORT */

# ifdef CONFIG_SYNC_PRE_CMD
    syncPreCommand(CO_LINE_PARA);
# endif /* CONFIG_SYNC_PRE_CMD */

    /* activates local synchronous PDOs */
    SET_COLIB_FLAG(COFLAG_SYNC_RECEIVED);
}

#endif /* CONFIG_SYNC_PRODUCER */


#if defined(CONFIG_SYNC_PRODUCER) || defined(CONFIG_SYNC_CONSUMER)
# ifdef CONFIG_SYNC_COUNTER
/*******************************************************************/
/*
*++ setSyncCounter - set the optional sync counter
*-- setSyncCounter - setzt den optionalen SYNC-Counter
*
* \internal
*
*++ This function sets the optional sync counter
*
* \return
*++ RET_T
*-- RET_T
*
*/
RET_T setSyncCounter(
	UNSIGNED8	syncCnt		/* new sync counter */
	CO_COMMA_LINE_PARA_DECL	/* number of CAN line 0..CO_MAX_CAN_LINES-1 */
    )
{
    /* check value range */
    if ((syncCnt == 1) || (syncCnt > 240))  {
	return(CO_E_RANGE);
    }

    /* changing is only allowed, if cycle period is null */
    if ((GL_ARRAY(co_Sync).flags & CO_SYNC_FLAG_ACTIVE) != 0)  {
	return(CO_E_DEVICE_STATE);
    }
    /* changing is only allowed, if cycle period is null */
    if (GL_ARRAY(co_Sync).timer.timerVal != 0)  {
	return(CO_E_DEVICE_STATE);
    }

    GL_ARRAY(co_Sync).maxCounter = syncCnt;

    /* value range ok ? (2..240) */
    if ((syncCnt > 1) && (syncCnt < 241))  {
	/* transmit sync with sync counter */
	GL_ARRAY(co_Sync).pCOB->bLength = 1;
    } else {
	/* transmit sync without sync counter */
	GL_ARRAY(co_Sync).pCOB->bLength = 0;
    }

#if defined(CONFIG_PDO_CONSUMER) || defined(CONFIG_PDO_PRODUCER)
# ifdef CONFIG_PDO_SYNC_START_VALUE
    updatePdoSyncStartValues(CO_LINE_PARA);
# endif /* CONFIG_PDO_SYNC_START_VALUE */
#endif /* defined(CONFIG_PDO_CONSUMER) || defined(CONFIG_PDO_PRODUCER) */

    return(CO_OK);
}
# endif /* CONFIG_SYNC_COUNTER */


/*******************************************************************
*
* initSyncVars - init all Sync variables
*
* \internal
*
* RETURNS
* \retval nothing
*
*/

void initSyncVars(
	CO_LINE_PARA_DECL
    )
{
    /* clear global variables (some compilers doesn't clear global variables */
# ifdef CONFIG_CLEAR_CO_GLOBAL_VARS
    memset(&GL_ARRAY(co_Sync), (int)0, (size_t)(sizeof(SYNC_T)));
# endif /* CONFIG_FAST_SORT */

    GL_ARRAY(co_syncCnt) = 1;
}
#endif /* defined(CONFIG_SYNC_PRODUCER) || defined(CONFIG_SYNC_CONSUMER) */

/*______________________________________________________________________EOF_*/
