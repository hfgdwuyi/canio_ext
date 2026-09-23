/*
 *++ examples - definitions for a STM32H7xx target
 *-- examples - Definitionen für Zielsystem STM32H7xx
 *
 * Copyright (c) 2020 port GmbH Halle (Saale)
 *------------------------------------------------------------------
 */

/**
 *  \file examples.h
 *++ Definitions for a STM32H7xx target
 *-- Definitionen für Zielsystem STM32H7xx
 *  \author port GmbH Halle (Saale)
 *
 *++ Header file for adapt to examples and debug enviroment.
 *-- Header für die Anpassung der Beispiele an die Zielhardware
 *-- und für Debug Settings. 
 */

#ifndef EXAMPLES_H
#  define EXAMPLES_H 1

/*
   configure example's main() and setOptions() as void or with arguments

   # define CO_EXAMPLE_ARGS_DECL  int argc, char **argv
   # define CO_EXAMPLE_ARGS       argc, argv
   or
   # define CO_EXAMPLE_ARGS_DECL  void
   # define CO_EXAMPLE_ARGS
 */
#  define CO_EXAMPLE_ARGS_DECL  void
#  define CO_EXAMPLE_ARGS

/* object dictionary version and target strings for examples */
#  define CO_MANUF_DEV_NAME	"CANopen - STM32H7xx"
#  define CO_HW_VER		"1.0"
#  define CO_SW_VER		"4.4"


/*
 * define fix bit timing or loadable bit timing from file
 *
 * CAN_START_BIT_RATE = bitRate : use bitRate variable of example
 */
 
#  define CAN_START_BIT_RATE	bitRate        /* load from file */

#define CONFIG_NO_PRINTF    1

extern void uart_printf(char *fmt, ...);

/******************************************************************/

/* choise with or without printf() 
------------------------------------------*/
#  ifdef CONFIG_NO_PRINTF
#    define PRINTF(...)
#    define PUTCHAR(c)
#    define FFLUSH(c)
#  else /* CONFIG_NO_PRINTF */
#    include <stdio.h>
#    define PRINTF uart_printf
#    define PUTCHAR(c)
#    define FFLUSH(dev)
#  endif /* CONFIG_NO_PRINTF */

/* common prototypes 
------------------------------------------*/
UNSIGNED8 	iniDevice(void);
BOOL_T    	endLoop(void);
BOOL_T 		setOptions(CO_EXAMPLE_ARGS_DECL);

#endif /* EXAMPLES_H */
