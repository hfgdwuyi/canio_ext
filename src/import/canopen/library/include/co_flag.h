/*
 * co_flag - defines for library flag usage
 *
 * Copyright (c) 2001-2017 port GmbH Halle/Saale
 *------------------------------------------------------------------
 */

/**
*  \file co_flag.h
*++ Defines of CANopen library flags
*-- Definitionen für CANopen library Flags
*  \author port GmbH Halle (Saale)
*
*++ The file contains definitions of structures and data types for
*++ flag usage.
*-- Diese Datei enthält Definitionen von Strukturen und Datentypen
*-- zur Verwendung von CANopen library Flags.
*/

#ifndef __CO_FLAG_H
# define __CO_FLAG_H

# include <co_def.h>		/* include canopen definition */
# ifdef CONFIG_REDUNDANCY_SUPPORT
#  include <co_redcy.h>
# endif /* CONFIG_REDUNDANCY_SUPPORT */


/* CANopen Library Flag Definition */
/* CAN flags are now defined at co_drv.h */
#ifndef TEST_COLIB_FLAG
extern UNSIGNED8	coLibFlags CO_LINE_PARA_ARRAY_DEF;
# define SET_COLIB_FLAG(FLAG)	(GL_ARRAY(coLibFlags) |= (UNSIGNED8)(FLAG))
# define RESET_COLIB_FLAG(FLAG) (GL_ARRAY(coLibFlags) &= (UNSIGNED8)~(FLAG))
# define TEST_COLIB_FLAG(FLAG)	(GL_ARRAY(coLibFlags) & (UNSIGNED8)(FLAG))
#endif

#define COFLAG_ALL		0xffu
#define COFLAG_SYNC_RECEIVED	0x01u	/* sync received */
#define COFLAG_TIMER_PULSED	0x02u	/* timer has pulsed */
#define COFLAG_SDO_BLOCKTRANS	0x04u	/* outstanding block transfers */
#define COFLAG_SDO_MANAGER	0x08u	/* outstanding sdo manager transfers */
#define COFLAG_CAN_EVENT	0x10u	/* CAN event */
#define COFLAG_NMT_STARTUP_MANAGER	0x20u	/* bootup manager active */


/* external data declarations */

/* function prototypes */


#endif		/*  __CO_FLAG_H */

/* end of source */

