/*
 *++ emerg - functions for Emergency Object (EMCY) handling
 *-- emerg - Funktionen für das Emergency Object (EMCY)
 *
 * Copyright (c) 1996-2017 port GmbH Halle (Saale)
 *------------------------------------------------------------------
 */


/****************************************************************************/
/**
*  \file emerg.c
*++ Functions for Emergency Object (EMCY) handling
*-- Funktionen für das Emergency Object (EMCY)
*  \author port GmbH Halle (Saale)
*
*++ This module contains functions for the handling and transmission
*++ of CANopen Emergency Objects (EMCY).
*++ Emergency messages are triggered by the occurence of a device internal
*++ error situation and are transmitted from the concerned
*++ application device to other devices with high priority.
*++ This makes them suitable for interrupt type error alerts.
*++ Transmitted errors are stored in index 0x1003.
*
*++ If an internal device error occures then the application
*++ transmits an error message with a predefined error code.
*
*-- Dieses Modul enthält Funktionen zur Manipulation und zum Senden
*-- von CANopen Emergency Objekten (EMCY).
*-- Emergency-Nachrichten werden bei Fehlern in einem Gerät
*-- ausgelöst und mit hoher Priorität übertragen.
*-- Sie enthalten vordefinierte und herstellerspezifische Fehlerinformationen.
*-- Versendete Fehler werden in der Fehlerhistory auf dem Index 0x1003
*-- hinterlegt.
*
*++ Emergency messages from other CANopen nodes can be received as well,
*++ if they are registered in the emergency consumer list (index 0x1028).
*++ The error information is presented in
*
*-- Fehlermeldung von anderen Knoten können ebenfalls empfangen werden,
*-- wenn sie in der Emcy-Consumer Liste eingetragen (Index 0x1028) sind.
*-- Die Fehlerinformationen werden über die
* emcyInd()
*-- Funktion bereitgestellt.
*
*
*/


/* header of standard C - libraries */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/* header of project specific types */

#include <cal_conf.h>
#include <co_drv.h>
#include <co_odidx.h>
#include <co_mcpy.h>
#include "emerg.h"
#include "nmt.h"
#include "drv.h"
#include "access.h"
#include "utility.h"

#ifdef CONFIG_NMT_STARTUP_MANAGER
# include "nmtstart.h"
#endif /* CONFIG_NMT_STARTUP_MANAGER */

#ifdef CONFIG_REDUNDANCY_SUPPORT
#include "reduncy.h"
#endif /* CONFIG_REDUNDANCY_SUPPORT */

/* constant definitions
---------------------------------------------------------------------------*/
#ifdef CONFIG_EMCY_CONSUMER
# ifdef CONFIG_DYN_MEM_ALLOC
#  define EMCY_CONSUMER_CNT	co_maxEmcyConsCnt
# else /* CONFIG_DYN_MEM_ALLOC */
#  define EMCY_CONSUMER_CNT	CONFIG_EMCY_CONSUMER
# endif /* CONFIG_DYN_MEM_ALLOC */
#endif /* CONFIG_EMCY_CONSUMER */

/* local defined data types
---------------------------------------------------------------------------*/

/* list of external used functions, if not in headers
---------------------------------------------------------------------------*/

/* list of global defined functions
---------------------------------------------------------------------------*/

/* list of local defined functions
---------------------------------------------------------------------------*/
#ifdef CONFIG_EMCY_CONSUMER
static UNSIGNED8 getEmcyConsIndex(UNSIGNED8 nodeId CO_COMMA_LINE_PARA_DECL);
static UNSIGNED8 searchEmcyConsCobId(COB_IDENT_T cobId CO_COMMA_LINE_PARA_DECL);
static RET_T setEmcyConsCobId(UNSIGNED8	nodeId, UNSIGNED32 cobId,
	BOOL_T sortLists CO_COMMA_LINE_PARA_DECL);
# ifdef CONFIG_FAST_SORT
static void sortEmcyConsLists(CO_LINE_PARA_DECL);
# endif /* CONFIG_FAST_SORT	 */
#endif /* CONFIG_EMCY_CONSUMER */

/* external variables
---------------------------------------------------------------------------*/

/* global variables
---------------------------------------------------------------------------*/
#ifdef CONFIG_NO_GLOBAL_VARS
#else /* CONFIG_NO_GLOBAL_VARS */

# ifdef CONFIG_EMCY_CONSUMER
#  ifdef CONFIG_DYN_MEM_ALLOC
CO_LIB_UNINIT_VAR EMCY_CONS_T		*p_emcyConsList[1];
CO_LIB_UNINIT_VAR UNSIGNED16		co_maxEmcyConsCnt;
#  else /* CONFIG_DYN_MEM_ALLOC */
CO_LIB_UNINIT_VAR EMCY_CONS_T		emcyConsList[CONFIG_EMCY_CONSUMER];
#  endif /* CONFIG_DYN_MEM_ALLOC */

#  ifdef CONFIG_MULT_LINES
		/* sdo server line counters */
#   ifdef CONFIG_DYN_MEM_ALLOC
CO_LIB_UNINIT_VAR UNSIGNED8	emcyConsLineCnts[CO_MAX_CAN_LINES];
#   else /* CONFIG_DYN_MEM_ALLOC */
CO_LIB_CONST_VAR UNSIGNED8	emcyConsLineCnts[CO_MAX_CAN_LINES] =
			    { CONFIG_EMCY_CONSUMER_LINECFG };
#   endif /* CONFIG_DYN_MEM_ALLOC */
			/* emcy consumer line offsets */
CO_LIB_UNINIT_VAR UNSIGNED16		emcyConsLineOffs CO_LINE_PARA_ARRAY_DEF;
#  endif /* CONFIG_MULT_LINES */
# endif /* CONFIG_EMCY_CONSUMER */
#endif /* CONFIG_NO_GLOBAL_VARS */

#ifdef CONFIG_EMCY_CONSUMER
# ifdef CONFIG_MULT_LINES
#  define CO_EMCY_CONS_LINE_CNTS	GL_ARRAY(emcyConsLineCnts)
# else /* CONFIG_MULT_LINES */
#  define CO_EMCY_CONS_LINE_CNTS	EMCY_CONSUMER_CNT
# endif /* CONFIG_MULT_LINES */
#endif /* CONFIG_EMCY_CONSUMER */

/* local defined variables
---------------------------------------------------------------------------*/
#ifdef CONFIG_RCS_IDENT
CO_LIB_INIT_VAR static char _rcsid[] = "$Id: emerg.c,v 2.65 2016/09/26 11:16:09 rli Exp $";
#endif /* CONFIG_RCS_IDENT */

#ifdef CONFIG_EMCY_PRODUCER
CO_LIB_UNINIT_VAR static UNSIGNED8 errorFreeData[5];
#endif /* CONFIG_EMCY_PRODUCER */

#ifdef CONFIG_NO_GLOBAL_VARS
#else /* CONFIG_NO_GLOBAL_VARS */

# ifdef CONFIG_EMCY_PRODUCER
CO_LIB_UNINIT_VAR static EMCY_T		co_EmcyProd CO_LINE_PARA_ARRAY_DEF;
# endif /* CONFIG_EMCY_PRODUCER */

# ifdef CONFIG_EMCY_CONSUMER
CO_LIB_INIT_VAR static UNSIGNED8	emcyConsFlags CO_LINE_PARA_ARRAY_DEF = { 0 };

#  ifdef CONFIG_FAST_SORT
#   ifdef CONFIG_DYN_MEM_ALLOC
CO_LIB_UNINIT_VAR UNSIGNED8	*p_emcyConsIdxList[1];
CO_LIB_UNINIT_VAR UNSIGNED8	*p_emcyConsCobIdxList[1];
#   else /* CONFIG_DYN_MEM_ALLOC */
CO_LIB_UNINIT_VAR static UNSIGNED8	emcyConsIdxList[CONFIG_EMCY_CONSUMER];
CO_LIB_UNINIT_VAR static UNSIGNED8	emcyConsCobIdxList[CONFIG_EMCY_CONSUMER];
#   endif /* CONFIG_DYN_MEM_ALLOC */
#  endif /* CONFIG_FAST_SORT */
# endif /* CONFIG_EMCY_CONSUMER */
#endif /* CONFIG_NO_GLOBAL_VARS */



#if defined(CONFIG_EMCY_PRODUCER) || defined(CONFIG_EMCY_CONSUMER)
/****************************************************************************/
/**
* \public
*
*++ \brief defineEmcy - defines an Emergency Object (EMCY)
*-- \brief defineEmcy - definiert ein Emergency Object (EMCY)
*
*++ This function defines an Emergency Object.
*++ It must be called once by both emergency producers
*++ and emergency consumers during initialization.
*++ All initialization data is taken from the object dictionary.
*++ Emergency producer relevant information is stored in index
*++ 0x1014 and 0x1015.
*++ Emergency consumer relevant information is stored in index
*++ 0x1028.

*-- Diese Funktion initialisiert die Emergency-Nutzung
*-- als Emergency Producer oder als Emergency-Consumer.
*-- Sie ist einmalig beim Programmstart für den jeweils
*-- erforderlichen Dienst (Emergency-Producer oder Emergency-Consumer)
*-- aufzurufen.
*-- Alle Initialisierungsdaten werden aus dem Objektverzeichnis entnommen.
*-- Für den Emergency-Producer ist das Objekt 0x1014 und 0x1015 relevant.
*-- Für den Emergency-Consumer enthält das Objekt 0x1028
*-- die Emergency Consumer Liste.
*
*++Example:
*--Beispiel:
*
* \code
* RET retVal;
*
* // producer
* retVal = defineEmcy(PRODUCER);
*
* // consumer
* retVal = defineEmcy(CONSUMER);
* \endcode
*
* \retval CO_OK
*++ success
*-- Erfolg
* \retval CO_E_MEM
*++ memory allocation fault
*-- Nicht genügend Speicher verfügbar
* \retval CO_E_NO_ACCESS
*++ no access to object dictionary (COB-ID EMCY)
*-- Kein Zugriff auf das Objektverzeichnis möglich (COB-ID EMCY)
* \retval CO_E_TRANS_TYPE
*++ type of service not available (not compiled)
*-- Dienst nicht möglich (nicht übersetzt)
*
*/

RET_T defineEmcy(
    CO_USER_T   kindOfUse	/**< kind of using (CONSUMER/PRODUCER) */
    CO_COMMA_LINE_PARA_DECL	/**< number of CAN line 0..CO_MAX_CAN_LINES-1 */
    )
{
RET_T		ret = CO_E_TRANS_TYPE;		/* return value */
#ifdef CONFIG_EMCY_CONSUMER
UNSIGNED8	i;
#endif /* CONFIG_EMCY_CONSUMER */

    if (kindOfUse == PRODUCER)  {

#ifdef CONFIG_EMCY_PRODUCER
	/* create COB */
	GL_ARRAY(co_EmcyProd).pCOB =
		DEFINE_COB(CO_COB_EMCY_PROD, 8 CO_COMMA_LINE_PARA);
	if (GL_ARRAY(co_EmcyProd).pCOB == NULL)  {
	    return(CO_E_NO_DATABASE);
	}
#else /* CONFIG_EMCY_PRODUCER */
	return(CO_E_TRANS_TYPE);
#endif /* CONFIG_EMCY_PRODUCER */
    }
    else {
#ifdef CONFIG_EMCY_CONSUMER

	/* create COBs */
	for (i = 0; i < CO_EMCY_CONS_LINE_CNTS; i++)  {
	    GL_PVAR(emcyConsList)[i
# ifdef CONFIG_MULT_LINES
		+ GL_ARRAY(emcyConsLineOffs)
# endif /* CONFIG_MULT_LINES */
		].pCOB = DEFINE_COB(CO_COB_EMCY_CONS, 8 CO_COMMA_LINE_PARA);
	    if (GL_PVAR(emcyConsList)[i
# ifdef CONFIG_MULT_LINES
		    + GL_ARRAY(emcyConsLineOffs)
# endif /* CONFIG_MULT_LINES */
		    ].pCOB == NULL)  {
		return(CO_E_NO_DATABASE);
	    }
	}
#else /* CONFIG_EMCY_CONSUMER */
	return(CO_E_TRANS_TYPE);
#endif /* CONFIG_EMCY_CONSUMER */
    }

    ret = initEmcy(kindOfUse CO_COMMA_LINE_PARA);

    return(ret);
}


/****************************************************************************/
/**
* \public
*
*++ initEmcy - intialize Emergency sercvice (EMCY)
*-- initEmcy - intitialisiert Emergency Dienst (EMCY)
*
* \internal
*
*-- Diese Funktion initialisiert die Emergency-Nutzung
*-- als Emergency Producer oder als Emergency-Consumer.
*-- Diese Funktion wird bei defineEmcy() und bei einem Reset-Comm aufgerufen
*
* \retval CO_OK
*++ success
*-- Erfolg
* \retval CO_E_MEM
*++ memory allocation fault
*-- Nicht genügend Speicher verfügbar
* \retval CO_E_NO_ACCESS
*++ no access to object dictionary (COB-ID EMCY)
*-- Kein Zugriff auf das Objektverzeichnis möglich (COB-ID EMCY)
* \retval CO_E_TRANS_TYPE
*++ type of service not available (not compiled)
*-- Dienst nicht möglich (nicht übersetzt)
*
*/

RET_T initEmcy(
	CO_USER_T   kindOfUse	/* kind of using (CONSUMER/PRODUCER) */
	CO_COMMA_LINE_PARA_DECL	/* number of CAN line 0..CO_MAX_CAN_LINES-1 */
    )
{
RET_T		ret = CO_E_TRANS_TYPE;		/* return value */
UNSIGNED32	size;		/* size of object */
UNSIGNED32	cobId;
#ifdef CONFIG_EMCY_CONSUMER
UNSIGNED8	i, cnt;
#endif /* CONFIG_EMCY_CONSUMER */

    if (kindOfUse == PRODUCER)  {

#ifdef CONFIG_EMCY_PRODUCER
	if (GL_ARRAY(co_EmcyProd).pCOB == NULL)  {
	    return(CO_E_NO_DATABASE);
	}

	/* get cob-id from od */
	if (getObjEntry(EMCY_COB_ID_INDEX, 0, (UNSIGNED8 *)&cobId, &size,
		CO_TRUE CO_COMMA_LINE_PARA) != CO_OK) {
	    return CO_E_NO_ACCESS;
	}

	GL_ARRAY(co_EmcyProd).flags = EMCYFLAG_DEFINED;

	/* setup the inhibit time if entry available */
	if (getObjEntry(EMCY_INHIBIT_INDEX, 0,
		(UNSIGNED8 *)&GL_ARRAY(co_EmcyProd).wInhibitTime,
		&size, CO_TRUE CO_COMMA_LINE_PARA) != CO_OK) {
	    /* no entry at ov found, reset the value */
	    GL_ARRAY(co_EmcyProd).wInhibitTime = 0;
	}
	GL_ARRAY(co_EmcyProd).inhibit.ticks = 0;

	/* set cob id */
	ret = setEmcyCobId(&cobId CO_COMMA_LINE_PARA);

#else /* CONFIG_EMCY_PRODUCER */
	return(CO_E_TRANS_TYPE);
#endif /* CONFIG_EMCY_PRODUCER */
    }
    else {

#ifdef CONFIG_EMCY_CONSUMER

	/* create COBs */
	for (i = 0; i < CO_EMCY_CONS_LINE_CNTS; i++)  {
	    if (GL_PVAR(emcyConsList)[i
# ifdef CONFIG_MULT_LINES
		    + GL_ARRAY(emcyConsLineOffs)
# endif /* CONFIG_MULT_LINES */
		    ].pCOB == NULL)  {
		return(CO_E_NO_DATABASE);
	    }
	}

	GL_ARRAY(emcyConsFlags) = EMCYFLAG_DEFINED;

	/* all emcy consumer entries from od are added automatically */
	/* get number of emcy consumer entries from od */
	if (getObjEntry(EMCY_CONSUMER_INDEX, 0, &cnt, &size, CO_TRUE
		    CO_COMMA_LINE_PARA) != CO_OK)  {
	    /* nop entries at od */
	    return(CO_OK);
	}

	GL_ARRAY(emcyConsFlags) |= EMCYFLAG_CONS_OV;

	for (i = 1; i <= cnt; i++)  {
	    /* subindex at ov refers to node-id !! */

	    /* get cob-id */
	    ret = getObjEntry(EMCY_CONSUMER_INDEX, i, (UNSIGNED8 *)&cobId,
		    &size, CO_TRUE CO_COMMA_LINE_PARA);
	    if (ret != CO_OK)  {
		return(ret);
	    }

	    ret = setEmcyConsCobId(i, cobId, CO_FALSE CO_COMMA_LINE_PARA);
	    if (ret != CO_OK)  {
		return(ret);
	    }
	}

# ifdef CONFIG_FAST_SORT
	/* sort consumer lists */
	sortEmcyConsLists(CO_LINE_PARA);
# endif	/* CONFIG_FAST_SORT	 */

#else /* CONFIG_EMCY_CONSUMER */
	return(CO_E_TRANS_TYPE);
#endif /* CONFIG_EMCY_CONSUMER */
    }

    return(ret);
}

#endif  /* defined( CONFIG_EMCY_PRODUCER || CONFIG_EMCY_CONSUMER ) */


#ifdef CONFIG_EMCY_PRODUCER

/****************************************************************************/
/**
*++ \brief writeEmcyReq - transmit an Emergency Object
*-- \brief writeEmcyReq - sendet ein Emergency Objekt
*
*++ This function transmits the passed by error information
*++ and the error register content (index 0x1001) with an
*++ CANopen emergency object (EMCY). Then the error is stored in the
*++ local pre-defined error field at index 0x1003.
*-- Diese Funktion sendet die übergebenen Fehlerinformationen
*-- und den Inhalt des globalen Error-Registers (index 0x1001)
*-- in einem CANopen Emergency Objekt (EMCY).
*-- und speichert sie im lokalen pre-defined error field (0x1003) ab.
*++ Entries in 0x1003 are UNSIGNED32 and are composed of the 16-bit \e errCode
*++ in the lower 2 bytes (LSB) and two lower two bytes
*++ of the manufacturer specific error code \e manu[0] and \e manu[1].
*-- Ein Eintrag im 0x1003 Array ist UNSIGNED32. Er setzt sich zusammen
*-- aus dem 16 Bit \e errCode in den unteren zwei Byte (LSB)
*-- und den niederwertigsten beiden Bytes des herstellerspezifischen
*-- Fehlercodes aus dem übergebenen Array \e manu[0] und \e manu[1].
*
*++ The user application is responsible for setting and resetting
*++ the global error register at index 0x1001.
*++ For error codes > 0x00ff the Generic error Bit ist set automatically.
*-- Der Anwender ist für das Setzen bzw. Rücksetzen des
*-- globalen Error-Registers auf dem Index 0x1001 verantwortlich.
*-- Bei Fehlercodes > 0x00ff wird das Generic error Bit automatisch gesetzt.
*
*++ This service is available in the node states PRE_OPERATIONAL and
*++ OPERATIONAL.
*-- Dieser Dienst ist in den Zuständen PRE_OPERATIONAL und
*-- OPERATIONAL verfügbar.
*
*++ Example:
*-- Beispiel:
*
* \code
* UNSIGNED8 manuErr[5];
* RET_T ret;
*
* manuErr[0] = 0x1;
* manuErr[1] = 0x2;
* manuErr[2] = 0x3;
* manuErr[3] = 0x4;
* manuErr[4] = 0x5;
*
* ret = writeEmcyReq(0xffff, &manuErr[0]);
* if (ret != CO_OK)  {
*     printf( "error EMCY 0xFF00 %d\n",(int)ret);
* }
* \endcode
*
* \retval CO_OK
*++ success
*-- Erfolg
* \retval CO_E_NOT_EXIST
*++ Emergency Object or error register (0x1001) doesn't exist
*-- Emergency Objekt oder Error Register (0x1001) existieren nicht
* \retval CO_E_STATE
*++ node isn't in state OPERATIONAL or PRE_OPERATIONAL
*-- Knoten ist nicht im Zustand OPERATIONAL oder PRE_OPERATIONAL
* \retval CO_E_DISABLED
*++ emergency service disabled
*-- Emergency Dienst disabled
* \retval CO_E_INHIBITED
*++ inhibit time not over
*-- Inhibit Zeit noch nicht abgelaufen
*
*/

RET_T writeEmcyReq(
	UNSIGNED16 errCode,	/**< error code */
	UNSIGNED8  *manu	/**< manufacturer spec. error code (Byte 3..7)*/
	CO_COMMA_LINE_PARA_DECL	/**< number of CAN line 0..CO_MAX_CAN_LINES-1 */
    )
{
UNSIGNED8	trBuf[8];	/* transmit buffer */
RET_T	   	ret;		/* return value */
UNSIGNED32  	size;		/* object size */
UNSIGNED8  	errCnt;		/* error counter */
UNSIGNED32 	err;		/* error code */
UNSIGNED8	*pData;		/* pointer to data */
UNSIGNED8	i;
#ifdef CONFIG_BIG_ENDIAN
UNSIGNED8	*pDat2;		/* second pointer to data */
#endif /* CONFIG_BIG_ENDIAN */

    /* if producer initialised */
    if ((GL_ARRAY(co_EmcyProd).flags & EMCYFLAG_ENABLED) == 0) {
	return(CO_E_DISABLED);
    }

    /* allow NULL data */
    if (manu == NULL)  {
	manu = &errorFreeData[0];
    }

    /* get content of error register */
    if (getObjAddr(ERROR_REGISTER_INDEX, 0, &pData, &size CO_COMMA_LINE_PARA)
		!= CO_OK) {
	return(CO_E_NOT_EXIST);
    }
    if (errCode > 0x00FF)  {/* 0 - ff no error/reset error message*/
	/* set Bit 0 - generic error */
	*pData |= 1;
    }

    trBuf[2] = *pData;

    /* generate entry for predefines error field */
    err = ((UNSIGNED32)manu[0] << 24) | ((UNSIGNED32)manu[1] << 16)
	| ((UNSIGNED32)errCode & 0xffff);

    /* create transmit buffer data */
    trBuf[0] = (UNSIGNED8)(errCode & 0xff);
    trBuf[1] = (UNSIGNED8)((errCode >> 8) & 0xff);
    i = 0;
    while (i < 5)  {
	trBuf[i + 3] = manu[i];
	i++;
    }

#ifdef CONFIG_REDUNDANCY_SUPPORT
#else /* CONFIG_REDUNDANCY_SUPPORT */
    if ((GL_ARRAY(co_Node).eState != OPERATIONAL)
     && (GL_ARRAY(co_Node).eState != PRE_OPERATIONAL))  {
	    return(CO_E_STATE);
    }
#endif /* CONFIG_REDUNDANCY_SUPPORT */

# ifdef CONFIG_FULLCAN
    UPDATE_COB(pEv->pCOB, pData);
# endif /* CONFIG_FULLCAN */

    if (GL_ARRAY(co_EmcyProd).inhibit.ticks > 0)  {
    	return(CO_E_INHIBITED);
    }

    ret = TRANSMIT_COB(GL_ARRAY(co_EmcyProd).pCOB, trBuf);
    if (ret != CO_OK)  {
	return(ret);
    }

    /* start inhibit timer */
    if (GL_ARRAY(co_EmcyProd).wInhibitTime > 0)  {
	startInhibitTimer(&GL_ARRAY(co_EmcyProd).inhibit,
		GL_ARRAY(co_EmcyProd).wInhibitTime
		CO_COMMA_LINE_PARA);
    }

    /* internal error handling */
    if (errCode > 0x00FF)  {/* 0 - ff no error/reset error message*/
	/* read the counter of errorcodes */
	ret = getObjAddr(ERROR_FIELD_INDEX, 0, &pData, &size CO_COMMA_LINE_PARA);
	if (ret != CO_OK) {
	    /* no entries, return */
	    return(CO_OK);
	}

	/* actually error count */
	errCnt = *pData;

	/* allocate security mechanism for object dictionary consistency */

	CO_COM_PART_ALLOC(CO_LINE_PARA);
	/* deletes the oldest entry, if counter == array size */

	if (errCnt == (getNumOfElem(ERROR_FIELD_INDEX CO_COMMA_LINE_PARA) - 1)){
	    errCnt--;
	}

	/* move error elements to pos+1 -
	 * (internal all index are saved as 32 bit val) */
# ifdef CONFIG_16BIT_CPU
	CO_NUM_MEMMOVE((void*)(pData+4),(void*)(pData+2),(size_t)(errCnt * 4),
		CO_NUM_VAL);
# else 	/* CONFIG_16BIT_CPU */
#  ifdef CONFIG_BIG_ENDIAN
	if (getObjAddr(ERROR_FIELD_INDEX, 1, &pDat2, &size CO_COMMA_LINE_PARA)
		== CO_OK) {
	    CO_NUM_MEMMOVE((void*)(pDat2+4),(void*)(pDat2),(size_t)(errCnt * 4),
		CO_NUM_VAL);
	}
#  else /* CONFIG_BIG_ENDIAN */
	CO_NUM_MEMMOVE((void*)(pData+8),(void*)(pData+4),(size_t)(errCnt * 4),
		CO_NUM_VAL);
#  endif /* CONFIG_BIG_ENDIAN */
# endif /* CONFIG_16BIT_CPU */
	errCnt++;                         /* increments the counter */

	*pData = errCnt;

	/* release security mechanism for object dictionary consistency */
	CO_COM_PART_RELEASE(CO_LINE_PARA);

	/* the error code is filled in at the begin of the array */
	if (putObj(ERROR_FIELD_INDEX, 1, (UNSIGNED8 *)&err, (UNSIGNED32)4,
			CO_TRUE CO_COMMA_LINE_PARA)
		!= CO_OK) {
	    return(CO_E_NOT_EXIST);
	}
    }
    return(CO_OK);
}
#endif /* CONFIG_EMCY_PRODUCER */


#ifdef CONFIG_EMCY_CONSUMER

/****************************************************************************/
/**
*++ \brief setEmcyConsumerCobId - set cob-id for emcy consumer
*-- \brief setEmcyConsumerCobId - setzt COB-Id für Emcy-Consumer
*
*++ This function sets the COB-Id for an emergency consumer.
*++ If the node doesn't exist at the consumer list
*++ it will be added automatically.
*++ If the invalid bit at the COB-Id is set it will be deleted.
*-- Diese Funktion setzt die COB-Id für einen Emergency Consumer.
*-- Wenn der Knoten noch nicht in der Consumer Liste eingetragen ist,
*-- wird er automatisch eingefügt.
*-- Wenn das Invalid Bit bei der COB-Id gesetzt ist,
*-- wird der Knoten aus der Consumer-Liste ausgetragen.
*
* \attention
*++ This function doesn't change anything at the consumer list
*++ at the object dictionary.
*++ If the list exists, the COB-IDs should be changed there
*++ and it should be validated by setCommPar().
*-- Diese Funktion ändert keine Einträge in der Emcy-Consumer-Liste
*-- im Objektverzeichnis !!
*-- Wenn dieser Eintrag im Objektverzeichnis existiert,
*-- sollte die COB-Id dort aktualisiert
*-- und anschliessend mit setCommPar() gültig gesetzt werden.
*
*
* \retval CO_OK
*--	Erfolg
*++	ok
* \retval RET_T
*++	Error
*--	bei Fehlern
*/
RET_T setEmcyConsumerCobId(
	UNSIGNED8	nodeId,		/**< Emcy Consumer Id */
	UNSIGNED32	cobId		/**< COB-Id */
	CO_COMMA_LINE_PARA_DECL	/**< number of CAN line 0..CO_MAX_CAN_LINES-1 */
    )
{
RET_T	retVal;

    retVal = setEmcyConsCobId(nodeId, cobId, CO_TRUE CO_COMMA_LINE_PARA);

    return(retVal);
}


/****************************************************************************/
/*
*++ setEmcyConsCobId - set cob-id for emcy consumer
*-- setEmcyConsCobId - setzt COB-Id für Emcy-Consumer
*
* \internal
*
*++ This function sets the COB-Id for an emergency consumer.
*++ If the node doesn't exist at the consumer list
*++ it will be added automatically.
*++ If the invalid bit at the COB-Id is set it will be deleted.
*-- Diese Funktion setzt die COB-Id für einen Emergency Consumer.
*-- Wenn der Knoten noch nicht in der Consumer Liste eingetragen ist,
*-- wird er automatisch eingefügt.
*-- Wenn das Invalid Bit bei der COB-Id gesetzt ist,
*-- wird der Knoten aus der Consumer-Liste ausgetragen.
*
*
* \retval CO_OK
*--	Erfolg
*++	ok
* \retval RET_T
*++	Error
*--	bei Fehlern
*/
static RET_T setEmcyConsCobId(
	UNSIGNED8	nodeId,		/* Emcy Consumer Id */
	UNSIGNED32	cobId,		/* COB-Id */
	BOOL_T		sortLists	/* sort node and cob-id list */
	CO_COMMA_LINE_PARA_DECL	/* number of CAN line 0..CO_MAX_CAN_LINES-1 */
    )
{
UNSIGNED8	idx;
EMCY_CONS_T	*pEmcy;
RET_T		retVal = CO_OK;

    /* check, if consumer is already at the list */
    idx = getEmcyConsIndex(nodeId CO_COMMA_LINE_PARA);
    if (idx == 0xff)  {
	/* no, search a new free entry */
	idx = 0;
	while (idx < CO_EMCY_CONS_LINE_CNTS)  {
	    /* node = 0 */
	    if (GL_PVAR(emcyConsList)[idx
# ifdef CONFIG_MULT_LINES
		+ GL_ARRAY(emcyConsLineOffs)
# endif /* CONFIG_MULT_LINES */
		    ].nodeId == 0)  {
		break;
	    }
	    idx++;
	}
    }

    /* list full ? */
    if (idx == CO_EMCY_CONS_LINE_CNTS)  {
	return(CO_E_MEM);
    }

    pEmcy = &GL_PVAR(emcyConsList)[idx
# ifdef CONFIG_MULT_LINES
		+ GL_ARRAY(emcyConsLineOffs)
# endif /* CONFIG_MULT_LINES */
	];

    /* not valid bit set ? */
    if ((cobId & EMCY_NOT_VALID_BIT) != 0)  {
	/* yes, remove entry from list */
	pEmcy->nodeId = 0;
	pEmcy->flags &= (FLAG_T)~EMCYFLAG_ENABLED;

#ifdef CONFIG_FAST_SORT
	if (sortLists == CO_TRUE)  {
	    sortEmcyConsLists(CO_LINE_PARA);
	}
#endif /* CONFIG_FAST_SORT */
	return(CO_OK);
    }

    /* cob-id 0 isn't allowed */
    if ((cobId & CAN_29_BIT_ID_MASK) == 0)  {
	return(CO_E_RANGE);
    }

    /* save (new) entry */

    retVal = SET_COB_ID(pEmcy->pCOB, cobId & CAN_BIT_ID_MASK, CO_COB_EMCY_CONS);
    if (retVal != CO_OK)  {
	return(retVal);
    }
    pEmcy->nodeId = nodeId;

    /* set emergency valid */
    pEmcy->flags = EMCYFLAG_ENABLED;

#ifdef CONFIG_FAST_SORT
    if (sortLists == CO_TRUE)  {
	sortEmcyConsLists(CO_LINE_PARA);
    }
#else /* CONFIG_FAST_SORT */
    sortLists = sortLists;
#endif /* CONFIG_FAST_SORT */

    return(CO_OK);
}


# ifdef CONFIG_FAST_SORT
/****************************************************************************/
/*
*++ sortEmcyConsLists - sort cobid and node id list
*
* \internal
*
*++ This function sorts the emcy consumer lists
*++ for cob-ids and node ids
*
* \retval nothing
*
*/
static void sortEmcyConsLists(
	CO_LINE_PARA_DECL
    )
{
    /* sort node-id list */
    sortNodeIdList(
# ifdef CONFIG_MULT_LINES
	&GL_PVAR(emcyConsIdxList)[GL_ARRAY(emcyConsLineOffs)],
	&GL_PVAR(emcyConsList)[GL_ARRAY(emcyConsLineOffs)].nodeId,
# else /* CONFIG_MULT_LINES */
	GL_PVAR(emcyConsIdxList),
	&GL_PVAR(emcyConsList)[0].nodeId,
# endif /* CONFIG_MULT_LINES */
	sizeof(EMCY_CONS_T),
	CO_EMCY_CONS_LINE_CNTS
    );

    sortCobIdList(
# ifdef CONFIG_MULT_LINES
	&GL_PVAR(emcyConsCobIdxList)[GL_ARRAY(emcyConsLineOffs)],
	&GL_PVAR(emcyConsList)[GL_ARRAY(emcyConsLineOffs)].pCOB,
# else /* CONFIG_MULT_LINES */
	&GL_PVAR(emcyConsCobIdxList)[0],
	&GL_PVAR(emcyConsList)[0].pCOB,
# endif /* CONFIG_MULT_LINES */
	sizeof(EMCY_CONS_T),
	CO_EMCY_CONS_LINE_CNTS
    );
}
# endif /* CONFIG_FAST_SORT */
#endif /* CONFIG_EMCY_CONSUMER */


#ifdef CONFIG_EMCY_PRODUCER
/****************************************************************************/
/**
*++ \brief eraseErr - erase error entries from the Predefined Errorfield
*-- \brief eraseErr - löscht Fehlereinträge aus dem Predefined Errorfield
*
*++ If a device error was repared then this function has to be called by the
*++ application.
*++ It does erase the error(s) from the
*++ CANopen error-field object at index 0x1003 and decrements the error
*++ counter at subindex 0.
*++ If the current error is the last error then it will
*++ send an emergency message with error code \c NO_ERROR (0)
*++ and the generic error bit will be reset.
*-- Wenn Gerätefehler behoben wurden,
*-- können mit dieser Funktion
*-- die Fehler im predefined error field (Index 0x1003)
*-- gelöscht werden.
*-- Gleichzeitig wird der Fehlerzähler auf Subindex 0 aktualisiert.
*-- Ist der letzte Fehlereintrag gelöscht, so sendet diese Funktion
*-- eine Emergency Nachricht mit dem Fehlerkode \c NO_ERROR (0).
*-- Gleichzeitig wird das Generic Error Bit gelöscht.
*
*++ If the parameter \em addErrCode equals zero
*++ the whole list is erased.
*++ For all other cases all entries are erased from the array,
*++ which have the same additional error code
*++ like the parameter \em addErrCode.
*-- Wenn der Parameter \em addErrCode Null ist,
*-- wird die gesamte Fehlerliste gelöscht.
*-- Anderenfalls werden nur die Fehlereinträge,
*-- die die Zusatzfehlerinformation
*-- \em addErrCode enthalten, gelöscht.
*
* \retval CO_OK
*++ success
*-- Erfolg
* \retval CO_E_NOT_EXIST
*++ Emergency Object or error register doesn't exists
*-- Emergency Objekt oder Error Register (0x1001) existieren nicht
* \retval CO_E_STATE
*++ node isn't in state PRE_OPERATIONAL or OPERATIONAL
*-- Knoten ist nicht im Zustand OPERATIONAL oder PRE_OPERATIONAL
* \retval CO_E_DISABLED
*++ emergency service disabled
*-- Emergency Dienst disabled
* \retval CO_E_INHIBITED
*++ inhibit time not over
*-- Inhibit Zeit noch nicht angelaufen
*
*/

RET_T eraseErr(
	UNSIGNED16 addErrCode   /**< additional error code of 0x1003 */
	CO_COMMA_LINE_PARA_DECL	/**< number of CAN line 0..CO_MAX_CAN_LINES-1 */
    )
{
RET_T 		ret=CO_OK;	/* return value */
UNSIGNED8	*ppData[1];     /* reference to predefined error field */
UNSIGNED32 	size;           /* dummy entry */
UNSIGNED8 	i;              /* loop counter */
UNSIGNED8 	max;            /* maximum of array entries */
UNSIGNED8       *pCount;	/* number of current entries */
UNSIGNED32	*pErr;		/* error field entry */
UNSIGNED8	*pErrReg;

    /* read the counter of errorcodes*/
    if (getObjAddr(ERROR_FIELD_INDEX, 0, &pCount, &size CO_COMMA_LINE_PARA)
	 != CO_OK) {
	return(CO_E_NOT_EXIST);
    }

    /* allocate security mechanism for object dictionary consistency */
    CO_COM_PART_ALLOC(CO_LINE_PARA);

    /* default case */
    if (addErrCode == 0) {
	/* get address of first entry */
	/* getObjAddr(ERROR_FIELD_INDEX, 1, (UNSIGNED8 **)&pErr, &size */
	(void) getObjAddr(ERROR_FIELD_INDEX, 1, &ppData[0], &size CO_COMMA_LINE_PARA);
	pErr = (UNSIGNED32 *)ppData[0];
	if (*pErr > 0) {
	    *pErr = 0;        	/* erase all entries */
	    *pCount = 0;
	} else {
	    /* error was already deleted */
	    /* release security mechanism for object dictionary consistency */
	    CO_COM_PART_RELEASE(CO_LINE_PARA);
	    return(CO_OK);
	}
    }
    else { /* erases selective */
	max = *pCount;
	for (i = 0; i < max; i++) {
	    (void) getObjAddr(ERROR_FIELD_INDEX, i+1, ppData, &size CO_COMMA_LINE_PARA);
	    pErr = (UNSIGNED32 *)*ppData;

	    if (((UNSIGNED16)((*pErr) >> 16)) == addErrCode) {
		(*pCount)--;
#ifdef CONFIG_16BIT_CPU
		CO_NUM_MEMMOVE(*ppData,(*ppData)+2,((max-i)-1)*4, CO_NUM_VAL);
#else /* CONFIG_16BIT_CPU */
		CO_NUM_MEMMOVE(*ppData,(*ppData)+4,((max-i)-1)*4, CO_NUM_VAL);
#endif /* CONFIG_16BIT_CPU */
	    }
	}
    }

    /* no more errors in the array */
    if (*pCount == 0) {
	/* delete general error flag */
	if (getObjAddr(ERROR_REGISTER_INDEX, 0, &pErrReg, &size
		CO_COMMA_LINE_PARA) == CO_OK) {
	    /* reset Bit 0 - generic error */
	    *pErrReg &= ~1;
	}

#ifdef CONFIG_EMCY_ERRORFREE_IND
	if (emcyErrFreeInd(CO_LINE_PARA) == CO_OK)
#endif /* CONFIG_EMCY_ERRORFREE_IND */
        {
	    ret = writeEmcyReq(0, &errorFreeData[0] CO_COMMA_LINE_PARA);
	}
    }
    /* release security mechanism for object dictionary consistency */
    CO_COM_PART_RELEASE(CO_LINE_PARA);

    return(ret);
}


/****************************************************************************/
/*
*++ \brief setEmcyCobId - set the COB-ID for a EMCY producer
*-- \brief setEmcyCobId - setzt die COB-ID eines EMCY Producer
*
* \internal
*
*++ This service sets a new local COB-ID for a EMCY producer.
*-- Mit dieser Funktion kann eine neue COB-ID beim EMCY Producer
*-- eingestellt werden.
*
* \retval CO_OK
*++ success
*-- Erfolg
* \retval CO_E_NOT_EXIST
*++ service not initialized
*-- service nicht initialisiert
* \retval CO_E_TRANS_TYPE
*++ 29 bit identifier are not allowed
*-- 29 bit identifier nicht erlaubt
*/

RET_T setEmcyCobId(
	UNSIGNED32 *pCobid	/* pointer to new cob-id */
	CO_COMMA_LINE_PARA_DECL	/* number of CAN line 0..CO_MAX_CAN_LINES-1 */
    )
{
RET_T	retVal = CO_OK;

    if ((GL_ARRAY(co_EmcyProd).flags & EMCYFLAG_DEFINED) == 0) {
	retVal = CO_E_NOT_EXIST;
    } else {

	if ((*pCobid & EMCY_NOT_VALID_BIT) != 0)  {
	    /* disable emcy */
	    GL_ARRAY(co_EmcyProd).flags &= (FLAG_T)~EMCYFLAG_ENABLED;
        } else {
	    /* enable emcy */
#ifndef CO_CONFIG_DONT_CHECK_RESTRICTED_COBID
            retVal = coCheckRestrictedCobId(EMCY_COB_ID_INDEX, *pCobid CO_COMMA_LINE_PARA);
            if (retVal != CO_OK)  {
                /* printf("setEmcyCobId:coCheckRestrictedCobId %d\n",retVal);*/
                return(retVal);
            }
#endif
	    retVal = SET_COB_ID(GL_ARRAY(co_EmcyProd).pCOB,
		(*pCobid & CAN_BIT_ID_MASK), CO_COB_EMCY_PROD);
	    GL_ARRAY(co_EmcyProd).flags |= EMCYFLAG_ENABLED;
        }
    }

    return(retVal);
}


/****************************************************************************/
/*
*++ \brief setEmcyInhibit - set the inhibit time for a EMCY producer
*-- \brief setEmcyInhibit - setzt die Inhibit Zeit eines EMCY Producer
*
* \internal
*
*++ This service sets a new inhibit time for a EMCY producer.
*++ The inhibit timer will be killed.
*-- Mit dieser Funktion kann eine neue Inhibit Zeit beim EMCY Producer
*-- eingestellt werden.
*-- Wenn der Inhibittimer aktiv ist, wird er gelöscht.
*
* \retval CO_OK
*++ success
*-- Erfolg
* \retval CO_E_NOT_EXIST
*++ service not initialized
*-- Dienst nicht initialisiert
*
*/

RET_T setEmcyInhibit(
	UNSIGNED16	*pInhibit	/* pointer to inhibit time */
	CO_COMMA_LINE_PARA_DECL	/* number of CAN line 0..CO_MAX_CAN_LINES-1 */
    )
{
RET_T retVal = CO_OK;

    if ((GL_ARRAY(co_EmcyProd).flags & EMCYFLAG_DEFINED) == 0) {
	retVal = CO_E_NOT_EXIST;
    } else {

        GL_ARRAY(co_EmcyProd).wInhibitTime = *pInhibit;

        /* stop inhibit timer */
        stopInhibitTimer(&GL_ARRAY(co_EmcyProd).inhibit CO_COMMA_LINE_PARA);
    }
    return(retVal);
}


/****************************************************************************/
/*
*++ \brief checkErrorFieldAccess - checks access to subindex
*-- \brief checkErrorFieldAccess - testet Zugriff auf Subindex
*
* \internal
*
*-- Nach aktuellem DS301 darf nur noch auf die Subindex zugegriffen werden,
*-- die im Subindex 0 als gültig gekennzeichnet sind.
*-- Diese Funktion testet,
*-- ob auf den angeforderten Subindex zugegriffen werden darf.
*++ The actual DS301 allow the access to subindex only,
*++ if it is sign as valid at subindex 0.
*++ This function tests this an returns the return value.
*
* \retval CO_OK
*++ success
*-- Erfolg
* \retval CO_E_NO_DATA_AVAILABLE
*++ Subindex doesn't allowed zu read
*-- Subindex nicht erlaubt zu lesen
*
*/
RET_T checkErrorFieldAccess(
	UNSIGNED8	subIndex	/* subindex */
	CO_COMMA_LINE_PARA_DECL
    )
{
UNSIGNED8	cnt;
UNSIGNED32	size;

    /* access to subindex 0 is alltimes allowed */
    if (subIndex == 0u)  {
	return(CO_OK);
    }

    /* access to all other subindex are only allowed
     * if subindex 0 is >= this subindex
     */
    (void) getObjEntry(ERROR_FIELD_INDEX, 0u, &cnt, &size, CO_TRUE CO_COMMA_LINE_PARA);
    if (subIndex > cnt) {
	return(CO_E_NO_DATA_AVAILABLE);
    }

    return(CO_OK);
}
#endif /* CONFIG_EMCY_PRODUCER */


#ifdef CONFIG_EMCY_CONSUMER
/****************************************************************************/
/*
*
*++ \brief emcyMsgReceived - Emergency message Received
*-- \brief emcyMsgReceived - Fehlermeldung empfangen
*
* \internal
*
* This function is called, if a new emergency was received.
* If the emergency consumer was initialized,
* then the indication function emcyInd() is called.
*
* \return
* nothing
*
*/
void emcyMsgReceived(
	CAN_MSG_T *canMsg	    /* Pointer to CAN Message */
	CO_COMMA_REDCY_PARA_DECL
    )
{
UNSIGNED8	idx;		/* idx at consumer list */
EMERGENCY_T	coLastEmcy;
UNSIGNED8	i;

    /* ignore messages in STOPPED */
    if ((GL_ARRAY(co_Node).eState != OPERATIONAL)
     && (GL_ARRAY(co_Node).eState != PRE_OPERATIONAL))  {
	    return;
    }

    /* check for old bootup message */
    if (canMsg->length == 0) {
#if defined(CONFIG_MASTER) || defined(CONFIG_HEARTBEAT_CONSUMER)
	mGuardErrorInd(canMsg->cobId & 0x7f, CO_BOOT_UP CO_COMMA_REDCY_PARA);

# ifdef CONFIG_NMT_STARTUP_MANAGER
	nmtsEventHandler(NMT_ERRCTRL_BOOTUP_RECEIVED,
	    (UNSIGNED8)(canMsg->cobId & 0x7f)
	    CO_COMMA_LINE_PARA);
# endif
#endif /* defined(CONFIG_MASTER) || defined(CONFIG_HEARTBEAT_CONSUMER) */
	return;
    }

    idx = searchEmcyConsCobId(canMsg->cobId CO_COMMA_LINE_PARA);
    if (idx == 0xff)  {
	return;
    }

    coLastEmcy.errCode = ((UNSIGNED16)canMsg->pData[1]) << 8u | canMsg->pData[0];
    coLastEmcy.errReg = canMsg->pData[2];
    i = 0;
    while (i < 5)  {
	coLastEmcy.manu[i] = canMsg->pData[i + 3];
	i++;
    }

# ifdef CONFIG_REDUNDANCY_SUPPORT
    /* emcy indication shall only indicated on active interface */
    if (canLine == GL_VAR(co_redcyActiveLine))
# endif /* CONFIG_REDUNDANCY_SUPPORT */
    {
	/* call user indication */
	emcyInd(GL_PVAR(emcyConsList)[idx
# ifdef CONFIG_MULT_LINES
		+ GL_ARRAY(emcyConsLineOffs)
# endif /* CONFIG_MULT_LINES */
		].nodeId, &coLastEmcy CO_COMMA_LINE_PARA);
    }
}


/*******************************************************************
*
* getEmcyConsIndex - searches for Emcy Consumer entry
*
* \internal
*
* This function checks the Emcy Consumer list
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
static UNSIGNED8 getEmcyConsIndex(
	UNSIGNED8 nodeId	/**< node id */
	CO_COMMA_LINE_PARA_DECL	/**< number of CAN line 0..CO_MAX_CAN_LINES-1 */
    )
{
EMCY_CONS_T	*pEmcy;
# ifdef CONFIG_FAST_SORT
INTEGER8	found = 0;
INTEGER16	low, mid = 0, high;
UNSIGNED8	*pIdx;
# else /* CONFIG_FAST_SORT */
UNSIGNED8	i;		/* loop variable */
# endif /* CONFIG_FAST_SORT */

# ifdef CONFIG_MULT_LINES
    pEmcy = &GL_PVAR(emcyConsList)[GL_ARRAY(emcyConsLineOffs)];
# else /* CONFIG_MULT_LINES */
    pEmcy = &GL_PVAR(emcyConsList)[0];
# endif /* CONFIG_MULT_LINES */

    /* ignore node 0 */
    if (nodeId == 0)  {
	return(0xff);
    }

# ifdef CONFIG_FAST_SORT

    low = 0;
    high = CO_EMCY_CONS_LINE_CNTS - 1;

#  ifdef CONFIG_MULT_LINES
    pIdx = &GL_PVAR(emcyConsIdxList)[GL_ARRAY(emcyConsLineOffs)];
#  else /* CONFIG_MULT_LINES */
    pIdx = &GL_PVAR(emcyConsIdxList)[0];
#  endif /* CONFIG_MULT_LINES */

    while (found == 0)  {
	if (high >= low) {
	    mid = (high + low) / 2;
	    if (pEmcy[pIdx[mid]].nodeId == nodeId)  {
		found = 1;
	    } else {
		if (pEmcy[pIdx[mid]].nodeId > nodeId) {
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
	return(pIdx[mid]);
    }
# else /* CONFIG_FAST_SORT */

    for (i = 0; i < CO_EMCY_CONS_LINE_CNTS; i++)  {
	/* get the entry */
	if (pEmcy[i].nodeId == nodeId)  {
	    return(i);
	}
    }
    return(0xff);
# endif /* CONFIG_FAST_SORT */
}


/*******************************************************************
*
* searchEmcyCobId - search address of Pdo by cobid
*
* \internal
*
* The function returns the address of an EMCY
* with the given COB-Id
*
* \retval
*	index for cobid list relative to line
* else NULL
*
*/
static UNSIGNED8 searchEmcyConsCobId(
	COB_IDENT_T	cobId		/* value to search for */
	CO_COMMA_LINE_PARA_DECL	/* number of CAN line 0..CO_MAX_CAN_LINES-1 */
    )
{
EMCY_CONS_T	*pEmcy;

# ifdef CONFIG_FAST_SORT
INTEGER8	found = 0;
INTEGER16	low, mid = 0, high;
UNSIGNED8	*pIdx;
# else /* CONFIG_FAST_SORT */
UNSIGNED16	i;
# endif /* CONFIG_FAST_SORT */


# ifdef CONFIG_MULT_LINES
    pEmcy = &GL_PVAR(emcyConsList)[GL_ARRAY(emcyConsLineOffs)];
# else /* CONFIG_MULT_LINES */
    pEmcy = &GL_PVAR(emcyConsList)[0];
# endif /* CONFIG_MULT_LINES */


# ifdef CONFIG_FAST_SORT

    low = 0;
    high = CO_EMCY_CONS_LINE_CNTS - 1;
#  ifdef CONFIG_MULT_LINES
    pIdx = &GL_PVAR(emcyConsCobIdxList)[GL_ARRAY(emcyConsLineOffs)];
#  else /* CONFIG_MULT_LINES */
    pIdx = &GL_PVAR(emcyConsCobIdxList)[0];
#  endif /* CONFIG_MULT_LINES */

    while (found == 0)  {
	if (high >= low) {
	    mid = (high + low) / 2;
	    if (pEmcy[pIdx[mid]].pCOB->cobId == cobId)  {
		found = 1;
	    } else {
		if (pEmcy[pIdx[mid]].pCOB->cobId > cobId) {
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
	return(pIdx[mid]);
    }
# else /* CONFIG_FAST_SORT */

    i = 0;
    while (i < CO_EMCY_CONS_LINE_CNTS)  {
	if (pEmcy[i].pCOB == NULL) {
	    /* COB not defined */
	    return(0xff);
	}
	if (pEmcy[i].pCOB->cobId == cobId) {
	    return((UNSIGNED8)i);
	}
	i++;
    }
    return(0xff);

# endif /* CONFIG_FAST_SORT */
}
#endif /* defined(CONFIG_EMCY_CONSUMER) */


#if defined(CONFIG_EMCY_PRODUCER) || defined(CONFIG_EMCY_CONSUMER)
/*******************************************************************
*
* initEmcyVars - init all EMCY variables
*
* \internal
*
* RETURNS
* \retval nthing
*
*/

void initEmcyVars(
	CO_LINE_PARA_DECL
    )
{
# ifdef CONFIG_EMCY_CONSUMER
#  ifdef CONFIG_MULT_LINES
UNSIGNED8	l;
UNSIGNED16	offs;
#  endif /* CONFIG_MULT_LINES */

    /* clear global variables (some compilers doesn't clear global variables */
#  ifdef CONFIG_CLEAR_CO_GLOBAL_VARS
    memset(&GL_PVAR(emcyConsList)[0], (int)0,
	(size_t)(sizeof(EMCY_CONS_T) * EMCY_CONSUMER_CNT));

#   ifdef CONFIG_FAST_SORT
    memset(&GL_PVAR(emcyConsIdxList)[0], 0x0,
	sizeof(UNSIGNED8) * EMCY_CONSUMER_CNT);
    memset(&GL_PVAR(emcyConsCobIdxList)[0], 0x0,
	sizeof(UNSIGNED8) * EMCY_CONSUMER_CNT);
#   endif /* CONFIG_FAST_SORT */
#  endif /* CONFIG_CLEAR_CO_GLOBAL_VARS */

    GL_ARRAY(emcyConsFlags) = 0;

#  ifdef CONFIG_MULT_LINES
    /* calculate emcy consumer line offsets */
    l = canLine;
    offs = 0;
    while (l > 0)  {
	l--;
	offs += emcyConsLineCnts[l];
    }
    GL_ARRAY(emcyConsLineOffs) = offs;
#  endif /* CONFIG_MULT_LINES */
# endif /* defined(CONFIG_EMCY_CONSUMER) */

# ifdef CONFIG_EMCY_PRODUCER
#  ifdef CONFIG_CLEAR_CO_GLOBAL_VARS
	memset(&GL_ARRAY(co_EmcyProd), 0x0, sizeof(EMCY_T));
#  endif /* CONFIG_CLEAR_CO_GLOBAL_VARS */

    memset(&errorFreeData[0], 0x0, 5);
# endif /* CONFIG_EMCY_PRODUCER */

}
#endif /* defined(CONFIG_EMCY_PRODUCER) || defined(CONFIG_EMCY_CONSUMER) */

/*______________________________________________________________________EOF_*/
