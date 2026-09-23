/*
 * co_led.h - public defines for led usage
 *
 * Copyright (c) 2001-2017 port GmbH Halle/Saale
 *------------------------------------------------------------------
 */

/**
* \file co_led.h
*++ Defines for LED state indication
*-- Definitionen für die Verwendung von LED zur Statussignalisierung
* \author port GmbH
*
*++ This file contains definitions for usage of state indication
*++ with LED.
*-- Diese Datei enthält Definitionen von Strukturen und Datentypen
*-- zur Verwendung von LED zur Statussignalisierung.
*
*/

#ifndef __CO_LED_H
# define __CO_LED_H

#include <co_def.h>


#define CO_ERR_LED		1	/* define for error led */
#define CO_RUN_LED		2	/* define for run led */

#define CO_LED_OFF		0	/* LED switch off */
#define CO_LED_ON		1	/* LED switch on */

#endif /* __CO_LED_H */


#ifdef CONFIG_WITHOUT_PROTOTYPES
#else /* CONFIG_WITHOUT_PROTOTYPES */

# ifndef __CO_LED_PROTOTYPES_H
#  define __CO_LED_PROTOTYPES_H

void ledInd(UNSIGNED8, UNSIGNED8 CO_COMMA_REDCY_PARA_DECL);
void setAutoBaudLed(UNSIGNED8 CO_COMMA_REDCY_PARA_DECL);
void setLssMasterLed(UNSIGNED8 state CO_COMMA_REDCY_PARA_DECL);
UNSIGNED8 coGetCoErrLedState(CO_LINE_PARA_DECL);
UNSIGNED8 coGetCoRunLedState(CO_LINE_PARA_DECL);

# endif /* __CO_LED_PROTOTYPES_H */
#endif /* CONFIG_WITHOUT_PROTOTYPES */
