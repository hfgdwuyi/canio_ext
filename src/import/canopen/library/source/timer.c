/*
 *++ timer - contains timer functionality
 *-- timer - beinhaltet die Timer Funktionalität
 *
 * Copyright (c) 2001-2017 port GmbH Halle (Saale)
 *------------------------------------------------------------------
 */


/****************************************************************************/
/**
*  \file timer.c
*++ Contains timer functionality
*-- Beinhaltet die Timer Funktionalität
*  \author port GmbH Halle (Saale)
*
*++ This module contains all functions for usage of timer driven events
*++ of the CANopen Library.
*-- Dieses Modul enthält alle Funktionen zur Benutzung von Timerdiensten
*-- in der Bibliothek.
*
*++ Some of the functions are for internal usage only.
*++ They have no manual entries.
*-- Einige dieser Funktionen werden nur intern benutzt.
*-- Sie haben keine Handbuch Einträge.
*
*/

/* header of standard C - libraries */
#include <stdio.h>

/* header of common types */
#include <cal_conf.h>
#ifdef CONFIG_CPU_FAMILY_LINUX
# include <stdlib.h>		/* defines exit() */
#endif
#include <co_drvif.h>
#include <co_emcy.h>
#if defined(CONFIG_SYNC_PRODUCER) || defined(CONFIG_SYNC_CONSUMER)
# include "sync.h"
#endif /* defined(CONFIG_SYNC_PRODUCER) || defined(CONFIG_SYNC_CONSUMER) */
#include "pdo.h"
#include "nmterr.h"
#ifdef CONFIG_HEARTBEAT_CONSUMER
# include "heartbt.h"
#endif /* CONFIG_HEARTBEAT_CONSUMER */
#if defined(CONFIG_NODE_GUARDING) && defined(CONFIG_MASTER)
# include "nmt_m.h"
#endif /* defined(CONFIG_NODE_GUARDING) && defined(CONFIG_MASTER) */
#ifdef CONFIG_SDO_CLIENT
# include "sdo.h"
#endif /* CONFIG_SDO_CLIENT */
#ifdef CONFIG_FLYING_MASTER
# include "flyma.h"
#endif /* CONFIG_FLYING_MASTER */
#if defined(CONFIG_SRDO_PRODUCER) || defined(CONFIG_SRDO_CONSUMER)
# include "srdo.h"
#endif /* CONFIG_SRDO_PRODUCER */
#if defined(CONFIG_LSS_SLAVE) || defined(CONFIG_LSS_MASTER)
# include "lss.h"
#endif /* defined(CONFIG_LSS_SLAVE) || defined(CONFIG_LSS_MASTER) */
#ifdef CONFIG_CO_LED
# include "led.h"
#endif /* CONFIG_CO_LED */
#ifdef CONFIG_REDUNDANCY_SUPPORT
# include "reduncy.h"
#endif /* CONFIG_REDUNDANCY_SUPPORT */
#ifdef CONFIG_NMT_STARTUP_MANAGER
# include "nmtstart.h"
#endif /* CONFIG_NMT_STARTUP_MANAGER */
# include <co_timer.h>


/* constant definitions
---------------------------------------------------------------------------*/

/* local defined data types
---------------------------------------------------------------------------*/

/* list of external used functions, if not in headers
---------------------------------------------------------------------------*/

/* list of global defined functions
---------------------------------------------------------------------------*/

/* list of local defined functions
---------------------------------------------------------------------------*/
#if defined(TIMER_DEBUG) || defined(TIMER_DEBUG2)
int printTimerType(UNSIGNED8 timerType);
#endif

/* external variables
---------------------------------------------------------------------------*/

/* global variables
---------------------------------------------------------------------------*/
#ifdef CONFIG_NO_GLOBAL_VARS
#else /* CONFIG_NO_GLOBAL_VARS */

CO_LIB_INIT_VAR TIMER_EVENT_T	*co_timerList CO_LINE_PARA_ARRAY_DEF = { NULL };
CO_LIB_INIT_VAR INHIBIT_EVENT_T	*co_inhibitList CO_LINE_PARA_ARRAY_DEF = { NULL };
# ifdef CONFIG_EVA_VERSION
CO_LIB_UNINIT_VAR TIMER_EVENT_T	evaTimer;
# endif /* CONFIG_EVA_VERSION */
#endif /* CONFIG_NO_GLOBAL_VARS */


/* local defined variables
---------------------------------------------------------------------------*/

/*******************************************************************/
/*
*++ \brief checkTimerEvent - check the timer list for next event
*-- \brief checkTimerEvent - prüft die Timerliste für das nächste Event
*
* \internal
*
*++ This function checks if a event timed out.
*++ If yes, then the apropriate function is called,
*++ and the time is adjusted for the next event.
*-- Diese Funktion testet, ob die Zeit für das nächste Timerereignis
*-- abgelaufen ist.
*-- Wenn ja, wird die entsprechende Funktion aufgerufen
*-- und die Zeit für das nächste Timerereignis gesetzt
*
* \retval CO_OK
*	nothing
*
*/

void checkTimerEvent(
	CO_LINE_PARA_DECL	/* number of CAN line 0..CO_MAX_CAN_LINES-1 */
   )
{
TIMER_EVENT_T	*pTimer;	/* pointer to timer structure */
INHIBIT_EVENT_T	*pInhibit,	/* pointer to inhibit structure */
		*pLastInhibit;	/* pointer to last inhibit structure */
UNSIGNED8	timerTicks;	/* temporary coTimerTicks */
# ifdef CO_CONFIG_PDO_INHIBITTIME_RESEND
PDO_T           *pPdo;          /* pointer to timed pdo structure */
# endif /* CO_CONFIG_PDO_INHIBITTIME_RESEND */
    /* coTimerTicks can be changed by the timer interrupt
     * therefore we save it at a temporary variable */
    timerTicks = GL_ARRAY(coTimerTicks);
    pTimer = GL_ARRAY(co_timerList);

    /* first, check the inhibit timers */
    if (GL_ARRAY(co_inhibitList) != NULL)  {
	pInhibit = GL_ARRAY(co_inhibitList);
	pLastInhibit = NULL;
	/* for each entry */
	while (pInhibit != NULL)  {

#ifdef TIMER_DEBUG
printf("inhibitTimer\n");
printf("ptr = %x, ticks: %d, next = %x\n", (int)pInhibit, (int)pInhibit->ticks, (int)pInhibit->pNext);
#endif /* TIMER_DEBUG */

	    if (pInhibit->ticks < timerTicks)  {
		pInhibit->ticks = 0u;
	    } else {
		pInhibit->ticks -= timerTicks;
	    }
	    if (pInhibit->ticks == 0u)
            {


#ifdef TIMER_DEBUG
printf("inhibitTime reached\n");
printf("nextPtr %x\n", (int)pInhibit->pNext);
printf("curPtr %x\n", (int)pInhibit);
printf("lastPtr %x\n", (int)pLastInhibit);
printf("globalPtr %x\n", (int)GL_ARRAY(co_inhibitList));
#endif /* TIMER_DEBUG */

# ifdef CO_CONFIG_PDO_INHIBITTIME_RESEND
                pPdo = (PDO_T* ) pInhibit->pPdo;
# endif /* CO_CONFIG_PDO_INHIBITTIME_RESEND */

		/* remove it from the timer list */
		if (pLastInhibit != NULL)  {
		    pLastInhibit->pNext = pInhibit->pNext;
		    pInhibit->pNext = NULL;
		    pInhibit = pLastInhibit->pNext;
		} else {
		    GL_ARRAY(co_inhibitList) = pInhibit->pNext;
		    pInhibit->pNext = NULL;
		    pInhibit = GL_ARRAY(co_inhibitList);
		}

#ifdef TIMER_DEBUG
printf("inhibitTime:afterRemoval \n");
printf("curPtr %x\n", (int)pInhibit);
printf("lastPtr %x\n", (int)pLastInhibit);
printf("globalPtr %x\n", (int)GL_ARRAY(co_inhibitList));
#endif /* TIMER_DEBUG */

# ifdef CO_CONFIG_PDO_INHIBITTIME_RESEND
                /* timer was on PDO */
                if (pPdo != NULL)
                {
                    /* PDO write was called while inhibited */
                    if ((pPdo->inhibitFlags & PDO_INHIBIT_FLAG_RETRANSMIT) != 0u)
                    {
                        if ((pPdo->pCOB->eType & CO_COB_PDO_PROD) == CO_COB_PDO_PROD)
                        {
#  ifdef CO_CONFIG_PDO_INHIBITTIME_INDICATION
                            /* ask the application */
                            if (coUserPdoInhibittimeInd(pPdo->pdoNr, TRANSMIT_PDO CO_COMMA_LINE_PARA) == CO_OK)
#  else
                            if (1)
#  endif /* CO_CONFIG_PDO_INHIBITTIME_INDICATION */
                            {
                                writePdoReq(pPdo->pdoNr CO_COMMA_LINE_PARA);
                                if (pLastInhibit == NULL)
                                {
		                    pLastInhibit = &(pPdo->inhibit);
                                }
                            }
                        }
                        else
                        {
                            if ((pPdo->pCOB->eType & CO_COB_PDO_CONS) == CO_COB_PDO_CONS)
                            {
#  ifdef CO_CONFIG_PDO_INHIBITTIME_INDICATION
                                /* ask the application */
                                if (coUserPdoInhibittimeInd(pPdo->pdoNr, RECEIVE_PDO CO_COMMA_LINE_PARA) == CO_OK)
#  else
                                if (1)
#  endif /* CO_CONFIG_PDO_INHIBITTIME_INDICATION */
                                {
                                    writePdoReq(pPdo->pdoNr CO_COMMA_LINE_PARA);
                                    if (pLastInhibit == NULL)
                                    {
                                        pLastInhibit = &(pPdo->inhibit);
                                    }
                                }
                            }
                        }
                    }
                }
# endif /* CO_CONFIG_PDO_INHIBITTIME_RESEND */

	    } else {
		pLastInhibit = pInhibit;
		pInhibit = pInhibit->pNext;
	    }
	}
    } else {
	/* no inhibit timer active */

	/* valid timer entries available ? */
	if (pTimer == NULL)  {
	    /* no timer event defined, then delete actual ticks */
	    /* only allowed, if no inhibit timers are active */
	    GL_ARRAY(coTimerTicks) = 0u;
	    return;
	}

	/* check, if the first timer event is reached */
	/* as a precaution test it only if no event timer is active */
	if ((timerTicks < pTimer->endTime)
	 && (timerTicks < 10u)) {
	    /* no, return */
	    return;
	}
    }

    /* we wait waited, until the first time is reached or timerticks > 10 */
    GL_ARRAY(coTimerTicks) -= timerTicks;

    /* for all timer events */
    while (pTimer != NULL)  {

#ifdef CO_TIMER_DEBUG
printf("ptr2 ");
printf("line %d ", canLine);
printTimerType(pTimer->timerType);
printf(" = %x, ticks: %d, next = %x\n", (int)pTimer, (int)pTimer->endTime, (int)pTimer->pNext);
#endif /* CO_TIMER_DEBUG */

	if (pTimer->endTime < timerTicks)  {
	    pTimer->endTime = 0u;
	} else {
	    pTimer->endTime -= timerTicks;
	}
	pTimer = pTimer->pNext;
    }

    /* remove the zero entries and restart cyclic timers */
    pTimer = GL_ARRAY(co_timerList);
    while (pTimer != NULL)  {
	/* if the time is up */
	if (pTimer->endTime == 0u)  {

#ifdef CO_TIMER_DEBUG
printTimerType(pTimer->timerType);
printf("line %d\n", canLine);
/* printf("timerticks: %d, endTime: %d\n", coTicks, pTimer->endTime); */
#endif /* CO_TIMER_DEBUG */

#ifdef CONFIG_EVA_VERSION
	    /* check for eva-version timer */
	    if (pTimer == &evaTimer)  {
#ifdef CONFIG_CPU_FAMILY_LINUX
		fprintf(stderr, "\nSorry Starter-kit Code timed out\n");
		exit(1);
#else
		((void (CO_CODE *) (void))0x0)(); /* reset */
#endif
	    }
#endif /* CONFIG_EVA_VERSION */

	    /* disable/restart timer */
	    removeTimerEvent(pTimer CO_COMMA_LINE_PARA);

	    /* call the timer event */
	    switch (pTimer->timerType & ~CO_TIMER_TYPE_CYCLIC)  {

#ifdef CONFIG_SYNC_PRODUCER
		case CO_TIMER_TYPE_SYNC:
		    writeSyncReq(CO_LINE_PARA);
		    break;
#endif /* CONFIG_SYNC_PRODUCER */

#if defined(CONFIG_HEARTBEAT_PRODUCER)
		case CO_TIMER_TYPE_HB_PROD:
		    NMT_HB_TimerPulse(CO_LINE_PARA);
		    break;
#endif /* CONFIG_HEARTBEAT_PRODUCER */

#if defined(CONFIG_PDO_CONSUMER) && defined(CONFIG_PDO_EVENTTIMER)
		case CO_TIMER_TYPE_EVENTRPDO:
		    /* printf("* Event Rec PDO\n"); */
		    eventRecPdo(pTimer CO_COMMA_LINE_PARA);
		    break;
#endif /* defined(CONFIG_PDO_CONSUMER) && defined(CONFIG_PDO_EVENTTIMER) */

#if defined(CONFIG_PDO_PRODUCER) && defined(CONFIG_PDO_EVENTTIMER)
		case CO_TIMER_TYPE_EVENTTPDO:
		    /* printf("* Event Trans PDO\n"); */
		    eventTransPdo(pTimer CO_COMMA_LINE_PARA);
		    break;
#endif /* defined(CONFIG_PDO_PRODUCER) && defined(CONFIG_PDO_EVENTTIMER) */

#if defined(CONFIG_NODE_GUARDING) && defined(CONFIG_MASTER)
		case CO_TIMER_TYPE_NG_MSTR:
		    /* printf("* NodeGuarding Master Event\n"); */
		    NMT_M_TimerPulse(pTimer CO_COMMA_LINE_PARA);
		    break;
#endif /* defined(CONFIG_NODE_GUARDING) && defined(CONFIG_MASTER) */

#if defined(CONFIG_NODE_GUARDING) && defined(CONFIG_SLAVE)
		case CO_TIMER_TYPE_NG_SLAVE:
		    /* printf("* NodeGuarding Slave Event\n"); */
		    NMT_NG_TimerPulse(CO_LINE_PARA);
		    break;
#endif /* defined(CONFIG_NODE_GUARDING) && defined(CONFIG_SLAVE) */

#ifdef CONFIG_HEARTBEAT_CONSUMER
		case CO_TIMER_TYPE_HB_CONS:
		    /* printf("* Heartbeat Consumer Event\n"); */
		    NMT_HB_Cons_TimerPulse(pTimer CO_COMMA_LINE_PARA);
		    break;
#endif /* CONFIG_HEARTBEAT_CONSUMER */

#ifdef CONFIG_FLYING_MASTER
		case CO_TIMER_TYPE_FLYMA:
		    flymaTimerEvent(pTimer CO_COMMA_LINE_PARA);
		    break;
#endif /* CONFIG_FLYING_MASTER */

#ifdef CONFIG_SRDO_PRODUCER
		case CO_TIMER_TYPE_SRDO_PROD:
		    writeSrdo(pTimer CO_COMMA_LINE_PARA);
		    break;
#endif /* CONFIG_SRDO_PRODUCER */

#ifdef CONFIG_SRDO_CONSUMER
		case CO_TIMER_TYPE_SRDO_CON:
# ifdef CO_TIMER_DEBUG
		    printf("* SRDO Consumer\n");
# endif /* CO_TIMER_DEBUG */
		    srdoTimeOut(pTimer CO_COMMA_LINE_PARA);
		    break;
#endif /* CONFIG_SRDO_CONSUMER*/

#ifdef CONFIG_SDO_CLIENT
		case CO_TIMER_TYPE_SDO:
		    sdoTimeOut(pTimer CO_COMMA_LINE_PARA);
		    break;
#endif /* defined(CONFIG_SDO_CLIENT) */

#ifdef CONFIG_LSS_MASTER
		case CO_TIMER_TYPE_LSS_MSTR:
		    lssTimeOut(CO_LINE_PARA);
		    break;
#endif /* CONFIG_LSS_MASTER */

#ifdef CONFIG_LSS_SLAVE
		case CO_TIMER_TYPE_LSS_SL:
		    lssSwitchTimeEvent(CO_LINE_PARA);
		    break;
#endif /* CONFIG_LSS_SLAVE */

#ifdef CONFIG_CO_LED
		case CO_TIMER_TYPE_LED:
		    setCoLed(pTimer CO_COMMA_LINE_PARA);
		    break;
#endif /*  CONFIG_CO_LED */

#ifdef CONFIG_NMT_STARTUP_MANAGER
		case CO_TIMER_TYPE_NMT_BOOT_TIME:
		    nmtsTimerEvent(pTimer CO_COMMA_LINE_PARA);
		    break;
#endif

#ifdef CONFIG_REDUNDANCY_SUPPORT
		case CO_TIMER_TYPE_REDCY:
		    redcyTimerEvent(pTimer CO_COMMA_GLOBVARS_PARA);
		    break;
#endif /* CONFIG_REDUNDANCY_SUPPORT */

#ifdef CONFIG_USER_TIMER_EVENT
		case CO_TIMER_TYPE_USERSPEC:
		    userTimerEvent(pTimer CO_COMMA_LINE_PARA);
		    break;
#endif /* CONFIG_USER_TIMER_EVENT */

		default:
# ifdef CO_TIMER_DEBUG
		    printf("*** Unknown Timervent\n");
# endif /* CO_TIMER_DEBUG */
		    break;
	    }

	    /* disable/restart timer */
	    /* removeTimerEvent(pTimer CO_COMMA_LINE_PARA); */
	    /* if a cyclic timer, start it again */
	    if ((pTimer->timerType &
			    (CO_TIMER_TYPE_CYCLIC | CO_TIMER_TYPE_AGAIN))
		    != 0u) {
		pTimer->timerType &= (UNSIGNED8)~CO_TIMER_TYPE_AGAIN;
		(void)addTimerEvent(pTimer, pTimer->timerVal,
			pTimer->timerType | CO_TIMER_TYPE_REMAIN
			CO_COMMA_LINE_PARA);
	    }
	    /* pTimer = pTimer->pNext; */
	    pTimer = GL_ARRAY(co_timerList);
	} else {
	    break;
	}
    }

#ifdef CO_TIMER_DEBUG
pTimer = co_timerList;
if(pTimer != NULL) {
printf("ptr4 ");
printTimerType(pTimer->timerType);
printf(" = %x, ticks: %d, next = %x\n", (int)pTimer, (int)pTimer->endTime, (int)pTimer->pNext);
} else {
printf("ptr4: pTimer==NULL\n");
}
#endif /* CO_TIMER_DEBUG */
}


/*******************************************************************/
/**
*++ \brief addTimerEvent - add a timerevent to the timer list
*-- \brief addTimerEvent - fügt ein Timerevent zur Timerliste hinzu
*
*++ This function adds a new timer event to the timer list.
*++ In case it already exists it is then deleted before
*++ it is added to the timer list.
*++ The timer list is a sorted linked list.
*++ When the timer expires the indication function
*-- Diese Funktion fügt ein neues Timerereignis in die Timerliste ein.
*-- Dabei wird zuerst geprüft,
*-- ob schon ein Eintrag von diesem Ereignis vorhanden ist
*-- und ggf. gelöscht.
*-- Anschliessend wird das Timerereignis an der richtigen Stelle
*-- in der verketten Timerliste eingehängt.
*-- Nach Ablauf des Timers wird die Indikation-Funktion
* userTimerEvent()
*++ is called and deleted if it is not a cyclical timer.
*++ With the function
*-- aufgerufen
*-- und falls es sich nicht um einen zyklischen Timer handelt,
*-- aus der Liste gelöscht.
*-- Mit der Funktion
* removeTimerEvent()
*++ a timer event can be deleted from the timer list.
*-- kann das Timerereignis
*-- jederzeit aus der Liste entfernt werden.
*
*++ The paramter
*-- über den Parameter
* \em timerType
*++ sets the type of the timer.
*++ For user timers the type
*-- können die Eigenschaften des Timers festgelegt werden.
*-- Für User-Timer ist immer der timerType
* \c CO_TIMER_TYPE_USERSPEC
*++ shall be used.
*-- zu verwenden.
*++ Cyclic timers additionally need the type
*-- Zyklische Timer
*-- sind zusätzlich mit dem
* \c CO_TIMER_TYPE_CYCLIC
*-- zu initialisieren.
*
* \code
* TIMER_EVENT_T myTimer1, myTimer2;
*
* // add a timer for 1 sec
* addTimerEvent(&myTimer1, 10000, CO_TIMER_TYPE_USERSPEC);
*
* // add a cyclic timer for 5 sec
* addTimerEvent(&myTimer2, 50000,
*	CO_TIMER_TYPE_USERSPEC | CO_TIMER_TYPE_CYCLIC);
*
* // remove the cyclic timer
* removeTimerEvent(&myTimer2);
*
*
* void userTimerEvent(TIMER_EVENT_T *pTimer)  {
*
*	if (pTimer == &myTimer1)  {
*		printf("Timer 1 done. It is removed automatically");
*	}
*
*	if (pTimer == &myTimer2)  {
*		printf("Timer 2 done. It will called again");
*	}
* }
*
* \endcode
*
*++ If the timer value cannot be stored in timerticks, then
*++ the function returns with an error.
*++ In order to use large timer values
*++ the define \c CONFIG_LARGE_TIMER can be set.
*++ Timerticks then are stored as a U32 value.
*++ This is then valid for all timer variables !!
*-- Wenn der Timerwert nicht in Timerticks gespeichert werden kann,
*-- kehrt die Funktion mit einem Fehler zurück.
*-- Als Abhilfe kann das define \c CONFIG_LARGE_TIMER
*-- gesetzt werden. Dabei wird die timerTick Variable als U32 gespeichert
*-- Diese Variablengröße gilt aber dann für alle Timervariablen !!
*-- Wenn die angegeben Zeit kleiner als die Zeitauflösung (1 Tick) ist,
*-- wird die Zeit auf 1 Tick aufgerundet.
*
* \retval 0
*++	success
*--	Erfolg
* \retval 1
*--	bad time - timerVal = 0
*++	Zeitwert timerVal = 0
* \retval 2
*--	timerVal to large or coTimerTicks to small
*++	timerVal zu groß oder coTimerTicks zu klein gewählt
*/

UNSIGNED8 addTimerEvent(
	TIMER_EVENT_T	*pTimer, /**< pointer to event structure */
	UNSIGNED32	timerVal,/**< timervalue in 1/10 of msec */
	UNSIGNED8	timerType /**< timer type, acyclic/cyclic */
	CO_COMMA_LINE_PARA_DECL	/**< number of CAN line 0..CO_MAX_CAN_LINES-1 */
    )
{
TIMER_EVENT_T	*nextTimer,	/* pointer to timer structure */
		*postTimer;	/* pointer to timer structure */
UNSIGNED32	tVal;		/* timer value */

    if (timerVal == 0u)  {
# ifdef CO_TIMER_DEBUG
printf("*** addTimerEvent: bad time: 0, ");
printf("type: %d,\n ", timerType);
# endif /* CO_TIMER_DEBUG */
	return(1u);
    }

    removeTimerEvent(pTimer CO_COMMA_LINE_PARA);

    /* save type at the timer struct */
    pTimer->timerType = (timerType & (UNSIGNED8)~CO_TIMER_TYPE_REMAIN);
    pTimer->timerVal = timerVal;

    /* if isn't a cyclic call, ignore resttime */
    if ((timerType & CO_TIMER_TYPE_REMAIN) == 0u)  {
	pTimer->restTime = 0u;
    }

#ifdef CO_TIMER_DEBUG
printf("addTimer: %x, ", (int)pTimer);
printTimerType(timerType);
printf(" line %d", canLine);
printf("\n");
#endif /* CO_TIMER_DEBUG */
    /* calculate the endtime in ticks */

#ifdef CONFIG_LARGE_TIMER
    /* no range checking for large timer */
#else /* CONFIG_LARGE_TIMER */
    if (timerVal > ((UNSIGNED32)GL_VAR(coTimerPulse) << 15))  {
	return(2);
    }
#endif /* CONFIG_LARGE_TIMER */

    /* correct the timer period with the resttime from last call */

    /* check for timervalue < timer cycle */
    if ((timerVal + pTimer->restTime) <= GL_VAR(coTimerPulse))  {
	pTimer->endTime = 1u;
	pTimer->restTime = 0u;
    } else {
	tVal = timerVal + pTimer->restTime;

#ifdef CONFIG_LARGE_TIMER
	pTimer->endTime = tVal / GL_VAR(coTimerPulse);
#else /* CONFIG_LARGE_TIMER */
	pTimer->endTime = (UNSIGNED16)(tVal / GL_VAR(coTimerPulse));
#endif /* CONFIG_LARGE_TIMER */

	pTimer->restTime = (UNSIGNED16)(tVal % GL_VAR(coTimerPulse));
    }

    /* for not cyclic timers round up */
    if ((pTimer->restTime != 0u) && ((timerType & CO_TIMER_TYPE_CYCLIC) == 0u)) {
	pTimer->endTime ++;
    }

    /* add occured and not worked coTimerTicks */
    pTimer->endTime += GL_ARRAY(coTimerTicks);

#ifdef CO_TIMER_DEBUG
printf("type: %d, ", timerType);
printf("time: %ld, ticks: %d, rest %d\n", timerVal, pTimer->endTime, pTimer->restTime);
#endif /* CO_TIMER_DEBUG */

    /* set start of the list */
    postTimer = NULL;
    nextTimer = GL_ARRAY(co_timerList);

#ifdef CO_TIMER_DEBUG
printf("addTimer: co_timerList: %x", co_timerList);
#endif /* CO_TIMER_DEBUG */
    while (nextTimer != NULL)  {
	/* if the new time less, save it before */
	if (pTimer->endTime < nextTimer->endTime)  {
	     break;
	}
	postTimer = nextTimer;
	nextTimer = nextTimer->pNext;
    }

#ifdef CO_TIMER_DEBUG
printf(" postTimer: %x", postTimer);
printf(" nextTimer: %x", nextTimer);
#endif /* CO_TIMER_DEBUG */
    /* setup the new timer */
    pTimer->pNext = nextTimer;

    /* if is the first entry at the list ? */
    if (postTimer == NULL)  {
	GL_ARRAY(co_timerList) = pTimer;
    } else {
	postTimer->pNext = pTimer;
    }
#ifdef CO_TIMER_DEBUG
printf(" co_timerList: %x, pNext %x", co_timerList, co_timerList->pNext);
#endif /* CO_TIMER_DEBUG */

    return(0u);
}


/*******************************************************************/
/**
*++ \brief removeTimerEvent - delete a timerevent from the timer list
*-- \brief removeTimerEvent - löscht ein Timerevent aus der Timerliste
*
*
*++ This function removes a timer event of the linked list.
*++ If this function is called from inside of an indication function
*++ then this timer is not deleted but marked for deletion
*++ and finally deleted at the end of the function timercheck().
*-- Diese Funktion löscht ein Timerereignis aus der verketten Timerliste.
*-- Wird diese Funktion während einer Timer Indication Funktion aufgerufen,
*-- wird der Timer nur zum Löschen vorgemerkt
*-- und erst am Ende der Timer-Check Funktion gelöscht
*
* \retval none
*
*/

void removeTimerEvent(
	TIMER_EVENT_T	*pTimer /**< pointer to event structure */
	CO_COMMA_LINE_PARA_DECL	/**< number of CAN line 0..CO_MAX_CAN_LINES-1 */
    )
{
TIMER_EVENT_T	*pT,		/* pointer to timer structure */
		*pLast;		/* pointer to timer structure */

#ifdef CO_TIMER_DEBUG
    printf("removeTimer: %x ", (int)pTimer);
    printTimerType(pTimer->timerType);
    printf("next: %x\n", pTimer->pNext);
#endif /* CO_TIMER_DEBUG */

    /* search Vorgänger */
    pT = GL_ARRAY(co_timerList);
    pLast = NULL;

    /* for all timer events */
    while (pT != NULL)
    {
        if (pT == pTimer)
        {
            break;
	}
        pLast = pT;
        pT = pT->pNext;
    }
    /* timer wasn't found */
    if (pT != NULL)
    {

#ifdef CO_TIMER_DEBUG
        printf("type: %d, ", pTimer->timerType);
        printf("time: %ld\n", pTimer->timerVal);
        printf("next: %x\n", pTimer->pNext);
#endif /* CO_TIMER_DEBUG */

        /* set Nachfolger */
        if (pLast == NULL)
        {
            GL_ARRAY(co_timerList) = pTimer->pNext;
        }
        else
        {
            pLast->pNext = pTimer->pNext;
        }

        /* delete restTime for acyclic timers */
        if ((pTimer->timerType & CO_TIMER_TYPE_CYCLIC) == 0u)
        {
	    pTimer->restTime = 0u;
        }
    }
}


/*******************************************************************/
/**
*++ \brief changeTimerEvent - change a timerevent from the timer list
*-- \brief changeTimerEvent - ändert ein Timerevent in der Timerliste
*
*
*++ This function removes a timer event of the linked list.
*++ The expired time is used to calculate the next timer event.
*-- Diese Funktion ändert ein Timerereignis in der verketten Timerliste.
*-- Beim Aufruf dieser Funktion wird die bisher abgelaufene Zeit
*-- mit der neuen Zeit verrechnet.
*
* \retval 0
*++ ok
*-- ok
* \retval >0
*++ not ok
*-- nicht ok
*/

UNSIGNED8 changeTimerEvent(
	TIMER_EVENT_T	*pTimer,	/**< pointer to event structure */
	UNSIGNED32	timerVal,	/**< timervalue in 1/10 of msec */
	UNSIGNED8	timerType	/**< timer type, acyclic/cyclic */
	CO_COMMA_LINE_PARA_DECL	/**< number of CAN line 0..CO_MAX_CAN_LINES-1 */
    )
{
UNSIGNED8	retVal = 0u;
#ifdef CONFIG_LARGE_TIMER
UNSIGNED32	restTime;
#else
UNSIGNED16	restTime;
#endif /*CONFIG_LARGE_TIMER*/

    /* is timer is active */
    if (checkActiveTimer(pTimer CO_COMMA_LINE_PARA) == CO_FALSE)  {
	/* no */
	retVal = addTimerEvent(pTimer, timerVal, timerType CO_COMMA_LINE_PARA);
    } else {
	/* calculate time to next expiration */
	restTime = pTimer->endTime * GL_VAR(coTimerPulse);
	if (restTime > timerVal)  {
	    /* use new value, but note the coTimerTicks */
	    retVal = addTimerEvent(pTimer, timerVal, timerType
		CO_COMMA_LINE_PARA);
	} else {
	    /* don't change the timer, setup only new timer interval */
	    pTimer->timerVal = timerVal;
	}
    }
    return(retVal);
}


/*******************************************************************/
/**
*++ \brief checkActiveTimer - check, if timer is active
*-- \brief checkActiveTimer - testet, ob der Timer aktiv ist
*
*++ This function checks if the timer is active.
*-- Diese Funktion prüft, ob der Timer aktiv ist
*
* \retval CO_TRUE
*++ timer active
*-- Timer aktiv
* \retval CO_FALSE
*++ timer inactive
*-- Timer nicht aktiv
*
*/

BOOL_T checkActiveTimer(
	TIMER_EVENT_T	*pTimer /**< pointer to event structure */
	CO_COMMA_LINE_PARA_DECL	/**< number of CAN line 0..CO_MAX_CAN_LINES-1 */
    )
{
BOOL_T retVal = CO_FALSE;
TIMER_EVENT_T	*pT;		/* pointer to timer structure */


#ifdef CO_TIMER_DEBUG
    printf("checkActiveTimer\n");
#endif /* CO_TIMER_DEBUG */

    pT = GL_ARRAY(co_timerList);

    /* for all timer events */
    while (pT != NULL)
    {
        if (pT == pTimer)
        {
            retVal = CO_TRUE;
            break;
        }
        pT = pT->pNext;
    }

    return(retVal);
}


/*******************************************************************
*
*++ \brief startInhibitTimer - starts a inhibit timer
*-- \brief startInhibitTimer - startet einen Inhibit Timer
*
* \internal
*
*++ This function starts a new inhibit timer.
*++ The evaluation is done in checkTimeEvent().
*++ Every new inhibit timer is inserted
*++ at the start of the linked timer list.
*++ If the pointer to nextPtr is != 0, then
*++ the inhibit timer is already in the timer list
*++ and it isn't inserted.
*-- Diese Funktion startet einen neuen Inhibit Timer
*-- Die Auswertung erfolgt in checktimerEvent()
*-- Jeder neue Inhibit Timer wird am Anfang der Liste eingefügt.
*-- Wenn der Zeiger auf nextPtr != 0 ist,
*-- steht der Inhibit Timer schon in der Liste
*-- und muss nicht noch einmaleingetragen werden
*
* RETURNS
* .TP
*/

void startInhibitTimer(
	INHIBIT_EVENT_T	*pInhibit,/* pointer to inhibit structure */
	UNSIGNED16	timerVal/* timervalue in 1/10 of msec */
	CO_COMMA_LINE_PARA_DECL	/* number of CAN line 0..CO_MAX_CAN_LINES-1 */
    )
{
UNSIGNED16 ticks;		/* timer ticks */

    /* calculate the endtime in ticks */
    ticks = timerVal / GL_VAR(coTimerPulse);

    if ((ticks * GL_VAR(coTimerPulse)) < timerVal)  {
	ticks ++;
    }

    /* the inhibit time is a minimum time -
     * the first tick can occure immediately after this function
     * therefore we add one tick
     */
    ticks ++;

    /* check, if the inhibittimer is active */
    if (pInhibit->ticks != 0u)  {
	pInhibit->ticks = ticks;
        /* printf("timer already active\n"); */
    } else {

        pInhibit->ticks = ticks;

#ifdef TIMER_DEBUG
printf("startInhibitTimer 0x%x\n", (UNSIGNED32*) pInhibit);
printf("time: %d, ticks: %d\n", timerVal, (int)pInhibit->ticks);
#endif /* TIMER_DEBUG */

        /* save it at start of the list */
        pInhibit->pNext = GL_ARRAY(co_inhibitList);
        GL_ARRAY(co_inhibitList) = pInhibit;

#ifdef TIMER_DEBUG
printf("startInhibitTimer:enterList\n");
printf("nextPtr %x\n", (int)pInhibit->pNext);
printf("curPtr %x\n", (int)pInhibit);
printf("globalPtr %x\n", (int)GL_ARRAY(co_inhibitList));
#endif /* TIMER_DEBUG */

    }
}


/*******************************************************************
*
*++ \brief stopInhibitTimer - stops a inhibit timer
*-- \brief stopInhibitTimer - stoppt einen Inhibit Timer
*
* \internal
*
*++ This function removes an inhibit timer from the linked list.
*-- Diese Funktion entfernt einen Inhibit Timer aus der verketteten Liste
*
* \retval
*	nothing
*/

void stopInhibitTimer(
	INHIBIT_EVENT_T	*pInhibit/* pointer to inhibit structure */
	CO_COMMA_LINE_PARA_DECL	/* number of CAN line 0..CO_MAX_CAN_LINES-1 */
    )
{
INHIBIT_EVENT_T	*pTmp;		/* pointer to inhibit structure */
INHIBIT_EVENT_T	*pLastInhibit;	/* pointer to inhibit structure */

    /* delete timer val */
    pInhibit->ticks = 0u;

    if (GL_ARRAY(co_inhibitList) != NULL)  {

        pTmp = GL_ARRAY(co_inhibitList);
        pLastInhibit = NULL;

        /* for each entry */
        while (pTmp != NULL)  {
	    /* is the right entry ? */
	    if (pTmp == pInhibit)  {
	        /* yes, remove it from the timer list */
	        if (pLastInhibit == NULL)  {
		    GL_ARRAY(co_inhibitList) = pInhibit->pNext;
	        } else {
		    pLastInhibit->pNext = pInhibit->pNext;
	        }
	        pInhibit->pNext = NULL;
	        pTmp = NULL;
	    } else {
	        pLastInhibit = pTmp;
	        pTmp = pTmp->pNext;
	    }
        }
    }
}





#if defined(TIMER_DEBUG) || defined(TIMER_DEBUG2)
int printTimerType(
    UNSIGNED8 timerType
    )
{
#define TXT_CNT2 3
struct {
    UNSIGNED8	typ;
    char	*strg;
} static type[TXT_CNT2] = {
    {CO_TIMER_TYPE_CYCLIC,	"c"},
    {CO_TIMER_TYPE_AGAIN,	"a"},
    {CO_TIMER_TYPE_REMAIN,	"r"},
};
#define TXT_CNT 15
struct {
    UNSIGNED8	typ;
    char	*strg;
} static txt[TXT_CNT] = {
    {CO_TIMER_TYPE_SYNC,	"sync transmit"},
    {CO_TIMER_TYPE_HB_PROD,	"heartbeat producer transmit"},
    {CO_TIMER_TYPE_HB_CONS,	"heartbeat consumer"},
    {CO_TIMER_TYPE_NG_MSTR,	"Nodeguarding master"},
    {CO_TIMER_TYPE_NG_SLAVE,	"Nodeguarding slave"},
    {CO_TIMER_TYPE_EVENTRPDO,	"event rec pdos"},
    {CO_TIMER_TYPE_EVENTTPDO,	"event trans pdos"},
    {CO_TIMER_TYPE_FLYMA,	"flyma"},
    {CO_TIMER_TYPE_REDCY,	"redcy"},
    {CO_TIMER_TYPE_SRDO_PROD,	"srdo producer"},
    {CO_TIMER_TYPE_SRDO_CON,	"srdo consumer"},
    {CO_TIMER_TYPE_LSS_SL,	"lss slave"},
    {CO_TIMER_TYPE_LSS_MSTR,	"lss master"},
    {CO_TIMER_TYPE_LED,		"led"},
    {CO_TIMER_TYPE_USERSPEC,	"user specific timers"}
};
int i;

    for (i = 0; i < TXT_CNT2; i++)  {
	if ((timerType & type[i].typ) != 0)  {
	    printf("%s", type[i].strg);
	} else {
	    printf(" ");
	}
    }
    for (i = 0; i < TXT_CNT; i++)  {
	if ((timerType & 0x1f) == txt[i].typ)  {
	    printf("%s ", txt[i].strg);
	}
    }
    return(0);
}


void showTimerList(
    CO_LINE_PARA_DECL	/**< number of CAN line 0..CO_MAX_CAN_LINES-1 */
    )
{
TIMER_EVENT_T	*pTimer;	/* pointer to timer structure */

    pTimer = GL_ARRAY(co_timerList);

    /* for all timer events */
    while (pTimer != NULL)  {

	printf("ticks: %3d - ", (int)pTimer->endTime);
	printTimerType(pTimer->timerType);
	printf("\n");

	pTimer = pTimer->pNext;
    }
}
#endif /* defined(TIMER_DEBUG) || defined(TIMER_DEBUG2) */

/*******************************************************************
*
* \brief co_getNextInternalEventTime - get the time till next timer expires
*
* This function this function reports the time (in 1/10th of a millisecond)
* until the next timer in the library expires.
*
* This could be used to time the next call of FlushMbox on threaded systems.
*
* The call to FlushMbox needs to happen after the closest TimerInterupt. FlushMbox
* should still be called close to a received message.
*
* \retval 0
*       no events
* \retval >0
*       time in 1/10th of a millisecond
*
*/
UNSIGNED32 co_getNextInternalEventTime(
     CO_LINE_PARA_DECL       /**< number of CAN line 0..CO_MAX_CAN_LINES-1 */
)
{
UNSIGNED32 retVal = 0;
TIMER_EVENT_T *pTimer;

    pTimer = GL_ARRAY(co_timerList);

    if (pTimer != NULL)
    {
        retVal = pTimer->endTime * GL_VAR(coTimerPulse);
    }

    return retVal;
}

/*______________________________________________________________________EOF_*/
