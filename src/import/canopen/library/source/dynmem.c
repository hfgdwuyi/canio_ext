/*
 *++ dynmem.c - Contains routines for dynamic memory requests
 *-- dynmem.c - Beinhaltet Funktionen für dynamische Speicheranforderung
 *
 * Copyright (c) 2014-2017 port GmbH Halle (Saale)
 *------------------------------------------------------------------
 */


/****************************************************************************/
/**
*  \file dynmem.c
*++ Contains routines for dynamic memory requests
*-- Beinhaltet Funktionen für dynamischen Speicheranforderung
*  \author port GmbH Halle (Saale)
*
*++ This module contains the functions to handle the dynamic memory
*++ allocation for the CANopen services.
*-- Diese Modul beinhaltet Funktionen für die Anforderung von dynamischen
*-- Speicher zum Anlegen der Dienste.
*
*-- Mit dieser Funktionalität
*-- kann der CANopen Library zur Laufzeit
*-- die Anzahl der zu konfigurierenden Dienste mitgeteilt werden.
*-- Verwaltungsstrukturen für die einzelnen Dienste
*-- müssen damit nicht mehr statisch zur Compilezeit festgelegt werden,
*-- sondern können dynamisch zur Laufzeit systemspezifisch angelegt werden.
*-- Der notwendige Speicher
*-- für die Library-internen Verwaltungsstrukturen
*-- wird über dynamische Speicherverwaltung (malloc() angefordert
*-- und beim Beenden der Library wieder freigegeben.
*-- Die dynamische Speicherverwaltung kann mit allen Diensten außer
*-- - Safety Erweiterung (SRDO)
*-- und
*-- - SDO Manager bzw Requester
*-- genutzt werden.
*++ With this functionality the CANopen Library can be informed
*++ about the number of the CANopen services at run-time.
*++ Thereby the management structures for the CANopen services
*++ do not have to be defined at compile-time, but can be defined
*++ at run-time in a system specific way.
*++ The required memory for the internal library structures
*++ is requested using malloc() and is freed when the library is
*++ shut down.
*++ The dynamic memory managmet can be used with all services except
*++ the safety extension (SRDO) and the SDO Manager/Requester.

*-- Die für die Dienste notwendigen Objekte im Kommunikationsprofil
*-- müssen entweder im Objektverzeichnis vorhanden sein,
*-- oder über virtuelle Objekte bereitgestellt werden.
*++ The required objects in the communication segment must be
*++ provided either in the object dictionary or
*++ as virtual objects.

*-- Konfigurationshinweise:
*-- Um die dynamische Speicherverwaltung nutzen zu können,
*-- ist das define
*-- #define CONFIG_DYN_MEM_ALLOC	1
*-- im File cal_conf.h zu definieren,
*-- und die Funktion
*-- RET_T initDynamicServices()
*-- vor der Initialisierung der Library aufzurufen.
*-- Bei Nutzung von mehreren CAN-Linien
*-- ist der Aufruf für jede CAN-Linie vorzunehmen.
*-- Dabei muß die Reihenfolge der CAN-Linien eingehalten werden,
*-- (Initialisierung CAN Linie 0, CAN-Linie 1, ... CAN-Linie n)
*-- da erst beim Aufruf der letzten CAN-Linie
*-- die Speicheranforderungen ausgeführt werden.
*++ Configurations hint:
*++ To use the dynamic memory management
*++ the define
*++ #define CONFIG_DYN_MEM_ALLOC	1
*++ has to be defined in the file cal_conf.h
*++ and the function initDynamicServices() has to be called
*++ before the library has been initialized.
*++ If more CAN lines are used, the initialization
*++ has to be done for each CAN line in the right order.
*++ (CAN line0, CAN line 1, ... CAN line n).

*-- Hinweise bei Nutzung des DesignTools:
*-- Das Freischalten von Diensten erfolgt automatisch mit dem Anlegen
*-- der entsprechenden Objekte im Objektverzeichnis.
*-- Wenn die notwendigen Objekte im Kommunikationsbereich
*-- über virtuelle Objekte bereitgestellt werden,
*-- sind die zugehörigen defines selber in cal_conf.h zu definieren.
*++ Hints for use with the DesignTool:
*++ The services are enabled automatically with the objects in the
*++ object dictionary.
*++ Only if the objects are virtual objects,
*++ the corresponding defines have to be defined by yourself
*++ in the file cal_conf.h.
* \code
  object-index	required define
  0x1016		CONFIG_HEARTBEAT_CONSUMER
  0x1028		CONFIG_EMCY_CONSUMER
  0x1200-0x127f	CONFIG_SDO_SERVER
  0x1280-0x12ff	CONFIG_SDO_CLIENT
  0x1400-15ff	CONFIG_PDO_CONSUMER und CONFIG_MAPPING_CNT
  0x1800-19ff	CONFIG_PDO_PRODUCER und CONFIG_MAPPING_CNT
  0x1f81		CONFIG_NMT_SLAVE_CNT (only for masters)
  0x1f81		CONFIG_GUARD_SLAVE_CNT (only for node guarding masters)
* \endcode
*
*/

/* header of standard C - libraries */
#include <stdlib.h>
#include <stdio.h>

/* header of project specific types */

#include <cal_conf.h>
#include <co_type.h>
#include "drv.h"
#include "sdo.h"
#include "pdo.h"
#include "heartbt.h"
#include "emerg.h"
#ifdef CONFIG_MASTER
# include "nmt_m.h"
#endif /* CONFIG_MASTER */
#ifdef CONFIG_NMT_STARTUP_MANAGER
# include "nmtstart.h"
#endif /* CONFIG_NMT_STARTUP_MANAGER */
#include "dynmem.h"

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

/* local defined variables
---------------------------------------------------------------------------*/
#ifdef CONFIG_RCS_IDENT
CO_LIB_INIT_VAR static char _rcsid[] = "$Id: dynmem.c,v 2.8 2016/09/26 11:16:08 rli Exp $";
#endif /* CONFIG_RCS_IDENT */
#ifdef CONFIG_DYN_MEM_ALLOC
CO_LIB_UNINIT_VAR static void	*pCobList;	/* pointer to cob-list */
CO_LIB_UNINIT_VAR static void	*pIdxList;	/* pointer to cob-index list */
CO_LIB_INIT_VAR static UNSIGNED16	sdoServ = 0;	/* number of server sdos */
CO_LIB_INIT_VAR static UNSIGNED16	sdoClient = 0;	/* number of client sdos */
CO_LIB_INIT_VAR static UNSIGNED16	pdoCons = 0;	/* number of PDO Consumer */
CO_LIB_INIT_VAR static UNSIGNED16	pdoProd = 0;	/* number of PDO Producer */
CO_LIB_INIT_VAR static UNSIGNED16	map = 0;	/* number of mappings */
CO_LIB_INIT_VAR static UNSIGNED16	hb = 0;		/* number of heartbeat consumers */
CO_LIB_INIT_VAR static UNSIGNED16	emcy = 0;	/* number of emergency consumers */
CO_LIB_INIT_VAR static UNSIGNED16	nmtSlave = 0;	/* number of nmtslave */
CO_LIB_INIT_VAR static UNSIGNED16	nmtGuard = 0;	/* number of guard slave */
CO_LIB_INIT_VAR static UNSIGNED16	allCobs = 0;	/* number of all cobs */
#endif /* CONFIG_DYN_MEM_ALLOC */


#ifdef CONFIG_DYN_MEM_ALLOC
/****************************************************************************/
/**
*++ \brief initDynamicServices - init usage of dynamic services
*-- \brief initDynamicServices - intiialisiert dynamische Dienste
*
*-- Mit dieser Funktion
*-- kann der CANopen Library zur Laufzeit
*-- die Anzahl der zu konfigurierenden Dienste mitgeteilt werden.
*-- Sie muß vor der Initialisierung der Library mit
*-- initCANopen()
*-- aufgerufen werden.
*-- Verwaltungsstrukturen für die einzelnen Dienste
*-- müssen damit nicht mehr statisch zur Compilezeit festgelegt werden,
*-- sondern können dynamisch zur Laufzeit systemspezifisch angelegt werden.
*-- Der notwendige Speicher für die Library-internen Verwaltungsstrukturen
*-- wird über dynamische Speicherverwaltung (malloc() angefordert
*-- und beim Beenden der Library wieder freigegeben.
*-- Die dynamische Speicherverwaltung kann mit allen Diensten außer
*-- - Safety Erweiterung (SRDO)
*-- und
*-- - SDO Manager bzw Requester
*-- genutzt werden.
*++ With this functionality the CANopen Library can be informed
*++ about the number of the CANopen services at run-time.
*++ Thereby the management structures for the CANopen services
*++ do not have to be defined at compile-time, but can be defined
*++ at run-time in a system specific way.
*++ The required memory for the internal library structures
*++ is requested using malloc() and is freed when the library is
*++ shut down.
*++ The dynamic memory managmet can be used with all services except
*++ the safety extension (SRDO) and the SDO Manager/Requester.

*-- Um die dynamische Speicherverwaltung nutzen zu können,
*-- ist das define
*-- #define CONFIG_DYN_MEM_ALLOC	1
*-- im File cal_conf.h zu definieren,
*-- und die Funktion
*-- RET_T initDynamicServices()
*-- vor der Initialisierung der Library aufzurufen.
*-- Bei Nutzung von mehreren CAN-Linien
*-- ist der Aufruf für jede CAN-Linie vorzunehmen.
*-- Dabei muß die Reihenfolge der CAN-Linien eingehalten werden,
*-- (Initialisierung CAN Linie 0, CAN-Linie 1, ... CAN-Linie n)
*-- da erst beim Aufruf der letzten CAN-Linie
*-- die Speicheranforderungen ausgeführt werden.
*++ To use the dynamic memory management
*++ the define
*++ #define CONFIG_DYN_MEM_ALLOC	1
*++ has to be defined in the file cal_conf.h
*++ and the function initDynamicServices() has to be called
*++ before the library has been initialized.
*++ If more CAN lines are used, the initialization
*++ has to be done for each CAN line in the right order.
*++ (CAN line0, CAN line 1, ... CAN line n).

*-- Die für die Dienste notwendigen Objekte
*-- müssen entweder im Objektverzeichnis vorhanden sein,
*-- oder über virtuelle Objekte bereitgestellt werden.
*++ The required objects in the communication segment must be
*++ provided either in the object dictionary or
*++ as virtual objects.

* \retval CO_OK
*++ success
*-- Erfolg
* \retval CO_E_MEM
*++ memory allocation fault
*-- Speicherzuweisungsfehler
*
*/
RET_T initDynamicServices(
	UNSIGNED8	sdoServCnt,	/**< number of server sdos (max 127) */
	UNSIGNED8	sdoClientCnt,	/**< number of client sdos (max 127) */
	UNSIGNED16	pdoConsCnt,	/**< number of PDO Consumer (max 512) */
	UNSIGNED16	pdoProdCnt,	/**< number of PDO Producer (max 512) */
	UNSIGNED16	mapCnt,		/**< number of mappings */
	UNSIGNED8	hbCnt,		/**< number of heartbeat consumers */
	UNSIGNED8	emcyCnt,	/**< number of emergency consumers */
	UNSIGNED8	nmtSlaveCnt,	/**< number of NMT slaves */
	UNSIGNED8	nmtGuardCnt	/**< number of Guarding slaves */
	CO_COMMA_LINE_PARA_DECL		/**< line parameter */
    )
{
UNSIGNED16		error = 0;
UNSIGNED16		cobCnt;		/* nmt (0) and nmterr (0x700+n) */
UNSIGNED16		tmpVar;

    cobCnt = 2;		/* nmt (0) and nmterr (0x700+n) */

    /* calculate additionaly cobs */
#ifdef CONFIG_EMCY_PRODUCER
    cobCnt += 1;
#endif /* CONFIG_EMCY_PRODUCER */

#if defined(CONFIG_TIME_PRODUCER) || defined(CONFIG_TIME_CONSUMER)
    cobCnt += 1;
#endif /* defined(CONFIG_TIME_PRODUCER) || defined(CONFIG_TIME_CONSUMER) */

#if defined(CONFIG_LSS_MASTER) || defined(CONFIG_LSS_SLAVE)
    cobCnt += 2;
#endif /* CONFIG_LSS_MASTER */

#ifdef CONFIG_FLYING_MASTER
    cobCnt += 12;
#endif /* CONFIG_FLYING_MASTER */

#if defined(CONFIG_SYNC_PRODUCER) || defined(CONFIG_SYNC_CONSUMER)
    cobCnt += 1;
#endif /* defined(CONFIG_SYNC_PRODUCER) || defined(CONFIG_SYNC_CONSUMER) */


#ifdef CONFIG_MULT_LINES

# ifdef CONFIG_SDO_SERVER
    co_sdoServerLineCnts[canLine] = sdoServCnt;
    sdoServ += sdoServCnt;
# endif /* CONFIG_SDO_SERVER */


# ifdef CONFIG_SDO_CLIENT
    co_sdoClientLineCnts[canLine] = sdoClientCnt;
    sdoClient += sdoClientCnt;
# endif /* CONFIG_SDO_CLIENT */


# ifdef CONFIG_PDO_PRODUCER
    co_trPdoLineCnts[canLine] = pdoProdCnt;
    pdoProd += pdoProdCnt;
# endif /* CONFIG_PDO_PRODUCER */


# ifdef CONFIG_PDO_CONSUMER
    co_recPdoLineCnts[canLine] = pdoConsCnt;
    pdoCons += pdoConsCnt;
# endif /* CONFIG_PDO_CONSUMER */


#if defined(CONFIG_PDO_CONSUMER) || defined(CONFIG_PDO_PRODUCER)
    map += mapCnt;
#endif /* defined(CONFIG_PDO_CONSUMER) || defined(CONFIG_PDO_PRODUCER) */


# ifdef CONFIG_HEARTBEAT_CONSUMER
    co_hbConsLineCnts[canLine] = hbCnt;
    hb += hbCnt;
# endif /* CONFIG_HEARTBEAT_CONSUMER */


# ifdef CONFIG_EMCY_CONSUMER
    emcyConsLineCnts[canLine] = emcyCnt;
    emcy += emcyCnt;
# endif /* CONFIG_EMCY_CONSUMER */


#ifdef CONFIG_MASTER
# ifdef CONFIG_NMT_SLAVE_CNT
    co_nmtSlaveLineCnts[canLine] = nmtSlaveCnt;
    nmtSlave += nmtSlaveCnt;
# endif /* CONFIG_NMT_SLAVE_CNT */

# ifdef CONFIG_NODE_GUARDING
    co_guardSlaveLineCnts[canLine] = nmtGuardCnt;
    nmtGuard += nmtGuardCnt;
# endif /* CONFIG_NODE_GUARDING */

# ifdef CONFIG_NMT_STARTUP_MANAGER
    nmtSlave += nmtSlaveCnt;
# endif /* CONFIG_NMT_STARTUP_MANAGER */
#endif /* CONFIG_MASTER */


    /* setup driver cob line infos */
#undef LIB_VAR
#define LIB_VAR(v,vCnt,t,cnt,xx,cobs)	cobCnt += (cobs * cnt);
#include "dynmem_var.h"
    init_canDriverPtr(NULL, NULL, cobCnt CO_COMMA_LINE_PARA);

    allCobs += cobCnt;

    /* not the last can-line ? */
    if (canLine < (CO_MAX_CAN_LINES - 1))  {
	return(CO_OK);
    }

#else /* CONFIG_MULT_LINES */

#undef LIB_VAR
#define LIB_VAR(v,vCnt,t,cnt,xx,cobs)	cobCnt += (cobs * cnt);
#include "dynmem_var.h"

    allCobs += cobCnt;

    sdoServ = sdoServCnt;
    sdoClient = sdoClientCnt;
    pdoCons = pdoConsCnt;
    pdoProd = pdoProdCnt;
    map = mapCnt;
    hb = hbCnt;
    emcy = emcyCnt;
    nmtSlave = nmtSlaveCnt;
    nmtGuard = nmtGuardCnt;
    nmtSlave = nmtSlaveCnt;

#endif /* CONFIG_MULT_LINES */

    /* usage of one large memory isn't possible because the alignment.
       Some variables can be located outside of the aligment and so
       we would get an error trap */

    /* alloc memory for services */
#undef LIB_VAR
#define LIB_VAR(v,vCnt,t,xx,cnt,cobs)	\
	    if (cnt > 0)  {	\
		v = calloc(1, sizeof(t) * cnt);\
		/* printf("alloc %3d bytes at %p\n", (int)(sizeof(t) * cnt), v); */\
		vCnt = cnt;		\
		if (v == NULL) error++;	\
	    }
#include "dynmem_var.h"

#ifdef CONFIG_REDUNDANCY_SUPPORT
    allCobs *= 2;
#endif /* CONFIG_REDUNDANCY_SUPPORT */

#ifdef CONFIG_DT_ENABLE_ADDITIONAL_CAN_OBJ
    allCobs += CONFIG_DT_ADDITIONAL_CAN_OBJ_CNT;
#endif /* CONFIG_DT_ENABLE_ADDITIONAL_CAN_OBJ */

    /* printf("need %d cobs\n", cobCnt); */

    /* alloc memory for drivers */
    init_canDriverPtr(&pCobList, &pIdxList, allCobs CO_COMMA_LINE_PARA);

    if ((error != 0) || (pCobList == NULL)
#ifdef CONFIG_FAST_SORT
	|| (pIdxList == NULL)
#endif /* CONFIG_FAST_SORT */
	    ) {
	/* deallocate memory */
	freeDynMem();
	return(CO_E_MEM);
    }

    return(CO_OK);
}


/****************************************************************************/
/*
*++ freeMem - release requested memory
*-- freeMem - gibt dynamisch angeforderten Speicher wieder frei
*
* This function is called at deInitLibrary()
*/

void freeDynMem(
    )
{
#undef LIB_VAR
#define LIB_VAR(v,vCnt,t,fcnt,cnt,cobs)	\
	if (v != NULL) {	\
	    /* printf("free %p\n",v);	*/\
	    free(v);		\
	}
#include "dynmem_var.h"

    /* driver cob list */
    if (pCobList != NULL) {
	/* printf("free drv: %p\n", pCobList); */
	free(pCobList);
    }
    /* driver index list */
    if (pIdxList != NULL) {
	free(pIdxList);
    }

    /* reset static variables */
    sdoServ = 0;
    sdoClient = 0;
    pdoCons = 0;
    pdoProd = 0;
    map = 0;
    hb = 0;
    emcy = 0;
    nmtSlave = 0;
    nmtGuard = 0;
    allCobs = 0;
}

#endif /* CONFIG_DYN_MEM_ALLOC */
/*______________________________________________________________________EOF_*/
