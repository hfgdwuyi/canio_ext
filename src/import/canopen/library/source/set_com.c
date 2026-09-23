/*
 *++ set_com - routines for setting of internal communications variables
 *-- set_com - Routinen für das Setzen interner Kommunikationsvariablen
 *
 * Copyright (c) 1997-2017 port GmbH Halle (Saale)
 *------------------------------------------------------------------
 */

/****************************************************************************/
/**
*  \file set_com.c
*++ Routines for setting of internal communications variables
*-- Routinen für das Setzen interner Kommunikationsvariablen
*  \author port GmbH Halle (Saale)
*
*++ This module contains functions for setting of internal communication
*++ variables of the CANopen library.
*++ Additionally it is possible to
*++ use it in applications for configuration of communication variables.
*-- Diese Modul beinhaltet Funktionen zum Setzen interner
*-- Kommunikationsvariablen.
*/

/* header of standard C - libraries */

#include <stdio.h>
#include <string.h>

/* header of project specific types */

#include <cal_conf.h>
#include <co_odidx.h>
#include <co_cobid.h>
#include <co_mcpy.h>
#include <co_setcp.h>
#include <co_usr.h>
#include <co_def.h>
#include "access.h"
#include "nmt.h"
#include "nmt_s.h"
#include "nmterr.h"
#include "pdo.h"
#ifdef CONFIG_HEARTBEAT_CONSUMER
# include "heartbt.h"
#endif /* CONFIG_HEARTBEAT_CONSUMER */
#include "sdo.h"
#if defined(CONFIG_SYNC_PRODUCER) || defined(CONFIG_SYNC_CONSUMER)
# include "sync.h"
#endif /* defined(CONFIG_SYNC_PRODUCER) || defined(CONFIG_SYNC_CONSUMER) */
#include "emerg.h"
#include "cmscodec.h"
#if defined(CONFIG_TIME_PRODUCER) || defined(CONFIG_TIME_CONSUMER)
# include "time_lib.h"
#endif /* defined(CONFIG_TIME_PRODUCER) || defined(CONFIG_TIME_CONSUMER) */
#include "drv.h"
#if defined(CONFIG_SRDO_CONSUMER) || defined(CONFIG_SRDO_PRODUCER)
# include "srdo.h"
#endif /* defined(CONFIG_SRDO_CONSUMER) || defined(CONFIG_SRDO_PRODUCER) */
#if defined(CONFIG_MASTER)
# include "nmt_m.h"
#endif /* defined(CONFIG_MASTER) */

#if defined(CONFIG_MPDO_DEST) || defined(CONFIG_MPDO_SRC)
# include "mpdo.h"
#endif /* defined(CONFIG_MPDO_DEST) || defined(CONFIG_MPDO_SRC) */

#ifdef CONFIG_FLYING_MASTER
# include "flyma.h"
#endif /* CONFIG_FLYING_MASTER */

#ifdef CONFIG_REDUNDANCY_SUPPORT
# include "reduncy.h"
#endif

#ifdef CONFIG_NON_VOLATILE_MEM
# include "co_stor.h"
#endif /* CONFIG_NON_VOLATILE_MEM */

#ifdef CONFIG_NMT_STARTUP_MANAGER
# include "nmtstart.h"
#endif /* CONFIG_NMT_STARTUP_MANAGER */

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
#if defined(CONFIG_SDO_CLIENT) || defined(CONFIG_SDO_COB_ID)
static RET_T setSdoCommPara(UNSIGNED16 index, UNSIGNED8 subIndex,
	UNSIGNED8 *pData, USER_T kind CO_COMMA_LINE_PARA_DECL);
#endif /* defined(CONFIG_SDO_CLIENT) || defined(CONFIG_SDO_COB_ID) */
#ifdef CONFIG_DYN_PDO_MAPPING
static RET_T setPdoMapping(UNSIGNED16 index, UNSIGNED8 subIndex,
	UNSIGNED32 mapEntry CO_COMMA_LINE_PARA_DECL);
#endif /* CONFIG_DYN_PDO_MAPPING */
static RET_T resetObjDirSubIndex(LIST_ELEMENT_T *pObj, UNSIGNED8 subIndex,
	UNSIGNED8 subIndexDesc CO_COMMA_LINE_PARA_DECL);

/* external variables
---------------------------------------------------------------------------*/

/* global variables
---------------------------------------------------------------------------*/

/* local defined variables
---------------------------------------------------------------------------*/
#ifdef CONFIG_RCS_IDENT
CO_LIB_INIT_VAR static char _rcsid[] = "$Id: set_com.c,v 2.86 2016/09/20 15:03:52 rli Exp $";
#endif /* CONFIG_RCS_IDENT */

/****************************************************************************/
/**
*++ \brief setCommPar - set the internal Communication Parameter
*-- \brief setCommPar - setzt die internen Kommunikationsparameter
*
*++ If a new communication parameter is set by users application
*++ this function have to be called e.g. after definition of a
*++ communication object a new parameter will be loaded from a
*++ non volatile memory.
*++ It is responsible for passing the communication parameter through to
*++ the internal variables.
*-- Wenn Kommunikationsparameter im Objektverzeichnis
*-- durch das Anwenderprogramm modifiziert wurden,
*-- ist diese Funktion aufzurufen,
*-- um interne Variablen zu aktualisieren bzw. Daten im CAN-Controller
*-- zu aktualisieren.
*-- Zum Beispiel nach der Definition eines Kommunikationsobjektes
*-- können neue Parameter aus einem nicht flüchtigen Speicher
*-- geladen werden.
*-- Die Funktion übergibt die Kommunikationsparameter an die internen
*-- Variablen.
*
*
* \code
*  UNSIGNED32 cobId;
*
*  definePdo(RECEIVE_PDO, 2, CO_FALSE);           // define RPDO
*  cobId = readEeprom(pdo2cobid);                 // read new cobid from EEPROM
*  putObj(0x1401, 1, &cobId, size, CO_TRUE);          // write back to Obj. Dic.
*  setCommPar(0x1401, 1);                          // set value active
* \endcode
*
* \retval CO_OK
*++ success
*-- Erfolg
* \retval CO_E_NOT_EXIST
*++ internal communication object doesn't exist
*-- Interne Kommunictionsvariable existiert nicht
* \retval CO_E_STATE
*++ node is in state OPERATIONAL
*-- Node ist im Zustand OPERATIONAL
* \retval CO_E_RANGE
*++ COB-ID is out of range (1..1760) for data services
*-- COB-ID ist nicht im Bereich von (1..1760) für Datendienste
* \retval CO_E_MAP
*++ dynamic mapping is not available
*-- Dynamisches Mapping ist nicht verfügbar
* \retval CO_E_NO_INITIATE
*++ service not available (not defined/compiled)
*-- Service nicht verfügbar (nicht definiert/Compiliert)
* \retval CO_E_NONEXIST_SUBINDEX
*++ subindex doesn't exist
*-- Subindex existiert nicht
* \retval CO_E_TRANS_TYPE
*++ bad transmission type
*-- Falscher Transmission Type
* \retval CO_E_DATA_LENGTH
*++ to many bytes mapped
*-- Zu viele Bytes gemapped
* \retval CO_E_PARA_INCOMP
*++ parameter incompatible
*-- Parameter inkompatibel
*
*
*/

RET_T setCommPar(
	UNSIGNED16 index,    /**< index of communication object */
	UNSIGNED8  subIndex  /**< subindex of communication object */
	CO_COMMA_LINE_PARA_DECL	/**< number of CAN line 0..CO_MAX_CAN_LINES-1 */
   )
{
RET_T        	ret = CO_OK; 	/* return value */
UNSIGNED32      size;           /* size of object entry */
UNSIGNED8	*pObj;		/* pointer to object */

#ifdef CO_CONFIG_SELFSTARTING_SLAVE
UNSIGNED32      value;
#endif /* CO_CONFIG_SELFSTARTING_SLAVE */

    /* get the address of new value */
    ret = getObjAddr(index, subIndex, &pObj, &size CO_COMMA_LINE_PARA);
    if (ret != CO_OK) {
	return(ret);
    }

    /* PDO Parameter */
#ifdef CONFIG_PDO_CONSUMER
    if ((index >= RPDO_PARA_BASE_INDEX) && (index <= RPDO_PARA_LAST_INDEX)) {
	ret = setPdoCommPara(index, subIndex, pObj, RECEIVE_PDO
		CO_COMMA_LINE_PARA);
    } else

    if ((index >= RPDO_MAP_BASE_INDEX) && (index <= RPDO_MAP_LAST_INDEX)) {
# ifdef CONFIG_DYN_PDO_MAPPING
	ret = setPdoMapping(index, subIndex, *((UNSIGNED32 *)pObj)
		CO_COMMA_LINE_PARA);
# else /* CONFIG_DYN_PDO_MAPPING */
	ret = CO_E_MAP;
# endif /* CONFIG_DYN_PDO_MAPPING */
    }
#endif /* CONFIG_PDO_CONSUMER */

#ifdef CONFIG_PDO_PRODUCER
    if ((index >= TPDO_PARA_BASE_INDEX) && (index <= TPDO_PARA_LAST_INDEX)) {
	ret = setPdoCommPara(index, subIndex, pObj, TRANSMIT_PDO
		CO_COMMA_LINE_PARA);
    } else

    if ((index >= TPDO_MAP_BASE_INDEX) && (index <= TPDO_MAP_LAST_INDEX)) {
# ifdef CONFIG_DYN_PDO_MAPPING
	ret = setPdoMapping(index, subIndex, *((UNSIGNED32 *)pObj)
		CO_COMMA_LINE_PARA);
# else /* CONFIG_DYN_PDO_MAPPING */
	ret = CO_E_MAP;
# endif /* CONFIG_DYN_PDO_MAPPING */
    }
#endif /* CONFIG_PDO_PRODUCER */

#if defined(CONFIG_PDO_PRODUCER) && defined(CONFIG_MPDO_SRC)
    else if ((index >= MPDO_SCANNER_LIST_INDEX) &&
	     (index <= MPDO_SCANNER_LIST_LAST))
    {
	if (testMPdoScannerEntry(index,	subIndex CO_COMMA_LINE_PARA) < 0)  {
	    return(CO_E_MAP);
	}
    }
#endif /* defined(CONFIG_PDO_PRODUCER) && defined(CONFIG_MPDO_SRC) */

#if defined(CONFIG_PDO_CONSUMER) && defined(CONFIG_MPDO_SRC)
    else if ((index >= MPDO_DISP_LIST_INDEX) &&
	     (index <= MPDO_DISP_LIST_LAST))
    {
	if (testMPdoDispatchEntry(index, subIndex CO_COMMA_LINE_PARA) < 0)  {
	    return(CO_E_MAP);
	}
    }
#endif /* defined(CONFIG_PDO_CONSUMER) && defined(CONFIG_MPDO_SRC) */

#if defined(CONFIG_SDO_CLIENT) || defined(CONFIG_SDO_COB_ID)
    else if ((index >= SSDO_PARA_BASE_INDEX)
	  && (index <= SSDO_PARA_LAST_INDEX)) {

# ifdef CONFIG_SDO_COB_ID
# else /* CONFIG_SDO_COB_ID */
	/* ignore first server sdo if no entry at od */
	if (index != SSDO_PARA_BASE_INDEX)
# endif /* CONFIG_SDO_COB_ID */
	{
	    ret = setSdoCommPara(index, subIndex, pObj, SERVER
			CO_COMMA_LINE_PARA);
	}
    }
    else if ((index >= CSDO_PARA_BASE_INDEX)
	  && (index <= CSDO_PARA_LAST_INDEX)) {
	ret = setSdoCommPara(index, subIndex, pObj, CLIENT CO_COMMA_LINE_PARA);
    }
#endif /* defined(CONFIG_SDO_CLIENT) || defined(CONFIG_SDO_COB_ID) */

#if defined(CONFIG_SRDO_CONSUMER) || defined(CONFIG_SRDO_PRODUCER)
    else if ((index >= SRDO_GFC) && (index <= SRDO_CONFIG_CHECKSUM)) {
	ret = setSrdoData(index, subIndex, pObj CO_COMMA_LINE_PARA);
    }
#endif /* defined(CONFIG_SRDO_CONSUMER) || defined(CONFIG_SRDO_PRODUCER) */

    else {

	switch (index) {

#if defined(CONFIG_SYNC_PRODUCER) || defined(CONFIG_SYNC_CONSUMER)
	    case SYNC_COB_ID_INDEX:
		ret = setSyncCobId(*((UNSIGNED32 *)pObj) CO_COMMA_LINE_PARA);
		break;

# ifdef CONFIG_SYNC_COUNTER
	    case SYNC_COUNTER_INDEX:
		ret = setSyncCounter(*pObj CO_COMMA_LINE_PARA);
		break;
# endif /* CONFIG_SYNC_COUNTER */
#endif /* defined(CONFIG_SYNC_PRODUCER) || defined(CONFIG_SYNC_CONSUMER) */

#if defined(CONFIG_SYNC_PRODUCER)
	    case COMM_CYCLE_INDEX:
		(void) setSyncTimePara(*((UNSIGNED32 *)pObj) CO_COMMA_LINE_PARA);
		break;
#endif /* CONFIG_SYNC_PRODUCER */

#if defined(CONFIG_TIME_CONSUMER) || defined(CONFIG_TIME_PRODUCER)
	    case TIME_COB_ID_INDEX:
		ret = setTimeCobId(*(UNSIGNED32 *)pObj CO_COMMA_LINE_PARA);
		break;
#endif /* defined(CONFIG_TIME_CONSUMER) || defined(CONFIG_TIME_PRODUCER) */

#if defined(CONFIG_EMCY_PRODUCER)
	    case EMCY_COB_ID_INDEX:
		ret = setEmcyCobId((UNSIGNED32 *)pObj CO_COMMA_LINE_PARA);
		break;

	    case EMCY_INHIBIT_INDEX:
		ret = setEmcyInhibit((UNSIGNED16 *)pObj CO_COMMA_LINE_PARA);
		break;

	    case ERROR_FIELD_INDEX:
		/* only 0 is allowed to write */
		if (*((UNSIGNED8 *)pObj) != 0)  {
		    return(CO_E_TRANS_TYPE);
		}
		ret = eraseErr(0 CO_COMMA_LINE_PARA);
		break;
#endif /* defined(CONFIG_EMCY_PRODUCER) */

#ifdef CONFIG_NODE_GUARDING
# ifdef CONFIG_SLAVE
	    case GUARD_TIME_INDEX:
                /*printf("GUARD_TIME_INDEX\n");*/
		/* we need additional the life time factor */
		{
		UNSIGNED8	factor;

		    ret = getObjEntry(LIFE_TIME_FAC_INDEX, 0, &factor, &size,
			    CO_TRUE CO_COMMA_LINE_PARA);
		    if (ret == CO_OK) {
			ret = setLifeTimePara(*(UNSIGNED16 *)pObj, factor
			    CO_COMMA_LINE_PARA);
		    }
		}
		break;

	    case LIFE_TIME_FAC_INDEX:
                /*printf("LIFE_TIME_FAC_INDEX\n");*/
		/* we need additional the guard time*/
		{
		UNSIGNED16	guardTime;

		    ret = getObjEntry(GUARD_TIME_INDEX, 0,
			    (UNSIGNED8 *)&guardTime, &size,
			    CO_TRUE CO_COMMA_LINE_PARA);
		    if (ret == CO_OK) {
			ret = setLifeTimePara(guardTime, *(UNSIGNED8 *)pObj
			    CO_COMMA_LINE_PARA);
		    }
		}
		break;
# endif /* CONFIG_SLAVE */
#endif /* CONFIG_NODE_GUARDING */

#ifdef CONFIG_HEARTBEAT_PRODUCER
	    case HEARTBEAT_PROD_INDEX:
		ret = setHeartBeatProducerTime(*((UNSIGNED16 *)pObj)
			CO_COMMA_LINE_PARA);
		break;
#endif /* CONFIG_HEARTBEAT_PRODUCER */

#ifdef CONFIG_HEARTBEAT_CONSUMER
	    case HEARTBEAT_CON_INDEX:
		ret = setHeartBeatConsumerTime(*((UNSIGNED32 *)pObj), subIndex,
			CO_TRUE CO_COMMA_LINE_PARA);
# ifdef CONFIG_REDUNDANCY_SUPPORT
		/* check, if all HB consumers are ok */
		redcyCheckNodeAvailable(CAN_DEFAULT_LINE CO_COMMA_LINE_PARA);
# endif /* CONFIG_REDUNDANCY_SUPPORT */
		break;
#endif /* CONFIG_HEARTBEAT_CONSUMER */

#if defined(CONFIG_EMCY_CONSUMER)
	    case EMCY_CONSUMER_INDEX:
		ret = setEmcyConsumerCobId(subIndex, *((UNSIGNED32 *)pObj) CO_COMMA_LINE_PARA);
		break;
#endif /* defined(CONFIG_EMCY_CONSUMER) */

#ifdef CONFIG_FLYING_MASTER
	    case FLYMA_TIMEPAR_INDEX:
		ret = setFlymaTimePara(subIndex, *(UNSIGNED16 *)pObj
			CO_COMMA_LINE_PARA);
		break;

	    case FLYMA_DEVPAR_INDEX:
		ret = setFlymaDevPara(subIndex, *(UNSIGNED16 *)pObj
			CO_COMMA_LINE_PARA);
		break;
#endif /* CONFIG_FLYING_MASTER */

#ifdef CONFIG_REDUNDANCY_SUPPORT
	    case REDCY_INDEX:
		ret = setRedcyPara(subIndex, *pObj CO_COMMA_GLOBVARS_PARA);
		break;
#endif /* CONFIG_REDUNDANCY_SUPPORT */

#if defined(CONFIG_MASTER) || defined(CONFIG_SLAVE_PLUS) || defined(CO_CONFIG_SELFSTARTING_SLAVE)
	    case NMT_MASTER_INDEX:
#ifdef CO_CONFIG_SELFSTARTING_SLAVE
                value = *((UNSIGNED32*) pObj);
#if !defined(CONFIG_MASTER) || !defined(CONFIG_SLAVE_PLUS)
                /* bit other than 3 */
                if ((value | 0x8) != 0x8)
                {
                    ret = CO_E_TRANS_TYPE;
                    break;
                }
                value &= 0x8;
                pObj = (UNSIGNED8*) &value;
#endif /* !defined(CONFIG_MASTER) || !defined(CONFIG_SLAVE_PLUS) */
#endif /* CO_CONFIG_SELFSTARTING_SLAVE */

	        /* for flying master */
	        CO_NUM_MEMCPY(&GL_ARRAY(co_nmtStartUp), pObj, 4, CO_TRUE);

# if defined(CONFIG_MASTER) && defined(CONFIG_NMT_STARTUP_MANAGER)
		ret = setNmtStartupPara(index, subIndex,
	            *(UNSIGNED32*)pObj CO_COMMA_LINE_PARA);
# endif
		break;
#endif /* defined(CONFIG_MASTER) || defined(CONFIG_SLAVE_PLUS) */

#if defined(CONFIG_MASTER) && defined(CONFIG_NMT_STARTUP_MANAGER)
	    case NMT_BOOT_TIME_INDEX:
	    case NMT_SLAVE_ASSIGNMENT_INDEX:
		ret = setNmtStartupPara(index, subIndex,
	            *(UNSIGNED32*)pObj CO_COMMA_LINE_PARA);
		break;
#endif /* defined(CONFIG_MASTER) && defined(CONFIG_NMT_STARTUP_MANAGER) */

#ifdef CONFIG_NO_ERROR_BEHAVIOR
#else /* CONFIG_NO_ERROR_BEHAVIOR */
	    case ERROR_BEHAVIOR_INDEX:
		setupCommErrorBehavior(CO_LINE_PARA);
		break;
#endif /* CONFIG_NO_ERROR_BEHAVIOR */
		default:
			break;
	}
    }

    return ret;
}


/****************************************************************************/
/*
*++ \brief checkCommParAccess - get internal Communication Parameter Access
*-- \brief checkCommParAccess - checkt Kommunikationsparameter Zugriff
*
* internal
*
*++ Some communication parameters are not allowed to read
*++ by SDO for some reasons.
*++ This functions returns an error code
*++ if the object isn't allowed to be read at this moment.
*-- Einige Kommunikationsparameter dürfen per SDO Zugriff
*-- in bestimmten Situationen nicht gelesen werden.
*-- In diesem Fall kehrt die Funktion
*-- mit einem entsprechenden Error-Code zurück.
*
* \retval CO_OK
*++ success
*-- Erfolg
* \retval RET_T
*++ SDO Abort code
*-- SDO Abort code
*
*/

RET_T checkCommParAccess(
	UNSIGNED16 index,	/* index of communication object */
	UNSIGNED8  subIndex	/* subindex of communication object */
	CO_COMMA_LINE_PARA_DECL
    )
{
RET_T        	ret = CO_OK; 	/* return value */

#if (!defined(CONFIG_PDO_CONSUMER)) || (!defined(CONFIG_PDO_PRODUCER))
/* in case they are not usead to avoid 'unused' warning */
    CO_INTERNAL_NOT_USED(index);
    CO_INTERNAL_NOT_USED(subIndex);
#endif

    /* PDO Parameter */
#ifdef CONFIG_PDO_CONSUMER
    if ((index >= RPDO_PARA_BASE_INDEX) && (index <= RPDO_PARA_LAST_INDEX)) {
	/* compatibility entry */
	if (subIndex == 4u)  {
	    ret = CO_E_NONEXIST_SUBINDEX;
	}
    } else
#endif /* CONFIG_PDO_CONSUMER */

#ifdef CONFIG_PDO_PRODUCER
    if ((index >= TPDO_PARA_BASE_INDEX) && (index <= TPDO_PARA_LAST_INDEX)) {
	/* compatibility entry */
	if (subIndex == 4u)  {
	    ret = CO_E_NONEXIST_SUBINDEX;
	}
    } else
#endif /* CONFIG_PDO_PRODUCER */

    {

#if defined(CONFIG_EMCY_PRODUCER)
	switch (index) {
	    case ERROR_FIELD_INDEX:
		ret = checkErrorFieldAccess(subIndex CO_COMMA_LINE_PARA);
		break;

	    default:
		break;
	}
#endif /* defined(CONFIG_EMCY_PRODUCER) */
    }

    return(ret);
}

#if defined(CONFIG_SDO_CLIENT) || defined(CONFIG_SDO_COB_ID)
/*******************************************************************/
/*
* setSdoCommPar - set sdo communication parameter
*
* \internal
*
* This function sets communication parameter for sdos
*
* \retval CO_OK
*++ success
*-- Erfolg
* \retval CO_E_NOT_EXIST
* internal communication object doesn't exist
* \retval CO_E_RANGE
* COB-ID is out of the range
* \retval CO_E_TRANS_TYPE
* bad transtype
*
*/
static RET_T setSdoCommPara(
	UNSIGNED16	index,		/* index */
	UNSIGNED8	subIndex,	/* subindex */
	UNSIGNED8	*pData,		/* pointer to object data */
	USER_T		kind		/* kind of SDO SERVER/CLIENT */
	CO_COMMA_LINE_PARA_DECL	/* number of CAN line 0..CO_MAX_CAN_LINES-1 */
    )
{
UNSIGNED8	nr;			/* pdo number */
RET_T		ret = CO_E_NOT_EXIST;	/* return value */

    /* get sdo structure */
    nr = (UNSIGNED8)(index & 0x7f) + 1;

    switch (subIndex) {
	case 1:				/* cob-id */
	    if (kind == SERVER)  {
		ret = setSdoCobId(nr, *((UNSIGNED32 *)pData), kind,
			CO_COB_SDO_RX CO_COMMA_LINE_PARA);
	    } else {
		ret = setSdoCobId(nr, *((UNSIGNED32 *)pData), kind,
			CO_COB_SDO_TX CO_COMMA_LINE_PARA);
	    }
	    break;
	case 2:				/* cob-id */
	    if (kind == SERVER)  {
		ret = setSdoCobId(nr, *((UNSIGNED32 *)pData), kind,
			CO_COB_SDO_TX CO_COMMA_LINE_PARA);
	    } else {
		ret = setSdoCobId(nr, *((UNSIGNED32 *)pData), kind,
			CO_COB_SDO_RX CO_COMMA_LINE_PARA);
	    }
	    break;
	case 3:				/* node-id */
	    ret = CO_OK;		/* return value ok */
	    break;
        default:
            break;
    }

    return(ret);
}
#endif /* defined(CONFIG_SDO_CLIENT) || defined(CONFIG_SDO_SERVER) */


#if defined(CONFIG_PDO_CONSUMER) || defined(CONFIG_PDO_PRODUCER)
# ifdef CONFIG_DYN_PDO_MAPPING
/*******************************************************************
*
* setPdoMapping - sets the new Mapping
*
* \internal
*
* \retval CO_OK
*++ success
*-- Erfolg
* \retval CO_E_NOT_EXIST
* internal communication object doesn't exist
* \retval CO_E_MAP
* mapping not allowed, variable has no read or write access
* \retval CO_E_DEVICE_STATE
* mapping not changable in the current state (mapping not disabled)
*
* INTERNAL
* the consistency of variables is ensured by the function setCommPar,
* which calls setPdoMapping
*/

static RET_T setPdoMapping(
	UNSIGNED16 index,    /* index of mapping entry */
	UNSIGNED8  subIndex, /* subindex of mapping entry */
	UNSIGNED32 mapEntry  /* new mapping entry */
	CO_COMMA_LINE_PARA_DECL	/* number of CAN line 0..CO_MAX_CAN_LINES-1 */
   )
{
UNSIGNED16  nr;		    /* pdo number */
PDO_T	    *pPdo;	    /* pointer to pdo struct */
UNSIGNED8   kind;	    /* kind of PDO RECEIVE_PDO/TRANSMIT_PDO */
#  if defined(CONFIG_BIG_ENDIAN) || defined(CONFIG_16BIT_CPU)
UNSIGNED32  tmpU32;	    /* temporary variable */
#  endif /* defined(CONFIG_BIG_ENDIAN) || defined(CONFIG_16BIT_CPU) */

    /* get pdo type */
    if (index > RPDO_MAP_LAST_INDEX)  {
	kind = TRANSMIT_PDO;
    } else {
	kind = RECEIVE_PDO;
    }

    /* check, if pdo is intialized */
    nr = (index & 0x1ff) + 1;
    pPdo = pdoExist(nr, kind CO_COMMA_LINE_PARA);
    if (pPdo == NULL)  {
	 return(CO_E_NOT_EXIST);
    }

    /* DS301: it's not allowed to change the mapping if the pdo exist */
    if ((pPdo->flags & PDOFLAG_DISABLED) == 0)  {
	return(CO_E_MAP);
    }

    /* subindex 0 contains number of mapped objects */
    if (subIndex == 0) {
#  if defined(CONFIG_BIG_ENDIAN) || defined(CONFIG_16BIT_CPU)
	/* subindex 0 is an 8 bit value - but it comes as 32 bit value */
	tmpU32 = mapEntry;
	mapEntry = (*((UNSIGNED8 *)&tmpU32)) & 0xff;
#  endif /* defined(CONFIG_BIG_ENDIAN) || defined(CONFIG_16BIT_CPU) */

	mapEntry &= 0xFF;
	if (mapEntry != 0)  {
	    /* DS301: it's not allowed to modify the mapping
			if the mapping is enabled*/
	    if ((pPdo->flags & PDOFLAG_MAP_DISABLED) == 0)  {
		/* return(CO_E_MAP); */
		return(CO_E_DEVICE_STATE);
	    }
	}

	/* set new mapping count - check the complete mapping table
	    (incl. length for all mapping entries) */
	if (checkMappingTable(pPdo, index, kind CO_COMMA_LINE_PARA) != CO_OK)  {
	    return(CO_E_DATA_LENGTH);
	}

	return(CO_OK);
    }

    /* DS301: it's not allowed to modify the mapping if the mapping is enabled*/
    if ((pPdo->flags & PDOFLAG_MAP_DISABLED) == 0)  {
	return(CO_E_MAP);
    }

    /* check if the mapping entry is valid */
    if (checkMappingEntry(mapEntry, kind CO_COMMA_LINE_PARA) != CO_OK)  {
	return(CO_E_MAP);
    }

    return(CO_OK);
}
# endif /* CONFIG_DYN_PDO_MAPPING */
#endif /* defined(CONFIG_PDO_CONSUMER) || defined(CONFIG_PDO_PRODUCER) */

/*******************************************************************
*
*++ resetObjDir - sets the variables of the object dictionary to their defaults
*-- resetObjDir - setzt die Variables des Obj. Ver. auf ihre Standardwerte
*
* \internal
*
*++ This function sets writeable variables of the object dictionary
*++ to their defaults.
*-- Diese Funktion erlaubt das Rücksetzen von schreibbaren Objekten
*-- im Objektverzeichnis auf ihre Standardwerte.
*
*++ Range may be:
*-- Der angegebene Bereich \fIrange\fP kann sein:
*
* .TP
* RESET_ALL
*++ reset the whole object dictionary
*-- Rücksetzen des gesamten Objektverzeichnisses
* .TP
* RESET_COM
*++ reset only the communication part of the object dictionary
*-- Nur der kommunikationsspezifische Teil des Objektverzeichnisses
*-- wird rückgesetzt (Kommunikationsprofil)
* .TP
* RESET_NO_COM
*++ reset the whole object dictionary besides the communication part
*-- Das gesamte Objektverzeichnis ausschließlich des
*-- kommunikationsspezifische Teiles wird rückgesetzt
*
*++ 1) The bit_rate is a manufacturer specific extension
*++ provided by \fIport\fP GmbH
*++ for changing the CAN bit rate via SDO.
*-- 1) \fIbit_rate\fP ist eine hersteller spezifische Erweiterung
*-- von \fIport\fP GmbH und dient dem ändern der Bustaktfrequenz
*-- über einen SDO Transfer.
*
* \retval CO_OK
*++ success
*-- Erfolg
* \retval CO_E_RANGE
*++ parameter out of valid range
*-- Parameter \fIrange\fP nicht gültig
*
*/

RET_T resetObjDir(
	UNSIGNED8 range		/* range of the object dictionary */
	CO_COMMA_LINE_PARA_DECL	/* number of CAN line 0..CO_MAX_CAN_LINES-1 */
    )
{
UNSIGNED16	i;		/* variable for access to Object Dictionary*/
UNSIGNED8	subIndex;	/* sub index */
UNSIGNED16	min;		/* lowest valid index */
UNSIGNED16	max;		/* higest valid index */
LIST_ELEMENT_T	*pObj;		/* pointer to obj */
UNSIGNED8	subIndexDesc;	/* subindex for object description */
UNSIGNED16	index;		/* temp for index */
UNSIGNED8	numOfElem;	/* temp for member numOfElem */
UNSIGNED16	attribute;	/* temp for member attribute */

#ifdef CONFIG_MULT_LINES
/* For single line objDir is the unique od,
   for multiple lines objDir is local and get a pointer to choosen
   line od */
LIST_ELEMENT_T *pObjDir = (LIST_ELEMENT_T *)GL_ARRAY(pObjDirMan);
#endif /* CONFIG_MULT_LINES */

    /* setting of range */
    switch(range) {
	case MEM_SEG_ALL_PARAMETERS:
	    min = START_OBJ_DIC;
	    max = END_OBJ_DIC;
#ifdef CO_CONFIG_RESET_OBJ_DIR_IND
            if ( CO_OK != coResetObjDirInd(CO_RESET_OBJ_DIR_IND_APPL CO_COMMA_LINE_PARA)) {
                /* reset only the communication part */
                max = END_COM_PROF;
            }
#endif /*CO_CONFIG_RESET_OBJ_DIR_IND*/
	    break;
	case MEM_SEG_COM_PARAMETERS:
	    min = START_COM_PROF;
	    max = END_COM_PROF;
	    break;
	case MEM_SEG_APPL_PARAMETERS:
	    min = START_MANU_PROF;
	    max = END_OBJ_DIC;
#ifdef CO_CONFIG_RESET_OBJ_DIR_IND
            if ( CO_OK != coResetObjDirInd(CO_RESET_OBJ_DIR_IND_APPL CO_COMMA_LINE_PARA)) {
                /* skip the whole process */
                return (CO_OK);
            }
#endif /*CO_CONFIG_RESET_OBJ_DIR_IND*/
	    break;
	default:
	    return(CO_E_RANGE);
    }

    /* allocate security mechanism for object dictionary consistency */
    CO_COM_PART_ALLOC(CO_LINE_PARA);
    CO_APPL_PART_ALLOC(CO_LINE_PARA);

    /* pointer to first object */
#ifdef CONFIG_MULT_LINES
    pObj = (LIST_ELEMENT_T *)pObjDir;
#else /* CONFIG_MULT_LINES */
    pObj = (LIST_ELEMENT_T *)GL_VAR(pObjDir);
#endif /* CONFIG_MULT_LINES */

    /* for all entries at object dictionary */
    i = 0u;
#ifdef CONFIG_MULT_LINES
    while (i < GL_ARRAY(pMaxObjDicElements))  {
#else /* CONFIG_MULT_LINES */
    while (i < *GL_ARRAY(pMaxObjDicElements))  {
#endif /* CONFIG_MULT_LINES */

	index = CO_READ_OD16(pObj->index);

	/* if this index in the range */
	if ((index >= min) && (index <= max)) {
	    /* yes, it is */

	    attribute = CO_READ_OD_DESC_ATTR(pObj, 0);
	    /* ignore no numerical values */
	    if ((attribute & CO_NUM_VAL) != 0u) {

		/* test for short arrays */
		if ((attribute & CO_SHORT_ARRAY_DESC) != 0u) {
		    subIndexDesc = 1u;
		}  else  {
		    subIndexDesc = 0u;
		}

		numOfElem = CO_READ_OD8(pObj->numOfElem);
		for (subIndex = 0u; subIndex < numOfElem; subIndex++) {

		    CO_WATCH_DOG

		    /* set subindex */
		    (void)resetObjDirSubIndex(pObj, subIndex ,subIndexDesc
			CO_COMMA_LINE_PARA);
		}
	    }
	}

	pObj++;		/* next entry */
	i++;		/* incr counter */
    }

    /* release security mechanism for object dictionary consistency*/
    CO_COM_PART_RELEASE(CO_LINE_PARA);
    CO_APPL_PART_RELEASE(CO_LINE_PARA);

    return(CO_OK);
}


/*******************************************************************
*
*++ resetObjDirSubIndex - sets one entry to their defaults
*-- resetObjDirSubIndex - setzt eine Variables auf ihre Standardwerte
*
* \internal
*
*++ This function sets one variables of the object dictionary
*++ to their defaults.
*-- Diese Funktion erlaubt das Rücksetzen von einem Subindex
*-- im Objektverzeichnis auf ihre Standardwerte.
*
* RETURNS
* .TP
* \&OK
*++ success
*-- Erfolg
* .TP
* \&E_RANGE
*++ parameter out of valid range
*-- Parameter \fIrange\fP nicht gültig
*
*/
static RET_T resetObjDirSubIndex(
	LIST_ELEMENT_T	*pObj,		/* pointer to actual od entry */
	UNSIGNED8	subIndex,	/* subindex */
	UNSIGNED8	subIndexDesc	/* subIndex description */
	CO_COMMA_LINE_PARA_DECL	/* number of CAN line 0..CO_MAX_CAN_LINES-1 */
    )
{
UNSIGNED32	defaultVal = 0u;	/* default value */
UNSIGNED32	size;			/* byte size */
UNSIGNED8	*pDefaultVal;
UNSIGNED8	*pTargetAddr;		/* target address */
UNSIGNED16	odIndex;		/* index at od */
#if defined(CONFIG_SRDO_PRODUCER) || defined(CONFIG_SRDO_CONSUMER)
UNSIGNED8	defaultVal_u8;
#endif /* defined(CONFIG_SRDO_PRODUCER) || defined(CONFIG_SRDO_CONSUMER) */
#ifdef CONFIG_16BIT_CPU
#  ifdef CONFIG_EMULATE_U64
UNSIGNED8 defValueMem[CO_MAX_NUMDATA_SIZE];  /* temorary buffer for numerical value */
#  endif /* CONFIG_EMULATE_U64 */
#endif /* CONFIG_16BIT_CPU */

    if (subIndexDesc == 0u)  {
	subIndexDesc = subIndex;
    } else {
	if (subIndex == 0u)  {
	    subIndexDesc = 0u;
	}
    }

    /* only non-constant and numerical variables are allowed */
    if ((CO_READ_OD_DESC_ATTR(pObj, subIndexDesc)
		& (CO_NUM_VAL | CO_CONST_PERM)) != CO_NUM_VAL) {
	return(CO_E_RANGE);
    }

    /* get object size */
    size = getObjSize(pObj, subIndexDesc);
    pDefaultVal = getObjDefaultVal(pObj, subIndexDesc);
    if (pDefaultVal != NULL)  {
#ifdef CONFIG_16BIT_CPU
#  ifdef CONFIG_EMULATE_U64
        if ((size == 8) && ((CO_READ_OD_DESC_ATTR(pObj, subIndexDesc) & CO_NUM_VAL) == CO_NUM_VAL))
        {
            pack_memcpy(&defValueMem[0], pDefaultVal, size, CO_TRUE);
	    pDefaultVal = (UNSIGNED8 *)&defValueMem;
        }
#  endif /* CONFIG_EMULATE_U64 */
#endif /* CONFIG_16BIT_CPU */


#ifdef CONFIG_16BIT_CPU
	if (((size + 1u) >> 1) <= sizeof(defaultVal)) {
#else /* CONFIG_16BIT_CPU */
	if (size <= sizeof(defaultVal)) {
#endif /* CONFIG_16BIT_CPU */

#ifdef  CO_CODE_COPY
		/* pDefaultVal will be changed to a local buffer! */
	    CO_CODE_COPY(pDefaultVal, size);
#endif
	    CO_NUM_MEMCPY(&defaultVal, pDefaultVal, size, CO_TRUE);
	}
    }

    /* get address of subindex */
    pTargetAddr = (UNSIGNED8 *)getSubIndexAddr(pObj, subIndex);

    /* set cobs for predefined communication set ----------------------------*/
    odIndex = CO_READ_OD16(pObj->index);

    if ((odIndex >= START_COM_PROF)  && (odIndex <= END_COM_PROF))
    {
	if (odIndex < SYNC_COB_ID_INDEX)  {
	    /* use standard values from od */
	}

#if (defined(CONFIG_SYNC_CONSUMER) || defined(CONFIG_SYNC_PRODUCER))
	/* set SYNC default COB-ID */
	else if (odIndex == SYNC_COB_ID_INDEX)  {
	    pDefaultVal = (UNSIGNED8 *)&defaultVal;
	    defaultVal &= SYNC_PRODUCER_BIT;
	    defaultVal |= CO_COBID_SYNC;
	}
#endif /* (defined(CONFIG_SYNC_CONSUMER) || defined(CONFIG_SYNC_PRODUCER)) */

#ifdef CONFIG_NON_VOLATILE_MEM
	else if (odIndex == STORE_PARA_INDEX)  {
	    if (subIndex != 0u)  {
		/* change to the default value
		 * - Device don't save parameters autinomously
		 * - Device save parameters on command
		 */
		defaultVal = STORE_PARA_ON_COMMAND;
		pDefaultVal = (UNSIGNED8 *)&defaultVal;
	    }
	}
	else if (odIndex == RESTORE_DEF_PARA_INDEX)  {
	    if (subIndex != 0u)  {
		/* change to the default value
		 * - Device restores parameters
		 */
		defaultVal = RESTORE_PARA_ON_COMMAND;
		pDefaultVal = (UNSIGNED8 *)&defaultVal;
	    }
	}
#endif /* CONFIG_NON_VOLATILE_MEM */

	else if (odIndex < TIME_COB_ID_INDEX)  {
	    /* use standard values from od */
	}

#if (defined(CONFIG_TIME_CONSUMER) || defined(CONFIG_TIME_PRODUCER))
	/* set TIME stamp default COB-ID */
	else if (odIndex == TIME_COB_ID_INDEX)  {
# ifdef CONFIG_TIME_PRODUCER
	    defaultVal = CO_COBID_TIME | TIME_PRODUCER_BIT;
# endif /* CONFIG_TIME_PRODUCER */
# ifdef CONFIG_TIME_CONSUMER
	    defaultVal = CO_COBID_TIME | TIME_CONSUMER_BIT;
# endif /* CONFIG_TIME_CONSUMER */
	    pDefaultVal = (UNSIGNED8 *)&defaultVal;
	}
#endif /* (defined(CONFIG_TIME_CONSUMER) || defined(CONFIG_TIME_PRODUCER)) */

	else if (odIndex < EMCY_COB_ID_INDEX) {
	    /* use standard values from od */
	}

#if defined(CONFIG_EMCY_PRODUCER)
	else if (CO_READ_OD16(pObj->index) == EMCY_COB_ID_INDEX) {
	    /* set EMCY default COB-ID */
	    defaultVal = CO_COBID_EMCY + (UNSIGNED32)GL_ARRAY(coNodeId);
	    pDefaultVal = (UNSIGNED8 *)&defaultVal;
	}
#endif /* defined(CONFIG_EMCY_PRODUCER) */

	else if (odIndex < SSDO_PARA_BASE_INDEX) {
	    /* use standard values from od */
	}

	/* set SDO default COB-IDs */
	else if (odIndex == SSDO_PARA_BASE_INDEX) {
	    if (subIndex == 1u) {
		defaultVal = CO_COBID_CSDO + (UNSIGNED32)GL_ARRAY(coNodeId);
		pDefaultVal = (UNSIGNED8 *)&defaultVal;
	    }
	    if (subIndex == 2u) {
		defaultVal = CO_COBID_SSDO + (UNSIGNED32)GL_ARRAY(coNodeId);
		pDefaultVal = (UNSIGNED8 *)&defaultVal;
	    }
	}

	else if (odIndex <= CSDO_PARA_LAST_INDEX) {
	    /* disable all SDO besides SSDO 1 */
	    if ((subIndex == 1u) || (subIndex == 2u)) {
		defaultVal |= SDO_NO_VALID_BIT;
		pDefaultVal = (UNSIGNED8 *)&defaultVal;
	    }
	}

	else if (odIndex < SRDO_PARA_BASE_INDEX) {
	    /* use standard values from od */
	}

#if defined(CONFIG_SRDO_PRODUCER) || defined(CONFIG_SRDO_CONSUMER)
	else if (odIndex <= SRDO_PARA_LAST_INDEX) {
	    if (subIndex == 1u)  {
		/* only first srdo can be initialized  */
		if (pObj->index != SRDO_PARA_BASE_INDEX)  {
		    defaultVal_u8 = SRDO_NOT_VALID;
		    pDefaultVal = &defaultVal_u8;
		}
	    }

	    if ((subIndex == 5u) || (subIndex == 6u))  {

		/* only first srdo has predefined cob */
		if (pObj->index == SRDO_PARA_BASE_INDEX)  {
		    if (subIndex == 5u)  {
			defaultVal = 0xff + 2 * (UNSIGNED32)GL_ARRAY(coNodeId);
		    } else {
			defaultVal = 0x100 + 2 * (UNSIGNED32)GL_ARRAY(coNodeId);
		    }
		    pDefaultVal = (UNSIGNED8 *)&defaultVal;

		    /* default values only set for node 1..64 */
		    if (GL_ARRAY(coNodeId) > 64)  {
			defaultVal = 0;
		    }
		}
	    }
	}
	else if (odIndex <= SRDO_CONFIG_CHECKSUM) {
	    ;
	}
#endif /*  defined(CONFIG_SRDO_PRODUCER) || defined(CONFIG_SRDO_CONSUMER) */

#ifdef CONFIG_PDO_CONSUMER
	else if (odIndex <= RPDO_PARA_LAST_INDEX) {
	    if (subIndex == 1u)  {
	        /*--- RPDO 1-4: only first 4 pdo have pre-defined cob-IDs ---*/
                /* valid bit: profile- or manufacturer-specific
                 * RTR bit: is reserved, but the value is profile- or
                 *          manufacturer-specific
                 * frame bit: always 0 */
		if ((CO_READ_OD16(pObj->index) & 0x1ffu) < 4u)  {
		    defaultVal &= (PDO_NO_VALID_BIT | PDO_NO_RTR_ALLOWED_BIT);
		    defaultVal += CO_COBID_RPDO1
			    + (CO_READ_OD16(pObj->index) & 0x7u) * 0x100u
			    + (UNSIGNED32)GL_ARRAY(coNodeId);
	        /*--- RPDO 5-64 ---*/
                /* valid bit: 1 or profile-specific
                 * RTR bit: profile- or manufacturer-specific
                 * frame bit: profile- or manufacturer-specific */
		}
		pDefaultVal = (UNSIGNED8 *)&defaultVal;
	    }
	}
#endif /* CONFIG_PDO_CONSUMER */

	else if (odIndex < TPDO_PARA_BASE_INDEX) {
	    /* use standard values from od */
	}

#ifdef CONFIG_PDO_PRODUCER
	else if (odIndex <= TPDO_PARA_LAST_INDEX) {
	    if (subIndex == 1u)  {
	        /*--- TPDO 1-4: only first 4 pdo have pre-defined cob-IDs ---*/
                /* valid bit: profile- or manufacturer-specific
                 * RTR bit:   profile- or manufacturer-specific
                 * frame bit: always 0 */
		if ((CO_READ_OD16(pObj->index) & 0x1ff) < 4)  {
		    defaultVal &= (PDO_NO_VALID_BIT | PDO_NO_RTR_ALLOWED_BIT);
		    defaultVal += CO_COBID_TPDO1
			    + (CO_READ_OD16(pObj->index) & 0x1ff) * 0x100
			    + (UNSIGNED32)GL_ARRAY(coNodeId);
		/*--- TPDO 5-64 ---*/
                /* valid bit: 1 or profile-specific
                 * RTR bit: profile- or manufacturer-specific
                 * frame bit: profile- or manufacturer-specific */
		}
		pDefaultVal = (UNSIGNED8 *)&defaultVal;
	    }
	}
#endif /* CONFIG_PDO_PRODUCER */
    }

    if (pDefaultVal != NULL)  {
	/* save new value at od */
/* printf("sub %d, memcpy(%p, %p, %d)\n", subIndex, pTargetAddr, pDefaultVal, size); */
	CO_NUM_MEMCPY(pTargetAddr, pDefaultVal, size, CO_TRUE);
    }

    return(CO_OK);
}

/*******************************************************************
*
*++ coCheckRestrictedCobId - check if a given cobId is reserved
*-- coCheckRestrictedCobId - überprüft ob die gegebene CobId reserviert ist
*
*
*++ This function checks if the given cobId falls into a range reserved by a
*++ CANopen standard. If it does, it is rejected.
*-- Diese Funktion prüft ob die übergebene CobId in einen Bereich fällt, der
*-- von einem CANopen Standard für einen Service reserviert ist. Wenn ja, wird
*-- die CobId zurückgewiesen.
*
* \retval CO_OK
*++ success, cobId is not reserved
*-- Erfolg, CobId ist nicht reserviert
* \retval CO_E_TRANS_TYPE
*++ cobId is reserved
*-- CobId ist reserviert
*
*/
RET_T coCheckRestrictedCobId(
    UNSIGNED16 index,            /**< index in OD, where the cobId will be written */
    UNSIGNED32 newCobId          /**< cob-ID to check*/
    CO_COMMA_LINE_PARA_DECL      /**< number of CAN line 0..CO_MAX_CAN_LINES-1 */
    )
{
RET_T ret = CO_OK;            /* CANopen return value */

    /* general OK for disabled cobIDs */
    if ((newCobId & 0x80000000) == 0x80000000)
    {
        return CO_OK;
    }

    /* if nodeId == 255, don't do any tests */
    /* if (getNodeId(CO_COMMA_LINE_PARA) == 0xFF) */
    if (GL_ARRAY(coNodeId) == 0xff)
    {
        return CO_OK;
    }

    if (newCobId <= 0x0000007FUL)
    {
    	ret = CO_E_TRANS_TYPE;
    }
    else if ((newCobId >= 0x00000101UL) && (newCobId <= 0x00000180UL)) /* SRDO */
    {
    	ret = CO_E_TRANS_TYPE;
    }
    else if ((newCobId >= 0x00000581UL) && (newCobId <= 0x000005FFUL))      /* default SDO TX */
    {
        /* these cobIds may be used for SDO TX and RX, so they are only filtered out if
           the index is not an SDO object */
        if ((index < SSDO_PARA_BASE_INDEX) || (index > CSDO_PARA_LAST_INDEX))
        {
       	    ret = CO_E_TRANS_TYPE;
        }
    }
    else if ((newCobId >= 0x00000601UL) && (newCobId <= 0x0000067FUL)) /* default SDO RX */
    {
        if ((index < SSDO_PARA_BASE_INDEX) || (index > CSDO_PARA_LAST_INDEX))
        {
       	    ret = CO_E_TRANS_TYPE;
        }
    }
#ifndef CONFIG_DYN_SDO_CONNECTION_MANAGER
    else if ((newCobId >= 0x000006E0UL) && (newCobId <= 0x000006FFUL)) /* SDO Manager */
    {
       	ret = CO_E_TRANS_TYPE;
    }
#endif
    else if ((newCobId >= 0x00000701UL) && (newCobId <= 0x0000077FUL)) /* NMT error control */
    {
       	ret = CO_E_TRANS_TYPE;
    }
    else if ((newCobId >= 0x00000780UL) && (newCobId <= 0x000007FFUL))
    {
       	ret = CO_E_TRANS_TYPE;
    }

    return (ret);
}

/*______________________________________________________________________EOF_*/
