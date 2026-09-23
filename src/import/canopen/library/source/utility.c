/*
 *++ utility - contains utility functions
 *-- utility - beinhaltet Utility Funktionen
 *
 * Copyright (c) 1997-2017 port GmbH Halle (Saale)
 *------------------------------------------------------------------
 */


/****************************************************************************/
/**
*  \file utility.c
*++ Contains utility functions
*-- Beinhaltet Utility Funktionen
*  \author port GmbH Halle (Saale)
*
*++ This module contains some useful functions for a convenient usage of
*++ the CANopen Library.
*-- Dieses Modul enthält nützliche Funktionen zur Benutzung der CANopen
*-- Bibliothek.
*
*++ Some of the functions are for internal usage only.
*++ They have no manual entries.
*++ For example CRC calculation used by the SDO block-transfer.
*-- Einige dieser Funktionen werden nur intern benutzt.
*-- Sie haben keine Handbuch Einträge.
*-- Zum Beispiel eine CRC-Summenberechnung,
*-- welche vom SDO Blocktransfer genutzt wird.
*
*/

/* header of standard C - libraries */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

/* header of common types */

#include <cal_conf.h>
#include "drv.h"
#include "nmterr.h"
#include "pdo.h"
#include <co_flag.h>
#include <co_usr.h>
#include <co_mcpy.h>
#include <co_drvif.h>
#include <co_acces.h>
#include <co_setcp.h>
#include "utility.h"
#include "nmt.h"
#include "sdo.h"

#if defined(CONFIG_SYNC_PRODUCER) || defined(CONFIG_SYNC_CONSUMER)
#include "sync.h"
#endif /* defined(CONFIG_SYNC_PRODUCER) || defined(CONFIG_SYNC_CONSUMER) */

#if defined(CONFIG_TIME_PRODUCER) || defined(CONFIG_TIME_CONSUMER)
#include "timer.h"
#endif /* defined(CONFIG_TIME_PRODUCER) || defined(CONFIG_TIME_CONSUMER) */

#ifdef CONFIG_FLYING_MASTER
# include "flyma.h"
#endif /* CONFIG_FLYING_MASTER */

#ifdef CONFIG_SDO_BLOCKTRANSFER
# include "sdoblock.h"
#endif /* CONFIG_SDO_BLOCKTRANSFER */

#ifdef CONFIG_DYN_SDO_CONNECTION_MANAGER
# include "sdomgr.h"
#endif /* CONFIG_DYN_SDO_CONNECTION_MANAGER */

#ifdef CONFIG_CO_LED
# include "led.h"
#endif /* CONFIG_CO_LED */

#ifdef CONFIG_REDUNDANCY_SUPPORT
# include "reduncy.h"
#endif /* CONFIG_REDUNDANCY_SUPPORT */

#if defined(CONFIG_MASTER) && defined(CONFIG_NMT_STARTUP_MANAGER)
# include "nmtstart.h"
#endif

# if defined(CONFIG_SPDO_PRODUCER) || defined(CONFIG_SPDO_CONSUMER)
# include "srdo.h"
# endif /* defined(CONFIG_SPDO_PRODUCER) || defined(CONFIG_SPDO_CONSUMER) */

# ifdef CONFIG_EMCY_PRODUCER
# include "co_emcy.h"
# endif /* CONFIG_EMCY_PRODUCER */

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
CO_LIB_INIT_VAR static char _rcsid[] = "$Id: utility.c,v 2.68 2016/09/26 11:16:09 rli Exp $";
#endif /* CONFIG_RCS_IDENT */


/*******************************************************************
*
*++ flagIdentification - indicate library flags
*-- flagIdentification - indentifiziert library flags
*
* \internal
*
*++ This function evaluates the CANopen-library flags
*++ and calls the apropriate routines.
*++ To minimize the interrupt service routines
*++ there are flags for all necessary services.
*++ These flags are set at the interruptroutines and
*++ evaluate in this function
*-- Diese Funktion wertet die CANopen Library Flags
*-- aus und ruft die entsprechende Behandlungsroutine
*-- Um die Interruptroutinen möglichst kurz zu halten,
*-- werden für die notwendigen Aufgaben in den Interruptroutinen
*-- nur Flags gesetzt.
*-- Diese Flags werden in dieser Funktion ausgewertet.
*
* \retval
*	nothing
*/

void flagIdentification(
	CO_REDCY_PARA_DECL	/**< number of CAN line 0..CO_MAX_CAN_LINES-1 */
   )
{
#ifdef CONFIG_SYNC_CONSUMER
NODE_STATE_T	actState;
#endif /* CONFIG_SYNC_CONSUMER */
#ifdef CONFIG_CAN_ERROR_HANDLING
UNSIGNED8 	tmpFlags;
#endif /* CONFIG_CAN_ERROR_HANDLING */

#ifdef CONFIG_SYNC_CONSUMER

# ifdef CONFIG_REDUNDANCY_SUPPORT
    if ((TEST_COLIB_FLAG(COFLAG_SYNC_RECEIVED))
     && (canLine == GL_VAR(co_redcyActiveLine)))  {
# else /*  CONFIG_REDUNDANCY_SUPPORT */
    if (TEST_COLIB_FLAG(COFLAG_SYNC_RECEIVED))  {
# endif /*  CONFIG_REDUNDANCY_SUPPORT */

	/* check allowed node state */
# ifdef CONFIG_REDUNDANCY_SUPPORT
	/* process only data from active line */
	if (GL_VAR(co_redcyReceivedLine) == GL_VAR(co_redcyActiveLine))  {

	    /* check node state of received line */
	    if (GL_VAR(co_redcyReceivedLine) == CAN_DEFAULT_LINE)  {
		actState = GL_VAR(co_Node).eState;
	    } else {
		actState = GL_VAR(co_redcyNode).eState;
	    }
	} else {
	    /* data no from active line - skip */
	    actState = STOPPED;
	}
# else /* CONFIG_REDUNDANCY_SUPPORT */
	actState = GL_ARRAY(co_Node).eState;
# endif /* CONFIG_REDUNDANCY_SUPPORT */

	if ((actState == OPERATIONAL) || (actState == PRE_OPERATIONAL)) {
# ifdef CONFIG_SYNC_PRE_CMD
	    syncPreCommand(CO_LINE_PARA);
# endif /* CONFIG_SYNC_PRE_CMD */

# ifdef CONFIG_SYNC_COUNTER
#  ifdef CONFIG_EMCY_PRODUCER
        /* correct sync len ? */
        if (GL_ARRAY(co_syncCnt) == 0) {
            if (GL_ARRAY(co_Sync).maxCounter != 0) {
                /* send an emergency */
                (void) writeEmcyReq(ERRCODE_BAD_SYNCLEN, NULL CO_COMMA_LINE_PARA);
            }
        } else {
            if (GL_ARRAY(co_Sync).maxCounter == 0) {
                /* send an emergency */
                (void) writeEmcyReq(ERRCODE_BAD_SYNCLEN, NULL CO_COMMA_LINE_PARA);
            }
        }
#  endif /* CONFIG_EMCY_PRODUCER */
# endif /* CONFIG_SYNC_COUNTER */

# ifdef CONFIG_PDO_PRODUCER
#  ifdef CONFIG_NO_MAP_SYNC_PDO
#  else /* CONFIG_MAP_SYNC_PDO_AFTER_SEND */
	    (void) updateSyncTpdo(CO_LINE_PARA);
#  endif /* CONFIG_NO_MAP_SYNC_PDO */

	    /* transmit synch. TPDOs */
	    transSyncPdo(CO_LINE_PARA);
# endif /* CONFIG_PDO_PRODUCER */
# ifdef CONFIG_PDO_CONSUMER
	    updateSyncRpdo(CO_REDCY_PARA);
# endif /* CONFIG_PDO_CONSUMER */

# if defined(CONFIG_SPDO_PRODUCER) || defined(CONFIG_SPDO_CONSUMER)
	    handleSyncSpdo(CO_LINE_PARA);
# endif /* defined(CONFIG_SPDO_PRODUCER) || defined(CONFIG_SPDO_CONSUMER) */

# ifdef CONFIG_SYNC_CMD
	    syncCommand(CO_LINE_PARA);
# endif /* CONFIG_SYNC_CMD */

	}
	RESET_COLIB_FLAG(COFLAG_SYNC_RECEIVED);
    }
#endif  /* defined(CONFIG_SYNC_CONSUMER) */


    /* timer pulse */
    if ((TEST_COLIB_FLAG(COFLAG_TIMER_PULSED)) != 0u) {

	RESET_COLIB_FLAG(COFLAG_TIMER_PULSED);

	checkTimerEvent(CO_LINE_PARA);

#ifdef CONFIG_SYNC_PRODUCER
	/* sync producer has to send own sync pdos */
	if (TEST_COLIB_FLAG(COFLAG_SYNC_RECEIVED))  {
# ifdef CONFIG_PDO_PRODUCER
#  ifdef CONFIG_NO_MAP_SYNC_PDO
#  else /* CONFIG_MAP_SYNC_PDO_AFTER_SEND */
	    (void) updateSyncTpdo(CO_LINE_PARA);
#  endif /* CONFIG_NO_MAP_SYNC_PDO */

	    /* transmit synch. TPDOs */
	    transSyncPdo(CO_LINE_PARA);
# endif /* CONFIG_PDO_PRODUCER */
# ifdef CONFIG_PDO_CONSUMER
	    updateSyncRpdo(CO_REDCY_PARA);
# endif /* CONFIG_PDO_CONSUMER */

# if defined(CONFIG_SPDO_PRODUCER) || defined(CONFIG_SPDO_CONSUMER)
	    handleSyncSpdo(CO_LINE_PARA);
# endif /* defined(CONFIG_SPDO_PRODUCER) || defined(CONFIG_SPDO_CONSUMER) */

# ifdef CONFIG_SYNC_CMD
	    syncCommand(CO_LINE_PARA);
# endif /* CONFIG_SYNC_CMD */

	    RESET_COLIB_FLAG(COFLAG_SYNC_RECEIVED);
	}
#endif /* CONFIG_SYNC_PRODUCER */

#ifdef CONFIG_REDUNDANCY_SUPPORT
	checkRedundancyEvent(CO_LINE_PARA);
#endif /* CONFIG_REDUNDANCY_SUPPORT */
    }

#ifdef CONFIG_CAN_ERROR_HANDLING
    if ((TEST_COLIB_FLAG(COFLAG_CAN_EVENT)) != 0u)  {
	RESET_COLIB_FLAG(COFLAG_CAN_EVENT);

	tmpFlags = TEST_CAN_FLAG(CANFLAG_ALL);

	/* clear only indicated flags */
	RESET_CAN_FLAG(tmpFlags);

# ifdef CONFIG_CO_ERR_LED
	/* check for CAN controller state flags */
	if ((tmpFlags & CANFLAG_STATE_MASK) != 0)  {

	    /* new CAN controller state */
	    switch (getCanDriverState(CO_REDCY_PARA) & CANFLAG_STATE_MASK)  {
		case CANFLAG_INIT:
		case CANFLAG_BUSOFF:
		    setCoErrLedState(CO_ERR_LED_BUS_OFF CO_COMMA_REDCY_PARA);
		    break;
		case CANFLAG_ACTIVE:
		    resetCoErrLedState(CO_ERR_LED_BUS_OFF | CO_ERR_LED_WARNING
			CO_COMMA_REDCY_PARA);
		    break;
		case CANFLAG_PASSIVE:
		    setCoErrLedState(CO_ERR_LED_WARNING CO_COMMA_REDCY_PARA);
		    resetCoErrLedState(CO_ERR_LED_BUS_OFF CO_COMMA_REDCY_PARA);
		    break;
	    }
	}
# endif /* CONFIG_CO_ERR_LED */

# ifdef CONFIG_NO_ERROR_BEHAVIOR
# else /* CONFIG_NO_ERROR_BEHAVIOR */
	/* new bus off ? */
	if (tmpFlags & CANFLAG_BUSOFF)  {
	    execCommErrorBehavior(CO_REDCY_PARA);
	}
# endif /* CONFIG_NO_ERROR_BEHAVIOR */

	if (canErrorInd(tmpFlags CO_COMMA_REDCY_PARA) == CO_FALSE) {
	    /* try to go BUS ON again */
	    CLEAR_BUSOFF(CO_REDCY_PARA);
	}
    }
#endif /* CONFIG_CAN_ERROR_HANDLING */


#if defined(CONFIG_SDO_BLOCKTRANSFER)
    /* there are SDO to send for block down/up-load */
    if (TEST_COLIB_FLAG(COFLAG_SDO_BLOCKTRANS))  {

	RESET_COLIB_FLAG(COFLAG_SDO_BLOCKTRANS);

	sdoContBlockTrans(CO_LINE_PARA);
    }
#endif /* defined(CONFIG_SDO_BLOCKTRANSFER) */

#ifdef CONFIG_DYN_SDO_CONNECTION_MANAGER
    /* there are SDO manager function to execute */
    if (TEST_COLIB_FLAG(COFLAG_SDO_MANAGER))  {

	RESET_COLIB_FLAG(COFLAG_SDO_MANAGER);

	dynSdoManager(CO_LINE_PARA);
    }
#endif /* CONFIG_DYN_SDO_CONNECTION_MANAGER */

#ifdef CONFIG_NMT_STARTUP_MANAGER
    if (TEST_COLIB_FLAG(COFLAG_NMT_STARTUP_MANAGER))  {

	nmtStartupProcess(CO_LINE_PARA);
    }
#endif /* CONFIG_BOOTUP_MANAGER */
}


/****************************************************************************/
/**
*++ \brief setCobId - set the COB-Id for communication objects in the OD
*-- \brief setCobId - setzt COB-Id für Kommunikationsobjekte am OD-Index
*
*++ With this function the COB-ID for a communication service can be set.
*++ The given COB-ID is stored at the given index and subindex
*++ and will be validated by calling the function setCommPar().
*++ If an error occures at setCommPar()
*++ then the orginal value will be restored.
*-- Mit dieser Funktion kann die COB-ID für einen Dienst gesetzt werden.
*-- Die übergebene COB-Id wird im Objektverzeichnis hinterlegt
*-- und anschliessend über die Funktion setCommPar() validiert.
*-- Wenn beim Validieren mit setCommPar() ein Fehler auftritt,
*-- wird der ursprüngliche Wert im Objektverzeichnis restauriert.
*
* \retval CO_OK
*++ success
*-- Erfolg
* \retval CO_E_xxx
*++ return value from putObj() or setCommPar()
*-- return Wert von putObj() oder setCommPar()
*
*/

RET_T setCobId(
	UNSIGNED16  index,	/**< index of the object dictionary entry */
	UNSIGNED8   subIndex,	/**< subindex of the object dictionary entry */
	UNSIGNED32  cobId	/**< new Cob-Id */
	CO_COMMA_LINE_PARA_DECL	/**< number of CAN line 0..CO_MAX_CAN_LINES-1 */
    )
{
RET_T		retVal;		/* Rückgabewert */
BOOL_T		save = CO_FALSE;/* Orginalwert gespeichert */
UNSIGNED32	oldCobId;	/* old COB-ID */
UNSIGNED32	size;

    /* read actual value */
    retVal = getObjEntry(index, subIndex, (UNSIGNED8 *)&oldCobId, &size,
        CO_TRUE CO_COMMA_LINE_PARA);
    if (retVal == CO_OK)  {
        save = CO_TRUE;
    }

    /* save at OD-index */
    retVal = putObj(index, subIndex, (UNSIGNED8 *)&cobId, 4u, CO_TRUE
        CO_COMMA_LINE_PARA);
    if (retVal != CO_OK)  {
        return(retVal);
    }

    /* validate COB-Id */
    retVal = setCommPar(index, subIndex CO_COMMA_LINE_PARA);
    if (retVal != CO_OK)  {
    /* old value available ? */
        if (save == CO_TRUE)  {
            /* restore old value */
            (void)putObj(index, subIndex, (UNSIGNED8 *)&oldCobId, 4u, CO_TRUE
            CO_COMMA_LINE_PARA);
        }
        return(retVal);
    }

    return(CO_OK);
}


#ifdef CONFIG_CO_WAIT
/****************************************************************************/
/**
*++ \brief coWait - wait for certain time
*-- \brief coWait - wartet für eine bestimmte Zeit
*
*++ This function is useful for single tasking systems in order to get
*++ a synchronous program flow.
*++ It uses the CANopen Timer for realizing of the delay time.
*-- Diese Funktion ist nützlich beim Einsatz von Single Tasking Systemen,
*-- um einem synchronen Programmablauf zu erreichen.
*-- Die Funktion benutzt den CANopen Timer zur Realisierung
*-- der Wartezeit.
* \par
*++ While waiting(sleeping) the CANopen receive queue is tested
*++ and emptied (call CANopen services) if messages do arrive.
*-- In der Wartephase wird die Empfangsqueue überwacht
*-- und CANopen Funktionen gerufen, falls Nachrichten da sind.
*
* \return
*++ nothing
*-- nichts
*
*/

void coWait(
	UNSIGNED32 waitingTime	/**< time to wait 1/10 ms */
	CO_COMMA_GLOBVARS_PARA_DECL
    )
{
TIMER_EVENT_T	waitTimer;	/* timer structure */
#ifdef CONFIG_MULT_LINES
UNSIGNED8	canLine = 0;	/* can line */
UNSIGNED8	line;
#endif /* CONFIG_MULT_LINES */

    /* clear timer structure */
    memset(&waitTimer, (int)0, (size_t)(sizeof(TIMER_EVENT_T)));

    if (addTimerEvent(&waitTimer, waitingTime, 0 CO_COMMA_LINE_PARA) != 0) {
	return;
    }

    while (1)  {

	if (checkActiveTimer(&waitTimer CO_COMMA_LINE_PARA) == CO_FALSE)  {
	    break;
	}

#ifdef CONFIG_MULT_LINES

	for (line = 0; line < CO_ACT_CAN_LINES; line++) {
	    canLine = line;
#else
        {
#endif /* CONFIG_MULT_LINES */


	    FLUSH_MBOX(CO_LINE_PARA);
	}
#ifdef CONFIG_MULT_LINES

	canLine = 0;
#endif /* CONFIG_MULT_LINES */

    }
}
#endif /* CONFIG_CO_WAIT */


#ifdef CONFIG_SDO_CLIENT
/**
*
*/
UNSIGNED32 waitForSdoRes(
	UNSIGNED8 sdoNr
	CO_COMMA_LINE_PARA_DECL	/* number of CAN line 0..CO_MAX_CAN_LINES-1 */
	)
{
UNSIGNED32 retVal = E_SDO_NO_ERROR;
SDO_CLIENT_T	*pSdo;		/* pointer to current sdo */

    /* the setting of the global multiplexor is for more
       convenience and reliability */
    pSdo = searchForClientSdoNr(sdoNr & CO_NONUM_SDO CO_COMMA_LINE_PARA);
    if (pSdo == NULL) {
	retVal = (UNSIGNED32) CO_E_NOT_EXIST;
        return retVal;
    }

    /* test for active transfer */
    while ( pSdo->sdo.state != SDOSTATE_READY ) {
#ifdef CONFIG_MULT_LINES
        for (canLine = 0; canLine < CO_ACT_CAN_LINES; canLine++)
#endif /* CONFIG_MULT_LINES */
        {
            FLUSH_MBOX(CO_LINE_PARA);
        }
    }

    retVal = pSdo->sdoConf;

    return retVal;
}
#endif /* CONFIG_SDO_CLIENT */


#if defined(CONFIG_BLOCK_CRC) || defined(CONFIG_SRDO_PRODUCER) || defined(CONFIG_SRDO_CONSUMER)
/****************************************************************************/
/**
*++ \brief CRC calculation algorithm to verify SDO block transfers
*-- \brief CRC Summen Berechnung für SDO Block Transfers
*
*++ This function calculates the checksum for the SDO block transfer.
*++ The check polynom has the formula \f$x^16 + x^12 + x^5 + 1\f$.
*++ The are two variants to calculate the checksum:
*++ \li 1.
*++    using a crc_table
*++    this will costs 512 bytes for the crc table but the algorythm
*++    is very fast.
*++ \li 2.
*++    calculate the crc itself
*++    the necessary code size is equal to the 1st solution, but the runtime is
*++    up to four time slower than the table variant
*-- Diese Funktion berechnet die Checksumme für den SDO Block Transfer.
*-- Die Prüfsumme wird nach dem Polynom \f$x^16 + x^12 + x^5 + 1\f$
*-- berechnet.
*-- Zur Berechnung stehen 2 Varianten zur Verfügung
*-- \li 1.
*--    Nutzung einer CRC Tabelle.
*--    Diese Tabelle benötigt zusätzlich 512 Bytes für die Tabelle.
*--    Der Algorithmus ist aber sehr schnell.
*-- \li 2.
*--    CRC Berechnung ohne Tabelle.
*--    Der benötigte Algorithmus ist vom Code nicht viel länger,
*--    benötigt aber bis 4 mal so lange an Ausführungszeit.
*
* \return
*++ 16 bit CRC sum
*-- 16 Bit CRC Summe
*/

# ifdef CONFIG_BLOCK_CRC_TABLE

static UNSIGNED16 CO_CONST crc_table[] = {
#  ifdef CONFIG_LSB_CRC
    0x0000, 0x17CE, 0x0FDF, 0x1811, 0x1FBE, 0x0870, 0x1061, 0x07AF,
    0x1F3F, 0x08F1, 0x10E0, 0x072E, 0x0081, 0x174F, 0x0F5E, 0x1890,
    0x1E3D, 0x09F3, 0x11E2, 0x062C, 0x0183, 0x164D, 0x0E5C, 0x1992,
    0x0102, 0x16CC, 0x0EDD, 0x1913, 0x1EBC, 0x0972, 0x1163, 0x06AD,
    0x1C39, 0x0BF7, 0x13E6, 0x0428, 0x0387, 0x1449, 0x0C58, 0x1B96,
    0x0306, 0x14C8, 0x0CD9, 0x1B17, 0x1CB8, 0x0B76, 0x1367, 0x04A9,
    0x0204, 0x15CA, 0x0DDB, 0x1A15, 0x1DBA, 0x0A74, 0x1265, 0x05AB,
    0x1D3B, 0x0AF5, 0x12E4, 0x052A, 0x0285, 0x154B, 0x0D5A, 0x1A94,
    0x1831, 0x0FFF, 0x17EE, 0x0020, 0x078F, 0x1041, 0x0850, 0x1F9E,
    0x070E, 0x10C0, 0x08D1, 0x1F1F, 0x18B0, 0x0F7E, 0x176F, 0x00A1,
    0x060C, 0x11C2, 0x09D3, 0x1E1D, 0x19B2, 0x0E7C, 0x166D, 0x01A3,
    0x1933, 0x0EFD, 0x16EC, 0x0122, 0x068D, 0x1143, 0x0952, 0x1E9C,
    0x0408, 0x13C6, 0x0BD7, 0x1C19, 0x1BB6, 0x0C78, 0x1469, 0x03A7,
    0x1B37, 0x0CF9, 0x14E8, 0x0326, 0x0489, 0x1347, 0x0B56, 0x1C98,
    0x1A35, 0x0DFB, 0x15EA, 0x0224, 0x058B, 0x1245, 0x0A54, 0x1D9A,
    0x050A, 0x12C4, 0x0AD5, 0x1D1B, 0x1AB4, 0x0D7A, 0x156B, 0x02A5,
    0x1021, 0x07EF, 0x1FFE, 0x0830, 0x0F9F, 0x1851, 0x0040, 0x178E,
    0x0F1E, 0x18D0, 0x00C1, 0x170F, 0x10A0, 0x076E, 0x1F7F, 0x08B1,
    0x0E1C, 0x19D2, 0x01C3, 0x160D, 0x11A2, 0x066C, 0x1E7D, 0x09B3,
    0x1123, 0x06ED, 0x1EFC, 0x0932, 0x0E9D, 0x1953, 0x0142, 0x168C,
    0x0C18, 0x1BD6, 0x03C7, 0x1409, 0x13A6, 0x0468, 0x1C79, 0x0BB7,
    0x1327, 0x04E9, 0x1CF8, 0x0B36, 0x0C99, 0x1B57, 0x0346, 0x1488,
    0x1225, 0x05EB, 0x1DFA, 0x0A34, 0x0D9B, 0x1A55, 0x0244, 0x158A,
    0x0D1A, 0x1AD4, 0x02C5, 0x150B, 0x12A4, 0x056A, 0x1D7B, 0x0AB5,
    0x0810, 0x1FDE, 0x07CF, 0x1001, 0x17AE, 0x0060, 0x1871, 0x0FBF,
    0x172F, 0x00E1, 0x18F0, 0x0F3E, 0x0891, 0x1F5F, 0x074E, 0x1080,
    0x162D, 0x01E3, 0x19F2, 0x0E3C, 0x0993, 0x1E5D, 0x064C, 0x1182,
    0x0912, 0x1EDC, 0x06CD, 0x1103, 0x16AC, 0x0162, 0x1973, 0x0EBD,
    0x1429, 0x03E7, 0x1BF6, 0x0C38, 0x0B97, 0x1C59, 0x0448, 0x1386,
    0x0B16, 0x1CD8, 0x04C9, 0x1307, 0x14A8, 0x0366, 0x1B77, 0x0CB9,
    0x0A14, 0x1DDA, 0x05CB, 0x1205, 0x15AA, 0x0264, 0x1A75, 0x0DBB,
    0x152B, 0x02E5, 0x1AF4, 0x0D3A, 0x0A95, 0x1D5B, 0x054A, 0x1284,
#  else /* CONFIG_LSB_CRC */
	0x0000,	0x1021,	0x2042,	0x3063,	0x4084,	0x50a5,	0x60c6,	0x70e7,
	0x8108,	0x9129,	0xa14a,	0xb16b,	0xc18c,	0xd1ad,	0xe1ce,	0xf1ef,
	0x1231,	0x0210,	0x3273,	0x2252,	0x52b5,	0x4294,	0x72f7,	0x62d6,
	0x9339,	0x8318,	0xb37b,	0xa35a,	0xd3bd,	0xc39c,	0xf3ff,	0xe3de,
	0x2462,	0x3443,	0x0420,	0x1401,	0x64e6,	0x74c7,	0x44a4,	0x5485,
	0xa56a,	0xb54b,	0x8528,	0x9509,	0xe5ee,	0xf5cf,	0xc5ac,	0xd58d,
	0x3653,	0x2672,	0x1611,	0x0630,	0x76d7,	0x66f6,	0x5695,	0x46b4,
	0xb75b,	0xa77a,	0x9719,	0x8738,	0xf7df,	0xe7fe,	0xd79d,	0xc7bc,
	0x48c4,	0x58e5,	0x6886,	0x78a7,	0x0840,	0x1861,	0x2802,	0x3823,
	0xc9cc,	0xd9ed,	0xe98e,	0xf9af,	0x8948,	0x9969,	0xa90a,	0xb92b,
	0x5af5,	0x4ad4,	0x7ab7,	0x6a96,	0x1a71,	0x0a50,	0x3a33,	0x2a12,
	0xdbfd,	0xcbdc,	0xfbbf,	0xeb9e,	0x9b79,	0x8b58,	0xbb3b,	0xab1a,
	0x6ca6,	0x7c87,	0x4ce4,	0x5cc5,	0x2c22,	0x3c03,	0x0c60,	0x1c41,
	0xedae,	0xfd8f,	0xcdec,	0xddcd,	0xad2a,	0xbd0b,	0x8d68,	0x9d49,
	0x7e97,	0x6eb6,	0x5ed5,	0x4ef4,	0x3e13,	0x2e32,	0x1e51,	0x0e70,
	0xff9f,	0xefbe,	0xdfdd,	0xcffc,	0xbf1b,	0xaf3a,	0x9f59,	0x8f78,
	0x9188,	0x81a9,	0xb1ca,	0xa1eb,	0xd10c,	0xc12d,	0xf14e,	0xe16f,
	0x1080,	0x00a1,	0x30c2,	0x20e3,	0x5004,	0x4025,	0x7046,	0x6067,
	0x83b9,	0x9398,	0xa3fb,	0xb3da,	0xc33d,	0xd31c,	0xe37f,	0xf35e,
	0x02b1,	0x1290,	0x22f3,	0x32d2,	0x4235,	0x5214,	0x6277,	0x7256,
	0xb5ea,	0xa5cb,	0x95a8,	0x8589,	0xf56e,	0xe54f,	0xd52c,	0xc50d,
	0x34e2,	0x24c3,	0x14a0,	0x0481,	0x7466,	0x6447,	0x5424,	0x4405,
	0xa7db,	0xb7fa,	0x8799,	0x97b8,	0xe75f,	0xf77e,	0xc71d,	0xd73c,
	0x26d3,	0x36f2,	0x0691,	0x16b0,	0x6657,	0x7676,	0x4615,	0x5634,
	0xd94c,	0xc96d,	0xf90e,	0xe92f,	0x99c8,	0x89e9,	0xb98a,	0xa9ab,
	0x5844,	0x4865,	0x7806,	0x6827,	0x18c0,	0x08e1,	0x3882,	0x28a3,
	0xcb7d,	0xdb5c,	0xeb3f,	0xfb1e,	0x8bf9,	0x9bd8,	0xabbb,	0xbb9a,
	0x4a75,	0x5a54,	0x6a37,	0x7a16,	0x0af1,	0x1ad0,	0x2ab3,	0x3a92,
	0xfd2e,	0xed0f,	0xdd6c,	0xcd4d,	0xbdaa,	0xad8b,	0x9de8,	0x8dc9,
	0x7c26,	0x6c07,	0x5c64,	0x4c45,	0x3ca2,	0x2c83,	0x1ce0,	0x0cc1,
	0xef1f,	0xff3e,	0xcf5d,	0xdf7c,	0xaf9b,	0xbfba,	0x8fd9,	0x9ff8,
	0x6e17,	0x7e36,	0x4e55,	0x5e74,	0x2e93,	0x3eb2,	0x0ed1,	0x1ef0,
#  endif /* CONFIG_LSB_CRC */
  };

/*
*
* crc16Calc - calculate 16 bit CRC
* with table
*
* \internal
*
* \retval
*++ calculated CRC
*-- berechnete CRC
*
*/
UNSIGNED16 crc16Calc(
	UNSIGNED8 *buf,		/**< Start adress of data set in memory */
	UNSIGNED16 crc,		/**< value to begin with CRC calculation */
	UNSIGNED32 lng		/**< number of bytes to include in calculation*/
#  ifdef CONFIG_16BIT_CPU
	,BOOL_T	numeric		/**< data at buffer are numeric */
#  endif /* CONFIG_16BIT_CPU */
    )
{
UNSIGNED16	tcrc;		/* temporary U16 value */
#  ifdef CONFIG_16BIT_CPU
UNSIGNED8	val;		/* temporary U8 value */
UNSIGNED8	odd = 0;  	/* odd value */
#  endif /* CONFIG_16BIT_CPU */

    tcrc = crc;
    while (lng--) {
#  ifdef CONFIG_16BIT_CPU
	/* if numeric data field */
	if (numeric == CO_TRUE)  {
	    /* if odd address use high part */
	    if (odd != 0)  {
		val = (*(UNSIGNED16 *)buf >> 8) & 0xff;
		buf++;
		odd = 0;
	    } else {
		/* if even address use low part */
		val = *buf & 0xff;
		/* address is only incremented after odd addresses */
		odd ++;
	    }
	} else {
	    val = *buf & 0xff;
	    buf++;
	}
#   ifdef CONFIG_LSB_CRC
	tcrc = ((tcrc >> 8) & 0xff) ^ crc_table[(tcrc ^ val) & 0xff];
#   else /* CONFIG_LSB_CRC */
	tcrc = (tcrc << 8) ^ crc_table[ ((tcrc >> 8) & 0xff) ^ val];
#   endif /* CONFIG_LSB_CRC */

#  else /* CONFIG_16BIT_CPU */

#   ifdef CONFIG_LSB_CRC
	tcrc = ((tcrc >> 8) & 0xff) ^ crc_table[(tcrc ^ *buf++) & 0xff];
#   else /* CONFIG_LSB_CRC */
	tcrc = (tcrc << 8) ^ crc_table[ ((tcrc >> 8) & 0xff) ^ *buf++];
#   endif /* CONFIG_LSB_CRC */

#  endif /* CONFIG_16BIT_CPU */
    }
    return tcrc;
}

# else /* CONFIG_BLOCK_CRC_TABLE */

/***************************************************************************
*
* crc16Calc - calculate 16 bit CRC
* without table
*
* \internal
*
* CONFIG_LSB_CRC shift rights - LSB first
*/

UNSIGNED16  crc16Calc(
	UNSIGNED8 *buf,		/**< Start adress of data set in memory */
	UNSIGNED16 crc,		/**< value to begin with CRC calculation */
	UNSIGNED32 lng		/**< number of bytes to include in calculation*/
#  ifdef CONFIG_16BIT_CPU
	,BOOL_T	numeric		/**< data at buffer are numeric */
#  endif /* CONFIG_16BIT_CPU */
    )
{
#define M16	0xA001		/* crc-16 mask (x^16 + x^15 +x^2 + 1) */
#define MTT	0x1021		/* crc-ccitt mask (x^16 + x^12 + x^5 + 1) */
UNSIGNED8	carry;
UNSIGNED8	cnt;
#  ifdef CONFIG_LSB_CRC
UNSIGNED8	b;
#  else /* CONFIG_LSB_CRC */
UNSIGNED16	b;
#  endif /* CONFIG_LSB_CRC */
#  ifdef CONFIG_16BIT_CPU
UNSIGNED8	val;
UNSIGNED8	odd = 0;
#  endif /* CONFIG_16BIT_CPU */

    while (lng)
    {
#  ifdef CONFIG_16BIT_CPU
	/* if numeric data field */
	if (numeric == CO_TRUE)  {
	    /* if odd address use high part */
	    if (odd != 0)  {
#  ifdef CONFIG_LSB_CRC
		b = (*(UNSIGNED16 *)buf >> 8) & 0xff;
#  else /* CONFIG_LSB_CRC */
		b = *(UNSIGNED16 *)buf & 0xff00;
#  endif /* CONFIG_LSB_CRC */
		buf++;
		odd = 0;
	    } else {
		/* if even address use low part */
#  ifdef CONFIG_LSB_CRC
		b = *buf & 0xff;
#  else /* CONFIG_LSB_CRC */
		b = (*buf & 0xff) << 8;
#  endif /* CONFIG_LSB_CRC */
		/* address is only incremented after odd addresses */
		odd ++;
	    }
	} else
#  endif /* CONFIG_16BIT_CPU */
	{
#  ifdef CONFIG_LSB_CRC
	    b = *buf & 0xff;
#  else /* CONFIG_LSB_CRC */
	    b = ((UNSIGNED16)(*buf & 0xff)) << 8;
#  endif /* CONFIG_LSB_CRC */
	    buf++;
	}
	/* b = (UNSIGNED8) *buf++; */
	crc ^= b;
	cnt = 0;
	while (cnt < 8)
	{
#  ifdef CONFIG_LSB_CRC
	    carry =(UNSIGNED8) crc & 0x01;
	    crc >>= 1;
#  else /* CONFIG_LSB_CRC */
	    carry = (crc & 0x8000) ? 1 : 0;
	    crc <<= 1;
#  endif /* CONFIG_LSB_CRC */
	    if (carry) {
		crc ^= MTT;
	    }
	    cnt++;
	}
	lng--;
    }
    return (crc);
}
# endif /* CONFIG_BLOCK_CRC_TABLE */
#endif /* (CONFIG_BLOCK_CRC) || defined(CONFIG_SRDO_PRODUCER) || defined(CONFIG_SRDO_CONSUMER */


#ifdef CONFIG_8BIT_CRC
/****************************************************************************/
/**
*++ \brief CRC calculation algorithm
*-- \brief CRC Summen Berechnung
*
*++ This function calculates an 8-bit checksum
*++ The check polynom has the formula
*-- Diese Funktion berechnet eine 8-Bit Prüfsumme.
*-- Die Prüfsumme wird nach dem Polynom
* \f$x^8 + x^3 + x^2 + x + 1\f$
*-- berechnet.
*
*++ \internal
*++    The calculation is performed in little-endian format.
*-- \internal
*--    Die Berechnung wird im little-endian Format durchgeführt.
*
* \return
*++ 8 bit CRC sum
*-- 8 Bit CRC Summe
*/
UNSIGNED8 crc8Calc(
	UNSIGNED8 *buf,		/**< Start adress of data set in memory */
	UNSIGNED8 crc,		/**< value to begin with CRC calculation */
	UNSIGNED32 lng		/**< number of bytes to include in calculation*/
    )
{
#define CRC8_POL 0x4F
UNSIGNED8 carry, b;
UNSIGNED8 cnt;

    while (lng > 0) {

	b = (UNSIGNED8) *buf++;
	crc ^= b;
	cnt = 0;
	while (cnt < 8)
	{
	    carry =(UNSIGNED8) crc & 0x80;
	    crc <<= 1;
	    if (carry) {
	    	crc ^= CRC8_POL;
	    }
	    cnt++;
	}
	lng--;
    }
    return (crc);
}
#endif /* CONFIG_8BIT_CRC */


#ifdef CONFIG_16BIT_CPU
/***************************************************************************
*
* unpack_memcpy - special memcpy from packed values to unpacked values
*
*
* \internal
*
* Only valid for 16bit CPUs
* Copy wordwise to bytewise if numeric is true
* (p.e. from CAN-buffer to internal variables)
*
* \retval
*	nothing
*/
void unpack_memcpy(
	UNSIGNED8 *dest,	/**< destination area (bytewise) */
	UNSIGNED8 *src,		/**< source area (wordwise) */
	UNSIGNED32 size,	/**< size in bytes */
	UNSIGNED8 numeric	/**< numeric flag */
    )
{
UNSIGNED32	i;		/* loop counter */

    if (numeric != 0)  {
        for (i = 0; i < size; i++)  {
            if ((i & 1) != 0) {
                dest[i] = ((UNSIGNED16 *)src)[i >> 1] >> 8;
            }
            else  {
                dest[i] = ((UNSIGNED16 *)src)[i >> 1] & 0xFF;
            }
        }
    }
    else  {
        memcpy((void *)dest, (void *)src, (size_t)size);
    }
}


/***************************************************************************
*
* pack_memcpy - special memcpy from unpacked values to packed values
*
*
* \internal
*
* Only valid for 16bit CPUs
* Copy bytewise to wordwise if numeric is true
* (p.e. from CAN-buffer to internal variables)
*
* parameter:
*	numeric:	= 0	- no numeric value
*			!= 0	- numeric value
*			= 0xaa	- numeric and signed value if size == 1
* \retval
*	nothing
*/

void pack_memcpy(
	UNSIGNED8 *dest,	/**< destination area (wordwise) */
	UNSIGNED8 *src,		/**< source area (bytewise) */
	UNSIGNED32 size,	/**< size in bytes */
	UNSIGNED8 numeric	/**< numeric flag */
    )
{
UNSIGNED32	i;		/* loop counter */
UNSIGNED16	u16;

    if (numeric != 0)  {
	/* special handling for 1 byte signed values */
	if (size == 1)  {
	    if (numeric == CO_8BIT_SIGNED_VAL) {
		/* INTEGER8 */
		if((*(UNSIGNED16 *)src & 0x80) == 0x80) {
		    *(UNSIGNED16 *)dest = *(UNSIGNED16 *)src | 0xFF00;
		} else {
		    *(UNSIGNED16 *)dest = *(UNSIGNED16 *)src & 0x00FF;
		}
	    } else {
		/* UNSIGNED8 */
		*(UNSIGNED16 *)dest = *(UNSIGNED16 *)src & 0x00FF;
	    }

	/* for faster access default cases for 16 and 32 bit vars */
	} else if (size == 2) {
	    u16 = (*src & 0xff) | (*(src + 1) << 8);
	    (*(UNSIGNED16 *)dest) = u16;

	} else if (size == 4) {
	    u16 = (*src & 0xff) | (*(src + 1) << 8);
	    (*(UNSIGNED16 *)dest) = u16;
	    u16 = *(src + 2) & 0xff | (*(src + 3) << 8);
	    (*(UNSIGNED16 *)(dest + 1)) = u16;

	} else {
	    /* size != 1 - normal handling */
	    for (i = 0; i < size; i++)  {
		/* avoid interrupting write values use a help variable */
		if ((i & 1) != 0) {
		    u16 = ((UNSIGNED16 *)dest)[i >> 1];
		    u16 &= 0xFF;
		    u16 |= src[i] << 8;
		    ((UNSIGNED16 *)dest)[i >> 1] = u16;
		} else  {
		    u16 = ((UNSIGNED16 *)dest)[i >> 1];
		    u16 &= 0xFF00;
		    u16 |= src[i] & 0xFF;
		    ((UNSIGNED16 *)dest)[i >> 1] = u16;
		}
	    }
	}
    } else  {
	/* string */
	memcpy((void *)dest, (void *)src, (size_t)size);
    }
}


# ifdef CONFIG_SEG_SDO
/***************************************************************************
*
* pack_oddmemcpy - special memcpy from unpacked values to packed values
*			for odd adresses or sizes
*
*
* \internal
*
* Only valid for 16bit CPUs
* Copy bytewise to wordwise with odd byte counts
* If odd is set, copy first byte of src-address to upper byte of dest-address
*
* \retval
*	actual dest pointer as return value
*	odd
*/

UNSIGNED8 *pack_oddmemcpy(
	UNSIGNED8 *dest,	/**< destination area (wordwise) */
	UNSIGNED8 *src,		/**< source area (bytewise) */
	UNSIGNED32 size,	/**< size in bytes */
	BOOL_T	*odd		/**< start at odd address */
    )
{
UNSIGNED16	u16;

    /* start at odd address ? */
    if (*odd == CO_TRUE)  {
        /* copy first byte of src-address to upper byte of dest-address */
        (*(UNSIGNED16 *)dest) &= 0x00FF;
        (*(UNSIGNED16 *)dest) |= ((UNSIGNED16)*src) << 8;
        dest++;
        src++;
        size--;
    }
    /* now copy all even bytes */
    CO_PACK_MEMCPY(dest, src, size, CO_TRUE);
    dest += (size >> 1);

    /* copy last odd byte */
    if ((size & 1) != 0)  {
        *odd = CO_TRUE;
        u16 = *(UNSIGNED16 *)dest;
        u16 &= 0xFF00;
        u16 |= *(src + size - 1);
        (*(UNSIGNED16 *)dest) = u16;
    }
    else {
        *odd = CO_FALSE;
    }

    return(dest);
}


/***************************************************************************
*
* unpack_oddmemcpy - special memcpy from packed values to unpacked values
*			for odd adresses or sizes
*
*
* \internal
*
* Only valid for 16bit CPUs
* Copy wordwise to bytewise with odd byte counts
* If odd is set, copy lower byte of src-address to first dest address
*
* \retval
*	actual src pointer as return value
*	odd
*/

UNSIGNED8 *unpack_oddmemcpy(
	UNSIGNED8 *dest,	/**< destination area (bytewise) */
	UNSIGNED8 *src,		/**< source area (wordwise) */
	UNSIGNED32 size,	/**< size in bytes */
	BOOL_T	  *odd		/**< start at odd address */
    )
{
    /* start at odd address ? */
    if (*odd == CO_TRUE)  {
        /* copy first byte of src-address to upper byte of dest-address */
        *dest = *((UNSIGNED16 *)src) >> 8;
        src++;
        dest++;
        size--;
    }
    /* now copy all even bytes */
    CO_UNPACK_MEMCPY(dest, src, size, CO_TRUE);
    src += (size >> 1);

    /* copy last odd byte */
    if ((size & 1) != 0)  {
        *(dest + size - 1) = (*(UNSIGNED16 *)src) & 0xff;
        *odd = CO_TRUE;
    }
    else {
        *odd = CO_FALSE;
    }

    return(src);
}

# endif /* CONFIG_SEG_SDO */
#endif /* CONFIG_16BIT_CPU */


#ifdef CONFIG_FAST_SORT
/***************************************************************************
*
* sortCobIdList - sort cob-ids in a index list
*
*
* \internal
*
* This function creates a sorted index list for the cobs
* at the given cob list
*
* \retval
*	none
*/

void sortCobIdList(
	UNSIGNED8	pIdxList[],	/* pointer to index list */
	COB_T		**ppCob,	/* pointer to first COB */
	UNSIGNED16	elementSize,	/* size of one element */
	UNSIGNED8	listLen		/* actual list len */
    )
{
UNSIGNED8	i;
UNSIGNED8	cnt;
UNSIGNED8	exchange;
COB_IDENT_T	val, nextVal;
UNSIGNED8	tmpVal;
UNSIGNED8	*ptr;

    /* delete sorted list */
    for (cnt = 0u; cnt < listLen; cnt++)  {
        pIdxList[cnt] = cnt;
    }

    /* bubblesort algo */
    do {
        exchange = 0u;
        /* for (i = 0 ; i < N-1 ; i++){ */
        i = 0u;
        while (i < (listLen - 1u))  {
            /* if (feld[i] > feld[i+1]) { */
            ptr = (UNSIGNED8 *)ppCob + pIdxList[i] * elementSize;
            val = (*(COB_T **)ptr)->cobId;

            ptr = (UNSIGNED8 *)ppCob + pIdxList[i + 1u] * elementSize;
            nextVal = (*(COB_T **)ptr)->cobId;

            if (val > nextVal)  {
                tmpVal = pIdxList[i];
                pIdxList[i] = pIdxList[i + 1u];
                pIdxList[i + 1u] = tmpVal;

                exchange = 1u;
            }
            i++;
        }
    } while (exchange == 1u);
}


/***************************************************************************
*
* sortNodeIdList - sort node-ids in a index list
*
*
* \internal
*
* This function creates a sorted index list for all node-ids
* at the given list
*
* \retval
*	none
*/
void sortNodeIdList(
	UNSIGNED8	pIdxList[],	/* pointer to index list */
	UNSIGNED8	*pNodeId,	/* pointer to first node at plist */
	UNSIGNED16	elementSize,	/* size of one element */
	UNSIGNED8	listLen		/* actual list len */
    )
{
UNSIGNED8	i;
UNSIGNED8	cnt;
UNSIGNED8	exchange;
UNSIGNED8	val, nextVal, tmpVal;

    /* delete sorted list */
    for (cnt = 0u; cnt < listLen; cnt++)  {
        pIdxList[cnt] = cnt;
    }

    /* bubblesort algo */
    do {
        exchange = 0u;
        /* for (i = 0 ; i < N-1 ; i++){ */
        i = 0u;
        while (i < (listLen - 1u))  {
            /* if (feld[i] > feld[i+1]) { */
            val = *(pNodeId + pIdxList[i] * elementSize);
            nextVal = *(pNodeId + pIdxList[i + 1u] * elementSize);

            if (val > nextVal)  {
                tmpVal = pIdxList[i];
                pIdxList[i] = pIdxList[i + 1u];
                pIdxList[i + 1u] = tmpVal;

                exchange = 1u;
            }
            i++;
        }
    } while (exchange == 1u);
}
#endif /* CONFIG_FAST_SORT */

/*______________________________________________________________________EOF_*/
