/*
 * can_stm32_fdcan - definitions for a FDCAN controller
 *
 * Copyright (c) 2020 port GmbH Halle/Saale
 *------------------------------------------------------------------
 */

/**
*  \file can_stm32_fdcan.h
*++ Definitions for a CAN controller
*-- Definitionen für den CAN Controller
*  \author port GmbH Halle (Saale)
*
*/

#ifndef CAN_STM32_FDCAN_H
#define CAN_STM32_FDCAN_H 1

/* typdefinitions 
----------------------------------------------------------------------*/

#ifdef CONFIG_CAN_FAMILY_FDCAN

/*---------------------------------------------------------------------*/
/* additional use Macros within the driver                             */
/** 
*++ %COB_T : no hardware channel refered 
*-- %COB_T : kein Hardware Kanal zugewiesen
*/
#define CAN_NO_CHANNEL 		0xFF
/** 
*++ %COB_T : no COB ID refered 
*-- %COB_T : keine COB-ID zugewiesen
*/
#ifdef CONFIG_STANDARD_IDENTIFIER
#  define CAN_NO_COBID 		((COB_IDENT_T)0xFFFFu)
#else
#  define CAN_NO_COBID 		((COB_IDENT_T)0xFFFFFFFFul)
#endif
/*---------------------------------------------------------------------*/


/***** B I T  --  T I M I N G  ******************************************/
/*
 Bit Timing Register fuer 500 kBit/s   -- 2µs Bit time
 |       3       |       4       ||       0       |       1       |
 | x | 0 | 1 | 1 | 0 | 1 | 0 | 0 || 0 | 0 | 0 | 0 | 0 | 0 | 0 | 1 |
 | x |   TSEG2   |     TSEG1     ||  SJW  |          BRP          |
   |          BTR1               ||            BTR0
   |          CAN_BitTiming1Reg  ||            CAN_BitTiming0Reg 
   |
   +--- DIV8X
        if 1 : baudrate prescaler is driven by fcan/8

 1. possible :
 CLP = Clock Period ( = 0.1 us at 10 MHz)    e.g. 82527: CLP == SCLK
 1 Tq (Time quantum) = (BRP + 1) * CLP  
 BRP = Baud Rate Prescaler = 1 (=> Tq = 0.2 us)
 SJW = Sync Jump Width = 1 (= SJW + 1 = 0 + 1)
 TSEG1 = Time Segment before Sampling Point = 4
 TSEG2 = Time Segment after Sampling Point = 3
 Conclusion:
 number of bit time segments (must be  10 )
    n = 1 + TSEG1+1 + TSEG2+1 = 10
   --> Sampling Point bei 60 %
 baud rate = 10 MHz / ( n * (BRP + 1)) = 500000 Bit/s
   --> deviation 0.0 %

 2. CANopen DS301 recommends the sampling point at 1.75 µs:
 CLP = Clock Period ( = 0.1 us at 10 MHz)    e.g. 82527: CLP == SCLK
 1 Tq (Time quantum) = (BRP + 1) * CLP
 BRP = Baud Rate Prescaler = 0 (=> Tq = 0.1 us)
 SJW = Sync Jump Width = 1 (= SJW + 1 = 0 + 1)
 TSEG1 = Time Segment before Sampling Point = 15
 TSEG2 = Time Segment after Sampling Point = 2
 Conclusion:
 number of bit time segments (must be  20 )
    n = 1 + TSEG1+1 + TSEG2+1 = 20
   --> Sampling Point bei 1.7 µs == 85 %
 baud rate = 10 MHz / ( n * (BRP + 1)) = 500000 Bit/s
   --> deviation 0.0 %

*/

/* Bittiming Table
---------------------------------------------------------------------------*/
typedef struct {
        UNSIGNED16 rate;
        UNSIGNED16 presc;
        UNSIGNED16 btr0;
        UNSIGNED16 btr1;	
} BTR_TAB_FDCAN_T;

/**
* \def CONFIG_CAN_T_CLK
*++ input clock of the CAN controller
*++ servers the selection of the bittiming table defintions
*-- Taktfrequenz des CAN Controllers
*-- dient der Auswahl der Bittimingtabelle
*/

# ifdef CAN_BTR0_10K
  /* the Userdefines are used */
# else /* CAN_BTR0_10K */

#  ifndef CONFIG_CAN_T_CLK
#    error "Please specify an CAN_T_CLK value"
#  endif
#  ifdef DOXYGEN
	/* only set for generation of the documentation */
#    define CONFIG_CAN_T_CLK 20
#  endif

#  if CONFIG_CAN_T_CLK == 80
#   define CAN_SJW            0

#   define CAN_PRSC_10K          500
#   define CAN_BTR0_10K           13 
#   define CAN_BTR1_10K            2 
#   define CAN_PRSC_20K          250
#   define CAN_BTR0_20K           13
#   define CAN_BTR1_20K            2
#   define CAN_PRSC_50K          100
#   define CAN_BTR0_50K           13
#   define CAN_BTR1_50K            2
#   define CAN_PRSC_100K          50
#   define CAN_BTR0_100K          13
#   define CAN_BTR1_100K           2
#   define CAN_PRSC_125K          40
#   define CAN_BTR0_125K          13
#   define CAN_BTR1_125K           2
#   define CAN_PRSC_250K          20
#   define CAN_BTR0_250K          13
#   define CAN_BTR1_250K           2
#   define CAN_PRSC_500K          10
#   define CAN_BTR0_500K          13
#   define CAN_BTR1_500K           2
#   define CAN_PRSC_800K           5
#   define CAN_BTR0_800K          16
#   define CAN_BTR1_800K           3
#   define CAN_PRSC_1000K          5
#   define CAN_BTR0_1000K         13
#   define CAN_BTR1_1000K          2

#  else
#   error "Please specify an CONFIG_CAN_T_CLK value of 80!"
#   error "Or create new entries using "
#   error "http://www.port.de/canbittiming"
#  endif /* CONFIG_CAN_T_CLK == 20 */
# endif /* CAN_BTR0_10K */

/* prototypes
----------------------------------------------------------------------*/
UNSIGNED8 Init_CAN(UNSIGNED8 module, UNSIGNED16 CO_COMMA_LINE_PARA_DECL);
# endif /* CONFIG_CAN_FAMILY_FDCAN */
#endif /* CAN_STM32_FDCAN_H */
