/*
 *++ cfg_man.c - Contains Configuration Manager service routines
 *-- cfg_man.c - Beinhaltet Funktionen für Configuration Manager Dienste
 *
 * Copyright (c) 2014-2017 port GmbH Halle (Saale)
 *------------------------------------------------------------------
 */


/****************************************************************************/
/**
*  \file cfg_man.c
*++ Contains configuration manager service routines
*-- Beinhaltet Funktionen für Configuration Manager Dienste
*  \author port GmbH Halle (Saale)
*
*++ This module contains the functions for handling the
*-- Diese Modul beinhaltet Funktionen für
* Configuration Manager.
*
*/

/* header of standard C - libraries */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

/* header of project specific types */

#include <cal_conf.h>
#include <co_odidx.h>
#include <co_mcpy.h>
#include <co_emcy.h>
#include "access.h"
#include "sdo.h"
#include "pdo.h"


/* constant definitions
---------------------------------------------------------------------------*/
#define CONFIG_CFG_MANAGER_SDO_TIMEOUT 200

/* local defined data types
---------------------------------------------------------------------------*/

/* list of external used functions, if not in headers
---------------------------------------------------------------------------*/

/* list of global defined functions
---------------------------------------------------------------------------*/

/* list of local defined functions
---------------------------------------------------------------------------*/
#ifdef CONFIG_CFG_MANAGER
static RET_T writeNextConfigEntry(SDO_CLIENT_T *pSdo CO_COMMA_LINE_PARA_DECL);
static RET_T startUpdateNodeConfig(SDO_CLIENT_T *pSdo CO_COMMA_LINE_PARA_DECL);
static RET_T writeCfgDate(SDO_CLIENT_T *pSdo CO_COMMA_LINE_PARA_DECL);
#endif /* CONFIG_CFG_MANAGER */

# ifdef CONFIG_CFG_MANAGER_CONVERT
static int getKeyValue (char *ptr, char *key, char *buf, int bufLen,
	char delim);
static UNSIGNED8 prepareDcfEntry(UNSIGNED16 ovIdx, UNSIGNED8 ovSubIdx,
	char *pTyp, char *pVar, char  *pDcfBuf, UNSIGNED32 bufLen,
	UNSIGNED32 *pOffs, UNSIGNED8 mode);
static UNSIGNED8 saveDcfEntry(UNSIGNED16 ovIndex, UNSIGNED8 subIndex,
	char *pVar, UNSIGNED32 varLen, char *pBuf, UNSIGNED32 bufLen,
	UNSIGNED32 *pOffs);
static UNSIGNED8 *convertValAndSize(char *pVarType, char *pVarVal,
	UNSIGNED32  *pSize);
static UNSIGNED8 getPdoBit(UNSIGNED8 bitType, UNSIGNED16 ovIdx,UNSIGNED8 ovSub);
static void setPdoBit(UNSIGNED8	bitType, UNSIGNED16 ovIdx);
# endif /* CONFIG_CFG_MANAGER_CONVERT */

/* external variables
---------------------------------------------------------------------------*/

/* global variables
---------------------------------------------------------------------------*/

/* local defined variables
---------------------------------------------------------------------------*/
#ifdef CONFIG_RCS_IDENT
CO_LIB_INIT_VAR static char _rcsid[] = "$Id: cfg_man.c,v 2.10 2016/09/26 11:16:08 rli Exp $";
#endif /* CONFIG_RCS_IDENT */

#ifdef CONFIG_CFG_MANAGER_CONVERT
CO_LIB_INIT_VAR static UNSIGNED8 dcfNodeId = 0;
CO_LIB_UNINIT_VAR static UNSIGNED8 tPdoCobDisabled[512 / 8];
CO_LIB_UNINIT_VAR static UNSIGNED8 rPdoCobDisabled[512 / 8];
CO_LIB_UNINIT_VAR static UNSIGNED8 tPdoSub0Disabled[512 / 8];
CO_LIB_UNINIT_VAR static UNSIGNED8 rPdoSub0Disabled[512 / 8];
#endif /* CONFIG_CFG_MANAGER_CONVERT */



#ifdef CONFIG_CFG_MANAGER
/****************************************************************************/
/**
*++ \brief handleRemoteNodeConfig - check and update remote node config
*-- \brief handleRemoteNodeConfig - testet und updated die Remote Node Konfig
*
*++ This function reads the current configuration via SDO
*++ of a node and compares it with data
*++ of the objects 0x1f26 and 0x1f27.
*++ If they do not match
*++ the function
*-- Diese Funktion ermittelt per SDO die aktuelle Konfiguration
*-- für einen Knoten und vergleicht sie
*-- mit den Daten aus dem Index 0x1f26 bzw. 0x1f27.
*-- Wenn die Konfigurationsdaten nicht übereinstimmen,
*-- wird anschließend ein
*   updateRemoteNodeConfig()
*-- aufgerufen und die Konfiguration aktualisiert.
*++ is called and the configuration is updated.
*
*++ The result and occured errors
*++ are passed to the application with the
*++ indication function
*-- Das Ergebnis bzw. aufgetretene Fehler
*-- werden über die Indikation Funktion
*   cfgManagerInd()
*-- an die Appliktion übermittelt.
*
* \retval CO_OK
*++ success
*-- Erfolg
* \retval CO_E_
*-- see function checkRemoteNodeConfig()
*++ siehe Funktion checkRemoteNodeConfig()
*
*/
RET_T handleRemoteNodeConfig(
	UNSIGNED8  nodeId,	/**< node id for configuration */
	UNSIGNED8  sdoNr	/**< sdo number used for configuration */
	CO_COMMA_LINE_PARA_DECL
    )
{
RET_T retVal;
SDO_CLIENT_T	*pSdo;

    /* check config for remote node */
    retVal = checkRemoteNodeConfig(nodeId, sdoNr CO_COMMA_LINE_PARA);
    if (retVal == CO_OK)  {

	/* save update flag is ok */
	pSdo = searchForClientSdoNr(sdoNr CO_COMMA_LINE_PARA);
	if (pSdo == NULL)  {
	    return(CO_E_NOT_EXIST);
	}
	pSdo->cfg.flags |= CFGMAN_FLAG_CHK_UPD;
    }

    return(retVal);
}


/****************************************************************************/
/**
*++ \brief checkRemoteNodeConfig - read and check remote node config
*-- \brief checkRemoteNodeConfig - liest und testet die Remote Node Konfig
*
*++ This function reads the current configuration
*++ of a node and compares it against the values
*++ of the objects 0x1f26 and 0x1f27.
*++ The result and occured errors
*++ are passed to the application with the
*++ indication function
*-- Diese Funktion ermittelt per SDO die aktuelle Konfiguration
*-- für einen Knoten und vergleicht sie
*-- mit den Daten aus dem Index 0x1f26 bzw. 0x1f27.
*-- Das Ergebnis bzw. aufgetretenen Fehler
*-- werden über die Indikation Funktion
*   cfgManagerInd()
*-- an die Appliktion übermittelt.
*
*++ The SDO parameter (COB-IDs) have to be set
*++ to the approriate values
*++ by the application
*++ before this function is called.
*-- Die SDO Parameter (COB-IDs) müssen vor dem Aufruf dieser Funktion
*-- durch die Applikation auf gültige Werte gesetzt werden.
*
*
* \retval CO_OK
*++ success
*-- Erfolg
* \retval CO_E_NOT_EXIST
*--	sdo doesn't exist
*++	SDO existiert nicht
* \retval CO_E_
*--	see function readSdoReq()
*++	siehe Funktion readSdoReq()
*
*/

RET_T checkRemoteNodeConfig(
	UNSIGNED8  nodeId,	/**< node id for configuration */
	UNSIGNED8  sdoNr	/**< sdo number used for configuration */
	CO_COMMA_LINE_PARA_DECL
    )
{
SDO_CLIENT_T	*pSdo;
RET_T		ret;

    /* get sdo client number */
    pSdo = searchForClientSdoNr(sdoNr CO_COMMA_LINE_PARA);
    if (pSdo == NULL)  {
	return(CO_E_NOT_EXIST);
    }

    /* read configuration data from remote node */
    pSdo->cfg.size = 0;
    ret = readSdoReq(sdoNr, VERIFY_CONFIG_INDEX, 1,
	(UNSIGNED8 *)&pSdo->cfg.size, 4, (CONFIG_CFG_MANAGER_SDO_TIMEOUT * 10)
	CO_COMMA_LINE_PARA);
    if (ret != CO_OK)  {
	/* abort for sdo error */
	cfgManagerInd(nodeId, CFG_MANAGER_SDO_ERROR CO_COMMA_LINE_PARA);
	pSdo->cfg.addr = NULL;
	return(ret);
    }

    pSdo->cfg.addr = (UNSIGNED8 *)&pSdo->cfg.size;
    pSdo->cfg.node = nodeId;
    pSdo->cfg.flags &= (FLAG_T)~CFGMAN_FLAG_CHK_UPD;

    return(CO_OK);
}


/****************************************************************************/
/**
*++ \brief updateRemoteNodeConfig - write concise dcf data to node
*-- \brief updateRemoteNodeConfig - schreibt concise DCF Daten zu einem Knoten
*
*-- Diese Funktion schreibt die aktuelle Konfiguration für einen Knoten,
*-- der im Objektverzeichnis als Consive DCF im Objekt 0x1f22 hinterlegt ist
*-- mit SDO Zugriffen zu dem gewünschten Knoten.
*-- Anschließend werden die Identifikationsdaten aus dem Index 0x1f26 bzw 0x1f27
*-- ebenfalls zum konfigurierten Knoten geschrieben.
*-- Der erfolgreiche Abschluß der Konfiguration bzw. aufgetretenen Fehler
*-- werden über die Indikation Funktion
*   cfgManagerInd()
*-- an die Appliktion übermittelt.
*
*-- Die SDO Parameter (COB-IDs) müssen vor dem Aufruf dieser Funktion
*-- durch die Applikation auf gültige Werte gesetzt werden.
*
*
* \retval CO_OK
*++ success
*-- Erfolg
* \retval CO_E_NOT_EXIST
*++ sdo doesn't exist
*-- sdo existiert nicht
* \retval CO_E_NONEXIST_OBJECT
*++ domain at index 0x1F22 for this node doesn't exist or invalid
*-- domain auf Index 0x1F22 für diesen Knoten ist nicht gültig
* \retval CO_E_
*-- see function writeSdoReq()
*++ siehe Funktion writeSdoReq()
*
*/

RET_T updateRemoteNodeConfig(
	UNSIGNED8  nodeId,	/**< node id for configuration */
	UNSIGNED8  sdoNr	/**< sdo number used for configuration */
	CO_COMMA_LINE_PARA_DECL
    )
{
SDO_CLIENT_T	*pSdo;
RET_T		ret;

    /* get sdo client */
    pSdo = searchForClientSdoNr(sdoNr CO_COMMA_LINE_PARA);
    if (pSdo == NULL)  {
	return(CO_E_NOT_EXIST);
    }

    /* get addesss if configuration */
    pSdo->cfg.addr = getDomainAddr(CONSIZE_DCF_INDEX, nodeId CO_COMMA_LINE_PARA);
    if (pSdo->cfg.addr == NULL)  {
	return(CO_E_NONEXIST_OBJECT);
    }

    /* get size of configuration */
    pSdo->cfg.size = getDomainSize(CONSIZE_DCF_INDEX, nodeId CO_COMMA_LINE_PARA);
    if (pSdo->cfg.size == 0)  {
	pSdo->cfg.addr = NULL;
	return(CO_E_NONEXIST_OBJECT);
    }

    /* get number of configuration entries */
    CO_NUM_MEMCPY(&pSdo->cfg.cnt, pSdo->cfg.addr, 4, CO_TRUE);
    pSdo->cfg.addr += 4;
    pSdo->cfg.flags |= (CFGMAN_FLAG_DATE | CFGMAN_FLAG_TIME);

    /* call configuration loop */
    ret = writeNextConfigEntry(pSdo CO_COMMA_LINE_PARA);

    return(ret);
}


/****************************************************************************/
/*
* writeNextConfigEntry - write next config entry
*
* \retval CO_OK
*	configuration done
* \retval CO_E_MEM
*	not enough data available
* \retval CO_SDO_
*	sdo error
*/
static RET_T writeNextConfigEntry(
	SDO_CLIENT_T	*pSdo		/* pointer to used sdo */
	CO_COMMA_LINE_PARA_DECL
    )
{
UNSIGNED16	index;
UNSIGNED8	subIndex;
UNSIGNED32	size;
RET_T		ret;

    /* next step until table is empty */
    if (pSdo->cfg.cnt == 0)  {

	/* now send configuration information */
	if ((pSdo->cfg.flags & (CFGMAN_FLAG_DATE | CFGMAN_FLAG_TIME)) != 0)  {
	    writeCfgDate(pSdo CO_COMMA_LINE_PARA);
	} else {
	    /* configuration is done */
	    pSdo->cfg.addr = NULL;

	    /* inform application */
	    cfgManagerInd(pSdo->cfg.node, CFG_MANAGER_OK CO_COMMA_LINE_PARA);
	}
	return(CO_OK);
    }

    /* get information for next entry */
    CO_MEMCPY(&index, &pSdo->cfg.addr[0], 2);
    subIndex = pSdo->cfg.addr[2];
    CO_MEMCPY(&size, &pSdo->cfg.addr[3], 4);

    /* check if enough data are available */
    if ((size + 7) > pSdo->cfg.size)  {
	/* not enough data available */
	cfgManagerInd(pSdo->cfg.node, CFG_MANAGER_DATA_MISSING
		CO_COMMA_LINE_PARA);
	pSdo->cfg.addr = NULL;
	return(CO_E_MEM);
    }
    /* write configuration value */
    ret = writeSdoReq(pSdo->sdo.num, index, subIndex,
	(UNSIGNED8 *)&pSdo->cfg.addr[7], size,
	(CONFIG_CFG_MANAGER_SDO_TIMEOUT * 10) CO_COMMA_LINE_PARA);
    if (ret != CO_OK)  {
	/* abort for sdo errors */
	cfgManagerInd(pSdo->cfg.node, CFG_MANAGER_SDO_ERROR CO_COMMA_LINE_PARA);
	pSdo->cfg.addr = NULL;
	return(ret);
    }

    /* all its ok */
    pSdo->cfg.addr += (7 + size);
    pSdo->cfg.size -= (7 + size);
    pSdo->cfg.cnt --;

    return(CO_OK);
}


/****************************************************************************/
/*
* readNextConfigEntry - read next config entry
*
* \retval CO_OK
*	configuration done
* \retval CO_SDO_
*	sdo error
*/
static RET_T readNextConfigEntry(
	SDO_CLIENT_T	*pSdo		/* pointer to used sdo */
	CO_COMMA_LINE_PARA_DECL
    )
{
UNSIGNED32	size, cfgVal;
RET_T		ret;

    /* was answer for subIndex 1 - check it with ov-config */
    if (pSdo->sdo.subIndex == 1)  {
	/* get 0x1f26 */
	ret = getObjEntry(EXPECTED_CONFIG_DATE_INDEX, pSdo->cfg.node,
	    (UNSIGNED8 *)&cfgVal, &size, CO_TRUE CO_COMMA_LINE_PARA);
/* printf("%d\n", cfgVal); */
	if (ret != CO_OK) {
	    cfgManagerInd(pSdo->cfg.node, CFG_MANAGER_DATE_CHECK_FAIL
		CO_COMMA_LINE_PARA);
	    pSdo->cfg.addr = NULL;
	    return(ret);
	}
	/* match the config date ? */
	if ((cfgVal != pSdo->cfg.size) || (cfgVal == 0)) {
	    pSdo->cfg.addr = NULL;
	    if ((pSdo->cfg.flags & CFGMAN_FLAG_CHK_UPD)	== CO_TRUE)  {
		/* start update */
		startUpdateNodeConfig(pSdo CO_COMMA_LINE_PARA);
	    }
	    return(CO_E_PARA_INCOMP);
	}

	/* now start read second value */
	ret = readSdoReq(pSdo->sdo.num, VERIFY_CONFIG_INDEX, 2,
	    (UNSIGNED8 *)&pSdo->cfg.size, 4,
	    (CONFIG_CFG_MANAGER_SDO_TIMEOUT * 10) CO_COMMA_LINE_PARA);
	if (ret != CO_OK)  {
	    cfgManagerInd(pSdo->cfg.node, CFG_MANAGER_SDO_ERROR
		CO_COMMA_LINE_PARA);
	    pSdo->cfg.addr = NULL;

	    /* start update */
	    if ((pSdo->cfg.flags & CFGMAN_FLAG_CHK_UPD)	== CO_TRUE)  {
		ret = startUpdateNodeConfig(pSdo CO_COMMA_LINE_PARA);
	    }
	    return(ret);
	}
    } else {
	/* was answer for subIndex 2 - check it with ov-config */
	/* get 0x1f27 */
	ret = getObjEntry(EXPECTED_CONFIG_TIME_INDEX, pSdo->cfg.node,
	    (UNSIGNED8 *)&cfgVal, &size, CO_TRUE CO_COMMA_LINE_PARA);
	if (ret != CO_OK)  {
	    cfgManagerInd(pSdo->cfg.node, CFG_MANAGER_TIME_CHECK_FAIL
		CO_COMMA_LINE_PARA);
	    pSdo->cfg.addr = NULL;

	    /* start update */
	    if ((pSdo->cfg.flags & CFGMAN_FLAG_CHK_UPD)	== CO_TRUE)  {
		ret = startUpdateNodeConfig(pSdo CO_COMMA_LINE_PARA);
	    }
	    return(ret);
	}
	if ((cfgVal != pSdo->cfg.size) || (cfgVal == 0)) {
	    cfgManagerInd(pSdo->cfg.node, CFG_MANAGER_TIME_CHECK_FAIL
		CO_COMMA_LINE_PARA);
	    pSdo->cfg.addr = NULL;

	    /* start update */
	    if ((pSdo->cfg.flags & CFGMAN_FLAG_CHK_UPD)	== CO_TRUE)  {
		ret = startUpdateNodeConfig(pSdo CO_COMMA_LINE_PARA);
	    }
	    return(CO_E_PARA_INCOMP);
	}

	/* configuration ok - inform application */
	pSdo->cfg.addr = NULL;
	cfgManagerInd(pSdo->cfg.node, CFG_MANAGER_CFG_OK CO_COMMA_LINE_PARA);
    }

    return(CO_OK);
}


/***************************************************************************
*
* cfgManagerSdoEvent - check for configuration up/download
*
* This function is called after a sdo write/read transfer is finished
* and before the user indication is called.
* If an configuration download is active,
* the next sdo transfer will be started
* and the return value is unequal zero.
* If no configuration download is active,
* the function returns zero
* and the normal user confirmation (sdoWrCon()/sdoRdCon) can be called.
*
* \retval 0
*       not for configuration manager, call normal sdoWrCon()/sdoRdCon
* \retval 1
*       handled by configuration manager, don't call sdoWrCon()/sdoRdCon
*
*/
UNSIGNED8 cfgManagerSdoEvent(
	SDO_CLIENT_T	*pSdo,		/* pointer to sdo */
	UNSIGNED32	reason		/* error reason */
	CO_COMMA_LINE_PARA_DECL
    )
{
BOOL_T	cont;

    /* addr is only valid for active configuration transfers */
    if (pSdo->cfg.addr == NULL)  {
	/* not for configuration manager, call normal sdoWrCon */
	return(0);
    }

    /* if transfer was finished without error start next transfer */
    if (reason == 0)  {
	/* start next transfer */
	if (pSdo->upDnType == 1)  {
	    writeNextConfigEntry(pSdo CO_COMMA_LINE_PARA);
	} else {
	    readNextConfigEntry(pSdo CO_COMMA_LINE_PARA);
	}
    } else {
	/* inform application about error reason */
	if (reason == E_SDO_TIMEOUT)  {
	    cfgManagerInd(pSdo->cfg.node, CFG_MANAGER_SDO_TIMEOUT
		CO_COMMA_LINE_PARA);
	} else {
	    /* set special error messages for failure of cfg info */
	    if (pSdo->sdo.index == VERIFY_CONFIG_INDEX)  {
		cont = cfgManagerInd(pSdo->cfg.node,
			CFG_MANAGER_SDO_ABORT_CFG_INFO
			CO_COMMA_LINE_PARA);
		if (cont == CO_TRUE)  {
		    /* continue in spite of error */
		    if (pSdo->upDnType == 1)  {
			writeNextConfigEntry(pSdo CO_COMMA_LINE_PARA);
			return(1);
		    } else {
			readNextConfigEntry(pSdo CO_COMMA_LINE_PARA);
			return(1);
		    }
		}
	    } else {
		cfgManagerInd(pSdo->cfg.node, CFG_MANAGER_SDO_ABORT
			CO_COMMA_LINE_PARA);
	    }
	}
	pSdo->cfg.addr = NULL;
    }

    return(1);
}


/****************************************************************************/
/*
* writeCfgDate - write configuration date and time - write next config entry
*
* \retval CO_OK
*	configuration done
* \retval CO_SDO_
*	sdo error
*/
static RET_T writeCfgDate(
	SDO_CLIENT_T	*pSdo		/* pointer to used sdo */
	CO_COMMA_LINE_PARA_DECL
    )
{
UNSIGNED32	val, size;
UNSIGNED16	idx;
UNSIGNED8	subIdx;
RET_T		ret;

    /* write date ? */
    if ((pSdo->cfg.flags & CFGMAN_FLAG_DATE) != 0)  {
	idx = EXPECTED_CONFIG_DATE_INDEX;
	subIdx = 1;
	pSdo->cfg.flags &= (FLAG_T)~CFGMAN_FLAG_DATE;
    } else {
	idx = EXPECTED_CONFIG_TIME_INDEX;
	subIdx = 2;
	pSdo->cfg.flags &= (FLAG_T)~CFGMAN_FLAG_TIME;
    }

    /* get value */
    ret = getObjEntry(idx, pSdo->cfg.node, (UNSIGNED8 *)&val, &size, CO_TRUE
	CO_COMMA_LINE_PARA);
/* printf("%d\n", cfgVal); */
    if (ret != CO_OK) {
	cfgManagerInd(pSdo->cfg.node, CFG_MANAGER_WRITE_CFG_DATE_FAIL
		CO_COMMA_LINE_PARA);
	pSdo->cfg.addr = NULL;
	pSdo->cfg.flags = 0;
	return(ret);
    }

    /* write it to remote node */
    ret = writeSdoReq(pSdo->sdo.num, VERIFY_CONFIG_INDEX, subIdx,
	(UNSIGNED8 *)&val, size,
	(CONFIG_CFG_MANAGER_SDO_TIMEOUT * 10) CO_COMMA_LINE_PARA);
    if (ret != CO_OK)  {
	cfgManagerInd(pSdo->cfg.node, CFG_MANAGER_SDO_ERROR CO_COMMA_LINE_PARA);
	pSdo->cfg.addr = NULL;
	pSdo->cfg.flags = 0;
	return(ret);
    }

    return(CO_OK);
}


/****************************************************************************/
/*
* startUpdateNodeConfig - start update remote configuration
*
* \retval CO_OK
*	configuration done
* \retval CO_SDO_
*	sdo error
*/
static RET_T startUpdateNodeConfig(
	SDO_CLIENT_T	*pSdo
	CO_COMMA_LINE_PARA_DECL
    )
{
RET_T	ret;

    ret = updateRemoteNodeConfig(pSdo->cfg.node, pSdo->sdo.num
	CO_COMMA_LINE_PARA);
    if (ret != CO_OK)  {
	cfgManagerInd(pSdo->cfg.node, CFG_MANAGER_START_UPDATE_FAIL
		CO_COMMA_LINE_PARA);
	return(ret);
    }
    cfgManagerInd(pSdo->cfg.node, CFG_MANAGER_START_UPDATE CO_COMMA_LINE_PARA);

    return(CO_OK);
}
#endif /* CONFIG_CFG_MANAGER */


#ifdef CONFIG_CFG_MANAGER_CONVERT
/***************************************************************************
*
* setPdoBit - save pdo saved state
*
* save state for cob-id or disabled mapping for each pdo
*/
static void setPdoBit(
	UNSIGNED8	bitType,	/* DCF_PDOCOB or DCF_MAPPING */
	UNSIGNED16	ovIdx		/* index at OV */
    )
{
UNSIGNED16	index;
UNSIGNED8	bit;

    index = (ovIdx & 0x1ff) >> 3;
    bit = 1 << (ovIdx & 0x7);

    /* select kind of PDO */
    if ((ovIdx & 0x800) != 0)  {
	/* transmit pdo */
	if (bitType == DCF_PDOCOB)  {
	    tPdoCobDisabled[index] |= bit;
	} else {
	    tPdoSub0Disabled[index] |= bit;
	}
    } else {
	/* transmit pdo */
	if (bitType == DCF_PDOCOB)  {
	    rPdoCobDisabled[index] |= bit;
	} else {
	    rPdoSub0Disabled[index] |= bit;
	}
    }
}


/***************************************************************************
*
* getPdoBit - return saved pdo state
*
* return state for cob-id or disabled mapping for one pdo
*
* 0 - bit is not set
* 1 - bit is set
*/
static UNSIGNED8 getPdoBit(
	UNSIGNED8	bitType,	/* DCF_PDOCOB or DCF_MAPPING */
	UNSIGNED16	ovIdx,
	UNSIGNED8	ovSub
    )
{
UNSIGNED16	index;
UNSIGNED8	bit;

    /* check is PDO index */
    if ((ovIdx < RPDO_PARA_BASE_INDEX) || (ovIdx > TPDO_MAP_LAST_INDEX))  {
	return(0);
    }

    index = (ovIdx & 0x1ff) >> 3;
    bit = 1 << (ovIdx & 0x7);

    /* ignore all except comm para sub 1 and mapping data sub 0 */
    /* cob-id ? */
    if (bitType == DCF_PDOCOB)  {
	if (((ovIdx & 0x200) != 0) || (ovSub != 1)) {
	    return(0);
	}
	/* select kind of PDO */
	if ((ovIdx & 0x800) != 0)  {
	    /* transmit pdo */
	    if ((tPdoCobDisabled[index] & bit) != 0)  {
		return(1);
	    }
	} else {
	    if ((rPdoCobDisabled[index] & bit) != 0)  {
		return(1);
	    }
	}
    }

    if (bitType == DCF_MAPPING)  {
	/* mapping count ? */
	if (((ovIdx & 0x200) == 0) || (ovSub != 0)) {
	    return(0);
	}

	/* select kind of PDO */
	if ((ovIdx & 0x800) != 0)  {
	    /* transmit pdo */
	    if ((tPdoSub0Disabled[index] & bit) != 0)  {
		return(1);
	    }
	} else {
	    if ((rPdoSub0Disabled[index] & bit) != 0)  {
		return(1);
	    }
	}
    }
    return(0);
}


/***************************************************************************
*
* convertValAndSize - convert variable in correct vartype
*
* at the moment only numeric values until u32 are supported
*
* returns pointer to converted value with correct size
* NULL if an error occured
*/
static UNSIGNED8 *convertValAndSize(
	char		*pVarType,	/* pointer to variable type */
	char		*pVarVal,	/* pointer to variable value */
	UNSIGNED32 	*pSize		/* pointer to variable len */
    )
{
#define MAX_BUF_LEN	20
#define MAX_VAR_LEN_TAB 9
const UNSIGNED32 varLenTab[] = { 0, 1, 1, 2, 4, 1, 2, 4, -4}; /* -4 == REAL32 */
char		buf[MAX_BUF_LEN];
char		*pBuf =	buf;
UNSIGNED32	typ;
UNSIGNED8	*pVar = NULL;
UNSIGNED8	nodeIdOffs = 0;
static UNSIGNED32	u32;
static UNSIGNED16	u16;
static UNSIGNED8	u8;
#ifdef CONFIG_FLOAT_VALUES
static REAL32		r32;
#endif /* CONFIG_FLOAT_VALUES */


    /* get Var len */
    if (getKeyValue(pVarType, "DataType", buf, MAX_BUF_LEN, '\n') != 0)  {
	return(NULL);
    }
    typ = strtol(buf, NULL, 0);
    if (typ == 0)  {
	return(NULL);
    }

    if (typ < MAX_VAR_LEN_TAB)  {
	*pSize = varLenTab[typ];
    } else {
	*pSize = 0;
	/* at the moment we support only up to 4 byte numeric values */
	return(NULL);
    }

    /* get value */
    if (getKeyValue(pVarVal, "ParameterValue", buf, MAX_BUF_LEN, '\n') != 0)  {
	if (getKeyValue(pVarVal, "DefaultValue", buf, MAX_BUF_LEN, '\n') != 0) {
	    return(NULL);
	}
    }

    /* some values depends on NODEID ($NODEID+0x200)*/
    if (strstr(buf, "NODEID") != NULL)  {
	/* set nodeid offs */
	nodeIdOffs = dcfNodeId;
	/* pBuf += strlen("$NODEID+"); */
	pBuf += 7;
    }

    /* conversation depends on type  */
    switch (*pSize)  {
	/* numeric value */
	case 1:	u8 = (UNSIGNED8)strtol(pBuf, NULL, 0);
		pVar = &u8;
		break;
	case 2:	u16 = (UNSIGNED16)strtol(pBuf, NULL, 0);
		u16 += nodeIdOffs;
		pVar = (UNSIGNED8 *)&u16;
		break;
	case 4:	u32 = strtoll(pBuf, NULL, 0);
		u32 += nodeIdOffs;
		pVar = (UNSIGNED8 *)&u32;
		break;
#ifdef CONFIG_FLOAT_VALUES
	case -4: r32 = (REAL32) atof(pBuf);
		pVar = (UNSIGNED8 *)&r32;
		*pSize = 4;
		break;
#endif /* CONFIG_FLOAT_VALUES */
	default:
	    *pSize = 0;
	    return(NULL);
    }

    return(pVar);
}


/***************************************************************************
*
* \brief read key from licence file
*
* Search inifile for '=' and copy everything
* after '=' to the buffer buf
* until the character in delim is found
* or len is reached.
*
* \retval 0 - success
* \retval 1 - failure
*
*/
static int getKeyValue (
	char	*ptr,		/* pointer to buffer of inifile */
	char	*key,		/* key to search for */
	char	*buf,		/* buffer to store value */
	int	bufLen,		/* max len of buffer */
	char	delim		/* line/value delimiter */
    )
{
char	*pStart, *pEnd;
UNSIGNED16	len;

    pEnd = strchr(ptr, '\n');

    /* get start of string */
    pStart = strstr(ptr, key);
    /* if (pStart == NULL)  { */
    if ((pStart == NULL) || (pStart > pEnd)) {
	return(1);
    }

    /* get position of delimiter */
    pEnd = strchr(pStart, delim);
    if (pEnd == NULL)  {
	pEnd = pStart + strlen(ptr);
    }

    pStart += strlen(key);
    len = pEnd - pStart;

    while (len > 0)  {
	if ((isspace((int)*pStart) != 0) || (*pStart == '=')) {
	    pStart++;
	} else {
	    break;
	}
	len--;
    }

    if ((len == 0) || ((len + 1) > bufLen)) {
	return(1);
    }

    memcpy(buf, pStart, len);
    buf[len] = 0;

    return(0);
}


/***************************************************************************
*
* saveDcfEntry - save concise dcf entry at buffer
*
* save the given values at dcf buffer
*
*/
static UNSIGNED8 saveDcfEntry(
	UNSIGNED16	ovIndex,	/* index */
	UNSIGNED8	subIndex,	/* subIndex */
	char		*pVar,		/* pointer to variable value */
	UNSIGNED32	varLen,		/* variable len */
	char	 	*pBuf,		/* pointer to dcf buffer */
	UNSIGNED32	bufLen,		/* dcf buffer length */
	UNSIGNED32	*pOffs		/* pointer to offset at dcf buffer */
    )
{
printf("saveDcfEntry: %x:%d len %d %02x\n", ovIndex, subIndex, varLen, (unsigned char) pVar[0]);

    /* check for enough space at destination buffer */
    if ((7 + varLen) > bufLen)  {
	return(DCFCONVERT_BUF_TO_SMALL);
    }

    /* copy data to dcf buffer */
    memcpy(pBuf + *pOffs, &ovIndex, 2);
    *pOffs += 2;
    memcpy(pBuf + *pOffs, &subIndex, 1);
    *pOffs += 1;
    memcpy(pBuf + *pOffs, &varLen, 4);
    *pOffs += 4;
    memcpy(pBuf + *pOffs, pVar, varLen);
    *pOffs += varLen;

    return(DCFCONVERT_OK);
}


/***************************************************************************
*
* prepareDcfEntry - prepare concise dcf entry
*
* prepare a concise dcf entry
* - cob-ids have to be set invalid before
* - mapping entries have to be disabled sub 0 before
*
*/
static UNSIGNED8 prepareDcfEntry(
	UNSIGNED16	ovIdx,		/* index */
	UNSIGNED8	ovSubIdx,	/* subIndex */
	char		*pTyp,		/* pointer to variable type */
	char		*pVar,		/* pinter to variable value */
	char 		*pDcfBuf,	/* pointer to dcf buffer */
	UNSIGNED32	bufLen,		/* dcf buffer length */
	UNSIGNED32	*pOffs,		/* pointer to offset at dcf buffer */
	UNSIGNED8	mode		/* convert mode */
    )
{
UNSIGNED32	cnt = 0;
UNSIGNED8	ret;
UNSIGNED32	val32;
UNSIGNED8	val8;
UNSIGNED32	size;
UNSIGNED8	*pVarVal;
BOOL_T		specialIdx = CO_TRUE;

    memcpy(&cnt, pDcfBuf, 4);

    /* some indices need a special handling, e.g. COB-Ids, PDO mapping ...*/

    /*--------------------------------------------*/
    /* PDO-CobId */
    if ( (((ovIdx >= RPDO_PARA_BASE_INDEX) && (ovIdx <= RPDO_PARA_LAST_INDEX))
       || ((ovIdx >= TPDO_PARA_BASE_INDEX) && (ovIdx <= TPDO_PARA_LAST_INDEX)))
     && (ovSubIdx == 1))  {
	/* disable COB as first */
	if (getPdoBit(DCF_PDOCOB, ovIdx, ovSubIdx) == 0)  {
	    /* first disable pdo */
	    val32 = PDO_NO_VALID_BIT;
	    ret = saveDcfEntry(ovIdx, ovSubIdx, (char *)&val32, 4,
		    pDcfBuf + 4, bufLen - 4, pOffs);
	    if (ret != 0)  {
		/* convertDcfError(pType, ovIdx, ovSubIdx, ret); */
		return(ret);
	    }
	    cnt++;

	    setPdoBit(DCF_PDOCOB, ovIdx);

	    /* number of entries */
	    memcpy(pDcfBuf, &cnt, 4);
	}

	/* if not pdo cob mode, return */
	if (mode != DCF_PDOCOB) {
	    return(DCFCONVERT_OK);
	}
    }

    /*--------------------------------------------*/
    /* PDO-Mapping */
    if (((ovIdx >= RPDO_MAP_BASE_INDEX) && (ovIdx <= RPDO_MAP_LAST_INDEX))
     || ((ovIdx >= TPDO_MAP_BASE_INDEX) && (ovIdx <= TPDO_MAP_LAST_INDEX)))  {

	/* pdo must be disabled before we can change anything */
	/* pdo already disabled ? */
	if (getPdoBit(DCF_PDOCOB, ovIdx & ~0x200, 1) == 0)  {
	    /* no, disable it */
	    val32 = PDO_NO_VALID_BIT;
	    ret = saveDcfEntry(ovIdx & ~0x200, 1, (char *)&val32, 4,
		    pDcfBuf + 4, bufLen - 4, pOffs);
	    if (ret != 0)  {
		/* convertDcfError(pType, ovIdx, ovSubIdx, ret); */
		return(ret);
	    }
	    cnt++;
	    setPdoBit(DCF_PDOCOB, ovIdx);
	}

	/* if subindex != 0 set as first sub 0 to 0 */
	if (getPdoBit(DCF_MAPPING, ovIdx, 0) == 0)  {
	    val8 = 0;
	    ret = saveDcfEntry(ovIdx, 0, (char *)&val8, 1,
		    pDcfBuf + 4, bufLen - 4, pOffs);
	    if (ret != 0)  {
		/* convertDcfError(pType, ovIdx, ovSubIdx, ret); */
		return(ret);
	    }
	    cnt++;

	    /* set bit for disabled cob-id and sub0 */
	    setPdoBit(DCF_MAPPING, ovIdx);
	}

	if (ovSubIdx == 0)  {
	    /* if not pdo cob mode, return */
	    if (mode != DCF_MAPPING) {
		/* number of entries */
		memcpy(pDcfBuf, &cnt, 4);
		return(DCFCONVERT_OK);
	    }
	}
    }

    /*--------------------------------------------*/
    /* SDO */
    if ((ovIdx >= SSDO_PARA_BASE_INDEX) && (ovIdx <= CSDO_PARA_LAST_INDEX)) {
	val32 = SDO_NO_VALID_BIT;
    } else

    /*--------------------------------------------*/
    if (ovIdx == SYNC_COB_ID_INDEX)  {
	/* disable producer bit */
	val32 = CO_COBID_SYNC;
    } else
    if (ovIdx == TIME_COB_ID_INDEX)  {
	/* disable producer bit */
	val32 = CO_COBID_TIME;
    } else
    if (ovIdx == EMCY_COB_ID_INDEX)  {
	val32 = EMCY_NOT_VALID_BIT;
    } else
    {
	specialIdx = CO_FALSE;
    }

    /* save disable bit for some indices */
    if (specialIdx == CO_TRUE)  {
	ret = saveDcfEntry(ovIdx, ovSubIdx, (char *)&val32, 4,
		pDcfBuf + 4, bufLen - 4, pOffs);
	if (ret != 0)  {
	    return(ret);
	}
	cnt++;
    }

    /* now save new parameter value */
    /* get value and size */
    pVarVal = convertValAndSize(pTyp, pVar, &size);
    if (pVarVal == NULL)  {
	return(DCFCONVERT_BAD_TYPE);
    }
    /* save actual value */
    ret = saveDcfEntry(ovIdx, ovSubIdx, (char *)pVarVal, size,
	    pDcfBuf + 4, bufLen - 4, pOffs);
    if (ret != 0)  {
	return(ret);
    }
    cnt++;

    /* number of entries */
    memcpy(pDcfBuf, &cnt, 4);

    return(DCFCONVERT_OK);
}


/****************************************************************************/
/**
*++ \brief convertToConsiseDcf - convert standard DCF into consice DCF
*-- \brief convertToConsiseDcf - konvertiere in Consive DCF
*
*++ This function converts the passed DCF data
*++ into the consice DCF format.
*++ Required configurations sequences
*++ are considered
*++ like changing of PDO Mapping and setting of COB ID.
*++ Therefore it is necessary
*++ to call this function repeatedly
*++ with the following values
*++ as mode parameter:
*-- Diese Funktion konvertiert die übergebenen DCF-Daten
*-- in das Concise DCF-Format.
*-- Dabei werden auch notwendige Konfigurationsabfolgen
*-- wie z.B. bei Änderungen am Mapping oder Setzen von COB-IDs beachtet.
*-- Aus diesem Grund ist es notwendig,
*-- die Funktion jeweils einmal mit dem Parameter mode mit den Werten
*   - DCF_START
*   - DCF_MAPPING
*   - DCF_PDOCOB
*-- aufzurufen.
*
*++ The parameter pDcfData is a pointer to the buffer
*++ that will contain the converted DCF data.
*++ The size of the buffer is passed as parameter pDcfLen.
*++ On return of the function pDcfLen
*++ is updated to the current length.
*++ The parameter pDcfOffs determines the offset
*++ within the DCF buffer
*++ where the data shall be written to.
*++ The data that is to be converted
*++ is passed with the parameter pData and
*++ the parameter pDataLen contains
*++ the length of the buffer pData
*-- Der Parameter pDcfData ist ein Zeiger auf den Puffer
*-- für die konvertierten Concise DCF-Daten.
*-- Die Größe des Puffers wird mit dem Parameter pDcfLen übergeben.
*-- Der Parameter pDcfOffs gibt den Offset innerhalb des DCF-Puffers an,
*-- wo die Daten geschrieben werden sollen.
*-- Beim Verlassen der Funktion enthält pDcfLen die aktuell genutzte Länge.
*-- Die zu konvertierenden Daten werden mit dem Parameter pData übergeben,
*-- und die Länge der Daten mit dem Parameter dataLen.
*
*
* \retval CO_OK
*++ success
*-- Erfolg
*/
UNSIGNED8 convertToConciseDcf(
	char	  *pDcfData,	/**< pointer to converted concised dcf buffer */
	UNSIGNED32 *pDcfLen,	/**< len of dcf buffer, will be actualized */
	UNSIGNED32 *pDcfOffs,	/**< offset at dcf buffer, will be actualized */
	char	  *pData,	/**< pointer to dcf data */
	UNSIGNED32 dataLen,	/**< length of dcf data buffer */
	UNSIGNED8  mode		/**< mode DCF_START, DCF_MAPPING, DCF_PDOCOB */
    )
{
char		*pBrL, *pBrR, *pSub, *pType, *pParVal = NULL;
UNSIGNED16	ovIdx, i;
UNSIGNED8	ovSubIdx;
UNSIGNED8	ret;
UNSIGNED32	cnt = 0;
UNSIGNED32	len;
#define TMPBUF_LEN	30
char		buf[TMPBUF_LEN];

    /* delete PDO disable bits at first call */
    if (mode == DCF_CONVERT) {
	dcfNodeId = 0;
	for (i = 0; i < (512 / 8); i++)  {
	    tPdoCobDisabled[i] = 0;
	    rPdoCobDisabled[i] = 0;
	    tPdoSub0Disabled[i] = 0;
	    rPdoSub0Disabled[i] = 0;
	}
    }

    /* for all dcf data */
    while (dataLen != 0) {

	/* search for [ */
	pBrL = strchr(pData, '[');
	if (pBrL == NULL)  {
	    break;
	}
	/* search for ] */
	pBrR = strchr(pBrL, ']');
	if (pBrR == NULL)  {
	    return(DCFCONVERT_BRACKET_ERROR);
	}

	/* copy all data to local string */
	len = pBrR - pBrL - 1;
	if (len < TMPBUF_LEN)  {
	    memcpy(buf, pBrL + 1, len);
	    /* set end of string */
	    buf[len] = 0;
	} else {
	    buf[0] = 0;
	    /* dcfConfErr.notProcessed++; */
	}

	/* set next start */
	dataLen -= ((UNSIGNED32)((UNSIGNED8 *)pData - (UNSIGNED8 *)pBrR));
	pData = pBrR;

	/* index must start only with digit */
	if (isdigit(buf[0]) != 0)  {
	    /* "sub" in string ? */
	    pSub = strstr(buf, "sub");
	    if (pSub == NULL)  {
		/* no subindex available */
		ovIdx = (UNSIGNED16)strtol(buf, NULL, 16);
		ovSubIdx = 0;
	    } else {
		/* subindex available */
		ovIdx = (UNSIGNED16)strtol(buf, &pSub, 16);
		ovSubIdx = (UNSIGNED8)strtol(pSub + 3, NULL, 16);
	    }

	    /* check values */
	    if (ovIdx == 0) {
		/* inform application */
		/* convertDcfError(pBrL, ovIndex, subIndex, BAD_INDEX); */
		/* dcfConfErr.index = ovIdx; */
		/* dcfConfErr.subIndex = ovSubIdx; */
		return(DCFCONVERT_BAD_INDEX);
	    }
	    /* search for ParameterValue */
	    pParVal = strstr(pBrR, "ParameterValue");

	    /* if first call ? */
	    if ((mode == DCF_PDOCOB) || (mode == DCF_MAPPING))  {
		if (getPdoBit(mode, ovIdx, ovSubIdx) != 0)  {
		    /* pdo cobid and mapping count have to be set */
		    /* look for next entry */
		    pBrL = strchr(pBrR, '[');
		    if ((pParVal == NULL)
			/* is parameterval here before next [ */
		     || (pBrL == NULL)
		     || ((pBrL != NULL) && (pBrL < pParVal)))  {
			/* nothing found, use DefaultValue */
			pParVal = strstr(pBrR, "DefaultValue");
		    }
		} else  {
		    pParVal = NULL;
		}
	    }

	    /* parameter value avaible */
	    if (pParVal != NULL)  {

		/* look for next entry */
		pBrL = strchr(pBrR, '[');
		/* is parameterval here before next [ */
		if ((pBrL == NULL) || ((pBrL != NULL) && (pParVal < pBrL)))  {
		    /* get pointer for datatype */
		    pType = strstr(pBrR, "DataType");

		    /* save at concise dcf buffer */
		    ret = prepareDcfEntry(ovIdx, ovSubIdx, pType, pParVal,
			    pDcfData, *pDcfLen, pDcfOffs, mode);
		    if (ret != 0)  {
			/* convertDcfError(pType, ovIndex, subIndex, ret); */
			/* dcfConfErr.index = ovIdx; */
			/* dcfConfErr.subIndex = ovSubIdx; */
			return(ret);
		    }
		    cnt++;
		}
	    } else {
/* printf("%s - 0x%x:%d no default\n", buf, ovIdx, ovSubIdx); */
	    }
	} else {
	    /* ignore all except DeviceCommissioning nodeId */
	    if (strstr(buf, "DeviceCommissioning") != NULL)  {
		char	nodeBuf[10];

		/* search for ParameterValue */
		pParVal = strstr(pBrR, "NodeID");
		getKeyValue(pParVal, "NodeID", nodeBuf, 10, '\n');
		dcfNodeId = (UNSIGNED8)strtol(nodeBuf, NULL, 0);
	    } else {
/* printf("%s - ignored\n", buf); */
	    }
	}
    }

    *pDcfLen = *pDcfOffs + 4;

    return(DCFCONVERT_OK);
}
#endif /* CONFIG_CFG_MANAGER_CONVERT */

/*______________________________________________________________________EOF_*/
