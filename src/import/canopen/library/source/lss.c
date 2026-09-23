/*
 *++ lss - contains LSS service routines
 *-- lss - beinhaltet Funktionen für LSS Dienste
 *
 * Copyright (c) 2002-2017 port GmbH Halle (Saale)
 *------------------------------------------------------------------
 */


/****************************************************************************/
/**
*  \file lss.c
*++  Contains LSS service routines
*--  Beinhaltet Funktionen für LSS Dienste
*  \author port GmbH Halle (Saale)
*
*++ This module contains the functions for handling the
*-- Diese Modul beinhaltet Funktionen für
* Layer Setting Services (LSS)
*++ according to the CiA DSP 305.
*-- entsprechend dem Standard CiA DSP 305.
*
*-- Die CANopen Library unterstützt LSS Master und LSS Slave Geräte.
*-- Der LSS Master muss sich auf demselben Gerät wie der NMT Master befinden.
*-- Eine LSS Kommunikation wird immer vom LSS Master initiiert,
*-- der LSS Slave reagiert nur auf ihn zutreffende Anfragen.
*++ The CANopen library supports LSS Master and LSS Slave devices.
*++ The LSS master has to be on the same device as the NMT master.
*++ A LSS communication is always initialized by a LSS master and
*++ a LSS slave only responds to requests sent to him.
*-- Einzige Ausnahme ist das
*++ The single exception is the
* Ident-Non-Configured-Slave
*-- Protokoll
*++ protocol
* writeLssNonConfigSlaveReq(),
*-- dass von Slaves ohne gültige Node-Id gesendet wird.
*++ that is being sent by slaves without valid node id.
*
*-- Um den LSS Dienst nutzen zu können,
*-- ist die Initialisierungsfunktion
*++ In order to use LSS the initializing function
* defineLss()
*-- mit dem gewünschten Mode aufzurufen,
*-- ansonsten ist keine LSS Kommunikation möglich.
*++ has to be called with the desired mode.
*++ Otherwise no LSS communication is possible.
*
*-- LSS Master Geräte benötigen die Indication Funktion
*++ The indication function
* lssMasterCon()
*-- , die die Antwort des letzen LSS Kommandos zur Verfügung stellt.
*-- Auch im Falle eines Timeouts wird diese Funktion aufgerufen.
*-- Bis zum Eintreffen einer Antwort oder zum Ablauf des Timeouts
*-- kann kein weiteres LSS Kommando gesendet werden.
*++ is needed by LSS master devices.
*++ This function processes the response of the most recently sent LSS command.
*++ It is also called in case of a timeout.
*++ While waiting for a response or a timeout no other LSS command can be
*++ sent.
*
*-- Alle Identifikationsdaten beim LSS Slave
*-- werden aus dem Objekt 0x1018 entnommen
*-- und bei Anforderung automatisch von der Library versendet.
*-- Für Funktionen, die weitergehende Aktionen hervorrufen
*-- (z.B. Setzen der Node-Id oder Bitrate)
*-- wird die Indication Funktion
*++ All data for identification of the LSS slave device are
*++ taken of the object 0x1018.
*++ On request this data is transmitted automatically by the library.
*++ For LSS-functions which require further actions,
*++ e.g. setting of node id or bitrate, the function
* lssSlaveInd()
*-- aufgerufen.
*-- Der Anwender ist für die korrekte Verarbeitung der übergebenen Werte
*-- verantwortlich.
*++ is called.
*++ The user is responsible for the correct processing of the values.
*
*/

/* header of standard C - libraries */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/* header of project specific types */

#include <cal_conf.h>
#include <co_stru.h>
#include <co_mcpy.h>
#include <co_odidx.h>
#include <co_setcp.h>
#include <co_usr.h>
#include "access.h"
#include "lss.h"
#include "drv.h"
#include "nmt.h"
#include "nmterr.h"
#include "sdo.h"
#include "emerg.h"
#include "timer.h"
#include "nmt_s.h"

#ifdef CONFIG_MASTER
#include "nmt_m.h"
#endif /* CONFIG_MASTER */

#ifdef CONFIG_CO_RUN_LED
# include "led.h"
#endif /* CONFIG_CO_RUN_LED */

#ifdef CONFIG_CO_DEBUG
# include <co_debug.h>
#endif /* CONFIG_CO_DEBUG */

#ifdef CONFIG_REDUNDANCY_SUPPORT
# include <co_redcy.h>
#endif /* CONFIG_REDUNDANCY_SUPPORT */

#ifdef CONFIG_NO_GLOBAL_VARS
# include <co_lss.h>
#endif /*CONFIG_NO_GLOBAL_VARS*/



/* constant definitions
---------------------------------------------------------------------------*/
#ifdef CO_LSS_CON_TIMEOUT
# define CON_TIMEOUT	CO_LSS_CON_TIMEOUT
#else /* CO_LSS_CON_TIMEOUT */
# define CON_TIMEOUT	5000	/* LSS confirmation timeout in 1/10 msec */
#endif /* CO_LSS_CON_TIMEOUT */

/* local defined data types
---------------------------------------------------------------------------*/

/* list of external used functions, if not in headers
---------------------------------------------------------------------------*/

/* list of global defined functions
---------------------------------------------------------------------------*/

/* list of local defined functions
---------------------------------------------------------------------------*/

#ifdef CONFIG_LSS_SLAVE
static BOOL_T   checkIdentData(LSS_IDENT_T *lssIdent CO_COMMA_LINE_PARA_DECL);
static void     lssInquiryResponse(UNSIGNED8 cmd, UNSIGNED32 para CO_COMMA_LINE_PARA_DECL);
static void     lssConfigResponse(UNSIGNED8 cmd, UNSIGNED8 errcode, UNSIGNED8 errspec CO_COMMA_LINE_PARA_DECL);
static void     activateNewBitrate(UNSIGNED16 switchTime CO_COMMA_LINE_PARA_DECL);
static void     lssFastScan(CAN_MSG_T *canMsg CO_COMMA_LINE_PARA_DECL);
#endif /* CONFIG_LSS_SLAVE */
#ifdef CONFIG_LSS_MASTER
static void     lssFastScanResponse(CO_LINE_PARA_DECL);
static RET_T    pcoLssUtilCheckFlags( CO_LINE_PARA_DECL );
#endif /* CONFIG_LSS_MASTER */


/* external variables
---------------------------------------------------------------------------*/

/* global variables
---------------------------------------------------------------------------*/

/* local defined variables
---------------------------------------------------------------------------*/
#ifdef CONFIG_RCS_IDENT
CO_LIB_INIT_VAR static char _rcsid[] = "$Id: lss.c,v 2.54 2016/09/26 11:16:07 rli Exp $";
#endif /* CONFIG_RCS_IDENT */

#ifdef CONFIG_NO_GLOBAL_VARS
#else /* CONFIG_NO_GLOBAL_VARS */
# if defined(CONFIG_LSS_MASTER) || defined(CONFIG_LSS_SLAVE)
					/* pointer to COB structs */
CO_LIB_UNINIT_VAR static COB_T		*pLss_TrCOB CO_LINE_PARA_ARRAY_DEF;
CO_LIB_UNINIT_VAR static COB_T		*pLss_RecCOB CO_LINE_PARA_ARRAY_DEF;
					/* save the lss states */
CO_LIB_UNINIT_VAR static FLAG_T	 	lssFlags CO_LINE_PARA_ARRAY_DEF;
					/* pointer to identity object */
CO_LIB_UNINIT_VAR static IDENTITY_T	*pIdentity CO_LINE_PARA_ARRAY_DEF;
					/* timer data structure */
CO_LIB_UNINIT_VAR static TIMER_EVENT_T	lssTimer CO_LINE_PARA_ARRAY_DEF;

CO_LIB_UNINIT_VAR static UNSIGNED32	fastScanBits[4] CO_LINE_PARA_ARRAY_DEF;
CO_LIB_UNINIT_VAR static UNSIGNED8      lssFastPos CO_LINE_PARA_ARRAY_DEF;
# endif /* defined(CONFIG_LSS_MASTER) || defined(CONFIG_LSS_SLAVE) */

# ifdef CONFIG_LSS_MASTER
					/* expected answer from slave */
CO_LIB_UNINIT_VAR static UNSIGNED8	lssExpectAnswer CO_LINE_PARA_ARRAY_DEF;
					/* for fast scan */
CO_LIB_UNINIT_VAR static UNSIGNED8	lssSub CO_LINE_PARA_ARRAY_DEF;
CO_LIB_UNINIT_VAR static UNSIGNED8	bitChecked CO_LINE_PARA_ARRAY_DEF;
CO_LIB_UNINIT_VAR static UNSIGNED8	lssNext CO_LINE_PARA_ARRAY_DEF;
CO_LIB_UNINIT_VAR static UNSIGNED32	lssId CO_LINE_PARA_ARRAY_DEF;
CO_LIB_UNINIT_VAR static UNSIGNED32	fastScanIdent[4] CO_LINE_PARA_ARRAY_DEF;
# endif /* CONFIG_LSS_MASTER */

# ifdef CONFIG_LSS_SLAVE
					/* bitrate error from user indication */
CO_LIB_UNINIT_VAR static UNSIGNED8	lssBitrateSwitchState CO_LINE_PARA_ARRAY_DEF;
CO_LIB_UNINIT_VAR static UNSIGNED8	bitrateErr CO_LINE_PARA_ARRAY_DEF;
CO_LIB_UNINIT_VAR static UNSIGNED8	lssState CO_LINE_PARA_ARRAY_DEF;

# endif /* CONFIG_LSS_SLAVE */
#endif /* CONFIG_NO_GLOBAL_VARS */


#if defined(CONFIG_LSS_MASTER) || defined(CONFIG_LSS_SLAVE)

/****************************************************************************/
/**
*++ \brief defineLss - define the LSS services
*-- \brief defineLss - definiert den LSS Dienst
*
*-- Diese Funktion initialisiert den LSS Dienst.
*-- Wenn der Knoten als LSS Master initialisiert werden soll,
*-- prüft die Funktion, ob der Knoten der NMT Master ist.
*-- Deshalb darf diese Funktion erst nach dem Einrichten der NMT Master
*-- Funktionalität aufgerufen werden.
*++ This function initializes LSS.
*++ If the node is to be configured as a LSS master this function checks
*++ if the node is a NMT master, too.
*++ Therefore this function should only be called
*++ after setting up the NMT mster functionality.
*
* \retval CO_OK
*++ success
*-- Erfolg
* \retval CO_E_TRANS_TYPE
*-- nicht unterstützter Mode
*++ non-suported mode
* \retval CO_E_MEM
*-- kein Speicher verfügbar
*++ out of memory
* \retval CO_E_NONEXIST_OBJECT
*-- Identity Objekt nicht gefunden
*++ identity object not available
*
*/

RET_T defineLss(
	UNSIGNED8  kind		/**< kind of LSS MASTER/SLAVE */
	CO_COMMA_LINE_PARA_DECL	/**< number of CAN line 0..CO_MAX_CAN_LINES-1 */
    )
{
LIST_ELEMENT_T	*curObj;
RET_T		retVal;

    GL_ARRAY(lssFastPos) = 0xff;

# ifdef CONFIG_MASTER		/* check for configured master device */
# else /* CONFIG_LSS_MASTER */
    if (kind == LSS_MASTER)  {
	return(CO_E_TRANS_TYPE);
    }
# endif /* CONFIG_LSS_MASTER */

    /* get address of identity object */
    curObj = searchObj(IDENTITY_INDEX CO_COMMA_LINE_PARA);
    if (curObj == NULL) {
	/* object doesn't exist */
	return(CO_E_NONEXIST_OBJECT);
    }
    /* save it for fast automatically responses */
    GL_ARRAY(pIdentity) = (IDENTITY_T *)CO_READ_ODP(curObj->pObj);

    /* define the COB-structures */
    GL_ARRAY(pLss_TrCOB) = DEFINE_COB(CO_COB_LSS_TX, 8 CO_COMMA_LINE_PARA);
    GL_ARRAY(pLss_RecCOB) = DEFINE_COB(CO_COB_LSS_RX, 8 CO_COMMA_LINE_PARA);

    /* if an error occured, return */
    if ((GL_ARRAY(pLss_TrCOB) == NULL) || (GL_ARRAY(pLss_RecCOB) == NULL)) {
	GL_ARRAY(pIdentity) = 0;
	retVal = CO_E_NO_DATABASE;
    } else {
        retVal = setLssMode(kind CO_COMMA_LINE_PARA);
    }

    return(retVal);
}
#endif /* defined(CONFIG_LSS_MASTER) || defined(CONFIG_LSS_SLAVE) */


#if defined(CONFIG_LSS_MASTER) || defined(CONFIG_LSS_SLAVE)
/****************************************************************************/
/**
*++ \brief setLssMode - set LSS mode to master or slave
*-- \brief setLssMode - setzt LSS mode zu master oder slave
*
*-- Diese Funktion setzt den LSS Dienst zu dem übergebenen Mode.
*-- Dazu ist er vorher einmalig mit defineLss() zu initialisieren.
*-- Wenn der Knoten als LSS Master initialisiert werden soll
*-- prüft die Funktion, ob der Knoten der NMT Master ist.
*-- Ansonsten kehrt sie mit einem Fehler zurück.
*++ This function changes the LSS services to the given mode.
*++ Before this function can be used, it have to be initialized by defineLss().
*++ If LSS master mode is requested,
*++ the function checks if the node itself is the NMT master.
*
* \retval CO_OK
*++ success
*-- Erfolg
* \retval CO_E_TRANS_TYPE
*-- nicht unterstützter Mode
*++ non-suported mode
* \retval CO_E_NO_INITIATE
*++ Service not initialized
*-- Dienst nicht initialisiert
*/
RET_T setLssMode(
	UNSIGNED8  kind		/**< kind of LSS MASTER/SLAVE */
	CO_COMMA_LINE_PARA_DECL	/**< number of CAN line 0..CO_MAX_CAN_LINES-1 */
    )
{
    /* check for valid initialisation */
    if (GL_ARRAY(pIdentity) == NULL)  {
	return(CO_E_NO_INITIATE);
    }

    /* if lss master request */
    if (kind == LSS_MASTER) {
	/* and we are not the nmt master */
	if ((GL_ARRAY(co_Node).flags & NMTERRFLAG_MASTER) == 0)  {
	    /* return error */
	    return(CO_E_TRANS_TYPE);
	}
    }

    /* reset all flags */
    GL_ARRAY(lssFlags) = 0;

    /* set COB-IDs */
    if (kind == LSS_MASTER)  {
	(void) SET_COB_ID(GL_ARRAY(pLss_TrCOB), CO_COBID_LSS_REQ, CO_COB_LSS_TX);
	(void) SET_COB_ID(GL_ARRAY(pLss_RecCOB), CO_COBID_LSS_CON, CO_COB_LSS_RX);

	GL_ARRAY(lssFlags) |= LSS_FLAGS_MASTER;
    } else {
	(void) SET_COB_ID(GL_ARRAY(pLss_RecCOB), CO_COBID_LSS_REQ, CO_COB_LSS_RX);
	(void) SET_COB_ID(GL_ARRAY(pLss_TrCOB), CO_COBID_LSS_CON, CO_COB_LSS_TX);

# ifdef CONFIG_LSS_SLAVE
	GL_ARRAY(lssState) = LSS_STATE_NONE;
	GL_ARRAY(bitrateErr) = 255;
# endif /* CONFIG_LSS_SLAVE */
    }

    return(CO_OK);
}
#endif /* defined(CONFIG_LSS_MASTER) || defined(CONFIG_LSS_SLAVE) */


#if defined(CONFIG_LSS_MASTER) || defined(CONFIG_LSS_SLAVE)
/****************************************************************************/
/*
*++ \brief initLss - reinit LSS service
*-- \brief initLss - reinitialisert LSS service
*
*	\internal
*
* Diese Funktion reinitialisert den LSS Dienst
* zu dem zuvor eingestellten Mode.
* Wenn Flying Master aktiv ist,
* wird LSS auf den Slave Mode gesetzt.
*
*/

void initLss(
	CO_LINE_PARA_DECL	/* number of CAN line 0..CO_MAX_CAN_LINES-1 */
    )
{
#ifdef CONFIG_FLYING_MASTER
    (void) setLssMode(LSS_SLAVE CO_COMMA_LINE_PARA);
#else /* CONFIG_FLYING_MASTER */
    /* check for valid master initialisation */
    if ((GL_ARRAY(lssFlags) & LSS_FLAGS_MASTER) != 0)  {
	(void) setLssMode(LSS_MASTER CO_COMMA_LINE_PARA);
    } else {
	(void) setLssMode(LSS_SLAVE CO_COMMA_LINE_PARA);
    }
#endif /* CONFIG_FLYING_MASTER */
}

#endif /* defined(CONFIG_LSS_MASTER) || defined(CONFIG_LSS_SLAVE) */


#if defined(CONFIG_LSS_MASTER) && defined(CONFIG_LSS_SLAVE)
/****************************************************************************/
/*
*++ \brief lssMsgReceived - lss message received
*-- \brief lssMsgReceived - lss Message erhalten
*
*
* \internal
*
*-- Diese Funktion bearbeitet die empfangenen LSS Nachrichten
*-- vom Master oder Slave entsprechend dem initialisierten Mode
*-- und ruft die Master Konfirmation bzw. die Slave Request Funktion auf.
*++ This function processes the received lss messages from master or slave
*++ according to the initialized mode
*++ and calls the master confirmation or the slave request function.
*
*
*/

void lssMsgReceived(
	CAN_MSG_T *canMsg	/* Pointer to CAN Message */
	CO_COMMA_LINE_PARA_DECL	/* number of CAN line 0..CO_MAX_CAN_LINES-1 */
    )
{
    if (canMsg->cobId == CO_COBID_LSS_CON)  {
	/* check for master initialisation */
	if ((GL_ARRAY(lssFlags) & LSS_FLAGS_MASTER) != 0)  {
	    lssConMsgReceived(canMsg CO_COMMA_LINE_PARA);
	}
    }
    if (canMsg->cobId == CO_COBID_LSS_REQ)  {
	/* check for slave initialisation */
	if ((GL_ARRAY(lssFlags) & LSS_FLAGS_MASTER) == 0)  {
	    lssReqMsgReceived(canMsg CO_COMMA_LINE_PARA);
	}
    }
}

#endif /* defined(CONFIG_LSS_MASTER) && defined(CONFIG_LSS_SLAVE) */


#ifdef CONFIG_LSS_MASTER

/****************************************************************************/
/*
*++ \brief lssConMsgReceived - lss conformation message received
*-- \brief lssConMsgReceived - lss Conformation Message erhalten
*
*
* \internal
*
*-- Diese Funktion bearbeitet die empfangenen LSS Nachrichten
*-- vom Slave.
*++ This function processes the received lss messages from the slave.
*
*-- Bei LSS Master Geräten wird bei einer ausstehenden Antwort
*-- die Indikationsfunktion
*++ On LSS master devices the function
* lssMasterInd()
*-- aufgerufen.
*-- Geichzeitig wird der Überwachungstimer rückgesetzt.
*++ is called for a pending message.
*++ The monitoring timer for LSS commands is reset.
*
*
*/

void lssConMsgReceived(
	CAN_MSG_T *canMsg	/* Pointer to CAN Message */
	CO_COMMA_LINE_PARA_DECL	/* number of CAN line 0..CO_MAX_CAN_LINES-1 */
      )
{
UNSIGNED32	u32;		/* temorary u32 value */
UNSIGNED8	*par1 = NULL, *par2 = NULL;	/* pointer to parameter */
UNSIGNED8	mode;		/* mode */

    /* check for valid initialisation */
    if (GL_ARRAY(pIdentity) == NULL)  {
	return;
    }

    /* check for valid master initialisation */
    if ((GL_ARRAY(lssFlags) & LSS_FLAGS_MASTER) == 0)  {
	return;
    }

    /* ignore old messages */
    if ((GL_ARRAY(lssFlags) & LSS_FLAGS_WAITING) == 0)  {
	/* if this an unconfigured slave message ? */
	if (canMsg->pData[0] == LSS_CS_IDENT_SLAVE_CFG)  {
	    /* no */
	    lssMasterCon(LSS_CON_UNCONFIG_NODE, par1, par2
		    CO_COMMA_LINE_PARA);
	}
	return;
    }

    /* are we waiting for this answer ? */
    if (GL_ARRAY(lssExpectAnswer) != canMsg->pData[0])  {
	return;
    }

    /* fast scan active ? */
    if ((GL_ARRAY(lssFlags) & LSS_FLAGS_FAST_SCAN) != 0)  {
	/* save answer */
	GL_ARRAY(lssFlags) |= LSS_FLAGS_FAST_SCAN_ANSWER;
	return;
    }

    /* reset timeout timer */
    removeTimerEvent(&GL_ARRAY(lssTimer) CO_COMMA_LINE_PARA);
    /* delete wait for answer */
    GL_ARRAY(lssFlags) &= (FLAG_T)(~LSS_FLAGS_WAITING);

    /* call indication function */
    switch (canMsg->pData[0]) {
	case LSS_CS_SWITCH_SEL:		/* switch selektive */
	case LSS_CS_IDENT_SLAVE:	/* ident slave */
	case LSS_CS_IDENT_SLAVE_CFG:	/* ident slave without config */
	    mode = LSS_CON_ANSWER_OK;
	    break;

	case LSS_CS_SET_NODEID:		/* set node id */
	case LSS_CS_SET_BITRATE:	/* set bit rate */
	case LSS_CS_STORE_CFG:		/* store configuration*/
	    if (canMsg->pData[1] == LSS_ERROR_OK) {
		mode = LSS_CON_ANSWER_OK;
	    } else {
		mode = LSS_CON_ANSWER_ERROR;
		par1 = &canMsg->pData[1];
		par2 = &canMsg->pData[2];
	    }
	    break;

	case LSS_CS_INQ_VENDOR:		/* inquire vendor */
	case LSS_CS_INQ_PRODUCT:	/* inquire product */
	case LSS_CS_INQ_REV:		/* inquire revision */
	case LSS_CS_INQ_SNR:		/* inquire snr */
	    mode = LSS_CON_ANSWER_DATA;
	    CO_PACK_MEMCPY(&u32, &canMsg->pData[1], 4, CO_TRUE);
	    par1 = (UNSIGNED8 *)&u32;
	    break;

	case LSS_CS_INQ_NODEID:		/* inquire node id */
	    mode = LSS_CON_ANSWER_NODEID;
	    par1 = &canMsg->pData[1];
	    break;

	default:
	    mode = LSS_CON_TIMEOUT;
	    break;
    }
    lssMasterCon(mode, par1, par2 CO_COMMA_LINE_PARA);
}
#endif /* CONFIG_LSS_MASTER */


#ifdef CONFIG_LSS_MASTER
/****************************************************************************/
/**
*++ \brief writeLssSwitchModeReq - switch mode for one/all nodes
*-- \brief writeLssSwitchModeReq - Mode Umschaltung für einen/alle Knoten
*
*-- Diese Funktion setzt den vorgegebenen Mode für einen oder
*-- alle Knoten im Netzwerk.
*-- Wenn der Parameter \em vendor null ist,
*-- wird das globale Umschaltkommando
*-- entsprechend dem übergebenem Parameter \em mode versendet.
*++ This function switches to a given mode
*++ for one or all network nodes.
*++ If the parameter \em vendor is zero then
*++ the according global switch command
*++ with its approriate \em mode parameter is transmitted.
*-- Dabei bedeutet:
*++ Possible values for mode:
* \li 0 - operation mode
* \li 1 - config mode
*
*-- Beim selektiven Umschalten (Parameter vendor != 0)
*-- werden die übergebenen Daten versendet
*-- und nur der selektierte Knoten
*-- in den Configuration Mode versetzt.
*++ On a selective mode switching (parameter vendor != 0)
*++ the data is transmitted and
*++ only the selected node is set in configuration mode.
*
*-- Zum Umschalten zurück in den Operation Mode
*-- wird immer das globale Kommando versendet.
*++ For switching back to operation mode
*++ the global switching command is always used.
*
*-- Nur das selektive Umschaltkommando erfordert eine Reaktion des Slaves.
*-- Daher wird die Funktion
*++ Only the selective switching command requires a reaction of a slave.
*++ Therefore the function
* lssMasterInd()
*-- nur bei diesem Kommando aufgerufen.
*++ is called only on reception of this command.
*
* \retval CO_OK
*++ success
*-- Erfolg
* \retval CO_E_BUSY
*++ LSS is waiting for answer from slave
*-- LSS wartet noch auf Antwort vom Slave
* \retval CO_E_NO_INITIATE
*++ Service not initialized
*-- Dienst nicht initilaisiert
* \retval CO_E_PARA_INCOMP
*++ switch selective not allowed for waiting mode
*-- Selektives Umschalten in wait mode nicht erlaubt
*/

RET_T writeLssSwitchModeReq(
	UNSIGNED32  vendor,	/**< 0 - all nodes, != 0 vendor Id */
	UNSIGNED32  product,	/**< product code */
	UNSIGNED32  revision,	/**< revisioncode */
	UNSIGNED32  snr,	/**< serial number */
	UNSIGNED8   mode	/**< LSS_CONFIG_MODE, LSS_WAITING_MODE */
	CO_COMMA_LINE_PARA_DECL	/**< number of CAN line 0..CO_MAX_CAN_LINES-1 */
    )
{
RET_T           retVal = CO_OK;
UNSIGNED8	bData[8];

    retVal = pcoLssUtilCheckFlags( CO_LINE_PARA );

    if ( CO_OK == retVal ) {

        /* reset all data */
        memset(&bData[0], (int)0, (size_t)8);

        if (vendor == 0)  {
	    /* switch mode global */
	    bData[0] = LSS_CS_SWITCH_GLOBAL;
	    if (mode == LSS_SWITCH_MODE_CFG)  {
	        bData[1] = mode;
	    } else {
	        bData[1] = LSS_SWITCH_MODE_WAIT;
	    }
	    (void)TRANSMIT_COB(GL_ARRAY(pLss_TrCOB), &bData[0]);
        } else {
	    if (mode != LSS_SWITCH_MODE_CFG)  {
	        retVal = CO_E_PARA_INCOMP;
	    } else {
	        /* switch mode selective */
	        TRANS_LSS_DATA(LSS_CS_SWITCH_SEL_VENDOR, vendor);
	        TRANS_LSS_DATA(LSS_CS_SWITCH_SEL_PROD, product);
	        TRANS_LSS_DATA(LSS_CS_SWITCH_SEL_REV, revision);
	        TRANS_LSS_DATA(LSS_CS_SWITCH_SEL_SNR, snr);

	        LSS_WAIT_FOR_ANSWER(LSS_CS_SWITCH_SEL);
            }
        }
    }

    return(retVal);
}
#endif /* CONFIG_LSS_MASTER */


#ifdef CONFIG_LSS_MASTER
/****************************************************************************/
/**
*++ \brief writeLssConfigNodeIdReq - set node id for one node
*-- \brief writeLssConfigNodeIdReq - setzt die Node-Id für einen Knoten
*
*-- Diese Funktion setzt die Node-Id für einen Knoten.
*-- Der Knoten muss vorher durch die Funktion
*++ This function sets the node id of a node.
*++ The node has to be set in configuration mode with the function
* writeLssSwitchModeReq()
*-- in den Configuration Mode gesetzt worden sein.
*-- Ausserdem darf sich kein weiterer Knoten im Configuration Mode befinden.
*++ previously.
*++ Additionally no other node is allowed to be in the configuration mode.
*
*-- Der angesprochene Knoten quittiert das Setzen der Knoten Nummer
*-- mit einem speziellen Antworttelegramm.
*-- Das Ergebnis dieser Quittungsmeldung wird durch die Funktion
*++ The requested node acknowledges the new node id
*++ with a special respond message.
*++ The result of this respond message can be processed
*++ with the indication function
* lssMasterCon()
*-- angezeigt.
*
* \retval CO_OK
*++ success
*-- Erfolg
* \retval CO_E_BUSY
*++ LSS is waiting for answer from slave
*-- LSS wartet noch auf Antwort vom Slave
* \retval CO_E_NO_INITIATE
*++ Service not initialized
*-- Dienst nicht initialisiert
*/

RET_T writeLssConfigNodeIdReq(
	UNSIGNED8   nodeId	/**< node id */
	CO_COMMA_LINE_PARA_DECL	/**< number of CAN line 0..CO_MAX_CAN_LINES-1 */
      )
{
RET_T           retVal = CO_OK;
UNSIGNED8	bData[8];

    retVal = pcoLssUtilCheckFlags(CO_LINE_PARA);

    if ( CO_OK == retVal ) {
        /* reset data */
        memset(&bData[0], (int)0, (size_t)8);
        bData[0] = LSS_CS_SET_NODEID;
        bData[1] = nodeId;
        (void)TRANSMIT_COB(GL_ARRAY(pLss_TrCOB), &bData[0]);

        LSS_WAIT_FOR_ANSWER(LSS_CS_SET_NODEID);
    }

    return(retVal);
}
#endif /* CONFIG_LSS_MASTER */


#ifdef CONFIG_LSS_MASTER
/****************************************************************************/
/**
*++ \brief writeLssIdentNonCfgReq - send command ident nonconfig slaves
*-- \brief writeLssIdentNonCfgReq - sendet Kommando ident nonconfig slaves
*
*-- Diese Funktion sendet das Kommando identify non configured slaves.
*++ This function sends the command identify non configured slaves.
*
*-- Alle Knoten ohne gültige Node-Id quittieren das Kommando
*-- mit einen speziellen Antworttelegramm.
*-- Das Ergebnis dieser Quittungsmeldung wird durch die Funktion
*++ All nodes without a valid node id acknowledge this
*++ with a special respond message.
*++ The result of this respond message can be processed
*++ with the indication function
* lssMasterCon()
*-- angezeigt.
*
* \retval CO_OK
*++ success
*-- Erfolg
* \retval CO_E_BUSY
*++ LSS is waiting for answer from slave
*-- LSS wartet noch auf Antwort vom Slave
* \retval CO_E_NO_INITIATE
*++ Service not initialized
*-- Dienst nicht initialisiert
*/

RET_T writeLssIdentNonCfgReq(
	CO_LINE_PARA_DECL	/**< number of CAN line 0..CO_MAX_CAN_LINES-1 */
      )
{
RET_T           retVal = CO_OK;
UNSIGNED8	bData[8];

    retVal = pcoLssUtilCheckFlags(CO_LINE_PARA);

    if ( CO_OK == retVal ) {
        /* reset data */
        memset(&bData[0], (int)0, (size_t)8);
        bData[0] = LSS_CS_IDENT_SLAVE_NOCFG;
        (void)TRANSMIT_COB(GL_ARRAY(pLss_TrCOB), &bData[0]);

        LSS_WAIT_FOR_ANSWER(LSS_CS_IDENT_SLAVE_CFG);
    }
    return(retVal);
}
#endif /* CONFIG_LSS_MASTER */


#ifdef CONFIG_LSS_MASTER
/****************************************************************************/
/**
*++ \brief writeLssStoreReq - send command store config
*-- \brief writeLssStoreReq - sendet Kommando store config
*
*-- Diese Funktion sendet das Kommando store config
*-- zu den aktuell ausgewähltem Slave(s).
*-- Der Knoten muss vorher durch die Funktion
*++ This function sends the command store config
*++ to the selected nodes.
*++ The node has to be set in configuration mode with the function
* writeLssSwitchModeReq()
*-- in den Configuration Mode gesetzt worden sein.
*
*-- Der angesprochene Knoten quittiert das Kommando
*-- mit einem speziellen Antworttelegramm.
*-- Das Ergebnis dieser Quittungsmeldung wird durch die Funktion
*++ The requested node acknowledges the command
*++ with a special respond message.
*++ The result of this respond message can be processed
*++ with the indication function
* lssMasterCon()
*-- angezeigt.
*
* \retval CO_OK
*++ success
*-- Erfolg
* \retval CO_E_BUSY
*++ LSS is waiting for answer from slave
*-- LSS wartet noch auf Antwort vom Slave
* \retval CO_E_NO_INITIATE
*++ Service not initialized
*-- Dienst nicht initialisiert
*/

RET_T writeLssStoreReq(
	CO_LINE_PARA_DECL	/**< number of CAN line 0..CO_MAX_CAN_LINES-1 */
      )
{
RET_T           retVal = CO_OK;
UNSIGNED8	bData[8];

    retVal = pcoLssUtilCheckFlags(CO_LINE_PARA);

    if ( CO_OK == retVal ) {

        /* reset data */
        memset(&bData[0], (int)0, (size_t)8);
        bData[0] = LSS_CS_STORE_CFG;
        (void)TRANSMIT_COB(GL_ARRAY(pLss_TrCOB), &bData[0]);

        LSS_WAIT_FOR_ANSWER(LSS_CS_STORE_CFG);
    }

    return(retVal);
}
#endif /* CONFIG_LSS_MASTER */


#ifdef CONFIG_LSS_MASTER
/****************************************************************************/
/**
*++ \brief writeLssConfigBitrateReq - set bitrate for selected nodes
*-- \brief writeLssConfigBitrateReq - setzt die Bitrate für ausgewählte Knoten
*
*-- Diese Funktion setzt die Bitrate für vorher ausgewählte Knoten.
*-- Die Knoten müssen vorher durch die Funktion
*++ This Funktion sets the bitrate of selected network nodes.
*++ The nodes have to be set to configuration mode with the function
* writeLssSwitchModeReq()
*-- in den Configuration Mode gesetzt worden sein.
*
*-- Der angesprochene Knoten quittiert die neue Bitrate
*-- mit einen speziellen Antworttelegramm.
*-- Das Ergebnis dieser Quittungsmeldung wird durch die Funktion
*++ The requested node accepts the new bitrate
*++ with a special respond message.
*++ The result of this respond message can be processed
*++ with the indication function
* lssMasterCon()
*-- angezeigt.
*
*-- CANopen Standardbitraten:
*++ CANopen standard bitrates:
* \code
* index	bitrate
* 0	1000 kBit
* 1	800 kBit
* 2	500 kBit
* 3	250 kBit
* 4	125 kBit
* 5	100 kBit
* 6	50 kBit
* 7	20 kBit
* 8	10 kBit
* \endcode
*
*++ After setting the bitrate
*++ it has to be activated with the function
*-- Nach dem Setzen der Bitrate muss ein
* writeLssActivateBitrateReq()
*-- folgen.
*
* \code
* stopRemoteNodeReq(0);			// set all nodes to State STOPPED
*					// select one node
* writeLssSwitchModeReq(vendor, prod, rev, snr, LSS_CONFIG_MODE)
* writeLssConfigBitrateReq(0, 3);	// set new bitrate from CANopen table
* while (getLssState() == CO_LSS_BUSY)  {	// wait for answer or timeout
* 	FlushMbox();			// from slave
* }
* writeLssActivateBitrateReq(300);	// activate new bitrate
* \endcode
*
* \retval CO_OK
*++ success
*-- Erfolg
* \retval CO_E_BUSY
*++ LSS is waiting for answer from slave
*-- LSS wartet noch auf Antwort vom Slave
* \retval CO_E_NO_INITIATE
*++ Service not initialized
*-- Dienst nicht initialisiert
*/

RET_T writeLssConfigBitrateReq(
	UNSIGNED8   table,	/**< table index */
	UNSIGNED8   index	/**< table index */
	CO_COMMA_LINE_PARA_DECL	/**< number of CAN line 0..CO_MAX_CAN_LINES-1 */
      )
{
RET_T           retVal = CO_OK;
UNSIGNED8	bData[8];


    retVal = pcoLssUtilCheckFlags(CO_LINE_PARA);

    if ( CO_OK == retVal ) {

        /* reset data */
        memset(&bData[0], (int)0, (size_t)8);
        bData[0] = LSS_CS_SET_BITRATE;
        bData[1] = table;
        bData[2] = index;
        (void)TRANSMIT_COB(GL_ARRAY(pLss_TrCOB), &bData[0]);

        LSS_WAIT_FOR_ANSWER(LSS_CS_SET_BITRATE);
    }

    return(retVal);
}
#endif /* CONFIG_LSS_MASTER */


#ifdef CONFIG_LSS_MASTER
/****************************************************************************/
/**
*++ \brief writeLssActivateBitrateReq - activate bitrate for selected nodes
*-- \brief writeLssActivateBitrateReq - Aktiviert Bitrate für ausgewählte Knoten
*
*-- Diese Funktion aktiviert die Bitrate für vorher ausgewählte Knoten.
*-- Die Knoten müssen vorher durch die Funktion
*++ This function sets the bitrate of selected network nodes.
*++ The nodes have to be set to configuration mode previously with the function
* writeLssSwitchModeReq()
*-- in den Configuration Mode gesetzt worden sein,
*-- und es muss eine neue Bitrate mit Hilfe der Funktion
*++ and the bitrate has to be set with the function
* writeLssConfigBitrateReq()
*-- gesetzt worden sein.
*
*-- Das Setzen der neuen Bitrate bei den Slaves erfolgt in 2 Phasen:
*++ Setting a new bitrate takes place in 2 phases at the slaves.
* \n
*-- 1. Slave wartet switchDelay msec (kein Senden mehr erlaubt)
*++ 1. Slave waits switchDelay msec (sending is not allowed)
* \n
*-- 2. Slave schaltet seine Bitrate um
*++ 2. Slave switches bitrate
* \n
*-- 3. Slave wartet nochmals switchDelay msec
*++ 3. Slaves wait another switchDelay msec
* \n
*-- 4. Weiteres Senden erlaubt
*++ 4. Sending is allowed again
*
* \retval CO_OK
*++ success
*-- Erfolg
* \retval CO_E_BUSY
*++ LSS is waiting for answer from slave
*-- LSS wartet noch auf Antwort vom Slave
* \retval CO_E_NO_INITIATE
*++ Service not initialized
*-- Dienst nicht initialisiert
*
*/

RET_T writeLssActivateBitrateReq(
	UNSIGNED16  switchDelay	/**< activation delay */
	CO_COMMA_LINE_PARA_DECL	/**< number of CAN line 0..CO_MAX_CAN_LINES-1 */
      )
{
RET_T           retVal = CO_OK;
UNSIGNED8	bData[8];

    retVal = pcoLssUtilCheckFlags(CO_LINE_PARA);

    if ( CO_OK == retVal ) {

        /* reset data */
        memset(&bData[0], (int)0, (size_t)8);
        bData[0] = LSS_CS_ACTIVATE_BITRATE;
        bData[1] = (UNSIGNED8)(switchDelay & 0xff);
        bData[2] = (UNSIGNED8)((switchDelay >> 8) & 0xff);
        (void)TRANSMIT_COB(GL_ARRAY(pLss_TrCOB), &bData[0]);
    }

    return(retVal);
}
#endif /* CONFIG_LSS_MASTER */


#ifdef CONFIG_LSS_MASTER
/****************************************************************************/
/**
*++ \brief writeLssInquiryReq - inquiry slaves
*-- \brief writeLssInquiryReq - ermittle Slaves
*
*-- Diese Funktion ermittelt Knoten mit Hilfe der vorgegebenen
*-- Hersteller-Id, Produktcode, Revisionsnummer, Seriennummer oder Knotennummer.
*-- Die Knoten müssen vorher durch die Funktion
*++ This function detects the nodes with the help of
*++ vendor-id, product code, revision number, serial nummer or node-id.
*++ The nodes have to be set to configuration mode before with the function
* writeLssSwitchModeReq()
*-- in den Configuration Mode gesetzt worden sein.
*
*-- Falls es Knoten mit den vorgegebenen Eigenschaften im Netzwerk gibt,
*-- quittieren die Knoten die Anfrage
*-- mit einen speziellen Antworttelegramm.
*-- Das Ergebnis dieser Quittungsmeldung wird durch die Funktion
*++ If there are nodes with the defined properties, e.g. vendor id etc.,
*++ then this nodes will acknowledge the request
*++ with a special response message.
*++ The result of the response message can be processed
*++ in the indication function
* lssMasterCon()
*-- angezeigt.
*
*-- Der Parameter mode kann folgende Bedeutung haben:
*++ The parameter mode can have the following values:
*++ LSS_VENDOR - search for vendor-Id
*-- LSS_VENDOR - suche nach Herstellernummer
*++ LSS_PRODUCT - search for product code
*-- LSS_PRODUCT - suche nach Produktcode
*++ LSS_REVISION - search for revision number
*-- LSS_REVISION - suche nach Revisionsnummer
*++ LSS_SNR - search for serial number
*-- LSS_SNR - suche nach Seriennummer
*++ LSS_NODEID - search for node id
*-- LSS_NODEID - suche nach node id
*
* \retval CO_OK
*++ success
*-- Erfolg
* \retval CO_E_BUSY
*++ LSS is waiting for an answer from slave
*-- LSS wartet noch auf Antwort vom Slave
* \retval CO_E_NO_INITIATE
*++ Service not initialized
*-- Dienst nicht initialisiert
* \retval CO_E_STATE
*++ unknown mode
*-- unbekannter Mode
*/

RET_T writeLssInquiryReq(
	UNSIGNED8   mode	/**< mode */
	CO_COMMA_LINE_PARA_DECL	/**< number of CAN line 0..CO_MAX_CAN_LINES-1 */
      )
{
RET_T           retVal = CO_OK;
UNSIGNED8	bData[8];

    retVal = pcoLssUtilCheckFlags(CO_LINE_PARA);

    if ( CO_OK == retVal ) {

        /* delete data */
        memset(&bData[0], (int)0, (size_t)8);
        /* data depends to given mode */
        switch (mode)  {
	    case LSS_VENDOR:
	        bData[0] = LSS_CS_INQ_VENDOR;
                /* send message */
                (void)TRANSMIT_COB(GL_ARRAY(pLss_TrCOB), &bData[0]);
                /* wait for answert */
                LSS_WAIT_FOR_ANSWER(bData[0]);
	        break;

	    case LSS_PRODUCT:
	        bData[0] = LSS_CS_INQ_PRODUCT;
                /* send message */
                (void)TRANSMIT_COB(GL_ARRAY(pLss_TrCOB), &bData[0]);
                /* wait for answert */
                LSS_WAIT_FOR_ANSWER(bData[0]);
	        break;

	    case LSS_REVISION:
	        bData[0] = LSS_CS_INQ_REV;
                /* send message */
                (void)TRANSMIT_COB(GL_ARRAY(pLss_TrCOB), &bData[0]);
                /* wait for answert */
                LSS_WAIT_FOR_ANSWER(bData[0]);
	        break;

	    case LSS_SNR:
	        bData[0] = LSS_CS_INQ_SNR;
                /* send message */
                (void)TRANSMIT_COB(GL_ARRAY(pLss_TrCOB), &bData[0]);
                /* wait for answert */
                LSS_WAIT_FOR_ANSWER(bData[0]);
	        break;

	    case LSS_NODEID:
	        bData[0] = LSS_CS_INQ_NODEID;
                /* send message */
                (void)TRANSMIT_COB(GL_ARRAY(pLss_TrCOB), &bData[0]);
                /* wait for answert */
                LSS_WAIT_FOR_ANSWER(bData[0]);
	        break;

	    default:
	        retVal = CO_E_STATE;
                break;
        }
    }

    return(retVal);
}
#endif /* CONFIG_LSS_MASTER */


#ifdef CONFIG_LSS_MASTER
/****************************************************************************/
/**
*++ \brief writeLssIdentityReq - identify slaves
*-- \brief writeLssIdentityReq - ermittle Knoten über Identity
*
*-- Diese Funktion ermittelt Knoten mit Hilfe
*-- der vorgegebenen Hersteller-Id, Produktcode
*-- und dem vorgegeben Revisionsnummer und Seriennummern Bereich.
*-- Die Knoten müssen vorher durch die Funktion
*++ This function detects network nodes with the help of
*++ vendor-id, product code and the
*++ revision number range and serial number range.
*++ The network nodes have to be set in configuration mode
*++ with the function
* writeLssSwitchModeReq()
*-- in den Configuration Mode gesetzt worden sein.
*
*-- Falls es Knoten mit den vorgegebenen Eigenschaften im Netzwerk gibt,
*-- quittieren die Knoten die Anfrage
*-- mit einen speziellen Antworttelegramm.
*-- Das Ergebnis dieser Quittungsmeldung wird durch die Funktion
*++ If there are nodes with the defined properties, e.g. vendor id etc.,
*++ then this nodes will acknowledge the request
*++ with a special response message.
*++ The result of the response message can be processed
*++ in the indication function
* lssMasterCon()
*-- angezeigt.
*
*
* \retval CO_OK
*++ success
*-- Erfolg
* \retval CO_E_BUSY
*++ LSS is waiting for answer from slave
*-- LSS wartet noch auf Antwort vom Slave
* \retval CO_E_NO_INITIATE
*++ Service not initialzed
*-- Dienst nicht initialisiert
*/

RET_T writeLssIdentityReq(
	UNSIGNED32  vendor,	/**< 0 - all nodes, != 0 vendor Id */
	UNSIGNED32  product,	/**< product code */
	UNSIGNED32  revision_low,/**< lowest revisioncode */
	UNSIGNED32  revision_high,/**< highest revisioncode */
	UNSIGNED32  snr_low,	/**< lowest serial number */
	UNSIGNED32  snr_high	/**< hihest serial number */
	CO_COMMA_LINE_PARA_DECL	/**< number of CAN line 0..CO_MAX_CAN_LINES-1 */
      )
{
RET_T           retVal = CO_OK;
UNSIGNED8	bData[8];

    retVal = pcoLssUtilCheckFlags(CO_LINE_PARA);

    if ( CO_OK == retVal ) {

        /* reset data */
        memset(&bData[0], (int)0, (size_t)8);
        /* transmit data */
        TRANS_LSS_DATA(LSS_CS_IDENT_SLAVE_VENDOR, vendor);
        TRANS_LSS_DATA(LSS_CS_IDENT_SLAVE_PRODUCT, product);
        TRANS_LSS_DATA(LSS_CS_IDENT_SLAVE_REV_LOW, revision_low);
        TRANS_LSS_DATA(LSS_CS_IDENT_SLAVE_REV_HIGH, revision_high);
        TRANS_LSS_DATA(LSS_CS_IDENT_SLAVE_SNR_LOW, snr_low);
        TRANS_LSS_DATA(LSS_CS_IDENT_SLAVE_SNR_HIGH, snr_high);

        /* wait for answer */
        LSS_WAIT_FOR_ANSWER(LSS_CS_IDENT_SLAVE);
    }
    return(retVal);
}
#endif /* CONFIG_LSS_MASTER */


#ifdef CONFIG_LSS_MASTER
/****************************************************************************/
/**
*++ \brief writeLssFastScanReq - detect nodes by  LSS FastScan
*-- \brief writeLssFastScanReq - ermittle Knoten über FastScan
*
*-- Diese Funktion ermittelt mit Hilfe des Fast-Scan Algorithmus
*-- einen unkonfigurierten Knoten.
*-- Mit den Parametern
*-- Hersteller-Id, Produktcode, Revisions- und Seriennummer
*-- kann bitcodiert angegeben werden,
*-- welche Bits bei den LSS Slaves getestet werden sollen.
*-- Alle nichtgesetzten Bits müssen bei den LSS Slaves Identifiern 0 sein.
*++ This function detects unconfigured nodes by the LSS fastScan algorithm.
*++ The parameters vendor, product, revision and snr
*++ are bitcoded
*++ and contain the relevant bits to test the LSS slaves.
*++ All not used bits have to be zero at the LSS slave identifiers.
*
*-- Das Ende des FastScan wird durch die Confirmation Funktion
*++ The fastScan will be finished by the
* lssMasterCon()
*++ function.
*-- mitgeteilt.
*-- Wenn kein Knoten gefunden wurde, wird ein
*++ If no LSS node was found,
*++ the confirmation function is called with the parameter
* LSS_CON_FAST_SCAN_NO_NODE
*-- gemeldet.
*-- Ansonsten werden ein
*++ Otherwise,
*++ the confirmation function is called with the parameter
* LSS_CON_FAST_SCAN_DATA
*-- und die Indentifikationsdaten für den LSS-Slave
*-- geliefert.
*++ and the identification data of the LSS slave.
*-- Der identifizierte LSS Slave befindet sich an dieser Stelle
*-- schon im Configuration-Mode
*-- und kann nun konfiguriert werden.
*++ At this time the identified slave is in the CONFIGURATION-MODE
*++ and can be configured.
*
*
* \retval CO_OK
*++ success
*-- Erfolg
* \retval CO_E_BUSY
*++ LSS is waiting for answer from slave
*-- LSS wartet noch auf Antwort vom Slave
* \retval CO_E_NO_INITIATE
*++ Service not initialzed
*-- Dienst nicht initialisiert
*/
RET_T writeLssFastScanReq(
	UNSIGNED32  vendor,	/**< relevant bits of vendor id */
	UNSIGNED32  product,	/**< relevant bits of product code */
	UNSIGNED32  revision,	/**< relevant bits of revisioncode */
	UNSIGNED32  snr		/**< relevant bits of serial number */
	CO_COMMA_LINE_PARA_DECL	/**< number of CAN line 0..CO_MAX_CAN_LINES-1 */
      )
{
UNSIGNED8	bData[8];
#define FS_VENDOR_IDX	0	/* fast scan index for vendor id */
#define FS_PRODUCT_IDX	1	/* fast scan index for product code */
#define FS_REVISION_IDX	2	/* fast scan index for revision */
#define FS_SERNR_IDX	3	/* fast scan index for sernr */

    /* check for valid master initialisation */
    if ((GL_ARRAY(lssFlags) & LSS_FLAGS_MASTER) == 0)  {
	return(CO_E_NO_INITIATE);
    }

    if ((GL_ARRAY(lssFlags) & LSS_FLAGS_WAITING) != 0)  {
	/* lss waits for an answer */
	return(CO_E_BUSY);
    }

    /* Init all variables with zero */
    GL_ARRAY(bitChecked) = 0x80;
    GL_ARRAY(lssId) = 0;
    GL_ARRAY(lssNext) = 0;
    GL_ARRAY(lssSub) = 0;		/* 0..3 - lss subindex of 1018 */

    bData[5] = GL_ARRAY(bitChecked);
    bData[6] = GL_ARRAY(lssSub);
    bData[7] = GL_ARRAY(lssNext);
    TRANS_LSS_DATA(LSS_CS_FAST_SCAN, 0);
    GL_ARRAY(lssFlags) |= (LSS_FLAGS_WAITING | LSS_FLAGS_FAST_SCAN);
    GL_ARRAY(lssFlags) &= ~LSS_FLAGS_FAST_SCAN_ANSWER;
    GL_ARRAY(lssExpectAnswer) = LSS_CS_IDENT_SLAVE;

    /* start timeout timer */
    (void) addTimerEvent(&GL_ARRAY(lssTimer), CON_TIMEOUT,	\
		CO_TIMER_TYPE_LSS_MSTR | CO_TIMER_TYPE_CYCLIC	\
		CO_COMMA_LINE_PARA); \

    /* save scan bits - bit 0 have to be alltimes checked */
    GL_ARRAY(fastScanBits[FS_VENDOR_IDX]) = vendor | 1;
    GL_ARRAY(fastScanBits[FS_PRODUCT_IDX]) = product | 1;
    GL_ARRAY(fastScanBits[FS_REVISION_IDX]) = revision | 1;
    GL_ARRAY(fastScanBits[FS_SERNR_IDX]) = snr | 1;

    /* reset scanned identifier */
    GL_ARRAY(fastScanIdent[0]) = 0;
    GL_ARRAY(fastScanIdent[1]) = 0;
    GL_ARRAY(fastScanIdent[2]) = 0;
    GL_ARRAY(fastScanIdent[3]) = 0;

    return(CO_OK);
}
#endif /* CONFIG_LSS_MASTER */


#ifdef CONFIG_LSS_MASTER
/****************************************************************************/
/*
*++ \brief lssFastScanResponse - LSS Fast Scan mode continue
*-- \brief lssFastScanResponse - lss Fast Scan Fortsetzung
*
* \internal
*
*-- Diese Funktion wird vom timer aufgerufen,
*-- wenn der FastScan aktiv ist.
*-- Hier wird überprüft, ob in der abgelaufenen Zeit eine Antwort
*-- von den LSS Slaves erhalten wurde.
*-- Entsprechend wird die LSS-ID damit gesetzt.
*-- Wenn am Ende eines Zyklus genau ein Knoten selektiert wurde
*-- und dieser nicht antwortet,
*-- wird der Scan abgebrochen.
*
*
* no RETURNS
*
*/
static void lssFastScanResponse(
	CO_LINE_PARA_DECL	/**< number of CAN line 0..CO_MAX_CAN_LINES-1 */
    )
{
UNSIGNED8	bData[8];
BOOL_T		found = CO_FALSE;

    /* answer received ? */
    if ((GL_ARRAY(lssFlags) & LSS_FLAGS_FAST_SCAN_ANSWER) == 0) {
	/* no, was global check ? */
	if ((GL_ARRAY(bitChecked) == 0x80)
	 || (GL_ARRAY(lssNext) != GL_ARRAY(lssSub)))  {
	    /* reset timeout timer */
	    GL_ARRAY(lssTimer).timerType &= ~(CO_TIMER_TYPE_CYCLIC);
	    GL_ARRAY(lssFlags) &=
			(FLAG_T)~(LSS_FLAGS_WAITING | LSS_FLAGS_FAST_SCAN);

	    /* inform application */
	    lssMasterCon(LSS_CON_FAST_SCAN_NO_NODE, 0, 0 CO_COMMA_LINE_PARA);
	    return;
	} else {
	    /* answer was received, set corresponednding bit */
	    GL_ARRAY(lssId) |= (1UL << GL_ARRAY(bitChecked));
	}
    } else {
	if (GL_ARRAY(bitChecked) == 0x80)  {
	    GL_ARRAY(bitChecked) = 31;
	}
    }

    /* search next bit for check */
    while (GL_ARRAY(lssSub) < 4)  {
	/* are  bits set ? */
	while (GL_ARRAY(fastScanBits[GL_ARRAY(lssSub)]) != 0)  {
	    do  {
		/* search next bit */
		if ((GL_ARRAY(fastScanBits[GL_ARRAY(lssSub)])
			& (1u << GL_ARRAY(bitChecked))) != 0)  {
		    /* found, delete bit */
		    GL_ARRAY(fastScanBits[GL_ARRAY(lssSub)])
			&= ~(1u << GL_ARRAY(bitChecked));
		    found = CO_TRUE;
		    break;
		}
		GL_ARRAY(bitChecked)--;
	    } while (GL_ARRAY(bitChecked) > 0);

	    if (found == CO_TRUE)  {
		break;
	    }
	}
	if (found == CO_TRUE)  {
	    break;
	}

	/* at this point send idNumber again and set LssNext */
	if (GL_ARRAY(lssNext) == GL_ARRAY(lssSub))  {
	    GL_ARRAY(lssNext)++;
	    found = CO_TRUE;
	    break;
	}

	/* go to next subindex of identity object */
	GL_ARRAY(fastScanIdent[GL_ARRAY(lssSub)]) = GL_ARRAY(lssId);
	GL_ARRAY(lssSub)++;
	GL_ARRAY(lssNext) = GL_ARRAY(lssSub);
	GL_ARRAY(bitChecked) = 31;
	GL_ARRAY(lssId) = 0;
    }

    /* last cycle ? */
    if (found != CO_TRUE)  {
	/* reset timeout timer */
	GL_ARRAY(lssTimer).timerType &= ~(CO_TIMER_TYPE_CYCLIC);
	GL_ARRAY(lssFlags) &=(FLAG_T)~(LSS_FLAGS_WAITING | LSS_FLAGS_FAST_SCAN);

	/* answer received ? */
	if ((GL_ARRAY(lssFlags) & LSS_FLAGS_FAST_SCAN_ANSWER) == 0) {
	    /* inform application */
	    lssMasterCon(LSS_CON_FAST_SCAN_NO_NODE, 0, 0 CO_COMMA_LINE_PARA);
	} else {
#ifdef CO_CONFIG_LSS_FASTSCANDATA_REPORT_FULL_ARRAY
	    /* LSS confirmation fastscan node data */
            lssMasterCon(LSS_CON_FAST_SCAN_DATA,
                        (UNSIGNED8 *)&(fastScanIdent),0
                        CO_COMMA_LINE_PARA);
#else /* CO_CONFIG_LSS_FASTSCANDATA_REPORT_FULL_ARRAY */
	    /* LSS confirmation fastscan node data */
	    lssMasterCon(LSS_CON_FAST_SCAN_DATA,
			(UNSIGNED8 *)&GL_ARRAY(fastScanIdent[0]),0
			CO_COMMA_LINE_PARA);
#endif /* CO_CONFIG_LSS_FASTSCANDATA_REPORT_FULL_ARRAY */
	}
	return;
    }

    /* continue fast scan */
    bData[5] = GL_ARRAY(bitChecked);
    bData[6] = GL_ARRAY(lssSub);
    bData[7] = GL_ARRAY(lssNext);
    if (GL_ARRAY(lssNext) == 4)  {
	bData[7] = 0;
    }
    TRANS_LSS_DATA(LSS_CS_FAST_SCAN, GL_ARRAY(lssId));
    GL_ARRAY(lssFlags) &= ~LSS_FLAGS_FAST_SCAN_ANSWER;
}
#endif /* CONFIG_LSS_MASTER */


#ifdef CONFIG_LSS_MASTER
/****************************************************************************/
/*
*++ \brief lssTimeOut - lss answer time out
*-- \brief lssTimeOut - lss Antwort time out
*
* \internal
*
*-- Diese Funktion wird vom timer aufgerufen,
*-- wenn keine Antwort vom LSS Slave vorliegt.
*-- Die Funktion meldet dies dem Anwender über die Indikationfunktion
*++ This function is called by the timer
*++ if there is no response of a LSS slave.
*++ With the indication function
* lssMasterCon()
*-- mit dem Parameter
*++ the user is informed that
*++ there was no respond to a LSS command.
*++ The given parameter has the value
* LSS_CON_TIMEOUT
*
*
* no RETURNS
*
*/
void lssTimeOut(
	CO_LINE_PARA_DECL	/**< number of CAN line 0..CO_MAX_CAN_LINES-1 */
      )
{
    if ((GL_ARRAY(lssFlags) & (FLAG_T)(LSS_FLAGS_FAST_SCAN)) != 0)  {
	lssFastScanResponse(CO_LINE_PARA);
    } else {
        /* reset timeout timer */
        /* delete wait for answer */
        GL_ARRAY(lssFlags) &= (FLAG_T)(~LSS_FLAGS_WAITING);

        /* call the confirmation function with parameter timeout */
        lssMasterCon(LSS_CON_TIMEOUT, NULL, NULL CO_COMMA_LINE_PARA);
    }
    return;
}
#endif /* CONFIG_LSS_MASTER */


#ifdef CONFIG_LSS_MASTER
/****************************************************************************/
/**
*++ \brief lssCheckState - check, if a LSS command is in progress
*-- \brief lssCheckState - testet, ob LSS Kommando bearbeitet wird
*
*-- Mit dieser Funktion kann ermittelt werden,
*-- ob gerade ein LSS Kommando in Arbeit ist
*-- und auf eine Antwort vom Slave gewartet wird.
*++ This function ascertains,
*++ if an LSS command is in progress and
*++ the master waits for an answer of a slave.
*
* \retval CO_OK
*++ LSS not active
*-- LSS ist frei
*
* \retval CO_E_BUSY
*++ LSS is busy
*-- LSS ist beschäftigt und wartet auf Antwort
*
*/
RET_T lssCheckState(
	CO_LINE_PARA_DECL	/**< number of CAN line 0..CO_MAX_CAN_LINES-1 */
      )
{
RET_T retVal = CO_OK;

    /* check for active connection */
    if ((GL_ARRAY(lssFlags) & LSS_FLAGS_WAITING) != 0)  {
	/* lss waits for an answer */
	retVal = CO_E_BUSY;
    }

    return(retVal);
}

#endif /* CONFIG_LSS_MASTER */


#ifdef CONFIG_LSS_SLAVE

/****************************************************************************/
/*
*++ \brief lssReqMsgReceived - lss master message received
*-- \brief lssReqMsgReceived - lss master Message erhalten
*
*
* \internal
*
*++ This function proccesses the received lss commands from LSS master
*-- Diese Funktion bearbeitet die LSS Kommandos vom LSS-Master
*
*++ LSS knows 2 Substates:
*-- LSS kennt 2 Substati:
* \c WAITING
* \c CONFIGURATION
*
*-- Im LSS-Zustand WAITING werden keine LSS Kommandos entgegengenommen
*-- ausser dem Umschaltkommando zu CONFIGURATION.
*++ In the LSS state WAITING no LSS commands are processed except the
*++ global switching command.
*
* no RETURNS
*
*/

void lssReqMsgReceived(
	CAN_MSG_T *canMsg	/* Pointer to CAN Message */
	CO_COMMA_LINE_PARA_DECL	/* number of CAN line 0..CO_MAX_CAN_LINES-1 */
      )
{
# ifndef CONFIG_NO_GLOBAL_VARS
static LSS_IDENT_T lssIdent CO_LINE_PARA_ARRAY_DEF;
# endif /*CONFIG_NO_GLOBAL_VARS*/

UNSIGNED8	u8;

    /* check for valid initialisation */
    if (GL_ARRAY(pIdentity) == NULL)  {
	return;
    }

    /* check for valid slave initialisation */
    if ((GL_ARRAY(lssFlags) & LSS_FLAGS_MASTER) != 0)  {
	return;
    }

#ifndef CO_CONFIG_LSS_NO_REACTION_IN_OPERATIONAL
    /* ignore LSS commands in OPERATIONAL */
    if (GL_ARRAY(co_Node).eState == OPERATIONAL)
    {
	return;
    }
#endif /* CO_CONFIG_LSS_NO_REACTION_IN_OPERATIONAL */

    /* select the message type */
    switch (canMsg->pData[0])  {
	case LSS_CS_SWITCH_GLOBAL:	/* switch mode global */
	    setLssState(canMsg->pData[1] CO_COMMA_LINE_PARA);
	    break;

	case LSS_CS_IDENT_SLAVE_NOCFG:	/* ident unconfigured slaves */
	    if (GL_ARRAY(coNodeId) == 255)  {
		lssConfigResponse(LSS_CS_IDENT_SLAVE_CFG, 0, 0
			CO_COMMA_LINE_PARA);
	    }
	    break;

	case LSS_CS_SWITCH_SEL_VENDOR:	/* switch mode selective - start */
	case LSS_CS_IDENT_SLAVE_VENDOR:	/* identify - start */
	    CO_PACK_MEMCPY(
		(UNSIGNED8 *)&GL_ARRAY(lssIdent).vendor, &canMsg->pData[1], 4, 1);
	    /* delete other vars */
	    GL_ARRAY(lssIdent).product = 0xffffffff;
	    GL_ARRAY(lssIdent).rev_low = 0xffffffff;
	    GL_ARRAY(lssIdent).rev_high = 0x0;
	    GL_ARRAY(lssIdent).snr_low = 0xffffffff;
	    GL_ARRAY(lssIdent).snr_high = 0x0;
	    break;

	case LSS_CS_SWITCH_SEL_PROD:	/* switch mode selective - product */
	case LSS_CS_IDENT_SLAVE_PRODUCT:/* identity - product */
	    CO_PACK_MEMCPY(
		(UNSIGNED8 *)&GL_ARRAY(lssIdent).product, &canMsg->pData[1],
		4, 1);
	    break;

	case LSS_CS_SWITCH_SEL_REV:	/* switch mode selective - revision */
	    CO_PACK_MEMCPY(
		(UNSIGNED8 *)&GL_ARRAY(lssIdent).rev_low, &canMsg->pData[1],
		4, 1);
	    CO_PACK_MEMCPY(
		(UNSIGNED8 *)&GL_ARRAY(lssIdent).rev_high,
		&canMsg->pData[1],
		4, 1);
	    break;

	case LSS_CS_SWITCH_SEL_SNR:	/* switch mode selective - serial number */
	    CO_PACK_MEMCPY(
		(UNSIGNED8 *)&GL_ARRAY(lssIdent).snr_low, &canMsg->pData[1],
		4, 1);
	    CO_PACK_MEMCPY(
		(UNSIGNED8 *)&GL_ARRAY(lssIdent).snr_high, &canMsg->pData[1],
		4, 1);

	    /* ignore command in config mode */
	    if (GL_ARRAY(lssState) == LSS_STATE_CONFIG)  {
		return;
	    }

	    /* check received data */
	    if (checkIdentData(&GL_ARRAY(lssIdent) CO_COMMA_LINE_PARA)
			== CO_TRUE) {
#ifdef CONFIG_CO_DEBUG
		BDEBUG(CO_DEBUG_LSS, "lss: selective switch mode\n");
#endif /* CONFIG_CO_DEBUG */
		setLssState(LSS_SWITCH_MODE_CFG CO_COMMA_LINE_PARA);
		/* send positive response */
		lssConfigResponse(LSS_CS_SWITCH_SEL, 0, 0 CO_COMMA_LINE_PARA);
	    }
	    break;

	case LSS_CS_IDENT_SLAVE_REV_LOW:/* identity - revision low */
	    CO_PACK_MEMCPY(
		(UNSIGNED8 *)&GL_ARRAY(lssIdent).rev_low, &canMsg->pData[1],
		4, 1);
	    break;

	case LSS_CS_IDENT_SLAVE_REV_HIGH:/* identity - revision high */
	    CO_PACK_MEMCPY(
		(UNSIGNED8 *)&GL_ARRAY(lssIdent).rev_high, &canMsg->pData[1],
		4, 1);
	    break;

	case LSS_CS_IDENT_SLAVE_SNR_LOW:/* identity - serial number low */
	    CO_PACK_MEMCPY(
		(UNSIGNED8 *)&GL_ARRAY(lssIdent).snr_low, &canMsg->pData[1],
		4, 1);
	    break;

	case LSS_CS_IDENT_SLAVE_SNR_HIGH:/* identity - serial number high */
	    CO_PACK_MEMCPY(
		(UNSIGNED8 *)&GL_ARRAY(lssIdent).snr_high, &canMsg->pData[1],
		4, 1);
	    if (checkIdentData(&GL_ARRAY(lssIdent) CO_COMMA_LINE_PARA)
			== CO_TRUE) {
		lssConfigResponse(LSS_CS_IDENT_SLAVE, 0, 0 CO_COMMA_LINE_PARA);
	    }
	    break;

	case LSS_CS_FAST_SCAN:	/* LSS Fast Scan */
	    lssFastScan(canMsg CO_COMMA_LINE_PARA);
	    break;
        default:
            break;
    }

    /* the following services are only available in configuration mode */
    if (GL_ARRAY(lssState) == LSS_STATE_CONFIG)  {

	/* select the message type */
	switch (canMsg->pData[0])  {
	    case LSS_CS_SET_NODEID:		/* set node id */
#ifdef CONFIG_CO_DEBUG
		BDEBUG(CO_DEBUG_LSS, "lss: new node id: %d\n",canMsg->pData[1]);
#endif /* CONFIG_CO_DEBUG */
		/* check for valid node-id */
		if (((canMsg->pData[1] > 0) && (canMsg->pData[1] < 128))
		  || (canMsg->pData[1] == 255)) {
		    u8 = lssSlaveInd(LSS_IND_NODEID, canMsg->pData[1], 0
			    CO_COMMA_LINE_PARA);
		    /* set cob-ids for sdo to new node-id */
		    if (u8 == 0)  {
			GL_ARRAY(lssFlags) |= LSS_FLAGS_NODEID_CHANGED;
			lssConfigResponse(LSS_CS_SET_NODEID, u8, 0
				CO_COMMA_LINE_PARA);
		    } else {
			lssConfigResponse(LSS_CS_SET_NODEID, 255, u8
				CO_COMMA_LINE_PARA);
		    }
		} else {
		    lssConfigResponse(LSS_CS_SET_NODEID, LSS_ERROR_NODEID_RANGE,
			0 CO_COMMA_LINE_PARA);
		    break;
		}
		break;

	    case LSS_CS_SET_BITRATE:	/* configure bit timing */
		/* save return value */
		GL_ARRAY(bitrateErr) = lssSlaveInd(LSS_IND_BITRATE,
			canMsg->pData[1],
			canMsg->pData[2] CO_COMMA_LINE_PARA);
		if (GL_ARRAY(bitrateErr) == 0)  {
		    /* no error */
		    lssConfigResponse(LSS_CS_SET_BITRATE, 0, 0
			CO_COMMA_LINE_PARA);
		} else {
		    if (GL_ARRAY(bitrateErr) == 1)  {
			/* invalid table index */
			lssConfigResponse(LSS_CS_SET_BITRATE, 1, 0
			    CO_COMMA_LINE_PARA);
		    } else {
			/* user specific error */
			lssConfigResponse(LSS_CS_SET_BITRATE, 255,
			    GL_ARRAY(bitrateErr)
			    CO_COMMA_LINE_PARA);
		    }
		}
		break;

	    case LSS_CS_ACTIVATE_BITRATE:/* activate new bitrate */
		activateNewBitrate(((UNSIGNED16)canMsg->pData[2] << 8)
			| canMsg->pData[1] CO_COMMA_LINE_PARA);
		break;

	    case LSS_CS_STORE_CFG:	/* store parameter */
		u8 = lssSlaveInd(LSS_IND_STORE, 0, 0 CO_COMMA_LINE_PARA);
		/* error code 0..2 are save at position 2 */
		if (u8 < 3)  {
		    lssConfigResponse(LSS_CS_STORE_CFG, u8, 0
			CO_COMMA_LINE_PARA);
		} else {
		    lssConfigResponse(LSS_CS_STORE_CFG, 255, u8
			CO_COMMA_LINE_PARA);
		}
		break;

	    case LSS_CS_INQ_VENDOR:	/* inquire vendor */
		lssInquiryResponse(LSS_CS_INQ_VENDOR,
			GL_ARRAY(pIdentity)->vendorId
			CO_COMMA_LINE_PARA);
		break;

	    case LSS_CS_INQ_PRODUCT:	/* inquire product */
		lssInquiryResponse(LSS_CS_INQ_PRODUCT,
			GL_ARRAY(pIdentity)->productCode
			CO_COMMA_LINE_PARA);
		break;

	    case LSS_CS_INQ_REV:	/* inquire revision */
		lssInquiryResponse(LSS_CS_INQ_REV,
			GL_ARRAY(pIdentity)->revisionNumber
			CO_COMMA_LINE_PARA);
		break;

	    case LSS_CS_INQ_SNR:	/* inquire snr */
		lssInquiryResponse(LSS_CS_INQ_SNR,
			GL_ARRAY(pIdentity)->serialNumber
			CO_COMMA_LINE_PARA);
		break;

	    case LSS_CS_INQ_NODEID:	/* inquire node-id */
		lssInquiryResponse(LSS_CS_INQ_NODEID, GL_ARRAY(coNodeId)
			CO_COMMA_LINE_PARA);
		break;
            default:
                break;
	}
    }
}
#endif /* CONFIG_LSS_SLAVE */


#ifdef CONFIG_LSS_SLAVE
/****************************************************************************/
/*
*++ \brief checkIdentData - check received identification data
*-- \brief checkIdentData - check empfangene Identification Daten
*
*
* \internal
*
*-- Diese Funktion testet die empfangenen Identification Daten
*-- und liefert bei Übereinstimmung mit den Daten im Object 1018
*-- CO_TRUE zurück
*++ This function checks the received identification data and
*++ returns CO_TRUE on conformity with the data in Object 1018.
*
* \retval CO_OK
*++ success
*-- Erfolg
*
*/

static BOOL_T checkIdentData(
	LSS_IDENT_T *lssIdent	/* pointer to ident data */
	CO_COMMA_LINE_PARA_DECL	/* number of CAN line 0..CO_MAX_CAN_LINES-1 */
    )
{
    if (GL_ARRAY(pIdentity)->vendorId != lssIdent->vendor)  {
#ifdef CONFIG_CO_DEBUG
	BDEBUG(CO_DEBUG_LSS, "lss: invalid vendor\n");
#endif /* CONFIG_CO_DEBUG */
	return(CO_FALSE);
    }
    if (GL_ARRAY(pIdentity)->productCode != lssIdent->product)  {
#ifdef CONFIG_CO_DEBUG
	BDEBUG(CO_DEBUG_LSS, "lss: invalid product\n");
#endif /* CONFIG_CO_DEBUG */
	return(CO_FALSE);
    }
    if (GL_ARRAY(pIdentity)->revisionNumber < lssIdent->rev_low) {
#ifdef CONFIG_CO_DEBUG
	BDEBUG(CO_DEBUG_LSS, "lss: invalid revision (to low)\n");
#endif /* CONFIG_CO_DEBUG */
	return(CO_FALSE);
    }
    if (GL_ARRAY(pIdentity)->revisionNumber > lssIdent->rev_high) {
#ifdef CONFIG_CO_DEBUG
	BDEBUG(CO_DEBUG_LSS, "lss: invalid revision (to high)\n");
#endif /* CONFIG_CO_DEBUG */
	return(CO_FALSE);
    }
    if (GL_ARRAY(pIdentity)->serialNumber < lssIdent->snr_low) {
#ifdef CONFIG_CO_DEBUG
	BDEBUG(CO_DEBUG_LSS, "lss: invalid serial-Nr (to low)\n");
#endif /* CONFIG_CO_DEBUG */
	return(CO_FALSE);
    }
    if (GL_ARRAY(pIdentity)->serialNumber > lssIdent->snr_high){
#ifdef CONFIG_CO_DEBUG
	BDEBUG(CO_DEBUG_LSS, "lss: invalid serial-Nr (to high)\n");
#endif /* CONFIG_CO_DEBUG */
	return(CO_FALSE);
    }
    return(CO_TRUE);
}
#endif /* CONFIG_LSS_SLAVE */


#ifdef CONFIG_LSS_SLAVE
/****************************************************************************/
/*
*++ \brief setLssState - enter lss mode
*-- \brief setLssState - enter lss mode
*
*
* \internal
*
*++ This function set the lss mode iof the device.
*-- Diese Funktion setzt den Knoten in den übergebenen LSS Mode
*
*-- Wenn vom Config in den WAITING Mode geschaltet wird
*-- und vorher die Node-Id geändert wurde,
*-- wird ein Reset Comm aufgerufen,
*-- um die COB-Ids zu aktualisieren
*-- und die Boot-Up zu senden.
*++ If the node is switched from CONFIG to WAITING mode
*++ and prior to this the node id was set
+++ the node will reset the communication parameter
*++ with a call to resetComm().
*++ Then a boot-up will be sent.
*
* \code
* modes:
*	2 - WAITING, if CONFIGURATION mode not entered
*	1 - CONFIGURATION
*	0 - WAITING
* \endcode
*
* no RETURNS
*
*/

void setLssState(
	UNSIGNED8 mode		/* new lss state */
	CO_COMMA_LINE_PARA_DECL/* number of CAN line 0..CO_MAX_CAN_LINES-1 */
    )
{
    /* which mode */
    if (mode == LSS_SWITCH_MODE_WAIT_C)  {
	/* go into waiting mode, but only if config not entered already */
	if (GL_ARRAY(lssState) == LSS_STATE_CONFIG)  {
	    return;
	}
	mode = LSS_SWITCH_MODE_WAIT;
    }

    if (mode == LSS_SWITCH_MODE_CFG)  {
	/* enter configuration mode */
#ifdef CONFIG_LSS_STATE_IND
	(void) lssSlaveInd(LSS_IND_MODE_CFG, 0, 0 CO_COMMA_LINE_PARA);
#endif /* CONFIG_LSS_STATE_IND */

	GL_ARRAY(lssState) = LSS_STATE_CONFIG;
#ifdef CONFIG_CO_DEBUG
	BDEBUG(CO_DEBUG_LSS, "lss: enter CONFIG mode\n");
#endif /* CONFIG_CO_DEBUG */

#ifdef CONFIG_CO_RUN_LED
# ifdef CONFIG_REDUNDANCY_SUPPORT
	setCoRunLedState(CO_RUN_LED_LSS, CAN_DEFAULT_LINE);
	setCoRunLedState(CO_RUN_LED_LSS, CAN_REDCY_LINE);
# else /* CONFIG_REDUNDANCY_SUPPORT */
	setCoRunLedState(CO_RUN_LED_LSS CO_COMMA_LINE_PARA);
# endif /* CONFIG_REDUNDANCY_SUPPORT */
#endif /* defined(CONFIG_CO_RUN_LED) */

    }

    if (mode == LSS_SWITCH_MODE_WAIT)  {

	/* return if state was already entered */
	if (GL_ARRAY(lssState) == LSS_STATE_WAITING)  {
	    return;
	}

	/* enter operation mode */
#ifdef CONFIG_LSS_STATE_IND
	(void) lssSlaveInd(LSS_IND_MODE_WAITING, 0, 0 CO_COMMA_LINE_PARA);
#endif /* CONFIG_LSS_STATE_IND */

	GL_ARRAY(lssState) = LSS_STATE_WAITING;

	/* if the node-id was changed */
	if ((GL_ARRAY(lssFlags) & LSS_FLAGS_NODEID_CHANGED) != 0) {
#ifdef CONFIG_CO_DEBUG
	    BDEBUG(CO_DEBUG_LSS, "lss: Id was changed - reset Comm\n");
#endif /* CONFIG_CO_DEBUG */

	    /* was the node-id 0xff before, then call reset comm */
	    if (GL_ARRAY(coNodeId) == 0xff)  {
		/* disable first server sdo COB-Ids */
		(void) setSdoCobId(1, SDO_NO_VALID_BIT, SERVER, CO_COB_SDO_RX
		    CO_COMMA_LINE_PARA);
		(void) setSdoCobId(1, SDO_NO_VALID_BIT, SERVER, CO_COB_SDO_TX
		    CO_COMMA_LINE_PARA);

#ifdef CONFIG_REDUNDANCY_SUPPORT
		resetCommMsg(CAN_DEFAULT_LINE);
		resetCommMsg(CAN_REDCY_LINE);
#else /* CONFIG_REDUNDANCY_SUPPORT */
		resetCommMsg(CO_LINE_PARA);
#endif /* CONFIG_REDUNDANCY_SUPPORT */

	    }

	    GL_ARRAY(lssFlags) &= (FLAG_T)~LSS_FLAGS_NODEID_CHANGED;
	}

#ifdef CONFIG_CO_RUN_LED
	/* if node-id = 255 don't leave lss mode */
	if (GL_ARRAY(coNodeId) != 255)  {
# ifdef CONFIG_REDUNDANCY_SUPPORT
	    updateNMTState_led(CAN_DEFAULT_LINE);
	    updateNMTState_led(CAN_REDCY_LINE);
# else /* CONFIG_REDUNDANCY_SUPPORT */
	    updateNMTState_led(CO_LINE_PARA);
# endif /* CONFIG_REDUNDANCY_SUPPORT */
	}
#endif /* defined(CONFIG_CO_RUN_LED) */

#ifdef CONFIG_CO_DEBUG
	BDEBUG(CO_DEBUG_LSS, "lss: leave CONFIG mode\n");
#endif /* CONFIG_CO_DEBUG */
    }
}
#endif /* CONFIG_LSS_SLAVE */


#ifdef CONFIG_LSS_SLAVE
/****************************************************************************/
/*
*++ \brief lssInquiryResponse - identity response
*-- \brief lssInquiryResponse - identity Antwort
*
*
* \internal
*
*-- Diese Funktion sendet eine Identity Antwort
*++ This function transmits a response to an identify request.
*
*
*/

static void lssInquiryResponse(
	UNSIGNED8 cmd,		/* command specifier */
	UNSIGNED32 para		/* parameter */
	CO_COMMA_LINE_PARA_DECL	/* number of CAN line 0..CO_MAX_CAN_LINES-1 */
    )
{
UNSIGNED8	bData[8];

    memset(&bData[0], (int)0, (size_t)8);
    bData[0] = cmd;
    bData[1] = para & 0xff;
    bData[2] = (para >> 8) & 0xff;
    bData[3] = (para >> 16) & 0xff;
    bData[4] = (para >> 24) & 0xff;

    (void)TRANSMIT_COB(GL_ARRAY(pLss_TrCOB), &bData[0]);
}
#endif /* CONFIG_LSS_SLAVE */


#ifdef CONFIG_LSS_SLAVE
/****************************************************************************/
/*
*++ \brief lssConfigResponse - configuration response
*-- \brief lssConfigResponse - configuration Antwort
*
* \internal
*
*-- Diese Funktion sendet eine Antwort auf eine Configurations-
*-- anforderung vom LSS Master.
*-- Der Parameter errcode enthält den zu versendenden Fehlercode.
*-- Er wird gewöhnlich durch die User-Indikationfunktionen geliefert.
*-- Wenn er 0 ist, wurde die Funktion fehlerfrei abgearbeitet,
*++ This function transmits a response to a request of a global switch command
*++ sent by the master.
*++ The parameter errcode contains the error code.
*++ This error code is given by the indication function.
*++ If it is 0 then the function was processed correctly.
*
* no RETURNS
*
*/

static void lssConfigResponse(
	UNSIGNED8 cmd,		/* command specifier */
	UNSIGNED8 errcode,	/* errorcode from indication function */
	UNSIGNED8 errspec	/* errorspec */
	CO_COMMA_LINE_PARA_DECL	/* number of CAN line 0..CO_MAX_CAN_LINES-1 */
    )
{
UNSIGNED8	bData[8];

    memset(&bData[0], (int)0, (size_t)8);
    bData[0] = cmd;

    /* set only error, if the errorcode was != 0 */
    if (errcode != 0)  {
	bData[1] = errcode;		/* implementation specific error code */
	bData[2] = errspec;

#ifdef CONFIG_CO_ERR_LED
	/* setCoRunLedState(CO_ERR_LED_LSS CO_COMMA_LINE_PARA); */
#endif /* CONFIG_CO_RUN_LED */
    }
#ifdef CONFIG_CO_ERR_LED
    else {
	/* resetCoLedState(CO_ERR_LED_LSS CO_COMMA_LINE_PARA); */
    }
#endif /* CONFIG_CO_RUN_LED */

    (void)TRANSMIT_COB(GL_ARRAY(pLss_TrCOB), &bData[0]);
}
#endif /* CONFIG_LSS_SLAVE */


#ifdef CONFIG_LSS_SLAVE
/****************************************************************************/
/**
*++ \brief writeLssNonConfigSlaveReq - transmit a non config message
*-- \brief writeLssNonConfigSlaveReq - sendet ein Non Config Nachricht
*
*-- Diese Funktion sendet eine LSS Identity Non-Configured Slave Nachricht,
*-- so dass ein LSS Master im Netzwerk diesen unkonfigurierten Knoten
*-- erkennen und konfigurieren kann.
*++ This function transmits a LSS identity non configures slave message.
*++ The LSS master can recognize and can configure the node.
*
* \retval CO_OK
*++ success
*-- Erfolg
* \retval CO_E_NO_INITIATE
*++ Service not initialized
*-- Dienst nicht initialisiert
*
*/

RET_T writeLssNonConfigSlaveReq(
	CO_LINE_PARA_DECL	/**< number of CAN line 0..CO_MAX_CAN_LINES-1 */
      )
{
RET_T retVal = CO_OK;

    /* check for valid initialisation or only unconfigured nodes are allow to send this */
    if ( (GL_ARRAY(pIdentity) == NULL) || (GL_ARRAY(coNodeId) != 255) ) {
	retVal = CO_E_NO_INITIATE;
    } else {
	lssConfigResponse(LSS_CS_IDENT_SLAVE_CFG, 0, 0 CO_COMMA_LINE_PARA);
    }
    return(retVal);
}
#endif /* CONFIG_LSS_SLAVE */


#ifdef CONFIG_LSS_SLAVE
/***************************************************************************/
/*
*++ \brief activateNewBitrate - activate new bitrate
*-- \brief activateNewBitrate - neue Bitrate aktivieren
*
* \internal
*
*-- Diese Funktion startet die Aktivierung einer neuen Bitrate.
*-- Die neue Bitrate muss vorher durch das LSS Kommando eingestellt
*-- worden sein.
*++ This function starts the activation of a new bitrate.
*++ The new bitrate has to be set before with a LSS command.
*-- Die Aktivierung erfolgt in 2 Schritten:
*++ The activation takes place in two steps:
*-- - Wartezeit einhalten
*++ - adhere to a timeout
*-- - Bitrate umstellen
*++ - change bitrate
*-- - Wartezeit nochmals abwarten
*++ - adhere to a timeout
*-- Dazu wird der Library Timer verwendet.
*++ The timer of the library is used.
*
* no RETURNS
*
*/

static void activateNewBitrate(
	UNSIGNED16 switchTime	/* switch delay time */
	CO_COMMA_LINE_PARA_DECL	/* number of CAN line 0..CO_MAX_CAN_LINES-1 */
    )
{
    /* check for valid bitrate */
    if (GL_ARRAY(bitrateErr) == 0)  {
	/* call indication function */
	(void) lssSlaveInd(LSS_IND_BITRATE_SWITCH, 0, 0 CO_COMMA_LINE_PARA);
	/* start first waittime */
	(void) addTimerEvent(&GL_ARRAY(lssTimer), switchTime * 10,
	    (CO_TIMER_TYPE_LSS_SL | CO_TIMER_TYPE_CYCLIC) CO_COMMA_LINE_PARA);
	/* save first call of start timer */
	GL_ARRAY(lssBitrateSwitchState) = 0;
    }
}
#endif /* CONFIG_LSS_SLAVE */


#ifdef CONFIG_LSS_SLAVE
/***************************************************************************/
/*
*++ \brief lssFastScan - lss fastScan message received
*-- \brief lssFastScan - lss fastScan Nachricht erhalten
*
* \internal
*
*-- Diese Funktion behandelt den Empfang der LSS FastScan Nachrichten.
*
* no RETURNS
*
*/
static void lssFastScan(
	CAN_MSG_T	*canMsg		/* Pointer to CAN Message */
	CO_COMMA_LINE_PARA_DECL
    )
{
#define LSSMSG_LSS_BIT_CHECK	canMsg->pData[4]
#define LSSMSG_BIT_CHECKED	canMsg->pData[5]
#define LSSMSG_LSS_SUB		canMsg->pData[6]
#define LSSMSG_LSS_NEXT		canMsg->pData[7]

UNSIGNED8		i, nodeId;
RET_T			retVal;
UNSIGNED32		v, idNumber;

    /* the services is only available in waiting mode */
    if (GL_ARRAY(lssState) != LSS_STATE_WAITING)  {
	return;
    }

    /* have we already a valid node id ? */
    nodeId = getNodeId(CO_LINE_PARA);
    /* if ((nodeId > 0) && (nodeId < 128))  { */
    if (nodeId != 255)  {
	/* yes, valid node id is available */
	return;
    }

    /* start new fast scan ? */
    if (LSSMSG_BIT_CHECKED == 0x80)  {
	GL_ARRAY(lssFastPos) = 0;
	/* send LSS identity slave */
	lssConfigResponse(LSS_CS_IDENT_SLAVE, 0, 0 CO_COMMA_LINE_PARA);

	/* search data from identity object */
	for (i = 0; i < 4; i++)  {
	    retVal = getObjEntry(IDENTITY_INDEX, i + 1,
		(UNSIGNED8 *)&GL_ARRAY(fastScanBits[i]), &v, CO_TRUE
			CO_COMMA_LINE_PARA);
	    if (retVal != CO_OK)  {
		GL_ARRAY(fastScanBits[i]) = 0;
	    }
	}
	return;

    } else {
	if (GL_ARRAY(lssFastPos) == 0xff)  {
	    /* don't participate until bitChecked = 128 */
	    return;
	}
    }

    /* is this the expected subindex of identity object */
    if (GL_ARRAY(lssFastPos) != LSSMSG_LSS_SUB)  {
	/* wait for next master message */
	return;
    }

    /* current lss nr matches with idNumber in non-masked area ? */
    CO_PACK_MEMCPY((UNSIGNED8 *)&idNumber, &canMsg->pData[1], 4, CO_TRUE);
    /* EXOR with idNumber and mask all relevant bits */
    v = (idNumber ^ GL_ARRAY(fastScanBits[GL_ARRAY(lssFastPos)])) & (0xfffffffful << LSSMSG_BIT_CHECKED);
    if (v != 0)  {
	/* doesn't match more */
	return;
    }

    /* don't take part on the fast scan, if the value doesnt fit us */
    if ((LSSMSG_BIT_CHECKED == 0) && (GL_ARRAY(lssFastPos) != LSSMSG_LSS_NEXT)) {
	GL_ARRAY(lssFastPos) = LSSMSG_LSS_NEXT;
    }

    /* send LSS identity slave */
    lssConfigResponse(LSS_CS_IDENT_SLAVE, 0, 0 CO_COMMA_LINE_PARA);

    /* last message an our node identified ? */
    if ((LSSMSG_BIT_CHECKED == 0) && (LSSMSG_LSS_SUB == 3) && (LSSMSG_LSS_NEXT == 0)) {
	/* node identified, enter configuration mode */
	setLssState(LSS_SWITCH_MODE_CFG CO_COMMA_LINE_PARA);
    }
}
#endif /* CONFIG_LSS_SLAVE */


#ifdef CONFIG_LSS_SLAVE
/***************************************************************************/
/*
*++ \brief lssSwitchTimeEvent1 - timer event for switchtime
*-- \brief lssSwitchTimeEvent1 - timer event for switchtime
*
*
* \internal
*
*-- Diese Funktion wird vom Timer aufgerufen,
*-- wenn die Umschaltzeit zur Baudratenumstellung abgelaufen ist
*-- oder die Umschaltzeit nach der Baudratenumstellung abgelaufen ist.
*++ If the time for changing the bitrate has exceeded
*++ or after the baudrate was changed
*++ then this function is called.
*
*-- Diese Funktion wird vom Timer aufgerufen,
*-- Der Anwender wird über die Funktion
*++ The user is informed via
* lssSlaveInd()
*-- informiert.
*++ This function is called by the timer.
* no RETURNS
*
*/

void lssSwitchTimeEvent(
	CO_LINE_PARA_DECL	/* number of CAN line 0..CO_MAX_CAN_LINES-1 */
    )
{
    if (GL_ARRAY(lssBitrateSwitchState) == 0)  {
	/* timer 1 is up */
	/* call indication function to set new bitrate */
	(void) lssSlaveInd(LSS_IND_BITRATE_SET, 0, 0 CO_COMMA_LINE_PARA);
	/* set second call of switch timer */
    } else {
	/* timer 2 is up */
	if (GL_ARRAY(lssBitrateSwitchState) == 1)  {
	    /* call indication function to set new bitrate */
	    (void) lssSlaveInd(LSS_IND_BITRATE_ACTIVE, 0, 0 CO_COMMA_LINE_PARA);
	    GL_ARRAY(bitrateErr) = 255;
	} else {
	    /* not allowed state - ignore it */
	}
	/* delete timer - we use a cyclic timer, reset bit */
	GL_ARRAY(lssTimer).timerType &= ~CO_TIMER_TYPE_CYCLIC;
    }
    GL_ARRAY(lssBitrateSwitchState) ++;
}

#endif /* CONFIG_LSS_SLAVE */

#ifdef CONFIG_LSS_SLAVE
/****************************************************************************/
/**
* \brief lssGetLocalSlaveState
*
* This funtion returns the actual lss state of the local node.
*
* \retval CO_LSS_STATE_NONE
* No valid lss state.
* \retval LSS_STATE_CONFIG
* LSS slave is in lss state configuration.
* \retval LSS_STATE_WAITING
* LSS slave is in lss state waiting.
*
*/
UNSIGNED8 lssGetLocalSlaveState(
	CO_LINE_PARA_DECL	/* number of CAN line 0..CO_MAX_CAN_LINES-1 */
    )
{
UNSIGNED8 retVal = CO_LSS_STATE_NONE;

    if (GL_ARRAY(lssState) == LSS_STATE_CONFIG)  {
        retVal = CO_LSS_STATE_CFG;
    }

    if (GL_ARRAY(lssState) == LSS_STATE_WAITING)  {
        retVal = CO_LSS_STATE_WAIT;
    }

    return retVal;
}
#endif /* CONFIG_LSS_SLAVE */


#ifdef CONFIG_LSS_MASTER
/****************************************************************************/
/*
* \brief pcoLssUtilCheckFlags
*
* \internal
*
* This funtion just checks some flags and one pointer to be not NULL. Its just
* a helper function for repeated tasks from other functions.
*
*
*
* \retval CO_OK
* success
* \retval CO_E_BUSY
* LSS is waiting for answer from slave
* \retval CO_E_NO_INITIATE
* Service not initialzed

*
*/
static RET_T pcoLssUtilCheckFlags( CO_LINE_PARA_DECL )
{
RET_T retVal = CO_OK;

    /* check for valid initialisation */
    if (GL_ARRAY(pIdentity) == NULL)  {
	return(CO_E_NO_INITIATE);
    }

    /* check for valid master initialisation */
    if ((GL_ARRAY(lssFlags) & LSS_FLAGS_MASTER) == 0)  {
	return(CO_E_NO_INITIATE);
    }

    /* check for active connection */
    if ((GL_ARRAY(lssFlags) & LSS_FLAGS_WAITING) != 0)  {
	/* lss waits for an answer */
	return(CO_E_BUSY);
    }

    return retVal;
}

#endif /* CONFIG_LSS_MASTER */
