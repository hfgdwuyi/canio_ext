/*
 * cpu.c - special cpu driver for a STM32H7xx target device
 *
 * Copyright (c) 2020 port GmbH Halle/Saale
 *------------------------------------------------------------------
 */

/**
*  \file cpu.c
*++ Special cpu driver for a STM32H7xx target device
*-- CPU spezifische Funktionen für den STM32H7xx Treiber
*  \author port GmbH Halle (Saale)
*
*++ This module contains adoptions between the CPU specific and
*++ CAN specific part of the driver.
*++ Here are also a place for functions that reside in the
*++ common driver but are not used.
*-- In diesem File befinden sich die Anpassungen zwischen CPU und CAN Treiber.
*-- Hier sollten ausserdem die Funktionen Platz finden, die sich in den
*-- allgemeinen Treibern befinden, aber nicht benutzt werden können.
*/

/**
* \def DEF_HW_PART
*++ activates hardware dependent settings
*++ within the header file cal_conf.h
*-- aktiviert hardwareabhängige Einstellungen
*-- in der Konfigurationsdatei cal_conf.h
*/

#include <environ.h>

/* includes  CANopen library */
#define DEF_HW_PART
#include <cal_conf.h>

#include <co_def.h>
#include <co_type.h>
#include <co_flag.h>
#include <co_stru.h>
#include <co_drv.h>


/* includes hardware libraries
---------------------------------------------------------------------------*/
#ifdef CONFIG_NO_GLOBAL_VARS
#  include <glob_drv.h>
#else /* CONFIG_NO_GLOBAL_VARS */
#  include <cdriver.h>
#  include <can_stm32_fdcan.h>
#endif /* CONFIG_NO_GLOBAL_VARS */


/* externals
---------------------------------------------------------------------------*/


/* globals
---------------------------------------------------------------------------*/
#ifdef CONFIG_NO_GLOBAL_VARS
#else /* CONFIG_NO_GLOBAL_VARS */
/** counter of disable/restore interrupt calls */
UNSIGNED8 cocnt_canInt CO_REDCY_PARA_ARRAY_DEF;
#endif /* CONFIG_NO_GLOBAL_VARS */


/* some defaults
---------------------------------------------------------------------------*/
#  if defined(CONFIG_MULT_LINES) || defined(CONFIG_REDUNDANCY_SUPPORT)

#  ifndef CONFIG_CAN_INT_PRIORITY_LINE0
#    define CONFIG_CAN_INT_PRIORITY_LINE0 1
#  endif /* CONFIG_CAN_INT_PRIORITY_LINE0 */
#  ifndef CONFIG_CAN_INT_PRIORITY_LINE1
#    define CONFIG_CAN_INT_PRIORITY_LINE1 1
#  endif /* CONFIG_CAN_INT_PRIORITY_LINE1 */

#  ifndef CONFIG_CAN_INT_SUBPRIORITY_LINE0
#  define CONFIG_CAN_INT_SUBPRIORITY_LINE0 0
# endif /*CONFIG_CAN_INT_SUBPRIORITY_LINE0 */
# ifndef CONFIG_CAN_INT_SUBPRIORITY_LINE1
#  define CONFIG_CAN_INT_SUBPRIORITY_LINE1 0
# endif /* CONFIG_CAN_INT_SUBPRIORITY_LINE1 */

#else /* CONFIG_MULT_LINES */

# ifndef CONFIG_CAN_INT_PRIORITY
#  define CONFIG_CAN_INT_PRIORITY 1
# endif /* CONFIG_CAN_INT_PRIORITY */

# ifndef CONFIG_CAN_INT_SUBPRIORITY
#  define CONFIG_CAN_INT_SUBPRIORITY 0
# endif /* CONFIG_CAN_INT_SUBPRIORITY */

#endif /* CONFIG_MULT_LINES */

/***************************************************************************/
/**
*++ \brief Init_CAN_Interrupts - initialize the CAN interrupt handling
*-- \brief Init_CAN_Interrupts - Initialisiert das CAN Interrupt Handling
*
*++ This function disables the CAN interrupts and resets the counter
*++ \em cocnt_canInt to 1.
*-- Diese Funktion deaktiviert alle CAN Interrupte
*-- und setzt den Zähler \em cocnt_canInt auf 1.
*
* \retval
*++ nothing
*-- nichts
*/
void Init_CAN_Interrupts(
		CO_REDCY_PARA_DECL
		)
{
    GL_DRV_ARRAY(cocnt_canInt) = 0;
    DISABLE_CAN_INTERRUPTS(CO_REDCY_PARA);

#if defined(CONFIG_MULT_LINES) || defined(CONFIG_REDUNDANCY_SUPPORT)
    if (canLine == 0)
    {
    	/* enable line 0 */
    	if (CONFIG_CAN_CONTROLLER_NUMBER_LINE0 == 1)
    	{
    	    HAL_NVIC_SetPriority(
    	    		FDCAN1_IT0_IRQn, 	    	
    	    		CONFIG_CAN_INT_PRIORITY_LINE0,
    	    		CONFIG_CAN_INT_SUBPRIORITY_LINE0
    	    		);
    	}
    	else if (CONFIG_CAN_CONTROLLER_NUMBER_LINE0 == 2)
    	{
#  ifdef FDCAN2
    	    HAL_NVIC_SetPriority(
    	    		FDCAN2_IT0_IRQn,
    	    		CONFIG_CAN_INT_PRIORITY_LINE0,
    	    		CONFIG_CAN_INT_SUBPRIORITY_LINE0
    	    		);
#  endif
    	}
    }
    else if (canLine == 1) {
    	/* enable line 1 */
    	if (CONFIG_CAN_CONTROLLER_NUMBER_LINE1 == 1)
    	{
    	    HAL_NVIC_SetPriority(
    	    		FDCAN1_IT0_IRQn,
    	    		CONFIG_CAN_INT_PRIORITY_LINE1,
    	    		CONFIG_CAN_INT_SUBPRIORITY_LINE1
    	    		);
    	}
    	else if (CONFIG_CAN_CONTROLLER_NUMBER_LINE1 == 2)
    	{
#  ifdef FDCAN2
    	    HAL_NVIC_SetPriority(
    	    		FDCAN2_IT0_IRQn,
    	    		CONFIG_CAN_INT_PRIORITY_LINE1,
    	    		CONFIG_CAN_INT_SUBPRIORITY_LINE1
    	    		);
#  endif
    	}
    }
#else /* CONFIG_MULT_LINES || CONFIG_REDUNDANCY_SUPPORT */
#  if (CONFIG_CAN_CONTROLLER_NUMBER == 1)
    HAL_NVIC_SetPriority(
    		FDCAN1_IT0_IRQn,
    		CONFIG_CAN_INT_PRIORITY,
    		CONFIG_CAN_INT_SUBPRIORITY
    		);
#  elif (CONFIG_CAN_CONTROLLER_NUMBER == 2)
#   ifdef FDCAN2
    HAL_NVIC_SetPriority(
    		FDCAN2_IT0_IRQn,
    		CONFIG_CAN_INT_PRIORITY,
    		CONFIG_CAN_INT_SUBPRIORITY
    		);
#   endif 
#  endif /* CONFIG_CAN_CONTROLLER_NUMBER */
#endif /* CONFIG_MULT_LINES */

} /* void Init_CAN_Interrupts */


/***************************************************************************/
/**
*++ \brief Enable_CAN_Interrupts - enables the CAN-controller interrupt
*-- \brief Enable_CAN_Interrupts - gibt CAN-Controller Interrupt frei
*
*++ Enables the interrupt for the CAN-controller.
*++ The disable/enable counter is reset.
*++ If possible in this function the CAN interrupt in the
*++ CPU interrupt modul is used, not the Interrupt settings
*++ in the CAN controller directly.
*-- Schaltet den Interrupt für den CAN-Controller aktiv.
*-- Ausserdem wird der Zähler für die Disable-Aufrufe gelöscht.
*-- Wenn möglich wird hier der CAN Interrupt im CPU-Interrupt-Modul
*-- und nicht direkt im CAN Controller bearbeitet.
*
* \returns
*++ nothing
*-- nichts
*/
void Enable_CAN_Interrupts(
		CO_REDCY_PARA_DECL
		)
{

    GL_DRV_ARRAY(cocnt_canInt) = 0;

    /* enable CAN Interrupt */
#if defined(CONFIG_MULT_LINES) || defined(CONFIG_REDUNDANCY_SUPPORT)
    if (canLine == 0) {
    	/* enable line 0 */
    	if (CONFIG_CAN_CONTROLLER_NUMBER_LINE0 == 1)
    	{
    		HAL_NVIC_EnableIRQ(FDCAN1_IT0_IRQn);
    	}
    	else if (CONFIG_CAN_CONTROLLER_NUMBER_LINE0 == 2)
    	{
#  ifdef FDCAN2
    		HAL_NVIC_EnableIRQ(FDCAN2_IT0_IRQn);
#  endif
    	}
    }
    else if (canLine == 1) {
    	/* enable line 1 */
    	if (CONFIG_CAN_CONTROLLER_NUMBER_LINE1 == 1)
    	{
    		HAL_NVIC_EnableIRQ(FDCAN1_IT0_IRQn);
    	}
    	else if (CONFIG_CAN_CONTROLLER_NUMBER_LINE1 == 2)
    	{
#  ifdef FDCAN2
    		HAL_NVIC_EnableIRQ(FDCAN2_IT0_IRQn);
#  endif
    	}
    }
#else /* CONFIG_MULT_LINES || CONFIG_REDUNDANCY_SUPPORT */
#  if (CONFIG_CAN_CONTROLLER_NUMBER == 1)
    HAL_NVIC_EnableIRQ(FDCAN1_IT0_IRQn);
#  elif (CONFIG_CAN_CONTROLLER_NUMBER == 2)
#   ifdef FDCAN2
    HAL_NVIC_EnableIRQ(FDCAN2_IT0_IRQn);
#   endif 
#  endif /* CONFIG_CAN_CONTROLLER_NUMBER */
#endif /* CONFIG_MULT_LINES */
} /* void Enable_CAN_Interrupts */


/***************************************************************************/
/**
*++ \brief Disable_CAN_Interrupts - disables the CAN-controller interrupt
*-- \brief Disable_CAN_Interrupts - sperrt CAN-Controller Interrupt
*
*++ Disables the interrupt for the CAN-controller.
*++ If possible in this function the CAN interrupt in the
*++ CPU interrupt modul is used, not the Interrupt settings
*++ in the CAN controller directly.
*++ Additionally the counter for disabling calls is incremented.
*-- Diese Funktion deaktiviert den CAN Interrupt.
*-- Wenn möglich wird hier der CAN Interrupt im CPU-Interrupt-Modul
*-- und nicht direkt im CAN Controller bearbeitet.
*-- Ausserdem wird der Zähler für die Disable-Aufrufe erhöht.
*
* \returns
*++ nothing
*-- nichts
*/
void Disable_CAN_Interrupts(
		CO_REDCY_PARA_DECL
		)
{

   if( GL_DRV_ARRAY(cocnt_canInt) == 0)
   {
	   /* disable CAN Interrupt */
#if defined(CONFIG_MULT_LINES) || defined(CONFIG_REDUNDANCY_SUPPORT)
	   if (canLine == 0) {
		   /* enable line 0 */
		   if (CONFIG_CAN_CONTROLLER_NUMBER_LINE0 == 1)
		   {
			   HAL_NVIC_DisableIRQ(FDCAN1_IT0_IRQn);
		   }
		   else if (CONFIG_CAN_CONTROLLER_NUMBER_LINE0 == 2)
		   {
#  ifdef FDCAN2
			   HAL_NVIC_DisableIRQ(FDCAN2_IT0_IRQn);
#  endif
		   }
	   }
	   else if (canLine == 1) {
		   /* enable line 1 */
		   if (CONFIG_CAN_CONTROLLER_NUMBER_LINE1 == 1)
		   {
			   HAL_NVIC_DisableIRQ(FDCAN1_IT0_IRQn);
		   }
		   else if (CONFIG_CAN_CONTROLLER_NUMBER_LINE1 == 2)
		   {
#  ifdef FDCAN2
			   HAL_NVIC_DisableIRQ(FDCAN2_IT0_IRQn);
#  endif
		   }
	   }
#else /* CONFIG_MULT_LINES || CONFIG_REDUNDANCY_SUPPORT */
#  if (CONFIG_CAN_CONTROLLER_NUMBER == 1)
	HAL_NVIC_DisableIRQ(FDCAN1_IT0_IRQn);
#  elif (CONFIG_CAN_CONTROLLER_NUMBER == 2)
#   ifdef FDCAN2
    HAL_NVIC_DisableIRQ(FDCAN2_IT0_IRQn);
#   endif
#  endif /* CONFIG_CAN_CONTROLLER_NUMBER */
#endif /* CONFIG_MULT_LINES */
   }
   GL_DRV_ARRAY(cocnt_canInt) ++;
} /* void Disable_CAN_Interrupts */


/***************************************************************************/
/**
*++ \brief Restore_CAN_Interrupts - restore the CAN-Controller interrupt
*-- \brief Restore_CAN_Interrupts - restauriert den CAN-Controller interrupt
*
*-- Dies Funktion wird zum Aktivieren des CAN Interrupts nach einem
*-- Disable_CAN_Interrupts() aufgerufen.
*-- Es wird der Zähler für die Disable-Aufrufe dekrementiert.
*-- Wird dieser 0, wird der CAN-Interrupt freigegeben.
*-- Diese Funktion wird für verschachtelte
*-- Disable_CAN_Interrupts() / Restore_CAN_Interrupts()
*-- benötigt.
*++ This function is used to enable the CAN interrupts after a
*++ Disable_CAN_Interrups() call.
*++ The disable/enable counter is decrementing.
*++ If this counter is 0 the CAN interrupt is enabled.
*++ This function is needed for nested
*++ Disable_CAN_Interrupts() / Restore_CAN_Interrupts()
*++ calls.
*
* \returns
*++ nothing
*-- nichts
*/
void Restore_CAN_Interrupts(
		CO_REDCY_PARA_DECL
		)
{
    if( GL_DRV_ARRAY(cocnt_canInt) > 1 )
    {
    	GL_DRV_ARRAY(cocnt_canInt) --;
    }
    else
    {
    	ENABLE_CAN_INTERRUPTS(CO_REDCY_PARA);
    }	
} /* void Restore_CAN_Interrupts */


/***************************************************************************/
/**
*++ \brief SetIntMask - installs the ISR for the CAN controller
*-- \brief SetIntMask - installiert die ISR für den CAN-Controller
*
*++ Commonly this function installs the ISR for the CAN controller.
*++ For many targets this is done by the compiler.
*-- Standardmäßig installiert diese Funktion die ISR des CAN-Controllers.
*-- Für viele Zielsystem übernimmt der Compiler diesen Teil.
*
* \returns
*++ nothing
*-- nichts
*/
void SetIntMask(
		CO_GLOBVARS_PARA_DECL
		)
{
   ;
} /* void SetIntMask */


/***************************************************************************/
/**
*++ \brief ResetIntMask - deinstalls the ISR for the CAN controller
*-- \brief ResetIntMask - deinstalliert die ISR für den CAN-Controller
*
* \returns
*++ nothing
*-- nichts
*/
void ResetIntMask(
		CO_GLOBVARS_PARA_DECL
		)
{
   ;
} /* void ResetIntMask */
