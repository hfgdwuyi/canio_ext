/*
 * can_stm32_fdcan - STM32 FDCAN CAN driver
 *
 * Copyright (c) 2020 port GmbH Halle (Saale)
 *
 *------------------------------------------------------------------
 */

/**
*  \file can_stm32_fdcan.c
*  \author port GmbH Halle (Saale)
*/
/**
*   \defgroup CanDriverFlags CAN driver State
*/
/**
*++  STM32 FDCAN CAN controller family
*--  STM32 FDCAN CAN controller Familie
*/
/**
* \def CONFIG_CAN_FULLCAN_SOFT_RTR
*-- Der CAN controller wird im FullCAN Mode betrieben.
*-- Andernfalls wird der BasicCAN Mode benutzt.
*-- Im FullCAN Mode werden mittels Akzeptanzmasken und Akzeptanzfilter
*-- die empfangenen CAN Nachrichten per Hardware gefiltert.
*-- Im Allgemeinen benötigt man je CAN Identifier eine
*-- eigene CAN Controller Message box.
*-- Im BasicCAN mode werden nur einige wenige CAN Controller Message Boxen
*-- benutzt. Typischerweise ein Kanal zum Senden ::CAN_TRANSMIT_OBJ
*-- und ein Kanal zum Empfangen ::CAN_ALL_RECEIVE_OBJ.
*++ The CAN controller is used in FullCAN mode.
*++ If this define is not set BasicCAN mode is used.
*++ In the FullCAN mode the CAN messages are filtered by hardware using
*++ acceptance masks and acceptance filters.
*++ Typically for every CAN identifier an own hardware message buffer
*++ is needed.
*++ In BasicCAN mode only few hardware messages are used.
*++ Typically one hardware message for sending ::CAN_TRANSMIT_OBJ
*++ and one hardware message for receiving ::CAN_RECEIVE_OBJ.
*
*-- RTR Messages werden per Software beantwortet.
*++ RTR messages are handled by the software.
*/
/**
* \def CONFIG_CAN_FULLCAN
*-- Der CAN controller wird im FullCAN Mode betrieben.
*-- Andernfalls wird der BasicCAN Mode benutzt.
*-- Im FullCAN Mode werden mittels Akzeptanzmasken und Akzeptanzfilter
*-- die empfangenen CAN Nachrichten per Hardware gefiltert.
*-- Im Allgemeinen benötigt man je CAN Identifier eine
*-- eigene CAN Controller Message box.
*-- Im BasicCAN mode werden nur einige wenige CAN Controller Message Buffer
*-- benutzt. Typischerweise ein Kanal zum Senden ::CAN_TRANSMIT_OBJ
*-- und ein Kanal zum Empfangen ::CAN_ALL_RECEIVE_OBJ.
*++ The CAN controller is used in FullCAN mode.
*++ If this define is not set BasicCAN mode is used.
*++ In the FullCAN mode the CAN messages are filtered by hardware using
*++ acceptance masks and acceptance filters.
*++ Typically for every CAN identifier an own hardware message buffer
*++ is needed.
*++ In BasicCAN mode only few hardware messages are used.
*++ Typically one hardware message for sending ::CAN_TRANSMIT_OBJ
*++ and one hardware message for receiving ::CAN_RECEIVE_OBJ.
*
*-- RTR Messages werden nicht unterstützt.
*++ RTR messages are not supported
*/
/**
* \def CONFIG_CAN_ONLY_ONE_TRANSMIT_CHANNEL
*-- Sonderform des FullCAN Modes
*-- (CONFIG_CAN_FULLCAN_SOFT_RTR muss gesetzt sein).
*-- Für alle Sendeobjekte wird trotz FullCAN Modes nur ein einziger
*-- Message Buffer benutzt.
*++ Special mode for FullCAN mode.
*++ The define CONFIG_FULLCAN_SOFT_RTR has to be set.
*++ For all transmit objects one hardware message
*++ is used.
*/
/**
* \def CONFIG_DONT_USE_ISR
*++ Do not use the predefined interrupt function.
*++ Use your own interrupt function
*-- Die Interruptfunktion des Anwenders wird eingebunden.
*/
/**
* \def CONFIG_CAN_TX_TEST
*++ Send a test message within Init_CAN() without interrupts
*++ This is only for test purposes of the CAN controller.
*-- In Init_CAN() wird eine Test-Message
*-- ohne Benutzung von Interrupts verschickt.
*-- Es ist nur geeignet, um den CAN-controller zu testen.
*/
/**
* \def DEF_HW_PART
*++ Activate hardware depend settings within the configuration
*-- Aktiviert die hardwareabhängigen Einstellungen in der Konfiguration
*/
/**
* \def CONFIG_CAN_SENDING_FLAG
*++ Use eSending flag instead of/additional to the hardware access
*++ for 'Transmission active' check.
*++ This is only seldom used
*-- Benutzt das Flag eSending anstatt Hardwarezugriffe,
*-- um den SendeStatus zu prüfen.
*-- Das ist eine selten gebrauchte Einstellung.
*/
/**
* \def CONFIG_EXPERIMENTAL
*++ Activate debug settings.
*-- Aktiviert Debug Einstellungen.
*/
/**
* \def CONFIG_CAN_DEBUG_VARS
*-- Zusätzliche Debug-Variablen werden aktiviert.
*++ Debugging variables are defined.
*/
/**
* \def CONFIG_SIMULATOR
*++ For use with a Simulator.
*++ This definition deactivates hardware checks.
*-- Zur Verwendung im Simulator.
*-- Es werden Hardwareprüfungen deaktiviert.
*/
/**
* \def CONFIG_CAN_ISR_DISABLE_INT
*++ Call 'disable CAN interrupt' within CAN ISR.
*-- In der CAN ISR wird 'disable CAN interrupt' aufgerufen.
*
*++ Default is: not set.
*-- Normalerweise nicht gesetzt.
*/
#ifdef DOXYGEN
	/* only for generation of the documentation */
# define CONFIG_CAN_FAMILY_FDCAN
# define CONFIG_DONT_USE_ISR
# undef CONFIG_DONT_USE_ISR
# define CONFIG_CAN_TX_TEST
# define DEF_HW_PART
# define CONFIG_EXPERIMENTAL
# define CONFIG_CAN_DEBUG_VARS
# define CONFIG_SIMULATOR
# define CONFIG_CAN_SENDING_FLAG
# undef CONFIG_CAN_SENDING_FLAG
# define CONFIG_CAN_ISR_DISABLE_INT
# define CONFIG_CAN_FULLCAN
# undef CONFIG_CAN_FULLCAN
# define CONFIG_CAN_FULLCAN_SOFT_RTR
# define CONFIG_CAN_ONLY_ONE_TRANSMIT_CHANNEL
# undef CONFIG_CAN_ONLY_ONE_TRANSMIT_CHANNEL
#endif
/*---------------------------------------------------------------------------*/

/* includes */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

# include <environ.h>

#define DEF_HW_PART
#include <cal_conf.h>

#ifdef CONFIG_CAN_FAMILY_FDCAN

# include <co_type.h>
# include <co_def.h>
# include <co_drv.h>
# include <co_drvif.h>
# include <co_flag.h>
# include <co_mcpy.h>
# ifdef CONFIG_SYNC_CONSUMER
#  include <co_sync.h>
# endif /* CONFIG_SYNC_CONSUMER */
# ifdef CONFIG_REDUNDANCY_SUPPORT
#  include <co_redcy.h>
# endif /* CONFIG_REDUNDANCY_SUPPORT */

#ifdef CONFIG_NO_GLOBAL_VARS
	/* special driver header -> user adaptation */
# include <glob_drv.h>
#else /* CONFIG_NO_GLOBAL_VARS */
# include <cdriver.h>
# include <can_stm32_fdcan.h>
#endif /* CONFIG_NO_GLOBAL_VARS */

# if defined (CONFIG_EXPERIMENTAL) || \
     defined(CONFIG_DRIVER_TEST)
	/* for driver test or time measurement */
#  include <examples.h>
# endif /* CONFIG_EXPERIMENTAL */

/* constant definitions
---------------------------------------------------------------------------*/
#ifndef CONFIG_CAN_ISR_PRESTRING
# define CONFIG_CAN_ISR_PRESTRING void
#endif
#ifndef CONFIG_CAN_ISR_POSTSTRING
# define CONFIG_CAN_ISR_POSTSTRING
#endif

/* local defined data types
---------------------------------------------------------------------------*/

/* list of external used functions, if not in headers
---------------------------------------------------------------------------*/

/* list of global defined functions
---------------------------------------------------------------------------*/
void * getBitTiming(UNSIGNED16, void *);

#ifdef CONFIG_EXPERIMENTAL
void print_eType( COB_KIND_T eType);
void print_hex( UNSIGNED8 * addr, UNSIGNED8 length );
#endif

/* list of local defined functions
---------------------------------------------------------------------------*/
#ifdef CONFIG_DRIVER_TEST
static void	sbuf(unsigned char *s, int n, char *fmt);
#endif /* CONFIG_DRIVER_TEST */

static void GetNext_TX_Request(CO_REDCY_PARA_DECL );
static void setNewDriverState( UNSIGNED8 newState CO_COMMA_REDCY_PARA_DECL);
#ifdef CONFIG_MULT_LINES
static UNSIGNED8 CAN_GetCanLine(FDCAN_HandleTypeDef *tmp_hfdcan);
#endif
 
#ifdef CONFIG_CAN_FULLCAN_SOFT_RTR
#warning: " ---- FULLCAN mode not implemented yet! ----------- "
static UNSIGNED8 NEAR initChannel( COB_T * CO_COMMA_GLOBVARS_PARA_DECL);
#endif /* CONFIG_CAN_FULLCAN_SOFT_RTR */


/* external variables
---------------------------------------------------------------------------*/

/* global variables
---------------------------------------------------------------------------*/
FDCAN_HandleTypeDef hfdcan CO_REDCY_PARA_ARRAY_DEF;

/* bit timing table
----------------------------------------------------------------------------*/
                /* rate, prescaler, segment 1, segment 2 */
CO_CONST BTR_TAB_FDCAN_T can_btr_tab_fdcan[] = {
			    {  10, CAN_PRSC_10K, CAN_BTR0_10K,  CAN_BTR1_10K },
			    {  20, CAN_PRSC_20K, CAN_BTR0_20K,  CAN_BTR1_20K },
			    {  50, CAN_PRSC_50K, CAN_BTR0_50K,  CAN_BTR1_50K },
			    { 100, CAN_PRSC_100K, CAN_BTR0_100K, CAN_BTR1_100K },
			    { 125, CAN_PRSC_125K, CAN_BTR0_125K, CAN_BTR1_125K },
			    { 250, CAN_PRSC_250K, CAN_BTR0_250K, CAN_BTR1_250K },
			    { 500, CAN_PRSC_500K, CAN_BTR0_500K, CAN_BTR1_500K },
			    { 800, CAN_PRSC_800K, CAN_BTR0_800K, CAN_BTR1_800K },
			    {1000, CAN_PRSC_1000K, CAN_BTR0_1000K,  CAN_BTR1_1000K },
			    {0, 0, 0, 0}  /* last entry */
                };


/* local defined variables
---------------------------------------------------------------------------*/
#ifdef CONFIG_NO_GLOBAL_VARS
#else /* CONFIG_NO_GLOBAL_VARS */

# ifdef CONFIG_CAN_SENDING_FLAG
static VOLATILE BOOL_T eSending CO_REDCY_PARA_ARRAY_DEF;
# endif /* CONFIG_CAN_SENDING_FLAG */

# ifdef CONFIG_FAST_SORT
/**
* Start_CAN() called?
*/
static BOOL_T fStartCan CO_REDCY_PARA_ARRAY_DEF;
# endif
static volatile BOOL_T canInitialized CO_REDCY_PARA_ARRAY_DEF;

/* types for HAL layer API access */                
FDCAN_FilterTypeDef sFilterConfig;
static FDCAN_TxHeaderTypeDef TxHeader;
static FDCAN_RxHeaderTypeDef RxHeader;
static uint8_t TxData[8] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
static uint8_t RxData[8] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
static const uint8_t BytestoDLC[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 12, 16, 20, 24, 32, 48, 64};
                
/*-----------------------------*/
# ifdef CONFIG_SYNC_CONSUMER
#  ifdef CONFIG_CAN_FULLCAN_SOFT_RTR
static UNSIGNED8 coSyncChannel CO_REDCY_PARA_ARRAY_DEF;
#  else /* CONFIG_CAN_FULLCAN_SOFT_RTR */
static COB_IDENT_T coSyncId CO_REDCY_PARA_ARRAY_DEF;
#  endif /* CONFIG_CAN_FULLCAN_SOFT_RTR */
# endif /* CONFIG_SYNC_CONSUMER */
/*-----------------------------*/

# ifdef CONFIG_CAN_FULLCAN_SOFT_RTR
/** CAN controller channel count */
static UNSIGNED8 coSavedChannel CO_REDCY_PARA_ARRAY_DEF;
# endif /* CONFIG_CAN_FULLCAN_SOFT_RTR */

#endif /* CONFIG_NO_GLOBAL_VARS */


#ifdef CONFIG_DRIVER_TEST
/***************************************************************************
*
*++ sbuf - shows a byte array with the specified format
*-- sbuf - zeigt ein Byte Array mit Formatierung an
*
*/
static void sbuf(
	unsigned char *s,	/* pointer to byte array */
	int n,			/* nuber of conversions  */
	char *fmt		/* printf() format string */
	)
{
int i;

    for(i = 0; i< n; i++) {
	fprintf(stdout, fmt, *s++);
    }
    putchar('\n');
}
#endif /* CONFIG_DRIVER_TEST */


/*******************************************************************/
/**
*
*++ \brief Init_CAN - initialize the CAN-Controller
*-- \brief Init_CAN - initialisiert den CAN-Controller
*
*++ This function resets the global message buffer.
*++ It sets the bit rate for the CAN controller.
*++ The CAN controller is not started by this function.
*-- Diese Funktion setzt die Nachrichtenpuffer zurück.
*-- Sie initialisiert den CAN-Controller mit der Bitrate.
*-- Der CAN-Controller wird durch diese Funktion nicht gestartet.
*++ Without the define CONFIG_CAN_FULLCAN_SOFT_RTR the
*++ CAN Controller works in the Basic CAN mode.
*-- Wenn \c CONFIG_CAN_FULLCAN_SOFT_RTR _nicht_ gesetzt ist,
*-- wird der CAN-Controller im Basic-CAN-Mode betrieben.
*
*
* \retval CO_INIT_CAN_OK
*++ success
*-- Erfolg
*
* \retval CO_E_INIT_HARD_RES_ACTIVE
*++ reset is active, init failed
*-- Fehler, Hardware Reset is aktiv
*
* \retval CO_E_INIT_BAUD
*++ couldn't adjust baudrate, init failed
*-- Fehler, ungültige Baudrate
*
*/

UNSIGNED8 Init_CAN(
	  UNSIGNED8 module,      /**< module of the FDCAN controller */
      UNSIGNED16 wBaudRate  /**< baudrate (e.g. 50 = 50kBaud) */
	  CO_COMMA_REDCY_PARA_DECL
          )
{
# if defined(CONFIG_DRIVER_TEST) && defined(CONFIG_NO_GLOBAL_VARS)
/* for better debugging */
VOLATILE DRIVER_DATA_T * pDriverData;

    pDriverData = (DRIVER_DATA_T*)(GL_VAR(canDrvPtr) CO_REDCY_PARA_ARRAY_INDEX);
# endif /* defined(CONFIG_DRIVER_TEST) && defined(CONFIG_NO_GLOBAL_VARS) */

# ifdef  CONFIG_DRIVER_TEST
#  ifdef CONFIG_CAN_FULLCAN_SOFT_RTR
#   ifdef CONFIG_CAN_ONLY_ONE_TRANSMIT_CHANNEL
	PRINTF(".. def CONFIG_CAN_FULLCAN_SOFT_RTR, ONLY_ONE_TX ->%s\n", __TIME__);
#   else /* CONFIG_CAN_ONLY_ONE_TRANSMIT_CHANNEL */
	PRINTF(".. def CONFIG_CAN_FULLCAN_SOFT_RTR, MANY_TX ->%s\n", __TIME__);
#   endif /* CONFIG_CAN_ONLY_ONE_TRANSMIT_CHANNEL */
#  else /* CONFIG_CAN_FULLCAN_SOFT_RTR */
	PRINTF(".. notdef CONFIG_CAN_FULLCAN_SOFT_RTR ->%s\n", __TIME__);
#  endif /* CONFIG_CAN_FULLCAN_SOFT_RTR */
# endif	/* CONFIG_DRIVER_TEST */

    /* all CAN interrupt must be disabled for base address switch */
    DISABLE_CAN_INTERRUPTS(CO_REDCY_PARA);

# ifdef CONFIG_CAN_DEBUG_VARS
    /* interrupt counters */
    GL_DRV_ARRAY(cal_ch_ints) = 0;
    GL_DRV_ARRAY(cal_tx_ints) = 0;
    GL_DRV_ARRAY(cal_rx_ints) = 0;
# endif /* CONFIG_CAN_DEBUG_VARS */

    /* reset internal driver state information */
    GL_DRV_ARRAY(coCanDriverState) = CANFLAG_INIT;
    /* set internal driver state */
	GL_DRV_ARRAY(canInitialized) = CO_FALSE;

    /* reset driver signal flags */
    RESET_CAN_FLAG(0xFF);

# ifdef CONFIG_FAST_SORT
    GL_DRV_ARRAY(fStartCan) = CO_FALSE;
# endif

# ifdef CONFIG_CAN_SENDING_FLAG
    GL_DRV_ARRAY(eSending) = CO_FALSE;
# endif /* CONFIG_CAN_SENDING_FLAG */

    /* reset TX/RX buffer */
    clearTxBuffer(CO_REDCY_PARA);
    clearRxBuffer(CO_REDCY_PARA);

    /* set the used CAN instance */
    if(module == 1) {
       GL_DRV_ARRAY(hfdcan).Instance = FDCAN1;
    }
    else if(module == 2) {
# ifdef FDCAN2
        GL_DRV_ARRAY(hfdcan).Instance = FDCAN2;
# endif
    }
    else if(module == 3) {
# ifdef FDCAN3
        GL_DRV_ARRAY(hfdcan).Instance = FDCAN3;
# endif
    }
    else {
        /* module not possible! */
        return(CO_E_INIT_PROP);
    }
    /* build the device init struct */
# ifdef CONFIG_CPU_FAMILY_STM32_G4
    GL_DRV_ARRAY(hfdcan).Init.ClockDivider = FDCAN_CLOCK_DIV1;
# endif
    GL_DRV_ARRAY(hfdcan).Init.FrameFormat = FDCAN_FRAME_CLASSIC;
    GL_DRV_ARRAY(hfdcan).Init.Mode = FDCAN_MODE_NORMAL;
    GL_DRV_ARRAY(hfdcan).Init.AutoRetransmission = ENABLE;
    GL_DRV_ARRAY(hfdcan).Init.TransmitPause = DISABLE;
    GL_DRV_ARRAY(hfdcan).Init.ProtocolException = DISABLE;
    GL_DRV_ARRAY(hfdcan).Init.StdFiltersNbr = 1;
    GL_DRV_ARRAY(hfdcan).Init.ExtFiltersNbr = 0;
# ifdef CONFIG_CPU_FAMILY_STM32_H7
    GL_DRV_ARRAY(hfdcan).Init.RxFifo0ElmtsNbr = 2;
    GL_DRV_ARRAY(hfdcan).Init.RxFifo0ElmtSize = FDCAN_DATA_BYTES_8;
    GL_DRV_ARRAY(hfdcan).Init.TxFifoQueueElmtsNbr = 2;
    GL_DRV_ARRAY(hfdcan).Init.TxElmtSize = FDCAN_DATA_BYTES_8;
# endif
    GL_DRV_ARRAY(hfdcan).Init.TxFifoQueueMode = FDCAN_TX_FIFO_OPERATION;

    /* initialize baudrate */
    if (Set_Baudrate( wBaudRate, NULL CO_COMMA_REDCY_PARA) != CO_INIT_CAN_OK)
    {
	    /* error handling */
        GL_DRV_ARRAY(hfdcan).Instance = NULL;
        return(CO_E_INIT_BAUD);
    }
    /* init the CAN instance */
    if (HAL_FDCAN_Init(&GL_DRV_ARRAY(hfdcan)) != HAL_OK)
    {
        GL_DRV_ARRAY(hfdcan).Instance = NULL;
        return(CO_E_INIT_UNSPEC_ERROR);
    }
    /* initialize COB Array access */
    initCobList( CO_REDCY_PARA );

/* ================================================================= */
# ifdef CONFIG_CAN_FULLCAN_SOFT_RTR
/* ================================================================= */
    /* calculate first free channel without predefined functionality */
    GL_DRV_ARRAY(coSavedChannel) = CAN_FIRST_FREE_OBJ;

    /* special handling for some services */
#  ifdef CONFIG_SYNC_CONSUMER
    GL_DRV_ARRAY(coSyncChannel) = CAN_NO_CHANNEL;
#  endif /* CONFIG_SYNC_CONSUMER */

#  ifdef CONFIG_CAN_ONLY_ONE_TRANSMIT_CHANNEL
/* ================================================================= */
/*  create a Transmit Object */
/* ================================================================= */
    CAN_INIT_OBJ_PTR(CAN_TRANSMIT_OBJ);

    /* TODO:
     * init hardware message buffer within the CAN Controller
     * for the Transmit Object */

#  endif /* CONFIG_CAN_ONLY_ONE_TRANSMIT_CHANNEL */

/* ================================================================= */
# else /* CONFIG_CAN_FULLCAN_SOFT_RTR */
/* ================================================================= */

/* ----------------------------------------------------------------- */
/* ------------ BasicCAN mode -------------------------------------- */
/* ----------------------------------------------------------------- */

#  ifdef CONFIG_SYNC_CONSUMER
    /* special handling for some services */
    GL_DRV_ARRAY(coSyncId) = CAN_NO_COBID;
#  endif

/* ================================================================= */
/*  create a Transmit Object */
/* ================================================================= */

     /* Prepare Tx message Header */
    TxHeader.Identifier = 0;
    TxHeader.IdType = FDCAN_STANDARD_ID;
    TxHeader.TxFrameType = FDCAN_DATA_FRAME;
    TxHeader.DataLength = FDCAN_DLC_BYTES_0;
    TxHeader.ErrorStateIndicator = FDCAN_ESI_ACTIVE;
    TxHeader.BitRateSwitch = FDCAN_BRS_OFF;
    TxHeader.FDFormat = FDCAN_CLASSIC_CAN;
    TxHeader.TxEventFifoControl = FDCAN_NO_TX_EVENTS;
    TxHeader.MessageMarker = 0;

/* ================================================================= */
/*  create a Receive Object */
/* ================================================================= */

    /* Configure reception filter to Rx FIFO 0 on FDCAN instance */
    sFilterConfig.IdType = FDCAN_STANDARD_ID;
    sFilterConfig.FilterIndex = 0;
    sFilterConfig.FilterType = FDCAN_FILTER_MASK;
    sFilterConfig.FilterConfig = FDCAN_FILTER_TO_RXFIFO0;
    sFilterConfig.FilterID1 = 0x0000;
    sFilterConfig.FilterID2 = 0x0000;
    if (HAL_FDCAN_ConfigFilter(&GL_DRV_ARRAY(hfdcan), &sFilterConfig) != HAL_OK)
    {
        GL_DRV_ARRAY(hfdcan).Instance = NULL;
        return(CO_E_INIT_UNSPEC_ERROR);
    }

/* ================================================================= */
# endif /* CONFIG_CAN_FULLCAN_SOFT_RTR */
/* ================================================================= */

    /* Activate Rx FIFO 0 new message and message lost notification on FDCAN instance */
    if (HAL_FDCAN_ActivateNotification(&GL_DRV_ARRAY(hfdcan), (FDCAN_IT_RX_FIFO0_NEW_MESSAGE | 
                                    FDCAN_IT_RX_FIFO0_MESSAGE_LOST), 0) != HAL_OK)
    {
        GL_DRV_ARRAY(hfdcan).Instance = NULL;
        return(CO_E_INIT_UNSPEC_ERROR);
    }
    /* Activate Tx FIFO Transmission Completed notification on FDCAN instance */
    if (HAL_FDCAN_ActivateNotification(&GL_DRV_ARRAY(hfdcan), FDCAN_IT_TX_COMPLETE, FDCAN_TX_BUFFER0) != HAL_OK)
    {
        GL_DRV_ARRAY(hfdcan).Instance = NULL;
        return(CO_E_INIT_UNSPEC_ERROR);
    }
    /* Activate Error state (Passiv, BussOff) notification on FDCAN instance */
    if (HAL_FDCAN_ActivateNotification(&GL_DRV_ARRAY(hfdcan), 
                         (FDCAN_IT_ERROR_PASSIVE | FDCAN_IT_BUS_OFF), 0) != HAL_OK)
    {
        GL_DRV_ARRAY(hfdcan).Instance = NULL;
        return(CO_E_INIT_UNSPEC_ERROR);
    }
    RESTORE_CAN_INTERRUPTS(CO_REDCY_PARA);
    
     /* CAN successful initialized */
	GL_DRV_ARRAY(canInitialized) = CO_TRUE;

    return(CO_INIT_CAN_OK);
}

/*******************************************************************/
/**
*
*-- \brief getBitTiming - gibt einen Zeiger auf einen
*--			BitTiming-Eintrag zurück
*++ \brief getBitTiming - returns a pointer to an entry
*++			in the bittiming table
*
* \param rate
*	Baudrate (125 == 125 kBit/s)
*
* \param p_usr_tab
*--	Zeiger auf eine anwenderdefinierte Tabelle\n
*--	NULL == interne Tabelle (nach CiA Definition)
*++     pointer to a manufacturer specific table\n
*++     NULL == internal table (defined by the CiA)
*
* \retval NULL
*--	Eintrag existiert nicht
*++	entry doesn't exist
*
* \retval not NULL
*--	Zeiger auf einen BitTiming-Eintrag
*++	pointer to the bittiming entry
*
*/
void * getBitTiming(
	UNSIGNED16 rate, 	/* Baudrate (125 == 125kBit/s )*/
	void * p_usr_tab	/* NULL == internal table */
)
{
/* volatile */ CO_CONST BTR_TAB_FDCAN_T * p_tab;
UNSIGNED8 i;

    if (rate == 0) {    	/* Baudrate 0 isn't allowed */
    	return( NULL );
    }

    if (p_usr_tab == NULL) {  	/* default table for Bittimings */
    	p_tab = can_btr_tab_fdcan;
    } else {
    	p_tab = (CO_CONST BTR_TAB_FDCAN_T*)p_usr_tab;
    }

    /* search data from table */
    i = 0; /* init */
    while(1) {
        if (p_tab[i].rate == 0) {
            return( NULL );
        }
    	if (p_tab[i].rate == rate) {
    	    break; /* bittiming entry found */
    	}
    	i++;
    }

    /* Is it a possible entry? */
    if((p_tab[i].btr0 == 0) && (p_tab[i].btr1 == 0)) {
    	return NULL;
    }

    return( (void *)&p_tab[i] );
}


/*******************************************************************/
/**
*
*++ \brief Set_Baudrate - setting the bitrate of the CAN controlers
*-- \brief Set_Baudrate - setzt den CAN-Controller auf die gewünschte Bitrate
*
*++ The function recalibrates the CAN controller
*++ with the given bitrate in Kbit/s.
*++ The bit timing values for this bitrate are obtained from
*++ the table given as argument.
*++ If no table is given, the default table is used.
*++ After finishing the function, the CAN controller is left stopped.
*
*-- Diese Funktion stellt den CAN-Controller auf die übergebene
*-- Bitrate (in kBit/s) ein.
*-- Die dazugehörigen Bittimings werden
*-- der übergebenen Tabelle entnommen.
*-- Wird keine solche Tabelle übergeben,
*-- wird die vordefinierte Tabelle benutzt.
*-- Nach Ausführung der Funktion
*-- befindet sich der CAN-Controller im gestoppten
*-- Zustand.
*
* \param rate
*	Bitrate (125 == 125 kBit/s)
*
* \param p_usr_tab
*--	Zeiger auf eine anwenderdefinierte Tabelle\n
*--	NULL == interne Tabelle (nach CiA Definition)
*++     pointer to a manufacturer specific table\n
*++     NULL == internal table (defined by the CiA)
*
* \retval CO_INIT_CAN_OK
*++ success
*-- Erfolg
*
* \retval CO_E_INIT_BAUD
*++ invalid bitrate, init failed
*-- Fehler, ungültige Bitrate
*
*/

UNSIGNED8 Set_Baudrate(
	UNSIGNED16 rate, 	/* Bitrate (125 == 125kBit/s )*/
	BTR_TAB_T * p_usr_tab	/* NULL == internal table */
	CO_COMMA_REDCY_PARA_DECL
)
{
BTR_TAB_FDCAN_T *p_tab;	/* pointer to a bittime entry */

    if((GL_DRV_ARRAY(hfdcan).Instance) == NULL) {
    	return( CO_E_INIT_BAUD );
    }

    DISABLE_CAN_INTERRUPTS(CO_REDCY_PARA);

    /* signal new state */
    setNewDriverState(CANFLAG_INIT CO_COMMA_REDCY_PARA);

    p_tab = getBitTiming( rate, p_usr_tab);
    if (p_tab == NULL) {
        return( CO_E_INIT_BAUD );
    }

    /*
     * write timings to CAN init struct
     * p_tab->presc
     * p_tab->btr0
     * p_tab->btr1
     */
    GL_DRV_ARRAY(hfdcan).Init.NominalPrescaler = p_tab->presc;
    GL_DRV_ARRAY(hfdcan).Init.NominalSyncJumpWidth = CAN_SJW;
    GL_DRV_ARRAY(hfdcan).Init.NominalTimeSeg1 = p_tab->btr0;
    GL_DRV_ARRAY(hfdcan).Init.NominalTimeSeg2 = p_tab->btr1;
    
    if(GL_DRV_ARRAY(canInitialized) == CO_TRUE) {
        /* if necessary new init the CAN instance */
        if (HAL_FDCAN_Init(&GL_DRV_ARRAY(hfdcan)) != HAL_OK)
        {
            return(CO_E_INIT_UNSPEC_ERROR);
        }
    }
    /* The CAN controller is in the stopped mode at this position.
     * -> Start_CAN() activate the controller with the new baudrate.
     */

    RESTORE_CAN_INTERRUPTS(CO_REDCY_PARA);

    return(CO_INIT_CAN_OK);
}


/*******************************************************************/
/**
*
*++ \brief Start_CAN - starts the CAN-Controller
*-- \brief Start_CAN - startet den CAN-Controller
*
*++ Starts the CAN-Controller by resetting the init bit.
*-- Startet den CAN-Controller durch Rücksetzen des Init Bits.
*
* \returns
*++ nothing
*-- nichts
*
*/

void Start_CAN(
	CO_REDCY_PARA_DECL
     )
{
    if((GL_DRV_ARRAY(hfdcan).Instance) == NULL) {
    	return;
    }

    DISABLE_CAN_INTERRUPTS(CO_REDCY_PARA);

# ifdef CONFIG_FAST_SORT
    GL_DRV_ARRAY(fStartCan) = CO_TRUE;

    createCobIdIndex(CO_REDCY_PARA);
# endif /* CONFIG_FAST_SORT */

# ifdef CONFIG_CAN_SENDING_FLAG
    /* typically Stop_CAN() and Clear_busoff() set eSending to false */
    /* GL_DRV_ARRAY(eSending) = CO_FALSE; */
# endif /* CONFIG_CAN_SENDING_FLAG */

    /* Start the FDCAN module */
    if(HAL_FDCAN_Start(&GL_DRV_ARRAY(hfdcan)) != HAL_OK)
    { /* error in start CAN */
# ifdef CONFIG_EXPERIMENTAL     
        PRINTF("error in Start_CAN!");
#endif
    }
	
	/* signal new state */
    setNewDriverState(CANFLAG_ACTIVE CO_COMMA_LINE_PARA);

    /* enable CAN interrupts for this line */
    ENABLE_CAN_INTERRUPTS(CO_REDCY_PARA);
    
    /* get next transmission request */
	GetNext_TX_Request(CO_REDCY_PARA);
}


/*******************************************************************/
/**
*
*++ \brief Stop_CAN - stops the CAN-Controller
*-- \brief Stop_CAN - stoppt den CAN-Controller
*
*++ Stops the CAN controller by setting the init bit.
*++ All error at the CAN controller are reset.
*-- Stoppt den CAN-Controller durch Setzen des Init Bits.
*-- Alle Fehler auf dem CAN-Controller werden zurückgesetzt.
*
* \returns
*++ nothing
*-- nichts
*
*/

void Stop_CAN(
	CO_REDCY_PARA_DECL
     )
{
    if((GL_DRV_ARRAY(hfdcan).Instance) == NULL) {
    	return;
    }

    DISABLE_CAN_INTERRUPTS(CO_REDCY_PARA);
    /* signal new state */
    setNewDriverState(CANFLAG_INIT CO_COMMA_REDCY_PARA);

# ifdef CONFIG_FAST_SORT
    GL_DRV_ARRAY(fStartCan) = CO_FALSE;
# endif

# ifdef CONFIG_CAN_SENDING_FLAG
    GL_DRV_ARRAY(eSending) = CO_FALSE;
# endif /* CONFIG_CAN_SENDING_FLAG */
    
    /* Stop the FDCAN module */
    if(HAL_FDCAN_Stop(&GL_DRV_ARRAY(hfdcan)) != HAL_OK)
    { /* error in stop CAN */
# ifdef CONFIG_EXPERIMENTAL     
        PRINTF("error in Stop_CAN!");
# endif
    }
}

/*******************************************************************/
/**
*
*++ \brief Clear_busoff - starts the CAN-Controller after bus-off
*-- \brief Clear_busoff - startet den CAN Controller nach einem Bus OFF
*
*++ In case of a Bus-OFF the controller is in init/reset mode.
*++ The init bit has to be cleared to go bus on again.
*-- Im Fall, dass der CAN Controller in den Bus-Off Zustand gegangen
*-- und das Init-Reset Bit gesetzt ist,
*-- kann mit dem Aufruf dieser Funktion das Rücksetzen des Init-Bit
*-- versucht werden, den CAN Controller wieder an den Bus zu bringen.
*
*++ If this call is used more times successively,
*++ a waiting time between the calls should be used.
*-- Bei wiederholten Aufrufen, sollte eine Wartezeit zwischen
*-- den Aufrufen liegen.
*
* \returns
*++ nothing
*-- nichts
*/

void Clear_busoff(
	CO_REDCY_PARA_DECL
     )
{
    if((GL_DRV_ARRAY(hfdcan).Instance) == NULL) {
    	return;
    }

    /* go Bus on, e.g. Start_CAN() */
    Stop_CAN(CO_REDCY_PARA);
    Start_CAN(CO_REDCY_PARA);
}

/***************************************************************************/
/**
*++ \brief Define_COB - creates a COB in the COB-list with attributes
*-- \brief Define_COB - erzeugt ein COB in der COB-list mit Attributen
*
*++ Creates a COB in the COB-list with attributes given as parameter.
*-- Es wird ein COB in der COB-Liste mit den als Parametern übergebenen
*-- Attributen erzeugt.
*
*++ With Full-CAN controllers also object channels
*++ in the controllers hardware are occupied.
*++ The channel is configured according to the type of the COB
*++ as transmit or receive object.
*++ The COB ID is assigned later on, with a call to Set_COB_ID().
*-- Bei Full-CAN-Controllern werden die Objektkanäle
*-- im CAN Controller belegt.
*-- Die Kanäle werden je nach Typ des COB als
*-- Sende- oder Empfangsobjekt konfiguriert.
*-- Die COB-ID wird erst später beim Aufruf von Set_COB_ID()
*-- gesetzt.
*
* \see CONFIG_CAN_FULLCAN_SOFT_RTR
* \see Set_COB_ID()
*
* \returns
*++ pointer to COB
*-- Zeiger auf COB
*
* \retval  not NULL
*++ success
*-- Erfolg
*
* \retval NULL
*++ definition failed, e.g. no more channels within Full-CAN controller
*++ or no COB-memory.
*-- Fehler, z.B. keine Kanäle mehr frei auf Full-CAN Controller oder
*-- kein COB-Speicher frei.
*/
COB_T *Define_COB(
		COB_KIND_T eType,			/**< COB type */
		UNSIGNED8 bLength			/**< COB length */
		CO_COMMA_REDCY_PARA_DECL
		)
{
COB_T *pCOB;
#  ifdef CONFIG_CAN_FULLCAN_SOFT_RTR
UNSIGNED8 bFilter; /* filter number */
#  endif /* CONFIG_CAN_FULLCAN_SOFT_RTR */


#  ifdef CONFIG_DRIVER_TEST
#    ifdef CONFIG_NO_GLOBAL_VARS
    /* for better debugging */
VOLATILE DRIVER_DATA_T * pDriverData;

    pDriverData = (DRIVER_DATA_T*)(GL_VAR(canDrvPtr) CO_REDCY_PARA_ARRAY_INDEX);
#    endif /* CONFIG_NO_GLOBAL_VARS */

#    if defined(CONFIG_MULT_LINES) || defined(CONFIG_REDUNDANCY_SUPPORT)
    PRINTF("L %d: ", (int)canLine);
#    endif /* CONFIG_MULT_LINES || CONFIG_REDUNDANCY_SUPPORT */
    PRINTF("Define_COB(): ");
#  endif /* CONFIG_DRIVER_TEST */

/*---------------------------------------------------------------------*/
/* create a new COB */
/*---------------------------------------------------------------------*/
    pCOB = initCobEntry(CO_REDCY_PARA);

    if (pCOB == NULL) {
    	return(NULL); /* not enough COB entries  */
    }

/*---------------------------------------------------------------------*/
    pCOB->cobId    = CAN_NO_COBID; /* disable ID */
    pCOB->bLength  = bLength;
    pCOB->eType    = (COB_KIND_T)(CO_COB_DISABLED | eType);
    pCOB->bChannel = CAN_NO_CHANNEL; /* no channel */
#  if defined(CONFIG_MULT_LINES) || defined(CONFIG_REDUNDANCY_SUPPORT)
    pCOB->canLine  = canLine;
#  endif /* CONFIG_MULT_LINES || CONFIG_REDUNDANCY_SUPPORT */
/*---------------------------------------------------------------------*/

/*---------------------------------------------------------------------*/

/*---------------------------------------------------------------------*/
#  ifdef CONFIG_CAN_FULLCAN_SOFT_RTR
    bFilter = initChannel(pCOB CO_COMMA_GLOBVARS_PARA);
    if (bFilter == CAN_NO_CHANNEL)
    {
    	return NULL;
    }
#  endif /* CONFIG_CAN_FULLCAN_SOFT_RTR */
/*---------------------------------------------------------------------*/

#  ifdef CONFIG_DRIVER_TEST
    PRINTF("pCOB = %p\n", pCOB);
#  endif /* CONFIG_DRIVER_TEST */

    return(pCOB);
} /* COB_T *Define_COB() */

/***************************************************************************/
/**
*++ \brief Set_COB_ID - assigns an identifier to a specific COB
*-- \brief Set_COB_ID - weist einen Identifier einem COB zu
*
*++ Change the COB-ID in software and in the CAN controller channel.
*++ Also a new COB Type will be set. If needed, a new channel will used.
*++ If the change is not possible, the old setting will used.
*
*-- Ändert die COB-ID in der Software und im CAN-Controllerkanal.
*-- Ändert sich der COB-Type, wird versucht einen passenden Kanal
*-- einzustellen.
*-- Bei einem Fehler wird versucht der alte Zustand vor dem Funktionsaufruf
*-- wieder hergestellt.
*
* \retval CO_E_NO_INITIATE
*++ pCOB is NULL, no memory available
*-- pCOB ist NULL, kein Speicher verfügbar
*
* \retval CO_E_TRANS_TYPE
*++ 29 Bit identifier are not supported
*-- 29 Bit identifier sind nicht unterstützt
*
* \retval CO_E_CAN_TRANS_ERROR
*++ could not restore old settings
*-- konnte alte Einstellungen nicht wiederherstellen
*
* \retval CO_OK
*-- Änderung erfolgreich durchgeführt
*++ no error
*/
RET_T Set_COB_ID(
		COB_T *pCOB,   			/**< pointer to COB in list */
		UNSIGNED32 cobId,      		/**< identifier */
		COB_KIND_T	cobType		/**< (new) COB-ID Type */
		CO_COMMA_GLOBVARS_PARA_DECL	/**< canLine (multiLine) */
		)
{
#  ifdef CONFIG_CAN_FULLCAN_SOFT_RTR
UNSIGNED8 bFilter;	/* COB-IDs filter number */
COB_T 	oldCobT; /* Backup */
#  endif /* CONFIG_CAN_FULLCAN_SOFT_RTR */
#  if defined(CONFIG_MULT_LINES) || defined(CONFIG_REDUNDANCY_SUPPORT)
UNSIGNED8	canLine;
#  endif /* (CONFIG_MULT_LINES) || (CONFIG_REDUNDANCY_SUPPORT) */
#  if defined(CONFIG_DRIVER_TEST) && defined(CONFIG_NO_GLOBAL_VARS)
/* for better debugging */
VOLATILE DRIVER_DATA_T * pDriverData;
#  endif /* (CONFIG_DRIVER_TEST) && (CONFIG_NO_GLOBAL_VARS) */

    if(pCOB == NULL)
    {
        return CO_E_NO_INITIATE;
    }

#  ifdef CONFIG_STANDARD_IDENTIFIER
    /* 29 bit IDs not allowed, yet */
    if ((cobId & CAN_29_BIT_ID_FLAG) != 0)
    {
    	return(CO_E_TRANS_TYPE);
    }
    if((cobId & CAN_29_BIT_ID_MASK) > 0x7FF)
    {
    	return(CO_E_VALUE_TO_HIGH);
    }
#  else /* CONFIG_STANDARD_IDENTIFIER */

    if(((cobId & CAN_29_BIT_ID_FLAG) == 0)
    		&& ((cobId & CAN_29_BIT_ID_MASK) > 0x7FF))
    {
    	return(CO_E_VALUE_TO_HIGH);
    }
#  endif /* CONFIG_STANDARD_IDENTIFIER */

#  if defined(CONFIG_MULT_LINES) || defined(CONFIG_REDUNDANCY_SUPPORT)
    canLine = pCOB->canLine;
#  endif /* (CONFIG_MULT_LINES) || (CONFIG_REDUNDANCY_SUPPORT) */

#  ifdef CONFIG_CAN_FULLCAN_SOFT_RTR
    /* create a backup */
    CO_MEMCPY(&oldCobT, pCOB, sizeof(COB_T));
#  endif /* CONFIG_CAN_FULLCAN_SOFT_RTR */

    /* save the new Cob ID and new COB_KIND_T */
    pCOB->cobId = (COB_IDENT_T)cobId;
    pCOB->eType = cobType;

/*---------------------------------------------------------------------*/
/*  hardware changes */
/*---------------------------------------------------------------------*/
#  ifdef CONFIG_CAN_FULLCAN_SOFT_RTR
    bFilter = initChannel(pCOB CO_COMMA_GLOBVARS_PARA);
    if ((bFilter == CAN_NO_CHANNEL) &&
    		((cobType & CO_COB_DISABLED) == 0))
    {
    	/* restore settings */
    	CO_MEMCPY(pCOB, &oldCobT, sizeof(COB_T));
    	bFilter = initChannel(pCOB CO_COMMA_GLOBVARS_PARA);
    	if (bFilter == CAN_NO_CHANNEL)
    	{
    		/* Error, also the old settings are not possible */
    		/* schwerer Fehler, alte settings konnten nicht
    		 * wieder hergestellt werden.
    		 * Sollte nie auftreten.
    		 */
    		return CO_E_CAN_TRANS_ERROR;
    	} /* if */

    	return CO_E_CAN_TRANS_TYPE;
    }
#  endif /* CONFIG_CAN_FULLCAN_SOFT_RTR */
/*---------------------------------------------------------------------*/
/* hardware changes ends */
/*---------------------------------------------------------------------*/

#  ifdef CONFIG_SYNC_CONSUMER
    /* in BasicCAN mode we check later the received MessageID */
    if( pCOB->eType == CO_COB_SYNC_CONS )
    {
    	GL_DRV_ARRAY(coSyncId) = (COB_IDENT_T)cobId;
    }
#  endif /* CONFIG_SYNC_CONSUMER */

#  ifdef CONFIG_DRIVER_TEST
#    if defined(CONFIG_NO_GLOBAL_VARS)
/* for better debugging */
    pDriverData = (DRIVER_DATA_T*)(GL_VAR(canDrvPtr) CO_REDCY_PARA_ARRAY_INDEX);
#    endif /* defined(CONFIG_NO_GLOBAL_VARS) */
#    if defined(CONFIG_MULT_LINES) || defined(CONFIG_REDUNDANCY_SUPPORT)
    PRINTF("L %d: ", (int)canLine);
#    endif /* CONFIG_MULT_LINES || CONFIG_REDUNDANCY_SUPPORT */
    PRINTF("Set_COB_ID(), pCOB %p, Channel=%d, Type=0x%x, id=%d/%x\n",\
    		pCOB,\
	       (int)0,\
	       (int)pCOB->eType,\
	       (int)cobId,\
	       (int)cobId);
#  endif /* CONFIG_DRIVER_TEST */

#  ifdef CONFIG_DRIVER_FAST_SORT
	/* update index list only in the active state */
    if (GL_DRV_ARRAY(fStartCan) == CO_TRUE)
    {
    	createCobIdIndex(CO_REDCY_PARA);
    }
#  endif /* CONFIG_DRIVER_FAST_SORT */

    return CO_OK;
} /* RET_T Set_COB_ID() */

#ifdef CONFIG_MULT_LINES
/*************************************************************************/
/**
* ++\brief CAN_GetCanLine - get canLine from a given can controller
* --\brief CAN_GetCanLine - gibt die canLine zurueck von einem gegebenen Can Controller
*
* \return
*++	channel number
*--	kanal nummer
*/
static UNSIGNED8 CAN_GetCanLine(FDCAN_HandleTypeDef *tmp_hfdcan)
{
static volatile UNSIGNED8 tempCanLine = 0; /* default */

	if(hfdcan[0].Instance == tmp_hfdcan->Instance) {
		tempCanLine = 0;	
	}
    else if(hfdcan[1].Instance == tmp_hfdcan->Instance) {
		tempCanLine = 1;	
	}
#  ifdef CONFIG_CAN_CONTROLLER_NUMBER_LINE2
	else if(hfdcan[2].Instance == hfdcan->Instance) {
		tempCanLine = 2;	
	}
#  endif

	return(tempCanLine);	
}
#endif /* CONFIG_MULT_LINES */

/**
  * @brief  Rx FIFO 0 callback.
  * @param  hfdcan pointer to an FDCAN_HandleTypeDef structure that contains
  *         the configuration information for the specified FDCAN.
  * @param  RxFifo0ITs indicates which Rx FIFO 0 interrupts are signaled.
  *         This parameter can be any combination of @arg FDCAN_Rx_Fifo0_Interrupts.
  * @retval None
  */
void HAL_FDCAN_RxFifo0Callback(FDCAN_HandleTypeDef *hfdcan, uint32_t RxFifo0ITs)
{
BUFFER_ENTRY_PTR_T pBuffer; /* Pointer to Message Buffer */
COB_IDENT_T cobId; /* local buffer of COB-ID */
UNSIGNED8 bLength; /* local buffer of data length */
BOOL_T	  extId = CO_FALSE;
FDCAN_ProtocolStatusTypeDef ProtocolStatus;
#ifdef CONFIG_MULT_LINES
UNSIGNED8 canLine;

	canLine = CAN_GetCanLine(hfdcan);
    
#endif /* CONFIG_MULT_LINES */  
    
    if((RxFifo0ITs & FDCAN_IT_RX_FIFO0_NEW_MESSAGE) != 0) {
        /* Retrieve Rx messages from RX FIFO0 */
        if (HAL_FDCAN_GetRxMessage(hfdcan, FDCAN_RX_FIFO0, &RxHeader, RxData) != HAL_OK) {
           /* error in get message */ 
            return;
        }
        else {
            /* read a message from the CAN layer */
            cobId = (COB_IDENT_T)RxHeader.Identifier;
            bLength = (UNSIGNED8)(RxHeader.DataLength >> 16);
            
            if(RxHeader.IdType == FDCAN_EXTENDED_ID) {
                /* ext. ID */
				extId = CO_TRUE;
#  ifndef CONFIG_STANDARD_IDENTIFIER
				cobId = (cobId) & CAN_29_BIT_ID_MASK;
				cobId |= CAN_29_BIT_ID_FLAG;
#  endif /* CONFIG_STANDARD_IDENTIFIER */   
            }
            if(RxHeader.RxFrameType == FDCAN_REMOTE_FRAME) {
                bLength |= CO_RTR_REQ; /* rtr request received */
            }
#  ifdef CONFIG_STANDARD_IDENTIFIER
			if (extId == CO_FALSE)	/* only standard identifier used ignore 29bit Ids */
#  endif /* CONFIG_STANDARD_IDENTIFIER */
			{
                extId = extId; /* reduce compiler warning */
            
#    if defined(CONFIG_SYNC_CONSUMER) && !defined(CONFIG_REDUNDANCY_SUPPORT)
                /* With Redundancy the information about the 'Active line' is needed
                 * -> check in FlushMbox() !!!
                 */

                if (cobId == GL_DRV_ARRAY(coSyncId))
                {
                    /* read Sync Counter */
                    GL_ARRAY(co_syncCnt) = 0;
                    /*
                     * read Message length
                     * if Messagelength == 1
                     * co_syncCnt = Data[0]
                     */
                    if(bLength != 0)
                    {
                        GL_ARRAY(co_syncCnt) = RxData[0];
                    }

                    /* SYNC message received */
                    SET_COLIB_FLAG(COFLAG_SYNC_RECEIVED);
                }
                else
#    endif /* CONFIG_SYNC_CONSUMER && !CONFIG_REDUNDANCY_SUPPORT */
                {
                    /*
                     * write it in the buffer
                     * cobId, bLength, data
                     */

                    BUFFER_INIT_PTR(RX, Write);
                    CHECK_BUFFER_WRITE(RX, CANFLAG_RXBUFFER_OVERFLOW)
                    {
                        /* reset error flags */
                        GL_DRV_ARRAY(coCanDriverState) &=
                                (UNSIGNED8)~CANFLAG_RXBUFFER_OVERFLOW;

                        BUFFER_WRITE(RX, cobId, cobId);
                        BUFFER_WRITE(RX, bLength, bLength);

                        if((bLength & CO_RTR_REQ) == 0) {
                
                            BUFFER_WRITE(RX, pData[0], RxData[0]);
                            BUFFER_WRITE(RX, pData[1], RxData[1]);
                            BUFFER_WRITE(RX, pData[2], RxData[2]);
                            BUFFER_WRITE(RX, pData[3], RxData[3]);
                            BUFFER_WRITE(RX, pData[4], RxData[4]);
                            BUFFER_WRITE(RX, pData[5], RxData[5]);
                            BUFFER_WRITE(RX, pData[6], RxData[6]);
                            BUFFER_WRITE(RX, pData[7], RxData[7]);
                        }

                        BUFFER_ENTRY_INCR(RX, Write, FULL);
                    } /* check buffer */

                } /* other message */
            }
        }
    }

#  ifdef CONFIG_CAN_ERROR_HANDLING
    /*
     * check for receiver hardware overflow
     */

    /* CAN Overflow ?  - only signal to the user - no local save */
    if((RxFifo0ITs & FDCAN_IT_RX_FIFO0_MESSAGE_LOST) != 0) {
        setNewDriverState(CANFLAG_OVERFLOW CO_COMMA_REDCY_PARA);   
    }

    /* Error handling - the controller can transmit or receive
     *    if the former state was INIT or BUSOFF,
     *    now the controller is in ACTIVE
     *
     * Fehlerhandling - wenn ich senden oder empfangen kann und vorher
     * im Busoff oder Init war, dann bin ich nun im Error active
     *
     * The next part is not needed, if an Error Active Interrupt
     * is calling.
     */
    if ((GL_DRV_ARRAY(coCanDriverState) \
		    & (CANFLAG_INIT | CANFLAG_BUSOFF)) != 0)
    {
    	setNewDriverState(CANFLAG_ACTIVE CO_COMMA_REDCY_PARA);
    }
    else if ((GL_DRV_ARRAY(coCanDriverState) & CANFLAG_PASSIVE) != 0)
    {
    	HAL_FDCAN_GetProtocolStatus(hfdcan, &ProtocolStatus);
        if( ProtocolStatus.ErrorPassive != 1) {
    		setNewDriverState(CANFLAG_ACTIVE CO_COMMA_REDCY_PARA);
    	}
    }
#  endif /* CONFIG_CAN_ERROR_HANDLING */
}

/**
  * @brief  Transmission Complete callback.
  * @param  hfdcan pointer to an FDCAN_HandleTypeDef structure that contains
  *         the configuration information for the specified FDCAN.
  * @param  BufferIndexes Indexes of the transmitted buffers.
  *         This parameter can be any combination of @arg FDCAN_Tx_location.
  * @retval None
  */
void HAL_FDCAN_TxBufferCompleteCallback(FDCAN_HandleTypeDef *hfdcan, uint32_t BufferIndexes)
{
FDCAN_ProtocolStatusTypeDef ProtocolStatus;
#ifdef CONFIG_MULT_LINES
UNSIGNED8 canLine;

	canLine = CAN_GetCanLine(hfdcan);
    
#endif /* CONFIG_MULT_LINES */    
    
# ifdef CONFIG_CAN_SENDING_FLAG
	GL_DRV_ARRAY(eSending) = CO_FALSE;
# endif /* CONFIG_CAN_SENDING_FLAG */
    
/* --------------------------------------------------------------------------*/
    /* Error handling - the controller can transmit or receive
     *    if the former state was INIT or BUSOFF,
     *    now the controller is in ACTIVE
     *
     * Fehlerhandling - wenn ich senden oder empfangen kann und vorher
     * im Busoff oder Init war, dann bin ich nun im Error active
     *
     * The next part is not needed, if an Error Active Interrupt
     * is calling.
     */

#  ifdef CONFIG_CAN_ERROR_HANDLING
    if ((GL_DRV_ARRAY(coCanDriverState) \
		    & (CANFLAG_INIT | CANFLAG_BUSOFF)) != 0)
    {
    	/* busoff -> active */
    	setNewDriverState(CANFLAG_ACTIVE CO_COMMA_REDCY_PARA);
    }
    else if ((GL_DRV_ARRAY(coCanDriverState) & CANFLAG_PASSIVE) != 0)
    {
    	HAL_FDCAN_GetProtocolStatus(hfdcan, &ProtocolStatus);
        if( ProtocolStatus.ErrorPassive != 1) {
    		/* passive -> active */
    		setNewDriverState(CANFLAG_ACTIVE CO_COMMA_REDCY_PARA);
    	}
    }
#  endif /* CONFIG_CAN_ERROR_HANDLING */
    
	/* get next transmission request */
	GetNext_TX_Request(CO_REDCY_PARA);
}

/**
  * @brief  Error status callback.
  * @param  hfdcan pointer to an FDCAN_HandleTypeDef structure that contains
  *         the configuration information for the specified FDCAN.
  * @param  ErrorStatusITs indicates which Error Status interrupts are signaled.
  *         This parameter can be any combination of @arg FDCAN_Error_Status_Interrupts.
  * @retval None
  */
void HAL_FDCAN_ErrorStatusCallback(FDCAN_HandleTypeDef *hfdcan, uint32_t ErrorStatusITs)
{
#ifdef CONFIG_MULT_LINES
UNSIGNED8 canLine;

	canLine = CAN_GetCanLine(hfdcan);
    
#endif /* CONFIG_MULT_LINES */  
    
    if(ErrorStatusITs == FDCAN_IT_BUS_OFF) {
        /* Busoff */
        setNewDriverState(CANFLAG_BUSOFF CO_COMMA_REDCY_PARA);
    }
    else if(ErrorStatusITs == FDCAN_IT_ERROR_PASSIVE) {
        /* Error Passive */
    	setNewDriverState(CANFLAG_PASSIVE CO_COMMA_REDCY_PARA);
    }
    else {
        /* Error Active */
    	setNewDriverState(CANFLAG_ACTIVE CO_COMMA_REDCY_PARA);
    }
}

/*******************************************************************/
/**
*
*++ \brief GetNext_TX_Request - transmits message from the transmission queue
*-- \brief GetNext_TX_Request - sendet Nachricht aus der Sendequeue
*
* \attention
*++ Do NOT call from user code !
*-- Nicht vom Anwenderprogramm aufrufen !
*
*++ The function is called within the CAN ISR and directly in
*-- Die Funktion wird innerhalb der CAN ISR aufgerufen und direkt in
* Transmit_COB();
*++ A Full CAN controller in Basic CAN mode
*++ uses object CAN_TRANSMIT_OBJ to Transmit a COB.
*++ In Full CAN mode the object specified in bChannel
*++ of the COB structure is used.
*-- Ein Full-CAN-Controller im Basic-CAN-Mode
*-- nutzt das Objekt CAN_TRANSMIT_OBJ um ein COB zu versenden.
*-- Im Full-CAN-Mode wird das durch bChannel innerhalb
*-- der COB-Struktur spezifizierte Objekt benutzt.
*
* \returns
*++ nothing
*-- nichts
*
*/

static void GetNext_TX_Request(
	CO_REDCY_PARA_DECL
     )
{
BUFFER_ENTRY_PTR_T pBuffer; /* Pointer to Message Buffer */
UNSIGNED8 bLength; /* local buffer of data length */
COB_KIND_T	eType;

# ifdef CONFIG_CAN_SENDING_FLAG
    if(GL_DRV_ARRAY(eSending) == CO_TRUE) {
	/* CAN controller is sending */
    	return;
    }
# endif /* CONFIG_CAN_SENDING_FLAG */

    BUFFER_INIT_PTR(TX, Read);
    CHECK_BUFFER_READ( TX )
    {/* we have a message to send */    
        /* check if fifo buffer from HAL layer full */
        if(HAL_FDCAN_GetTxFifoFreeLevel(&GL_DRV_ARRAY(hfdcan)) <= 0)
        {
            /* CAN controller channel/fifo busy */
            return;
        }

        bLength = BUFFER_READ( TX, bLength );
        eType = BUFFER_READ(TX, eType);

        /* Write transmit identifier. */
        TxHeader.Identifier = BUFFER_READ( TX , cobId); 
        TxHeader.Identifier &= CAN_29_BIT_ID_MASK;
# ifdef CONFIG_STANDARD_IDENTIFIER
        TxHeader.IdType = FDCAN_STANDARD_ID;			
# else /* CONFIG_STANDARD_IDENTIFIER */			
		if( (BUFFER_READ( TX , cobId) & CAN_29_BIT_ID_FLAG) != 0)
		{/* set IDE bit */
			TxHeader.IdType = FDCAN_EXTENDED_ID;
		}
        else 
        {
            TxHeader.IdType = FDCAN_STANDARD_ID;
        }
# endif /* CONFIG_STANDARD_IDENTIFIER */


        /* Write transmit data length */
        TxHeader.DataLength = (BytestoDLC[bLength] << 16);
        
        if ((eType & CO_COB_DIR_MASK) == CO_COB_TX)
        {  /* data frame */
            TxHeader.TxFrameType = FDCAN_DATA_FRAME;
            /* Set the data to be tranmitted */
            TxData[0] = BUFFER_READ( TX , pData[0]);
            TxData[1] = BUFFER_READ( TX , pData[1]);
            TxData[2] = BUFFER_READ( TX , pData[2]);
            TxData[3] = BUFFER_READ( TX , pData[3]);
            TxData[4] = BUFFER_READ( TX , pData[4]);
            TxData[5] = BUFFER_READ( TX , pData[5]);
            TxData[6] = BUFFER_READ( TX , pData[6]);
            TxData[7] = BUFFER_READ( TX , pData[7]);
        }

        if ( (eType & CO_COB_DIR_RTR_MASK) == CO_COB_RX_RTR )
        {   /* Transmit RTR */
            TxHeader.TxFrameType = FDCAN_REMOTE_FRAME;
        }

# ifdef CONFIG_CAN_SENDING_FLAG
	GL_DRV_ARRAY(eSending) = CO_TRUE;
# endif /* CONFIG_CAN_SENDING_FLAG */

        /* Add message to TX FIFO of FDCAN instance */
        if (HAL_FDCAN_AddMessageToTxFifoQ(&GL_DRV_ARRAY(hfdcan), &TxHeader, TxData) != HAL_OK)
        { /* error in add message to FIFO queue! */ 
           return;
        }

        /* release buffer */
        BUFFER_ENTRY_INCR( TX , Read , EMPTY);
    }
}

/*******************************************************************/
/**
*++ \brief Transmit_COB - transmits a COB
*-- \brief Transmit_COB - sendet ein COB
*
* \em Transmit_COB()
*-- sendet eine Nachricht mit den Eigenschaften aus \em pCOB
*-- und den Daten \em pMsg.
*++ Transmits a CAN message with the attribute of \em pCOB
*++ and the data of \em pMsg.
*
* \retval CO_OK
*++ no error
*-- kein Fehler
*
*/

RET_T Transmit_COB(
     COB_T     *pCOB,   /**< pointer to COB in list */
     UNSIGNED8 *pMsg    /**< pointer to data or NULL */
     CO_COMMA_GLOBVARS_PARA_DECL
     )
{
RET_T retval;

# if defined(CONFIG_MULT_LINES) || defined(CONFIG_REDUNDANCY_SUPPORT)
UNSIGNED8	canLine;
# endif /* CONFIG_MULT_LINES */

# if defined(CONFIG_MULT_LINES) || defined(CONFIG_REDUNDANCY_SUPPORT)
    canLine = pCOB->canLine;
# endif

# ifdef CONFIG_DRIVER_TEST
#  if defined(CONFIG_MULT_LINES) || defined(CONFIG_REDUNDANCY_SUPPORT)
    PRINTF("Line %d ", (int)canLine);
#  endif
    PRINTF("Transmit_COB: Id: 0x%02X\n", (int)(pCOB->cobId));
# endif /* CONFIG_DRIVER_TEST */

    /* insert message to queue */
    retval = Insert_TX_Request(pCOB, pMsg CO_COMMA_GLOBVARS_PARA);

    /* get next transmission request from queue
     * in this time the TX IRQ is not allowed */
    DISABLE_CAN_INTERRUPTS(CO_REDCY_PARA);
    GetNext_TX_Request(CO_REDCY_PARA);
    RESTORE_CAN_INTERRUPTS(CO_REDCY_PARA);

    return retval;
}


/*******************************************************************/
/**
*++ \brief getCanDriverState - returns the current driver state
*-- \brief getCanDriverState - liefert den aktuellen Status der CAN Treibers
*   \ingroup CanDriverFlags
*
*++ The driver state is returned bit endcoded.
*++ For testing the driver state use the compiler defines
*++ \c CANFLAG_INIT , \c CANFLAG_BUSOFF , \c CANFLAG_PASSIVE,
*++ \c CANFLAG_ACTIVE .
*++ A change in the driver state should only be reported once.
*++ This can be achieved by keeping the old state.
*
*-- Der Treiberstatus wird bitcodiert zurückgegeben.
*-- Für die Prüfung des Treiberstatus sind die Compiler defines
*-- \c CANFLAG_INIT, \c CANFLAG_BUSOFF, \c CANFLAG_PASSIVE,
*-- \c CANFLAG_ACTIVE
*-- vorgesehen.
*-- Ein Treiberstatuswechsel sollte nur einmal signalisiert werden.
*-- Dazu wird der alte Treiberstatus gerettet.
*
*++ In the simplest case only return coCanDriverState.
*-- Im einfachsten Fall muß nur die Variable coCanDriverState zurückgegeben
*-- werden.
*
* \returns
*++ CAN controller and CAN driver state
*-- CAN Controller und CAN Treiber Zustand
*/

UNSIGNED8 getCanDriverState(
	CO_REDCY_PARA_DECL
	)
{
    return GL_DRV_ARRAY(coCanDriverState);
}

/*******************************************************************/
/*
* setNewDriverState - set a new CAN driver state
*
* If the state was changed, the library will be informed.
* In some implementations this function is also called from the CAN IRQ.
*
*/
static void setNewDriverState(
        UNSIGNED8 newState
        CO_COMMA_REDCY_PARA_DECL
)
{
REGISTER UNSIGNED8 tmpState;

    tmpState = GL_DRV_ARRAY(coCanDriverState);

    if ((newState & CANFLAG_STATE_MASK) != 0) {
        /* switch to a different CAN bus state */
        tmpState &= (UNSIGNED8)~CANFLAG_STATE_MASK;
    }
    tmpState |= newState;

    if(GL_DRV_ARRAY(coCanDriverState) != tmpState) {
        /* only trigger changes  - CAN Overrun will not saved */
        GL_DRV_ARRAY(coCanDriverState) =
        		tmpState & (UNSIGNED8)~CANFLAG_OVERFLOW;
        SET_CAN_FLAG(newState);
        /* signal new state */
        SET_COLIB_FLAG(COFLAG_CAN_EVENT);
    }
}

#  ifdef CONFIG_CAN_FULLCAN_SOFT_RTR
/*******************************************************************/
/**
* \brief initChannel - init hardware
*
*++ internal functions to manipulate the hardware channels
*++ after leave this function the channel is only valid
*++ if all COB_T parameters are known
*
* \param pCOB->bChannel
*++	old channel number
*++	0xFF means - no old channel
*
* \param pCOB->eType
*++	COB type
*++	Note: Bit CO_COB_DISABLED  could be set
*
* \param pCOB->wID COB ID
*++	0xFFFFFFFF means - no ID
*
* \returns
*++	new channel number
*++	0xFF means - no free channel
*
*/

static UNSIGNED8 initChannel(
	COB_T * pCOB /* contains old channel */
	CO_COMMA_GLOBVARS_PARA_DECL
)
{
UNSIGNED8 bChannel;
CAN_ADDR_T pChannel; /* channel base pointer */
UNSIGNED8 fReInit;   /* 1 means, the hardware must initialize */
COB_KIND_T eType;

# ifdef CONFIG_MULT_LINES
UNSIGNED8 canLine;
# endif /* CONFIG_MULT_LINES */

    fReInit = 1;

    bChannel = pCOB->bChannel;
    eType = pCOB->eType;

# if defined(CONFIG_MULT_LINES) || defined(CONFIG_REDUNDANCY_SUPPORT)
    canLine = pCOB->canLine;
# endif /* CONFIG_MULT_LINES */

    /* Define_COB() doesn't reserve a channel (CAN_NO_CHANNEL).
     * Now we have to do it.
     */
    if (pCOB->bChannel == CAN_NO_CHANNEL) {
	/* channel number of current line */
	bChannel = GL_DRV_ARRAY(coSavedChannel);

#  ifdef CONFIG_CAN_ONLY_ONE_TRANSMIT_CHANNEL
	/* check for new usable channel */
	if( (eType & CO_COB_DIR_RTR_MASK) == CO_COB_TX )
	{
	    bChannel = CAN_TRANSMIT_OBJ;
	    /* predefined channel == no initialize (did in Init_Can()) */
	    fReInit = 0;
	}
    } else {
	/* if bChannel a predefined channel, we cannot change
	 * the configuration
	 *
	 * Move a object from a FullCAN channel to the
	 * predefined CAN_TRANSMIT_OBJ is not supported.
	 * For a support also this, we has to check
	 * free channels between busy channels.
	 */
	if ((bChannel == CAN_TRANSMIT_OBJ)
		&& ((eType & CO_COB_DISABLED) == 0))
	{
	    if( (eType &  CO_COB_DIR_RTR_MASK) == CO_COB_TX ) {
	    /* predefined channel == no initialize (did in Init_Can()) */
		fReInit = 0;
	    } else {
		/* eType change and this eType cannot used
		 * with the predefined channel */

		/* channel number of current line */
		bChannel = GL_DRV_ARRAY(coSavedChannel);
	    }
	}
#  endif /* CONFIG_CAN_ONLY_ONE_TRANSMIT_CHANNEL */
    }
/*---------------------------------------------------------------------*/
/*  	check bChannel a useable hardware channel */
/*---------------------------------------------------------------------*/
    if (fReInit == 1) {
    	/* do not check predefined objects */
	if (bChannel >= (CAN_LAST_OBJ + 1)) {
	    /* No more channels available */
#  ifdef CONFIG_DRIVER_TEST
	    PRINTF("No more Message Objects\n");
#  endif /* CONFIG_DRIVER_TEST */
	    return CAN_NO_CHANNEL;
	}
    }

/*---------------------------------------------------------------------*/
/*  	check end */
/*---------------------------------------------------------------------*/


/*---------------------------------------------------------------------*/
/*	hardware initialization */
/*---------------------------------------------------------------------*/

    if (fReInit == 1) {
    	/* Typically every call of this function reinitialize the
    	 * hardware. Only predefined channels will not reinitialize.
    	 */

	DISABLE_CAN_INTERRUPTS(CO_REDCY_PARA);

	CAN_INIT_OBJ_PTR(bChannel);

	/* use same functionality for not defined ID like object disabled */
	if(pCOB->cobId == CAN_NO_COBID) {
	    eType |= CO_COB_DISABLED;
	}

	/* TODO:
	 * set channel invalid for reconfiguration */


	/* don't activate disabled channels */
	if((eType & CO_COB_DISABLED) != CO_COB_DISABLED)
	{
	    /* TODO:
	     * set interrupt enable for this channel
	     */

	    /* TODO:
	     * set Message ID
	     */

	    /* The Channel must be configured according to the Type of the COB
	     * The COB ID would be write later
	     */
	    if ((eType & CO_COB_DIR_MASK) == CO_COB_RX) {
		/* TODO:
		 * receive channel
		 */
	    } else {
		/* TODO:
		 * transmit channel
		 */
	    }

	    if ((eType & CO_COB_DIR_MASK) == CO_COB_RX) {
		/* TODO:
		 * set buffer ready
		 */
	    }
	}

	RESTORE_CAN_INTERRUPTS(CO_REDCY_PARA);
/*---------------------------------------------------------------------*/
/*  hardware initialization end */
/*---------------------------------------------------------------------*/
    } /* ReInit */


/*---------------------------------------------------------------------*/
/* save if a new used channel */
/*---------------------------------------------------------------------*/
    if( bChannel == GL_DRV_ARRAY(coSavedChannel) ) {
	GL_DRV_ARRAY(coSavedChannel) = bChannel + 1;
    }
    pCOB->bChannel = bChannel;

    return bChannel;
    /* in BasicCAN mode TX, RX and so on initialize in Init_CAN() */
    /* This function don't support BasicCAN changes at the moment */
    return CAN_NO_CHANNEL;
}
# endif /* CONFIG_CAN_FULLCAN_SOFT_RTR*/

/*******************************************************************/
/**
* releaseCan  - close driver
*
*/
void releaseCan(
        CO_REDCY_PARA_DECL
    )
{
    Stop_CAN(CO_REDCY_PARA);
    DISABLE_CAN_INTERRUPTS(CO_REDCY_PARA);
    
    HAL_FDCAN_DeInit(&GL_DRV_ARRAY(hfdcan));
    /* set internal driver state */
	GL_DRV_ARRAY(canInitialized) = CO_FALSE;
}

# ifdef CONFIG_REDUNDANCY_SUPPORT
/*******************************************************************/
/*
*++ \brief checkRedcyTimer - check redundancy timer events
*-- \brief checkRedcyTimer - tested Redundancy Timer Ereignisse
*
* This function checks all redundancy timer events
* Time out monitoring for actual transmitted message
* and checks max. delay time for PDOs
*
* \retval
*	REDCY_TRANS_STATE_OK		transmision ok
*	REDCY_TRANS_STATE_FAILED	current transmission timeout/failed
*	REDCY_TRANS_STATE_PDO_OK	PDO transmision ok
*	REDCY_TRANS_STATE_PDO_FAILED	PDO transmision failed
*
*/
UNSIGNED8 checkRedcyTimer(
	CO_REDCY_PARA_DECL
    )
{
UNSIGNED8	retVal;

#  if defined(CONFIG_DRIVER_TEST) && defined(CONFIG_NO_GLOBAL_VARS)
/* for better debugging */
VOLATILE DRIVER_DATA_T * pDriverData;
    pDriverData = (DRIVER_DATA_T*)(GL_VAR(canDrvPtr) CO_REDCY_PARA_ARRAY_INDEX);
#  endif

    /* default - no error and no pdo transmitted */
    retVal = REDCY_TRANS_STATE_OK;

    DISABLE_CAN_INTERRUPTS(CO_REDCY_PARA);

    /* one or more PDOs transmission successfull ? */
    if (GL_DRV_ARRAY(transmittedPdo) != 0)  {
	retVal = REDCY_TRANS_STATE_PDO_OK;
	GL_DRV_ARRAY(transmittedPdo) = 0;
    }

    /* current transmission timeout monitoring */
    if (GL_DRV_ARRAY(transmissionActive) == CO_TRUE)  {
	/* transmission is active - timeout ticks != 0 ? */
	if (GL_DRV_ARRAY(actTransTickCnt) != 0)  {
	    /* decrement counter */
	    GL_DRV_ARRAY(actTransTickCnt) --;
	    /* counter finished ? */
	    if (GL_DRV_ARRAY(actTransTickCnt) == 0)  {
		/* yes timeout, abort transmission */
		if (GL_DRV_ARRAY(actTransType) == CO_TRUE) {
		    retVal = REDCY_TRANS_STATE_PDO_FAILED;
		} else {
		    retVal = REDCY_TRANS_STATE_FAILED;
		}
	    }
	}
    }

    /* check for timeout in buffered messages */
    if (checkBufferTimeout(CO_REDCY_PARA) > 0) {
	retVal = REDCY_TRANS_STATE_PDO_FAILED;
    }

    RESTORE_CAN_INTERRUPTS(CO_REDCY_PARA);

#  ifdef CONFIG_DRIVER_TEST
    switch(retVal) {
    case REDCY_TRANS_STATE_OK: break;
    case REDCY_TRANS_STATE_PDO_OK:
    	PRINTF("L%d: REDCY_TRANS_STATE_PDO_OK\n", (int)canLine);
    	break;
    case REDCY_TRANS_STATE_PDO_FAILED:
    	PRINTF("L%d: REDCY_TRANS_STATE_PDO_FAILED\n", (int)canLine);
    	break;
    case REDCY_TRANS_STATE_FAILED:
    	PRINTF("L%d: REDCY_TRANS_STATE_FAILED\n", (int)canLine);
    	break;
    }
#  endif

    return retVal;
}
# endif /* CONFIG_REDUNDANCY_SUPPORT */




# ifdef CONFIG_EXPERIMENTAL
/*******************************************************************/
/*******************************************************************/
/*
* print_hex - debug function
*
*	Note:
*	When used to dump CAN controller register contents
*	reading these registers can change CAN Controller internals!
*
*
* \internal
*/
void print_hex(
	UNSIGNED8 * addr,
	UNSIGNED8 length
	)
{
UNSIGNED8 i;

    for (i = 0; i < length; i++) {
    	if( ((UNSIGNED8)addr % 8) == 0) {
	    PRINTF("\n %p ", addr);
    	}
    	PRINTF("0x%02x ", (unsigned int)*addr++);
    }
    PRINTF("\n");

}


/*******************************************************************/
/*
* print_eType - debug function
*
* \internal
*/
void print_eType(
      COB_KIND_T eType   /* COB type     */
)
{
/*
  COB_KIND_T:

  CO_COB_RX	    receive message
  CO_COB_RX_RTR receive message, which can be requested by the local device
  CO_COB_TX	transmit message
  CO_COB_TX_RTR	transmit message, which can be requested by remote devices

  That means that both CO_COB_TX and CO_COB_TX_RTR
  are data messages to transmit.
  That other can be of type CO_COB_RX_RTR,
  that means transmit a RTR message from local node.

  Additional the type of CAN object is inserted in ths type
*/
char s[30] = "";
char* ps = s;
        if ((eType & CO_COB_DISABLED) == CO_COB_DISABLED) {
            ps += sprintf(ps, "DISABLE ");
        }
        if ((eType & CO_COB_DIR_MASK) == CO_COB_RX) {
            ps += sprintf(ps, "RX ");
        }
        if ((eType & CO_COB_DIR_MASK) == CO_COB_TX) {
            ps += sprintf(ps, "TX ");
        }
        if ((eType & CO_COB_RTR) == CO_COB_RTR) {
            ps += sprintf(ps, "RTR ");
        }
        if (eType == CO_COB_GUARD_SLAVE) {
            ps += sprintf(ps, "NG ");
        }
        if (eType == CO_COB_HB_PROD) {
            ps += sprintf(ps, "HB_P ");
        }
        if (eType == CO_COB_HB_CONS) {
            ps += sprintf(ps, "HB_C ");
        }
        if (eType == CO_COB_SYNC_CONS) {
            ps += sprintf(ps, "SYNC ");
        }
        ps += sprintf(ps , "\n");

        PRINTF( s );
}


/*******************************************************************/
/*******************************************************************/
# endif /* CONFIG_EXPERIMENTAL */
#endif /* CONFIG_CAN_FAMILY_FDCAN */

/*----- end of source ---------------------------------------------*/
