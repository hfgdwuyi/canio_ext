/*
 *++ cmsmain - contains functions for all CMS services of CAL
 *-- cmsmain - beinhaltet Funktionen für alle CMS Dienste von CAL
 *
 * Copyright (c) 1994-2017 port GmbH Halle (Saale)
 *------------------------------------------------------------------
 */

/**
*  \file cmsmain.c
*  \author port GmbH Halle (Saale)
*
*++ This file contains functions for all CMS services.
*-- Diese Datei enthält Funktionen für alle CMS-Dienste.
*/

/* header of standard C - libraries */

#include <string.h>
#include <stdio.h>

/* header of project specific types */

#include <cal_conf.h>
#include <co_drvif.h>
#include <co_cobid.h>
#include "cmsmain.h"
#include "nmt.h"
#include "nmterr.h"
#include "pdo.h"
#include "sdomain.h"
#include "emerg.h"
#include "drv.h"

#if defined(CONFIG_TIME_PRODUCER) || defined(CONFIG_TIME_CONSUMER)
# include "time_lib.h"
#endif /* defined(CONFIG_TIME_PRODUCER) || defined(CONFIG_TIME_CONSUMER) */

#if defined(CONFIG_MASTER)
# include "nmt_m.h"
#endif /* defined(CONFIG_MASTER) */

#ifdef CONFIG_DYN_SDO_CONNECTION_MANAGER
# include "sdomgr.h"
#endif /* CONFIG_DYN_SDO_CONNECTION_MANAGER */

#ifdef CONFIG_FLYING_MASTER
# include "flyma.h"
#endif /* CONFIG_FLYING_MASTER */

#ifdef CONFIG_SRDO_CONSUMER
# include "srdo.h"
#endif /* CONFIG_SRDO_CONSUMER */

#if defined(CONFIG_LSS_SLAVE) || defined(CONFIG_LSS_MASTER)
# include "lss.h"
#endif /* defined(CONFIG_LSS_SLAVE) || defined(CONFIG_LSS_MASTER) */

#ifdef CONFIG_REDUNDANCY_SUPPORT
# include "reduncy.h"
#endif /* CONFIG_REDUNDANCY_SUPPORT */

#ifdef CO_CONFIG_GFC_CONSUMER
# include "gfc.h"
#endif /* CO_CONFIG_GFC_CONSUMER */

#ifdef CONFIG_CAN_SEND
# include "cansend.h"
#endif /* CONFIG_CAN_SEND */

#ifdef CO_CONFIG_USER_MESSAGE_TEST
# include <co_usr.h>
#endif /* CO_CONFIG_USER_MESSAGE_TEST */


/* constant definitions
---------------------------------------------------------------------------*/

/* local defined data types
---------------------------------------------------------------------------*/

/* list of external used functions, if not in headers
---------------------------------------------------------------------------*/
#ifdef CONFIG_USER_CAN_MSG
void usrCanMsgReceived(CAN_MSG_T *canMsg CO_COMMA_LINE_PARA_DECL);
#endif /*CONFIG_USER_CAN_MSG */


/* list of global defined functions
---------------------------------------------------------------------------*/

/* list of local defined functions
---------------------------------------------------------------------------*/
#if (defined(CONFIG_NODE_GUARDING) && defined(CONFIG_SLAVE)) \
  || defined(CONFIG_HEARTBEAT_PRODUCER) \
  || defined(CONFIG_PDO_PRODUCER)
static void rtrRequestReceived(CAN_MSG_T *canMsg CO_COMMA_LINE_PARA_DECL);
#endif

/* external variables
---------------------------------------------------------------------------*/

/* global variables
---------------------------------------------------------------------------*/

/* local defined variables
---------------------------------------------------------------------------*/
#ifdef CONFIG_RCS_IDENT
CO_LIB_INIT_VAR static char _rcsid[] = "$Id: cmsmain.c,v 2.44 2016/09/26 11:16:08 rli Exp $";
#endif /* CONFIG_RCS_IDENT */


/****************************************************************************/
/**
*
*++ \brief msgIdentification - Entrypoint for CANopen Library
*-- \brief msgIdentification - Einsprungspunkt für die CANopen Bibliothek
*
*-- Diese funktion wird von \a FlushMbox() aufgerufen.
*-- Sie fährt die Erkennung der CAN-Telegramme durch.
*++ This function is called from inside \a FlushMbox().
*++ It performs the identification if a CAN message.
*
* \return
*++ nothing
*-- nichts
*
*/

void msgIdentification(
	CAN_MSG_T *canMsg	/*!< Pointer to CAN Message */
	CO_COMMA_REDCY_PARA_DECL
    )
{

#ifdef CO_CONFIG_USER_MESSAGE_TEST
    /* let the application see and test any message received by the library */
    /* SYNC messages have already been processed at this point */
    if (coUserMessageTestInd(canMsg CO_COMMA_REDCY_PARA) != CO_OK)
    {
        return;
    }
#endif /* CO_CONFIG_USER_MESSAGE_TEST */

     /* if rtr request */
    if ((canMsg->length & CO_RTR_REQ) != 0u) {
	/* yes, rtr request */
#if (defined(CONFIG_NODE_GUARDING) && defined(CONFIG_SLAVE)) \
  || defined(CONFIG_HEARTBEAT_PRODUCER) \
  || defined(CONFIG_PDO_PRODUCER)
	rtrRequestReceived(canMsg CO_COMMA_LINE_PARA);
#endif /* (defined(CONFIG_NODE_GUARDING) && defined(CONFIG_SLAVE)) || defined(CONFIG_PDO_PRODUCER) */
	return;
    }

#ifdef CONFIG_REDUNDANCY_SUPPORT
     /* Sometimes we need the information on which line the data was received;
     * we save it in a variable
     * - the user can detect this over the function reduncyGetReceivedLine()
     */
    GL_VAR(co_redcyReceivedLine) = canLine;
#endif /* CONFIG_REDUNDANCY_SUPPORT */

    /* for faster access */
    switch ((int)canMsg->cobType)  {

#ifdef CONFIG_PDO_CONSUMER
	case CO_COB_PDO_CONS:		/* PDO */
	case CO_COB_PDO_CONS_RTR:	/* PDO */
	    pdoMsgReceived(canMsg CO_COMMA_REDCY_PARA);
	    break;
#endif /* CONFIG_PDO_CONSUMER */

	case CO_COB_NMT_SLAVE:	/* NMT Event */
	case CO_COB_NMT_MASTER:	/* NMT Event */
	/* NMT Event */
#if defined(CONFIG_SLAVE) || defined(CONFIG_FLYING_MASTER)
	    NMT_NodeStartStopMsg(canMsg CO_COMMA_REDCY_PARA);
#endif /* CONFIG SLAVE */
	    break;

	case CO_COB_SDO_RX:	/* SDO */
	    sdoMsgReceived(canMsg CO_COMMA_REDCY_PARA);
	    break;

#if (defined(CONFIG_MASTER) && defined(CONFIG_NODE_GUARDING)) \
    || defined(CONFIG_HEARTBEAT_CONSUMER)
	case CO_COB_HB_CONS:	/* NMT ERROR control */
	case CO_COB_GUARD_MASTER:
	    NMT_M_NodeGuardingMsg(canMsg CO_COMMA_REDCY_PARA);
	    break;
#endif /* (defined(CONFIG_MASTER) && (defined(CONFIG_NODE_GUARDING) || */

#ifdef CONFIG_EMCY_CONSUMER
	case CO_COB_EMCY_CONS:	/* Emergency */
	    emcyMsgReceived(canMsg CO_COMMA_REDCY_PARA);
	    break;
#endif /* CONFIG_EMCY_CONSUMER */

#ifdef CONFIG_TIME_CONSUMER
	case CO_COB_TIME_CONS:	/* Time Stamp */
	    timeMsgReceived(canMsg CO_COMMA_LINE_PARA);
	    break;
#endif /* CONFIG_TIME_CONSUMER */

#ifdef CONFIG_FLYING_MASTER
	case CO_COB_FLYMA_RX:
	    flymaMsgReceived(canMsg CO_COMMA_REDCY_PARA);
	    break;
#endif /* CONFIG_FLYING_MASTER */

#ifdef CONFIG_REDUNDANCY_SUPPORT
	case CO_COB_REDCY_RX:
	    redcySetActiveLine(CO_REDCY_PARA);
	    break;
#endif /* CONFIG_REDUNDANCY_SUPPORT */

#ifdef CONFIG_SRDO_CONSUMER
	case CO_COB_SRDO_CONS:
	    srdoMsgReceived(canMsg CO_COMMA_LINE_PARA);
	    break;
#endif /* CONFIG_SRDO_CONSUMER */

#ifdef CONFIG_DYN_SDO_CONNECTION_MANAGER
	case CO_COB_SDOMGR_RX:
	    dynSdoRegistration(CO_LINE_PARA);
	    break;
#endif /* CONFIG_DYN_SDO_CONNECTION_MANAGER */

#if defined(CONFIG_LSS_MASTER) && defined(CONFIG_LSS_SLAVE)
	case CO_COB_LSS_RX:
	    lssMsgReceived(canMsg CO_COMMA_LINE_PARA);
	    break;
#else /* defined(CONFIG_LSS_MASTER) && defined(CONFIG_LSS_SLAVE) */

# ifdef CONFIG_LSS_MASTER
	case CO_COB_LSS_RX:
	    lssConMsgReceived(canMsg CO_COMMA_LINE_PARA);
	    break;
# endif /* CONFIG_LSS_MASTER */

# ifdef CONFIG_LSS_SLAVE
	case CO_COB_LSS_RX:
	    lssReqMsgReceived(canMsg CO_COMMA_LINE_PARA);
	    break;
# endif /* CONFIG_LSS_SLAVE */
#endif /* defined(CONFIG_LSS_MASTER) && defined(CONFIG_LSS_SLAVE) */

#ifdef CONFIG_CAN_SEND
	case CO_COB_DEBUG_RX:
	    can_receive(canMsg CO_COMMA_LINE_PARA);
	    break;
#endif /* CONFIG_CAN_SEND */

#ifdef CONFIG_USER_CAN_MSG
	case CO_COB_USER_RX:
	    usrCanMsgReceived(canMsg CO_COMMA_LINE_PARA);
	    break;
#endif /*CONFIG_USER_CAN_MSG */

#ifdef CO_CONFIG_GFC_CONSUMER
        case CO_COB_GFC_RX:
            gfcMsgReceived(CO_LINE_PARA);
            break;
#endif /* CO_CONFIG_GFC_CONSUMER */

	default:
	    break;
    }
}


#if (defined(CONFIG_NODE_GUARDING) && defined(CONFIG_SLAVE)) \
  || defined(CONFIG_HEARTBEAT_PRODUCER) \
  || defined(CONFIG_PDO_PRODUCER)
/*******************************************************************
*
* rtrRequestReceived
*
* NOMANUAL
*
* If a message is identified as an rtr messages then
* this function will be called
* Here the appropriate function for this event is called
*
* \return
*++ nothing
*-- nichts
*
*/
static void rtrRequestReceived(
	CAN_MSG_T *canMsg	    /* Pointer to CAN Message */
	CO_COMMA_LINE_PARA_DECL	/* number of CAN line 0..CO_MAX_CAN_LINES-1 */
    )
{

#if (defined(CONFIG_NODE_GUARDING) && defined(CONFIG_SLAVE))	\
  || defined(CONFIG_HEARTBEAT_PRODUCER)
    /* CAN line is slave - it is the nodeguarding cob ?*/
    if (canMsg->cobId == (GL_ARRAY(co_Node)).pGuard_COB->cobId) {

	NMT_NodeGuardingMsg(CO_LINE_PARA);
    } else
#endif /* CONFIG_SLAVE && CONFIG_NODE_GUARDING */
    {

#ifdef CONFIG_PDO_PRODUCER
    if ((canMsg->cobType) == CO_COB_PDO_PROD_RTR){
	pdoRtrMsgReceived(canMsg CO_COMMA_LINE_PARA);
    }
#endif /* CONFIG_PDO_PRODUCER */
    }
}

#endif /* (defined(CONFIG_NODE_GUARDING) && defined(CONFIG_SLAVE)) || defined(CONFIG_PDO_PRODUCER) */

/*______________________________________________________________________EOF_*/
