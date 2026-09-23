/*
 * cpu_stm32_h7 - common cpu driver for a STM32H7xx cpu (CAN controller independend)
 *
 * Copyright (c) 2020 port GmbH Halle/Saale
 *------------------------------------------------------------------
 */

/**
*  \file cpu_stm32_h7.c
*++ Common cpu driver for a STM32H7xx cpu (CAN controller independend)
*-- Allgemeiner CPU Treiber f�r den STM32H7xx Treiber (CAN controller unabh�ngig)
*  \author port GmbH Halle (Saale)
*
*
*++ This file contains cpu specific routines for all STM32H7xx processors.
*-- Diese Datei enth�lt CPU spezifische Funktionen
*-- f�r den STM32H7xx Treiber.
*
*/

/**
* \def CONFIG_CPU_FAMILY_STM32_H7
*
*-- Dieses Define aktiviert allgemeinen Code f�r die STM32H7xx-CPU-Familie.
*-- In diesem Modul werden daf�r Funktionen f�r den CPU-Interrupt
*-- und das Timer-Handling bereitgestellt.
*-- Zum Freischalten der allgemeinen Timerfuktionalit�t mu� zus�tzlich
*-- das Define \c CONFIG_COLIB_TIMER gesetzt sein.
*++ This define enables the common code for CPUs of the STM32H7xx family.
*++ This module provides functions for the CPU-Interrupt
*++ and timer handling.
*++ In order to enable the timer functionality the define
*++ \c CONFIG_COLIB_TIMER has to be set.
*/
/**
* \def CONFIG_COLIB_TIMER
*-- Dieses Define aktiviert die allgemeine Timerfunktionalit�t,
*-- die f�r unsere Entwicklung genommen wurde.
*-- Diese mu� in jedem Falle vom Kunden gepr�ft und angepa�t werden.
*-- Mu� der Code ge�ndert werden, sollten die Funktionen nach
*-- \em drivers/\<hardware\>/cpu.c
*-- kopiert werden und dort ge�ndert werden.
*-- In dem Fall darf das Define \b CONFIG_COLIB_TIMER
*-- nicht gesetzt werden.
*-- In jedem Falle mu� die Timer-Interrupt-Periode in
*-- \b ::CONFIG_TIMER_INC [1/10 ms]
*-- eingetragen werden.
*-- Dies erfolgt �blicherweise mit dem Design Tool.
*++ This define enables the common timer functionality
*++ that was used for our development.
*++ It has to be checked by the customer if it suits his needs
*++ and may be adapted.
*++ If the code has to be changed then it is recommended
*++ to copy the functions into
*++ \em drivers/<hardware>/cpu.c
*++ and change them there.
*++ In this case the define \b CONFIG_COLIB_TIMER
*++ must not be set.
*++ In any case it is needed to set the define
*++ \b ::CONFIG_TIMER_INC [1/10ms].
*++ This is normally done by means of the DesignTool.
*/
/**
* \def CONFIG_TIMER_INC
*-- Dieses Define gibt den Abstand zwischen 2 Timerinterrupten an.
*-- Dieser Wert hat die Einheit 0.1 ms.
*-- Mit diesem Wert und dem Wert aus ::coTimerTicks berechnet die
*-- CANopen Library die zeitabh�ngigen Funktionen.
*++ This define sets the time period that elapses between two
*++ timer interrupts.
*++ With its value and the value of ::coTimerTicks
*++ the CANopen library calculates timing dependent functions.
*/
#ifdef DOXYGEN
	/* only for generation of the documentation */
#  define CONFIG_TIMER_INC 10
#  define CONFIG_COLIB_TIMER 1
#  define CONFIG_CPU_FAMILY_STM32_H7 1
#endif /* DOXYGEN */

/* includes  common c libraries */
#include <environ.h>

/* includes  CANopen library */
#define DEF_HW_PART
#include <cal_conf.h>


#ifdef CONFIG_CPU_FAMILY_STM32_H7
#  include <co_type.h>
#  include <co_def.h>
#  include <co_flag.h>
#  include <co_drv.h>

#  ifdef CONFIG_REDUNDANCY_SUPPORT
#    include <co_redcy.h>
#  endif /* CONFIG_REDUNDANCY_SUPPORT */

#  ifdef CONFIG_WITHOUT_GLOBAL_VARS
    /* include user specific header */
#    include <glob_drv.h>
#  endif /* CONFIG_WITHOUT_GLOBAL_VARS */

/* includes hardware libraries
---------------------------------------------------------------------*/


/* externals
---------------------------------------------------------------------*/


/* globals
---------------------------------------------------------------------*/
#  ifdef CONFIG_WITHOUT_GLOBAL_VARS
/* coTimerTicks is part of the library datatype */
#  else /* CONFIG_WITHOUT_GLOBAL_VARS */
VOLATILE UNSIGNED8  coTimerTicks CO_LINE_PARA_ARRAY_DEF; /**< CANopen timer ticks */
#  endif /* CONFIG_WITHOUT_GLOBAL_VARS */

UNSIGNED16 CO_CONST coTimerPulse = CONFIG_TIMER_INC; /**< length of 1 tick / unit 100us */


/* locals
---------------------------------------------------------------------*/
/* everytime global - only one CPU Interrupt in the hole system */
UNSIGNED8 cocnt_cpuInt;/**< counter of disable/restore interrupt calls */


#  ifndef CONFIG_TIMER_ISR_PRESTRING
#    define CONFIG_TIMER_ISR_PRESTRING  void
#  endif /* CONFIG_TIMER_ISR_PRESTRING */
#  ifndef CONFIG_TIMER_ISR_POSTSTRING
#    define CONFIG_TIMER_ISR_POSTSTRING
#  endif /* CONFIG_TIMER_ISR_POSTSTRING */

#  ifndef SET_COLIB_FLAG_ISR
#    define SET_COLIB_FLAG_ISR(flag) SET_COLIB_FLAG(flag)
#  endif /* SET_COLIB_FLAG_ISR */


/* global functions
---------------------------------------------------------------------*/
CONFIG_TIMER_ISR_PRESTRING Timer_int( CO_GLOBVARS_PARA_DECL ) CONFIG_TIMER_ISR_POSTSTRING;


/***************************************************************************/
/**
*++ \brief Init_CPU_Interrupts - initialize the CPU interrupt handling
*-- \brief Init_CPU_Interrupts - Initialisiert das CPU Interrupt Handling
*
*++ This function disables the CPU interrupt and sets the counter
*++ \em ::cocnt_cpuInt to 1.
*-- Diese Funktion deaktiviert alle Interrupte
*-- und setzt den Z�hler \em ::cocnt_cpuInt auf 1.
*
* \retval
*++ nothing
*-- nichts
*/
void Init_CPU_Interrupts(void)
{
    __disable_irq();

    cocnt_cpuInt = 1;
} /* void Init_CPU_Interrupts() */


/***************************************************************************/
/**
*++ \brief Enable_CPU_Interrupts - enables the CPU interrupt
*-- \brief Enable_CPU_Interrupts - gibt den CPU interrupt frei
*
*++ Enables the CPU interrupt.
*++ The disable/enable counter is reset.
*-- Aktiviert den CPU Interrupt.
*-- Ausserdem wird der Z�hler f�r die Disable-Aufrufe gel�scht.
*
* \returns
*++ nothing
*-- nichts
*/
void Enable_CPU_Interrupts(void)
{
    cocnt_cpuInt = 0;

    __enable_irq();

} /* void Enable_CPU_Interrupts() */


/***************************************************************************/
/**
*++ \brief Disable_CPU_Interrupts - disables the CPU interrupt
*-- \brief Disable_CPU_Interrupts - sperrt den CPU Interrupt
*
*++ Disables the CPU interrupt.
*++ Additionally the counter for disabling calls is incremented.
*-- Deaktiviert den CPU Interrupt.
*-- Ausserdem wird der Z�hler f�r die Disable-Aufrufe erh�ht.
*
* \returns
*++ nothing
*-- nichts
*/
void Disable_CPU_Interrupts(void)
{
    __disable_irq();

    cocnt_cpuInt++;
} /* void Disable_CPU_Interrupts() */


/****************************************************************************/
/**
*++ \brief Restore_CPU_Interrupts - restore the CPU interrupt state
*-- \brief Restore_CPU_Interrupts - Restauriert den vorigen CPU Interrupt Status
*
*++ This function restores the state of the CPU interrupt
*++ that was before the last call to \em Disable_CPU_Interrupt().
*-- Diese Funktion restauriert den Status des CPU Interrupts,
*-- der vorlag, bevor die Funktion \em Disable_CPU_Interrupts()
*-- aufgerufen wurde.
*
* \retval
*++ nothing
*-- nichts
*/
void Restore_CPU_Interrupts(void)
{
	if (cocnt_cpuInt > 1)
	{
		cocnt_cpuInt--;
	}
	else
	{
		Enable_CPU_Interrupts();
	}
} /* void Restore_CPU_Interrupts() */


#  ifdef CONFIG_COLIB_TIMER


/***************************************************************************/
/**
*++ \brief Timer_int - ISR for timer interrupt
*-- \brief Timer_int - ISR f�r den Timer-Interrupt
*
*++ This function contains actions which should be performed
*++ every timer interval.
*++ The value of the global variable
*++ ::coTimerTicks is incremented.
*++ With this value and the value of \b ::CONFIG_TIMER_INC
*++ the CANopen Library calculates the current time.
*++ The function is called at each periodic interrupt.
*-- Diese Funktion beinhaltet Ma�nahmen, die mit jedem
*-- Timerintervall ausgef�hrt werden sollen.
*-- Der Wert der globalen Variablen
*-- ::coTimerTicks wird erh�ht.
*-- Mit dieser Variablen und dem Wert aus \b ::CONFIG_TIMER_INC
*-- kann die Bibliothek die aktuelle Zeit bestimmen.
*-- Die Funktion wird mit jedem
*-- periodischen Interrupt aufgerufen.
*
* \returns
*++ nothing
*-- nichts
*/
CONFIG_TIMER_ISR_PRESTRING Timer_int(
		CO_GLOBVARS_PARA_DECL
		) CONFIG_TIMER_ISR_POSTSTRING
{
#    if defined(CONFIG_MULT_LINES) || defined(CONFIG_REDUNDANCY_SUPPORT)
UNSIGNED8 canLine;
#    endif /* (CONFIG_MULT_LINES) || defined(CONFIG_REDUNDANCY_SUPPORT) */

    CO_SET_BIT(4);

#    ifdef CONFIG_MULT_LINES
    for (canLine = 0; canLine < CO_MAX_CAN_LINES;canLine++)
    {
    	GL_ARRAY(coTimerTicks) ++; /* CANopen timer ticks */

    	SET_COLIB_FLAG_ISR(COFLAG_TIMER_PULSED);

    	/* wake-up CAN-Task */
    	CO_NEW_RX_MSG(CO_LINE_PARA);
    }
#    else /* CONFIG_MULT_LINES */

    GL_ARRAY(coTimerTicks) ++; /* CANopen timer ticks line independend */

#      ifdef CONFIG_REDUNDANCY_SUPPORT
    /* timer is only used for single line */
    canLine = CAN_DEFAULT_LINE;
#      endif /* CONFIG_REDUNDANCY_SUPPORT */

    SET_COLIB_FLAG_ISR(COFLAG_TIMER_PULSED); /* for all CAN lines */

    /* wake-up CAN-Task */
    CO_NEW_RX_MSG(CO_LINE_PARA);
#    endif /* CONFIG_MULT_LINES */

    CO_RESET_BIT(4);

} /* CONFIG_TIMER_ISR_PRESTRING Timer_int() */


/***************************************************************************/
/**
*++ \brief initTimer - initialize timer
*-- \brief initTimer - initialisiert Timer
*
*++ Resets the internal timer value to zero.
*++ Commonly it set the timer ISR to interrupt vector table.
*++ For this target it is done by the compiler.
*++ dTimerValue will only supported by compatibility.
*-- Setzt den internen Zeitz�hler auf 0.
*-- Standardm��ig wird die Timer-ISR in die
*-- Interruptvektortabelle eingetragen.
*-- F�r dieses System wird es durch den Compiler getan.
*-- dTimerValue wird nur aus Kompatibilit�tsgr�nden unterst�tzt.
*
* \retval 0
*++ success
*-- Erfolg
*
* \retval not 0
*++ failure
*-- Fehler
*/
UNSIGNED8 initTimer(CO_GLOBVARS_PARA_DECL)
{
#    if defined(CONFIG_MULT_LINES)
UNSIGNED8 canLine;
#    endif /* CONFIG_MULT_LINES */

#    ifdef CONFIG_MULT_LINES
    for (canLine = 0; canLine < CO_MAX_CAN_LINES; canLine++)
#    endif /* CONFIG_MULT_LINES */
    {
    	/* with Redundancy only a line-independend coTimerTicks exists */
    	GL_ARRAY(coTimerTicks) = 0; /* CANopen timer ticks */
    }

    /* used ST firmware HAL-Layer */
    /*
    */
	HAL_SYSTICK_Config(SystemCoreClock/1000);

    return(0);
} /* UNSIGNED8 initTimer() */


/***************************************************************************/
/**
*++ \brief releaseTimer - releases the timer
*-- \brief releaseTimer - deinitialisiert den Timer
*
*++ Actually only disables the Timer Interrupt.
*-- Diese Version verbietet nur den Timer Interrupt.
*
* \returns
*++ nothing
*-- nichts
*/
void releaseTimer(CO_GLOBVARS_PARA_DECL)
{
	/* disable Systick */
	HAL_SuspendTick();
	
	return;
} /* void releaseTimer() */

#  endif /* CONFIG_COLIB_TIMER */

#endif /* CONFIG_CPU_FAMILY_STM32_H7 */
