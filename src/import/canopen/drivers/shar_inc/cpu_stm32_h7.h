/*
 * cpu_stm32_h7.h - cpu specific header for STM32H7xx driver
 *
 * Copyright (c) 2020 port GmbH Halle/Saale
 *------------------------------------------------------------------
 */

/**
* \file cpu_stm32_h7.h
*++ CPU specific defines for STM32H7xx driver.
*-- CPU spezifische Defines für den STM32H7xx Treiber.
* \author port GmbH
*
*
*++ CPU specific defines for the STM32H7xx driver.
*-- CPU spezifische Defines für den STM32H7xx Treiber.
*/

#ifndef CPU_STM32_H7_H
#  define CPU_STM32_H7_H	1

#  ifdef CONFIG_CPU_FAMILY_STM32_H7

/* macro for time test on port pins */

/**
* \def CONFIG_TIME_TEST
*++ Activate defines for time measurements.
*++ For the examples the concrete defines
*++ are located in examples.h.
*++ For customer specific measurements
*++ the defines can be set here.
*-- Aktivierung der Makrodefinitionen
*-- für Zeitmessungen.
*-- Für die Beispiele befinden sich
*-- die Makros in examples.h
*-- Für eigene Zeitmessungen können die Makros
*-- hier definiert werden.
*
* \note
*++ CONFIG_EXPERIMENTAL must be set, too.
*-- CONFIG_EXPERIMENTAL muß auch aktiviert werden.
*/
#    ifdef DOXYGEN
	/* this part is only used for the generation of the documentation */
#      define CONFIG_TIME_TEST 1
#      undef CONFIG_TIME_TEST
#    endif /* DOXYGEN */

#    ifdef CONFIG_TIME_TEST
    /* !!! Please include your own definitions within the cal_conf.h !!! */
#    else /* CONFIG_TIME_TEST */

/**
 *++ set time measurement bit n 
 *-- Setzt Bit n für Start der Zeitmessung
 */
#      define CO_SET_BIT(n)

/**
 *++ reset time measurement bit n
 *-- Löscht Bit n für Ende der Zeitmessung
 */
#      define CO_RESET_BIT(n)

#    endif /* CONFIG_TIME_TEST */

#    ifndef INIT_CAN_INTERRUPTS
/**
 *++ initialize and disable CAN interrupts
 *-- Initialisiert und deaktiviert den CAN Interrupt
 */
#      define INIT_CAN_INTERRUPTS 	Init_CAN_Interrupts
#    endif /* INIT_CAN_INTERRUPTS */

#    ifndef DISABLE_CAN_INTERRUPTS
/**
 *++ disable CAN interrupts
 *-- Deaktiviert die CAN Interrupte
 */
#      define DISABLE_CAN_INTERRUPTS	Disable_CAN_Interrupts
#    endif /* DISABLE_CAN_INTERRUPTS */

#    ifndef ENABLE_CAN_INTERRUPTS
/**
 *++ enable CAN interrupts everytime
 *-- Aktiviert immer die CAN Interrupte 
 */
#      define ENABLE_CAN_INTERRUPTS		Enable_CAN_Interrupts
#    endif /* ENABLE_CAN_INTERRUPTS */

#    ifndef RESTORE_CAN_INTERRUPTS
/**
 *++ restore CAN interrupt state as it was
 *++ before DISABLE_CAN_INTERRUPTS
 *-- Stellt den CAN Interruptstatus von vor
 *-- DISABLE_CAN_INTERRUPTS wieder her
 */
#      define RESTORE_CAN_INTERRUPTS	Restore_CAN_Interrupts
#    endif /* RESTORE_CAN_INTERRUPTS */

#    ifndef INIT_CPU_INTERRUPTS
/**
 *++ initialize and disable CPU interrupts
 *-- Initialisiert und deaktiviert den CPU Interrupt
 */
#      define INIT_CPU_INTERRUPTS 	Init_CPU_Interrupts
/* #    define INIT_CPU_INTERRUPTS() */
#    endif /* INIT_CPU_INTERRUPTS */

#    ifndef DISABLE_CPU_INTERRUPTS
/**
 *++ disable CPU interrupts
 *++ For CANopen it is enough to disable CAN and Timer interrupt
 *-- Deaktiviert die CPU Interrupte.
 *-- Für die CANopen Bibliothek reicht es, wenn der CAN
 *-- und der Timer-Interrupt gesperrt wird.
 */
#      define DISABLE_CPU_INTERRUPTS 	Disable_CPU_Interrupts
/* #    define DISABLE_CPU_INTERRUPTS() */
#    endif /* DISABLE_CPU_INTERRUPTS */

#    ifndef ENABLE_CPU_INTERRUPTS
/**
 *++ enable CPU interrupts everytime
 *-- Aktiviert immer die CPU Interrupte 
 */
#      define ENABLE_CPU_INTERRUPTS 	Enable_CPU_Interrupts
/* #    define ENABLE_CPU_INTERRUPTS() */
#    endif /* ENABLE_CPU_INTERRUPTS */

#    ifndef RESTORE_CPU_INTERRUPTS
/**
 *++ restore CPU interrupt state as it was
 *++ before DISABLE_CPU_INTERRUPTS
 *-- Stellt den CPU Interruptstatus von vor
 *-- DISABLE_CPU_INTERRUPTS wieder her
 */
#      define RESTORE_CPU_INTERRUPTS 	Restore_CPU_Interrupts
/* #    define RESTORE_CPU_INTERRUPTS() */
#    endif /* RESTORE_CPU_INTERRUPTS */

#    ifdef CONFIG_NO_GLOBAL_VARS
#    else /* CONFIG_NO_GLOBAL_VARS */
#      if defined(CONFIG_MULT_LINES) || defined(CONFIG_REDUNDANCY_SUPPORT)
extern void Init_CAN_Interrupts(unsigned char);
extern void Disable_CAN_Interrupts(unsigned char);
extern void Enable_CAN_Interrupts(unsigned char);
extern void Restore_CAN_Interrupts(unsigned char);
#      else /* (CONFIG_MULT_LINES) || defined(CONFIG_REDUNDANCY_SUPPORT) */
extern void Init_CAN_Interrupts(void);
extern void Disable_CAN_Interrupts(void);
extern void Enable_CAN_Interrupts(void);
extern void Restore_CAN_Interrupts(void);
#      endif /* (CONFIG_MULT_LINES) || defined(CONFIG_REDUNDANCY_SUPPORT) */
#    endif /* CONFIG_NO_GLOBAL_VARS */

extern void Init_CPU_Interrupts(void);
extern void Disable_CPU_Interrupts(void);
extern void Enable_CPU_Interrupts(void);
extern void Restore_CPU_Interrupts(void);

#  endif /* CONFIG_CPU_FAMILY_STM32_H7 */
#endif /* CPU_STM32_H7_H */


