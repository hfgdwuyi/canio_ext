/*
 * co_drv - declarations for public driver interface
 *
 * Copyright (c) 2001-2017 port GmbH Halle/Saale
 *------------------------------------------------------------------
 */

/**
*  \file co_drv.h
*++ Defines for the public driver interface 
*-- Definitionen für die öffentliche Treiber-API
*  \author port GmbH Halle (Saale)
*
*++ The file contains definitions of structures and complex data types
*++ for the public driver interface.
*-- Diese Datei enthält Definitionen von Strukturen und Datentypen
*-- für die öffentliche Treiber-API.
*/

#ifndef __CO_DRV_H
# define __CO_DRV_H

#include <co_stru.h>

/* if RTX51 not defined */
#ifndef CONFIG_OS_RTX51
# ifndef RTX51_MODIFIER
#  define RTX51_MODIFIER
# endif
#endif

/** CAN Controller is init/busoff   \ingroup CanDriverFlags */
#define CANFLAG_INIT			0x01u
/** CAN Controller is error active  \ingroup CanDriverFlags */
#define CANFLAG_ACTIVE			0x02u	
/** CAN Controller is busoff        \ingroup CanDriverFlags */
#define CANFLAG_BUSOFF			0x04u	
/** CAN Controller is error passive \ingroup CanDriverFlags */
#define CANFLAG_PASSIVE			0x08u	
#define CANFLAG_STATE_MASK		0x0Fu	

/** CAN Controller RX buffer hardware overrun  \ingroup CanDriverFlags */
#define CANFLAG_OVERFLOW		0x10u
/** TX software buffer overflow     \ingroup CanDriverFlags */
#define CANFLAG_TXBUFFER_OVERFLOW	0x20u
/** RX software buffer overflow     \ingroup CanDriverFlags */
#define CANFLAG_RXBUFFER_OVERFLOW	0x40u	

#define CANFLAG_ALL			0xFFu

/* returns for Init_CAN */

# define CO_INIT_CAN_OK			0
# define CO_E_INIT_HARD_RES_ACTIVE 	1
# define CO_E_INIT_CLK_FREQ		2
# define CO_E_INIT_BAUD			3
# define CO_E_INIT_PROP			4
# define CO_E_INIT_WRONG_ADDRESS        5
# define CO_E_INIT_UNSPEC_ERROR         255


/* table for CAN-Timings */
/* for every baudrate must exist a entry in the table 	*/
/* for 125kbit/s rate must be 125                       */
/* the last entry must be 0,0,0				*/
/* e.g. 	
 * const BTR_TAB_T can_btr_tab[] = { {rate1, 1_BTR0, 1_BTR1},
 *				     {rate2, 2_BTR0, 2_BTR1},
 *				     {    0,      0,      0} ***last Entry***
 *				   }
 */
typedef struct {
    UNSIGNED16 rate;	/**< bitrate */
    UNSIGNED8  btr0;	/**< value of bittiming register 0 */
    UNSIGNED8  btr1;	/**< value of bittiming register 1 */
} BTR_TAB_T;    


/* set defaults for CAN driver buffers */

# ifndef CONFIG_RX_BUFFER_SIZE       /* if not defined in cal_conf.h */
#  define CONFIG_RX_BUFFER_SIZE 10
# endif

# ifndef CONFIG_TX_BUFFER_SIZE       /* if not defined in cal_conf.h */
#  define CONFIG_TX_BUFFER_SIZE 10
# endif

# ifndef CONFIG_BIT_RATE_INDEX       /* is necessary for switching bitrate on devices, */
#  define CONFIG_BIT_RATE_INDEX 0    /* which have no non volatile memory */
# endif

# ifdef CONFIG_REDUNDANCY_SUPPORT
#  include <co_drvry.h>
# endif /* CONFIG_REDUNDANCY_SUPPORT */

# ifndef TEST_CAN_FLAG
#  ifdef CONFIG_NO_GLOBAL_VARS
#  else /* CONFIG_NO_GLOBAL_VARS */
extern UNSIGNED8		coCanFlags CO_REDCY_PARA_ARRAY_DEF;
#  endif /* CONFIG_NO_GLOBAL_VARS */
#  define SET_CAN_FLAG(FLAG)	(GL_ARRAY(coCanFlags) |= (UNSIGNED8)(FLAG))
#  define RESET_CAN_FLAG(FLAG)	(GL_ARRAY(coCanFlags) &= (UNSIGNED8)~(FLAG))
#  define TEST_CAN_FLAG(FLAG)	(GL_ARRAY(coCanFlags) & (UNSIGNED8)(FLAG))
# endif /* TEST_CAN_FLAG */


/* external variable declarations */

#ifdef CONFIG_NO_CONSTANT_TIMER
extern UNSIGNED16 coTimerPulse;
#else /* CONFIG_NO_CONSTANT_TIMER */
extern UNSIGNED16 CO_CONST coTimerPulse;
#endif /* CONFIG_NO_CONSTANT_TIMER */

extern CO_CONST UNSIGNED16	co_bittiming_table[];

# if defined(CONFIG_MULT_LINES) && defined(CONFIG_MULT_CANCONTROL_TYPE)
#  include <co_drvmc.h>
# endif /* defined(CONFIG_MULT_LINES) && defined(CONFIG_MULT_CANCONTROL_TYPE) */

/* include special defines for redundancy support */
# ifdef CONFIG_REDUNDANCY_SUPPORT
#  include <co_drvry.h>
# endif /* CONFIG_REDUNDANCY_SUPPORT */


/* are the macros not defined use the standard functions */
# ifndef CLEAR_BUSOFF
#   define CLEAR_BUSOFF		Clear_busoff
# endif /* CLEAR_BUSOFF */

# ifndef FLUSH_MBOX
#   define FLUSH_MBOX		FlushMbox
# endif /* FLUSH_MBOX */

# ifndef START_CAN
#   define START_CAN		Start_CAN
# endif /* START_CAN */

# ifndef STOP_CAN
#   define STOP_CAN		Stop_CAN
# endif /* STOP_CAN */

# ifndef INIT_CAN
#  define INIT_CAN		initCan
# endif /* INIT_CAN */

# ifndef SET_BAUDRATE
#  define SET_BAUDRATE		Set_Baudrate
# endif /* SET_BAUDRATE */

# ifndef CLEAR_BUSOFF
#  define CLEAR_BUSOFF		Clear_busoff
# endif /* CLEAR_BUSOFF */

# ifndef SETINTMASK
#  define SETINTMASK		SetIntMask
# endif /* SETINTMASK */

# ifndef RESETINTMASK
#  define RESETINTMASK		ResetIntMask
# endif /* RESETINTMASK */




/* function prototypes */

void		FlushMbox(CO_REDCY_PARA_DECL);
UNSIGNED8	initTimer(CO_GLOBVARS_PARA_DECL);
void		releaseTimer(CO_GLOBVARS_PARA_DECL);
void		clearTxBuffer(CO_REDCY_PARA_DECL);
void		clearRxBuffer(CO_REDCY_PARA_DECL);
BOOL_T		checkTxBuffer(CO_REDCY_PARA_DECL);
UNSIGNED8	iniDevice(void);
UNSIGNED8	initCan(UNSIGNED16 CO_COMMA_REDCY_PARA_DECL);
UNSIGNED8	Set_Baudrate(UNSIGNED16, BTR_TAB_T * CO_COMMA_REDCY_PARA_DECL);
void		Start_CAN(CO_REDCY_PARA_DECL);
void		Stop_CAN(CO_REDCY_PARA_DECL);
void		Clear_busoff(CO_REDCY_PARA_DECL);
void		SetIntMask(CO_GLOBVARS_PARA_DECL);
void		ResetIntMask(CO_GLOBVARS_PARA_DECL);
UNSIGNED8	getCanDriverState(CO_REDCY_PARA_DECL);
void		releaseCan(CO_REDCY_PARA_DECL);

#endif /* __CO_DRV_H */

/* end of source */

