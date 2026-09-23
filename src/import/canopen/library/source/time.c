/*
 *++ time - functions for the Time Stamp Object (TIME)
 *-- time - Funktionen für das Time Stamp Object (TIME)
 *
 * Copyright (c) 1997-2017 port GmbH Halle (Saale)
 *--------------------------------------------------------------------------
 */


/****************************************************************************/
/**
*  \file time.c
*++ Functions for the Time Stamp Object (TIME)
*-- Funktionen für das Time Stamp Object (TIME)
*  \author port GmbH Halle (Saale)
*
*++ This module contains the functions
*++ for handling the CANopen Time Stamp Object (TIME).
*++ The Time Stamps data type is
*-- Dieses Modul beinhaltet Funktionen für das
*-- CANopen Time Stamp Object (TIME).
*-- Der Datentyp des Time Stamps ist
* \b "Time of Day" .
*/


/* header of standard C - libraries */

#include <stdio.h>
#include <string.h>

/* header of project specific types */

#include <cal_conf.h>
#include <co_odidx.h>
#include <co_acces.h>
#include <co_mcpy.h>
#include "time_lib.h"
#include "nmt.h"
#include "cmscodec.h"
#include "drv.h"

#ifdef CONFIG_NO_GLOBAL_VARS
# include <co_time.h>
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

# if defined(CONFIG_TIME_PRODUCER) || defined(CONFIG_TIME_CONSUMER)
CO_LIB_UNINIT_VAR CO_TIME_T	co_Time CO_LINE_PARA_ARRAY_DEF;
# endif /* defined(CONFIG_TIME_PRODUCER) || defined(CONFIG_TIME_CONSUMER) */
#endif /* CONFIG_NO_GLOBAL_VARS */


/* local defined variables
---------------------------------------------------------------------------*/
#ifdef CONFIG_RCS_IDENT
CO_LIB_INIT_VAR static char _rcsid[] = "$Id: time.c,v 2.35 2016/09/26 11:16:08 rli Exp $";
#endif /* CONFIG_RCS_IDENT */

#ifdef CONFIG_NO_GLOBAL_VARS
#else /* CONFIG_NO_GLOBAL_VARS */

# if defined(CONFIG_TIME_CONSUMER)
CO_LIB_UNINIT_VAR static TIME_OF_DAY_T 	stdTime CO_LINE_PARA_ARRAY_DEF;
# endif /* defined(CONFIG_TIME_PRODUCER) || defined(CONFIG_TIME_CONSUMER) */
#endif /* CONFIG_NO_GLOBAL_VARS */


#if defined(CONFIG_TIME_PRODUCER) || defined(CONFIG_TIME_CONSUMER)
/****************************************************************************/
/**
*++ \brief defineTime - define an Time Stamp Object
*-- \brief defineTime - definiert ein Time Stamp Objekt
*
*++ This function defines a Time Stamp Object.
*++ It must be called by both Time Stamp producer and consumers.
*++ If the TIME object uses only the default COB-ID (256) then
*++ the object dictionary entry 0x1012 (COB-ID TIME) is not necessary.
*++ In this case the compiler define
*++ \c CONFIG_TIME_COB_ID needs not to be set.
*-- Diese Funktion definiert ein Time Stamp Object.
*-- Diese Funktion ist sowohl von Time Stamp Produzenten als auch Konsumenten
*-- aufzurufen.
*-- Wenn das TIME Objekt nur die Standard-COB-ID (256) benutzt
*-- ist der Objektverzeichniseintrag 0x1012 nicht erforderlich.
*
*++ \b Example
*-- \b Beispiel
*
* \code
* RET_T retVal;
*
* // consumer
* retVal = defineTime(CONSUMER);
*
* // producer
* retVal = defineTime(PRODUCER);
* \endcode
*
* \retval CO_OK
*++ success
*-- Erfolg
* \retval CO_E_MEM
*++ memory allocation fault
*-- Fehler bei der dynamischen Speicheranforderung
* \retval CO_E_RANGE
*++ COB-ID is outside of the valid limits (1 - 1760)
*-- COB-ID ist außerhalb der gültigen Grenzen (1 - 1760)
* \retval CO_E_NO_ACCESS
*++ no access to object dictionary (COB-ID TIME, or node ID)
*-- Kein Zugriff auf das Objektverzeichnis möglich (COB-ID TIME oder Node ID)
* \retval CO_E_TRANS_TYPE
*++ only one PRODUCER or CONSUMER are allowed
*-- Nur ein Producer oder ein Consumer ist erlaubt
*
*/

RET_T defineTime(
	CO_USER_T kindOfUse  /**< kind of using (PRODUCER/CONSUMER) */
	CO_COMMA_LINE_PARA_DECL	/**< number of CAN line 0..CO_MAX_CAN_LINES-1 */
    )
{
UNSIGNED32	cobId;		/* temporary cob-id */
COB_KIND_T	cobKind;	/* cob-di kind */
RET_T		retVal;
UNSIGNED32	size;		/* size of object */

    if (getObjEntry(TIME_COB_ID_INDEX, 0,(UNSIGNED8 *)&cobId, &size, CO_TRUE
		CO_COMMA_LINE_PARA) != CO_OK) {
	return CO_E_NO_ACCESS;
    }

    if (kindOfUse == PRODUCER)  {
# ifdef CONFIG_TIME_PRODUCER
	cobId |= TIME_PRODUCER_BIT;
	cobId &= ~TIME_CONSUMER_BIT;
	cobKind = CO_COB_TIME_PROD;
# else /* CONFIG_TIME_PRODUCER */
	return(CO_E_TRANS_TYPE);
# endif /* CONFIG_TIME_PRODUCER */
    } else  {
# ifdef CONFIG_TIME_CONSUMER
	cobId |= TIME_CONSUMER_BIT;
	cobId &= ~TIME_PRODUCER_BIT;
	cobKind = CO_COB_TIME_CONS;
# else /* CONFIG_TIME_CONSUMER */
	return(CO_E_TRANS_TYPE);
# endif /* CONFIG_TIME_CONSUMER */
    }

    /* ensures consistency of object dictionary entry */
    if (putObj(TIME_COB_ID_INDEX, 0,(UNSIGNED8 *)&cobId, 4, CO_TRUE
		CO_COMMA_LINE_PARA) != CO_OK) {
	return CO_E_NO_ACCESS;
    }

    /* if time services is initialized */
    if ((GL_ARRAY(co_Time).flags & TIMEFLAG_INITIALIZED) == 0) {
	GL_ARRAY(co_Time).pCOB = DEFINE_COB(cobKind, 0 CO_COMMA_LINE_PARA);
	if (GL_ARRAY(co_Time).pCOB == NULL)  {
	    return(CO_E_NO_DATABASE);
	}
    }

    GL_ARRAY(co_Time).pCOB->bLength = 6;
    GL_ARRAY(co_Time).flags = TIMEFLAG_INITIALIZED;

    retVal = setTimeCobId(cobId CO_COMMA_LINE_PARA);

    return(retVal);
}


/****************************************************************************/
/*
*++ \brief setTimeCobId - set time cob-id
*-- \brief setTimeCobId - setzt die COB-Id für time Dienst
*
* \internal
*
*++ This function sets the cob-id for time services.
*-- Diese Funktion setzt die COB-Id für den Time Service.
*
* \retval CO_OK
*++ success
*-- Erfolg
* \retval CO_E_NOT_EXIST
*++ service not initialized
*-- Dienst nicht initialisiert
* \retval CO_E_TRANS_TYPE
*++ 29 bit identifier not allowed
*++ or consumer/producer bits not changable
*-- 29 bit idendtifier nicht erlaubt
*-- oder consumer/producer bits nicht änderbar
*
*/
RET_T setTimeCobId(
	UNSIGNED32 cobId	/* pointer to cob-id */
	CO_COMMA_LINE_PARA_DECL	/* number of CAN line 0..CO_MAX_CAN_LINES-1 */
    )
{
RET_T retVal = CO_OK;

    /* if time services isn't initialized */
    if ((GL_ARRAY(co_Time).flags & TIMEFLAG_INITIALIZED) == 0){
	return(CO_E_NOT_EXIST);
    }

    /* only one service is allowed */
    if ((cobId & (TIME_CONSUMER_BIT | TIME_PRODUCER_BIT))
	    == (TIME_CONSUMER_BIT | TIME_PRODUCER_BIT)) {
	return(CO_E_BAD_SERVICE);
    }


    /* are we time consumer */
    if ((cobId & TIME_CONSUMER_BIT) != 0)  {
# ifdef CONFIG_TIME_CONSUMER
	retVal = SET_COB_ID(GL_ARRAY(co_Time).pCOB,
		(cobId & CAN_BIT_ID_MASK),
		CO_COB_TIME_CONS);
	if (retVal != CO_OK)  {
	    return(retVal);
	}
	GL_ARRAY(co_Time).flags &= (FLAG_T)~TIMEFLAG_PRODUCER;
	GL_ARRAY(co_Time).flags |= TIMEFLAG_CONSUMER;
# else /* CONFIG_TIME_CONSUMER */
	return(CO_E_BAD_SERVICE);
# endif /* CONFIG_TIME_CONSUMER */
    } else

    /* are we time producer */
    if ((cobId & TIME_PRODUCER_BIT) != 0)  {
# ifdef CONFIG_TIME_PRODUCER
	/* change cob-id is only allowed if producer isn't set */
	if ((GL_ARRAY(co_Time).flags & TIMEFLAG_PRODUCER) != 0) {
	    return(CO_E_TRANS_TYPE);
	}
#  ifndef CO_CONFIG_DONT_CHECK_RESTRICTED_COBID
        retVal = coCheckRestrictedCobId(TIME_COB_ID_INDEX, cobId CO_COMMA_LINE_PARA);
        if (retVal != CO_OK)
        {
            return(retVal);
        }
#  endif
	retVal = SET_COB_ID(GL_ARRAY(co_Time).pCOB,
		(cobId & CAN_BIT_ID_MASK),
		CO_COB_TIME_PROD);
	if (retVal != CO_OK)  {
	    return(retVal);
	}
	GL_ARRAY(co_Time).flags |= TIMEFLAG_PRODUCER;
	GL_ARRAY(co_Time).flags &= (FLAG_T)~TIMEFLAG_CONSUMER;
# else /* CONFIG_TIME_PRODUCER */
	return(CO_E_BAD_SERVICE);
# endif /* CONFIG_TIME_PRODUCER */
    } else  {
	GL_ARRAY(co_Time).flags &= (FLAG_T)~(TIMEFLAG_PRODUCER | TIMEFLAG_CONSUMER);
    }
    GL_ARRAY(co_Time).cobId = (UNSIGNED16)cobId;

    return(CO_OK);
}
#endif  /* CONFIG_TIME_PRODUCER || CONFIG_TIME_CONSUMER */


#ifdef CONFIG_TIME_PRODUCER

/****************************************************************************/
/**
*++ \brief writeTimeReq - transmit a Time Stamp Object to the consumer(s)
*-- \brief writeTimeReq - sendet ein Time Stamp Object zu dem(n) Konsument(en)
*
*++ This function transmits a Time Stamp message to the consumer(s).
*++ This service is available in the node state OPERATIONAL.
*-- Diese Funktion sendet ein Time Stamp Object zu dem(n) Konsument(en).
*-- Dieser Dienst ist nur in dem Zustand OPERATIONAL verfügbar.
*
* \retval CO_OK
*++ success
*-- Erfolg
* \retval CO_E_STATE
*++ node isn't in state OPERATIONAL
*-- Knoten ist nicht im Zustand OPERATIONAL
*
*/

RET_T writeTimeReq(
	UNSIGNED32 time,     /**< time in ms after midnight */
	UNSIGNED16 days      /**< number of day since January 1, 1984 */
	CO_COMMA_LINE_PARA_DECL	/**< number of CAN line 0..CO_MAX_CAN_LINES-1 */
    )
{
UNSIGNED8	transBuf[6];
RET_T		retVal;

    if ((GL_ARRAY(co_Time).flags & TIMEFLAG_PRODUCER) == 0)  {
	return(CO_E_NOT_EXIST);
    }

    CO_UNPACK_MEMCPY(&transBuf[0], (UNSIGNED8 *)&time, 4, CO_TRUE);
    CO_UNPACK_MEMCPY(&transBuf[4], (UNSIGNED8 *)&days, 2, CO_TRUE);

# ifdef CONFIG_REDUNDANCY_SUPPORT
# else /* CONFIG_REDUNDANCY_SUPPORT */
    if ((GL_ARRAY(co_Node).eState != OPERATIONAL)
     && (GL_ARRAY(co_Node).eState != PRE_OPERATIONAL))  {
	return(CO_E_STATE);
    }
# endif /* CONFIG_REDUNDANCY_SUPPORT */

    retVal =  TRANSMIT_COB(GL_ARRAY(co_Time).pCOB, transBuf);

    return(retVal);
}

#endif /* CONFIG_TIME_PRODUCER */

#ifdef CONFIG_TIME_CONSUMER

/*******************************************************************/
/*
*++ timeReceived - read the Time Stamp Object
*-- timeReceived - liest das Time Stamp Object
*
* \internal
*
* This function is called, if a new time object was received
* \retval
*	nothing
*
*/

void timeMsgReceived(
	CAN_MSG_T *canMsg    /* Pointer to CAN Message */
	CO_COMMA_LINE_PARA_DECL	/* number of CAN line 0..CO_MAX_CAN_LINES-1 */
    )
{
NODE_STATE_T actState;

    /* check initialisation */
    if ((GL_ARRAY(co_Time).flags & TIMEFLAG_CONSUMER) == 0)  {
	return;
    }


#ifdef CONFIG_REDUNDANCY_SUPPORT
    /* process only data from active line */
    if (GL_VAR(co_redcyReceivedLine) != GL_VAR(co_redcyActiveLine))  {
	return;
    }

    /* check node state of received line */
    if (GL_VAR(co_redcyReceivedLine) == CAN_DEFAULT_LINE)  {
	actState = GL_ARRAY(co_Node).eState;
    } else {
	actState = GL_ARRAY(co_Node).pRedcy->eState;
    }
#else /* CONFIG_REDUNDANCY_SUPPORT */
    actState = GL_ARRAY(co_Node).eState;
#endif /* CONFIG_REDUNDANCY_SUPPORT */

    if ((actState != OPERATIONAL) && (actState != PRE_OPERATIONAL)) {
	return;
    }

    /* check cob-id */
    if (GL_ARRAY(co_Time).pCOB->cobId != canMsg->cobId) {
	return;
    }

    CO_PACK_MEMCPY((UNSIGNED8 *)&GL_ARRAY(stdTime).time,
	&canMsg->pData[0], 4, CO_TRUE);
    CO_PACK_MEMCPY((UNSIGNED8 *)&GL_ARRAY(stdTime).days,
	&canMsg->pData[4], 2, CO_TRUE);

    timeInd(&GL_ARRAY(stdTime) CO_COMMA_LINE_PARA);

    return;
}

#endif /* CONFIG_TIME_CONSUMER */


#if defined(CONFIG_TIME_PRODUCER) || defined(CONFIG_TIME_CONSUMER)
/*******************************************************************
*
* initTimeVars - init all Time variables
*
* \internal
*
* RETURNS
* \retval nothing
*
*/

void initTimeVars(
	CO_LINE_PARA_DECL
    )
{
    /* clear global variables (some compilers doesn't clear global variables */
# ifdef CONFIG_MULT_LINES
    (void)canLine;
# endif
# ifdef CONFIG_CLEAR_CO_GLOBAL_VARS
    memset(&GL_ARRAY(co_Time), 0x0, sizeof(CO_TIME_T));
# endif /* CONFIG_CLEAR_CO_GLOBAL_VARS */
}
#endif  /* CONFIG_TIME_PRODUCER || CONFIG_TIME_CONSUMER */
/*______________________________________________________________________EOF_*/
