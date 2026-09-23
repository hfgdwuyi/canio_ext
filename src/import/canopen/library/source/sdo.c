/*
 *++ sdo - contains SDOs service functions
 *-- sdo - beinhaltet Servicefunktionen für SDOs
 *
 * Copyright (c) 1996-2017 port GmbH Halle (Saale)
 *------------------------------------------------------------------
 */


/****************************************************************************/
/**
*  \file sdo.c
*++ Contains SDO service functions
*-- Beinhaltet Servicefunktionen für SDO
*  \author port GmbH Halle (Saale)
*
*++ This module contains the functions for handling the
*++ Service Data Objects (SDO).
*++ Each SDO has to be defined by the defineSdo () service.
*++ The handling of server SDOs is hidden in the library.
*++ For client SDO usage the functions
*++ writeSdoReq() and readSdoReq()
*++ are defined in this module.
*-- Dieses Modul enthält Funktionen für Service Daten Objekte (SDO).
*-- Jedes SDO ist mit der Funktion defineSdo() anzulegen.
*-- Die Abarbeitung der Dienste für Server-SDOs ist transparent
*-- für den Anwender.
*-- Zur Nutzung von Client-SDOs stehen die Dienste
*-- writeSdoReq() und readSdoReq()
*-- zur Verfügung.
*
*++ The CANopen Library supports up to 128 client
*++ and 128 server SDOs.
*-- Von der CANopen Library werden bis zu 128 Client-
*-- und 128 Server-SDOs unterstützt.
*
*++ The CANopen Library by \em port supports the down- and upload
*++ of domains e.g. programs up to a size of 4 294 967 295 bytes.
*-- Die CANopen Library von \em port unterstützt das Herunter- und Hinaufladen
*-- von Domains z.B Programmen bis zu einer Größe von 4 294 967 295 Bytes.
*
*++ Each SDO transfer is always initialized by a SDO client
*++ with the function
*-- Ein SDO Transfer wird immer vom SDO Client
*-- über die Funktion
*  readSdoReq()
*++ or
*-- bzw.
* writeSdoReq()
*++ , respectively.
*-- eingeleitet.
*++ After the transmission is finished or a timeout has been occured
*++ an indication function
*-- Nach dem Abschluss der Übertragung bzw. einem Timeout
*-- wird die entsprechende Indikation- bzw. Confirmation-Funktion
* (sdoRdCon(), sdoWrCon(), sdoRdInd(), sdoWrInd())
*++ is called.
*-- aufgerufen.
*
* \section d Domain Download
*++ For SDO domain transfers an information during the transmission
*++ for server SDOs can be activated with the define
*-- Für einen SDO Domaintransfer kann eine Benachrichtigung während
*-- der Übertragung für Server-SDOs mit dem define
* \c CONFIG_DOMAIN_INDICATION_SIZE
*-- aktiviert werden.
*++ In this case the indication function
*-- Damit wird nach dem Empfang von
*-- n * CONFIG_DOMAIN_INDICATION_SIZE Bytes
*-- die Indikationfunktion
* sdoDomainInd()
*++ is called after every
*++ n * \c CONFIG_DOMAIN_INDICATION_SIZE bytes.
*-- aufgerufen.
*++ For SDO client connections the define
*-- Für Client SDO Verbindungen
*-- ist das define
* \c CONFIG_DOMAIN_CONFIRMATION
*++ must be set.
*-- zu setzen.
*++ The transmission must be started by
*-- Die Übertragung muss mit der Funktion
* writeSdoDomainReq()
*-- eingeleitet werden.
*++ After n * 7 Bytes are transmitted/received
*++ the confirmation function
*-- Damit wird nach der Übertragung bzw. dem Empfang
*-- von n * 7 Bytes
*-- die Confirmationfunktion
* sdoDomainCon()
*++ is called.
*-- aufgerufen.
*
*++ \section f Further Reading
*-- \section f Weitere Hinweise
*++ This module is very scalable through compiler defines.
*++ Please see the appendix in the CANopen User Manual.
*-- Durch Compiler Direktiven ist dieses Modul in großem Umfang
*-- in der Kodegröße skalierbar
*-- (siehe im Anhang des CANopen Library UserManual).
*/


/* header of standard C - libraries */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/* header of project specific types */

#include <cal_conf.h>
#include <co_odidx.h>
#include <co_mcpy.h>
#include <co_debug.h>
#include <co_timer.h>
#include "sdo.h"
#include "nmt.h"
#include "access.h"
#include "cmscodec.h"
#include "drv.h"
#include "utility.h"

#ifdef CONFIG_SDO_BLOCKTRANSFER
# include "sdoblock.h"
#endif /* CONFIG_SDO_BLOCKTRANSFER */

#ifdef CONFIG_REDUNDANCY_SUPPORT
# include "reduncy.h"
#endif /* CONFIG_REDUNDANCY_SUPPORT */

/* constant definitions
---------------------------------------------------------------------------*/
#ifdef CONFIG_DYN_MEM_ALLOC
# define SDO_SERVER_CNT		co_maxSdoServer
# define SDO_CLIENT_CNT		co_maxSdoClient
#else /* CONFIG_DYN_MEM_ALLOC */
# define SDO_SERVER_CNT		CONFIG_SDO_SERVER
# define SDO_CLIENT_CNT		CONFIG_SDO_CLIENT
#endif /* CONFIG_DYN_MEM_ALLOC */

/* local defined data types
---------------------------------------------------------------------------*/

/* list of external used functions, if not in headers
---------------------------------------------------------------------------*/

/* list of global defined functions
---------------------------------------------------------------------------*/

/* list of local defined functions
---------------------------------------------------------------------------*/
#ifdef CONFIG_SDO_CLIENT
static RET_T pcoInitSdoReq( SDO_CLIENT_T **ppSdo, UNSIGNED8   sdoNr,
	UNSIGNED16  index, UNSIGNED8   subIndex, UNSIGNED32  timeOut
	CO_COMMA_LINE_PARA_DECL	);
#endif /* CONFIG_SDO_CLIENT */




/* external variables
---------------------------------------------------------------------------*/

/* global variables
---------------------------------------------------------------------------*/
#ifdef CONFIG_NO_GLOBAL_VARS
#else /* CONFIG_NO_GLOBAL_VARS */
# ifdef CONFIG_SDO_CLIENT
#  ifdef CONFIG_DYN_MEM_ALLOC
CO_LIB_UNINIT_VAR SDO_CLIENT_T	*p_co_sdoClient[1];
CO_LIB_UNINIT_VAR UNSIGNED16	co_maxSdoClient;
#  else /* CONFIG_DYN_MEM_ALLOC */
		/* sdo client data */
CO_LIB_UNINIT_VAR SDO_CLIENT_T	co_sdoClient[CONFIG_SDO_CLIENT];
#  endif /* CONFIG_DYN_MEM_ALLOC */
		/* actual sdo client cnt */
CO_LIB_UNINIT_VAR INTEGER8	co_sdoClientCnt CO_LINE_PARA_ARRAY_DEF;

#  ifdef CONFIG_MULT_LINES
		/* sdo client line counters */
#   ifdef CONFIG_DYN_MEM_ALLOC
CO_LIB_UNINIT_VAR UNSIGNED8	co_sdoClientLineCnts[CO_MAX_CAN_LINES];
#   else /* CONFIG_DYN_MEM_ALLOC */
CO_LIB_CONST_VAR UNSIGNED8	co_sdoClientLineCnts[CO_MAX_CAN_LINES] =
			    { CONFIG_SDO_CLIENT_LINECFG };
#   endif /* CONFIG_DYN_MEM_ALLOC */
		/* sdo client line offsets */
CO_LIB_UNINIT_VAR UNSIGNED16	co_sdoClientLineOffs CO_LINE_PARA_ARRAY_DEF;
#  else /* CONFIG_MULT_LINES */
#define co_sdoClientLineCnts	CONFIG_SDO_CLIENT
#  endif /* CONFIG_MULT_LINES */

#  ifdef CONFIG_FAST_SORT
#   ifdef CONFIG_DYN_MEM_ALLOC
CO_LIB_UNINIT_VAR UNSIGNED8	*p_co_sdoClientNrList[CONFIG_SDO_CLIENT];
CO_LIB_UNINIT_VAR UNSIGNED8	*p_co_sdoClientCobIdxList[CONFIG_SDO_CLIENT];
#   else /* CONFIG_DYN_MEM_ALLOC */
CO_LIB_UNINIT_VAR UNSIGNED8	co_sdoClientNrList[CONFIG_SDO_CLIENT];
CO_LIB_UNINIT_VAR UNSIGNED8	co_sdoClientCobIdxList[CONFIG_SDO_CLIENT];
#   endif /* CONFIG_DYN_MEM_ALLOC */
#  endif /* CONFIG_FAST_SORT */
# endif /* CONFIG_SDO_CLIENT */

# ifdef CONFIG_SDO_SERVER
#  ifdef CONFIG_DYN_MEM_ALLOC
CO_LIB_UNINIT_VAR SDO_T		*p_co_sdoServer[1];
CO_LIB_UNINIT_VAR UNSIGNED16	co_maxSdoServer;
#  else /* CONFIG_DYN_MEM_ALLOC */
		/* sdo server data */
CO_LIB_UNINIT_VAR SDO_T		co_sdoServer[CONFIG_SDO_SERVER];
#  endif /* CONFIG_DYN_MEM_ALLOC */
		/* actual sdo server cnt */
CO_LIB_UNINIT_VAR INTEGER8	co_sdoServerCnt CO_LINE_PARA_ARRAY_DEF;

#  ifdef CONFIG_MULT_LINES
		/* sdo server line counters */
#   ifdef CONFIG_DYN_MEM_ALLOC
CO_LIB_UNINIT_VAR UNSIGNED8	co_sdoServerLineCnts[CO_MAX_CAN_LINES];
#   else /* CONFIG_DYN_MEM_ALLOC */
CO_LIB_CONST_VAR UNSIGNED8	co_sdoServerLineCnts[CO_MAX_CAN_LINES] =
			    { CONFIG_SDO_SERVER_LINECFG };
#   endif /* CONFIG_DYN_MEM_ALLOC */
		/* sdo server line offsets */
CO_LIB_UNINIT_VAR UNSIGNED16	co_sdoServerLineOffs CO_LINE_PARA_ARRAY_DEF;
#  endif /* CONFIG_MULT_LINES */

#  ifdef CONFIG_FAST_SORT
#   ifdef CONFIG_DYN_MEM_ALLOC
CO_LIB_UNINIT_VAR UNSIGNED8	*p_co_sdoServerNrList[1];
CO_LIB_UNINIT_VAR UNSIGNED8	*p_co_sdoServerCobIdxList[1];
#   else /* CONFIG_DYN_MEM_ALLOC */
CO_LIB_UNINIT_VAR UNSIGNED8	co_sdoServerNrList[CONFIG_SDO_SERVER];
CO_LIB_UNINIT_VAR UNSIGNED8	co_sdoServerCobIdxList[CONFIG_SDO_SERVER];
#   endif /* CONFIG_DYN_MEM_ALLOC */
#  endif /* CONFIG_FAST_SORT */
# endif /* CONFIG_SDO_SERVER */
#endif /* CONFIG_NO_GLOBAL_VARS */



/* local defined variables
---------------------------------------------------------------------------*/
#ifdef CONFIG_RCS_IDENT
CO_LIB_INIT_VAR static char _rcsid[] = "$Id: sdo.c,v 2.59 2016/09/26 11:16:08 rli Exp $";
#endif /* CONFIG_RCS_IDENT */



#if defined(CONFIG_SDO_SERVER) || defined(CONFIG_SDO_CLIENT)
/****************************************************************************/
/**
*++ \brief defineSdo - defines a SDO
*-- \brief defineSdo - definiert ein SDO
*
*++ This function defines a Service Data Object SDO.
*++ The user has to ensure that the number of SDO is unique
*++ between 1...127.
*-- Diese Funktion definiert ein SDO Objekt:
*-- Der Anwender sorgt für eine eindeutige Nummernvergabe von 1..127.
*++ \em kindOfUse can be \c CLIENT or \c SERVER
*-- \em kindOfUse kann \c CLIENT oder \c SERVER sein
*
*++ For the RSDO/TSDO pair of the first Server-SDO the resulting COB-IDs
*++ will be computed from the Node ID according to DS301
*++ to match to the predefined connection set.
*-- Die COB-IDs für das RSDO/TSDO Paar des ersten Server-SDO werden
*-- anhand der Node-ID gemäß DS301 berechnet.
*-- Sie entsprechen somit dem Predefined Connection Set.
*
* \code
* 1st  Receive  SDO     1536 (0x600) + node ID
* 1st  Transmit SDO     1408 (0x580) + node ID
* \endcode
*
*++ All other SDO COB-IDs are set to ( 0x80000000 | 1760) = 0x80006E0 ).
*++ Thus they are disabled after they are defined.
*++ To change their COB-ID the object dictionary must be modified
*++ and
*++ setCommPar()
*++ has to be called in order to set the internal values.
*-- Alle anderen SDOs erhalten eine COB-ID von
*-- (0x80000000 | 1760) = 0x80006E0.
*-- Nach der Initialisierung sind sie zunächst deaktiviert.
*-- Die COB-ID kann mit der Funktion
*-- setCobId()
*-- geändert werden.
*-- Gleichzeitig werden die internen Werte gültig gesetzt.
*
* \code
* defineSdo(2, CLIENT);               // define SDO 2 as client SDO
* cobId = 1200;                       // new COB-ID
* setCobId(0x1281, 1, cobId);         // set new Value to OD
* \endcode
*
*++ \note
*
*++ If only one Server-SDO should be available at the device, it is not
*++ necessary to have an object dictionary entry for the SDOs.
*++ In such a case don't set the compiler directive CONFIG_SDO_COB_ID.
*-- Falls nur eine Server-SDO auf dem Gerät implementiert werden soll, ist
*-- es nicht notwendig Einträge für die SDOs im Objektverzeichnis zu haben.
*-- In diesem Falle ist die Compilerdirektive CONFIG_SDO_COB_ID nicht zu setzen.
*
* \retval CO_OK
*++ success
*-- Erfolg
* \retval CO_E_MEM
*++ memory allocation fault
*-- Speicherallozierungsfehler
* \retval CO_E_NO_ACCESS
*++ no access to object dictionary (COB-ID SDO or Node-ID)
*-- Zugriffsfehler auf das Objektverzeichnis (COB-ID SDO or Node-ID)
* \retval CO_E_TRANS_TYPE
*++ bad transmission type requested
*-- Nicht compilierter Transmission Type angefordert
*
*/

RET_T defineSdo(
	UNSIGNED8 sdoNr,    /**< number of SDO	 */
	USER_T kindOfUse    /**< kind of using the SDO */
	CO_COMMA_LINE_PARA_DECL	/**< number of CAN line 0..CO_MAX_CAN_LINES-1 */
    )
{
UNSIGNED32	cobIdReq;	/* COB-ID for Request/Indication*/
UNSIGNED32	cobIdRes;	/* COB-ID for Response/Confirm */
UNSIGNED16	baseIndex = 0u;	/* base for client or server SDO index */
UNSIGNED32	size;		/* size of object */
RET_T		retVal;
SDO_T		*pSdo;		/* pointer to actual sdo structure */
COB_KIND_T	cobType1, cobType2;
OBJDIR_T	*pObj = NULL;   /* pointer to SDO object */
# ifdef CONFIG_SDO_CLIENT
SDO_CLIENT_T	*pClientSdo;	/* pointer to actual sdo structure */
# endif /* CONFIG_SDO_CLIENT */

    /* detect kind of use */
    if (kindOfUse == SERVER) {
# ifdef CONFIG_SDO_SERVER

	/* free entry available ? */
	if ((UNSIGNED8)GL_ARRAY(co_sdoServerCnt) >=
#  ifdef CONFIG_MULT_LINES
		GL_ARRAY(co_sdoServerLineCnts)
#  else /* CONFIG_MULT_LINES */
		SDO_SERVER_CNT
#  endif /* CONFIG_MULT_LINES */
		)  {
	    return(CO_E_NO_DATABASE);
	}

	pSdo = &GL_PVAR(co_sdoServer)[GL_ARRAY(co_sdoServerCnt)
#  ifdef CONFIG_MULT_LINES
		+ GL_ARRAY(co_sdoServerLineOffs)
#  endif /* CONFIG_MULT_LINES */
		];

	GL_ARRAY(co_sdoServerCnt) ++;

	baseIndex = SSDO_PARA_BASE_INDEX;
	pSdo->num = sdoNr;
	pSdo->userType = SERVER;

#  ifdef CONFIG_FAST_SORT
 	/* sort into nr list */
	sortNodeIdList(
#   ifdef CONFIG_MULT_LINES
	    &GL_PVAR(co_sdoServerNrList)[GL_ARRAY(co_sdoServerLineOffs)],
	    &GL_PVAR(co_sdoServer)[GL_ARRAY(co_sdoServerLineOffs)].num,
#   else /* CONFIG_MULT_LINES */
	    &GL_PVAR(co_sdoServerNrList)[0],
	    &GL_PVAR(co_sdoServer)[0].num,
#   endif /* CONFIG_MULT_LINES */
	    (UNSIGNED8) sizeof(SDO_T),
	    (UNSIGNED8) GL_ARRAY(co_sdoServerCnt));
#  endif /* CONFIG_FAST_SORT */

	/* set default values for first server sdo */
	if (sdoNr == 1u) {
	    /* default COB-ID */
	    cobIdReq = CO_COBID_CSDO
		+ (UNSIGNED32)GL_ARRAY(coNodeId);
	    /* default COB-ID */
	    cobIdRes = CO_COBID_SSDO
		+ (UNSIGNED32)GL_ARRAY(coNodeId);
	}

	cobType1 = CO_COB_SDO_RX;
	cobType2 = CO_COB_SDO_TX;

# else /* CONFIG_SDO_SERVER */
	    return(CO_E_TRANS_TYPE);
# endif /* CONFIG_SDO_SERVER */

    } else {
# ifdef CONFIG_SDO_CLIENT
	/* client sdo */
	/* free entry available ? */
	if (GL_ARRAY(co_sdoClientCnt) >=
#  ifdef CONFIG_MULT_LINES
	    GL_ARRAY(co_sdoClientLineCnts)
#  else /* CONFIG_MULT_LINES */
	    SDO_CLIENT_CNT
#  endif /* CONFIG_MULT_LINES */
		)  {
	    return(CO_E_NO_DATABASE);
	}

	pClientSdo = &GL_PVAR(co_sdoClient)[GL_ARRAY(co_sdoClientCnt)
#  ifdef CONFIG_MULT_LINES
		+ GL_ARRAY(co_sdoClientLineOffs)
#  endif /* CONFIG_MULT_LINES */
		];
	pSdo = &pClientSdo->sdo;
	GL_ARRAY(co_sdoClientCnt) ++;

	baseIndex = CSDO_PARA_BASE_INDEX;
	pSdo->num = sdoNr;
	pSdo->userType = CLIENT;

#  ifdef CONFIG_FAST_SORT
	sortNodeIdList(
#   ifdef CONFIG_MULT_LINES
	    &GL_PVAR(co_sdoClientNrList)[GL_ARRAY(co_sdoClientLineOffs)],
	    &GL_PVAR(co_sdoClient)[GL_ARRAY(co_sdoClientLineOffs)].sdo.num,
#   else /* CONFIG_MULT_LINES */
	    &GL_PVAR(co_sdoClientNrList)[0],
	    &GL_PVAR(co_sdoClient)[0].sdo.num,
#   endif /* CONFIG_MULT_LINES */
	    (UNSIGNED8) sizeof(SDO_CLIENT_T),
	    (UNSIGNED8) GL_ARRAY(co_sdoClientCnt));
#  endif /* CONFIG_FAST_SORT */

	cobType1 = CO_COB_SDO_TX;
	cobType2 = CO_COB_SDO_RX;

# else /* CONFIG_SDO_CLIENT */
	return(CO_E_TRANS_TYPE);
# endif /* CONFIG_SDO_CLIENT */
    }

    /* read COB-ID from object dictionary */
    retVal = getObjPtrAtIndex(baseIndex + (UNSIGNED16)sdoNr - 1u, &pObj
		CO_COMMA_LINE_PARA);
# if defined(CONFIG_SDO_COB_ID) || defined(CONFIG_SDO_CLIENT)
#  ifndef CONFIG_DYN_MEM_ALLOC
    if (retVal != CO_OK)  {
	return CO_E_NO_ACCESS;
    }
#  endif /* CONFIG_DYN_MEM_ALLOC */
# endif /* defined(CONFIG_SDO_COB_ID) || defined(CONFIG_SDO_CLIENT) */

    retVal = getObjPtrEntry(pObj, baseIndex + (UNSIGNED16)sdoNr - 1u, 1u,
	(UNSIGNED8 *)&cobIdReq, &size, CO_TRUE  CO_COMMA_LINE_PARA);
# if defined(CONFIG_SDO_COB_ID) || defined(CONFIG_SDO_CLIENT)
    if (retVal != CO_OK)  {
	return CO_E_NO_ACCESS;
    }
# endif /* defined(CONFIG_SDO_COB_ID) || defined(CONFIG_SDO_CLIENT) */

    retVal = getObjPtrEntry(pObj, baseIndex + (UNSIGNED16)sdoNr - 1u, 2u,
	(UNSIGNED8 *)&cobIdRes, &size , CO_TRUE CO_COMMA_LINE_PARA);
# if defined(CONFIG_SDO_COB_ID) || defined(CONFIG_SDO_CLIENT)
    if (retVal != CO_OK)  {
	return CO_E_NO_ACCESS;
    }
# endif /* defined(CONFIG_SDO_COB_ID) || defined(CONFIG_SDO_CLIENT) */

    /* fill in domain attributes */
    pSdo->flags = 0u;

    pSdo->state = SDOSTATE_DISABLED;

# ifdef CONFIG_SDO_BLOCKTRANSFER
    pSdo->blkSegDefaultSize = CONFIG_BLOCK_MAX_CNT;

#  ifdef CO_CONFIG_BLOCKTRANSFER_INHIBITED_SEND
    pSdo->inhibit.ticks = 0u;
    pSdo->inhibitTime = CO_CONFIG_BLOCKTRANSFER_INHIBITED_SEND;
#  endif /* CO_CONFIG_BLOCKTRANSFER_INHIBITED_SEND */
# endif /* CONFIG_SDO_BLOCKTRANSFER */

# ifdef CO_CONFIG_SDO_EXPEDITED_NO_VALID_SIZE_BIT
    pSdo->expedited_sdo_with_valid_size_bit = CO_TRUE;
# endif /* CO_CONFIG_SDO_EXPEDITED_NO_VALID_SIZE_BIT */

    pSdo->pTrCOB = DEFINE_COB(CO_COB_SDO_TX, 8u CO_COMMA_LINE_PARA);
    if (pSdo->pTrCOB == NULL) {
	return(CO_E_NO_DATABASE);
    }

    pSdo->pRecCOB = DEFINE_COB(CO_COB_SDO_RX, 8u CO_COMMA_LINE_PARA);
    if (pSdo->pRecCOB == NULL) {
	return(CO_E_NO_DATABASE);
    }

    retVal = pcoSetSdoPtrCobId(pSdo, cobIdReq, kindOfUse, cobType1
	CO_COMMA_LINE_PARA);

    if (retVal == CO_OK)  {
        retVal = pcoSetSdoPtrCobId(pSdo, cobIdRes, kindOfUse, cobType2
	CO_COMMA_LINE_PARA);
    }

    return(retVal);
}
#endif /* defined(CONFIG_SDO_SERVER) || defined(CONFIG_SDO_CLIENT) */


#ifdef CONFIG_SDO_CLIENT
/****************************************************************************/
/**
*++ \brief writeSdoReq - write a dataobject to the server's object dictionary
*-- \brief writeSdoReq - schreibt ein Datenobjekt zum Serverobjektverzeichnis
*
*++ The function writes data objects, which are referenced by the index and
*++ subindex to the servers object dictionary.
*++ After a successful confirmation by sdoWrCon() the value is valid
*++ on the remote device.
*++ Errors on transmission have to be checked within
*++ sdoWrCon() .
*-- Diese Funktion schreibt Datenobjekte, die über die Angabe von Index
*-- und Subindex addressiert werden zum Objektverzeichnis des SDO Servers.
*-- Nach erfolgreicher Bestätigung durch die Funktion sdoWrCon()
*-- ist der geschriebene Wert in dem angesprochenen Gerät gültig.
*-- Treten Fehler beim Schreibzugriff auf, so sind diese innerhalb von
*-- sdoWrCon() auszuwerten.
*
*++ There is an exception for big endian devices (\c CONFIG_BIG_ENDIAN is set)
*++ and real 16-bit CPUs (\c CONFIG_16BIT_CPU is set).
*++ For these the bit 7 of the parameter
*++ \b sdo
*++ has to be set, if numeric values
*++ should be transmited e.g. \c sdo \c = \c sdo \c | \c 0x80.
*++ It is recommended to use the constant \c CO_NUM_SDO in order to
*++ ensure the portability of the application.
*
*-- Für Big-Endian-Geräte (\c CONFIG_BIG_ENDIAN ist gesetzt)
*-- und real 16-bit CPUs (\c CONFIG_16BIT_CPU is set).
*-- gibt es eine Ausnahme.
*-- Sollen numerische Werte übertragen werden, so ist das Bit 7
*-- des Parameters sdo zu setzen z.B. sdo = sdo | 0x80.
*-- Soll Portabilität der Programme gesichert werden, so wird empfohlen
*-- die Konstante \c CO_NUM_SDO zur SDO-Nummer zu addieren.
*
*++ The following example uses the
*++ writeSdoReq()
*++ to set the COB-ID of the first receive PDO of another node.
*-- Das folgende Beispiel zeigt,
*-- wie man den
*-- writeSdoReq()
*-- benutzt, um die COB-ID des ersten Empfangs PDO eines anderen Knoten
*-- umzukonfigurieren.
*
* \code
* UNSIGNED32 cobId;
* UNSIGNED32 timeout; // time in 1/10 ms
*
* timeout = 5000;  // wait 500 ms for response
*
* // deactivate SDO
* cobId = SDO_NO_VALID_BIT;
* ret = writeSdoReq(CO_NUM_SDO + 1, 0x1400, 1, &cobId, 4, timeout);
*
* // activate SDO with new COB-ID
* cobId = 200;
* ret = writeSdoReq(CO_NUM_SDO + 1, 0x1400, 1, &cobId, 4, timeout);
* \endcode
*
* \retval CO_OK
*++ success
*-- Erfolg
* \retval CO_E_NOT_EXIST
*++ SDO with current code number doesn't exist
*-- SDO mit aktueller Nummer existiert nicht
* \retval CO_E_STATE
*++ node isn't in state OPERATIONAL or PRE_OPERATIONAL
*-- Knoten ist nicht im Zustand OPERATIONAL oder PRE_OPERATIONAL
* \retval CO_E_TYPE
*++ SDO usertype isn't CLIENT
*-- SDO Nutzertyp ist nicht CLIENT
* \retval CO_E_BUSY
*++ an automatic domain transfer is still operating for this SDO
*-- Ein automatischer Domaintransfer für diese SDO ist noch aktiv
* \retval CO_E_DISABLED
*++ selected SDO is disabled
*-- Das gewählte SDO ist inaktiv
* \retval CO_E_TIMEOUT
*++ start timeout failed
*-- Start timeout fehlerhaft
*/
RET_T writeSdoReq(
      UNSIGNED8   sdoNr,	/**< number of SDO */
      UNSIGNED16  index,	/**< index of object dictionary */
      UNSIGNED8   subIndex,	/**< subindex of object dictionary */
      UNSIGNED8   *pData,	/**< sdo data address */
      UNSIGNED32  length,	/**< length of sdo data */
      UNSIGNED32  timeOut	/**< timeout in 1/10 msec */
      CO_COMMA_LINE_PARA_DECL	/**< number of CAN line 0..CO_MAX_CAN_LINES-1 */
      )
{
SDO_CLIENT_T	*pSdo;		/* pointer to current sdo */
RET_T		retVal;		/* return value */

    retVal = pcoInitSdoReq(&pSdo, sdoNr, index, subIndex, timeOut CO_COMMA_LINE_PARA );
    if ( retVal != CO_OK ) {
        return retVal;
    }

    retVal = initUpDnLd_req(pSdo, pData, length,
# ifdef CONFIG_SDO_BLOCKTRANSFER
		CO_SDOBLK_CCS_DOWN
# else /* CONFIG_SDO_BLOCKTRANSFER */
		CCS_INI_DN_LD_REQ
# endif /* CONFIG_SDO_BLOCKTRANSFER */
		CO_COMMA_LINE_PARA);

    if (retVal != CO_OK)  {
	return(retVal);
    }

    pSdo->upDnType = SDO_DOWNLOAD;

    return(CO_OK);
}


/****************************************************************************/
/**
*++ \brief readSdoReq - read a data object from the server's object dictionary
*-- \brief readSdoReq - liest ein Datenobjekt vom Serverobjektverzeichnis
*
*++ The function reads data objects,
*++ which are referenced by the index and subindex
*++ from the servers object dictionary.
*++ After a successful confirmation in sdoRdCon() the value was received
*++ from the remote device.
*++ Errors on transmission have to be checked within
*++ sdoRdCon().
*-- Diese Funktion liest Datenobjekte, die über die Angabe von Index
*-- und Subindex adressiert werden, vom Objektverzeichnis des SDO Servers.
*-- Nach erfolgreicher Bestätigung durch die Funktion sdoRdCon()
*-- ist der gelesene Wert auf dem lokalen Gerät gültig.
*-- Treten Fehler beim Lesezugriff auf, so sind diese innerhalb von
*-- sdoRdCon() auszuwerten.
*
*++ There is an exception for big endian devices (\c CONFIG_BIG_ENDIAN is set)
*++ and real 16-bit CPUs (\c CONFIG_16BIT_CPU is set).
*++ For these the bit 7 of the parameter
*++ \b sdo
*++ has to be set, if numeric values
*++ should be transmited e.g. \c sdo \c = \c sdo \c | \c 0x80.
*++ It is recommended to use the constant \c CO_NUM_SDO in order to
*++ ensure the portability of the application.
*
*++ The following example uses the
*++ readSdoReq()
*++ to read a value from a remote device.
*-- Das folgende Beispiel zeigt,
*-- wie man den
*-- readSdoReq()
*-- zum Lesen von Werten von einem anderen Gerät verwendet.
*
* \code
* UNSIGNED32 value;
* UNSIGNED32 timeout; // time in 1/10 ms
*
* timeout = 5000;  // wait 500 ms for response
*
* ret = readSdoReq(CO_NUM_SDO + 1, 0x2000, 0, &value, 4, timeout);
* \endcode
*
* \retval CO_OK
*++ success
*-- Erfolg
* \retval CO_E_NOT_EXIST
*++ SDO with current codenumber doesn't exist
*-- SDO mit aktueller Nummer existiert nicht
* \retval CO_E_STATE
*++ node isn't in state OPERATIONAL or PRE_OPERATIONAL
*-- Knoten ist nicht im Zustand OPERATIONAL oder PRE_OPERATIONAL
* \retval CO_E_TYPE
*++ SDO usertype isn't CLIENT
*-- SDO Nutzertyp ist nicht CLIENT
* \retval CO_E_BUSY
*++ an automatic domain transfer is still operating for this SDO
*-- Ein automatischer Domaintransfer für diese SDO ist noch aktiv
* \retval CO_E_DISABLED
*++ selected SDO is disabled
*-- Die gewählte SDO ist deaktiviert
*
*/
RET_T readSdoReq(
	UNSIGNED8   sdoNr,	/**< Codenumber of SDO */
	UNSIGNED16  index,    	/**< index of object dictionary */
	UNSIGNED8   subIndex,	/**< subindex of object dictionary */
	UNSIGNED8   *pObj,	/**< pointer to destination address for object*/
	UNSIGNED32  size,		/**< size of object */
	UNSIGNED32  timeOut	/**< timeout in 1/10 msec */
	CO_COMMA_LINE_PARA_DECL	/**< number of CAN line 0..CO_MAX_CAN_LINES-1 */
    )
{
SDO_CLIENT_T	*pSdo;		/* pointer to current sdo */
RET_T		retVal;		/* return value */


    retVal = pcoInitSdoReq(&pSdo, sdoNr, index, subIndex, timeOut CO_COMMA_LINE_PARA );
    if ( retVal != CO_OK ) {
        return retVal;
    }

    retVal = initUpDnLd_req(pSdo, pObj, size,
# ifdef CONFIG_SDO_BLOCKTRANSFER
		CO_SDOBLK_CCS_UP
# else /* CONFIG_SDO_BLOCKTRANSFER */
		CCS_INI_UP_LD_REQ
# endif /* CONFIG_SDO_BLOCKTRANSFER */
		CO_COMMA_LINE_PARA);

    if (retVal != CO_OK)  {
	return(retVal);
    }

    pSdo->upDnType = SDO_UPLOAD;

    return(CO_OK);
}


/****************************************************************************/
/**
*++ \brief writeSdoSegReq - write a object on the server with segmented sdo
*-- \brief writeSdoSegReq - schreibt ein Ojekt auf dem Server mit segementierten SDO
*
*++ The function writes data objects, which are referenced by the index and
*++ subindex to the servers object dictionary.
*++ After a successful confirmation by sdoWrCon() the value is valid
*++ on the remote device.
*++ Errors on transmission have to be checked within
*++ sdoWrCon() .
*-- Diese Funktion schreibt Datenobjekte, die über die Angabe von Index
*-- und Subindex addressiert werden zum Objektverzeichnis des SDO Servers.
*-- Nach erfolgreicher Bestätigung durch die Funktion sdoWrCon()
*-- ist der geschriebene Wert in dem angesprochenen Gerät gültig.
*-- Treten Fehler beim Schreibzugriff auf, so sind diese innerhalb von
*-- sdoWrCon() auszuwerten.
*
*++ There is an exception for big endian devices (\c CONFIG_BIG_ENDIAN is set)
*++ and real 16-bit CPUs (\c CONFIG_16BIT_CPU is set).
*++ For these the bit 7 of the parameter
*++ \b sdo
*++ has to be set, if numeric values
*++ should be transmited e.g. \c sdo \c = \c sdo \c | \c 0x80.
*++ It is recommended to use the constant \c CO_NUM_SDO in order to
*++ ensure the portability of the application.
*
*-- Für Big-Endian-Geräte (\c CONFIG_BIG_ENDIAN ist gesetzt)
*-- und real 16-bit CPUs (\c CONFIG_16BIT_CPU is set).
*-- gibt es eine Ausnahme.
*-- Sollen numerische Werte übertragen werden, so ist das Bit 7
*-- des Parameters sdo zu setzen z.B. sdo = sdo | 0x80.
*-- Soll Portabilität der Programme gesichert werden, so wird empfohlen
*-- die Konstante \c CO_NUM_SDO zur SDO-Nummer zu addieren.
*
*++ The following example uses the
*++ writeSdoSegReq()
*++ to set the COB-ID of the first receive PDO of another node.
*-- Das folgende Beispiel zeigt,
*-- wie man den
*-- writeSdoSegReq()
*-- benutzt, um die COB-ID des ersten Empfangs PDO eines anderen Knoten
*-- umzukonfigurieren.
*
* \code
* UNSIGNED32 cobId;
* UNSIGNED32 timeout; // time in 1/10 ms
*
* timeout = 5000;  // wait 500 ms for response
*
* // deactivate SDO
* cobId = SDO_NO_VALID_BIT;
* ret = writeSdoSegReq(CO_NUM_SDO + 1, 0x1400, 1, &cobId, 4, timeout);
*
* // activate SDO with new COB-ID
* cobId = 200;
* ret = writeSdoSegReq(CO_NUM_SDO + 1, 0x1400, 1, &cobId, 4, timeout);
* \endcode
*
* \retval CO_OK
*++ success
*-- Erfolg
* \retval CO_E_NOT_EXIST
*++ SDO with current code number doesn't exist
*-- SDO mit aktueller Nummer existiert nicht
* \retval CO_E_STATE
*++ node isn't in state OPERATIONAL or PRE_OPERATIONAL
*-- Knoten ist nicht im Zustand OPERATIONAL oder PRE_OPERATIONAL
* \retval CO_E_TYPE
*++ SDO usertype isn't CLIENT
*-- SDO Nutzertyp ist nicht CLIENT
* \retval CO_E_BUSY
*++ an automatic domain transfer is still operating for this SDO
*-- Ein automatischer Domaintransfer für diese SDO ist noch aktiv
* \retval CO_E_DISABLED
*++ selected SDO is disabled
*-- Das gewählte SDO ist inaktiv
* \retval CO_E_TIMEOUT
*++ start timeout failed
*-- Start timeout fehlerhaft
*/
RET_T writeSdoSegReq(
      UNSIGNED8   sdoNr,	/**< number of SDO */
      UNSIGNED16  index,	/**< index of object dictionary */
      UNSIGNED8   subIndex,	/**< subindex of object dictionary */
      UNSIGNED8   *pData,	/**< sdo data address */
      UNSIGNED32  length,	/**< length of sdo data */
      UNSIGNED32  timeOut	/**< timeout in 1/10 msec */
      CO_COMMA_LINE_PARA_DECL	/**< number of CAN line 0..CO_MAX_CAN_LINES-1 */
      )
{
SDO_CLIENT_T	*pSdo;		/* pointer to current sdo */
RET_T		retVal;		/* return value */

    retVal = pcoInitSdoReq(&pSdo, sdoNr, index, subIndex, timeOut CO_COMMA_LINE_PARA );
    if ( retVal != CO_OK ) {
        return retVal;
    }

    retVal = initUpDnLd_req(pSdo, pData, length,
		CCS_INI_DN_LD_REQ
		CO_COMMA_LINE_PARA);

    if (retVal != CO_OK)  {
	return(retVal);
    }

    pSdo->upDnType = SDO_DOWNLOAD;

    return(CO_OK);
}


/****************************************************************************/
/**
*++ \brief readSdoSegReq - read a data from a server with segmented sdo
*-- \brief readSdoSegReq - liest ein Daten vom Server mit segmentierten SDO
*
*++ The function reads data objects,
*++ which are referenced by the index and subindex
*++ from the servers object dictionary.
*++ After a successful confirmation in sdoRdCon() the value was received
*++ from the remote device.
*++ Errors on transmission have to be checked within
*++ sdoRdCon().
*-- Diese Funktion liest Datenobjekte, die über die Angabe von Index
*-- und Subindex adressiert werden, vom Objektverzeichnis des SDO Servers.
*-- Nach erfolgreicher Bestätigung durch die Funktion sdoRdCon()
*-- ist der gelesene Wert auf dem lokalen Gerät gültig.
*-- Treten Fehler beim Lesezugriff auf, so sind diese innerhalb von
*-- sdoRdCon() auszuwerten.
*
*++ There is an exception for big endian devices (\c CONFIG_BIG_ENDIAN is set)
*++ and real 16-bit CPUs (\c CONFIG_16BIT_CPU is set).
*++ For these the bit 7 of the parameter
*++ \b sdo
*++ has to be set, if numeric values
*++ should be transmited e.g. \c sdo \c = \c sdo \c | \c 0x80.
*++ It is recommended to use the constant \c CO_NUM_SDO in order to
*++ ensure the portability of the application.
*
*++ The following example uses the
*++ readSdoSegReq()
*++ to read a value from a remote device.
*-- Das folgende Beispiel zeigt,
*-- wie man den
*-- readSdoSegReq()
*-- zum Lesen von Werten von einem anderen Gerät verwendet.
*
* \code
* UNSIGNED32 value;
* UNSIGNED32 timeout; // time in 1/10 ms
*
* timeout = 5000;  // wait 500 ms for response
*
* ret = readSdoSegReq(CO_NUM_SDO + 1, 0x2000, 0, &value, 4, timeout);
* \endcode
*
* \retval CO_OK
*++ success
*-- Erfolg
* \retval CO_E_NOT_EXIST
*++ SDO with current codenumber doesn't exist
*-- SDO mit aktueller Nummer existiert nicht
* \retval CO_E_STATE
*++ node isn't in state OPERATIONAL or PRE_OPERATIONAL
*-- Knoten ist nicht im Zustand OPERATIONAL oder PRE_OPERATIONAL
* \retval CO_E_TYPE
*++ SDO usertype isn't CLIENT
*-- SDO Nutzertyp ist nicht CLIENT
* \retval CO_E_BUSY
*++ an automatic domain transfer is still operating for this SDO
*-- Ein automatischer Domaintransfer für diese SDO ist noch aktiv
* \retval CO_E_DISABLED
*++ selected SDO is disabled
*-- Die gewählte SDO ist deaktiviert
*
*/
RET_T readSdoSegReq(
	UNSIGNED8   sdoNr,	/**< Codenumber of SDO */
	UNSIGNED16  index,    	/**< index of object dictionary */
	UNSIGNED8   subIndex,	/**< subindex of object dictionary */
	UNSIGNED8   *pObj,	/**< pointer to destination address for object*/
	UNSIGNED32  size,		/**< size of object */
	UNSIGNED32  timeOut	/**< timeout in 1/10 msec */
	CO_COMMA_LINE_PARA_DECL	/**< number of CAN line 0..CO_MAX_CAN_LINES-1 */
    )
{
SDO_CLIENT_T	*pSdo;		/* pointer to current sdo */
RET_T		retVal;		/* return value */

    retVal = pcoInitSdoReq(&pSdo, sdoNr, index, subIndex, timeOut CO_COMMA_LINE_PARA );
    if ( retVal != CO_OK ) {
        return retVal;
    }
    retVal = initUpDnLd_req(pSdo, pObj, size,
		CCS_INI_UP_LD_REQ
		CO_COMMA_LINE_PARA);

    if (retVal != CO_OK)  {
	return(retVal);
    }

    pSdo->upDnType = SDO_UPLOAD;

    return(CO_OK);
}



/*******************************************************************
*
* pcoInitSdoReq - prepares an SDO request
*
* \internal
*
* Little helper function to prepare an SDO request. Used by
* - readSdoReq
* - writeSdoReq
* - readSdoSegReq
* - writeSdoSegReq
*
* RETURNS
* \retval RET_T
*
*/
static RET_T pcoInitSdoReq(
	SDO_CLIENT_T **ppSdo,
	UNSIGNED8   sdoNr,	/**< Codenumber of SDO */
	UNSIGNED16  index,    	/**< index of object dictionary */
	UNSIGNED8   subIndex,	/**< subindex of object dictionary */
	UNSIGNED32  timeOut	/**< timeout in 1/10 msec */
	CO_COMMA_LINE_PARA_DECL	/**< number of CAN line 0..CO_MAX_CAN_LINES-1 */
	    )
{
RET_T retVal = CO_OK;

    /* the setting of the global multiplexor is for more
       convenience and reliability */
    *ppSdo = searchForClientSdoNr(sdoNr & CO_NONUM_SDO CO_COMMA_LINE_PARA);
    if (*ppSdo == NULL) {
	return(CO_E_NOT_EXIST);
    }

    /* test for active transfer */
    if ((*ppSdo)->sdo.state != SDOSTATE_READY)  {
	return(CO_E_BUSY);
    }
    /* FIXME */
    (*ppSdo)->sdoConf = E_SDO_NO_ERROR;

# if defined(CONFIG_BIG_ENDIAN) || defined(CONFIG_16BIT_CPU)
    if ((sdoNr & CO_NUM_SDO) != 0)  {
	(*ppSdo)->sdo.numeric = CO_TRUE;
    } else {
	(*ppSdo)->sdo.numeric = CO_FALSE;
    }
# endif /* defined(CONFIG_BIG_ENDIAN) || defined(CONFIG_16BIT_CPU) */

    (*ppSdo)->sdo.index = index;
    (*ppSdo)->sdo.subIndex = subIndex;
# ifdef CONFIG_DOMAIN_CONFIRMATION
    (*ppSdo)->domainIndSize = 0;
# endif /* CONFIG_DOMAIN_CONFIRMATION */

    /* save timeout */
    if (timeOut == 0)  {
	return(CO_E_SDO_TIMEOUT);
    }
    (*ppSdo)->timeOut = timeOut;

# ifdef CONFIG_DOMAIN_CONFIRMATION
    (*ppSdo)->domainIndSize = 0;
# endif /* CONFIG_DOMAIN_CONFIRMATION */

# ifdef CONFIG_16BIT_CPU
    (*ppSdo)->sdo.halfWord = CO_FALSE;
# endif /* CONFIG_16BIT_CPU */

# ifdef CONFIG_REDUNDANCY_SUPPORT
#   ifdef CO_CONFIG_REDUNDANCY_ALLOW_SDO_LINE_SELECT
    if (((*ppSdo)->sdo.commLine != GL_VAR(co_redcyActiveLine))
        && ((*ppSdo)->sdo.commLine != GL_VAR(co_redcyInActiveLine)))
#   endif /* CO_CONFIG_REDUNDANCY_ALLOW_SDO_LINE_SELECT */
    {
        (*ppSdo)->sdo.commLine = GL_VAR(co_redcyActiveLine);	/* communication line */
    }
# endif /* CONFIG_REDUNDANCY_SUPPORT */


    return retVal;
}

# ifdef CONFIG_DOMAIN_CONFIRMATION
/****************************************************************************/
/**
*++ \brief writeSdoDomainReq - write a domain to the server's object dictionary
*-- \brief writeSdoDomainReq - schreibt eine Domain zum Serverobjektverzeichnis
*
*++ The function writes domain data, which are referenced by the index and
*++ subindex to the servers object dictionary.
*-- Diese Funktion schreibt Domaindaten, die über die Angabe von Index
*-- und Subindex addressiert werden zum Objektverzeichnis des SDO Servers.
*++ After the transmission of 7 * domSizeCnt bytes
*++ the confirmation function
*-- Nach der Übertragung von 7 * domSizeCnt Bytes wird die User-funktion
* sdoDomainCon()
*++ is called.
*-- aufgerufen,
*++ In this function the data in the transmit buffer should be actualized.
*-- damit die Daten im Sendepuffer aktualisiert werden können.
*++ After the confirmation function is finished,
*++ the transfer continous at the start of the transmit buffer.
*-- Anschliessend wird die Datenübertragung am Beginn des Sendepuffers
*-- wieder aufgemommen.
*++ If the transfer is finished or a timeout has occured
*++ the confirmation function
*-- Nach erfolgreicher Beendigung der Übertragung
*-- oder durch ein Timeout
*-- wird die Funktion
* sdoWrCon()
*++ is called.
*-- aufgerufen.
*++ After a successful confirmation by sdoWrCon() the value is valid
*++ on the remote device.
*++ Errors on transmission have to be checked within
*++ sdoWrCon() .
*-- Dann ist der geschriebene Wert in dem angesprochenen Gerät gültig.
*-- Treten Fehler beim Schreibzugriff auf, so sind diese innerhalb von
*-- sdoWrCon() auszuwerten.
*
*++ There is an exception for big endian devices (\c CONFIG_BIG_ENDIAN is set)
*++ and real 16-bit CPUs (\c CONFIG_16BIT_CPU is set).
*++ For these the bit 7 of the parameter
*++ \b sdo
*++ has to be set, if numeric values
*++ should be transmited e.g. \c sdo \c = \c sdo \c | \c 0x80.
*++ It is recommended to use the constant \c CO_NUM_SDO in order to
*++ ensure the portability of the application.
*-- Für Big-Endian-Geräte (\c CONFIG_BIG_ENDIAN ist gesetzt)
*-- und real 16-bit CPUs (\c CONFIG_16BIT_CPU is set).
*-- gibt es eine Ausnahme.
*-- Sollen numerische Werte übertragen werden, so ist das Bit 7
*-- des Parameters sdo zu setzen z.B. sdo = sdo | 0x80.
*-- Soll Portabilität der Programme gesichert werden, so wird empfohlen
*-- die Konstante \c CO_NUM_SDO zur SDO-Nummer zu addieren.
*
* \retval CO_OK
*++ success
*-- Erfolg
* \retval CO_E_NOT_EXIST
*++ SDO with current code number doesn't exist
*-- SDO mit aktueller Nummer existiert nicht
* \retval CO_E_STATE
*++ node isn't in state OPERATIONAL or PRE_OPERATIONAL
*-- Knoten ist nicht im Zustand OPERATIONAL oder PRE_OPERATIONAL
* \retval CO_E_TYPE
*++ SDO usertype isn't CLIENT
*-- SDO Nutzertyp ist nicht CLIENT
* \retval CO_E_BUSY
*++ an automatic domain transfer is still operating for this SDO
*-- Ein automatischer Domaintransfer für diese SDO ist noch aktiv
* \retval CO_E_DISABLED
*++ selected SDO is disabled
*-- Das gewählte SDO ist inaktiv
* \retval CO_E_TIMEOUT
*++ start timeout failed
*-- Start timeout fehlerhaft
*/

RET_T writeSdoDomainReq(
	UNSIGNED8   sdoNr,	/**< number of SDO */
	UNSIGNED16  index,	/**< index of object dictionary */
	UNSIGNED8   subIndex,	/**< subindex of object dictionary */
	UNSIGNED8   *pData,	/**< sdo data address */
	UNSIGNED32  length,	/**< length of sdo data */
	UNSIGNED32  domSizeCnt,	/**< cnt * 7 bytes for user indication */
	UNSIGNED32  timeOut	/**< timeout in 1/10 msec */
	CO_COMMA_LINE_PARA_DECL	/**< number of CAN line 0..CO_MAX_CAN_LINES-1 */
    )
{
SDO_CLIENT_T	*pSdo;		/* pointer to current sdo */
RET_T		retVal;		/* return value */

    /* start transfer */
    retVal = writeSdoReq(sdoNr, index, subIndex, pData, length,
	timeOut CO_COMMA_LINE_PARA);
    if (retVal != CO_OK)  {
	return(retVal);
    }

    /* look for sdo structure */
    pSdo = searchForClientSdoNr(sdoNr CO_COMMA_LINE_PARA);
    if (pSdo == NULL) {
	return(CO_E_NOT_EXIST);
    }

    /* save domain counter for domain indication */
    pSdo->domainIndSize = domSizeCnt * 7;
    /* calculate first border */
    pSdo->nextDomainIndBorder = pSdo->sdo.restSize - pSdo->domainIndSize;

    return(CO_OK);
}


/****************************************************************************/
/**
*++ \brief readSdoDomainReq - read a domain from the server's object dictionary
*-- \brief readSdoDomainReq - liest eine Domain vom Serverobjektverzeichnis
*
*++ The function reads domain data, which are referenced by the index and
*++ subindex from the servers object dictionary.
*++ After a successful confirmation by sdoRdCon() the value is valid
*++ on the local device.
*-- Diese Funktion liest Domaindaten, die über die Angabe von Index
*-- und Subindex addressiert werden, im Objektverzeichnis des SDO Servers.
*++ After the transmission of 7 * domSizeCnt bytes
*++ the confirmation function
*-- Nach der Übertragung von 7 * domSizeCnt Bytes wird die User-funktion
* sdoDomainCon()
*++ is called.
*-- aufgerufen,
*-- damit die Daten im Empfangspuffer gespeichert werden können.
*++ In this function the data in the receive buffer should be saved
*++ by the user.
*++ After the confirmation function is finished,
*++ the transfer continious at the start of the receive buffer.
*-- Anschliessend wird die Datenübertragung am Beginn des Empfangspuffers
*-- wieder aufgemommen.
*++ If the transfer is finished or a timeout was occured
*++ the confirmation function
*-- Nach erfolgreicher Beendigung der Übertragung
*-- oder durch ein Timeout
*-- wird die Funktion
* sdoRdCon()
*++ will be called.
*-- aufgerufen.
*++ Errors on transmission have to be checked within
*++ sdoRdCon() .
*-- Dann ist der geschriebene Wert im lokalen Gerät gültig.
*-- Treten Fehler beim Lesezugriff auf, so sind diese innerhalb von
*-- sdoRdCon() auszuwerten.
*
*++ There is an exception for big endian devices (\cCONFIG_BIG_ENDIAN is set)
*++ and real 16-bit CPUs (\c CONFIG_16BIT_CPU is set).
*++ For these the bit 7 of the parameter
*++ \b sdo
*++ has to be set, if numeric values
*++ should be transmited e.g. \c sdo \c = \c sdo \c | \c 0x80.
*++ It is recommended to use the constant \c CO_NUM_SDO in order to
*++ ensure the portability of the application.
*-- Für Big-Endian-Geräte (\c CONFIG_BIG_ENDIAN ist gesetzt)
*-- und real 16-bit CPUs (\c CONFIG_16BIT_CPU is set).
*-- gibt es eine Ausnahme.
*-- Sollen numerische Werte übertragen werden, so ist das Bit 7
*-- des Parameters sdo zu setzen z.B. sdo = sdo | 0x80.
*-- Soll Portabilität der Programme gesichert werden, so wird empfohlen
*-- die Konstante \c CO_NUM_SDO zur SDO-Nummer zu addieren.
*
* \retval CO_OK
*++ success
*-- Erfolg
* \retval CO_E_NOT_EXIST
*++ SDO with current code number doesn't exist
*-- SDO mit aktueller Nummer existiert nicht
* \retval CO_E_STATE
*++ node isn't in state OPERATIONAL or PRE_OPERATIONAL
*-- Knoten ist nicht im Zustand OPERATIONAL oder PRE_OPERATIONAL
* \retval CO_E_TYPE
*++ SDO usertype isn't CLIENT
*-- SDO Nutzertyp ist nicht CLIENT
* \retval CO_E_BUSY
*++ an automatic domain transfer is still operating for this SDO
*-- Ein automatischer Domaintransfer für diese SDO ist noch aktiv
* \retval CO_E_DISABLED
*++ selected SDO is disabled
*-- Das gewählte SDO ist inaktiv
* \retval CO_E_TIMEOUT
*++ start timeout failed
*-- Start timeout fehlerhaft
*/

RET_T readSdoDomainReq(
	UNSIGNED8   sdoNr,	/**< number of SDO */
	UNSIGNED16  index,	/**< index of object dictionary */
	UNSIGNED8   subIndex,	/**< subindex of object dictionary */
	UNSIGNED8   *pData,	/**< sdo data address */
	UNSIGNED32  length,	/**< length of sdo data */
	UNSIGNED32  domSizeCnt,	/**< cnt * 7 bytes for user indication */
	UNSIGNED32  timeOut	/**< timeout in 1/10 msec */
	CO_COMMA_LINE_PARA_DECL	/**< number of CAN line 0..CO_MAX_CAN_LINES-1 */
   )
{
SDO_CLIENT_T	*pSdo;		/* pointer to current sdo */
RET_T		retVal;		/* return value */

    /* start transfer */
    retVal = readSdoReq(sdoNr, index, subIndex, pData, length,
	timeOut CO_COMMA_LINE_PARA);
    if (retVal != CO_OK)  {
	return(retVal);
    }

    /* look for sdo structure */
    pSdo = searchForClientSdoNr(sdoNr CO_COMMA_LINE_PARA);
    if (pSdo == NULL) {
	return(CO_E_NOT_EXIST);
    }

    /* save domain counter for domain indication */
    pSdo->domainIndSize = domSizeCnt * 7;
    /* calculate first border */
    pSdo->nextDomainIndBorder = pSdo->sdo.restSize - pSdo->domainIndSize;

    return(CO_OK);
}
# endif /* CONFIG_DOMAIN_CONFIRMATION */
#endif /* CONFIG_SDO_CLIENT */


#if defined(CONFIG_SDO_SERVER) || defined(CONFIG_SDO_CLIENT)
/****************************************************************************/
/**
*++ \brief getSdoSize - get the size of data read by SDO
*-- \brief getSdoSize - ermittelt die Größe von per SDO gelesenen Daten
*
*++ This function returns the size of data read via SDO.
*++ This function is
*++ useful for data types with no fixed length e.g. Octet-Strings.
*++ For Visual-Strings this function is not necessary, because the
*++ CANopen Library appends an End of String character at the string
*++ if estimated size > real size.
*-- Diese Funktion ermittelt die reale Größe der per SDO gelesenen Daten.
*-- Sie wird genutzt für Datentypen ohne feste Länge z.B. Octet-Strings.
*-- Für Visual-Strings ist sie nicht notwendig, da die CANopen Bibliothek
*-- ein End of String - Zeichen anhängt, falls die geschützte Größe größer
*-- als die reale Größe ist.
*
* \code
* // for SDO Server
* sdoWrInd(UNSIGNED16 index, UNSIGNED8 subIndex)
* {
*     sdo = getActualSdo(index, subIndex);         // get number of active SDO
*     size = getSdoSize(sdo, SERVER);              // size of written data
* }
* \endcode
*
* \retval size
*++ size of data, at success of read SDO
*-- Datengröße, bei Erfolg von read SDO
* \retval 0
*++ undefined value, when read SDO not succesful
*-- Undefiniert Wert, wenn read SDO nicht erfolgreich
*
*/

UNSIGNED32 getSdoSize(
	UNSIGNED8   sdo, 	/**< number of SDO */
	USER_T      kindOfUse	/**< CLIENT/SERVER */
	CO_COMMA_LINE_PARA_DECL	/**< number of CAN line 0..CO_MAX_CAN_LINES-1 */
    )
{
#ifdef CONFIG_SDO_SERVER
SDO_T		*pServerSdo;
#endif /* CONFIG_SDO_SERVER */
#ifdef CONFIG_SDO_CLIENT
SDO_CLIENT_T	*pClientSdo;
#endif /* CONFIG_SDO_CLIENT */

#ifdef CONFIG_SDO_SERVER
    if (kindOfUse == SERVER)  {
	pServerSdo = searchForServerSdoNr(sdo & CO_NONUM_SDO CO_COMMA_LINE_PARA);
	if (pServerSdo == NULL)  {
	    return(0u);
	}
	return(pServerSdo->domSize);
    } else
#endif /* CONFIG_SDO_SERVER */
    {
#ifdef CONFIG_SDO_CLIENT
	pClientSdo = searchForClientSdoNr(sdo & CO_NONUM_SDO CO_COMMA_LINE_PARA);
	if (pClientSdo == NULL)  {
	    return(0u);
	}
	return(pClientSdo->sdo.domSize);
#else  /* CONFIG_SDO_CLIENT */
    return(0u);
#endif /* CONFIG_SDO_CLIENT */
    }
}
#endif /* defined(CONFIG_SDO_SERVER) || defined(CONFIG_SDO_CLIENT) */


#if defined(CONFIG_SDO_SERVER) || defined(CONFIG_SDO_CLIENT)
/****************************************************************************/
/**
*++ \brief co_getSdoRestSize
*-- \brief co_getSdoRestSize
*/

UNSIGNED32 co_getSdoRestSize(
	UNSIGNED8   sdo, 	/**< number of SDO */
	USER_T      kindOfUse	/**< CLIENT/SERVER */
	CO_COMMA_LINE_PARA_DECL	/**< number of CAN line 0..CO_MAX_CAN_LINES-1 */
    )
{
#ifdef CONFIG_SDO_SERVER
SDO_T		*pServerSdo;
#endif /* CONFIG_SDO_SERVER */
#ifdef CONFIG_SDO_CLIENT
SDO_CLIENT_T	*pClientSdo;
#endif /* CONFIG_SDO_CLIENT */

#ifdef CONFIG_SDO_SERVER
    if (kindOfUse == SERVER)  {
	pServerSdo = searchForServerSdoNr(sdo & CO_NONUM_SDO CO_COMMA_LINE_PARA);
	if (pServerSdo == NULL)  {
	    return(0u);
	}
	return(pServerSdo->restSize);
    } else
#endif /* CONFIG_SDO_SERVER */
    {
#ifdef CONFIG_SDO_CLIENT
	pClientSdo = searchForClientSdoNr(sdo & CO_NONUM_SDO CO_COMMA_LINE_PARA);
	if (pClientSdo == NULL)  {
	    return(0u);
	}
	return(pClientSdo->sdo.restSize);
#else  /* CONFIG_SDO_CLIENT */
    return(0u);
#endif /* CONFIG_SDO_CLIENT */
    }
}
#endif /* defined(CONFIG_SDO_SERVER) || defined(CONFIG_SDO_CLIENT) */

#ifdef CONFIG_SDO_SERVER
/****************************************************************************/
/**
*++ \brief getActualSdo - get the number of the current Server SDO
*-- \brief getActualSdo - ermittelt die Nummer der aktuellen Server-SDO
*
*++ This function returns the number of the current Server SDO.
*++ This function is useful to get the sdo number within sdoWrInd().
*-- Diese Funktion ermittelt die Nummer der aktuellen Server-SDO.
*-- Sie wird genutzt innerhalb von sdoWrInd, um die Nummer der aktuellen
*-- SDO zu ermitteln.
*
* \code
* sdoWrInd(UNSIGNED16 index, UNSIGNED8 subIndex)
* {
*     sdo = getActualSdo(index, subIndex);     // get number of active SDO
*     size = getSdoSize(sdo, SERVER);          // size of written data
* }
* \endcode
*
* \retval number
*++ number of Server SDO (1-128)
*-- Server-SDO Nummer (1-128)
* \retval 0
*++ failure
*-- Fehler
*
*/

UNSIGNED8 getActualSdo(
	UNSIGNED16 index,	/**< index of transfered object */
	UNSIGNED8  subIndex	/**< sub index of transfered object */
	CO_COMMA_LINE_PARA_DECL	/**< number of CAN line 0..CO_MAX_CAN_LINES-1 */
   )
{
UNSIGNED8   idx = (UNSIGNED8)GL_ARRAY(co_sdoServerCnt);
# ifdef CONFIG_MULT_LINES
SDO_T	    *pSdo = &GL_PVAR(co_sdoServer)[GL_ARRAY(co_sdoServerLineOffs)];
# else /* CONFIG_MULT_LINES */
SDO_T	    *pSdo = GL_PVAR(co_sdoServer);
# endif /* CONFIG_MULT_LINES */

    while (idx != 0u)  {
	idx--;
	if ((pSdo[idx].index == index)
	 && (pSdo[idx].subIndex == subIndex)) {
	   return(pSdo[idx].num);
	}
    }
    return(0u);
}
#endif /* CONFIG_SDO_SERVER */


#if defined(CONFIG_SDO_SERVER) || defined(CONFIG_SDO_CLIENT)
/****************************************************************************/
/**
*++ \brief abortSdoReq - request the remote service abort domain transfer
*-- \brief abortSdoReq - Anforderung eines Abort Domain Transfers
*
*-- Der Client
*-- oder der Server der Domain
*-- versuchen den Domaintransfer wegen eines Fehlers abzubrechen.
*-- Dieser Dienst ist unbestätigt.
*-- Der Fehlercode \em errReason kann folgende Werte annehmen:
*++ Client or server of a domain try to interrupt the transmission
*++ due do a error condition.
*++ This service is un-confirmed.
*++ The error code \em errReason can have the following values:
*
* \arg \c CO_OK
* \arg \c CO_E_NONEXIST_OBJECT
* \arg \c CO_E_NONEXIST_SUBINDEX
* \arg \c CO_E_NO_READ_PERM
* \arg \c CO_E_NO_WRITE_PERM
* \arg \c CO_E_MAP
* \arg \c CO_E_DATA_LENGTH
* \arg \c CO_E_TRANS_TYPE
* \arg \c CO_E_VALUE_TO_HIGH
* \arg \c CO_E_VALUE_TO_LOW
* \arg \c CO_E_WRONG_SIZE
* \arg \c CO_E_PARA_INCOMP
* \arg \c CO_E_HARDWARE_FAULT
* \arg \c CO_E_SRD_NO_RESSOURCE
* \arg \c CO_E_SDO_CMD_SPEC_INVALID
* \arg \c CO_E_MEM
* \arg \c CO_E_SDO_INVALID_BLKSIZE
* \arg \c CO_E_SDO_INVALID_BLKCRC
* \arg \c CO_E_SDO_TIMEOUT
* \arg \c CO_E_INVALID_TRANSMODE
* \arg \c CO_E_SDO_OTHER
* \arg \c CO_E_DEVICE_STATE
*
*++ see also user manual appendix 5 - sdo abort codes
*-- siehe auch User Manual Anhang 5 - Sdo Abort Codes
*
* \retval CO_OK
*-- Erfolg
*++ Success
* \retval CO_E_NOT_EXIST
*-- Domain mit dieser Codenummer existiert nicht
*++ domain with this number does not exist
* \retval CO_E_TRANS_TYPE
*++ bad transmission type requested
*-- Nicht compilierter Transmission Type angefordert
* \retval CO_E_STATE
*-- Knoten nicht im Zustand OPERATIONAL
*++ node not in state OPERATIONAL
* \internal
* input: handle, error reason
*/
RET_T abortSdoReq(
	UNSIGNED8	sdoNr, 		/**< Codenumber of SDO */
	USER_T		kindOfUse,	/**< CLIENT/SERVER */
	RET_T		abortCode	/**< return error code */
	CO_COMMA_LINE_PARA_DECL	/**< number of CAN line 0..CO_MAX_CAN_LINES-1 */
    )
{
RET_T		commonRet;
SDO_T		*pSdo;		/* pointer to current SDO */
#ifdef CONFIG_SDO_CLIENT
SDO_CLIENT_T	*pClSdo;	/* pointer to client SDO */
#endif /* CONFIG_SDO_CLIENT */

    if (kindOfUse == SERVER) {
#ifdef CONFIG_SDO_SERVER
	pSdo = searchForServerSdoNr(sdoNr CO_COMMA_LINE_PARA);
	if (pSdo == NULL)  {
	    /* SDO number doesn't exist */
	    return(CO_E_NOT_EXIST);
	}
#else /* CONFIG_SDO_SERVER */
	return(CO_E_TRANS_TYPE);
#endif /* CONFIG_SDO_SERVER */
    } else {

#ifdef CONFIG_SDO_CLIENT
	pClSdo = searchForClientSdoNr(sdoNr CO_COMMA_LINE_PARA);
	if (pClSdo == NULL)  {
	    /* SDO number doesn't exist */
	    return(CO_E_NOT_EXIST);
	}
	pSdo = &pClSdo->sdo;

	/* remove Timer */
	removeTimerEvent(&pClSdo->timer CO_COMMA_LINE_PARA);

#else /* CONFIG_SDO_CLIENT */
	return(CO_E_TRANS_TYPE);
#endif /* CONFIG_SDO_CLIENT */
    }

    commonRet = abortSdoTransf_Req(pSdo, abortCode CO_COMMA_LINE_PARA);
    return(commonRet);
}


/*******************************************************************
*
* initSdoVars - init all SDO variables
*
* \internal
*
* RETURNS
* \retval nthing
*
*/

void initSdoVars(
	CO_LINE_PARA_DECL
    )
{
# ifdef CONFIG_MULT_LINES
UNSIGNED8	l;
UNSIGNED16	offs;		/* max 256 Lines * 128 SDO */
# endif /* CONFIG_MULT_LINES */

    /* clear global variables (some compilers doesn't clear global variables */
# ifdef CONFIG_CLEAR_CO_GLOBAL_VARS
#  ifdef CONFIG_SDO_SERVER
    memset(&GL_PVAR(co_sdoServer)[0], (int)0, (size_t)(sizeof(SDO_T) * SDO_SERVER_CNT));

#   ifdef CONFIG_FAST_SORT
    memset(&GL_PVAR(co_sdoServerNrList)[0], (int)0,
	/* (size_t)(sizeof(UNSIGNED8) * CONFIG_SDO_SERVER)); */
	(size_t)(sizeof(UNSIGNED8) * SDO_SERVER_CNT));
    memset(&GL_PVAR(co_sdoServerCobIdxList)[0], (int)0,
	/* (size_t)(sizeof(UNSIGNED8) * CONFIG_SDO_SERVER)); */
	(size_t)(sizeof(UNSIGNED8) * SDO_SERVER_CNT));
#   endif /* CONFIG_FAST_SORT */
#  endif /* CONFIG_SDO_SERVER */

#  ifdef CONFIG_SDO_CLIENT
    memset(&GL_PVAR(co_sdoClient)[0], (int)0,
	(size_t)(sizeof(SDO_CLIENT_T) * CONFIG_SDO_CLIENT));
#   ifdef CONFIG_FAST_SORT
    memset(&GL_PVAR(co_sdoClientNrList)[0], (int)0,
	(size_t)(sizeof(UNSIGNED8) * CONFIG_SDO_CLIENT));
    memset(&GL_PVAR(co_sdoClientCobIdxList)[0], (int)0,
	(size_t)(sizeof(UNSIGNED8) * CONFIG_SDO_CLIENT));
#   endif /* CONFIG_FAST_SORT */
#  endif /* CONFIG_SDO_CLIENT */
# endif /* CONFIG_CLEAR_CO_GLOBAL_VARS */


# ifdef CONFIG_MULT_LINES
#  ifdef CONFIG_SDO_SERVER
    /* calculate sdo server line offsets */
    l = canLine;
    offs = 0;
    while (l > 0)  {
	l--;
	offs += GL_VAR(co_sdoServerLineCnts)[l];
    }
    GL_ARRAY(co_sdoServerLineOffs) = offs;
#  endif /* CONFIG_SDO_SERVER */

#  ifdef CONFIG_SDO_CLIENT
    /* calculate sdo Client line offsets */
    l = canLine;
    offs = 0;
    while (l > 0)  {
	l--;
	offs += GL_VAR(co_sdoClientLineCnts)[l];
    }
    GL_ARRAY(co_sdoClientLineOffs) = offs;
#  endif /* CONFIG_SDO_CLIENT */
# endif /* CONFIG_MULT_LINES */

#ifdef CONFIG_SDO_SERVER
    GL_ARRAY(co_sdoServerCnt) = 0;
#endif /* CONFIG_SDO_SERVER */

#ifdef CONFIG_SDO_CLIENT
    GL_ARRAY(co_sdoClientCnt) = 0;
#endif /* CONFIG_SDO_CLIENT */

}


# ifdef CONFIG_SDO_CLIENT
RET_T getClientSdoNumFromNodeId(UNSIGNED8 nodeNum, UNSIGNED8* pSdoNum CO_COMMA_LINE_PARA_DECL )
{
RET_T retVal = CO_E_PARA_INCOMP;
#  ifdef CONFIG_MULT_LINES
SDO_CLIENT_T	*pSdo = &GL_PVAR(co_sdoClient)[GL_ARRAY(co_sdoClientLineOffs)];
#  else /* CONFIG_MULT_LINES */
SDO_CLIENT_T	*pSdo = GL_PVAR(co_sdoClient);
#  endif /* CONFIG_MULT_LINES */
INTEGER8 idx = GL_ARRAY(co_sdoClientCnt) - 1;

    if ( (nodeNum < 1u) || (nodeNum > 127u) ) {
        return retVal;
    }

    retVal = CO_E_HARDWARE_FAULT;

    if ( pSdoNum == NULL ) {
        return retVal;
    }

    retVal = CO_E_UNKNOWN_NODE;

    while (idx >= 0)  {
	if ( (pSdo[idx].sdo.pTrCOB->cobId == (CO_COBID_CSDO + nodeNum))
          && (pSdo[idx].sdo.pRecCOB->cobId == (CO_COBID_SSDO + nodeNum)) ) {
            *pSdoNum = pSdo[idx].sdo.num;
	    return CO_OK;
	}
	idx--;
    }
    return retVal;
}
# endif /* CONFIG_SDO_CLIENT */


#endif /* defined(CONFIG_SDO_SERVER) || defined(CONFIG_SDO_CLIENT) */

/*______________________________________________________________________EOF_*/
