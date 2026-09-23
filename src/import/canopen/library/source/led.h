/*
 * led.h - defines for led
 *
 * Copyright (c) 2001-2017 port GmbH Halle/Saale
 *------------------------------------------------------------------
 */

/**
* \file led.h
* \author port GmbH
*/

#include <co_led.h>


#ifndef __LED_H
# define __LED_H


#define CO_RUN_LED_OFF		0x00	/**< state: unknown */
#define CO_RUN_LED_LSS		0x01	/**< state: LSS services */
#define CO_RUN_LED_AUTOBAUD	0x02	/**< state: autobaud in progress */
#define CO_RUN_LED_STOPPED	0x04	/**< state: node is STOPPED */
#define CO_RUN_LED_PREOP	0x08	/**< state: node is PRE-OPERATIONAL */
#define CO_RUN_LED_OPERATIONAL	0x10	/**< state: node is OPERATIONAL */

#define CO_ERR_LED_OFF		0x00	/**< no error */
#define CO_ERR_LED_AUTOBAUD	0x01	/**< lss state active */
#define CO_ERR_LED_LSS		0x02	/**< lss state active */
#define CO_ERR_LED_INVALID	0x04	/**< invalid configuration*/
#define CO_ERR_LED_WARNING	0x08	/**< warning limit reached */
#define CO_ERR_LED_EVENT_ERROR	0x10	/**< event timer error */
#define CO_ERR_LED_NMT_ERROR	0x20	/**< error control failure */
#define CO_ERR_LED_SYNC_ERROR	0x40	/**< sync failure */
#define CO_ERR_LED_BUS_OFF	0x80	/**< bus off state */

#define CO_LED_STATE_OFF	0x00
#define CO_LED_STATE_ON		0x01
#define CO_LED_STATE_FLICKERING	0x40
#define CO_LED_STATE_BLINKING	0x80
#define CO_LED_STATE_BLINK_1	(CO_LED_STATE_BLINKING + 1)
#define CO_LED_STATE_BLINK_2	(CO_LED_STATE_BLINKING + 2)
#define CO_LED_STATE_BLINK_3	(CO_LED_STATE_BLINKING + 3)
#define CO_LED_STATE_BLINK_4	(CO_LED_STATE_BLINKING + 4)
#define CO_LED_STATE_BLINK_CNT	7



#if defined(CONFIG_CO_BOTH_LED) || defined(CONFIG_CO_BICOLOUR_LED)
# ifndef CONFIG_CO_RUN_LED
#  define CONFIG_CO_RUN_LED 1
# endif /* CONFIG_CO_RUN_LED */
# ifndef CONFIG_CO_ERR_LED
#  define CONFIG_CO_ERR_LED 1
# endif /* CONFIG_CO_ERR_LED */
#endif /* defined(CONFIG_CO_BOTH_LED) || defined(CONFIG_CO_BICOLOUR_LED) */

typedef struct {
	TIMER_EVENT_T	timer;		/* timer for led */
	UNSIGNED8 runLedState;		/* state of run led */
	UNSIGNED8 errLedState;		/* state of error led */
	UNSIGNED8 errState;		/* actual error states */
	UNSIGNED8 runOnBlinkCnt;	/* run led on blink cnt */
	UNSIGNED8 runOffBlinkCnt;	/* run led off blink cnt */
	UNSIGNED8 runOnBlinkVal;	/* act run led on blink value */
	UNSIGNED8 runOffBlinkVal;	/* act run led off blink value*/
	UNSIGNED8 errOnBlinkCnt;	/* error led on blink cnt */
	UNSIGNED8 errOffBlinkCnt;	/* error led off blink cnt */
	UNSIGNED8 errOnBlinkVal;	/* act err led on blink value */
	UNSIGNED8 errOffBlinkVal;	/* act err led off blink value*/
	UNSIGNED8 timerState;		/* act. led timer state */
	UNSIGNED8 errLed;
	UNSIGNED8 runLed;
	UNSIGNED8 ledOnState;		/* on state between err + run */
	UNSIGNED8 ledFlickerState;	/* flicker state for blinkmode*/
} CO_LED_T;

#endif /* __LED_H */

#ifdef CONFIG_WITHOUT_PROTOTYPES
#else /* CONFIG_WITHOUT_PROTOTYPES */

# ifndef __LED_PROTOTYPES_H
#  define __LED_PROTOTYPES_H

void setCoLed(TIMER_EVENT_T *pTimer CO_COMMA_LINE_PARA_DECL);
void setCoRunLedState(UNSIGNED8	newState CO_COMMA_REDCY_PARA_DECL);
void setCoErrLedState(UNSIGNED8 newErr CO_COMMA_REDCY_PARA_DECL);
void resetCoErrLedState(UNSIGNED8 oldErr CO_COMMA_REDCY_PARA_DECL);
void initLedVars(CO_LINE_PARA_DECL);

# endif /* __LED_PROTOTYPES_H */
#endif /* CONFIG_WITHOUT_PROTOTYPES */

