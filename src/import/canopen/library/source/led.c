/*
 *++ led.c - implementation of state leds see DRP-303-3
 *-- led.c - implementiert die CANopen LEDS nach Standard DRP-303-3
 *
 * Copyright (c) 2001-2017 port GmbH Halle/Saale
 *------------------------------------------------------------------
 */


/**
* \file led.c
*++ Implementation of state leds see DRP-303-3
*-- Implementiert die CANopen LED nach Standard DRP-303-3
* \author port GmbH
*
*++ This module implements the standard DRP-303-3.
*++ It is used to control of two single color or one bicolor
*++ LED according to the current communication and
*++ CAN state respectively.
*++ The logical control of the LED states
*++ is carried out by the CANopen library.
*++ Every state change of the LED (on/off)
*++ is signalled with the function
*-- Dieses Modul implementiert den Standard DRP-303-3.
*-- Es dient zur Steuerung der dort beschriebenen Ansteuerung
*-- von 2 einzelnen bzw. einer 2-farb LED
*-- entprechend dem aktuellen Kommunikations- bzw. CAN-Status.
*-- Die logische Steuerung der LED-Zustände
*-- erfolgt automatisch durch die Library.
*-- Jede Änderung der Zustände der LEDs (ein/aus)
*-- wird über die Funktion
* ledInd()
*++ where the application can set the physical outputs
*++ for controlling the LED.
*-- signalisiert,
*-- so daß der Anwender die physikalische Steuerung der LEDs
*-- vornehmen kann.
*
*++ No initialzation is necessary.
*-- Für das LED Modul ist keine Initialisierung notwendig.
*
*/



/* header of standard C - libraries */
#include <stdio.h>
#include <string.h>
#include <cal_conf.h>
#include "timer.h"
#include "led.h"
#include "nmt.h"
#include <co_timer.h>

#ifdef CONFIG_REDUNDANCY_SUPPORT
# include "reduncy.h"
#endif /* CONFIG_REDUNDANCY_SUPPORT */


#ifdef CONFIG_CO_LED

/* constant definitions
---------------------------------------------------------------------------*/
#define CO_LED_FLICKER_TIME	500	/* 50 ms */
#define CO_LED_BLINK_TIME	2000	/* 200 ms */

/* local defined data types
---------------------------------------------------------------------------*/

/* list of external used functions, if not in headers
---------------------------------------------------------------------------*/

/* list of global defined functions
---------------------------------------------------------------------------*/

/* list of local defined functions
---------------------------------------------------------------------------*/
static void setLed(UNSIGNED8 led, UNSIGNED8 newState CO_COMMA_REDCY_PARA_DECL);
static void setupLedTimer(CO_REDCY_PARA_DECL);
static void runLedTimerInd(UNSIGNED8 ledOn CO_COMMA_REDCY_PARA_DECL);
static void errLedTimerInd(UNSIGNED8 ledOn CO_COMMA_REDCY_PARA_DECL);
static void setupErrLedState(CO_REDCY_PARA_DECL);

/* external variables
---------------------------------------------------------------------------*/

/* global variables
---------------------------------------------------------------------------*/

/* local defined variables
---------------------------------------------------------------------------*/
#ifdef CONFIG_RCS_IDENT
CO_LIB_INIT_VAR static char _rcsid[] = "$Id: led.c,v 2.25 2016/09/26 11:16:08 rli Exp $";
#endif /* CONFIG_RCS_IDENT */

#ifdef CONFIG_NO_GLOBAL_VARS
#else /* CONFIG_NO_GLOBAL_VARS */
CO_LIB_UNINIT_VAR static CO_LED_T		coLed CO_LINE_PARA_ARRAY_DEF;

# ifdef CONFIG_REDUNDANCY_SUPPORT
CO_LIB_UNINIT_VAR static CO_LED_T		redcyCoLed;
# endif /* CONFIG_REDUNDANCY_SUPPORT */
#endif /* CONFIG_NO_GLOBAL_VARS */


/***************************************************************************/
/**
*
*++ \brief setAutoBaudLed - set autobaud led on/off
*-- \brief setAutoBaudLed - setze LED Zustand für autobaud ein/aus
*
*++ This function sets the autobaud state for the ERROR and the RUN led.
*++ If the parameter is 0, both leds are switched off,
*++ otherwise switch on.
*-- Diese Funktion setzt den Autobaud Zustand für die Error und die RUN-LED.
*-- Wenn als Parameter 0 übergeben wird, wird ausgeschalten,
*-- ansonsten wird eingeschalten.
*
* \return
*++    nothing
*--    nichts
*/

void setAutoBaudLed(
	UNSIGNED8	state	/**< new state (on/off) */
	CO_COMMA_REDCY_PARA_DECL/**< number of CAN line 0..CO_MAX_CAN_LINES-1 */
    )
{
    if (state != 0) {
	/* set autobaud on */
#ifdef CONFIG_CO_RUN_LED
	/* set run led */
	setCoRunLedState(CO_RUN_LED_AUTOBAUD CO_COMMA_REDCY_PARA);
#endif /* CONFIG_CO_RUN_LED */

#ifdef CONFIG_CO_ERR_LED
	setCoErrLedState(CO_ERR_LED_LSS CO_COMMA_REDCY_PARA);
#endif /* CONFIG_CO_ERR_LED */
    } else {
	/* set autobaud off */
#ifdef CONFIG_CO_RUN_LED
	/* set run led */
	setCoRunLedState(CO_RUN_LED_PREOP CO_COMMA_REDCY_PARA);
#endif /* CONFIG_CO_RUN_LED */

#ifdef CONFIG_CO_ERR_LED
	resetCoErrLedState(CO_ERR_LED_LSS CO_COMMA_REDCY_PARA);
#endif /* CONFIG_CO_ERR_LED */
    }
}


/***************************************************************************/
/**
*
*++ \brief setLssMasterLed - set LSS Master Mode led on/off
*-- \brief setLssMasterLed - setze LED Zustand für aktiven LSS Master ein/aus
*
*++ This function sets the LSS master mode state for the ERROR and the RUN led.
*++ If the parameter is 0, LSS Master is not active
*++ both leds are inactive, otherwise switch on.
*-- Diese Funktion setzt den LSS Master Mode Zustand für die Error und die RUN-LED.
*-- Wenn als Parameter 0 übergeben wird, ist der LSS Master nicht aktiv
*-- und die LEDs werden ausgeschalten,
*-- ansonsten wird eingeschalten.
*
* \return
*++    nothing
*--    nichts
*/

void setLssMasterLed(
	UNSIGNED8	state	/**< new state (on/off) */
	CO_COMMA_REDCY_PARA_DECL/**< number of CAN line 0..CO_MAX_CAN_LINES-1 */
    )
{
    if (state != 0) {
	/* set lss master active on */
#ifdef CONFIG_CO_RUN_LED
	/* set run led */
	setCoRunLedState(CO_RUN_LED_LSS CO_COMMA_REDCY_PARA);
#endif /* CONFIG_CO_RUN_LED */

#ifdef CONFIG_CO_ERR_LED
	setCoErrLedState(CO_ERR_LED_LSS CO_COMMA_REDCY_PARA);
#endif /* CONFIG_CO_ERR_LED */
    } else {
	/* set autobaud off */
#ifdef CONFIG_CO_RUN_LED
	/* set run led */
	/* setCoRunLedState(CO_RUN_LED_PREOP CO_COMMA_REDCY_PARA); */
	updateNMTState_led(CO_REDCY_PARA);
#endif /* CONFIG_CO_RUN_LED */

#ifdef CONFIG_CO_ERR_LED
	resetCoErrLedState(CO_ERR_LED_LSS CO_COMMA_REDCY_PARA);
#endif /* CONFIG_CO_ERR_LED */
    }
}


/***************************************************************************/
/**
*
*++ \brief setCoLed - call user function ledInd() with correct parameters
*-- \brief setCoLed - Aufruf der Anwenderfunktion ledInd() mit korrekten Parametern
*
* \internal
*
*++ This function is called by the timer of the CANopen library.
*++ The function itself calls the user function ledInd() with the correct parameters.
*++ Turning the LEDs on or off is controlled with the internal variables
*++ errLedBlinkState and runLedBlinkState.
*++ These two variables contain information
*++ how often to blink and if the LED should be turned on or off.
*++ In the high-nibble information is kept on how often to blink.
*++ And in the low-nibble the current state of the led ON or OFF is stored.
*++ After a blinking sequence is over it is started again.
*-- Diese Funktion wird von dem Timer der CANopen-Bibliothek aufgerufen.
*-- Sie ruft die Anwenderfunktion ledInd() mit den korrekten Parametern auf.
*-- Anhand der beiden Variablen errLedBlinkState und runLedBlinkState
*-- wird das An- und Abschalten der LEDs gesteuert.
*-- Im höherwertigen Teil steht die Anzahl,
*-- wie oft die LED aufleuchten soll, und im niederwertigen Teil steht der
*-- Zustand AN oder AUS.
*-- Wenn eine Leuchtsequenz vorüber ist, wird sie von vorn gestartet.
*
*
* \return
*++    nothing
*--    nichts
*/

void setCoLed(
	TIMER_EVENT_T	*pTimer
	CO_COMMA_LINE_PARA_DECL	/* number of CAN line 0..CO_MAX_CAN_LINES-1 */
    )
{
#define ERR_LED_ON	1
#define ERR_LED_OFF	0
#define RUN_LED_ON	ERR_LED_OFF
#define RUN_LED_OFF	ERR_LED_ON

# ifdef CONFIG_REDUNDANCY_SUPPORT
UNSIGNED8	canLine;
# endif /* CONFIG_REDUNDANCY_SUPPORT */

CO_LED_T	*pLed = (CO_LED_T *)pTimer;

# ifdef CONFIG_REDUNDANCY_SUPPORT
    if (pLed == &GL_ARRAY(coLed))  {
	canLine = CAN_DEFAULT_LINE;
    } else {
	canLine = CAN_REDCY_LINE;
    }
# endif /* CONFIG_REDUNDANCY_SUPPORT */

    /* toggle on state between err and run led */
    pLed->ledOnState ^= 1;

    /* flickering active ? */
    if (pLed->timerState == CO_LED_STATE_FLICKERING)  {

#ifdef CONFIG_CO_RUN_LED
	/* should run led flicker ? */
	if ((pLed->runLedState & CO_LED_STATE_FLICKERING) != 0)  {
	    /* yes */
	    runLedTimerInd(pLed->ledOnState CO_COMMA_REDCY_PARA);
	}
#endif /* CONFIG_CO_RUN_LED */

#ifdef CONFIG_CO_ERR_LED
	/* should err led flicker ? */
	if ((pLed->errLedState & CO_LED_STATE_FLICKERING) != 0)  {
	    /* yes */
	    errLedTimerInd(pLed->ledOnState CO_COMMA_REDCY_PARA);
	}
#endif /* CONFIG_CO_ERR_LED */

	pLed->ledFlickerState ++;
	if (pLed->ledFlickerState > 3)  {
	    pLed->ledFlickerState = 0;
	}
    }

    /* work with blinkstates */
    /* valid, if flickering is disabled or each 4th of flickering */
    if (( pLed->timerState != CO_LED_STATE_FLICKERING)
     || ((pLed->timerState == CO_LED_STATE_FLICKERING)
	&& (pLed->ledFlickerState == 0))) {

#ifdef CONFIG_CO_RUN_LED
	/* should run led blink ? */
	if ((pLed->runLedState & CO_LED_STATE_BLINKING) != 0)  {
	    /* yes */
	    runLedTimerInd(pLed->ledOnState CO_COMMA_REDCY_PARA);
	}
#endif /* CONFIG_CO_RUN_LED */

#ifdef CONFIG_CO_ERR_LED
	/* should err led blink ? */
	if ((pLed->errLedState & CO_LED_STATE_BLINKING) != 0)  {
	    /* yes */
	    errLedTimerInd(pLed->ledOnState CO_COMMA_REDCY_PARA);
	}
#endif /* CONFIG_CO_ERR_LED */
    }
}


#ifdef CONFIG_CO_RUN_LED
/***************************************************************************/
/**
*++ \brief runLedTimerInd - set/reset run LED
*-- \brief runLedTimerInd - run LED Bearbeitung während der Timer Indication
*
* \internal
*
*++ This function is called by the timer indication at setCoLed()
*++ and set/reset the RUN Led.
*-- Diese Funktion wird von der TimerIndikation Funktion setCoLed gerufen
*-- und bearbeitet die RUN-Led
*
* \return
*++    nothing
*--    nichts
*/
static void runLedTimerInd(
	UNSIGNED8	ledOn	/* run state for err or run led */
	CO_COMMA_REDCY_PARA_DECL/* number of CAN line 0..CO_MAX_CAN_LINES-1 */
    )
{
CO_LED_T	*pLed;

# ifdef CONFIG_REDUNDANCY_SUPPORT
    if (canLine != CAN_DEFAULT_LINE)  {
	pLed = &GL_ARRAY(redcyCoLed);
    } else
# endif /* CONFIG_REDUNDANCY_SUPPORT */
    {
	pLed = &GL_ARRAY(coLed);
    }

    /* blink phase ? */
    if (pLed->runOnBlinkCnt != 0)  {
	if (ledOn == RUN_LED_ON)  {
	    /* switch on */
	    setLed(CO_RUN_LED, CO_LED_ON CO_COMMA_REDCY_PARA);
	    /* decr counter */
	    pLed->runOnBlinkCnt --;
	} else {
	    /* switch off */
	    setLed(CO_RUN_LED, CO_LED_OFF CO_COMMA_REDCY_PARA);
	}
    } else {
	/* switch off */
	setLed(CO_RUN_LED, CO_LED_OFF CO_COMMA_REDCY_PARA);
	/* blink counter is zero */
	/* check for off counter */
	if (pLed->runOffBlinkCnt != 0)  {
	    pLed->runOffBlinkCnt --;
	}
	if (pLed->runOffBlinkCnt == 0)  {
	    /* reload on counter */
	    pLed->runOnBlinkCnt = pLed->runOnBlinkVal;
	    pLed->runOffBlinkCnt = pLed->runOffBlinkVal;
	}
    }
}
#endif /* CONFIG_CO_RUN_LED */


#ifdef CONFIG_CO_ERR_LED
/***************************************************************************/
/**
*++ \brief errLedTimerInd - set/reset err led
*-- \brief errLedTimerInd - err LED Bearbeitung während der Timer Indication
*
* \internal
*
*++ This function is called by the timer indication at setCoLed()
*++ and set/reset the ERR Led.
*-- Diese Funktion wird von der TimerIndikation Funktion setCoLed gerufen
*-- und bearbeitet die ERR-Led
*
* \return
*++    nothing
*--    nichts
*/
static void errLedTimerInd(
	UNSIGNED8	ledOn	/* run state for err or run led */
	CO_COMMA_REDCY_PARA_DECL/* number of CAN line 0..CO_MAX_CAN_LINES-1 */
    )
{
CO_LED_T	*pLed;

# ifdef CONFIG_REDUNDANCY_SUPPORT
    if (canLine != CAN_DEFAULT_LINE)  {
	pLed = &GL_ARRAY(redcyCoLed);
    } else
# endif /* CONFIG_REDUNDANCY_SUPPORT */
    {
	pLed = &GL_ARRAY(coLed);
    }

    /* blink phase ? */
    if (pLed->errOnBlinkCnt != 0)  {
	if (ledOn == ERR_LED_ON)  {
	    /* switch on */
	    setLed(CO_ERR_LED, CO_LED_ON CO_COMMA_REDCY_PARA);
	    /* decr counter */
	    pLed->errOnBlinkCnt --;
	} else {
	    /* switch off */
	    setLed(CO_ERR_LED, CO_LED_OFF CO_COMMA_REDCY_PARA);
	}
    } else {
	/* switch off */
	setLed(CO_ERR_LED, CO_LED_OFF CO_COMMA_REDCY_PARA);
	/* blink counter is zero */
	/* check for off counter */
	if (pLed->errOffBlinkCnt != 0)  {
	    pLed->errOffBlinkCnt--;
	}
	if (pLed->errOffBlinkCnt == 0)  {
	    /* reload on counter */
	    pLed->errOnBlinkCnt = pLed->errOnBlinkVal;
	    pLed->errOffBlinkCnt = pLed->errOffBlinkVal;
	}
    }
}
#endif /* CONFIG_CO_ERR_LED */


#ifdef CONFIG_CO_RUN_LED
/***************************************************************************/
/**
*
*++ \brief setCoRunLedState - set RUN LED state
*-- \brief setCoRunLedState - setzt RUN LED Status
*
* \internal
*
*++ This function sets a new RUN-LED state if an event (state change)
*++ was occured.
*-- Diese Funktion setzt einen neuen RUN-LED Status,
*-- wenn ein Ereignis (neuer Betriebszustand) eingetreten ist.
*-- Zwischen den NMT-Zuständen kann dabei beliebig gewechselt werden.
*
* \return
*++    nothing
*--    nichts
*/

void setCoRunLedState(
	UNSIGNED8	newState/* new led state */
	CO_COMMA_REDCY_PARA_DECL/* number of CAN line 0..CO_MAX_CAN_LINES-1 */
    )
{
CO_LED_T	*pLed;

# ifdef CONFIG_REDUNDANCY_SUPPORT
    if (canLine != CAN_DEFAULT_LINE)  {
	pLed = &GL_ARRAY(redcyCoLed);
    } else
# endif /* CONFIG_REDUNDANCY_SUPPORT */
    {
	pLed = &GL_ARRAY(coLed);
    }

    switch (newState)  {
	case CO_RUN_LED_LSS:
	case CO_RUN_LED_AUTOBAUD:
	    pLed->runLedState = CO_LED_STATE_FLICKERING;
	    pLed->runOnBlinkVal = 1;
	    pLed->runOffBlinkVal = 1;
	    break;
	case CO_RUN_LED_STOPPED:
	    pLed->runLedState = CO_LED_STATE_BLINK_1;
	    pLed->runOnBlinkVal = 1;
	    pLed->runOffBlinkVal = 5;
	    break;
	case CO_RUN_LED_PREOP:
	    pLed->runLedState = CO_LED_STATE_BLINKING;
	    pLed->runOnBlinkVal = 1;
	    pLed->runOffBlinkVal = 1;
	    break;
	case CO_RUN_LED_OPERATIONAL:
	    pLed->runLedState = CO_LED_STATE_ON;
	    pLed->runOnBlinkVal = 0;
	    pLed->runOffBlinkVal = 0;
	    break;
	case CO_RUN_LED_OFF:
	default:
	    pLed->runLedState = CO_LED_STATE_OFF;
	    pLed->runOnBlinkVal = 0;
	    pLed->runOffBlinkVal = 0;
	    break;
    }

    /* set static codes */
    if ((pLed->runLedState &
	    (CO_LED_STATE_BLINKING | CO_LED_STATE_FLICKERING)) == 0)  {
	/* set static codes */
	if (pLed->runLedState == CO_LED_STATE_ON)  {
	    setLed(CO_RUN_LED, CO_LED_ON CO_COMMA_REDCY_PARA);
	} else {
	    setLed(CO_RUN_LED, CO_LED_OFF CO_COMMA_REDCY_PARA);
	}
    }
    pLed->runOnBlinkCnt = pLed->runOnBlinkVal;
    pLed->runOffBlinkCnt = pLed->runOffBlinkVal;

    /* enable/disable timer */
    setupLedTimer(CO_REDCY_PARA);
}
#endif /* CONFIG_CO_RUN_LED */


#ifdef CONFIG_CO_ERR_LED
/***************************************************************************/
/**
*++ \brief setCoErrLedState - set ERROR LED state
*-- \brief setCoErrLedState - setzt ERROR LED Status
*
* \internal
*
*++ This function sets the state variable if an event (error change)
*++ occured. The state can only change from a lower priority to a higher
*++ priority.
*++ Each error is saved until it is explicitely deleted by resetCoErrledState.
*-- Diese Funktion setzt einen neuen ERROR-LED Status,
*-- wenn ein Ereignis (neuer Fehlerzustand) eingetreten ist.
*-- Neue Fehlerzustände werden nur angezeigt,
*-- wenn kein Fehler höherer Priorität bereits angezeigt wird.
*-- (Prioritäten siehe DS 303)
*-- Jeder einmal eingetragene Fehler muß über die Funktion resetCoErrLedState
*-- auch wieder gelöscht werden.
*
* \par
*++ Sync Errors are not evaluated by the library. Therefore it is never
*++ going to be indicated. It is not always possible to read the error counter of
*++ CAN-controller. That is why the error <warning limit reached> can
*++ only be indicated if the controller supports reading this information.
*++ This has then to be handled by the driver. As it is the same with
*++ Autobaud.
*-- Sync-Fehler werden von der Bibliothek nicht ausgewertet.
*-- Daher wird dieser Fehlerzustand nicht angezeigt.
*-- Nicht jeder CAN-Controller bietet die Möglichkeit,
*-- den Fehlerzählerstand auszulesen.
*-- Deshalb kann der Fehlerzustand <warning limit reached> nur in Abhängigkeit
*-- von der verwendeten Hardware angezeigt werden.
*-- Das muss dann vom Treiber vorgenommen werden.
*-- Ebenso kann die Anzeige Autobaud nur vom Treiber aus erfolgen.
*
* \return
*++    nothing
*--    nichts
*/
void setCoErrLedState(
	UNSIGNED8	newErr	/* new error state */
	CO_COMMA_REDCY_PARA_DECL/* number of CAN line 0..CO_MAX_CAN_LINES-1 */
    )
{
# ifdef CONFIG_REDUNDANCY_SUPPORT
    if (canLine != CAN_DEFAULT_LINE)  {
	GL_ARRAY(redcyCoLed).errState |= newErr;
    } else
# endif /* CONFIG_REDUNDANCY_SUPPORT */
    {
	GL_ARRAY(coLed).errState |= newErr;
    }

    setupErrLedState(CO_REDCY_PARA);
}


/***************************************************************************/
/**
*++ \brief resetCoErrLedState - reset ERROR LED state
*-- \brief resetCoErrLedState - rücksetzen ERROR LED Status
*
* \internal
*
*++ This function resets an error state, looks for the last error and calls
*++ setCoLedState to set the last error state.
*-- Diese Funktion löscht LED Zustände.
*-- Dabei wird nur die angegebenen Fehler gelöscht.
*-- Andere noch vorhandene Fehler werden anschliessend wieder angezeigt.
*-- Damit können bisher nicht angezeigte niederpriore Fehler
*-- sichtbar gemacht werden.
*
* \return
*++    nothing
*--    nichts
*/
void resetCoErrLedState(
	UNSIGNED8	oldErr	/* error state */
	CO_COMMA_REDCY_PARA_DECL/* number of CAN line 0..CO_MAX_CAN_LINES-1 */
    )
{
# ifdef CONFIG_REDUNDANCY_SUPPORT
    if (canLine != CAN_DEFAULT_LINE)  {
	GL_ARRAY(redcyCoLed).errState &= ~oldErr;
    } else
# endif /* CONFIG_REDUNDANCY_SUPPORT */
    {
	GL_ARRAY(coLed).errState &= ~oldErr;
    }

    setupErrLedState(CO_REDCY_PARA);
}


/***************************************************************************/
/**
*++ \brief setupErrLedState - evaluation of the ERRORE Led state
*-- \brief setupErrLedState - auswerten des ERR Led Status
*
* \internal
*
*++ This function evaluates the highest ERROR LED state
*++ and shows this error on the error led.
*-- Diese Funktion wertet den aktuellen ERROR-Led Zustand aus
*-- und bringt den Fehler mit der höchsten Priorität zu Anzeige.
*
* \return
*++    nothing
*--    nichts
*/
static void setupErrLedState(
	CO_REDCY_PARA_DECL	/**< number of CAN line 0..CO_MAX_CAN_LINES-1 */
    )
{
CO_LED_T	*pLed;

# ifdef CONFIG_REDUNDANCY_SUPPORT
    if (canLine != CAN_DEFAULT_LINE)  {
	pLed = &GL_ARRAY(redcyCoLed);
    } else
# endif /* CONFIG_REDUNDANCY_SUPPORT */
    {
	pLed = &GL_ARRAY(coLed);
    }

    /* show only highest state */
    if ((pLed->errState & CO_ERR_LED_BUS_OFF) != 0)  {
	pLed->errLedState = CO_LED_STATE_ON;
	pLed->errOnBlinkVal = 0;
	pLed->errOffBlinkVal = 0;
    } else
    if ((pLed->errState & CO_ERR_LED_SYNC_ERROR) != 0)  {
	pLed->errLedState = CO_LED_STATE_BLINKING;
	pLed->errOnBlinkVal = 3;
	pLed->errOffBlinkVal = 5;
    } else
    if ((pLed->errState & CO_ERR_LED_NMT_ERROR) != 0)  {
	pLed->errLedState = CO_LED_STATE_BLINKING;
	pLed->errOnBlinkVal = 2;
	pLed->errOffBlinkVal = 5;
    } else
    if ((pLed->errState & CO_ERR_LED_EVENT_ERROR) != 0)  {
	pLed->errLedState = CO_LED_STATE_BLINKING;
	pLed->errOnBlinkVal = 4;
	pLed->errOffBlinkVal = 5;
    } else
    if ((pLed->errState & CO_ERR_LED_WARNING) != 0)  {
	pLed->errLedState = CO_LED_STATE_BLINKING;
	pLed->errOnBlinkVal = 1;
	pLed->errOffBlinkVal = 5;
    } else
    if ((pLed->errState & CO_ERR_LED_INVALID) != 0)  {
	pLed->errLedState = CO_LED_STATE_BLINKING;
	pLed->errOnBlinkVal = 1;
	pLed->errOffBlinkVal = 1;
    } else
    if (((pLed->errState & CO_ERR_LED_LSS) != 0)
     || ((pLed->errState & CO_ERR_LED_AUTOBAUD) != 0))  {
	pLed->errLedState = CO_LED_STATE_FLICKERING;
	pLed->errOnBlinkVal = 1;
	pLed->errOffBlinkVal = 1;
    } else {
	pLed->errLedState = CO_LED_STATE_OFF;
	pLed->errOnBlinkVal = 0;
	pLed->errOffBlinkVal = 0;
    }

    /* set static codes */
    if ((pLed->errLedState &
		(CO_LED_STATE_BLINKING | CO_LED_STATE_FLICKERING)) == 0)  {
	/* set static codes */
	if (pLed->errLedState == CO_LED_STATE_ON)  {
	    setLed(CO_ERR_LED, CO_LED_ON CO_COMMA_REDCY_PARA);
	} else {
	    setLed(CO_ERR_LED, CO_LED_OFF CO_COMMA_REDCY_PARA);
	}
    }
    pLed->errOnBlinkCnt = pLed->errOnBlinkVal;
    pLed->errOffBlinkCnt = pLed->errOffBlinkVal;

    /* enable/disable timer */
    setupLedTimer(CO_REDCY_PARA);
}
#endif /* CONFIG_CO_ERR_LED */


/***************************************************************************/
/**
*++ \brief setupLedTimer - switch LED timer on/off
*-- \brief setupLedTimer - LED Timer ein/ausschalten
*
* \internal
*
*++ This function switch the LED timer in one of the modes
*++ off, blinking oder flickering.
*-- Diese Funktion schaltet den LED Timer in einen der Modi
*-- aus, blinken oder flackern.
*
* \return
*++    nothing
*--    nichts
*/

static void setupLedTimer(
	CO_REDCY_PARA_DECL	/* number of CAN line 0..CO_MAX_CAN_LINES-1 */
    )
{
CO_LED_T	*pLed;

# ifdef CONFIG_REDUNDANCY_SUPPORT
    if (canLine != CAN_DEFAULT_LINE)  {
	pLed = &GL_ARRAY(redcyCoLed);
    } else
# endif /* CONFIG_REDUNDANCY_SUPPORT */
    {
	pLed = &GL_ARRAY(coLed);
    }

    /* blinking not necessary ? */
    if (((pLed->runLedState | pLed->errLedState)
	& (CO_LED_STATE_BLINKING | CO_LED_STATE_FLICKERING)) == 0)  {
	/* is timer is on ? */
	if (pLed->timerState != 0)  {
	    /* timer active, delete it */
	    removeTimerEvent(&pLed->timer CO_COMMA_LINE_PARA);
/* printf("setupLedTimer: STOP\n"); */
	    pLed->timerState = 0;
	}
    } else {
	/* blinking must be enabled, is it already active ? */
	if (((pLed->runLedState | pLed->errLedState)
		& CO_LED_STATE_FLICKERING) != 0)  {
	    /* flickerung should be on */
	    if (pLed->timerState != CO_LED_STATE_FLICKERING)  {
		/* start timer */
		(void)addTimerEvent(&pLed->timer,
			CO_LED_FLICKER_TIME,
		        CO_TIMER_TYPE_LED | CO_TIMER_TYPE_CYCLIC
			CO_COMMA_LINE_PARA);
/* printf("setupLedTimer: FLICKER\n"); */
		pLed->timerState = CO_LED_STATE_FLICKERING;
	    }
	} else {
	    if (pLed->timerState != CO_LED_STATE_BLINKING)  {
		(void)addTimerEvent(&pLed->timer,
			CO_LED_BLINK_TIME,
		        CO_TIMER_TYPE_LED | CO_TIMER_TYPE_CYCLIC
			CO_COMMA_LINE_PARA);
/* printf("setupLedTimer: BLINK\n"); */
		pLed->timerState = CO_LED_STATE_BLINKING;
	    }
	}
    }
}


/***************************************************************************/
/**
*++ \brief setLed - switch one of the LEDs on or off
*-- \brief setLed - schaltet eine der LEDs ein oder aus
*
* \internal
*
*++ This function switch one of the LED on or off
*++ and calls the user indication function if a new state should be shown.
*-- Diese Funktion schaltet eine der beiden LEDs ein oder aus
*-- und ruft die User-Indication, wenn ein neuer Status anzuzeigen ist.
*
* \return
*++    nothing
*--    nichts
*/
static void setLed(
	UNSIGNED8	led,
	UNSIGNED8	newState
	CO_COMMA_REDCY_PARA_DECL/* number of CAN line 0..CO_MAX_CAN_LINES-1 */
    )
{
CO_LED_T	*pLed;

# ifdef CONFIG_REDUNDANCY_SUPPORT
    if (canLine != CAN_DEFAULT_LINE)  {
	pLed = &GL_ARRAY(redcyCoLed);
    } else
# endif /* CONFIG_REDUNDANCY_SUPPORT */
    {
	pLed = &GL_ARRAY(coLed);
    }

    if (led == CO_ERR_LED)  {
#ifdef CONFIG_CO_ERR_LED
	/* new state to show ? */
	if (pLed->errLed != newState)  {
	    pLed->errLed = newState;
	    ledInd(CO_ERR_LED, newState CO_COMMA_REDCY_PARA);
	}
#endif /* CONFIG_CO_ERR_LED */
    } else {
#ifdef CONFIG_CO_RUN_LED
	/* new state to show ? */
	if (pLed->runLed != newState)  {
	    pLed->runLed = newState;
	    ledInd(CO_RUN_LED, newState CO_COMMA_REDCY_PARA);
	}
#endif /* CONFIG_CO_RUN_LED */
    }
}


/********************************************************************/
/**
*++ \brief initLedVars - init all LED variables
*++ \brief initLedVars - initialisiert alle LED Variablen
*
* \internal
*
* RETURNS
*++ \retval nothing
*-- \retval nichts
*
*/
void initLedVars(
	CO_LINE_PARA_DECL
    )
{
    memset(&GL_ARRAY(coLed), (int)0, (size_t)sizeof(CO_LED_T));

#  ifdef CONFIG_REDUNDANCY_SUPPORT
    memset(&GL_ARRAY(redcyCoLed), (int)0, (size_t)sizeof(CO_LED_T));
#  endif /* CONFIG_REDUNDANCY_SUPPORT */

    GL_ARRAY(coLed).errLed = 0xff;
    GL_ARRAY(coLed).runLed = 0xff;

# ifdef CONFIG_REDUNDANCY_SUPPORT
    GL_ARRAY(redcyCoLed).errLed = 0xff;
    GL_ARRAY(redcyCoLed).runLed = 0xff;
# endif /* CONFIG_REDUNDANCY_SUPPORT */

}

#ifdef CONFIG_CO_ERR_LED
/***************************************************************************/
/**
*++ \brief coGetCoErrLedState - returns the state of the CANopen error led
*-- \brief coGetCoErrLedState - übermittelt den Status der CANopen Fehler LED
*
* \public
*
* \return
*++    state of the led
*--    Status der LED
*/
UNSIGNED8 coGetCoErrLedState(
    CO_LINE_PARA_DECL
    )
{
CO_LED_T* pLed;

# ifdef CONFIG_REDUNDANCY_SUPPORT
    if (canLine != CAN_DEFAULT_LINE)  {
        pLed = &GL_ARRAY(redcyCoLed);
    } else
# endif /* CONFIG_REDUNDANCY_SUPPORT */
    {
        pLed = &GL_ARRAY(coLed);
    }
    return pLed->errLedState;
}
#endif /* CONFIG_CO_ERR_LED */

#ifdef CONFIG_CO_RUN_LED
/***************************************************************************/
/**
*++ \brief coGetCoRunLedState - returns the state of the CANopen run led
*-- \brief coGetCoRunLedState - übermittelt den Status der CANopen Status LED
*
* \public
*
* \return
*++    state of the led
*--    Status der LED
*/
UNSIGNED8 coGetCoRunLedState(
    CO_LINE_PARA_DECL
    )
{
CO_LED_T* pLed;

# ifdef CONFIG_REDUNDANCY_SUPPORT
    if (canLine != CAN_DEFAULT_LINE)  {
        pLed = &GL_ARRAY(redcyCoLed);
    } else
# endif /* CONFIG_REDUNDANCY_SUPPORT */
    {
        pLed = &GL_ARRAY(coLed);
    }
    return pLed->runLedState;
}
#endif /* CONFIG_CO_RUN_LED */


#endif /* CONFIG_CO_LED */

/*______________________________________________________________________EOF_*/
