/*
 *++ pdo.c - Contains PDO service routines
 *-- pdo.c - Beinhaltet Funktionen für PDO Dienste
 *
 * Copyright (c) 1996-2017 port GmbH Halle (Saale)
 *------------------------------------------------------------------
 */


/****************************************************************************/
/**
*  \file pdo.c
*++ Contains PDO service routines
*-- Beinhaltet Funktionen für PDO Dienste
*  \author port GmbH Halle (Saale)
*
*++ This module contains the functions for handling the
*-- Diese Modul beinhaltet Funktionen für
* Process Data Objects (PDO).
*
*++ The CANopen Library supports asynchronous, synchrounous,
*++ cyclic and acyclic PDOs.
*++ All Transmit-PDO can be requested via the CAN-RTR request
*++ if the CAN driver supports RTR messages.
*-- Die CANopen Library unterstützt asynchrone, synchrone
*-- zyklische und azyklische PDOs.
*-- Alle Transmit-PDO's können über ein RTR angefordert werden,
*-- wenn der CAN-Treiber RTR-Telegramme unterstützt.
*
*++ In the states OPERATIONAL and PRE_OPERATIONAL the node's
*++ PDO parameter set can be changed via SDO transfer or
*++ the node's local application program.
*++ But changing of the transmission type is only possible in the state
*++ PRE_OPERATIONAL.
*-- In den Zuständen OPERATIONAL und PRE_OPERATIONAL
*-- lassen sich die PDO-Parameter über SDO
*-- oder durch das lokale Applikationsprogramm ändern.
*-- Der Übertragungstyp kann nur um Zustand PRE_OPERATIONAL
*-- geändert werden.
*
*++ The CANopen Library by \em port support bitwise mapping.
*++ That means up to 64 variables (1 bit) can be mapped into one PDO.
*++ Futher a so called dummy entry mapping
*++ (with index entries 1-7) is possible.
*++ For that kind of mapping the corresponding data in the PDO is
*++ not evaluated by the device.
*++ This feature is useful for transmitting data to several devices
*++ by using one PDO.
*++ Each device is only utilizing a certain part of the PDO.
*-- Die CANopen Library von \em port unterstützt wahlweise bitweises Mapping.
*-- Das bedeutet, daß bis zu 64 Variablen (1 bit) auf ein PDO gemappt
*-- werden können.
*-- Weiterhin ist das sogenannte dummy entry mapping
*-- (mit den Indexnumern 1-7) möglich.
*-- Mit diesem Mapping werden die korrespondierenden Daten vom
*-- Gerät nicht übernommen.
*-- Diese Methode ist nützlich für das Versenden von Daten an verschiedene
*-- Geräte mit einem PDO.
*-- Jedes Gerät übernimmt nur den Teil der PDO Daten, die für ihn bestimmt sind.
*
*++ Dynamically PDO mapping is possible.
*-- Dynamisches PDO Mapping wird unterstützt.
*
*++ This modul is very scalable through compiler defines.
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
#include <co_emcy.h>
#include <co_mcpy.h>
#include "pdo.h"
#include "access.h"
#include "cmscodec.h"
#include "nmt.h"
#include "drv.h"

#if defined(CONFIG_MPDO_DEST) || defined(CONFIG_MPDO_SRC)
# include "mpdo.h"
#endif /* defined(CONFIG_MPDO_DEST) || defined(CONFIG_MPDO_SRC) */

#if defined(CONFIG_SYNC_PRODUCER) || defined(CONFIG_SYNC_CONSUMER)
# include "sync.h"
#endif /* defined(SYNC_PRODUCER) || defined SYNC_CONSUMER) */

#ifdef CONFIG_REDUNDANCY_SUPPORT
# include "reduncy.h"
#endif /* CONFIG_REDUNDANCY_SUPPORT */

#ifdef CONFIG_NO_GLOBAL_VARS
# include <co_timer.h>
#endif /*CONFIG_NO_GLOBAL_VARS*/

#ifdef CO_CONFIG_ENABLE_OBJ_CALLBACK
# include <co_acces.h>
#endif /* CO_CONFIG_ENABLE_OBJ_CALLBACK */


/* constant definitions
---------------------------------------------------------------------------*/
#ifdef CONFIG_FAST_SORT
# define CONFIG_PDO_FAST_SORT
#endif /* CONFIG_FAST_SORT */

#ifdef CO_PRE_SYNC_MAP_USER_FCT
#else
# define CO_PRE_SYNC_MAP_USER_FCT
#endif

#ifdef CONFIG_DYN_MEM_ALLOC
# define PDO_PRODUCER_CNT	co_pdoProdCnt
# define PDO_CONSUMER_CNT	co_pdoConsCnt
#else /* CONFIG_DYN_MEM_ALLOC */
# define PDO_PRODUCER_CNT	CONFIG_PDO_PRODUCER
# define PDO_CONSUMER_CNT	CONFIG_PDO_CONSUMER
#endif /* CONFIG_DYN_MEM_ALLOC */


#ifndef CO_CONFIG_MAX_PDO_MAP_BYTES
# define CO_CONFIG_MAX_PDO_MAP_BYTES 8
#endif /* CO_CONFIG_MAX_PDO_MAP_BYTES */

/* local defined data types
---------------------------------------------------------------------------*/

/* list of external used functions, if not in headers
---------------------------------------------------------------------------*/
#if defined(CONFIG_MPDO_DEST) || defined(CONFIG_MPDO_SRC)
RET_T mpdoTimerEventInd(UNSIGNED16 pdoNr, UNSIGNED8 mpdoType CO_COMMA_LINE_PARA_DECL);
#endif /* CONFIG_MPDO_DEST || CONFIG_MPDO_SRC */

/* list of global defined functions
---------------------------------------------------------------------------*/

/* list of local defined functions
---------------------------------------------------------------------------*/
#if defined(CONFIG_PDO_CONSUMER) || defined(CONFIG_PDO_PRODUCER)
static RET_T setPdoCobId(PDO_T *pPdo, UNSIGNED32 cobId, UNSIGNED8 kind
	CO_COMMA_LINE_PARA_DECL);
static RET_T setPdoTransType(PDO_T *pPdo, UNSIGNED8 kind, UNSIGNED8 transType
	CO_COMMA_LINE_PARA_DECL);
static RET_T setPdoInhibitTime(PDO_T *pPdo, UNSIGNED16 inhibitTime
	CO_COMMA_LINE_PARA_DECL);

# ifdef CONFIG_PDO_EVENTTIMER
static RET_T setPdoEventTime(PDO_T *, UNSIGNED8, UNSIGNED16 eventTime
	CO_COMMA_LINE_PARA_DECL);
# endif /* CONFIG_PDO_EVENTTIMER */

# ifdef CONFIG_PDO_SYNC_START_VALUE
static RET_T setPdoSyncStartVal(PDO_T *pPdo, UNSIGNED8 syncVal
	CO_COMMA_LINE_PARA_DECL);
# endif /* CONFIG_PDO_SYNC_START_VALUE */

static PDO_T *searchForPdoCobId(PDO_T	*pdoList,
# ifdef CONFIG_PDO_FAST_SORT
	UNSIGNED16 *idxList,
# endif /* CONFIG_PDO_FAST_SORT */
	INTEGER16 max, COB_IDENT_T cobId);

static INTEGER16 searchForPdoNr(PDO_T *pdoList,
# ifdef CONFIG_PDO_FAST_SORT
	UNSIGNED16 *idxList,
# endif /* CONFIG_PDO_FAST_SORT */
	INTEGER16 listLen, UNSIGNED16 val);

# ifdef CONFIG_PDO_FAST_SORT
static void sortIntoPdoNrList(PDO_T *pdoList, UNSIGNED16 *idxList,
	UNSIGNED16 valIdx, UNSIGNED16 listLen);
static void sortPdoCobIdList(PDO_T *pdoList, UNSIGNED16 *idxList,
	UNSIGNED16 listLen);
# endif /* CONFIG_PDO_FAST_SORT */
#endif /* defined(CONFIG_PDO_CONSUMER) || defined(CONFIG_PDO_PRODUCER) */


/* external variables
---------------------------------------------------------------------------*/

/* global variables
---------------------------------------------------------------------------*/
#ifdef CONFIG_MULT_LINES
# define CO_TPDO_LINE_CNTS	GL_ARRAY(co_trPdoLineCnts)
# define CO_RPDO_LINE_CNTS	GL_ARRAY(co_recPdoLineCnts)
#else /* CONFIG_MULT_LINES */
# define CO_TPDO_LINE_CNTS	PDO_PRODUCER_CNT
# define CO_RPDO_LINE_CNTS	PDO_CONSUMER_CNT
#endif /* CONFIG_MULT_LINES */

#ifdef CONFIG_NO_GLOBAL_VARS
#else /* CONFIG_NO_GLOBAL_VARS */

# ifdef CONFIG_PDO_PRODUCER
#  ifdef CONFIG_DYN_MEM_ALLOC
CO_LIB_UNINIT_VAR PDO_T		*p_co_trPdo[1];			/* transmit pdo structures */
CO_LIB_UNINIT_VAR UNSIGNED16	co_pdoProdCnt;			/* number of PDOs */
#  else /* CONFIG_DYN_MEM_ALLOC */
CO_LIB_UNINIT_VAR PDO_T		co_trPdo[CONFIG_PDO_PRODUCER];	/* transmit pdo structures */
#  endif /* CONFIG_DYN_MEM_ALLOC */
CO_LIB_UNINIT_VAR INTEGER16	co_trPdoCnt CO_LINE_PARA_ARRAY_DEF;/* actual pdo count */

#  ifdef CONFIG_MULT_LINES
			/* pdo producer line counters */
#   ifdef CONFIG_DYN_MEM_ALLOC
CO_LIB_UNINIT_VAR INTEGER16	co_trPdoLineCnts[CO_MAX_CAN_LINES];
#   else /* CONFIG_DYN_MEM_ALLOC */
CO_LIB_CONST_VAR INTEGER16	co_trPdoLineCnts[CO_MAX_CAN_LINES] =
			    { CONFIG_PDO_PRODUCER_LINECFG };
#   endif /* CONFIG_DYN_MEM_ALLOC */
			/* pdo producer line offsets */
CO_LIB_UNINIT_VAR UNSIGNED16	co_trPdoLineOffs CO_LINE_PARA_ARRAY_DEF;
#  else /* CONFIG_MULT_LINES */
CO_LIB_UNINIT_VAR INTEGER16	co_trPdoLineCnts;
#  endif /* CONFIG_MULT_LINES */
# endif /* CONFIG_PDO_PRODUCER */

# ifdef CONFIG_PDO_CONSUMER
#  ifdef CONFIG_DYN_MEM_ALLOC
CO_LIB_UNINIT_VAR PDO_T		*p_co_recPdo[1];		/* receive pdo structures */
CO_LIB_UNINIT_VAR UNSIGNED16	co_pdoConsCnt;			/* number of RPDOs */
#  else /* CONFIG_DYN_MEM_ALLOC */
CO_LIB_UNINIT_VAR PDO_T		co_recPdo[CONFIG_PDO_CONSUMER];	/* receive pdo structures */
#  endif /* CONFIG_DYN_MEM_ALLOC */
CO_LIB_UNINIT_VAR INTEGER16	co_recPdoCnt CO_LINE_PARA_ARRAY_DEF;/* actual pdo count */

#  ifdef CONFIG_MULT_LINES
			/* pdo consumer line counters */
#   ifdef CONFIG_DYN_MEM_ALLOC
CO_LIB_UNINIT_VAR INTEGER16	co_recPdoLineCnts[CO_MAX_CAN_LINES];
#   else /* CONFIG_DYN_MEM_ALLOC */
CO_LIB_CONST_VAR INTEGER16	co_recPdoLineCnts[CO_MAX_CAN_LINES] =
			    { CONFIG_PDO_CONSUMER_LINECFG };
#   endif /* CONFIG_DYN_MEM_ALLOC */
CO_LIB_UNINIT_VAR UNSIGNED16	co_recPdoLineOffs CO_LINE_PARA_ARRAY_DEF;
#  else /* CONFIG_MULT_LINES */
CO_LIB_UNINIT_VAR INTEGER16	co_recPdoLineCnts;
#  endif /* CONFIG_MULT_LINES */
# endif /* CONFIG_PDO_CONSUMER */
#endif /* CONFIG_NO_GLOBAL_VARS */


/* local defined variables
---------------------------------------------------------------------------*/

#ifdef CONFIG_NO_GLOBAL_VARS
#define co_maxMappingCnt	CONFIG_MAPPING_CNT
#else /* CONFIG_NO_GLOBAL_VARS */

# ifdef CONFIG_PDO_PRODUCER
#  ifdef CONFIG_PDO_FAST_SORT
CO_LIB_UNINIT_VAR UNSIGNED16	*p_co_trPdoNrIdxList[1];
CO_LIB_UNINIT_VAR UNSIGNED16	*p_co_trPdoCobIdxList[1];
#   ifdef CONFIG_DYN_MEM_ALLOC
#   else /* CONFIG_DYN_MEM_ALLOC */
					/* sorted pdo number index list */
CO_LIB_UNINIT_VAR static UNSIGNED16	co_trPdoNrIdxList[CONFIG_PDO_PRODUCER];
					/* sorted COB index list */
CO_LIB_UNINIT_VAR static UNSIGNED16	co_trPdoCobIdxList[CONFIG_PDO_PRODUCER];
#   endif /* CONFIG_DYN_MEM_ALLOC */
#  endif /* CONFIG_PDO_FAST_SORT */
# endif /* CONFIG_PDO_PRODUCER */

# ifdef CONFIG_PDO_CONSUMER
#  ifdef CONFIG_PDO_FAST_SORT
#   ifdef CONFIG_DYN_MEM_ALLOC
CO_LIB_UNINIT_VAR UNSIGNED16	*p_co_recPdoNrIdxList[1]; /* sorted pdo number index list */
CO_LIB_UNINIT_VAR UNSIGNED16	*p_co_recPdoCobIdxList[1]; /* sorted COB index list */
#   else /* CONFIG_DYN_MEM_ALLOC */
					/* sorted pdo number index list */
CO_LIB_UNINIT_VAR static UNSIGNED16	co_recPdoNrIdxList[CONFIG_PDO_CONSUMER];
					/* sorted COB index list */
CO_LIB_UNINIT_VAR static UNSIGNED16	co_recPdoCobIdxList[CONFIG_PDO_CONSUMER];
#   endif /* CONFIG_DYN_MEM_ALLOC */
#  endif /* CONFIG_PDO_FAST_SORT */
#  ifdef CONFIG_PDO_DATA_PTR_FCT
CO_LIB_UNINIT_VAR static UNSIGNED8	*pPdoRecData CO_LINE_PARA_ARRAY_DEF;
#  endif /* CONFIG_PDO_DATA_PTR_FCT */
# endif /* CONFIG_PDO_CONSUMER */

# if defined(CONFIG_PDO_CONSUMER) || defined(CONFIG_PDO_PRODUCER)
/* Mapping entries are not depending on the lines */
			/* PDO mapping table */
#  ifdef CONFIG_DYN_MEM_ALLOC
CO_LIB_UNINIT_VAR PDO_MAP_T		*p_co_mappingTable[1];
CO_LIB_UNINIT_VAR UNSIGNED16		co_maxMappingCnt;
#  else /* CONFIG_DYN_MEM_ALLOC */
CO_LIB_UNINIT_VAR PDO_MAP_T		co_mappingTable[CONFIG_MAPPING_CNT];
#define co_maxMappingCnt	CONFIG_MAPPING_CNT
#  endif /* CONFIG_DYN_MEM_ALLOC */
			/* actual mapping count */
CO_LIB_INIT_VAR static UNSIGNED16	co_mappingCnt = 0u;
# endif /* defined(CONFIG_PDO_CONSUMER) || defined(CONFIG_PDO_PRODUCER) */
#endif /* CONFIG_NO_GLOBAL_VARS */

#if defined(CONFIG_PDO_CONSUMER) || defined(CONFIG_PDO_PRODUCER)
/*******************************************************************
*
* pdoExist - searchs for PDO in event list
*
* This function tests whether a PDO exists or not
* and returns a pointer to the pdo structure.
* This is doing by comparing the PDO number and the direction flag.
*
* \retval address
* pointer to event structure - success
* \retval NULL
* error
*
*/

PDO_T *pdoExist(
	UNSIGNED16 num,		/* number of the PDO */
	UNSIGNED8  pdoType	/* RECEIVE/TRANSMIT PDO */
	CO_COMMA_LINE_PARA_DECL	/* number of CAN line 0..CO_MAX_CAN_LINES-1 */
    )
{
PDO_T	*pPdoInUse;		/* pointer to actual pdo */
INTEGER16 idx;

    /* set the event list */
    if (pdoType == TRANSMIT_PDO)  {

# ifdef CONFIG_PDO_PRODUCER
	idx = searchForPdoNr(
#  ifdef CONFIG_MULT_LINES
		&GL_PVAR(co_trPdo)[GL_ARRAY(co_trPdoLineOffs)],
#   ifdef CONFIG_PDO_FAST_SORT
		&GL_PVAR(co_trPdoNrIdxList)[GL_ARRAY(co_trPdoLineOffs)],
#   endif /* CONFIG_PDO_FAST_SORT */
#  else /* CONFIG_MULT_LINES */
		&GL_PVAR(co_trPdo)[0],
#   ifdef CONFIG_PDO_FAST_SORT
		&GL_PVAR(co_trPdoNrIdxList)[0],
#   endif /* CONFIG_PDO_FAST_SORT */
#  endif /* CONFIG_MULT_LINES */
		GL_ARRAY(co_trPdoCnt), num);
	if (idx < 0)  {
	    return(NULL);
	}

	pPdoInUse = &GL_PVAR(co_trPdo)[idx
#  ifdef CONFIG_MULT_LINES
		+ GL_ARRAY(co_trPdoLineOffs)
#  endif /* CONFIG_MULT_LINES */
		];
# else /* CONFIG_PDO_PRODUCER */
	return(NULL);
# endif /* CONFIG_PDO_PRODUCER */
    } else  {

# ifdef CONFIG_PDO_CONSUMER
	idx = searchForPdoNr(
#  ifdef CONFIG_MULT_LINES
		&GL_PVAR(co_recPdo)[GL_ARRAY(co_recPdoLineOffs)],
#   ifdef CONFIG_PDO_FAST_SORT
		&GL_PVAR(co_recPdoNrIdxList)[GL_ARRAY(co_recPdoLineOffs)],
#   endif /* CONFIG_PDO_FAST_SORT */
#  else /* CONFIG_MULT_LINES */
		&GL_PVAR(co_recPdo)[0],
#   ifdef CONFIG_PDO_FAST_SORT
		&GL_PVAR(co_recPdoNrIdxList)[0],
#   endif /* CONFIG_PDO_FAST_SORT */
#  endif /* CONFIG_MULT_LINES */
		GL_ARRAY(co_recPdoCnt), num);
	if (idx < 0)  {
	    return(NULL);
	}

	pPdoInUse = &GL_PVAR(co_recPdo)[idx
#  ifdef CONFIG_MULT_LINES
		+ GL_ARRAY(co_recPdoLineOffs)
#  endif /* CONFIG_MULT_LINES */
		];
# else /* CONFIG_PDO_CONSUMER */
	return(NULL);
# endif /* CONFIG_PDO_CONSUMER */
    }

    return(pPdoInUse);
}
#endif /* defined(CONFIG_PDO_CONSUMER) || defined(CONFIG_PDO_PRODUCER) */

#if defined(CONFIG_PDO_CONSUMER) || defined(CONFIG_PDO_PRODUCER)
/*******************************************************************
*
*++ checkMappingEntry - checks one mapping entry
*-- checkMappingEntry - prueft einen Mapping Eintrag
*
* \internal
*
*++ This function checks one mapping entry, looking at attributes and
*++ object size. Dummy mappings are allowed only for RPDOs.
*-- Diese Funktion testet einen einen Mapping Eintrag.
*-- Dazu werden für normale Mappingeinträge die Attribute getestet.
*-- Dummy Mapping Einträge sind nur für Receive PDOs erlaubt.
*-- Weiterhin wird die Objektgröße mit dem Mapping Eintrag überprüft
*
*
* \retval
*	RET_T
*/

RET_T checkMappingEntry(
	UNSIGNED32 newMapEntry,	/* new mapping entry */
	UNSIGNED8  kind		/* kind of PDO RECEIVE_PDO/TRANSMIT_PDO */
	CO_COMMA_LINE_PARA_DECL	/* number of CAN line 0..CO_MAX_CAN_LINES-1 */
   )
{
LIST_ELEMENT_T  *curObj;        /* pointer to current object */
UNSIGNED16      mapIndex = 0;   /* mapping index */
UNSIGNED8       mapSubIndex = 0;/* mapping subIndex */
UNSIGNED8       mapLength = 0;  /* mapping len */
UNSIGNED16      attr = 0;       /* object attribute */
UNSIGNED8       *addr;          /* pointer to address */
UNSIGNED32      size = 0;       /* object size */

    mapIndex    = (UNSIGNED16)
		  ((newMapEntry  >> MAP_INDEX_SHIFT) & MAP_INDEX_MASK);
    mapSubIndex = (UNSIGNED8)
		  ((newMapEntry  >> MAP_SUBINDEX_SHIFT) & MAP_SUBINDEX_MASK);
    mapLength   = (UNSIGNED8)(newMapEntry & MAP_LENGTH_MASK);

    /* index value exceeds the physical limitations */
    curObj = searchObj(mapIndex CO_COMMA_LINE_PARA);

    /* get attribute of the mapping index */
    (void)getObjPtrAttr(curObj, mapIndex, mapSubIndex, &attr CO_COMMA_LINE_PARA);

    /* PDO Mapping Receive PDO */
    if (kind == RECEIVE_PDO) {
	/* for objects 1 -7 (basic datatypes) mapping is always
	   possible (dummy mapping) */
	/* dummy or real mapping ? */
	if (mapIndex > 0x7u) {
	    /* real mapping - mapped object needs mapping and write perm */
	    if ((attr & (CO_MAP_PERM | CO_WRITE_PERM))
		    != (CO_MAP_PERM | CO_WRITE_PERM))  {
		return(CO_E_MAP);
	    }
	}
    }

    /* PDO Mapping Transmit PDO */
    else  {
	/* check for dummy mapping */
	if (mapIndex > 0x7u) {
	    /* no dummy mapping - var needs read and mapping permission */
	    if ((attr & (CO_MAP_PERM | CO_READ_PERM))
		    != (CO_MAP_PERM | CO_READ_PERM))  {
		return(CO_E_MAP);
	    }
	} else {
	    /* dummy mapping for transmit PDOs is not allowed */
	    /* exception for delete this mapping entry */
	    if (newMapEntry != 0u)  {
		return(CO_E_MAP);
	    }
	}
    }

    /* check for correct mapping length */
    /* dummy mapping ? */
    if (mapIndex > 0x7u) {
	/* no, get size and check for correct data len */
	if (getObjPtrAddr(curObj, mapIndex, mapSubIndex, &addr, &size
		    CO_COMMA_LINE_PARA) != CO_OK)  {
	    return(CO_E_NOT_EXIST);
	}
        /* if not BOOLEAN as 1 bit */
        if (mapLength != 0x1)
	{
	    /* calculate the length in bits */
            size = size << 3;
        }
    } else {
	/* dummy mapping */
	size = getOvDataTypeLen(mapIndex);
    }

    if (mapLength > size) {
	return(CO_E_MAP);
    }

# ifndef CO_CONFIG_NO_MAPPING_LOWER_SIZE_CHECK
    if (mapLength < size) {
	return(CO_E_MAP);
    }
# endif /* CO_CONFIG_NO_MAPPING_LOWER_SIZE_CHECK */

    /* zero mapping isn't allowed */
    if (mapLength == 0u) {
	return(CO_E_MAP);
    }

    return(CO_OK);
}
#endif /* (defined(CONFIG_PDO_CONSUMER) || defined(CONFIG_PDO_PRODUCER)) */

#if defined(CONFIG_PDO_CONSUMER) || defined(CONFIG_PDO_PRODUCER)
/*******************************************************************
*
*++ checkMappingTable - check the complete mapping table
*-- checkMappingTable - prüft die gesamte Mapping Tabelle
*
* \internal
*
*++ This function checks the whole mapping table.
*++ Each mapping entry is tested by the function
*-- Diese Funktion testet die komplette Mapping Tabelle.
*-- Jeder Mappingeintrag wird mit Hilfe der Funktion
* checkMappingEntry()
*++ If it is ok,
*++ an internal mapping table is created.
*-- auf Gültigkeit geprüft.
*-- Anschliessend wird die interne Mappingstruktur aufgebaut.
*
*
* \retval
*	RET_T
*
*/

RET_T checkMappingTable(
    PDO_T *pPdo,                /* pointer to pdo struct */
    UNSIGNED16 index,           /* pdo Mapping index */
    UNSIGNED8 kind              /* kind of PDO RECEIVE_PDO/TRANSMIT_PDO */
    CO_COMMA_LINE_PARA_DECL     /* number of CAN line 0..CO_MAX_CAN_LINES-1 */
   )
{
LIST_ELEMENT_T  *curObj;            /* pointer to current object */
UNSIGNED16      mapIndex = 0;       /* Mapping index */
UNSIGNED8       mapSubIndex = 0;    /* Mapping subIndex */
UNSIGNED8       mapLength = 0;      /* Mapping length */
UNSIGNED32      *mapEntry;          /* mapping entry */
UNSIGNED8       i, mappingCnt = 0;  /* number of mappings */
UNSIGNED32      size = 0;           /* object size */
PDO_MAP_T       *pMap;              /* pointer to mapping data */
UNSIGNED8       mappedBits = 0u;    /* for byte enconding in bytes ! */
UNSIGNED8       *pU8;               /* pointer to u8 */

    /* get mapping count */
    if (getObjEntry(index, 0u, (UNSIGNED8 *)&mappingCnt, &size, CO_TRUE
		CO_COMMA_LINE_PARA) != CO_OK)  {
	return(CO_E_NOT_EXIST);
    }

    /* disable mapping ? */
    if (mappingCnt == 0u)  {
	pPdo->flags |= PDOFLAG_MAP_DISABLED;
    }

# if defined(CONFIG_MPDO_DEST) || defined(CONFIG_MPDO_SRC)
    /* reset mpdo modes */
    pPdo->mpdoFlags = 0;
# endif /*  defined(CONFIG_MPDO_DEST) || defined(CONFIG_MPDO_SRC) */

# if defined(CONFIG_MPDO_DEST)
    /* MPDO destination mode is signed as 255 */
    if (mappingCnt == 255)  {
	/* MPDO dest. mode */
	/* if transmit PDO ? */
	if (kind == TRANSMIT_PDO)  {
	    /* MPDO Producer */
	    pPdo->mpdoFlags = MPDOFLAG_DEST_PRODUCER;
	    mappingCnt = 1;
	} else {
	    /* MPDO Consumer */
	    pPdo->mpdoFlags = MPDOFLAG_DEST_CONSUMER;
	    mappingCnt = 0;
	}
    }
# endif /* defined(CONFIG_MPDO_DEST) */

# if defined(CONFIG_MPDO_SRC)
    /* MPDO source mode is signed as 254 */
    if (mappingCnt == 254)  {
	/* MPDO src mode */
	mappingCnt = 0;
	/* if transmit PDO ? */
	if (kind == TRANSMIT_PDO)  {
#  ifdef CONFIG_PDO_PRODUCER
	    /* MPDO Producer */
	    pPdo->mpdoFlags = MPDOFLAG_SRC_PRODUCER;
	    if (testMPdoScannerList(CO_LINE_PARA) != CO_OK) {
		return(CO_E_MAP);
    	    }
#  endif /* CONFIG_PDO_PRODUCER */
	} else {
#  ifdef CONFIG_PDO_CONSUMER
	    /* MPDO Consumer */
	    pPdo->mpdoFlags = MPDOFLAG_SRC_CONSUMER;
	    /* test the mapping entries at the dispatcher list */
	    if (testMPdoDispatcherList(CO_LINE_PARA) != CO_OK)  {
		return(CO_E_MAP);
	    }
#endif /* CONFIG_PDO_CONSUMER */
	}
    }
# endif /* defined(CONFIG_MPDO_SRC) */

    pMap = &GL_PVAR(co_mappingTable)[pPdo->mapStartIdx];
    if (mappingCnt > pPdo->maxMapCnt)  {
	return(CO_E_MAP);
    }

# ifdef CONFIG_VIRTUAL_OBJECTS
    /* init virtual flag */
    pPdo->virtualObjFlags = PDOFLAG_VIRTUAL_OBJ_FALSE;
# endif /* CONFIG_VIRTUAL_OBJECTS */


    /* Now check each mapping entry */
    for (i = 1u; i <= mappingCnt; i++)  {
	/* get mapping entry */
	if (getObjAddr(index, i, &pU8, &size CO_COMMA_LINE_PARA) != CO_OK)  {
	    return(CO_E_NOT_EXIST);
	}
	mapEntry = (UNSIGNED32 *)pU8;

	/* first check the mapped value */
	if (checkMappingEntry(*mapEntry, kind CO_COMMA_LINE_PARA) != CO_OK)  {
	    return(CO_E_MAP);
	}

	/* set temporary variables for easier access */
	mapIndex    = (UNSIGNED16)
		      ((*mapEntry >> MAP_INDEX_SHIFT) & MAP_INDEX_MASK);
	mapSubIndex = (UNSIGNED8)
		      ((*mapEntry >> MAP_SUBINDEX_SHIFT) & MAP_SUBINDEX_MASK);
	mapLength   = (UNSIGNED8)(*mapEntry & MAP_LENGTH_MASK);

	/* if no dummy mapping, get address of mapped object */
	if (mapIndex > 0x7u) {

            /* index value exceeds the physical limitations */
            curObj = searchObj(mapIndex CO_COMMA_LINE_PARA);

	    /* get real mapping entry */
	    if (getObjPtrAddr(curObj, mapIndex, mapSubIndex, &pMap->pAddress,
		    &size CO_COMMA_LINE_PARA) != CO_OK)  {
		return(CO_E_NOT_EXIST);
	    }
# ifdef CONFIG_VIRTUAL_OBJECTS
            /* if I get an valid adress to a curObj which is NULL it must be virtual */
            if ( NULL == curObj ) {
                pPdo->virtualObjFlags = PDOFLAG_VIRTUAL_OBJ_TRUE;
                pMap->pAddress = NULL; /* This is our indicator for virtual map */
            }
# endif /* CONFIG_VIRTUAL_OBJECTS */

# ifdef CO_CONFIG_ENABLE_OBJ_CALLBACK
	    pMap->objIndex = mapIndex;
	    pMap->objSubindex = mapSubIndex;
            pMap->cbServiceNum = pPdo->pdoNr;
            pMap->ppObjCallback = getObjPtrFuncPtrAddr(curObj, mapIndex CO_COMMA_LINE_PARA);
# endif /* CO_CONFIG_ENABLE_OBJ_CALLBACK */
	    /* set object type */
	    /* is this a numeric value */
# ifdef CONFIG_16BIT_CPU
	    {
	    BASIC_DATA_T dataType;

		dataType = getObjBasicDataType(mapIndex, mapSubIndex CO_COMMA_LINE_PARA);
		if (dataType == CO_INTEGER)  {
		    pMap->eBasicType = CO_INTEGER;
		} else
		if (dataType == CO_UNSIGNED)  {
		    pMap->eBasicType = CO_UNSIGNED;
		} else {
		    pMap->eBasicType = CO_STRING;
		}
	    }
# else /* CONFIG_16BIT_CPU */
            {
            UNSIGNED16 attribute = 0;

	        (void)getObjPtrAttr(curObj, mapIndex, mapSubIndex, &attribute CO_COMMA_LINE_PARA);
	        if ((attribute	& CO_NUM_VAL) == CO_NUM_VAL)  {
		    pMap->eBasicType = CO_UNSIGNED;
	        } else {
		    pMap->eBasicType = CO_STRING;
	        }
            }
# endif /* CONFIG_16BIT_CPU */

	} else {/* dummy mapping objects 1 - 7 */

	    /* dummy mapping only for receive PDOs */
	    pMap->eBasicType = CO_DUMMY_SPACE;
	    pMap->pAddress = NULL;
# ifdef CO_CONFIG_ENABLE_OBJ_CALLBACK
	    pMap->objIndex = 0;
	    pMap->objSubindex = 0;
            pMap->ppObjCallback = NULL;
# endif /* CO_CONFIG_ENABLE_OBJ_CALLBACK */
	}

	pMap->bBitSize = mapLength;

# ifdef CONFIG_BIT_ENCODING
	mappedBits += mapLength;
# else /* CONFIG_BIT_ENCODING */
	/* attention - counting bytes here ! */
	mappedBits += ((mapLength >> 3u) + ((mapLength % 8u) ? 1u : 0u));
# endif /* CONFIG_BIT_ENCODING */
	/* next entry */
	pMap ++;

    }

    pPdo->actMapCnt = mappingCnt;

    /* check for valid data count */
    if (mappingCnt != 0u)  {
# ifdef CONFIG_BIT_ENCODING
	pPdo->pCOB->bLength = (mappedBits >> 3) + ((mappedBits % 8) ? 1 : 0);
# else /* CONFIG_BIT_ENCODING */
	pPdo->pCOB->bLength = mappedBits;
# endif /* CONFIG_BIT_ENCODING */
    } else {
	pPdo->pCOB->bLength = 0u;
    }

    /* check for valid mapping length */
    if ( pPdo->pCOB->bLength > CO_CONFIG_MAX_PDO_MAP_BYTES )  {
	return(CO_E_MAP);
    }

# if defined(CONFIG_MPDO_DEST) || defined(CONFIG_MPDO_SRC)
    if ((pPdo->mpdoFlags & MPDOFLAG_DEST) || (pPdo->mpdoFlags & MPDOFLAG_SRC)) {
	/* MPDO has always length 8 */
	pPdo->pCOB->bLength = 8;
    }
# endif /* defined(CONFIG_MPDO_DEST) && defined(CONFIG_PDO_PRODUCER) */

# ifdef CONFIG_REDUNDANCY_SUPPORT
    pPdo->pCOB->pNextLine->bLength = pPdo->pCOB->bLength;
# endif /* CONFIG_REDUNDANCY_SUPPORT */

    if (pPdo->pCOB->bLength != 0u)  {
	pPdo->flags &= (FLAG_T)~PDOFLAG_MAP_DISABLED;
    }

    return(CO_OK);
}
#endif /* defined(CONFIG_PDO_CONSUMER) || defined(CONFIG_PDO_PRODUCER) */

#if defined(CONFIG_PDO_CONSUMER) || defined(CONFIG_PDO_PRODUCER)
/****************************************************************************/
/**
*++ \brief definePdo - define PDO properties
*-- \brief definePdo - definiert PDO Eigenschaften
*
*++ This function defines a Process Data Object (PDO) with its usable
*++ properties.
*-- Diese Funktion definiert ein Process Data Object (PDO)
*-- mit den im Objektverzeichnis hinterlegten Eigenschaften.
*
*++ All necessary parameters for creating a PDO besides the COB-ID
*++ are read from the
*++ corresponding object dictionary entries (0x1400 - 0x1BFF).
*++ For the first four RPDO/TPDO pairs the resulting COB-IDs will be
*++ computed from the Node ID according to DS301.
*-- Alle notwendigen Parameter
*-- werden aus den korrespondierenden Objektverzeichniseinträgen
*-- (0x1400 - 0x1BFF) gelesen.
*-- Für die ersten vier TPDOs und RPDOs werden die Standard COB-IDs
*-- nach CiA DS 301 beim Reset des OV eingetragen.
*
* \code
* 1st  RPDO      0x200 + node ID
* 2nd  RPDO      0x300 + node ID
* 3rd  RPDO      0x400 + node ID
* 4th  RPDO      0x500 + node ID
* 1st  TPDO      0x180 + node ID
* 2nd  TPDO      0x280 + node ID
* 3rd  TPDO      0x380 + node ID
* 4th  TPDO      0x480 + node ID
* \endcode
*
*++ All other PDO COB-IDs are set to be invalid.
*-- Die COB-IDs aller weiteren PDOs werden als ungültig
*-- gekennzeichnet und sind damit nicht aktiv.
*++ After 'defining' they are disabled.
*++ To change their COB-ID the object dictionary must be modified and
*-- Um die COB-IDs zu ändern kann die Funktion
* \em setCobId()
*++ has to be called in order to set the internal values.
*-- genutzt werden,
*-- die gleichzeitig die internen Bibliotheksvariablen aktualisiert.
*
* \code
* definePdo(RECEIVE_PDO, 6, CO_TRUE);  // define PDO
* cobId = PDO_NO_VALID_BIT;            // disable PDO
* setCobId(0x1405, 1, cobId);          // set new Value to OD
* nr = 0;                              // mapping count
* putObj(0x1605, 0, &nr, 1, CO_TRUE);  // disable mapping
* setCommPar(0x1605, 0);               // set internal values
* mapping = 0x20000120;                // new Mapping
*                                      // (Index 2000, Subindex 0, Length 32 bit)
* putObj(0x1605, 1, &mapping, 4, CO_TRUE);  // set new Mapping to OD
* nr = 1;                              // new number of mappings
* putObj(0x1605, 0, &nr, 1, CO_TRUE);  // set new Value to OD
* setCommPar(0x1605, 0);               // set internal values
* cobId = 600;                         // set new cobId and enable PDO
* setCobId(0x1405, 1, cobId);          // set new Value to OD
* \endcode
*
* \note
*++ if your CAN controller doesn't support RTR-Request
*++ or the define \c ONLY_ONE_TRANSMIT_CHANNEL has been set,
*++ the bit \c PDO_NO_RTR_ALLOWED_BIT has to be set for each COB-Id.
*++ Otherwise the function returns error!
*-- Wenn der CAN-Controller kein RTR unterstützt
*-- oder das define \c ONLY_ONE_TRANSMIT_CHANNEL gesetzt ist,
*-- muss das Bit \c PDO_NO_RTR_ALLOWED_BIT bei jeder COB-Id gesetzt werden,
*-- ansonsten kehrt die Funktion mit einer Fehlermeldung zurück.
*
* \retval CO_OK
*++ success
*-- Erfolg
* \retval CO_E_MEM
*++ memory allocation fault
*-- Speicherzuweisungsfehler
* \retval CO_E_NO_ACCESS
*++ no access to object dictionary (PDO parameter, Node Id)
*-- Kein Zugriff auf Objektverzeichnis (PDO parameter, Node Id) möglich
* \retval CO_E_RANGE
*++ COB_ID is outside of the valid limit (1 - 1760)
*-- COB_ID ist außer des gültigen Bereiches (1 - 1760)
* \retval CO_E_MAP
*++ mapping error, e.g. mapping not possible
*-- Mappingfehler, z.B. Mapping nicht möglich
* \retval CO_E_TRANS_TYPE
*++ bad transmission type
*-- Eingestellter Transmission Typ nicht möglich
* \retval CO_E_NO_DATABASE
*++ no more CAN objects available (FullCAN Mode)
*-- Keine weiteren CAN-Objekte verfügbar (FullCAN Mode)
*
*/

RET_T definePdo(
	UNSIGNED8  kind,   /**< kind of PDO RECEIVE_PDO/TRANSMIT_PDO */
	UNSIGNED16 pdoNr,  /**< number of PDO */
	BOOL_T     dynMap  /**< permission flag for dynamically PDO mapping */
	CO_COMMA_LINE_PARA_DECL	/**< number of CAN line 0..CO_MAX_CAN_LINES-1 */
     )
{
UNSIGNED8  actMapCnt;		/* max. count of mapping objects (subindex 0)*/
UNSIGNED8  maxMapValue;		/* max. possible count of mapping objects */
UNSIGNED32 size;		/* size of object */
UNSIGNED16 pdoOffs;		/* pdo index address offset */
UNSIGNED16 pdoMapBase;		/* base index of PDO mapping */
UNSIGNED16 pdoParaBase;		/* base index of PDO parameter */
UNSIGNED16 inhibitTime = 0u;	/* inhibittime */
RET_T      retVal;		/* return value */
UNSIGNED8  count;		/* count of subindicies of PDO parameter*/
UNSIGNED32 cobId;		/* COB_ID of PDO */
PDO_T	   *pPdo;		/* pointer to actual pdo structure */
FLAG_T	   f;			/* help var */
COB_KIND_T cobKind;		/* cob-id kind (transmit/receive */
LIST_ELEMENT_T *curObj;	    	/* pointer to current object for faster access */

    /* for faster access */
    pdoOffs = pdoNr - 1u;
    /* get pointer to PDO structure */
    pPdo = pdoExist(pdoNr, kind CO_COMMA_LINE_PARA);

    if (kind == TRANSMIT_PDO) {
# ifdef CONFIG_PDO_PRODUCER
	pdoMapBase = TPDO_MAP_BASE_INDEX;
	pdoParaBase = TPDO_PARA_BASE_INDEX;
	cobKind = CO_COB_PDO_PROD_RTR;

	if (pPdo == NULL)  {
	    /* not initialized, create new entry */

	    /* check for free pdo structure */
	    if (GL_ARRAY(co_trPdoCnt) >= CO_TPDO_LINE_CNTS)  {
		return(CO_E_NO_DATABASE);
	    }
	    /* set start address */
	    pPdo = &GL_PVAR(co_trPdo)[GL_ARRAY(co_trPdoCnt)
# ifdef CONFIG_MULT_LINES
		+ GL_ARRAY(co_trPdoLineOffs)
# endif /* CONFIG_MULT_LINES */
		];
	    pPdo->pdoNr = pdoNr;

	    /* incr. pdo numbers */
	    GL_ARRAY(co_trPdoCnt) ++;

#ifdef CONFIG_PDO_FAST_SORT
	    /* sort pdo number into number list */
	    sortIntoPdoNrList(
# ifdef CONFIG_MULT_LINES
		&GL_PVAR(co_trPdo)[GL_ARRAY(co_trPdoLineOffs)],
		&GL_PVAR(co_trPdoNrIdxList)[GL_ARRAY(co_trPdoLineOffs)],
# else /* CONFIG_MULT_LINES */
		&GL_PVAR(co_trPdo)[0],
		&GL_PVAR(co_trPdoNrIdxList)[0],
# endif /* CONFIG_MULT_LINES */
		GL_ARRAY(co_trPdoCnt) - 1,
		GL_ARRAY(co_trPdoCnt));
#endif /* CONFIG_PDO_FAST_SORT */
	}
# else /* CONFIG_PDO_PRODUCER */
	return(CO_E_TRANS_TYPE);
# endif /* CONFIG_PDO_PRODUCER */

    } else {

# ifdef CONFIG_PDO_CONSUMER
	pdoMapBase = RPDO_MAP_BASE_INDEX;
	pdoParaBase = RPDO_PARA_BASE_INDEX;
	cobKind = CO_COB_PDO_CONS_RTR;

	if (pPdo == NULL)  {
	    /* not initialized, create new entry */

	    /* check for free pdo structure */
	    if (GL_ARRAY(co_recPdoCnt) >= CO_RPDO_LINE_CNTS)  {
		return(CO_E_NO_DATABASE);
	    }

	    pPdo = &GL_PVAR(co_recPdo)[GL_ARRAY(co_recPdoCnt)
# ifdef CONFIG_MULT_LINES
		+ GL_ARRAY(co_recPdoLineOffs)
# endif /* CONFIG_MULT_LINES */
		];
	    pPdo->pdoNr = pdoNr;
	    GL_ARRAY(co_recPdoCnt) ++;

#ifdef CONFIG_PDO_FAST_SORT
	    /* sort pdo number into number list */
	    sortIntoPdoNrList(
# ifdef CONFIG_MULT_LINES
		&GL_PVAR(co_recPdo)[GL_ARRAY(co_recPdoLineOffs)],
		&GL_PVAR(co_recPdoNrIdxList)[GL_ARRAY(co_recPdoLineOffs)],
# else /* CONFIG_MULT_LINES */
		&GL_PVAR(co_recPdo)[0],
		&GL_PVAR(co_recPdoNrIdxList)[0],
# endif /* CONFIG_MULT_LINES */
		GL_ARRAY(co_recPdoCnt) - 1,
		GL_ARRAY(co_recPdoCnt));
#endif /* CONFIG_PDO_FAST_SORT */
	}
# else /* CONFIG_PDO_CONSUMER */
	return(CO_E_TRANS_TYPE);
# endif /* CONFIG_PDO_CONSUMER */
    }

    /* get pdoParaBaseObj */
    curObj = searchObj((UNSIGNED16)(pdoParaBase + pdoOffs) CO_COMMA_LINE_PARA);


    /* read COB-ID parameter */
    if (getObjPtrEntry(curObj ,(UNSIGNED16)(pdoParaBase + pdoOffs), 1u,
		(UNSIGNED8 *)&cobId, &size,
		CO_TRUE CO_COMMA_LINE_PARA) != CO_OK) {
	return CO_E_NO_ACCESS;
    }

    if ((cobId & PDO_NO_RTR_ALLOWED_BIT) != 0u)  {
	if (kind == TRANSMIT_PDO) {
	    cobKind = CO_COB_PDO_PROD;
	} else {
	    cobKind = CO_COB_PDO_CONS;
	}
    }

    if (pPdo->pCOB == NULL)  {
	pPdo->pCOB = DEFINE_COB(cobKind, 0u CO_COMMA_LINE_PARA);
	if (pPdo->pCOB == NULL)  {
	    return(CO_E_NO_DATABASE);
	}
    }

    /* reset pdo flags */
    pPdo->flags = PDOFLAG_DISABLED;

# if defined(CONFIG_SYNC_CONSUMER) || defined(CONFIG_SYNC_PRODUCER)
    pPdo->flags |= PDOFLAG_SYNC_POSSIBLE;
# endif /* defined(CONFIG_SYNC_CONSUMER) || defined(CONFIG_SYNC_PRODUCER) */

# if defined(CONFIG_MPDO_DEST) || defined(CONFIG_MPDO_SRC)
    pPdo->mpdoFlags = 0;
# endif /*  defined(CONFIG_MPDO_DEST) || defined(CONFIG_MPDO_SRC) */

# ifdef CONFIG_VIRTUAL_OBJECTS
    pPdo->virtualObjFlags = PDOFLAG_VIRTUAL_OBJ_FALSE;
#endif /* CONFIG_VIRTUAL_OBJECTS */

    /* set cob-id */
    retVal = setPdoCobId(pPdo, cobId, kind CO_COMMA_LINE_PARA);
    if (retVal != CO_OK) {
	return(retVal);
    }

    /* get transmission type */
    if (getObjPtrEntry(curObj, (UNSIGNED16)(pdoParaBase + pdoOffs), 2u, &pPdo->transType,
		&size, CO_TRUE CO_COMMA_LINE_PARA)
	    != CO_OK) {
	return CO_E_NO_ACCESS;
    }

    /* set transmission type */
    retVal = setPdoTransType(pPdo, kind, pPdo->transType CO_COMMA_LINE_PARA);
    if (retVal != CO_OK)  {
	return(retVal);
    }

    /* get number of sub indicies of PDO parameter */
    if (getObjPtrEntry(curObj, (UNSIGNED16)(pdoParaBase + pdoOffs), 0u, &count, &size,
	    CO_TRUE CO_COMMA_LINE_PARA) != CO_OK) {
	return CO_E_NO_ACCESS;
    }

    /* get optional entry inhibit time */
    if (count >= 3u) {
	if (getObjPtrEntry(curObj, (UNSIGNED16)(pdoParaBase + pdoOffs), 3u,
		(UNSIGNED8 *)&inhibitTime, &size, CO_TRUE
		CO_COMMA_LINE_PARA)
			!= CO_OK) {
	    return CO_E_NO_ACCESS;
	}
    }

    f = pPdo->flags;
    pPdo->flags |= PDOFLAG_DISABLED;
    retVal = setPdoInhibitTime(pPdo, inhibitTime CO_COMMA_LINE_PARA);
    if (retVal != CO_OK)  {
	return(retVal);
    }
    pPdo->flags = f;
    pPdo->inhibit.ticks = 0u;

# ifdef CO_CONFIG_PDO_INHIBITTIME_RESEND
    pPdo->inhibit.pPdo = (UNSIGNED8* ) pPdo;
# endif /* CO_CONFIG_PDO_INHIBITTIME_RESEND */

# ifdef CONFIG_PDO_EVENTTIMER
    /* get optional entry event timer */
    if (count > 4) {
	/* entry for event timer exist */
	UNSIGNED16	tmpU16;

	if (getObjPtrEntry(curObj, (UNSIGNED16)pdoParaBase + pdoOffs, 5,
		(UNSIGNED8 *)&tmpU16, &size,
		CO_TRUE CO_COMMA_LINE_PARA)
	      != CO_OK) {
	    return CO_E_NO_ACCESS;
	}

	retVal = setPdoEventTime(pPdo, kind, tmpU16 CO_COMMA_LINE_PARA);
	if (retVal != CO_OK)  {
	    return(retVal);
	}
    }
# endif /* CONFIG_PDO_EVENTTIMER */

# ifdef CONFIG_PDO_SYNC_START_VALUE
    /* sync start values are only processed for transmit PDOs */
    if (kind == TRANSMIT_PDO)  {
	UNSIGNED8	tmpU8 = 0;

	/* get optional entry sync start value */
	if (count > 5) {
	    /* entry for sync start exist */

	    if (getObjPtrEntry(curObj, (UNSIGNED16)pdoParaBase + pdoOffs, 6,
		    &tmpU8, &size, CO_TRUE CO_COMMA_LINE_PARA)
		  != CO_OK) {
		return CO_E_NO_ACCESS;
	    }
	}

	f = pPdo->flags;
	pPdo->flags |= PDOFLAG_DISABLED;
	retVal = setPdoSyncStartVal(pPdo, tmpU8 CO_COMMA_LINE_PARA);
	pPdo->flags = f;
	if (retVal != CO_OK)  {
	    return(retVal);
	}
    }
# endif /* CONFIG_PDO_SYNC_START_VALUE */

    /* get number of mapping objects */
    if (getObjEntry((UNSIGNED16)(pdoMapBase + pdoOffs), 0u, &actMapCnt, &size,
		CO_TRUE CO_COMMA_LINE_PARA) != CO_OK) {
	return CO_E_MAP;
    }

# if defined(CONFIG_MPDO_DEST)
    /* MPDO destination mode is signed as 255 */
    if (actMapCnt == 255)  {
	/* MPDO dest. mode */
	if (kind == TRANSMIT_PDO) {
	    pPdo->mpdoFlags = MPDOFLAG_DEST_PRODUCER;
	    actMapCnt = 1;
	} else {
	    pPdo->mpdoFlags = MPDOFLAG_DEST_CONSUMER;
	    actMapCnt = 0;
	}
    }
# endif /* CONFIG_MPDO_DEST_PRODUCER */

# if defined(CONFIG_MPDO_SRC)
    /* MPDO source mode is signed as 254 */
    if (actMapCnt == 254)  {
	/* test the transmission type - only 254 or 255 ar allowed */
	if ((pPdo->transType != 254) && (pPdo->transType != 255)) {
	    return CO_E_TRANS_TYPE;
	}

	/* MPDO src mode */
#  if defined(CONFIG_PDO_PRODUCER)
	if (kind == TRANSMIT_PDO) {
	    pPdo->mpdoFlags = MPDOFLAG_SRC_PRODUCER;
	    /* test the mapping entries at the object scanner list */
	    retVal = testMPdoScannerList(CO_LINE_PARA);
	    if (retVal != CO_OK)  {
		return(retVal);
	    }
	} else {
#  endif /* defined(CONFIG_PDO_PRODUCER) */
#  if defined(CONFIG_PDO_CONSUMER)
	    pPdo->mpdoFlags = MPDOFLAG_SRC_CONSUMER;
	    /* test the mapping entries at the dispatcher list */
	    retVal = testMPdoDispatcherList(CO_LINE_PARA);
	    if (retVal != CO_OK)  {
		return(retVal);
	    }
#  endif /* defined(CONFIG_PDO_CONSUMER) */
#  if defined(CONFIG_PDO_PRODUCER)
	}
#  endif /* defined(CONFIG_PDO_PRODUCER) */

/* =========================== FIXME ================================ */
	/* increment counter for current mappings */
#  if defined(CONFIG_PDO_CONSUMER)
	actMapCnt = 0;
#  endif /* defined(CONFIG_PDO_CONSUMER) */
#  if defined(CONFIG_PDO_PRODUCER)
	actMapCnt = 1;            /* for now, only one possible mapping for
				     SAM MPDO */
#  endif /* defined(CONFIG_PDO_PRODUCER) */
/* =========================== FIXME ================================ */




    }
# endif /* CONFIG_MPDO_SRC */

    maxMapValue = actMapCnt;

    if (pPdo->maxMapCnt == 0u)  {
	/* Mapping wurde noch nicht eingerichtet */

	if (dynMap == CO_TRUE) {
#  ifdef CONFIG_DYN_PDO_MAPPING
	    /* get max. number of mapping objects */
	    /* numOfElemnts are mappings + subindex 0 */
	    maxMapValue = getNumOfElem(pdoMapBase + pdoOffs
		    CO_COMMA_LINE_PARA) - 1;
	} else {  /* no dynamic mapping */
	    UNSIGNED16	i;

	    /* set objectattribute from subindex 0 to readonly */
	    i = getObjAttr(pdoMapBase + pdoOffs, 0 CO_COMMA_LINE_PARA);
	    if ((i & CO_WRITE_PERM) != 0)  {
		i &= (UNSIGNED8)~CO_WRITE_PERM;
		if (setObjAttr(pdoMapBase + pdoOffs, 0, i CO_COMMA_LINE_PARA)
			!= CO_TRUE)  {
		    return(CO_E_NO_ACCESS);
		}
	    }
#  else /* CONFIG_DYN_PDO_MAPPING */
	    return(CO_E_MAP);	/* ! CONFIG_DYN_PDO_MAPPING */
#  endif /* CONFIG_DYN_PDO_MAPPING */
	}

	/* check for enough free mapping entries */
	if ((GL_VAR(co_mappingCnt) + maxMapValue) > co_maxMappingCnt)  {
	    return(CO_E_MAP);
	}
	pPdo->mapStartIdx = GL_VAR(co_mappingCnt); /* start index at mapping table */
	pPdo->actMapCnt = maxMapValue;		/* actual mapping count */
	pPdo->maxMapCnt = maxMapValue;		/* max. mapping count */
	GL_VAR(co_mappingCnt) += maxMapValue;
    }

    /* check Mapping */
    retVal = checkMappingTable(pPdo, pdoMapBase + pdoOffs, kind
		CO_COMMA_LINE_PARA);
    return(retVal);
}
#endif /* defined(CONFIG_PDO_CONSUMER) || defined(CONFIG_PDO_PRODUCER) */


#if defined(CONFIG_PDO_CONSUMER) || defined(CONFIG_PDO_PRODUCER)
/*******************************************************************/
/*
* setPdoCommPar - set pdo communication parameter
*
* \internal
*
* This function sets communication parameter for pdos
*
* \retval CO_OK
*++ success
*-- Erfolg
* \retval CO_E_NOT_EXIST
* internal communication object doesn't exist
* \retval CO_E_RANGE
* COB-ID is out of the range (1..1760)
* \retval CO_E_TRANS_TYPE
* bad transtype
*
*/
RET_T setPdoCommPara(
	UNSIGNED16	index,		/* index */
	UNSIGNED8	subIndex,	/* subindex */
	UNSIGNED8	*pData,		/* pointer to object data */
	UNSIGNED8	kind		/* kind of PDO RECEIVE/TRANSMIT */
	CO_COMMA_LINE_PARA_DECL	/* number of CAN line 0..CO_MAX_CAN_LINES-1 */
    )
{
UNSIGNED16	nr;			/* pdo number */
PDO_T		*pPdo;			/* pointer to actual pdo */
RET_T		ret = CO_E_NOT_EXIST;	/* return value */

    /* get pdo structure */
    if (kind == RECEIVE_PDO)  {
	nr = (index - RPDO_PARA_BASE_INDEX) + 1u;
	pPdo = pdoExist(nr, RECEIVE_PDO CO_COMMA_LINE_PARA);
	if (pPdo == NULL)  {
	     return(CO_E_NOT_EXIST);
	}
    } else {
	nr = (index - TPDO_PARA_BASE_INDEX) + 1u;
	pPdo = pdoExist(nr, TRANSMIT_PDO CO_COMMA_LINE_PARA);
	if (pPdo == NULL)  {
	     return(CO_E_NOT_EXIST);
	}
    }

    switch (subIndex) {
	case 1:				/* cob-id */
	    ret = setPdoCobId(pPdo, *((UNSIGNED32 *)pData), kind
		CO_COMMA_LINE_PARA);
	    break;

	case 2:				/* transmission type */
	    ret = setPdoTransType(pPdo, kind, *pData CO_COMMA_LINE_PARA);
	    break;

	case 3:				/* inhibit time */
	    ret = setPdoInhibitTime(pPdo, *((UNSIGNED16 *)pData)
		CO_COMMA_LINE_PARA);
	    break;

	case 4:				/* compatibility entry */
	    ret = CO_E_NONEXIST_SUBINDEX;
	    break;

# ifdef CONFIG_PDO_EVENTTIMER
	case 5:				/* event timer */
	    ret = setPdoEventTime(pPdo, kind, *((UNSIGNED16 *)pData)
		CO_COMMA_LINE_PARA);
	    break;
# endif /* CONFIG_PDO_EVENTTIMER */

# ifdef CONFIG_PDO_SYNC_START_VALUE
	case 6:				/* sync start value */
	    if (kind == RECEIVE_PDO)  {
		ret = CO_E_NONEXIST_SUBINDEX;
	    } else {
		ret = setPdoSyncStartVal(pPdo, *pData CO_COMMA_LINE_PARA);
	    }
	    break;
# endif /* CONFIG_PDO_SYNC_START_VALUE */
	default:
		break;
    }
    return(ret);
}
#endif /* defined(CONFIG_PDO_CONSUMER) || defined(CONFIG_PDO_PRODUCER) */



#ifdef CONFIG_PDO_PRODUCER
/*******************************************************************
*
*++ prepareTransPdo - prepared a PDO for transmitting to the client(s)
*-- prepareTransPdo - bereitet ein PDO zur Sendung a die Client(s) vor
*
* \internal
*
*++ The function writes a PDO to the given transmit buffer.
*++ With synchronous PDOs the PDOs sync-buffer values will be updated
*++ and they are transmitted with the following SYNC object.
*-- Diese Funktion schreibt ein PDO in den übergebenen Sendepuffer.
*-- Bei synchronen PDOs werden nur die Daten des Schattenpuffers aktualisiert,
*-- die mit dem nächsten gültigen SYNC gesendet werden.
*
*++ This service is only available in the node state OPERATIONAL.
*-- Dieser Dienst ist nur im Zustand OPERATIONAL verfügbar.
*
* \retval CO_OK
*++ success
*-- Erfolg
* \retval CO_E_STATE
*++ node isn't in state OPERATIONAL
*-- Knoten ist nicht im Zustand OPERATIONAL
* \retval CO_E_DISABLED
*++ PDO is disabled
*-- PDO ist disabled
* \retval CO_E_MAP
*++ mapping incorrect
*-- Mapping Fehler
*
*/

RET_T prepareTransPdo(
	PDO_T		*pPdo,	/* Pointer to PDO Data */
	UNSIGNED8	*pBuf	/* Sende Puffer */
	CO_COMMA_LINE_PARA_DECL	/* number of CAN line 0..CO_MAX_CAN_LINES-1 */
    )
{
UNSIGNED8 mapLen;
# ifdef CONFIG_VIRTUAL_OBJECTS_PDO
RET_T ret;            /* CANopen return value */
    ret = CO_OK;
# endif /* CONFIG_VIRTUAL_OBJECTS_PDO */


# ifdef CONFIG_REDUNDANCY_SUPPORT
# else /* CONFIG_REDUNDANCY_SUPPORT */
    if (GL_ARRAY(co_Node).eState != OPERATIONAL) {
	return(CO_E_STATE);
    }
# endif /* CONFIG_REDUNDANCY_SUPPORT */

    /* if asynchronous PDO disabled do nothing */
    if ((pPdo->flags & PDOFLAG_DISABLED) != 0) {
	return(CO_E_DISABLED);
    }

# if (defined(CONFIG_SYNC_CONSUMER) || defined(CONFIG_SYNC_PRODUCER)) && defined(CONFIG_PDO_PRODUCER)
    /* for synchronous PDO set the shadow buffer */
    if ((pPdo->flags & PDOFLAG_SYNC) != 0)  {
	pBuf = pPdo->shadowData;
    }
# endif /* (defined(CONFIG_SYNC_CONSUMER) || defined(CONFIG_SYNC_PRODUCER)) && defined(CONFIG_PDO_PRODUCER) */

    /* init buffer */
    memset(pBuf, (int)0, (size_t)CO_CONFIG_MAX_PDO_MAP_BYTES );

# ifdef CONFIG_VIRTUAL_OBJECTS_PDO
    /* if the pdo contains virtual objects, pass the buffer to the application */
    if ( PDOFLAG_VIRTUAL_OBJ_TRUE == pPdo->virtualObjFlags ) {
        ret = coUserVirtualTpdoInd(pPdo->pdoNr, pBuf CO_COMMA_LINE_PARA);
        if (ret != CO_OK)
        {
            return (ret);
        }
    }
# endif /* CONFIG_VIRTUAL_OBJECTS_PDO */

    /* allocate security mechanism for object dictionary consistency */
    CO_COM_PART_ALLOC(CO_LINE_PARA);
    CO_APPL_PART_ALLOC(CO_LINE_PARA);

    mapLen = CMS_MapEncode(&GL_PVAR(co_mappingTable)[pPdo->mapStartIdx], pBuf,
	    pPdo->actMapCnt CO_COMMA_LINE_PARA);

    /* release security mechanism for object dictionary consistency */
    CO_COM_PART_RELEASE(CO_LINE_PARA);
    CO_APPL_PART_RELEASE(CO_LINE_PARA);

    if (mapLen > CO_CONFIG_MAX_PDO_MAP_BYTES)  {
	return (CO_E_MAP);
    }

    /* Test if any object mapped, since in CiA301 an PDO should have a least one byte */
    if (mapLen == 0u) {
        return (CO_E_MAP);
    }

    return(CO_OK);
}
#endif /* CONFIG_PDO_PRODUCER */

#if (defined(CONFIG_SYNC_CONSUMER) || defined(CONFIG_SYNC_PRODUCER)) && defined(CONFIG_PDO_PRODUCER)
/*******************************************************************
*
*++ updateSyncTpdo - update all synchronous pdos after received the sync
*-- updateSyncTpdo - update alle synchronen pdos nach dem sync Empfang
*
*++ The function updates the new PDO data to the transmit buffer.
*++ they will be transmitted with the following SYNC object.
*-- Diese Funktion aktualisiert die Sendepuffer der zyklischen PDOs
*-- Diese werden mit dem nächsten SYNC gesendet.
*
*++ This service is only available in the node state OPERATIONAL.
*-- Dieser Dienst ist nur im Zustand OPERATIONAL verfügbar.
*
* \retval CO_OK
*++ success
*-- Erfolg
* \retval CO_E_STATE
*++ node isn't in state OPERATIONAL
*-- Knoten ist nicht im Zustand OPERATIONAL
*
*/

RET_T updateSyncTpdo(
	CO_LINE_PARA_DECL
    )
{
PDO_T	*pPdo;			/* pointer to actual pdo */
UNSIGNED16	cnt;

# ifdef CONFIG_REDUNDANCY_SUPPORT
# else /* CONFIG_REDUNDANCY_SUPPORT */
    if (GL_ARRAY(co_Node).eState != OPERATIONAL) {
	return(CO_E_STATE);
    }
# endif /* CONFIG_REDUNDANCY_SUPPORT */

    cnt = 0;
    while (cnt < GL_ARRAY(co_trPdoCnt))  {
	pPdo = &GL_PVAR(co_trPdo)[cnt
# ifdef CONFIG_MULT_LINES
		+ GL_ARRAY(co_trPdoLineOffs)
# endif /* CONFIG_MULT_LINES */
	    ];
	cnt++;

        /* PDO enabled and synchronous and cyclic */
        if ((pPdo->flags & (PDOFLAG_DISABLED | PDOFLAG_SYNC | PDOFLAG_CYCLIC))
			 == (PDOFLAG_SYNC | PDOFLAG_CYCLIC)) {

# ifdef CONFIG_PDO_SYNC_START_VALUE
	    /* handle with sync counter ? */
	    if ((pPdo->syncFlags & PDOSYNCFLAG_ENABLED) != 0)  {
		/* yes */
		/* check for actual sync counter */
		if (GL_ARRAY(co_syncCnt) == pPdo->syncStartValue) {
		    /* yes, value is reached, send PDO immediately */
		    pPdo->curCount = 1;
		    pPdo->syncFlags &= (FLAG_T)~PDOSYNCFLAG_SYNCSTART;
		} else {
		    /* waiting for first sync value ? */
		    if ((pPdo->syncFlags & PDOSYNCFLAG_SYNCSTART) != 0) {
			/* yes, no transmit allowed */
			pPdo->curCount = 2;
		    }
		}
	    }
# endif /* CONFIG_PDO_SYNC_START_VALUE */

	    pPdo->curCount--;

	    if (pPdo->curCount == 0)  {
		pPdo->curCount = pPdo->transType;

		CO_PRE_SYNC_MAP_USER_FCT

		/* update the buffer -
		 * test for valid count was tested at mapset */
		(void)prepareTransPdo(pPdo, pPdo->shadowData CO_COMMA_LINE_PARA);

		pPdo->flags |= PDOFLAG_TOTRANSMIT;
	    }
	}
    }

    return(CO_OK);
}
#endif /* (defined(CONFIG_SYNC_CONSUMER) || defined(CONFIG_SYNC_PRODUCER)) && defined(CONFIG_PDO_PRODUCER) */


#ifdef CONFIG_PDO_PRODUCER
/****************************************************************************/
/**
*++ \brief writePdoReq - transmit a PDO to the client(s)
*-- \brief writePdoReq - sendet eine PDO zu den Client(s)
*
*++ The function writes a PDO to a transmit buffer.
*++ Asynchronous PDOs are transmitted immediately.
*++ With synchronous PDOs the PDOs sync-buffer values will be updated
*++ and they are transmitted with the following valid SYNC object.
*-- Diese Funktion schreibt ein PDO in den Sendepuffer.
*-- Asynchrone PDOs werden sofort gesendet.
*-- Bei synchronen PDOs werden nur die Daten des Schattenpuffers aktualisiert,
*-- die mit dem nächsten gültigen SYNC gesendet werden.
*
*++ This service is only available in the node state OPERATIONAL.
*-- Dieser Dienst ist nur im Zustand OPERATIONAL verfügbar.
*
*++ The paramter is only the PDO number to send.
*++ The function is looking in the mapping list to determine
*++ which entries of the object dictionary it has
*++ copy into the transmit COB.
*-- Als Parameter ist die PDO Nummer zu übergeben.
*-- Die Funktion stellt damit das erforderliche Mapping
*-- anhand der Einträge im Objektverzeichnis zusammen
*-- und versendet die PDO-Nachricht.
*
* \retval CO_OK
*++ success
*-- Erfolg
* \retval CO_E_NOT_EXIST
*++ PDO doesn't exists
*-- PDO existiert nicht
* \retval CO_E_STATE
*++ node isn't in state OPERATIONAL
*-- Knoten ist nicht im Zustand OPERATIONAL
* \retval CO_E_INHIBIT
*++ inhibit time is still valid
*-- Sperrzeit ist noch gültig
* \retval CO_E_DISABLED
*++ PDO is disabled
*-- PDO ist disabled
* \retval CO_E_TYPE
*++ bad transmission type (MPDO usage)
*-- Falscher Transmission Type (bei MPDO)
* \retval CO_E_MAP
*++ invalid mapping
*-- Mapping ungültig
*
*/

RET_T writePdoReq(
	UNSIGNED16 pdoNr	/**< number of Transmit PDO */
	CO_COMMA_LINE_PARA_DECL	/**< number of CAN line 0..CO_MAX_CAN_LINES-1 */
    )
{
UNSIGNED8  pData[8];		/* mapping transmit buffer */
PDO_T	  *pPdo;		/* pointer to actual pdo structure */
RET_T	   ret;			/* return value */

    pPdo = pdoExist(pdoNr, TRANSMIT_PDO CO_COMMA_LINE_PARA);
    if (pPdo == NULL) {
	return(CO_E_NOT_EXIST);
    }

    if ((pPdo->flags & PDOFLAG_ONLY_RTR) != 0) {
	return(CO_E_NOT_EXIST);
    }

# ifdef CONFIG_MPDO_DEST
    if ((pPdo->mpdoFlags & MPDOFLAG_DEST_PRODUCER) != 0) {
	/* mpdo has to send by writeMPdoReq() */
	return(CO_E_TYPE);
    }
# endif /* CONFIG_MPDO_DEST_PRODUCER */
# ifdef CONFIG_MPDO_SRC
    if ((pPdo->mpdoFlags & MPDOFLAG_SRC_PRODUCER) != 0) {
	/* mpdo has to send by writeMPdoReq() */
	return(CO_E_TYPE);
    }
# endif /* CONFIG_MPDO_DEST_PRODUCER */

    /* only asynchronous PDOs are to transmit immediately
	synchronous acyclic have to be marked for sending */
# if defined(CONFIG_SYNC_CONSUMER) || defined(CONFIG_SYNC_PRODUCER)
    if ((pPdo->flags & PDOFLAG_SYNC) != 0) {
	ret = prepareTransPdo(pPdo, pPdo->shadowData CO_COMMA_LINE_PARA);
	if (ret != CO_OK)  {
	    return(ret);
	}
	if ((pPdo->flags & PDOFLAG_CYCLIC) == 0)  {
	    pPdo->flags |= PDOFLAG_TOTRANSMIT;
	}
    } else
# endif /* defined(CONFIG_SYNC_CONSUMER) || defined(CONFIG_SYNC_PRODUCER) */
    {
	ret = prepareTransPdo(pPdo, pData CO_COMMA_LINE_PARA);
	if (ret != CO_OK)  {
	    return(ret);
	}
	/* if inhibit timer is running */
	if (pPdo->inhibit.ticks > 0) {
# ifdef CO_CONFIG_PDO_INHIBITTIME_RESEND
            pPdo->inhibitFlags |= PDO_INHIBIT_FLAG_RETRANSMIT;
            /* printf("PDO inhibited. ticks = %u \n", pPdo->inhibit.ticks); */
# endif /* CO_CONFIG_PDO_INHIBITTIME_RESEND */
	    return(CO_E_INHIBITED);
	}
	ret = TRANSMIT_COB(pPdo->pCOB, pData);

# ifdef CO_CONFIG_PDO_INHIBITTIME_RESEND
        pPdo->inhibitFlags &= ~PDO_INHIBIT_FLAG_RETRANSMIT;
# endif /* CO_CONFIG_PDO_INHIBITTIME_RESEND */

# ifdef CO_CONFIG_PDO_SEND_IND
        sendPdoInd( pdoNr, SEND_PDO_IND_ACYC, ret CO_COMMA_LINE_PARA );
# endif /*CO_CONFIG_PDO_SEND_IND*/
	if (ret != CO_OK)  {
	    return(ret);
	}
	if (pPdo->wInhibitTime > 0)  {
	    startInhibitTimer(&pPdo->inhibit, pPdo->wInhibitTime
		CO_COMMA_LINE_PARA);
	}

# ifdef CONFIG_PDO_EVENTTIMER
	/* start event timer */
	if (pPdo->timer.timerVal != 0)  {
	    (void)addTimerEvent(&pPdo->timer, pPdo->timer.timerVal,
		CO_TIMER_TYPE_EVENTTPDO | CO_TIMER_TYPE_CYCLIC
		CO_COMMA_LINE_PARA);
	}
# endif /* CONFIG_PDO_EVENTTIMER */
    }

    return(CO_OK);
}


/****************************************************************************/
/**
*++ \brief updatePdoReq - update a PDO at CAN controller
*-- \brief updatePdoReq - aktualisiseren eines PDO im CAN Controller
*
*++ The function updates only a PDO at the CAN transmit buffer.
*++ It is used for RTR only PDOs to update the CAN controller.
*++ This is only required for FullCAN controllers.
*-- Diese Funktion aktualisiert die Daten eines PDO im CAN-Controller.
*-- Sie wird für RTR only PDOs verwendet,
*-- um den CAN Controller zu aktualisieren.
*-- Das ist notwendig, wenn ein FullCAN controller verwendet wird.
*
*++ This service is only available in the node state OPERATIONAL.
*-- Dieser Dienst ist nur im Zustand OPERATIONAL verfügbar.
*
*++ The paramter is only the PDO number to send.
*++ The function is looking in the mapping list to determine
*++ which entries of the object dictionary it has
*++ copy into the transmit COB.
*-- Als Parameter ist dieser Funktion die PDO Nummer zu übergeben.
*-- Die Funktion erstellt das PDO anhand der Mapping-Liste.
*
* \retval CO_OK
*++ success
*-- Erfolg
* \retval CO_E_NOT_EXIST
*++ PDO doesn't exists
*-- PDO existiert nicht
* \retval CO_E_STATE
*++ node isn't in state OPERATIONAL
*-- Knoten ist nicht im Zustand OPERATIONAL
* \retval CO_E_DISABLED
*++ PDO is disabled
*-- PDO ist disabled
* \retval CO_E_MAP
*++ invalid mapping
*-- Mapping ungültig
*
*/

RET_T updatePdoReq(
	UNSIGNED16 pdoNr	/**< number of Transmit PDO */
	CO_COMMA_LINE_PARA_DECL	/**< number of CAN line 0..CO_MAX_CAN_LINES-1 */
    )
{
UNSIGNED8  pData[8];		/* mapping transmit buffer */
PDO_T	  *pPdo;		/* pointer to actual pdo structure */
RET_T	   ret;			/* return value */

    pPdo = pdoExist(pdoNr, TRANSMIT_PDO CO_COMMA_LINE_PARA);
    if (pPdo == NULL) {
	return(CO_E_NOT_EXIST);
    }

    ret = prepareTransPdo(pPdo, pData CO_COMMA_LINE_PARA);
    if (ret != CO_OK)  {
	return(ret);
    }

# if defined(CONFIG_SYNC_CONSUMER) || defined(CONFIG_SYNC_PRODUCER)
    if ((pPdo->flags & PDOFLAG_SYNC) != 0) {
	(void)prepareTransPdo(pPdo, pPdo->shadowData CO_COMMA_LINE_PARA);
    }
# endif /* defined(CONFIG_SYNC_CONSUMER) || defined(CONFIG_SYNC_PRODUCER) */

    return(CO_OK);
}
#endif /* CONFIG_PDO_PRODUCER */


#ifdef CONFIG_PDO_CONSUMER
/****************************************************************************/
/**
*++ \brief readPdoReq - request a remote transmission for a PDO
*-- \brief readPdoReq - fordert eine PDO über RTR an
*
*++ This function requests a remote transmission for a PDO.
*++ The service is available for all PDOs
*++ expect for PDO that have the NO RTR bit set in the COB-Id
*++ This function can only be used in the node state OPERATIONAL.
*-- Diese Funktion fordert ein PDO über einen RTR-Frame an.
*-- Der Dienst ist für alle PDOs verfügbar,
*-- bei denen das Non-RTR Bit in der COB-Id nicht gesetzt ist.
*-- Diese Funktion kann nur
*-- im Zustand OPERATIONAL genutzt werden.
*
*++ A special kind are the 'only RTR PDOs' (type 252 and 253).
*++ The difference between
*++ both kind of RTR is the updating of the variables.
*++ Type 252 (sychronous RTR PDO) will be updated with the SYNC.
*++ With the other type (asynchonous RTR PDO)
*++ the values will be updated if an RTR occurs.
*-- Einen Spezialfall sind die 'only RTR PDO' (type 252 and 253).
*-- Der Unterschied zwischen beiden Typen ist die Aktualisierung der Daten.
*-- Daten von sychronen RTR PDO (252) werden mit dem SYNC,
*-- Daten von asynchronen PDO (253) mit dem RTR Signal aktualisiert.
*
* \retval CO_OK
*++ success
*-- Erfolg
* \retval CO_E_NOT_EXIST
*++ PDO doesn't exists or isn't RTR
*-- PDO existiert nicht oder unterstützt kein RTR
* \retval CO_E_STATE
*++ node isn't in state OPERATIONAL
*-- Knoten ist nicht im Zustand OPERATIONAL
* \retval CO_E_INHIBIT
*++ inhibit time is still valid
*-- Sperrzeit ist noch gültig
* \retval CO_E_DISABLED
*++ PDO is disabled
*-- PDO ist disabled
*
*/

RET_T readPdoReq(
	UNSIGNED16 pdoNr	/**< number of Receive PDO */
	CO_COMMA_LINE_PARA_DECL	/**< number of CAN line 0..CO_MAX_CAN_LINES-1 */
    )
{
PDO_T	*pPdo;		/* current used PDO */
RET_T	retVal;		/* return value */

# ifdef CONFIG_REDUNDANCY_SUPPORT
# else /* CONFIG_REDUNDANCY_SUPPORT */
    if (GL_ARRAY(co_Node).eState != OPERATIONAL)  {
	return(CO_E_STATE);
    }
# endif /* CONFIG_REDUNDANCY_SUPPORT */

    pPdo = pdoExist(pdoNr, RECEIVE_PDO CO_COMMA_LINE_PARA);
    if (pPdo == NULL)  {
	return(CO_E_NOT_EXIST);
    }

    /* if not RTR or PDO disabled do nothing */
    if ((pPdo->flags & PDOFLAG_DISABLED) != 0u) {
	return(CO_E_DISABLED);
    }

    /* transmit RTR */
    retVal = TRANSMIT_COB(pPdo->pCOB, NULL);
    if (retVal != CO_OK)  {
	return(retVal);
    }

    return(CO_OK);
}
#endif /* CONFIG_PDO_CONSUMER */


#ifdef CONFIG_PDO_CONSUMER
/*******************************************************************
*
*++ pdoMsgReceived - pdo message received
*-- pdoMsgReceived - PDO message erhalten
*
* \internal
*
*++ This function edit the received pdos
*-- Diese Funktion bearbeitet die Empfangs-PDOs
*
* RETURNS
*	nothing
*
*/

void pdoMsgReceived(
	CAN_MSG_T *canMsg	    /* Pointer to CAN Message */
	CO_COMMA_REDCY_PARA_DECL
    )
{
PDO_T	 *pPdo;			/* pointer to actual pdo */
# if defined(CONFIG_MPDO_DEST)
UNSIGNED8	*pData;		/* pointer to object address */
UNSIGNED32	size;		/* object size */
RET_T		retVal;
# endif /* defined(CONFIG_MPDO_DEST) */
# if defined(CONFIG_MPDO_DEST) || defined(CONFIG_MPDO_SRC)
UNSIGNED8	tmpU8;		/* temporary U8 variable */
UNSIGNED16	tmpU16;		/* temporary U16 variable */
# endif /* defined(CONFIG_MPDO_DEST) || defined(CONFIG_MPDO_SRC) */

# ifdef CONFIG_REDUNDANCY_SUPPORT
    if (canLine == CAN_DEFAULT_LINE)  {
	if (GL_ARRAY(co_Node).eState != OPERATIONAL) {
	    return;
	}
    } else {
	if (GL_ARRAY(co_redcyNode).eState != OPERATIONAL) {
	    return;
	}
    }
# else /* CONFIG_REDUNDANCY_SUPPORT */
    if (GL_ARRAY(co_Node).eState != OPERATIONAL) {
	return;
    }
# endif /* CONFIG_REDUNDANCY_SUPPORT */

    pPdo = searchForPdoCobId(
# ifdef CONFIG_MULT_LINES
		&GL_PVAR(co_recPdo)[GL_ARRAY(co_recPdoLineOffs)],
#  ifdef CONFIG_PDO_FAST_SORT
		&GL_PVAR(co_recPdoCobIdxList)[GL_ARRAY(co_recPdoLineOffs)],
#  endif /* CONFIG_PDO_FAST_SORT */
# else /* CONFIG_MULT_LINES */
		&GL_PVAR(co_recPdo)[0],
#  ifdef CONFIG_PDO_FAST_SORT
		GL_PVAR(co_recPdoCobIdxList),
#  endif /* CONFIG_PDO_FAST_SORT */
# endif /* CONFIG_MULT_LINES */
		GL_ARRAY(co_recPdoCnt),
		canMsg->cobId);
    if (pPdo == NULL)  {
	return;
    }

    /* pdo found, test is it enabled */
    if ((pPdo->flags & PDOFLAG_DISABLED) != 0u) {
	return;
    }

# ifdef CONFIG_PDO_DATA_PTR_FCT
    GL_ARRAY(pPdoRecData) = &canMsg->pData[0];
# endif /* CONFIG_PDO_DATA_PTR_FCT */

    /* test for valid data count */
    if (canMsg->length < pPdo->pCOB->bLength)  {
# ifdef CONFIG_PDO_BAD_LEN_INDICATION
        RET_T retVal = CO_OK;
	/* inform application about bad pdo length */
	retVal = pdoLenInd(pPdo->pdoNr, PDO_LEN_TO_SHORT CO_COMMA_LINE_PARA);
# endif /* CONFIG_PDO_BAD_LEN_INDICATION */

# ifdef CO_CONFIG_V44_EMCY_PRODUCER
	/* send an emergency */
	(void)writeEmcyReq(ERRCODE_BAD_PDOPARA, NULL CO_COMMA_LINE_PARA);
# endif /* CO_CONFIG_V44_EMCY_PRODUCER */
# ifdef CONFIG_PDO_BAD_LEN_INDICATION
	if ( retVal != CO_OK ) {
            return;
        }
# else  /* CONFIG_PDO_BAD_LEN_INDICATION */
        return;
# endif /* CONFIG_PDO_BAD_LEN_INDICATION */
    }

    if (canMsg->length > pPdo->pCOB->bLength)  {
# ifdef CONFIG_PDO_BAD_LEN_INDICATION
        RET_T retVal = CO_OK;
	/* inform application about bad pdo length */
	retVal = pdoLenInd(pPdo->pdoNr, PDO_LEN_TO_LONG CO_COMMA_LINE_PARA);
# endif /* CONFIG_PDO_BAD_LEN_INDICATION */

# ifdef CO_CONFIG_V44_EMCY_PRODUCER
	/* send an emergency */
	(void)writeEmcyReq(ERRCODE_BAD_PDOLEN, NULL CO_COMMA_LINE_PARA);
# endif /* CO_CONFIG_V44_EMCY_PRODUCER */
# ifdef CONFIG_PDO_BAD_LEN_INDICATION
	if ( retVal != CO_OK ) {
            return;
        }
# endif /* CONFIG_PDO_BAD_LEN_INDICATION */
    }

# if defined(CONFIG_SYNC_CONSUMER) || defined(CONFIG_SYNC_PRODUCER)
    /* synchronous PDO */
    if ((pPdo->flags & PDOFLAG_SYNC) != 0)  {
	/* toUpdate */
	pPdo->flags |= PDOFLAG_TOUPDATE;
	pPdo->curCount = pPdo->transType;
	CO_MEMCPY(pPdo->shadowData, &canMsg->pData[0], 8);
    } else  /* asynchronous PDO */
# endif /* defined(CONFIG_SYNC_CONSUMER) || defined(CONFIG_SYNC_PRODUCER) */
    {
# if defined(CONFIG_MPDO_DEST)
	if ((pPdo->mpdoFlags & MPDOFLAG_DEST_CONSUMER) != 0)  {
	    /* src node id */
	    tmpU8 = canMsg->pData[0] & 0x7f;
	    /* test for valid node id */
	    if ((tmpU8 != 0) && (tmpU8 != GL_ARRAY(coNodeId))) {
		return;
	    }
	    /* dst index */
	    tmpU16 = (((UNSIGNED16)canMsg->pData[2]) << 8u)
		    + canMsg->pData[1];
	    /* dst subIndex */
	    tmpU8 = canMsg->pData[3];
	    /* get object size */
	    retVal = getObjAddr(tmpU16, tmpU8, &pData, &size CO_COMMA_LINE_PARA);
	    if (retVal != CO_OK)  {
		return;
	    }

# ifdef CO_CONFIG_ENABLE_OBJ_CALLBACK
#  ifdef CO_CONFIG_OBJ_CB_PRE_PDO_WRITE
        {
            CO_OBJ_CB_T callback = NULL;
            /* check if the object has an function pointer*/
            callback = getObjFuncPtr(tmpU16 CO_COMMA_LINE_PARA);
            if ( callback != NULL ) {
                CO_OBJ_CB_TYPE_T callReason;
                callReason.reason = CO_OBJ_CB_TYPE_PRE_PDO_WRITE;
                callReason.serviceNbr = pPdo->pdoNr;
#   ifdef CO_CONFIG_ENABLE_EXTOBJ_CALLBACK
				callReason.objAccess.pData = pData;
				callReason.objAccess.dataSize = size;
#   endif /* CO_CONFIG_ENABLE_EXTOBJ_CALLBACK */

                /* call the function pointer */
#   ifdef CONFIG_NO_GLOBAL_VARS
                (void)(*callback)( tmpU16, tmpU8, callReason ,(void*)CO_LINE_PARA );
#   else /* CONFIG_NO_GLOBAL_VARS */
                (void)(*callback)( tmpU16, tmpU8, callReason CO_COMMA_LINE_PARA );
#   endif /* CONFIG_NO_GLOBAL_VARS */
            }
        }
#  endif /* CO_CONFIG_OBJ_CB_PRE_PDO_WRITE */
# endif /* CO_CONFIG_ENABLE_OBJ_CALLBACK */

	    /* save data */
	    retVal = putObj(tmpU16, tmpU8, &canMsg->pData[4], size, CO_FALSE
		    CO_COMMA_LINE_PARA);
	    if (retVal != CO_OK)  {
		/* save error, return */
		return;
	    }

# ifdef CONFIG_REDUNDANCY_SUPPORT
	    /* pdo indication shall only indicated on active interface */
	    if (GL_VAR(co_redcyReceivedLine) == GL_VAR(co_redcyActiveLine))
# endif /* CONFIG_REDUNDANCY_SUPPORT */
	    {
		mpdoInd(pPdo->pdoNr, tmpU16, tmpU8 CO_COMMA_LINE_PARA);
	    }

# ifdef CO_CONFIG_ENABLE_OBJ_CALLBACK
#  ifdef CO_CONFIG_OBJ_CB_POST_PDO_WRITE
        {
            CO_OBJ_CB_T callback = NULL;
            /* check if the object has an function pointer*/
			callback = getObjFuncPtr(tmpU16 CO_COMMA_LINE_PARA);
            if ( callback != NULL ) {
                CO_OBJ_CB_TYPE_T callReason;
                callReason.reason = CO_OBJ_CB_TYPE_POST_PDO_WRITE;
                callReason.serviceNbr = pPdo->pdoNr;
#   ifdef CO_CONFIG_ENABLE_EXTOBJ_CALLBACK
				callReason.objAccess.pData = pData;
				callReason.objAccess.dataSize = size;
#   endif /* CO_CONFIG_ENABLE_EXTOBJ_CALLBACK */

				/* call the function pointer */
#   ifdef CONFIG_NO_GLOBAL_VARS
                (void)(*callback)( tmpU16, tmpU8, callReason ,(void*)CO_LINE_PARA );
#   else /* CONFIG_NO_GLOBAL_VARS */
                (void)(*callback)( tmpU16, tmpU8, callReason CO_COMMA_LINE_PARA );
#   endif /* CONFIG_NO_GLOBAL_VARS */
            }
        }
#  endif /* CO_CONFIG_OBJ_CB_POST_PDO_WRITE */
# endif /* CO_CONFIG_ENABLE_OBJ_CALLBACK */

	} else
# endif /* defined(CONFIG_MPDO_DEST) */
# if defined(CONFIG_MPDO_SRC)
	if ((pPdo->mpdoFlags & MPDOFLAG_SRC_CONSUMER) != 0) {
	    (void) mpdoSrcModeReceived(pPdo, canMsg->pData CO_COMMA_LINE_PARA);
	} else
# endif /* defined(CONFIG_MPDO_SRC) */
 	{
	    /* allocate security mechanism for object dictionary consistency */
	    CO_COM_PART_ALLOC(CO_LINE_PARA);
	    CO_APPL_PART_ALLOC(CO_LINE_PARA);

	    (void)CMS_MapDecode(&GL_PVAR(co_mappingTable)[pPdo->mapStartIdx],
		&canMsg->pData[0],
		pPdo->actMapCnt CO_COMMA_LINE_PARA);

	    /* release security mechanism for object dictionary consistency */
	    CO_COM_PART_RELEASE(CO_LINE_PARA);
	    CO_APPL_PART_RELEASE(CO_LINE_PARA);

# ifdef CONFIG_VIRTUAL_OBJECTS_PDO
            /* if the pdo contains virtual objects, pass the buffer to the application */
            if ( PDOFLAG_VIRTUAL_OBJ_TRUE == pPdo->virtualObjFlags ) {
                  (void)coUserVirtualRpdoInd(pPdo->pdoNr, &canMsg->pData[0] CO_COMMA_LINE_PARA);
            }
# endif /* CONFIG_VIRTUAL_OBJECTS_PDO */

# ifdef CONFIG_REDUNDANCY_SUPPORT
	        /* pdo indication shall only indicated on active interface */
            if (canLine == GL_VAR(co_redcyActiveLine))
# endif /* CONFIG_REDUNDANCY_SUPPORT */
            {
        	  pdoInd(pPdo->pdoNr CO_COMMA_LINE_PARA);
	    }

	}
# ifdef CONFIG_PDO_EVENTTIMER
	/* restart event timer */
	if (pPdo->timer.timerVal != 0)  {
	    (void)addTimerEvent(&pPdo->timer, pPdo->timer.timerVal,
		CO_TIMER_TYPE_EVENTRPDO CO_COMMA_LINE_PARA);
	}
# endif /* CONFIG_PDO_EVENTTIMER */
    }
}


# ifdef CONFIG_PDO_DATA_PTR_FCT
/****************************************************************************/
/**
*++ \brief getPdoDataPtr - returns pointer to CAN message buffer
*-- \brief getPdoDataPtr - liefert Pointer zu CAN Puffer des PDOs
*
*
*-- Diese Funktion liefert einen Pointer auf den Messagepuffer
*-- der empfangenen Daten für das aktuelle PDO.
*-- Achtung !!
*-- Der Pointer ist nur gültig, wenn die Funktion innerhalb von pdoInd()
*-- aufgerufen wird.
*++ This functions returns a pointer to the message buffer
*++ for the received data of the actual PDO.
*++ Attention !
*++ The pointer is only valid if the function is called at pdoInd().
*
*-- Diese Funktion ist nur verfügbar, wenn das define
*++ This function is only available if the define
* CONFIG_PDO_DATA_PTR_FCT
*-- gesetzt ist.
*++ is set.
*
*
* \retval ptr
*++ pointer to data buffer (UNSIGNED8 buf[8])
*-- Pointer auf Datenpuffer (UNSIGNED8 buf[8])
*
*/

UNSIGNED8 *getPdoDataPtr(
	CO_LINE_PARA_DECL	/**< number of CAN line 0..CO_MAX_CAN_LINES-1 */
    )
{
    return(GL_ARRAY(pPdoRecData));
}
# endif /* CONFIG_PDO_DATA_PTR_FCT */
#endif /* CONFIG_PDO_CONSUMER */


#ifdef CONFIG_PDO_PRODUCER
/*******************************************************************
*
*++ pdoRtrMsgReceived - pdo RTR message received
*-- pdoRtrMsgReceived - PDO RTR message erhalten
*
* \internal
*
*++ This function edit the received RTR for transmit pdos
*-- Diese Funktion bearbeitet die RTR für Transmit-PDOs
*
* RETURNS
*	nothing
*
*/

void pdoRtrMsgReceived(
	CAN_MSG_T *canMsg	/* Pointer to CAN Message */
	CO_COMMA_LINE_PARA_DECL	/**< number of CAN line 0..CO_MAX_CAN_LINES-1 */
    )
{
PDO_T	 *pPdo;			/* pointer to actual pdo */
UNSIGNED8  	pData[8];	/* transmit buffer */
RET_T retVal = CO_OK;

    pPdo = searchForPdoCobId(
# ifdef CONFIG_MULT_LINES
		&GL_PVAR(co_trPdo)[GL_ARRAY(co_trPdoLineOffs)],
#  ifdef CONFIG_PDO_FAST_SORT
		&GL_PVAR(co_trPdoCobIdxList)[GL_ARRAY(co_trPdoLineOffs)],
#  endif /* CONFIG_PDO_FAST_SORT */
# else /* CONFIG_MULT_LINES */
		&GL_PVAR(co_trPdo)[0],
#  ifdef CONFIG_PDO_FAST_SORT
		&GL_PVAR(co_trPdoCobIdxList)[0],
#  endif /* CONFIG_PDO_FAST_SORT */
# endif /* CONFIG_MULT_LINES */
		GL_ARRAY(co_trPdoCnt),
		canMsg->cobId);
    if (pPdo == NULL)  {
	return;
    }

    /* ignore request if RTR is disabled at COB-Id */
    if (pPdo->pCOB->eType != CO_COB_PDO_PROD_RTR)  {
	return;
    }

# ifdef CONFIG_PDO_RTR_IND
    rtrPdoInd(pPdo->pdoNr CO_COMMA_LINE_PARA);
# endif /* CONFIG_PDO_RTR_IND */

    /* PDO found */
    if (prepareTransPdo(pPdo, pData CO_COMMA_LINE_PARA) == CO_OK)  {
	/* changed the behavior to always send a response to a RTR
	   because of a decision of the CiA 15.10.2012 / Conformance Test */
# ifdef CONFIG_TPDO_RTR_AFTER_NEXT_SYNC
	/* asynchron PDO has to be transmitted immediately */
	/* synchron PDO has to transmit at the next SYNC */
	if ((pPdo->flags & PDOFLAG_SYNC) != 0) {
	    pPdo->flags |= PDOFLAG_TOTRANSMIT;
	} else
# endif
	{
	    retVal = TRANSMIT_COB(pPdo->pCOB, pData);
# ifdef CO_CONFIG_PDO_SEND_IND
            sendPdoInd( pPdo->pdoNr, SEND_PDO_IND_RTR, retVal CO_COMMA_LINE_PARA );
# endif /*CO_CONFIG_PDO_SEND_IND*/
	    /* if inhibittime is set, start timer */
	    if (pPdo->wInhibitTime > 0)  {
		startInhibitTimer(&pPdo->inhibit, pPdo->wInhibitTime
			CO_COMMA_LINE_PARA);
	    }
# ifdef CONFIG_PDO_EVENTTIMER
	    /* reload the event time */
	    if (pPdo->timer.timerVal != 0)  {
		(void)addTimerEvent(&pPdo->timer, pPdo->timer.timerVal,
		    CO_TIMER_TYPE_EVENTTPDO | CO_TIMER_TYPE_CYCLIC
		    CO_COMMA_LINE_PARA);
	    }
# endif /* CONFIG_PDO_EVENTTIMER */
	}
    }

    /* silence compiler warning */
    CO_INTERNAL_NOT_USED(retVal);
}
#endif /* CONFIG_PDO_PRODUCER */


#if (defined(CONFIG_SYNC_CONSUMER) || defined(CONFIG_SYNC_PRODUCER)) && defined(CONFIG_PDO_PRODUCER)
/*******************************************************************
*
* transSyncPdo - start transmission of synchronous PDO
*
* \internal
*
* This function inserts all transmission requests for synchronous PDOs
* into transmission buffer.
*
* \return
* nothing
*
*/

void transSyncPdo(
	CO_LINE_PARA_DECL
     )
{
PDO_T   *pPdo;			/* pointer to actual pdo */
UNSIGNED16	cnt;
RET_T retVal = CO_OK;

# ifdef CONFIG_REDUNDANCY_SUPPORT
# else /* CONFIG_REDUNDANCY_SUPPORT */
    if (GL_ARRAY(co_Node).eState != OPERATIONAL)  {
        return;
    }
# endif /* CONFIG_REDUNDANCY_SUPPORT */

    cnt = 0;
    while (cnt < GL_ARRAY(co_trPdoCnt))  {
	pPdo = &GL_PVAR(co_trPdo)[cnt
# ifdef CONFIG_MULT_LINES
		+ GL_ARRAY(co_trPdoLineOffs)
# endif /* CONFIG_MULT_LINES */
		];
	cnt++;

        /* PDO enabled and toTransmit */
        if ((pPdo->flags & (PDOFLAG_DISABLED | PDOFLAG_SYNC | PDOFLAG_TOTRANSMIT))
		    == (PDOFLAG_SYNC | PDOFLAG_TOTRANSMIT)) {
            /* copy to transmit buffer and send */
            retVal = TRANSMIT_COB(pPdo->pCOB, pPdo->shadowData);
# ifdef CO_CONFIG_PDO_SEND_IND
            sendPdoInd( pPdo->pdoNr, SEND_PDO_IND_SYNC, retVal CO_COMMA_LINE_PARA );
# endif /*CO_CONFIG_PDO_SEND_IND*/
            /* for acyclic PDO reset transmit request */
            pPdo->flags &= (FLAG_T)~PDOFLAG_TOTRANSMIT;
	}
    }

    /* silence compiler warning */
    CO_INTERNAL_NOT_USED(retVal);
}
#endif /* (defined(CONFIG_SYNC_CONSUMER) || defined(CONFIG_SYNC_PRODUCER)) && defined(CONFIG_PDO_PRODUCER) */




#if (defined(CONFIG_SYNC_CONSUMER) || defined(CONFIG_SYNC_PRODUCER)) && defined(CONFIG_PDO_CONSUMER)
/*******************************************************************
*
* updateSyncRpdo - updates the values of synchronous RPDOs
*
* \internal
*
* This function will be called if a SYNC occurs.
* Its task is to copy the values to the object dictionary.
* This procedure will be done only
* for acyclic PDOs and cyclic PDOs which SYNC counter
* was decremented to zero and additionally the flag toUpdate is 1.
*
* \return
* nothing
*
*/

void updateSyncRpdo(
	CO_REDCY_PARA_DECL
    )
{
PDO_T	*pPdo;		/* pointer to current sync. RPDO buffer */
INTEGER16	cnt;

# ifdef CONFIG_REDUNDANCY_SUPPORT
# else /* CONFIG_REDUNDANCY_SUPPORT */
    if (GL_ARRAY(co_Node).eState != OPERATIONAL)  {
	return;
    }
# endif /* CONFIG_REDUNDANCY_SUPPORT */

    cnt = 0;
    while (cnt < GL_ARRAY(co_recPdoCnt))  {
	pPdo = &GL_PVAR(co_recPdo)[cnt
# ifdef CONFIG_MULT_LINES
		+ GL_ARRAY(co_recPdoLineOffs)
# endif /* CONFIG_MULT_LINES */
		];
	cnt++;
	/* PDO disabled */
	if ((pPdo->flags & (PDOFLAG_DISABLED | PDOFLAG_SYNC | PDOFLAG_TOUPDATE))
		== (PDOFLAG_SYNC | PDOFLAG_TOUPDATE )) {
	    /*
	     * laut DS301 V4.01 wird bei einem zyklischen Sync-RPDO
	     * der Transmission Type nicht ausgewertet.
	     * siehe: Seite  9-8
	     */
	    pPdo->flags &= (FLAG_T)~PDOFLAG_TOUPDATE;

	    /* allocate security mechanism for object dictionary consistency */
	    CO_COM_PART_ALLOC(CO_LINE_PARA);
	    CO_APPL_PART_ALLOC(CO_LINE_PARA);

	    (void) CMS_MapDecode(&GL_PVAR(co_mappingTable)[pPdo->mapStartIdx],
			pPdo->shadowData,
			pPdo->actMapCnt CO_COMMA_LINE_PARA);

	    /* release security mechanism for object dictionary consistency */
	    CO_APPL_PART_RELEASE(CO_LINE_PARA);
	    CO_COM_PART_RELEASE(CO_LINE_PARA);


# ifdef CONFIG_REDUNDANCY_SUPPORT
	    /* pdo indication shall only indicated on active interface */
	    if (canLine == GL_VAR(co_redcyActiveLine))
# endif /* CONFIG_REDUNDANCY_SUPPORT */
	    {

# ifdef CONFIG_PDO_DATA_PTR_FCT
		GL_ARRAY(pPdoRecData) = &pPdo->shadowData[0],
# endif /* CONFIG_PDO_DATA_PTR_FCT */

# ifdef CONFIG_VIRTUAL_OBJECTS_PDO
            /* if the pdo contains virtual objects, pass the buffer to the application */
                if ( PDOFLAG_VIRTUAL_OBJ_TRUE == pPdo->virtualObjFlags ) {
                    (void) coUserVirtualRpdoInd(pPdo->pdoNr, &pPdo->shadowData[0] CO_COMMA_LINE_PARA);
                }
# endif /* CONFIG_VIRTUAL_OBJECTS_PDO */

		pdoInd(pPdo->pdoNr CO_COMMA_LINE_PARA);
	    }
	}
    }
}


/*******************************************************************
*
*++ delSyncShadowBuffer - delete all data at sync shadow buffer
*-- delSyncShadowBuffer - lösche alle SYNC Daten im Shadow-Puffer
*
*++ This function delete all data at shadow buffer for synchron receive PDOs.
*-- Diese Funktion löscht alle Daten in den Schattenpuffern
*-- der synchronen Empfangs-PDOs.
*
* RETURNS
*	nothing
*
*/

void delSyncShadowBuffer(
	CO_LINE_PARA_DECL
    )
{
UNSIGNED16	nr;
PDO_T		*pPdo;

    nr = 0;
    /* for all yet defined pdos */
    while (nr < GL_ARRAY(co_recPdoCnt))  {
	pPdo = &GL_PVAR(co_recPdo)[nr
# ifdef CONFIG_MULT_LINES
		+ GL_ARRAY(co_recPdoLineOffs)
# endif /* CONFIG_MULT_LINES */
		];
	pPdo->flags &= (FLAG_T)~PDOFLAG_TOUPDATE;
	nr++;
    }
}
#endif /* (defined(CONFIG_SYNC_CONSUMER) || defined(CONFIG_SYNC_PRODUCER)) && defined(CONFIG_PDO_CONSUMER) */


#if (defined(CONFIG_PDO_CONSUMER) || defined(CONFIG_PDO_PRODUCER))
# ifdef CONFIG_PDO_EVENTTIMER
#  ifdef CONFIG_PDO_PRODUCER
/*******************************************************************
*
*++ eventTransPdo - process event Transmit PDOs
*-- eventTransPdo - event Transmit PDOs bearbeiten
*
* \internal
*
*++ This function tests all event TPDOs
*++ and transmits it if the event-time is over.
*++ All RTR requested RPDOs are supervised according there TimeOut value.
*-- Diese Funktion testet alle TPDOs und weist die Sendung an
*-- wenn die Event-Zeit abgelaufen ist.
*-- Gleichzeitig werden alle per RTR angeforderten RPDOs
*-- auf ihr TimeOut überwacht.
*
* \return
*	nothing
*
*/

void eventTransPdo(
	TIMER_EVENT_T	*pTimer	/* pointer to timer event structure */
	CO_COMMA_LINE_PARA_DECL	/* number of CAN line 0..CO_MAX_CAN_LINES-1 */
    )
{
PDO_T		*pPdo;			/* pointer to actual pdo */
UNSIGNED8	buffer[8];		/* transmit buffer */

    /* timer structure is the first structure at pdo structure,
     * therefore the pointer are the same */
    pPdo = (PDO_T *)pTimer;

# ifdef CONFIG_REDUNDANCY_SUPPORT
# else /* CONFIG_REDUNDANCY_SUPPORT */
    if (GL_ARRAY(co_Node).eState != OPERATIONAL)  {
	return;
    }
# endif /* CONFIG_REDUNDANCY_SUPPORT */

# ifdef CONFIG_PDO_EVENTTIMER_INDICATION
    pdoEventTimerInd(pPdo->pdoNr CO_COMMA_LINE_PARA);
# endif /* CONFIG_PDO_EVENTTIMER_INDICATION */

    /* accept event timer for PDO transmission only if inhibit time is elapsed*/
    if (pPdo->inhibit.ticks == 0)  {
# if defined(CONFIG_MPDO_DEST) || defined(CONFIG_MPDO_SRC)
        if ( pPdo->mpdoFlags == MPDOFLAG_DEST_PRODUCER ) {
            (void)mpdoTimerEventInd(pPdo->pdoNr, CO_MPDO_DEST_IND CO_COMMA_LINE_PARA);
        } else if ( pPdo->mpdoFlags == MPDOFLAG_SRC_PRODUCER ) {
            (void)mpdoTimerEventInd(pPdo->pdoNr, CO_MPDO_SRC_IND CO_COMMA_LINE_PARA);
        } else
# endif /* (CONFIG_MPDO_DEST) || defined(CONFIG_MPDO_SRC) */
        {
	    if (prepareTransPdo(pPdo, buffer CO_COMMA_LINE_PARA) == CO_OK)  {
# ifdef CO_CONFIG_PDO_SEND_IND
                RET_T retVal;
	        retVal = TRANSMIT_COB(pPdo->pCOB, buffer);
                sendPdoInd( pPdo->pdoNr, SEND_PDO_IND_TIME, retVal CO_COMMA_LINE_PARA );
# else /*CO_CONFIG_PDO_SEND_IND*/
	        (void)TRANSMIT_COB(pPdo->pCOB, buffer);
# endif /*CO_CONFIG_PDO_SEND_IND*/
	    /* reload for the timer event is not necessary (it's a cyclic timer) */
	    }
        }

	/* event timer tx-event causes reload of inhibit timer */
	if (pPdo->wInhibitTime > 0) {
	    startInhibitTimer(&pPdo->inhibit, pPdo->wInhibitTime
		CO_COMMA_LINE_PARA);
	}
    }
}


/*******************************************************************/
/*
*
* checkPdoInhibitTime - check the PDO Inhibit Timer
*
* This function checks if the PDO inhibit timer is running
*
* \retval CO_TRUE
*++ Inhibit timer is running
*-- Inhibit timer ist aktiv
* \retval CO_FALSE
*++ Inhibit timer is off
*-- Inhibit timer ist aus
*
*/

BOOL_T checkPdoInhibitTime(
	UNSIGNED16 pdoNr	/**< number of Transmit PDO */
	CO_COMMA_LINE_PARA_DECL	/* number of CAN line 0..CO_MAX_CAN_LINES-1 */
    )
{
PDO_T	  *pPdo;		/* pointer to actual pdo structure */

    pPdo = pdoExist(pdoNr, TRANSMIT_PDO CO_COMMA_LINE_PARA);
    if (pPdo == NULL) {
	return(CO_FALSE);
    }

    if (pPdo->inhibit.ticks > 0) {
	return(CO_TRUE);
    }

    return(CO_FALSE);
}
#  endif /* CONFIG_PDO_PRODUCER */

#  ifdef CONFIG_PDO_CONSUMER
/*******************************************************************
*
*++ eventRecPdo - process event Receive PDOs
*-- eventRecPdo - event Receive PDOs bearbeiten
*
* \internal
*
*++ This function tests all event RPDOs
*++ and checks it if the event-time is over.
*-- Diese Funktion testet alle RPDOs und checkt
*-- ob die Event-Zeit abgelaufen ist.
*
* \return
*	nothing
*
*/
void eventRecPdo(
	TIMER_EVENT_T	*pTimer		/* pointer to timer event structure */
	CO_COMMA_LINE_PARA_DECL	/* number of CAN line 0..CO_MAX_CAN_LINES-1 */
    )
{
PDO_T	*pPdo;			/* pointer to actual pdo */

    /* timer structure is the first structure at pdo structure,
     * therefore the pointer are the same */
    pPdo = (PDO_T *)pTimer;

# ifdef CONFIG_REDUNDANCY_SUPPORT
# else /* CONFIG_REDUNDANCY_SUPPORT */
    if (GL_ARRAY(co_Node).eState != OPERATIONAL)  {
	return;
    }
# endif /* CONFIG_REDUNDANCY_SUPPORT */

    /* call user function */
    pdoTimerInd(pPdo->pdoNr CO_COMMA_LINE_PARA);
    /* pPdo->flags &= (UNSIGNED8)~PDOFLAG_OUTSTANDING; */
}
#  endif /* CONFIG_PDO_CONSUMER */
# endif /* CONFIG_PDO_EVENTTIMER */
#endif /* (defined(CONFIG_PDO_CONSUMER) || defined(CONFIG_PDO_PRODUCER)) */




#if defined(CONFIG_PDO_CONSUMER) || defined(CONFIG_PDO_PRODUCER)
/*******************************************************************/
/*
*
* setPdoTransType - sets the new PDO Transmission Type
*
* \internal
*
* This service sets the Transmission Type of PDOs.
*
* \retval CO_OK
* success
* \retval CO_TRANSTYPE
* bad transmission type
*
*/

static RET_T setPdoTransType(
	PDO_T		*pPdo,		/* pointer to actual pdo */
	UNSIGNED8	kind,		/* kind of pdo */
	UNSIGNED8	transType	/* new Transmission Type */
	CO_COMMA_LINE_PARA_DECL	/* number of CAN line 0..CO_MAX_CAN_LINES-1 */
    )
{
# if defined(CONFIG_MULT_LINES) || defined(CONFIG_NO_GLOBAL_VARS)
	/* to avoid compiler warnings */
	/* canLine = canLine; */
	CO_LINE_PARA = CO_LINE_PARA;
# endif /* CONFIG_MULT_LINES */

    /* sync pdo ? */
    if (transType < 241u)  {

	/* sync PDO */
# if (defined(CONFIG_SYNC_PRODUCER) || defined(CONFIG_SYNC_CONSUMER))

	/* bit must be set for SYNC PDOs */
	if ((pPdo->flags & PDOFLAG_SYNC_POSSIBLE) == 0) {
	    return(CO_E_TRANS_TYPE);
	}

#  ifdef CONFIG_PDO_EVENTTIMER
	/* disable event timer */
	{ UNSIGNED16 tmpU16 = 0;
	(void) setPdoEventTime(pPdo, kind, 0 CO_COMMA_LINE_PARA);
	if (kind == RECEIVE_PDO)  {
	    (void) putObj((UNSIGNED16)(RPDO_PARA_BASE_INDEX + pPdo->pdoNr - 1), 5,
		(UNSIGNED8 *)&tmpU16, 2, CO_TRUE CO_COMMA_LINE_PARA);
	} else {
	    (void) putObj((UNSIGNED16)(TPDO_PARA_BASE_INDEX + pPdo->pdoNr - 1), 5,
		(UNSIGNED8 *)&tmpU16, 2, CO_TRUE CO_COMMA_LINE_PARA);
	}
	}
#  endif /* CONFIG_PDO_EVENTTIMER */

	/* set sync sign */
	pPdo->flags |= PDOFLAG_SYNC;
	if (transType > 0)  {
	    /* cyclic pdo */
	    pPdo->flags |= PDOFLAG_CYCLIC;
	} else {
	    /* acyclic pdo */
	    pPdo->flags &= (FLAG_T)~PDOFLAG_CYCLIC;
	}

	pPdo->flags &= (FLAG_T)~(PDOFLAG_ONLY_RTR);
	pPdo->curCount = transType;
# else /* (defined(CONFIG_SYNC_PRODUCER) || defined(CONFIG_SYNC_CONSUMER)) */
	return(CO_E_TRANS_TYPE);
# endif /* (defined(CONFIG_SYNC_PRODUCER) || defined(CONFIG_SYNC_CONSUMER)) */

    } else

    if (transType < 252u)  {
	/* reserved transmission type */
	return(CO_E_TRANS_TYPE);
    } else

    if (transType < 254u)  {
	/* only RTR pdos */

# ifdef CONFIG_M4D_SERVER
# else /* CONFIG_M4D_SERVER */
	/* RTR for Receive PDOs are not allowed */
	if (kind == RECEIVE_PDO)  {
	    return(CO_E_TRANS_TYPE);
	}
# endif /* CONFIG_M4D_SERVER */

# ifdef CONFIG_ONLY_ONE_TRANSMIT_CHANNEL
	/* RTR are not allowed */
	return(CO_E_TRANS_TYPE);
# endif /* CONFIG_ONLY_ONE_TRANSMIT_CHANNEL */

	/* sync ? */
	if (transType == 252u)  {
	    /* synchron RTR only */

# if (defined(CONFIG_SYNC_PRODUCER) || defined(CONFIG_SYNC_CONSUMER))
	    pPdo->flags &= (FLAG_T)~PDOFLAG_CYCLIC;
	    pPdo->flags |= PDOFLAG_SYNC;
# else /* (defined(CONFIG_SYNC_PRODUCER) || defined(CONFIG_SYNC_CONSUMER)) */
	    return(CO_E_TRANS_TYPE);
# endif /* (defined(CONFIG_SYNC_PRODUCER) || defined(CONFIG_SYNC_CONSUMER)) */
	} else {
	    pPdo->flags &= (FLAG_T)~(PDOFLAG_SYNC | PDOFLAG_CYCLIC);
	}

	pPdo->flags |= PDOFLAG_ONLY_RTR;
    } else  {
	pPdo->flags &= (FLAG_T)~(PDOFLAG_SYNC | PDOFLAG_CYCLIC | PDOFLAG_ONLY_RTR);
    }

    pPdo->transType = transType;

    return(CO_OK);
}
#endif /* defined(CONFIG_PDO_CONSUMER) || defined(CONFIG_PDO_PRODUCER) */

#if defined(CONFIG_PDO_CONSUMER) || defined(CONFIG_PDO_PRODUCER)
# ifdef CONFIG_PDO_EVENTTIMER
/*******************************************************************/
/*
*
* setPdoEventTime - sets the new PDO Event Time
*
* \internal
*
* This service sets the Event Time of PDOs.
* For the other CANopen Communication Objects
* there is no inhibit time entry in Object Dictionary.
*
* \retval CO_OK
*++ success
*-- Erfolg
* \retval CO_E_NOT_EXIST
* internal communication object doesn't exist
*
*/

static RET_T setPdoEventTime(
	PDO_T		*pPdo,		/* pointer to actual pdo */
	UNSIGNED8	kind,		/* kind of pdo */
	UNSIGNED16	eventTime	/* new Event Time, unit 1ms */
	CO_COMMA_LINE_PARA_DECL	/* number of CAN line 0..CO_MAX_CAN_LINES-1 */
    )
{
    pPdo->timer.timerVal = (UNSIGNED32)eventTime * 10;

    /* remove old event */
    removeTimerEvent(&pPdo->timer CO_COMMA_LINE_PARA);

    if (eventTime != 0)  {
	/* eventtimer isn' allowed for sync pdos */
	if ((pPdo->flags & PDOFLAG_SYNC) != 0)  {
	    return(CO_E_TRANS_TYPE);
	}

	if (kind == TRANSMIT_PDO)  {
	    (void)addTimerEvent(&pPdo->timer, pPdo->timer.timerVal,
		CO_TIMER_TYPE_EVENTTPDO | CO_TIMER_TYPE_CYCLIC
		CO_COMMA_LINE_PARA);
	} else {
/* eventtimer starts with the next reception of this  PDO */
	    /* addTimerEvent(&pPdo->timer, pPdo->timer.timerVal, */
		/* CO_TIMER_TYPE_EVENTRPDO | CO_TIMER_TYPE_CYCLIC */
		/* CO_COMMA_LINE_PARA); */
	}
    }

    return(CO_OK);
}
# endif /* CONFIG_PDO_EVENTTIMER */
#endif /* defined(CONFIG_PDO_CONSUMER) || defined(CONFIG_PDO_PRODUCER) */

#if defined(CONFIG_PDO_CONSUMER) || defined(CONFIG_PDO_PRODUCER)
/*******************************************************************/
/*
*
* setPdoInhibitTime - sets the new Inhibit Time
*
* \internal
*
* This service sets the Inhibit Time of PDOs.
* For the other CANopen Communication Objects
* there is no inhibit time entry in Object Dictionary.
*
* The DS301 doesn't allowed to change the inhibit time
* while the PDO is enabled
*
* \retval CO_OK
*++ success
*-- Erfolg
* \retval CO_E_NOT_EXIST
* internal communication object doesn't exist
*
*/

static RET_T setPdoInhibitTime(
	PDO_T	 *pPdo,		/* pointer to actual PDO */
	UNSIGNED16 inhibitTime	/* new Inhibit Time, unit 100us */
	CO_COMMA_LINE_PARA_DECL	/* number of CAN line 0..CO_MAX_CAN_LINES-1 */
      )
{
RET_T retVal = CO_OK;

    /* inhibit can only be changed if PDO is disabled */
    if ((pPdo->flags & PDOFLAG_DISABLED) == 0u)  {
	retVal = CO_E_TRANS_TYPE;
    } else {

        pPdo->wInhibitTime = inhibitTime;

        /* stop inhibit timer */
        stopInhibitTimer(&pPdo->inhibit CO_COMMA_LINE_PARA);
    }

    return(retVal);
}
#endif /* defined(CONFIG_PDO_CONSUMER) || defined(CONFIG_PDO_PRODUCER) */


#if defined(CONFIG_PDO_CONSUMER) || defined(CONFIG_PDO_PRODUCER)
/*******************************************************************/
/*
*
* setPdoCobId - sets the COB-ID of a PDO
*
* \internal
*
* This function sets the cob-id of a PDO
*
* \retval CO_OK
*++ success
*-- Erfolg
* \retval CO_E_NOT_EXIST
* internal communication object doesn't exist
* \retval CO_E_RANGE
* COB-ID is out of the range (1..1760)
* \retval CO_E_TRANS_TYPE
* bad transtype
*
*/

static RET_T setPdoCobId(
	PDO_T		*pPdo,		/* pointer to pdo */
	UNSIGNED32	cobId,		/* new COB-ID */
	UNSIGNED8	kind		/* kind of PDO RECEIVE/TRANSMIT */
	CO_COMMA_LINE_PARA_DECL	/* number of CAN line 0..CO_MAX_CAN_LINES-1 */
    )
{
#ifdef CONFIG_PDO_FAST_SORT
INTEGER16	cnt = 0;
PDO_T		*pdoList = NULL;	/* pointer to pdo list */
UNSIGNED16	*idxList = NULL;	/* pointer to sorted index list */
#endif /* CONFIG_PDO_FAST_SORT */
COB_KIND_T	cobKind = CO_COB_PDO_CONS;
RET_T		retVal;
FLAG_T		flags = pPdo->flags;


#ifndef CO_CONFIG_DONT_CHECK_RESTRICTED_COBID
     retVal = coCheckRestrictedCobId(0x0000, cobId CO_COMMA_LINE_PARA); /* PDO checks all restricted cobIds thus index does not matter */
     if (retVal != CO_OK)  {
         return(retVal);
     }
#endif

    /* determine cob type */
    if (kind == TRANSMIT_PDO) {

# ifdef CONFIG_PDO_PRODUCER
	/* RTR not allowed ? */
	if ((cobId & PDO_NO_RTR_ALLOWED_BIT) != 0)  {
	    cobKind = CO_COB_PDO_PROD;
	} else {
	    cobKind = CO_COB_PDO_PROD_RTR;
	}

#  ifdef CONFIG_PDO_FAST_SORT
	cnt = GL_ARRAY(co_trPdoCnt);
#   ifdef CONFIG_MULT_LINES
	pdoList = &GL_PVAR(co_trPdo)[GL_ARRAY(co_trPdoLineOffs)];
	idxList = &GL_PVAR(co_trPdoCobIdxList)[GL_ARRAY(co_trPdoLineOffs)];
#   else /* CONFIG_MULT_LINES */
	pdoList = &GL_PVAR(co_trPdo)[0];
	idxList = &GL_PVAR(co_trPdoCobIdxList)[0];
#   endif /* CONFIG_MULT_LINES */
#  endif /* CONFIG_PDO_FAST_SORT */
# endif /* CONFIG_PDO_PRODUCER */

# ifdef CONFIG_PDO_CONSUMER
    } else {
	/* RTR not allowed ? */
	if ((cobId & PDO_NO_RTR_ALLOWED_BIT) != 0u)  {
	    cobKind = CO_COB_PDO_CONS;
	} else {
	    cobKind = CO_COB_PDO_CONS_RTR;
	}
#  ifdef CONFIG_PDO_FAST_SORT
	cnt = GL_ARRAY(co_recPdoCnt);
#   ifdef CONFIG_MULT_LINES
	pdoList = &GL_PVAR(co_recPdo)[GL_ARRAY(co_recPdoLineOffs)];
	idxList = &GL_PVAR(co_recPdoCobIdxList)[GL_ARRAY(co_recPdoLineOffs)];
#   else /* CONFIG_MULT_LINES */
	pdoList = &GL_PVAR(co_recPdo)[0u];
	idxList = &GL_PVAR(co_recPdoCobIdxList)[0u];
#   endif /* CONFIG_MULT_LINES */
#  endif /* CONFIG_PDO_FAST_SORT */
# endif /* CONFIG_PDO_CONSUMER */
    }

    /* set internal valid bit */
    if ((cobId & PDO_NO_VALID_BIT) != 0u) {

	cobId = 0u;
	cobKind = (COB_KIND_T)(cobKind | CO_COB_DISABLED);
	flags |= PDOFLAG_DISABLED;

    } else {

	/* we allow all cob-ids now */
	/* test for valid COB-Ids */
	/* if (((cobId & CAN_29_BIT_ID_MASK) < CO_COBID_PDO_FIRST) */
	 /* || ((cobId & CAN_29_BIT_ID_MASK) > CO_COBID_PDO_LAST))  { */
	    /* return(CO_E_RANGE); */
	/* } */

	/* changing the cob-id is only allowed if the pdo is disabled */
	if ((flags & PDOFLAG_DISABLED) == 0u)  {
	    /* we make an exception for the RTR bit */
	    if (((pPdo->pCOB->cobId ^ cobId) & CAN_BIT_ID_MASK) != 0u)  {
		return(CO_E_TRANS_TYPE);
	    }
	}
	/* enable PDO */
	flags &= (FLAG_T)~PDOFLAG_DISABLED;
    }

    retVal = SET_COB_ID(pPdo->pCOB, cobId & ~PDO_NO_VALID_BIT, cobKind);
    if (retVal != CO_OK)  {
	return(retVal);
    }

    pPdo->flags = flags;

# ifdef CONFIG_PDO_FAST_SORT
    /* sort COB-list */
    sortPdoCobIdList(pdoList, idxList, (UNSIGNED16)cnt);
# else /* CONFIG_PDO_FAST_SORT */
#  ifdef CONFIG_MULT_LINES
#   ifdef CONFIG_NO_GLOBAL_VARS
#   else /* CONFIG_NO_GLOBAL_VARS */
    canLine = canLine;
#   endif /* CONFIG_NO_GLOBAL_VARS */
#  endif /* CONFIG_MULT_LINES */
# endif /* CONFIG_PDO_FAST_SORT */

    return(CO_OK);
}
#endif /* defined(CONFIG_PDO_CONSUMER) || defined(CONFIG_PDO_PRODUCER) */


#if defined(CONFIG_PDO_CONSUMER) || defined(CONFIG_PDO_PRODUCER)
# ifdef CONFIG_PDO_SYNC_START_VALUE
/*******************************************************************/
/*
*
* setPdoSyncStartVal - set sync start value
*
* \internal
*
* This function sets the sync start value
*
* \retval CO_OK
*++ success
*-- Erfolg
* \retval CO_E_TRANS_TYPE);
* bad state - pdo not disabled
*
*/
static RET_T setPdoSyncStartVal(
	PDO_T		*pPdo,		/* pointer to pdo */
	UNSIGNED8	syncVal		/* new sync start val */
	CO_COMMA_LINE_PARA_DECL	/* number of CAN line 0..CO_MAX_CAN_LINES-1 */
    )
{
RET_T retVal = CO_OK;

    /* syncStartVal can only be changed if PDO is disabled */
    if ((pPdo->flags & PDOFLAG_DISABLED) == 0)  {
	retVal = CO_E_TRANS_TYPE;
    } else {

        /* ignore entry if sync Counter Val (0x1019) is disabled */
        if (GL_ARRAY(co_Sync).maxCounter == 0)  {
	    pPdo->syncFlags &= (FLAG_T)~(PDOSYNCFLAG_ENABLED | PDOSYNCFLAG_SYNCSTART);
        } else {

	    /* sync start is only valid if sync value is != 0 */
	    if (syncVal != 0)  {
	        pPdo->syncFlags |= (PDOSYNCFLAG_ENABLED | PDOSYNCFLAG_SYNCSTART);
	    } else {
	        pPdo->syncFlags &= (FLAG_T)~(PDOSYNCFLAG_ENABLED | PDOSYNCFLAG_SYNCSTART);
	    }
        }

        pPdo->syncStartValue = syncVal;
    }

    return(retVal);
}
# endif /* CONFIG_PDO_SYNC_START_VALUE */
#endif /* defined(CONFIG_PDO_CONSUMER) || defined(CONFIG_PDO_PRODUCER) */


#if defined(CONFIG_PDO_CONSUMER) || defined(CONFIG_PDO_PRODUCER)
/*******************************************************************
*
* resetAllPdos - reset all PDOs at resetComm
*
* \internal
*
* The function resets all PDOs at resetComm command
* The cob-id was set by resetObjDir
*
*
*/
void resetAllPdos(
	CO_LINE_PARA_DECL	/* number of CAN line 0..CO_MAX_CAN_LINES-1 */
    )
{
INTEGER16	nr;
UNSIGNED16	pdoNr;

# ifdef CONFIG_PDO_CONSUMER
    nr = 0;
    /* for all yet defined pdos */
    while (nr < GL_ARRAY(co_recPdoCnt))  {
	pdoNr = GL_PVAR(co_recPdo)[nr
# ifdef CONFIG_MULT_LINES
		+ GL_ARRAY(co_recPdoLineOffs)
# endif /* CONFIG_MULT_LINES */
		].pdoNr;	/* receive pdo structures */
	(void)definePdo(RECEIVE_PDO, pdoNr, CO_FALSE CO_COMMA_LINE_PARA);
	nr++;
    }
# endif /* CONFIG_PDO_CONSUMER */
# ifdef CONFIG_PDO_PRODUCER
    nr = 0;
    /* for all yet defined pdos */
    while (nr < GL_ARRAY(co_trPdoCnt))  {
	pdoNr = GL_PVAR(co_trPdo)[nr
# ifdef CONFIG_MULT_LINES
		+ GL_ARRAY(co_trPdoLineOffs)
# endif /* CONFIG_MULT_LINES */
		].pdoNr;	/* transmit pdo structures */
	(void)definePdo(TRANSMIT_PDO, pdoNr, CO_FALSE CO_COMMA_LINE_PARA);
	nr++;
    }
# endif /* CONFIG_PDO_PRODUCER */
}
#endif /* definded(CONFIG_PDO_CONSUMER) || defined(CONFIG_PDO_PRODUCER) */


#if defined(CONFIG_PDO_CONSUMER) || defined(CONFIG_PDO_PRODUCER)
# ifdef CONFIG_PDO_SYNC_START_VALUE
/*******************************************************************
*
* updatePdoSyncStartValues - update pdo sync start values
*
* \internal
*
* The function updates all sync start values
* it is called if the sync counter was changed and
* if the nodes goes to OPERATIONAL
*
*
*/
void updatePdoSyncStartValues(
	CO_LINE_PARA_DECL
    )
{
UNSIGNED16	cnt;
PDO_T		*pPdo;
FLAG_T		flags;
UNSIGNED8	syncVal;

    /* reset sync start value for sync tpdos with sync counter */
    cnt = 0;
    while (cnt < GL_ARRAY(co_trPdoCnt))  {
	pPdo = &GL_PVAR(co_trPdo)[cnt
#  ifdef CONFIG_MULT_LINES
		+ GL_ARRAY(co_trPdoLineOffs)
#  endif /* CONFIG_MULT_LINES */
	    ];
	cnt++;

        /* PDO enabled and synchronous and cyclic */
        if ((pPdo->flags & (PDOFLAG_DISABLED | PDOFLAG_SYNC | PDOFLAG_CYCLIC))
			 == (PDOFLAG_SYNC | PDOFLAG_CYCLIC)) {
	    flags = pPdo->flags;
	    pPdo->flags |= PDOFLAG_DISABLED;
	    syncVal = pPdo->syncStartValue;
	    (void) setPdoSyncStartVal(pPdo, syncVal CO_COMMA_LINE_PARA);
	    pPdo->flags = flags;
	}
    }
}
# endif /* CONFIG_PDO_SYNC_START_VALUE */
#endif /* definded(CONFIG_PDO_CONSUMER) || defined(CONFIG_PDO_PRODUCER) */


#if defined(CONFIG_PDO_CONSUMER) || defined(CONFIG_PDO_PRODUCER)
/****************************************************************************/
/**
*++ \brief getPdoMapObjAddr - return the address of the mapping entry
*-- \brief getPdoMapObjAddr - liefert die Adresse des Mappingeintrages
*
*++ This function looks for the address of the mapping entry.
*++ If the entry is not available or not readable
*++ the function returns the NULL pointer.
*-- Diese Funktion ermittelt die Adresse des gesuchten Mappingeintrages.
*-- Falls kein Mappingeintrag im Objektverzeichnis vorhanden ist
*-- oder der Eintrag nicht lesbar ist,
*-- wird der NULL Zeiger zurückgeliefert.
*
*++ \retval address of mapping entry
*-- \retval Adresse des Mapping Eintrages
*++ \retval NULL
*++ if error occures
*-- bei Fehler
*/

void *getPdoMapObjAddr(
	UNSIGNED8  kind,   /**< kind of PDO RECEIVE_PDO/TRANSMIT_PDO */
	UNSIGNED16 pdoNr,  /**< number of RPDO */
	UNSIGNED8  mapNr   /**< number of mapping object (sub index)*/
	CO_COMMA_LINE_PARA_DECL	/**< number of CAN line 0..CO_MAX_CAN_LINES-1 */
    )
{
PDO_T	   *pPdo;		/* pointer to actual pdo */

    pPdo = pdoExist(pdoNr, kind CO_COMMA_LINE_PARA);
    if (pPdo == NULL) {
        return(NULL);
    }

    if (mapNr > pPdo->actMapCnt)  {
	return(NULL);
    }

    return (void *)
	(GL_PVAR(co_mappingTable)[pPdo->mapStartIdx + mapNr - 1u].pAddress);
}
#endif /* defined(CONFIG_PDO_CONSUMER) || defined(CONFIG_PDO_PRODUCER) */

#if defined(CONFIG_PDO_CONSUMER) || defined(CONFIG_PDO_PRODUCER)
/****************************************************************************/
/**
*++ \brief getMapObjAddr - return the address of the mapping entry
*-- \brief getMapObjAddr - liefert die Adresse des Mappingeintrages
*
*++ This function looks for the address of the mapping entry.
*++ If the entry is not available or not readable
*++ the function returns the NULL pointer.
*-- Diese Funktion ermittelt die Adresse des gesuchten Mappingeintrages.
*-- Falls kein Mappingeintrag im Objektverzeichnis vorhanden ist
*-- oder der Eintrag nicht lesbar ist,
*-- wird der NULL Zeiger zurückgeliefert.
*
*++ (This function is only valid for reveive PDOs)
*-- (Diese Funktion ist nur für Reveive PDOs gültig)
*
*++ \retval address of mapping entry
*-- \retval Adresse des Mapping Eintrages
*++ \retval NULL
*++ if error occures
*-- bei Fehler
*/

void *getMapObjAddr(
	UNSIGNED16 pdoNr,  /**< number of RPDO */
	UNSIGNED8  mapNr   /**< number of mapping object (sub index)*/
	CO_COMMA_LINE_PARA_DECL	/**< number of CAN line 0..CO_MAX_CAN_LINES-1 */
    )
{
    return(getPdoMapObjAddr(RECEIVE_PDO, pdoNr, mapNr CO_COMMA_LINE_PARA));
}
#endif /* defined(CONFIG_PDO_CONSUMER) || defined(CONFIG_PDO_PRODUCER) */



#if defined(CONFIG_PDO_CONSUMER) || defined(CONFIG_PDO_PRODUCER)
/*******************************************************************
*
* searchForPdoCobId - search address of Pdo by cobid
*
* \internal
*
* The function returns the address of an Pdo
* with the given COB-Id but only if the PDO is enabled
*
* \retval
*	pointer to PDO struct if cobid exists
* else NULL
*
*/
static PDO_T *searchForPdoCobId(
	PDO_T		*pdoList,	/* pointer to pdo list */
# ifdef CONFIG_PDO_FAST_SORT
	UNSIGNED16	*idxList,	/* pointer to sorted index list */
# endif /* CONFIG_PDO_FAST_SORT */
	INTEGER16	listLen,	/* new actual len of the list */
	COB_IDENT_T	cobId		/* value to search for */
    )
{
# ifdef CONFIG_PDO_FAST_SORT
INTEGER8  found = 0;
INTEGER16 low = 0, mid = 0, high = listLen - 1;

    while (found == 0)  {
	if (high >= low) {
	    mid = (high + low) / 2;
	    /* found cob-id ? */
	    if (pdoList[idxList[mid]].pCOB->cobId == cobId)  {
		found = 1;
	    } else {
		if (pdoList[idxList[mid]].pCOB->cobId > cobId) {
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
	return(NULL);
    } else {
	return(&pdoList[idxList[mid]]);
    }
# else /* CONFIG_PDO_FAST_SORT */
UNSIGNED16	i;

    i = 0;
    while (i < listLen)  {
	/* found cob-id ? */
	if (pdoList[i].pCOB->cobId == cobId) {
	    return(&pdoList[i]);
	}
	i++;
    }

    return(NULL);
# endif /* CONFIG_PDO_FAST_SORT */
}
#endif /* defined(CONFIG_PDO_CONSUMER) || defined(CONFIG_PDO_PRODUCER) */


#if defined(CONFIG_PDO_CONSUMER) || defined(CONFIG_PDO_PRODUCER)
/*******************************************************************
*
* searchForPdoNr - search index of Pdo by pdo number
*
* \internal
*
* The function returns the index of an Pdo
* with the given pdo number
*
* \retval
*	index at pdo list if pdo number exists
* else -1
*
*/
static INTEGER16 searchForPdoNr(
	PDO_T		*pdoList,	/* pointer to pdo list */
# ifdef CONFIG_PDO_FAST_SORT
	UNSIGNED16	*idxList,	/* pointer to sorted index list */
# endif /* CONFIG_PDO_FAST_SORT */
	INTEGER16	listLen,	/* new actual len of the list */
	UNSIGNED16	val		/* value to search for */
    )
{
#  ifdef CONFIG_PDO_FAST_SORT
INTEGER8  found = 0;
INTEGER16 low = 0, mid = 0, high = listLen - 1;

    while (found == 0)  {
	if (high >= low) {
	    mid = (high + low) / 2;
	    if (pdoList[idxList[mid]].pdoNr == val)  {
		found = 1;
	    } else {
		if (pdoList[idxList[mid]].pdoNr > val) {
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
	return(-1);
    } else {
	return(mid);
    }
# else /* CONFIG_PDO_FAST_SORT */
INTEGER16 idx = listLen - 1;

    while (idx >= 0)  {
	if (pdoList[idx].pdoNr == val)  {
	    return(idx);
	}
	idx--;
    }
    return (-1);
# endif /* CONFIG_PDO_FAST_SORT */
}
#endif /* defined(CONFIG_PDO_CONSUMER) || defined(CONFIG_PDO_PRODUCER) */


#if defined(CONFIG_PDO_CONSUMER) || defined(CONFIG_PDO_PRODUCER)
# ifdef CONFIG_PDO_FAST_SORT
/*******************************************************************
*
* sortIntoPdoNrList - sort new pdo number in sorted index list
*
* \internal
*
* The function sorts the new index of an pdo
* into the given and sorted list
*
* \retval
*	nothing
*
*/
static void sortIntoPdoNrList(
	PDO_T		pdoList[],	/* pointer to pdo list */
	UNSIGNED16	idxList[],	/* pointer to sorted index list */
	UNSIGNED16	valIdx,		/* idx for value to sort in */
	UNSIGNED16	listLen		/* new actual len of the list */
    )
{
UNSIGNED16 idx;

    /* start at end of the list */
    idx = listLen - 1u;

    /* loop until start of list is reached */
    while (idx > 0u)  {
	/* if listvalue less than new value */
	if (pdoList[idxList[idx - 1u]].pdoNr < pdoList[valIdx].pdoNr)  {
	    /* break */
	    break;
	}
	/* move value at index + 1 */
	idxList[idx] = idxList[idx - 1u];

	idx--;
    }
    /* save new value */
    idxList[idx] = valIdx;
}
# endif /* CONFIG_PDO_FAST_SORT */


# ifdef CONFIG_PDO_FAST_SORT
/*******************************************************************
*
* sortPdoCobIdList - sort new pdo cobid in sorted index list
*
* \internal
*
* This function creates a sorted index list (16 bit indexes) for the cobs
* at the given cob list
*
*
* \retval
*	nothing
*
*/
static void sortPdoCobIdList(
	PDO_T		*pdoList,	/* pointer to pdo list */
	UNSIGNED16	*idxList,	/* pointer to sorted index list */
	UNSIGNED16	listLen		/* new actual len of the list */
    )
{
UNSIGNED16	idx;
UNSIGNED8	exchange;
UNSIGNED16	tmpVal;

    /* delete sorted list */
    for (idx = 0u; idx < listLen; idx ++)  {
	idxList[idx] = idx;
    }

    /* bubblesort algo */
    do {
        exchange = 0u;
        /* for (i = 0 ; i < N-1 ; i++) { */
	idx = 0u;
        while (idx < (listLen - 1u))  {
	    /* if (feld[i] > feld[i+1]) { */
	    if (pdoList[idxList[idx]].pCOB->cobId
			> pdoList[idxList[idx + 1u]].pCOB->cobId)  {
		tmpVal = idxList[idx];
		idxList[idx] = idxList[idx + 1u];
		idxList[idx + 1u] = tmpVal;

		exchange = 1u;
	    }
	    idx++;
	}
    } while (exchange == 1u);
}
# endif /* CONFIG_PDO_FAST_SORT */


/*******************************************************************
*
* initPdoVars - init all PDO variables
*
* \internal
*
*
* \return nothing
*
*/

void initPdoVars(
	CO_LINE_PARA_DECL
    )
{
#ifdef CONFIG_MULT_LINES
UNSIGNED8	l;
UNSIGNED32	offs;		/* max 256 lines * 512 PDOs */
#endif /* CONFIG_MULT_LINES */

    /* clear global variables (some compilers doesn't clear global variables */
#ifdef CONFIG_CLEAR_CO_GLOBAL_VARS
# if defined(CONFIG_PDO_CONSUMER) || defined(CONFIG_PDO_PRODUCER)
    memset(&GL_PVAR(co_mappingTable)[0], (int)0,
	(size_t)(sizeof(PDO_MAP_T) * co_maxMappingCnt));
			/* actual mapping count */
    GL_VAR(co_mappingCnt) = 0;
# endif /* defined(CONFIG_PDO_CONSUMER) || defined(CONFIG_PDO_PRODUCER) */

# ifdef CONFIG_PDO_PRODUCER
    memset(&GL_PVAR(co_trPdo)[0], (int)0, (size_t)(sizeof(PDO_T) * PDO_PRODUCER_CNT));

#  ifdef CONFIG_PDO_FAST_SORT
    memset(&GL_PVAR(co_trPdoNrIdxList)[0], (int)0,
	sizeof(UNSIGNED16) * PDO_PRODUCER_CNT);
    memset(&GL_PVAR(co_trPdoCobIdxList)[0], (int)0,
	sizeof(UNSIGNED16) * PDO_PRODUCER_CNT);
#  endif /* CONFIG_PDO_FAST_SORT */
# endif /* CONFIG_PDO_PRODUCER */

# ifdef CONFIG_PDO_CONSUMER
    memset(&GL_PVAR(co_recPdo)[0], (int)0, (size_t)(sizeof(PDO_T) * PDO_CONSUMER_CNT));

#  ifdef CONFIG_PDO_FAST_SORT
    memset(&GL_PVAR(co_recPdoNrIdxList)[0], (int)0,
	sizeof(UNSIGNED16) * PDO_CONSUMER_CNT);
    memset(&GL_PVAR(co_recPdoCobIdxList)[0], (int)0,
	sizeof(UNSIGNED16) * PDO_CONSUMER_CNT);
#  endif /* CONFIG_PDO_FAST_SORT */
# endif /* CONFIG_PDO_CONSUMER */
#endif /* CONFIG_CLEAR_CO_GLOBAL_VARS */


#ifdef CONFIG_MULT_LINES
# ifdef CONFIG_PDO_PRODUCER
    /* calculate pdo producer line offsets */
    l = canLine;
    offs = 0;
    while (l > 0)  {
	l--;
	offs += co_trPdoLineCnts[l];
    }
    GL_ARRAY(co_trPdoLineOffs) = offs;
# endif /* CONFIG_PDO_PRODUCER */

# ifdef CONFIG_PDO_CONSUMER
    /* calculate pdo consumer line offsets */
    l = canLine;
    offs = 0;
    while (l > 0)  {
	l--;
	offs += co_recPdoLineCnts[l];
    }
    GL_ARRAY(co_recPdoLineOffs) = offs;
# endif /* CONFIG_PDO_CONSUMER */
#endif /* CONFIG_MULT_LINES */

# ifdef CONFIG_PDO_PRODUCER
    GL_ARRAY(co_trPdoCnt) = 0;
# endif /* CONFIG_PDO_PRODUCER */

# ifdef CONFIG_PDO_CONSUMER
    GL_ARRAY(co_recPdoCnt) = 0;
# endif /* CONFIG_PDO_CONSUMER */

}
#endif /* defined(CONFIG_PDO_CONSUMER) || defined(CONFIG_PDO_PRODUCER) */

/*______________________________________________________________________EOF_*/
