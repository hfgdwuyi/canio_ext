/*
 * cdriver - common CAN driver functions
 *
 * Copyright (c) 1997-2017 port GmbH Halle (Saale)
 *------------------------------------------------------------------
 */

/**
*  \file cdriver.c
*++ Common CAN driver functions
*-- Allgemeine Funktionen für den CAN-Treiber
*  \author port GmbH Halle (Saale)
*
*++ This module provides some functions used by all
*++ drivers for CAN controllers.
*-- Dieses Modul stellt Funktionen bereit, welche von allen
*-- Treibern für CAN Controller benutzt werden.
*
*++ This module defines also all global variables used within the driver.
*-- Dieses Module definiert auch alle globalen Variablen, welche in
*-- den Treibern benutzt werden.
*
*++ Mostly these are functions for manipulating the send
*++ and receive queue.
*-- Die meisten dieser Funktionen manipulieren den Sende- und
*-- Empfangspuffer.
*
*
*/
/**
* \defgroup bufferhandling CAN driver buffer handling
*/
/**
* \def CONFIG_COLIB_BUFFER
*++ Default Driver Package buffer handling is used.
*-- Standard Treiber Bufferhandling wird benutzt.
* \ingroup bufferhandling
*/
/**
* \def CONFIG_COLIB_FLUSHMBOX
*++ enable standard embedded FlushMbox() function
*-- aktiviert Standard Embedded FlushMbox() Funktion
*
*++ Please read also:
*-- Weitere Hinweise unter:
*	- \c ::CONFIG_FLUSHMBOX_READ_ALL,
* 	- \c ::CONFIG_FLUSHMBOX_READ_ONLY_ONE,
*	- \c ::CONFIG_FLUSHMBOX_READ_MAXMSG
*/
/**
* \def CONFIG_FLUSHMBOX_READ_ALL
*++ Read the complete Message buffer and all messages,
*++ that new come in in this time.
*-- In FlushMbox() werden alle bereits empfangenen Messages
*-- und alle während dieser Zeit zusätzlich empfangenen Messages
*-- verarbeitet.
*/
/**
* \def CONFIG_FLUSHMBOX_READ_ONLY_ONE
*++ Read only one Message from the buffer at every FlushMbox() call.
*++ FlushMbox() have to call more often!
*-- Es wird nur jeweils eine Message je FlushMbox() Aufruf verarbeitet.
*-- FlushMbox() sollte daher sehr häufig aufgerufen werden.
*/
/**
* \def CONFIG_FLUSHMBOX_READ_MAXMSG
*++ Read a maximum of Messages from the buffer.
*++ This is the default setting with the Value of the Receive Buffer count.
*-- FlushMbox() verarbeitet eine vorgegebene maximale Anzahl von Messages.
*-- Der Defaultwert ist die Anzahl der Receivebuffer.
*/
/*
* \def CONFIG_CAN_TEST_VALID_COB
*   currently not used!!
*++ enable the validCobId() function
*-- aktiviert die validCobId() Funktion
*/
/**
* \def CONFIG_COB_ARRAY
*++ COB Information is organized within an array
*++ default: enabled
*-- Die COB Information wird in einem Array verwaltet.
*-- default: aktiv
*/
/**
* \def CONFIG_COB_NUMBERS
*++ Number of reserved Communication objects (COB)
*++ sum of CANopen Service COBs (CONFIG_CAN_OBJECTS) and debug objects
*-- Anzahl der reservierten Kommunikationsobjekte (COB)
*-- Summme der CANopen Service COBs (CONFIG_CAN_OBJECTS) und Debugobjekte
*/
/**
* \def CONFIG_CAN_FLUSHMBOX_DISABLE_CPUINT
* internal setting - default not used
* In FlushMbox during the switching to the next software receive buffer
* the CPU Interrupt can be disabled. In the default implementation this
* setting is not needed.
*/
/**
* \def CONFIG_CAN_FLUSHMBOX_DISABLE_INT
* internal setting - default not used
* In FlushMbox during the readout of the software receive buffer
* the CAN Interrupt can be disabled. In the default implementation this
* setting is not needed.
*/

#ifdef DOXYGEN
	/* documentation should be SingleLine */
#  undef CONFIG_REDUNDANCY_SUPPORT
#  define CONFIG_COLIB_BUFFER
#  define CONFIG_COLIB_FLUSHMBOX

#  define CONFIG_FLUSHMBOX_READ_ALL
#  define CONFIG_FLUSHMBOX_READ_ONLY_ONE
#  define CONFIG_FLUSHMBOX_READ_MAXMSG
#  define CONFIG_DRIVER_FAST_SORT
#  define CONFIG_CAN_FLUSHMBOX_DISABLE_INT
#  define CONFIG_CAN_FLUSHMBOX_DISABLE_CPUINT
#endif /* DOXYGEN */


/* prototype for memcpy(), memmove(), memset() */
#include <string.h>
#include <stdlib.h>

#define DEF_HW_PART
#include <cal_conf.h>

#include <co_stru.h>
#include <co_flag.h>
#include <co_mcpy.h>
#include <co_drv.h>
#include <co_drvif.h>
/* #include <co_timer.h> */

#ifdef CONFIG_REDUNDANCY_SUPPORT
#  include <co_redcy.h>
#endif /* CONFIG_REDUNDANCY_SUPPORT */

#ifdef CONFIG_NO_GLOBAL_VARS
	/* special driver header -> user adaptation */
#  include <glob_drv.h>
#  include <co_init.h>
#else /* CONFIG_NO_GLOBAL_VARS */
#  include <cdriver.h>
#endif /* CONFIG_NO_GLOBAL_VARS */

#if defined(CONFIG_DRIVER_TEST) || defined(CONFIG_TIME_TEST)
#  include <stdio.h>
	/* need for printf() and CO_SET_BIT() definitions */
#  include <examples.h>
#endif /* CONFIG_DRIVER_TEST || CONFIG_TIME_TEST */

#ifdef CONFIG_SYNC_CONSUMER
#  include <co_sync.h>
#endif /* CONFIG_SYNC_CONSUMER */


#ifdef CONFIG_RCS_IDENT
static char _rcsid_c[] = "$Id: cdriver.c,v 2.93 2013/08/26 12:24:17 hes Exp $";
#endif

#ifdef CONFIG_REDUNDANCY_SUPPORT
#  ifndef DISABLE_TX_MESSAGES
#    define DISABLE_TX_MESSAGES(line)
#  endif /* DISABLE_TX_MESSAGES */
#endif /* CONFIG_REDUNDANCY_SUPPORT */

#ifdef CONFIG_COLIB_FLUSHMBOX
   /* check for setting of FlushMbox() */
#  ifdef CONFIG_FLUSHMBOX_READ_ALL
#    define TMP_FLUSHMBOX_OK
#  endif /* CONFIG_FLUSHMBOX_READ_ALL */
#  ifdef CONFIG_FLUSHMBOX_READ_ONLY_ONE
#    define TMP_FLUSHMBOX_OK
#  endif /* CONFIG_FLUSHMBOX_READ_ONLY_ONE */
#  ifdef CONFIG_FLUSHMBOX_READ_MAXMSG
#    define TMP_FLUSHMBOX_OK
#  endif /* CONFIG_FLUSHMBOX_READ_MAXMSG */
#  ifndef TMP_FLUSHMBOX_OK
    /* set default */
#    define CONFIG_FLUSHMBOX_READ_MAXMSG CONFIG_RX_BUFFER_SIZE
#  endif /* TMP_FLUSHMBOX_OK  */
#endif /* CONFIG_COLIB_FLUSHMBOX */

/*---- const -------------------------------------------------*/
/**
* \var co_bittiming_table
*++ Table of supported CAN bitrates as defined in CANopen.
*++ The order follows the specification DSP 305.
*-- Tabelle aller in CANopen verfügbaren CAN-Bitraten.
*-- Die Reihenfolge entspricht dem DSP 305.
*/
CO_CONST UNSIGNED16 co_bittiming_table[9] = {
	1000, 800, 500, 250, 125, 100, 50, 20, 10
};

/*---- externals -------------------------------------------------*/

/*---- global variables ------------------------------------------*/
#if defined(CONFIG_CPU_FAMILY_8051) && defined(CONFIG_CAN_FAMILY_82527)
/* ID shifting from internal registers to normal int
 * is not done within the ISR but here.
 * (It's time consuming with an 8051)
 */
#  define ID_SHIFT_WIDTH 5
#endif /* (CONFIG_CPU_FAMILY_8051) && (CONFIG_CAN_FAMILY_82527) */

#ifndef ID_SHIFT_WIDTH
#  define ID_SHIFT_WIDTH 0
#endif /* ID_SHIFT_WIDTH */

/**
* \def COB_NUMBER_CNT
*++ sum of all used COB entries
*-- Summe aller benutzten COB Einträge.
* \n Default Singleline mode:                     \n
* Designtool set #CONFIG_CAN_OBJECTS              \n
* # define #CONFIG_COB_NUMBERS #CONFIG_CAN_OBJECTS \n
* # define COB_NUMBER_CNT #CONFIG_COB_NUMBERS     \n
*
* \hideinitializer
*/
#ifdef CONFIG_NO_GLOBAL_VARS
#  define COB_NUMBER_CNT	CONFIG_COB_NUMBERS
#else /* CONFIG_NO_GLOBAL_VARS */

/**
* \var coCanFlags
* \ingroup CanDriverFlags
*
*++ CAN spezific state flags
*-- CAN spezifische Status Flags
*/
UNSIGNED8 coCanFlags CO_REDCY_PARA_ARRAY_DEF;

/**
* \var coCanDriverState
* \ingroup CanDriverFlags
*
*++ internal CAN driver state
*-- interner CAN Treiber Status
*/
UNSIGNED8 coCanDriverState CO_REDCY_PARA_ARRAY_DEF;


#  ifdef CONFIG_COLIB_BUFFER
/* !!!!!
 * With Keil-C and modC515C  hardware
 * place the transmit and receive buffers at absolute addresses
 *
 * not necessary, but uses this place and saves space
 * while testing with 32 K RAM, the C515c has 2Kbyte XRAM from
 * 0xf800 to 0xffff
 + 0xf800 and 0xf900  are two possible addresses for RX/TX queues
 */
#    if defined(CONFIG_CPU_TYPE_C515C) && defined(__C51__)
    BUFFER_ENTRY_T pRX_Buffer[CONFIG_RX_BUFFER_SIZE] CO_LINE_PARA_ARRAY_DEF _at_ 0xf800;
    BUFFER_ENTRY_T pTX_Buffer[CONFIG_TX_BUFFER_SIZE] CO_LINE_PARA_ARRAY_DEF _at_ 0xf900;
#    else /* defined (CONFIG_CPU_TYPE_C515C) && defined(__C51__) */
/** Software Transmit Buffer \ingroup bufferhandling */
    BUFFER_ENTRY_T CO_MEM_RAM pTX_Buffer[CONFIG_TX_BUFFER_SIZE] \
    						CO_REDCY_PARA_ARRAY_DEF;
/** Software Receive Buffer \ingroup bufferhandling */
    BUFFER_ENTRY_T  CO_MEM_RAM pRX_Buffer[CONFIG_RX_BUFFER_SIZE] \
    						CO_REDCY_PARA_ARRAY_DEF;
#    endif /* defined (CONFIG_CPU_TYPE_C515C) && defined(__C51__) */


/**
* \var bRX_WriteIndex
* \ingroup bufferhandling
*++ next free WriteIndex of Receive Buffer located in the fastest memory
*-- nächster freier Schreibindex im Receive Buffer
*/
/*VOLATILE*/ BUFFER_INDEX_T CO_MEM_QUICKRAM bRX_WriteIndex \
						CO_REDCY_PARA_ARRAY_DEF;

/**
* \var bRX_ReadIndex
* \ingroup bufferhandling
*++ next full ReadIndex of Receive Buffer located in the fastest memory
*-- nächster belegter Leseindex im Receive Buffer.
*/
/*VOLATILE*/ BUFFER_INDEX_T CO_MEM_QUICKRAM bRX_ReadIndex  \
						CO_REDCY_PARA_ARRAY_DEF;

/**
* \var bTX_WriteIndex
* \ingroup bufferhandling
*++ next free WriteIndex of Transmit Buffer located in the fastest memory
*-- nächster freier Schreibindex im Transmit Buffer.
*/
/*VOLATILE*/ BUFFER_INDEX_T CO_MEM_QUICKRAM bTX_WriteIndex \
						CO_REDCY_PARA_ARRAY_DEF;

/**
* \var bTX_ReadIndex
* \ingroup bufferhandling
++ next full ReadIndex of Transmit Buffer located in the fastest memory
-- nächster belegter Leseindex im Transmit Buffer
*/
/*VOLATILE*/ BUFFER_INDEX_T CO_MEM_QUICKRAM bTX_ReadIndex \
						CO_REDCY_PARA_ARRAY_DEF;


#  endif /* CONFIG_COLIB_BUFFER */

#  ifdef CONFIG_COB_ARRAY
#    ifdef CONFIG_COB_NUMBERS
#    else /* CONFIG_COB_NUMBERS */
#      error "Number of CAN Objects is absent (CONFIG_COB_NUMBERS)!"
#    endif /* CONFIG_COB_NUMBERS */


#    ifdef CONFIG_DYN_MEM_ALLOC
#      define COB_NUMBER_CNT	co_maxCobCnt
#    else /* CONFIG_DYN_MEM_ALLOC */
#      define COB_NUMBER_CNT	CONFIG_COB_NUMBERS
#    endif /* CONFIG_DYN_MEM_ALLOC */


#    ifdef CONFIG_NO_GLOBAL_VARS
#    else /* CONFIG_NO_GLOBAL_VARS */
/** CAN object list */

#      ifdef CONFIG_DYN_MEM_ALLOC
COB_T CO_MEM_RAM *p_cobList[1];
UNSIGNED16	co_maxCobCnt;
#      else /* CONFIG_DYN_MEM_ALLOC */
COB_T CO_MEM_RAM cobList[CONFIG_COB_NUMBERS];
#      endif /* CONFIG_DYN_MEM_ALLOC */


/**
*++ next free entry in ::cobList
*-- nächster freier Eintrag in ::cobList
*/
COB_ARRAY_INDEX_T CO_MEM_RAM cobListNextEntry CO_REDCY_PARA_ARRAY_DEF;

#      ifdef CONFIG_DRIVER_FAST_SORT
/**
*++ last valid list entry in ::cobIndexList
*-- Letzter gültiger Eintrag in ::cobIndexList
*/
COB_ARRAY_INDEX_T CO_MEM_RAM cobIdxListLen CO_REDCY_PARA_ARRAY_DEF;

/**
*++ index list for binary search in ::cobList
*-- Index Liste für die binäre Suche innerhalb von ::cobList
*/
#        ifdef CONFIG_DYN_MEM_ALLOC
COB_ARRAY_INDEX_T CO_MEM_RAM *p_cobIndexList[1];
#        else /* CONFIG_DYN_MEM_ALLOC */
static COB_ARRAY_INDEX_T CO_MEM_RAM cobIndexList[CONFIG_COB_NUMBERS];
#        endif /* CONFIG_DYN_MEM_ALLOC */
#      endif /* CONFIG_DRIVER_FAST_SORT */

#      if defined(CONFIG_MULT_LINES) || defined(CONFIG_REDUNDANCY_SUPPORT)
/**
*++ number of COB elements of every canLine
*-- Anzahl von COB Einträgen je CAN Linie
*/
#        ifdef CONFIG_DYN_MEM_ALLOC
static COB_ARRAY_INDEX_T cobListLineCnts CO_REDCY_PARA_ARRAY_DEF;
#        else /* CONFIG_DYN_MEM_ALLOC */
static CO_CONST COB_ARRAY_INDEX_T cobListLineCnts CO_REDCY_PARA_ARRAY_DEF =
					{ CONFIG_COB_NUMBERS_LINECFG };
#        endif /* CONFIG_DYN_MEM_ALLOC */
/**
*++ startoffset of canLine specific COB and COB-Index entries
*-- startoffset der COB Einträge je CAN Linie in ::cobList und ::cobIndexList
*/
static COB_ARRAY_INDEX_T CO_MEM_RAM cobListLineOffs CO_REDCY_PARA_ARRAY_DEF;
#      endif /* defined(CONFIG_MULT_LINES) || defined(CONFIG_REDUNDANCY_SUPPORT) */
#    endif /* CONFIG_NO_GLOBAL_VARS */
#  endif /* CONFIG_COB_ARRAY */

#  ifdef CONFIG_COLIB_FLUSHMBOX
/*
* buffer for CAN message
*
* Some compilers have problems, if this Message buffer is on Stack
* in FlushMbox(). A second thing is, that different lines could be
* call in different tasks (not allowed, but sometimes did).
*/
static CAN_MSG_T CO_MEM_RAM CAN_Msg CO_REDCY_PARA_ARRAY_DEF;

#  endif /* CONFIG_COLIB_FLUSHMBOX */
#endif /* CONFIG_NO_GLOBAL_VARS */

/*---- local functions --------------------------------------------*/

#ifdef CONFIG_DRIVER_TEST
static void print_msg(CAN_MSG_PTR_T pCAN_Msg CO_COMMA_REDCY_PARA_DECL);
#endif /* CONFIG_DRIVER_TEST */

#ifdef CONFIG_INDEXLIST_OPT
/* experimental! */
static void addIndexCOB(COB_ARRAY_INDEX_T idx CO_COMMA_REDCY_PARA_DECL);
static void markIndexedCOB(CO_REDCY_PARA_DECL);
static void removeIndexCOB(COB_ARRAY_INDEX_T idx CO_COMMA_REDCY_PARA_DECL);
static void checkIndexedCOB(CO_REDCY_PARA_DECL);
#endif /* CONFIG_INDEXLIST_OPT */

/* CO_BIT should be defined within the compiler header co_xxx.h */
#ifndef CO_BIT
# define CO_BIT UNSIGNED8
#endif /* CO_BIT */


#ifdef CONFIG_COLIB_BUFFER
/*******************************************************************/
/**
*
*++ \brief clearTxBuffer - clears the CAN transmit buffer
*-- \brief clearTxBuffer - löscht den CAN Sendepuffer
* \ingroup bufferhandling
*
* \returns
*++ nothing
*-- nichts
*
*/
void clearTxBuffer(
	CO_REDCY_PARA_DECL
    )
{
REGISTER BUFFER_INDEX_T i;

    DISABLE_CAN_INTERRUPTS(CO_REDCY_PARA);

    GL_DRV_ARRAY(bTX_WriteIndex) = 0;
    GL_DRV_ARRAY(bTX_ReadIndex) = 0;

    for (i = 0; i < CONFIG_TX_BUFFER_SIZE; i++)
	{
		GL_DRV_ARRAY(pTX_Buffer[i]) .eStat = EMPTY;
    }

    RESTORE_CAN_INTERRUPTS(CO_REDCY_PARA);
} /* void clearTxBuffer() */


/*******************************************************************/
/**
*
*++ \brief clearRxBuffer - clears the CAN receive buffer
*-- \brief clearRxBuffer - löscht den CAN Emfangspuffer
*
* \ingroup bufferhandling
*
*
* \returns
*++ nothing
*-- nichts
*
*/
void clearRxBuffer(
	CO_REDCY_PARA_DECL
     )
{
REGISTER BUFFER_INDEX_T i;

    DISABLE_CAN_INTERRUPTS(CO_REDCY_PARA);

    GL_DRV_ARRAY(bRX_WriteIndex) = 0;
    GL_DRV_ARRAY(bRX_ReadIndex) = 0;

    for (i = 0; i < CONFIG_RX_BUFFER_SIZE; i++)
	{
		GL_DRV_ARRAY(pRX_Buffer[i]) .eStat = EMPTY;
    }

    RESTORE_CAN_INTERRUPTS(CO_REDCY_PARA);
} /* void clearRxBuffer() */


/*******************************************************************/
/**
*
*++ \brief checkTxBuffer - returns the transmit buffer status
*-- \brief checkTxBuffer - gibt den Status des Sendepuffers zurück
*
* \ingroup bufferhandling
*
*
* \retval
*++ CO_TRUE - transmit buffer empty
*-- CO_TRUE - Sendepuffer frei
* \retval
*++ CO_FALSE - transmit buffer full
*-- CO_FALSE - Sendepuffer voll
*
*/
BOOL_T checkTxBuffer(
	CO_REDCY_PARA_DECL
     )
{
    if (GL_DRV_ARRAY(pTX_Buffer[GL_DRV_ARRAY(bTX_WriteIndex)]).eStat == FULL)
    {
		return(CO_FALSE);
    } else
	{
		return(CO_TRUE);
    }
} /* BOOL_T checkTxBuffer() */


/*******************************************************************/
/**
*
*++ \brief getNumberOfTxMessages - delivers the number of messages in TX queue
*-- \brief getNumberOfTxMessages - liefert die Anzahl von Nachrichten in der Sendequeue
*
* \ingroup bufferhandling
* \returns
*++ number of messages in TX queue
*-- Anzahl der Nachrichten in der Sende-Queue
*
*/
BUFFER_INDEX_T getNumberOfTxMessages(
	CO_REDCY_PARA_DECL	/**< number of CAN line 0..CO_MAX_CAN_LINES-1 */
     )
{
REGISTER BUFFER_INDEX_T cnt;
BUFFER_INDEX_T readIndex;
BUFFER_INDEX_T writeIndex;

    DISABLE_CAN_INTERRUPTS(CO_REDCY_PARA);

    readIndex = GL_DRV_ARRAY(bTX_ReadIndex);
    writeIndex = GL_DRV_ARRAY(bTX_WriteIndex);

    if( writeIndex >= readIndex )
    {
		cnt = writeIndex - readIndex;
    } else
	{
		cnt = CONFIG_TX_BUFFER_SIZE - (readIndex - writeIndex);
    }

    /* WriteIndex == ReadIndex : Is Buffer empty or full ? */
    if ( (cnt == 0) && (GL_DRV_ARRAY(pTX_Buffer[writeIndex]) .eStat == FULL) )
    {
		cnt = CONFIG_TX_BUFFER_SIZE;
    }

    RESTORE_CAN_INTERRUPTS(CO_REDCY_PARA);
    return(cnt);
} /* BUFFER_INDEX_T getNumberOfTxMessages() */


/*******************************************************************/
/**
*
*++ \brief getNumberOfRxMessages - delivers the number of messages in RX queue
*-- \brief getNumberOfRxMessages - liefert die Anzahl der Nachrichten in der Empfangsqueue
*
* \ingroup bufferhandling
* \returns
*++ number of messages in RX queue
*-- Anzahl der Nachrichten in der Empfangs-Queue
*
*/
BUFFER_INDEX_T getNumberOfRxMessages(
	CO_REDCY_PARA_DECL	/**< number of CAN line 0..CO_MAX_CAN_LINES-1 */
     )
{
REGISTER BUFFER_INDEX_T cnt;
BUFFER_INDEX_T readIndex;
BUFFER_INDEX_T writeIndex;

    DISABLE_CAN_INTERRUPTS(CO_REDCY_PARA);

    readIndex = GL_DRV_ARRAY(bRX_ReadIndex);
    writeIndex = GL_DRV_ARRAY(bRX_WriteIndex);

    if(writeIndex >= readIndex)
    {
		cnt = writeIndex - readIndex;
    } else
	{
		cnt = CONFIG_RX_BUFFER_SIZE - (readIndex - writeIndex);
    }

    /* WriteIndex == ReadIndex : Is Buffer empty or full ? */
    if ( (cnt == 0) && (GL_DRV_ARRAY(pRX_Buffer[ writeIndex ]) .eStat == FULL) )
    {
        cnt = CONFIG_RX_BUFFER_SIZE;
    }

    RESTORE_CAN_INTERRUPTS(CO_REDCY_PARA);
    return(cnt);
} /* BUFFER_INDEX_T getNumberOfRxMessages() */


/*******************************************************************/
/**
*++ \brief Insert_TX_Request - inserts the next transm. request into queue
*-- \brief Insert_TX_Request - fügt einen Transmission Request in den Puffer ein.
*
* \ingroup bufferhandling
* \retval CO_OK
*-- Nachricht in die Queue eingefügt
*++ Message was added to the queue
*
* \retval CO_E_CAN_TRANS_BUF
*-- Software-Puffer voll, Nachricht wurde nicht hinzugefügt
*++ software buffer full, message not added
*
* \retval CO_E_PARA_INCOMP
*-- übergebene Parameter fehlerhaft
*++ wrong function parameter
*
*/
RET_T Insert_TX_Request(
     COB_T *pCOB,		/**< pointer to COB to be transmitted */
     UNSIGNED8  *pData	/**< pointer to message data or NULL */
     CO_COMMA_GLOBVARS_PARA_DECL
     )
{
RET_T retval;
BUFFER_ENTRY_PTR_T CO_DATA pBuffer;
#  if defined(CONFIG_MULT_LINES) || defined(CONFIG_REDUNDANCY_SUPPORT)
UNSIGNED8 canLine;
#  endif /* CONFIG_MULT_LINES || CONFIG_REDUNDANCY_SUPPORT */

    /* check COB pointer */
    if(pCOB == NULL) {
		return CO_E_PARA_INCOMP;
    }

#  if defined(CONFIG_MULT_LINES) || defined(CONFIG_REDUNDANCY_SUPPORT)
    canLine = pCOB->canLine;
#  endif /* CONFIG_MULT_LINES || CONFIG_REDUNDANCY_SUPPORT */

    /* initialize with error
     * if the buffer is free, this value will be changed */
    retval = CO_E_CAN_TRANS_BUF;

    BUFFER_INIT_PTR(TX, Write);
    CHECK_BUFFER_WRITE( TX, CANFLAG_TXBUFFER_OVERFLOW )
	{
		/* reset error flags */
		GL_DRV_ARRAY(coCanDriverState) &= (UNSIGNED8)~CANFLAG_TXBUFFER_OVERFLOW;

		retval = CO_OK;

		BUFFER_WRITE( TX, bChannel, pCOB->bChannel);
		BUFFER_WRITE( TX, cobId,    pCOB->cobId);
		BUFFER_WRITE( TX, bLength,  pCOB->bLength);
		BUFFER_WRITE( TX, eType,    pCOB->eType);

#  ifdef CONFIG_CAN_TIMEOUT
#    ifdef CONFIG_REDUNDANCY_SUPPORT
		if ((pCOB->eType == CO_COB_PDO_PROD)
				|| (pCOB->eType == CO_COB_PDO_PROD_RTR)
				|| (pCOB->eType == CO_COB_EMCY_PROD) )
		{
			BUFFER_WRITE( TX, timeticks, GL_VAR(co_redcyMaxDelayTimeTicks));
		} else {
			BUFFER_WRITE( TX, timeticks, 0xFFFF);
		}
#    else /* CONFIG_REDUNDANCY_SUPPORT */
		BUFFER_WRITE( TX, timeticks, 0xFFFF);
#    endif /* CONFIG_REDUNDANCY_SUPPORT */
#  endif /* CONFIG_CAN_TIMEOUT */

		/* pData == NULL only for transmit Remote Frames, no check */
		if( pData != NULL) {
			CO_MEMCPY((void*)&(pBuffer->pData[0]), (void const *)pData,\
                                 pCOB->bLength);
		}

#  ifdef CONFIG_BUFFER_DEBUG
		PRINTF(" Buffer %d filled ",\
				(int)GL_DRV_ARRAY(bTX_WriteIndex) );
#  endif /* CONFIG_BUFFER_DEBUG */

#  ifdef CONFIG_CAN_INSERT_DISABLE_INT
		/* default: not used */
		DISABLE_CPU_INTERRUPTS();
#  endif /* CONFIG_CAN_INSERT_DISABLE_INT */

		/* increment buffer */
		BUFFER_ENTRY_INCR( TX, Write, FULL);

#  ifdef CONFIG_CAN_INSERT_DISABLE_INT
		/* default: not used */
		RESTORE_CPU_INTERRUPTS();
#  endif /* CONFIG_CAN_INSERT_DISABLE_INT */

#  ifdef CONFIG_BUFFER_DEBUG
		{
			int i;
			for(i = 0; i < CONFIG_TX_BUFFER_SIZE; i++) {
				if( GL_DRV_ARRAY(pTX_Buffer[i]) .eStat == FULL) {
					PUTCHAR('+');
				}else{
					PUTCHAR('-');
				}
			}
			PRINTF("\n");
		}
#  endif	/* CONFIG_BUFFER_DEBUG  */
    }

    return retval;
} /* RET_T Insert_TX_Request() */


#  ifdef CONFIG_CAN_TIMEOUT
/*******************************************************************/
/**
*++ \brief checkBufferTimeout - check all timeout values within the TXbuffer
*-- \brief checkBufferTimeout - prüft alle timeout Zeiten im TXbuffer
*
*
* \returns
* 0    - no timeout
* != 0 - number of timeout messages, for Redundancy only PDO timeout
*
*
*/
BUFFER_INDEX_T checkBufferTimeout(
	CO_REDCY_PARA_DECL
)
{
REGISTER BUFFER_INDEX_T i;
BUFFER_ENTRY_PTR_T CO_DATA pBuffer;
BUFFER_INDEX_T retval = 0;

    DISABLE_CAN_INTERRUPTS(CO_REDCY_PARA);

    for (i = 0; i < CONFIG_TX_BUFFER_SIZE; i++) {

		pBuffer = (BUFFER_ENTRY_PTR_T)&GL_DRV_ARRAY(pTX_Buffer[i]);
		/* calculate only active messages */
		if((pBuffer->eStat != EMPTY) && (pBuffer->timeticks != 0))
		{
			pBuffer->timeticks --;
			if (pBuffer->timeticks == 0)
			{
#    ifdef CONFIG_REDUNDANCY_SUPPORT
				if ((pBuffer->eType == CO_COB_PDO_PROD)
					|| (pBuffer->eType == CO_COB_PDO_PROD_RTR))
#    endif /* CONFIG_REDUNDANCY_SUPPORT */
				{
					retval++;
				}
			}
		}
    }

    RESTORE_CAN_INTERRUPTS(CO_REDCY_PARA);
    return retval;
} /* BUFFER_INDEX_T checkBufferTimeout() */
#  endif /* CONFIG_CAN_TIMEOUT */


#  ifdef CONFIG_REDUNDANCY_SUPPORT
/*******************************************************************/
/**
*++ \brief move_TxBuffer - move all Messages from a line to a other line
*-- \brief move_TxBuffer - verschiebt alle Messages von einer in eine andere Linie
*
*
* \returns
*++ nothing
*-- nichts
*/
#    ifdef CONFIG_NO_GLOBAL_VARS
/* not so easy to adapt */
#    else /* CONFIG_NO_GLOBAL_VARS */
void move_TxBuffer(
	UNSIGNED8 targetLine, /**< target CAN line */
	UNSIGNED8 sourceLine  /**< source CAN line */
)
{
BUFFER_ENTRY_PTR_T pBuffer;
UNSIGNED8 canLine = sourceLine;
COB_T cob;

    DISABLE_TX_MESSAGES(sourceLine); /* cancel sending of Messages */

    DISABLE_CAN_INTERRUPTS(sourceLine);
    while(1)
	{

		BUFFER_INIT_PTR(TX, Read);
		CHECK_BUFFER_READ( TX )
		{
			cob.cobId = BUFFER_READ( TX, cobId ); /* only, if isn't C505C / C515C */
			cob.bLength = BUFFER_READ( TX, bLength );
			cob.bChannel = BUFFER_READ( TX, bChannel );
			cob.canLine = targetLine;
			cob.eType = BUFFER_READ( TX, eType );
			cob.pNextLine = NULL;

			/* !!! don't use Transmit_COB_Redundancy !!! */
			Transmit_COB(
					&cob,
					&pTX_Buffer[ bTX_ReadIndex[sourceLine] ]
							[sourceLine].pData[0]
					CO_COMMA_GLOBVARS_PARA
					);

			DISABLE_CPU_INTERRUPTS();
			/* Do this changes atomic. */
			BUFFER_ENTRY_INCR( TX , Read , EMPTY);
			RESTORE_CPU_INTERRUPTS();

		} else
		{
			break;
		}
	}
    RESTORE_CAN_INTERRUPTS(sourceLine);
} /* void move_TxBuffer() */
#    endif /* CONFIG_NO_GLOBAL_VARS */
#  endif /* CONFIG_REDUNDANCY_SUPPORT */
#endif /* CONFIG_COLIB_BUFFER */


#ifdef CONFIG_COB_ARRAY
/*******************************************************************/
/**
* \brief initCobList - initialize COB array/list
*
* \returns
*++ nothing
*-- nichts
*/
void initCobList(
	CO_REDCY_PARA_DECL	/**< number of CAN line 0..CO_MAX_CAN_LINES-1 */
)
{
#  if defined(CONFIG_MULT_LINES) || \
     (defined(CONFIG_REDUNDANCY_SUPPORT) && !defined(CONFIG_NO_GLOBAL_VARS))
REGISTER UNSIGNED8 line;
REGISTER COB_ARRAY_INDEX_T idx;
#  endif /* defined(CONFIG_MULT_LINES) || defined(CONFIG_REDUNDANCY_SUPPORT) */


#  if defined(CONFIG_MULT_LINES) || \
     (defined(CONFIG_REDUNDANCY_SUPPORT) && !defined(CONFIG_NO_GLOBAL_VARS))
    /* initialize start-indices of COB and COB-Index Array */
    idx = 0;
    for(line = 0; line < canLine; line++)
	{
    	idx += GL_DRV_VAR(cobListLineCnts)[line];
    }
    GL_DRV_ARRAY(cobListLineOffs) = idx;
    GL_DRV_ARRAY(cobListNextEntry) = idx;

    /*
    * reset line depend  COB List memory
    * start of list: (void *)&GL_DRV_REDCY(cobList)[idx];
    * number of list member: GL_DRV_VAR(cobListLineCnts)[line];
    */
    memset( (void *)&GL_DRV_PREDCY(cobList)[idx], /* start of line depend list */
    		0, /* value */
	    sizeof(COB_T) * GL_DRV_VAR(cobListLineCnts)[canLine] /* size */
	    );

#  else /* defined(CONFIG_MULT_LINES) || defined(CONFIG_REDUNDANCY_SUPPORT) */
    GL_DRV_ARRAY(cobListNextEntry) = 0;

    /* reset COB List memory */
    memset((void*)&GL_DRV_PREDCY(cobList)[0], 0, sizeof(COB_T) * COB_NUMBER_CNT);
#  endif /* defined(CONFIG_MULT_LINES) || defined(CONFIG_REDUNDANCY_SUPPORT) */

#  ifdef CONFIG_DRIVER_FAST_SORT
	/* init */
    GL_DRV_ARRAY(cobIdxListLen) = GL_DRV_ARRAY(cobListNextEntry);
#  endif /* CONFIG_DRIVER_FAST_SORT */

} /* void initCobList() */


/*******************************************************************/
/**
* \brief initCobEntry - get a initialized COB entry
*
** \retval NULL
*++ - end of memory
*-- - nicht genügend Speicher
*
* \returns
*++ pointer to a COB_T entry
*-- pointer auf einen COB_T Eintrag
*/
COB_T * initCobEntry(
	CO_REDCY_PARA_DECL	/**< number of CAN line 0..CO_MAX_CAN_LINES-1 */
)
{
COB_PTR_T pCOB;
COB_ARRAY_INDEX_T nextEntry;

#  if defined(CONFIG_DRIVER_TEST) && defined(CONFIG_NO_GLOBAL_VARS)
/* for better debugging */
VOLATILE DRIVER_DATA_T * pDriverData;

    pDriverData = (DRIVER_DATA_T*)(GL_VAR(canDrvPtr) CO_REDCY_PARA_ARRAY_INDEX);
#  endif /* CONFIG_DRIVER_TEST && CONFIG_NO_GLOBAL_VARS */

    /* init local vars */
    nextEntry = GL_DRV_ARRAY(cobListNextEntry);

    /* check for free entries */
#  if defined(CONFIG_MULT_LINES) || \
     (defined(CONFIG_REDUNDANCY_SUPPORT)&& !defined(CONFIG_NO_GLOBAL_VARS))
    if( nextEntry == GL_DRV_ARRAY(cobListLineOffs) + GL_DRV_ARRAY(cobListLineCnts))
#  else /* defined(CONFIG_MULT_LINES) || defined(CONFIG_REDUNDANCY_SUPPORT) */
    if( nextEntry == COB_NUMBER_CNT)
#  endif /* defined(CONFIG_MULT_LINES) || defined(CONFIG_REDUNDANCY_SUPPORT) */
    {
        return NULL; /* error - no free entry in Array */
    }

    pCOB = &GL_DRV_PREDCY(cobList)[nextEntry];

    nextEntry++;

    GL_DRV_ARRAY(cobListNextEntry) = nextEntry;

    return ((COB_T *)pCOB);
} /* COB_T * initCobEntry() */


#  ifdef CONFIG_DRIVER_FAST_SORT
#    ifdef CONFIG_INDEXLIST_OPT
	/* experimental Optimization of Indexlist creation */
#      define FLAG_INDEXED 0x80
/*******************************************************************/
/*
* \brief markIndexedCOB - mark temporary indexed COBs
*
*-- markieren aller COBs, welche bereits in der Index-Liste enthalten sind
*
* - mark objects that are already in the index list,
* - for memory save use Bit 7 of bLength
* -> This bit must reset later, before this functionality
*    returns to the Libary!
*/
static void markIndexedCOB(
	CO_REDCY_PARA_DECL	/**< number of CAN line 0..CO_MAX_CAN_LINES-1 */
)
{
COB_ARRAY_INDEX_T CO_DATA start;	/* first entry */
COB_ARRAY_INDEX_T CO_DATA end;		/* last entry */
COB_ARRAY_INDEX_T CO_DATA i;		/* current entry */
COB_ARRAY_INDEX_T CO_DATA idx;		/* index entry */

#      if defined(CONFIG_MULT_LINES) || \
          (defined(CONFIG_REDUNDANCY_SUPPORT) && !defined(CONFIG_NO_GLOBAL_VARS))
    start = GL_DRV_ARRAY(cobListLineOffs);
#      else /* defined(CONFIG_MULT_LINES) || defined(CONFIG_REDUNDANCY_SUPPORT) */
    start = 0;
#      endif /* defined(CONFIG_MULT_LINES) || defined(CONFIG_REDUNDANCY_SUPPORT) */
    end = GL_DRV_ARRAY(cobIdxListLen);

    for (i = start; i < end; i++)
	{
    	idx = GL_DRV_PREDCY(cobIndexList)[i];
		GL_DRV_PREDCY(cobList)[idx].bLength |= FLAG_INDEXED;
    }
} /* static void markIndexedCOB() */


/*******************************************************************/
/*
* \brief addIndexCOB - add COB idx to the index list
*
* Add COB[idx] at the end of the current Index list.
*
*/
static void addIndexCOB(
	COB_ARRAY_INDEX_T idx /* array element number in COB List */
	CO_COMMA_REDCY_PARA_DECL
)
{
COB_ARRAY_INDEX_T idxListLen; /* length from index list */

    idxListLen = GL_DRV_ARRAY(cobIdxListLen);
    GL_DRV_PREDCY(cobIndexList)[idxListLen] = idx;
    idxListLen++;
    GL_DRV_ARRAY(cobIdxListLen) = idxListLen;
} /* static void addIndexCOB() */


/*******************************************************************/
/* \brief removeIndexCOB - remove COB idx from the index list
*
* The COB[idx] is searching in the index list.
* In the index list this element will removed by move the
* later elements one index up. The move is used to do not change
* the order - optimized for later used Bubble Sort.
*/
static void removeIndexCOB(
	COB_ARRAY_INDEX_T idx /* array element number in COB List */
	CO_COMMA_REDCY_PARA_DECL
)
{
COB_ARRAY_INDEX_T i;
COB_ARRAY_INDEX_T end;

    /*
     * at the first - find COB idx in the index list
     */

#      if defined(CONFIG_MULT_LINES) || \
          (defined(CONFIG_REDUNDANCY_SUPPORT) && !defined(CONFIG_NO_GLOBAL_VARS))
    i = GL_DRV_ARRAY(cobListLineOffs);
#      else /* defined(CONFIG_MULT_LINES) || defined(CONFIG_REDUNDANCY_SUPPORT) */
    i = 0;
#      endif /* defined(CONFIG_MULT_LINES) || defined(CONFIG_REDUNDANCY_SUPPORT) */

    end = GL_DRV_ARRAY(cobIdxListLen);

    while(GL_DRV_PREDCY(cobIndexList)[i] != idx)
	{
    	i++;
    	if(i == end) return; /* error - not in list */
    }

    /*
     * now remove this element
     * cobIdxListLen is 'last element + 1'
     */
    if ((i + 1) != GL_DRV_ARRAY(cobIdxListLen))
	{
		memmove(
			&GL_DRV_PREDCY(cobIndexList)[i],
			&GL_DRV_PREDCY(cobIndexList)[i+1],
			(size_t)(GL_DRV_ARRAY(cobIdxListLen) -1 - i)
			* sizeof(COB_ARRAY_INDEX_T)
			);
    }

    GL_DRV_ARRAY(cobIdxListLen)--;
} /* static void removeIndexCOB() */


/*******************************************************************/
/*
* checkIndexedCOB - check if an eType was changed
*
*-- prüfen, ob ein COB in den Index rein oder aus dem Index raus muss
*++ If an eType was changed, an element must add or remove from the
*++ index list. The old eType we don't know. We check, of all receiving
*++ elements are in the list and all only-transmit elements didn't are
*++ in the list.
*/
static void checkIndexedCOB(
	CO_REDCY_PARA_DECL	/**< number of CAN line 0..CO_MAX_CAN_LINES-1 */
)
{
COB_ARRAY_INDEX_T CO_DATA start;	/* first entry - in the COB list */
COB_ARRAY_INDEX_T CO_DATA end;		/* last entry - in the COB list */
COB_ARRAY_INDEX_T CO_DATA i;		/* current entry */
COB_KIND_T eType;
UNSIGNED8 bLength;
COB_PTR_T pCOB;

    /* mark bLength pf the COBs */
    markIndexedCOB();

#      if defined(CONFIG_MULT_LINES) || \
          (defined(CONFIG_REDUNDANCY_SUPPORT) && !defined(CONFIG_NO_GLOBAL_VARS))
    start = GL_DRV_ARRAY(cobListLineOffs);
#      else /* defined(CONFIG_MULT_LINES) || defined(CONFIG_REDUNDANCY_SUPPORT) */
    start = 0;
#      endif /* defined(CONFIG_MULT_LINES) || defined(CONFIG_REDUNDANCY_SUPPORT) */
    end = GL_DRV_ARRAY(cobListNextEntry);

    for (i = start; i < end; i++)
	{
    	pCOB = &GL_DRV_PREDCY(cobList)[i];
		eType = pCOB->eType;
		bLength = pCOB->bLength;
		/* 1) Should Message receive and Indexed Flag is not set -> add */
		/* 2) No Receive, but Indexed -> remove */
		if ( ((eType & CO_COB_DIR_RTR_MASK) != CO_COB_TX)
			&& ((eType & CO_COB_DISABLED) == 0) )
		{
			/* -> 1 <- */
			if( (bLength & FLAG_INDEXED) == 0)
			{
				addIndexCOB(i CO_COMMA_REDCY_PARA);
			}
		} else
		{
			/* -> 2 <- */
			if( (bLength & FLAG_INDEXED) != 0)
			{
				removeIndexCOB(i CO_COMMA_REDCY_PARA);
			}
		}

		pCOB->bLength &= (UNSIGNED8)~FLAG_INDEXED;
    }
} /* static void checkIndexedCOB() */
#    endif /* CONFIG_INDEXLIST_OPT */


/*******************************************************************/
/**
* \brief createCobIdIndex - create Index list for validCobId()
*
* \returns
*++ nothing
*-- nichts
*/
void createCobIdIndex(
	CO_REDCY_PARA_DECL	/**< number of CAN line 0..CO_MAX_CAN_LINES-1 */
    )
{
COB_ARRAY_INDEX_T CO_DATA idxVal;     /* index from index list */
COB_ARRAY_INDEX_T CO_DATA idxValNext; /* index from index list */
COB_ARRAY_INDEX_PTR_T CO_DATA pIdxList; /* pointer to index list */
COB_ARRAY_INDEX_PTR_T CO_DATA pIdxListNext; /* pointer to index list */

COB_ARRAY_INDEX_T CO_DATA idx;        /* index from index list */
COB_ARRAY_INDEX_T idxListLen; /* length from index list */
COB_ARRAY_INDEX_T firstIdx; /* first entry from index/cob list */
COB_ARRAY_INDEX_T lastIdx; /* first idx + length == last idx from index list */
CO_BIT exchange; /* flag */
COB_KIND_T eType;

#    if defined(CONFIG_DRIVER_TEST) && defined(CONFIG_NO_GLOBAL_VARS)
/* for better debugging */
VOLATILE DRIVER_DATA_T * pDriverData;

    pDriverData = (DRIVER_DATA_T*)(GL_VAR(canDrvPtr) CO_REDCY_PARA_ARRAY_INDEX);
#    endif /* CONFIG_DRIVER_TEST && CONFIG_NO_GLOBAL_VARS */

#    ifdef CONFIG_INDEXLIST_OPT
    checkIndexedCOB();
#    endif /* CONFIG_INDEXLIST_OPT */

    /* first entry in index list */
#    if defined(CONFIG_MULT_LINES) || \
        (defined(CONFIG_REDUNDANCY_SUPPORT) && !defined(CONFIG_NO_GLOBAL_VARS))
    firstIdx = GL_DRV_ARRAY(cobListLineOffs);
#    else /* defined(CONFIG_MULT_LINES) || defined(CONFIG_REDUNDANCY_SUPPORT) */
    firstIdx = 0;
#    endif /* defined(CONFIG_MULT_LINES) || defined(CONFIG_REDUNDANCY_SUPPORT) */

    /* max length from index list */
    lastIdx = GL_DRV_ARRAY(cobListNextEntry);

#    ifdef CONFIG_INDEXLIST_OPT
    idxListLen = GL_DRV_ARRAY(cobIdxListLen);
#    else /* CONFIG_INDEXLIST_OPT */
    /* length from index list */
    idxListLen = firstIdx;

    /* delete sorted list */
    for (idx = firstIdx; idx < lastIdx; idx ++)
	{
		eType = GL_DRV_PREDCY(cobList)[idx].eType;
		/* ignore TX cobs */
		if ((eType & CO_COB_DIR_RTR_MASK) != CO_COB_TX)
		{
			/* ignore disable COBs */
			if((eType & CO_COB_DISABLED) == 0)
			{
			/* cobIndexList[idx] = idx; */
			GL_DRV_PREDCY(cobIndexList)[idxListLen] = idx;
			idxListLen++;
			}
		}
    }

    GL_DRV_ARRAY(cobIdxListLen) = idxListLen;
#    endif /* CONFIG_INDEXLIST_OPT */

    if (idxListLen == firstIdx)
	{
		return;
    }

    /* bubblesort algo */
    do {
        exchange = 0;
        /* for (i = 0 ; i < N-1 ; i++) */
		idx = firstIdx;
        /* while (idx < (listLen - 1))  { */
        while (idx < (idxListLen - 1))
		{
			/* if (feld[i] > feld[i+1]) */
			pIdxList = &GL_DRV_PREDCY(cobIndexList)[idx];
			idxVal = *pIdxList;

			pIdxListNext = (pIdxList + 1);
			idxValNext = *pIdxListNext;

			/* (cobList[*pIdxList].cobId > cobList[ *(pIdxList + 1) ].cobId)*/
			if (GL_DRV_PREDCY(cobList)[idxVal].cobId > GL_DRV_PREDCY(cobList)[idxValNext].cobId)
			{
				*pIdxList = idxValNext;
				*pIdxListNext = idxVal;

				exchange = 1;
			} /* if */
			idx++;
		} /* while */
    } while (exchange == 1);


#    ifdef CONFIG_VALID_COB_DEBUG
    {
		COB_ARRAY_INDEX_T i;

		PRINTF("cobIndexList:\n");
		for( i = firstIdx; i < idxListLen; i++)
		{
			PRINTF("%d(%d, 0x%x) \n", (int)i, (int)GL_DRV_PREDCY(cobIndexList)[i], \
	    		(int)GL_DRV_PREDCY(cobList)[GL_DRV_PREDCY(cobIndexList)[i]].cobId);

		}
		PRINTF("\n");
    }
#    endif /* CONFIG_VALID_COB_DEBUG */
} /* void createCobIdIndex() */
#  endif /* CONFIG_DRIVER_FAST_SORT */


#  ifdef CONFIG_CAN_FAMILY_LPC21
/*******************************************************************/
/**
* \brief getSortRxCOB - get possible Receive-COBs sorted by COB-ID
*
*  \param cnt - number of element in the sorted list
*        0..x from every canLine
*
* Note: Currently only supported for SingleLine LPC21xx family!!!
*/
COB_T * getSortRxCOB(
    COB_ARRAY_INDEX_T cnt
    CO_COMMA_REDCY_PARA_DECL
)
{
#    ifdef CONFIG_DRIVER_FAST_SORT
COB_ARRAY_INDEX_T idx;

#      ifdef CONFIG_MULT_LINES
    idx = cnt + GL_DRV_ARRAY(cobListLineOffs);
#      else /* CONFIG_MULT_LINES */
    idx = cnt;
#      endif /* CONFIG_MULT_LINES */

    if (idx >= GL_DRV_ARRAY(cobIdxListLen) ) {
    	return NULL;
    }

    return &(GL_DRV_PREDCY(cobList)[GL_DRV_PREDCY(cobIndexList)[idx]]);
#    else /* CONFIG_DRIVER_FAST_SORT */
#      error "Only CONFIG_DRIVER_FAST_SORT implemented, yet!"
#    endif /* CONFIG_DRIVER_FAST_SORT */
} /* COB_T * getSortRxCOB() */
#  endif /* CONFIG_CAN_FAMILY_LPC21 */


/*******************************************************************/
/**
*++ validCobId() - checks whether the COB-ID of a received message is valid
*-- validCobId() - prueft die Gültigkeit der COB-IDs emfangener Nachrichten
*
*++ This function checks whether the COB-ID of a received message is valid
*++ for the local node.
*-- Diese Funktion prueft die Gültigkeit der COB-IDs emfangener Nachrichten
*-- für den localen Netzwerkknoten.
*
* \retval
*++COB_KIND_T		if cob-id exist
*++CO_COB_DISABLED	if cob-id doesn't exist or is disabled
*--COB_KIND_T		cob-id existiert
*--CO_COB_DISABLED	cob-id existiert nicht oder ist disabled
*
*/
COB_KIND_T validCobId(
	COB_IDENT_T	cobId,	/**< COB-ID */
	UNSIGNED8	rtrFlag	/**< RTR-BIT value [0 or !=0] */
	CO_COMMA_REDCY_PARA_DECL
    )
{
	COB_KIND_T eType;
	CO_BIT found;
	COB_ARRAY_INDEX_T CO_DATA start;	/* first entry */
	COB_ARRAY_INDEX_T CO_DATA end;		/* last entry */
	COB_PTR_T pCOB = (COB_PTR_T)NULL; /* init - to avoid Compiler warnings */
#  ifdef CONFIG_DRIVER_FAST_SORT
	COB_ARRAY_INDEX_T CO_DATA middle;	/* middle entry for binary search */
	COB_IDENT_T 	  CO_DATA lCobId;	/* COB-ID from middle entry */
#  else /* CONFIG_DRIVER_FAST_SORT */
	REGISTER COB_ARRAY_INDEX_T i;
#  endif /* CONFIG_DRIVER_FAST_SORT */

#  if defined(CONFIG_DRIVER_TEST) && defined(CONFIG_NO_GLOBAL_VARS)
	/* for better debugging */
	VOLATILE DRIVER_DATA_T * pDriverData;

    pDriverData = (DRIVER_DATA_T*)(GL_VAR(canDrvPtr) CO_REDCY_PARA_ARRAY_INDEX);
#  endif /* CONFIG_DRIVER_TEST && CONFIG_NO_GLOBAL_VARS */

    found = 0;

#  if defined(CONFIG_MULT_LINES) || \
      (defined(CONFIG_REDUNDANCY_SUPPORT) && !defined(CONFIG_NO_GLOBAL_VARS))
	start = GL_DRV_ARRAY(cobListLineOffs);
#  else /* defined(CONFIG_MULT_LINES) || defined(CONFIG_REDUNDANCY_SUPPORT) */
	start = 0;
#  endif /* defined(CONFIG_MULT_LINES) || defined(CONFIG_REDUNDANCY_SUPPORT) */


	/*-------------------------------*/
#  ifdef CONFIG_DRIVER_FAST_SORT
	/*-------------------------------*/
    end = GL_DRV_ARRAY(cobIdxListLen);

    while( start != end )
	{
    	middle = ((end - start) >> 1) + start;

#    ifdef CONFIG_VALID_COB_DEBUG
#      ifdef CONFIG_MULT_LINES
    	PRINTF("validCobId Line %d: ", (int)canLine);
#      endif /* CONFIG_MULT_LINES */

    	PRINTF("start: %d middle: %d end: %d\n", \
    			(int)start, (int)middle, (int)end);
#    endif /* CONFIG_VALID_COB_DEBUG */

    	pCOB = &GL_DRV_PREDCY(cobList)[GL_DRV_PREDCY(cobIndexList)[middle]];
    	lCobId = pCOB->cobId;

#    ifdef CONFIG_VALID_COB_DEBUG
    	PRINTF("cobId 0x%x middle 0x%x\n", (int)cobId, (int)lCobId);
#    endif /* CONFIG_VALID_COB_DEBUG */

    	if(cobId < lCobId)
		{
    	    end = middle;
    	}
    	else
		{
    		if (lCobId == cobId) {
    			found = 1;
    			break;
    		}
    	    /* check for end = start + 1 */
    		if (start == middle)  {
    			break;
    		}

    		start = middle;
    	}
    }

#    ifdef CONFIG_VALID_COB_DEBUG
    PRINTF("validCobId: 0x%x ", (int)cobId);
    if( found == 0 )
	{
    	PRINTF("not ");
    }
    PRINTF("found\n");
#    endif /* CONFIG_VALID_COB_DEBUG */

    if ( found == 0 )
	{
    	return(CO_COB_DISABLED);
    }

    /* index middle is the found COB-ID */
    eType = pCOB->eType;

    /*-------------------------------*/
#  else /* CONFIG_DRIVER_FAST_SORT */
    /*-------------------------------*/
    end = GL_DRV_ARRAY(cobListNextEntry);

    /* very simple check for a low number of CAN Objects */
    pCOB = &GL_DRV_PREDCY(cobList)[start];
    for ( i = start; i < end; i++) {
    	if (pCOB->cobId == cobId) {
    		if ((pCOB->eType & CO_COB_DISABLED) == 0) {
    			/* found only enabled COBs */
    			if(rtrFlag == 0) {
    				/* check for RX COB */
    				if ((pCOB->eType & CO_COB_DIR_MASK) == CO_COB_RX) {
    					/* found data frame */
    					found = 1;
    					break;
    				}
    			}
    			else {
    				/* check for RTR Frame */
    				if ((pCOB->eType & CO_COB_DIR_RTR_MASK) == CO_COB_TX_RTR) {
    					/* found request frame */
    					found = 1;
    					break;
    				}
    			}
    		}
    	}
    	pCOB++;
    }

    if ( found == 0 )
	{
    	return(CO_COB_DISABLED);
    }

    eType = pCOB->eType;

/*-------------------------------*/
#  endif /* CONFIG_DRIVER_FAST_SORT */
/*-------------------------------*/

    /*
     * CO_COB_TX CO_COB_RTR rtrFlag receive
     *    0           0/1      0      true(a)
     *    0           0/1      RTR    false(b)
     *    1           0        0/RTR  false(c)
     *    1           1        0      false(d)
     *    1           1        RTR    true(e)
     */

    if((eType & CO_COB_DIR_MASK) == CO_COB_RX)
	{
    	if (rtrFlag != 0) {
    		eType = CO_COB_DISABLED; /* (b) */
    	}
    }
    else
	{
    	if (rtrFlag == 0) {
    		eType = CO_COB_DISABLED; /* (c.1/d) */
    	}
    	else if((eType & CO_COB_RTR) == 0) {
    		eType = CO_COB_DISABLED; /* (c.2) */
    	}
    }

    return(eType);
} /* COB_KIND_T validCobId() */
#endif /* CONFIG_COB_ARRAY */


#ifdef CONFIG_COLIB_FLUSHMBOX
/*******************************************************************/
/**
*
*++ \brief FlushMbox - read can messages from buffer
*-- \brief FlushMbox - Auslesen des Message Emfangspuffers
*
*++ This function should be called cyclically or whenever
*++ the user is expecting a response to any request.
*++ The function flushes the message buffer filled by
*++ the CAN interrupt routine and starts
*++ processing of the messages.
*-- Diese Funktion sollte zyklisch aufgerufen werden
*-- oder wann immer die Applikation die Antwort auf einen
*-- CANopen-Request
*-- erwartet.
*-- Der von der Empfangs Interruptservice Routine gefüllte Puffer
*-- wird geleert und nacheinender immer eine Message
*-- zur Auswertung an eine interne CANopen Routine übergeben.
*
*++ Three methods are implemented:
*++ - CONFIG_FLUSHMBOX_READ_ONLY_ONE\n
*++   on every FlushMbox() call only one message is calculated
*++ - CONFIG_FLUSHMBOX_READ_MAXMSG\n
*++   the complete buffer is only one time calculated (default)
*++ - CONFIG_FLUSHMBOX_READ_ALL\n
*++   all, also new received messages are calculated
*-- Drei Abarbeitungs-Methoden sind implementiert:
*-- - CONFIG_FLUSHMBOX_READ_ONLY_ONE\n
*--   je FlushMbox() Aufruf nur eine CAN Message bearbeiten
*-- - CONFIG_FLUSHMBOX_READ_MAXMSG\n
*--   maximal einmal den Puffer leeren (default)
*-- - CONFIG_FLUSHMBOX_READ_ALL\n
*--   alle, auch während der Abarbeitung empfangenen Messages bearbeiten
*
* \returns
*++ nothing
*-- nichts
*
*/
/*
* TODO:
* - optimize validCobId() call
*
*/
void FlushMbox(
	CO_REDCY_PARA_DECL	/**< number of CAN line 0..CO_MAX_CAN_LINES-1 */
	)
{
	/* Buffer Pointer */
	BUFFER_ENTRY_PTR_T CO_DATA pBuffer;
	/* Msg Pointer */
	CAN_MSG_PTR_T CO_DATA pCAN_Msg;
#  ifdef CONFIG_FLUSHMBOX_READ_MAXMSG
	BUFFER_INDEX_T cnt = CONFIG_FLUSHMBOX_READ_MAXMSG;
#  endif /* CONFIG_FLUSHMBOX_READ_MAXMSG */

	/*-------------------------------*/
#  ifdef CONFIG_DRIVER_TEST
	/*-------------------------------*/
#    ifdef CONFIG_NO_GLOBAL_VARS
	/*-- for better debugging --*/
	VOLATILE DRIVER_DATA_T * pDriverData;

    pDriverData = (DRIVER_DATA_T*)(GL_VAR(canDrvPtr) CO_REDCY_PARA_ARRAY_INDEX);
#    endif /* CONFIG_NO_GLOBAL_VARS */
#    ifdef CONFIG_SYNC_CONSUMER
    if (TEST_COLIB_FLAG(COFLAG_SYNC_RECEIVED))
	{
        PRINTF("Sync Counter %d\n", (int)GL_ARRAY(co_syncCnt));
    }
#    endif /* CONFIG_SYNC_CONSUMER */
    /*-------------------------------*/
#  endif /* CONFIG_DRIVER_TEST */
    /*-------------------------------*/

    CO_SET_BIT(3); /* time measurement start */

    /* first test the library flags and work it
     * SYNC, Timer, Overflow, Error Passive/Busoff */
    if(TEST_COLIB_FLAG(COFLAG_ALL)){
    	FLAG_IDENTIFICATION(CO_REDCY_PARA);
    }

    BUFFER_INIT_PTR(RX, Read);

#  ifdef CONFIG_FLUSHMBOX_READ_ALL
    /* now look at the buffer for new can data */
    while( BUFFER_READ(RX, eStat) == FULL)
#  endif /* CONFIG_FLUSHMBOX_READ_ALL  */

#  ifdef CONFIG_FLUSHMBOX_READ_ONLY_ONE
    if ( BUFFER_READ(RX, eStat) == FULL)
#  endif /* CONFIG_FLUSHMBOX_READ_ONLY_ONE */

#  ifdef CONFIG_FLUSHMBOX_READ_MAXMSG
    while(( BUFFER_READ(RX, eStat) == FULL) && (cnt-- != 0))
#  endif /* CONFIG_FLUSHMBOX_READ_MAXMSG */
    {
    	/* buffer is full, in this time no new data will be
    	 * write in the buffer */
#  ifdef CONFIG_CAN_FLUSHMBOX_DISABLE_INT
    	/* default: not used */
    	DISABLE_CAN_INTERRUPTS(CO_REDCY_PARA);
#  endif /* CONFIG_CAN_FLUSHMBOX_DISABLE_INT */

    	pCAN_Msg = (CAN_MSG_PTR_T)&GL_DRV_ARRAY(CAN_Msg);

    	pCAN_Msg->cobId = BUFFER_READ(RX, cobId) >> ID_SHIFT_WIDTH;

    	pCAN_Msg->length = BUFFER_READ(RX, bLength);

    	/* check for eType */
    	pCAN_Msg->cobType = validCobId(pCAN_Msg->cobId,  \
			(pCAN_Msg->length & CO_RTR_REQ) CO_COMMA_REDCY_PARA);

    	/*-------------------------------*/
#  ifdef CONFIG_DRIVER_TEST
    	/*-------------------------------*/
    	print_msg(pCAN_Msg CO_COMMA_REDCY_PARA);
    	/*-------------------------------*/
#  endif /* CONFIG_DRIVER_TEST */
    	/*-------------------------------*/

    	if( (pCAN_Msg->cobType & CO_COB_DISABLED) == 0)
		{
    		CO_MEMCPY((void*)&(pCAN_Msg->pData[0]), \
    				(const void*)BUFFER_MEMBER_ADDR(RX, Read, pData[0]), \
    				(size_t)pCAN_Msg->length & 0x0F);
		} /* if */

#  ifdef CONFIG_CAN_FLUSHMBOX_DISABLE_CPUINT
    	/* default: not used */
    	DISABLE_CPU_INTERRUPTS();
#  endif /* CONFIG_CAN_FLUSHMBOX_DISABLE_CPUINT */

    	/* increment buffer */
    	BUFFER_ENTRY_INCR( RX, Read, EMPTY);

#  ifdef CONFIG_CAN_FLUSHMBOX_DISABLE_CPUINT
		/* default: not used */
    	RESTORE_CPU_INTERRUPTS();
#  endif /* CONFIG_CAN_FLUSHMBOX_DISABLE_CPUINT */

#  ifdef CONFIG_CAN_FLUSHMBOX_DISABLE_INT
    	/* default: not used */
    	RESTORE_CAN_INTERRUPTS(CO_REDCY_PARA);
#  endif /* CONFIG_CAN_FLUSHMBOX_DISABLE_INT */

#  ifdef CONFIG_SYNC_CONSUMER
		if( pCAN_Msg->cobType == CO_COB_SYNC_CONS)
		{
			/* Sync Counter */
			GL_ARRAY(co_syncCnt) = 0;
			if (pCAN_Msg->length != 0) {
				GL_ARRAY(co_syncCnt) = pCAN_Msg->pData[0];
			}
			/* SYNC message received */
	    	SET_COLIB_FLAG(COFLAG_SYNC_RECEIVED);
	    	FLAG_IDENTIFICATION(CO_REDCY_PARA);
		}
		else
#  endif /* CONFIG_SYNC_CONSUMER */

		if( (pCAN_Msg->cobType & CO_COB_DISABLED) == 0)
		{
			MSG_IDENTIFICATION((CAN_MSG_T *)pCAN_Msg);

			/* sometimes the customers are a "long" time within the
			 * indication functions or have a large full buffer
			 * calculate current time */
			if (TEST_COLIB_FLAG(COFLAG_ALL))
			{
				FLAG_IDENTIFICATION(CO_REDCY_PARA);
			}
		}

#  ifdef CONFIG_FLUSHMBOX_READ_ONLY_ONE
#  else /* CONFIG_FLUSHMBOX_READ_ONLY_ONE */
		/* bufferindex changed - set Pointer for the next loop cycle */
		BUFFER_INIT_PTR(RX, Read);
#  endif /* CONFIG_FLUSHMBOX_READ_ONLY_ONE */

    } /* if/while Buffer full */

    CO_RESET_BIT(3); /* time measurement end */
} /* void FlushMbox() */
#endif /* CONFIG_COLIB_FLUSHMBOX */

#ifdef CONFIG_COLIB_J1939
#include "kvcanhw.h"
/*******************************************************************/
/*
* co_getNextRxMessage - return next CAN msg as Raw Frame
*
* Intended for use with the kvaser NMEA stack
*
*/
void co_getNextRxMessage(
	kvRawCanMsg* frame              /**< kvRawCanMsg to fill with data */
	CO_COMMA_LINE_PARA_DECL		/**< number of CAN line 0..CO_MAX_CAN_LINES-1 */
)
{
  /* Buffer Pointer */
  BUFFER_ENTRY_PTR_T CO_DATA pBuffer;

  BUFFER_INIT_PTR(RX, Read);

  if ( BUFFER_READ(RX, eStat) == FULL)
  {
          frame->msgID = BUFFER_READ(RX, cobId) >> ID_SHIFT_WIDTH;

          frame->dlc = BUFFER_READ(RX, bLength);
          
          CO_MEMCPY((void*)&(frame->data[0]), \
                  (const void*)BUFFER_MEMBER_ADDR(RX, Read, pData[0]), \
                  (size_t)frame->dlc & 0x0F);
  }
		
  /* increment buffer */
  BUFFER_ENTRY_INCR( RX, Read, EMPTY);
						
}
#endif /* CONFIG_COLIB_J1939 */


#ifdef CONFIG_DRIVER_TEST
/*******************************************************************/
/*
* print_msg - debug function, printf() ID and length
*
* PRINT_MSG_LINE_MAX_MSG - max id's for every line
*
*/
#  if defined(CONFIG_MULT_LINES) || defined(CONFIG_REDUNDANCY_SUPPORT)
#    define PRINT_MSG_LINE_MAX_MSG 5
#  else /* defined(CONFIG_MULT_LINES) || defined(CONFIG_REDUNDANCY_SUPPORT) */
#    define PRINT_MSG_LINE_MAX_MSG 10
#  endif /* defined(CONFIG_MULT_LINES) || defined(CONFIG_REDUNDANCY_SUPPORT) */

static void print_msg(
    CAN_MSG_PTR_T pCAN_Msg
    CO_COMMA_REDCY_PARA_DECL	/**< number of CAN line 0..CO_MAX_CAN_LINES-1 */
)
{
static UNSIGNED8 n = PRINT_MSG_LINE_MAX_MSG; /* xx Msg in every line */
#  if defined(CONFIG_MULT_LINES) || defined(CONFIG_REDUNDANCY_SUPPORT)
static const char sl[2][5] = {"l0:", "l1:"};
const char * s = &sl[canLine][0];
#  else /* defined(CONFIG_MULT_LINES) || defined(CONFIG_REDUNDANCY_SUPPORT) */
static char s[] = "";
#  endif /* defined(CONFIG_MULT_LINES) || defined(CONFIG_REDUNDANCY_SUPPORT) */

    if( (pCAN_Msg->cobType & CO_COB_DISABLED) == 0)
	{
	    /* Msg for this node */
		PRINTF(" %s%x:%d: ", s, (int)pCAN_Msg->cobId, (int)pCAN_Msg->length);
    } else
	{
	    /* msg filtered by software */
		PRINTF(" (%s%x:%d:) ", s, (int)pCAN_Msg->cobId, (int)pCAN_Msg->length);
    }

    if( --n == 0)
	{
		PRINTF("\n");
		n = PRINT_MSG_LINE_MAX_MSG;
    }
} /* static void print_msg() */
#endif /* CONFIG_DRIVER_TEST */


#ifdef ______CONFIG_DUMMY_PRINTF
/*******************************************************************/
/*
 * empty stdio functions - only needed if you use our example without printf()
 * support.
 * For your application you should disable the define CONFIG_DUMMY_PRINTF
 * and should remove all references to these functions.
 *
 */
#  ifdef CONFIG_USE_DUMMY_FPRINTF
int fprintf(FILE *stream, const char *format, ...)
{
    return 0;
}
#  endif /* CONFIG_USE_DUMMY_FPRINTF */

#  ifdef CONFIG_USE_DUMMY_PRINTF
int printf(const char *format, ...)
{
    return 0;
}
#  endif /* CONFIG_USE_DUMMY_PRINTF */

#  ifdef CONFIG_USE_DUMMY_PUTCHAR
#    ifdef putchar
#      undef putchar
#    endif /* putchar */

#    ifdef PUTCHAR_CHAR
char putchar( char c)
#    else /* PUTCHAR_CHAR */
int putchar( int c )
#    endif /* PUTCHAR_CHAR */
{
    return c;
}
#  endif /* CONFIG_USE_DUMMY_PUTCHAR */

#  ifdef CONFIG_USE_DUMMY_FPUTC
int fputc( int c, FILE * stream)
{
    return c;
}
#  endif /* CONFIG_USE_DUMMY_FPUTC */

#  ifdef CONFIG_USE_DUMMY_FFLUSH
#    ifdef CONFIG_USE_DUMMY_FFLUSH_REGISTER
int fflush( register FILE * _fp)
#    else /* CONFIG_USE_DUMMY_FFLUSH_REGISTER */
int fflush( FILE * stream)
#    endif /* CONFIG_USE_DUMMY_FFLUSH_REGISTER */
{
    return 0;
}
#  endif /* CONFIG_USE_DUMMY_FFLUSH */

/*******************************************************************/
#endif /* ______CONFIG_DUMMY_PRINTF */


#ifdef CONFIG_NO_GLOBAL_VARS
/*******************************************************************/
/**
 * init driver pointer for usage of library without global variables
 *
 */
void init_canDriverPtr(
	UNSIGNED8	*ptr,		/**< pointer to drive data structure */
	CO_REDCY_PARA_DECL
    )
{
#  if defined(CONFIG_MULT_LINES) || defined(CONFIG_REDUNDANCY_SUPPORT)
    coptr->canDrvPtr[canLine] = ptr;
#  else /* defined(CONFIG_MULT_LINES) || defined(CONFIG_REDUNDANCY_SUPPORT) */
    coptr->canDrvPtr = ptr;
#  endif /* defined(CONFIG_MULT_LINES) || defined(CONFIG_REDUNDANCY_SUPPORT) */

    /* reset all can driver data */
    memset((void *)ptr, (int)0x0, sizeof(DRIVER_DATA_T));
} /* void init_canDriverPtr() */
#endif /* CONFIG_NO_GLOBAL_VARS */


#ifdef CONFIG_DYN_MEM_ALLOC
/*******************************************************************/
/**
 * init driver pointer for usage with dynamic memory allocation
 *
 */
void init_canDriverPtr(
	void		**pCobList,	/**< pointer to cob-list */
	void		**pIdxList,	/**< pointer to cob-index list */
	UNSIGNED16	cobCnt		/**< requested COB-Numbers */
	CO_COMMA_REDCY_PARA_DECL
    )
{
#  ifdef CONFIG_MULT_LINES
    if (pCobList == NULL)  {
	cobListLineCnts CO_LINE_PARA_ARRAY_INDEX = cobCnt;
	return;
    }
#  endif /* CONFIG_MULT_LINES */

#  ifdef CONFIG_REDUNDANCY_SUPPORT
    cobListLineCnts[CAN_DEFAULT_LINE] = cobCnt / 2;
    cobListLineCnts[CAN_REDCY_LINE] = cobCnt / 2;
#  endif /* CONFIG_REDUNDANCY_SUPPORT */

    p_cobList[0] = calloc(1, sizeof(COB_T) * cobCnt);
    *pCobList = p_cobList[0];
    co_maxCobCnt = cobCnt;

#  ifdef CONFIG_DRIVER_FAST_SORT
    p_cobIndexList[0] = calloc(1, sizeof(COB_ARRAY_INDEX_T) * cobCnt);
    *pIdxList = p_cobIndexList[0];
#  endif /* CONFIG_DRIVER_FAST_SORT */
} /* void init_canDriverPtr() */
#endif /* CONFIG_DYN_MEM_ALLOC */
/*______________________________________________________________________EOF_*/
