/*
 *++ nmt_s - additional network routines for slaves
 *-- nmt_s - Zusätzliche Netzwerkfunktionalität für Slaves
 *
 * Copyright (c) 2001-2017 port GmbH Halle (Saale)
 *------------------------------------------------------------------
 */


/*
*  \file nmt_s.c
*++ Additional network routines for slaves
*-- Zusätzliche Netzwerkfunktionalität für Slaves
*  \author port GmbH Halle (Saale)
*
*++ Additional network features NMT services are defined in this
*++ module.
*++ They are necessary for the CANopen communication profile DS 301.
*++ In this module functions are defined which influence the state machine
*++ of a CANopen Device.
*++ It defines the behaviour of communication and application reset.
*-- In diesem Modul sind die Routinen für NMT Slave Dienste kodiert.
*-- Sie beeinflussen die Zustandsmachine
*-- der CANopen Slaves.
.LP
*/

/* header of standard C - libraries */

#include <stdio.h>
#include <string.h>

/* header of project specific types */

#include <cal_conf.h>
#include <co_odidx.h>
#include <co_setcp.h>
#include <co_usr.h>
#include "nmt.h"
#include "nmt_s.h"
#include "nmterr.h"
#include "access.h"
#include "drv.h"
#include "pdo.h"
#include "sdo.h"

#if defined(CONFIG_SYNC_CONSUMER) || defined(CONFIG_SYNC_PRODUCER)
# include "sync.h"
#endif /* defined(CONFIG_SYNC_CONSUMER) || defined(CONFIG_SYNC_PRODUCER) */

#if defined(CONFIG_EMCY_PRODUCER) || defined(CONFIG_EMCY_CONSUMER)
# include "emerg.h"
#endif /* defined(CONFIG_EMCY_PRODUCER) || defined(CONFIG_EMCY_CONSUMER) */

#ifdef CONFIG_HEARTBEAT_CONSUMER
# include "heartbt.h"
#endif /* CONFIG_HEARTBEAT_CONSUMER */

#ifdef CONFIG_REDUNDANCY_SUPPORT
# include "reduncy.h"
#endif /* CONFIG_REDUNDANCY_SUPPORT */

#ifdef CONFIG_NON_VOLATILE_MEM
# include <co_stor.h>
#endif /*  CONFIG_NON_VOLATILE_MEM */

#if defined(CONFIG_SRDO_CONSUMER) || defined(CONFIG_SRDO_PRODUCER)
# include "srdo.h"
#endif /* defined(CONFIG_SRDO_CONSUMER) || defined(CONFIG_SRDO_PRODUCER) */

# ifdef CONFIG_FLYING_MASTER
# include "flyma.h"
# endif /* CONFIG_FLYING_MASTER */

# if defined(CONFIG_LSS_MASTER) || defined(CONFIG_LSS_SLAVE)
# include "lss.h"
# endif /* defined(CONFIG_LSS_MASTER) || defined(CONFIG_LSS_SLAVE) */

/* constant definitions
---------------------------------------------------------------------------*/
#ifdef CO_RESET_COMM_USER_FCT
#else
# define CO_RESET_COMM_USER_FCT
#endif

/* local defined data types
---------------------------------------------------------------------------*/

/* list of external used functions, if not in headers
---------------------------------------------------------------------------*/

/* list of global defined functions
---------------------------------------------------------------------------*/

/* list of local defined functions
---------------------------------------------------------------------------*/

/* external variables
---------------------------------------------------------------------------*/

/* global variables
---------------------------------------------------------------------------*/
#ifdef CONFIG_NO_ERROR_BEHAVIOR
#else /* CONFIG_NO_ERROR_BEHAVIOR */
# ifdef CONFIG_NO_GLOBAL_VARS
# else /* CONFIG_NO_GLOBAL_VARS */
CO_LIB_UNINIT_VAR UNSIGNED8	commErrorBehavior CO_LINE_PARA_ARRAY_DEF;
# endif /* CONFIG_NO_GLOBAL_VARS */
#endif /* CONFIG_NO_ERROR_BEHAVIOR */

/* local defined variables
---------------------------------------------------------------------------*/
#ifdef CONFIG_RCS_IDENT
CO_LIB_INIT_VAR static char _rcsid[] = "$Id: nmt_s.c,v 2.43 2016/09/26 11:16:09 rli Exp $";
#endif /* CONFIG_RCS_IDENT */


/*******************************************************************
*
* resetNodeMsg - This function will be indicated at the NMT-Slave.
*
* \internal
*
* It is responsible for the reset of the slave application by calling
* the user defined routine \fIresetApplication()\fP.
* Afterwards the function sets
* the NMT Slave in the state RESET_COMM and calls resetCommInd() the
* user defined function to set the commmunication parameter to their default
* values. The last step is forcing the NMT slave to the state PRE_OPERATIONAL.
*
* \retval
* nothing
*/

void resetNodeMsg(
	CO_REDCY_PARA_DECL
    )
{
#ifdef CONFIG_RESET_APPL_PRE_CMD
    resetApplPreInd(CO_REDCY_PARA);
#endif /* CONFIG_RESET_APPL_PRE_CMD */

    /* reset object dictionary */
    (void)resetObjDir(MEM_SEG_APPL_PARAMETERS CO_COMMA_LINE_PARA);

#ifdef CONFIG_NON_VOLATILE_MEM
    /* load communication values from flash ---------------------------- */
    loadParameterInd(MEM_SEG_ALL_PARAMETERS, CO_RESTORE_MODE_RESETCOMM
	CO_COMMA_LINE_PARA);
#endif /* CONFIG_NON_VOLATILE_MEM */

#ifdef CONFIG_NO_ERROR_BEHAVIOR
#else /* CONFIG_NO_ERROR_BEHAVIOR */
    /* load object 1029:1 if available */
    setupCommErrorBehavior(CO_LINE_PARA);
#endif /* CONFIG_NO_ERROR_BEHAVIOR */

    resetApplInd(CO_REDCY_PARA);

    /* automatically changing to RESET_COMM state */
#ifdef CONFIG_REDUNDANCY_SUPPORT
    GL_ARRAY(co_redcyNode).eState = RESET_COMM;
    GL_ARRAY(co_Node).eState = RESET_COMM;
#else /* CONFIG_REDUNDANCY_SUPPORT */
    GL_ARRAY(co_Node).eState = RESET_COMM;
#endif /* CONFIG_REDUNDANCY_SUPPORT */

#ifdef CONFIG_REDUNDANCY_SUPPORT
    resetCommMsg(CAN_DEFAULT_LINE CO_COMMA_LINE_PARA);
    resetCommMsg(CAN_REDCY_LINE CO_COMMA_LINE_PARA);
#else
    /* reset communication */
    resetCommMsg(CO_REDCY_PARA);
#endif
}


/*******************************************************************
*
* resetCommMsg - This function will be indicated at the NMT-Slave.
*
* \internal
*
* This function react to a reset communication from the NMT-master.
* The reaction of this command depends on the state
* of calling 'restore default parameter' by writing to 0x1011.
* For a complete reset communication (without 0x1011 write)
* it does the following steps:
* - get node id by \fIgetNodeId()\fP call
* - reset all object dictionary entries
* - calls \fIloadParameterInd()\fP
* - calls \fIresetCommInd()\fP
* - updates the library internal communication parameter
*
* For a reset communication with 0x1011 write:
* - calls \fIresetCommInd()\fP
* - updates the library internal communication parameter
*
* Afterwards it will forcing the NMT slave to the state PRE_OPERATIONAL.
*
* \retval
* nothing
*/
void resetCommMsg(
	CO_REDCY_PARA_DECL
     )
{
# if defined(CONFIG_HEARTBEAT_CONSUMER) || (defined(CONFIG_MASTER) && defined(CONFIG_NODE_GUARDING))
UNSIGNED8	i;
# endif /* defined(CONFIG_HEARTBEAT_CONSUMER) || (defined(CONFIG_MASTER) && defined(CONFIG_NODE_GUARDING)) */

#ifdef CO_CONFIG_RESET_COMM_PRE_CMD
    resetCommPreInd(CO_REDCY_PARA);
#endif /* CO_CONFIG_RESET_COMM_PRE_CMD */

    CO_RESET_COMM_USER_FCT;

#ifdef CONFIG_CO_RUN_LED
    updateNMTState_led(CO_REDCY_PARA);
#endif /* CONFIG_CO_RUN_LED */

    CO_RESET_COMM_START;

    /* clear TX/RX buffer */
    CLEAR_TX_BUFFER(CO_REDCY_PARA);
    CLEAR_RX_BUFFER(CO_REDCY_PARA);

    /* calls user function to get the node id from DIP switch or EEPROM */
    GL_ARRAY(coNodeId) = getNodeId(CO_LINE_PARA);

    /* reset object dictionary */
    (void)resetObjDir(MEM_SEG_COM_PARAMETERS CO_COMMA_LINE_PARA);

#ifdef CONFIG_NON_VOLATILE_MEM
    /* load communication values from flash ---------------------------- */
    loadParameterInd(MEM_SEG_COM_PARAMETERS, CO_RESTORE_MODE_RESETCOMM
	CO_COMMA_LINE_PARA);
#endif /* CONFIG_NON_VOLATILE_MEM */

    /* call user indication ----------------------------------------------- */
    resetCommInd(CO_REDCY_PARA);

    /* update internal variable and cob-ids --------------------------------*/

# if defined(CONFIG_HEARTBEAT_CONSUMER) || (defined(CONFIG_MASTER) && defined(CONFIG_NODE_GUARDING))
    /* reset failed nodes */
    for (i = 0; i < NMTERR_MAX_INDEX; i++)  {
#  ifdef CONFIG_REDUNDANCY_SUPPORT
	GL_VAR(nmtErrFailed)[i][0] = 0;
	GL_VAR(nmtErrStarted)[i][0] = 0;
	GL_VAR(nmtErrConfig)[i][0] = 0;
	GL_VAR(nmtErrFailed)[i][1] = 0;
	GL_VAR(nmtErrStarted)[i][1] = 0;
	GL_VAR(nmtErrConfig)[i][1] = 0;
	GL_VAR(nmtErr3HBok)[i] = 0;
#   ifdef CONFIG_MARITIME_SUPPORT
	GL_VAR(nmtErrRedundancy)[i] = 0;
#   endif /* CONFIG_MARITIME_SUPPORT */
#  else /* CONFIG_REDUNDANCY_SUPPORT */
	GL_ARRAY(nmtErrFailed[i]) = 0;
	GL_ARRAY(nmtErrStarted[i]) = 0;
	GL_ARRAY(nmtErrConfig[i]) = 0;
#  endif /* CONFIG_REDUNDANCY_SUPPORT */
    }
# endif /* defined(CONFIG_HEARTBEAT_CONSUMER) || (defined(CONFIG_MASTER) && defined(CONFIG_NODE_GUARDING)) */

# if defined(CONFIG_SYNC_CONSUMER) || defined(CONFIG_SYNC_PRODUCER)
    /* setCommPar(SYNC_COB_ID_INDEX, 0 CO_COMMA_LINE_PARA); */
    (void) initSync(CO_LINE_PARA);
# endif /* defined (CONFIG_SYNC_CONSUMER) */

#ifdef xxx
/* ??? Time Producer ???? */
#endif

# if defined (CONFIG_TIME_CONSUMER)
    (void) setCommPar(TIME_COB_ID_INDEX, 0 CO_COMMA_LINE_PARA);
# endif /* defined (CONFIG_TIME_CONSUMER) || defined (CONFIG_TIME_CONSUMER) */

# ifdef CONFIG_EMCY_PRODUCER
    (void) initEmcy(PRODUCER CO_COMMA_LINE_PARA);
# endif /* CONFIG_EMCY_PRODUCER */

# ifdef CONFIG_EMCY_CONSUMER
    (void) initEmcy(CONSUMER CO_COMMA_LINE_PARA);
# endif /* CONFIG_EMCY_CONSUMER */

# if defined(CONFIG_PDO_CONSUMER) || defined(CONFIG_PDO_PRODUCER)
    resetAllPdos(CO_LINE_PARA);
# endif /* definded(CONFIG_PDO_CONSUMER) || defined(CONFIG_PDO_PRODUCER) */

    /* set SDO Para */
# if defined(CONFIG_SDO_SERVER) || defined(CONFIG_SDO_CLIENT)
    resetAllSdos(CO_LINE_PARA);
# endif /* defined(CONFIG_SDO_SERVER) || defined(CONFIG_SDO_CLIENT) */

# if defined(CONFIG_SRDO_PRODUCER) || defined(CONFIG_SRDO_CONSUMER)
    resetAllSrdos(CO_LINE_PARA);
# endif /*  defined(CONFIG_SRDO_PRODUCER) || defined(CONFIG_SRDO_CONSUMER) */

# ifdef CONFIG_HEARTBEAT_CONSUMER
    (void) defineHeartbeatConsumer(CO_LINE_PARA);
# endif  /* CONFIG_HEARTBEAT_CONSUMER */

# ifdef CONFIG_REDUNDANCY_SUPPORT
    initRedundancy(CO_LINE_PARA);
# endif /* CONFIG_REDUNDANCY_SUPPORT */

# ifdef CONFIG_FLYING_MASTER
    (void) initFlyingMaster(CO_LINE_PARA);
# endif /* CONFIG_FLYING_MASTER */

# if defined(CONFIG_LSS_MASTER) || defined(CONFIG_LSS_SLAVE)
    initLss(CO_LINE_PARA);
# endif /* defined(CONFIG_LSS_MASTER) || defined(CONFIG_LSS_SLAVE) */

# if defined(CONFIG_MASTER) && defined(CONFIG_NMT_STARTUP_MANAGER)
    defineNmtStartup(CO_REDCY_PARA);
# endif /* defined(CONFIG_MASTER) && defined(CONFIG_NMT_STARTUP_MANAGER) */

    /* init NMT error variables */
    (void)initNmtErr(CO_LINE_PARA);

# ifndef CO_CONFIG_DONT_AUTOSEND_BOOTUP
    /* setup nmt state machine */
    (void)initNmtState(CO_REDCY_PARA);
# endif /* CO_CONFIG_DONT_AUTOSEND_BOOTUP */
        /* in case of CO_CONFIG_DONT_AUTOSEND_BOOTUP, the application then has to call initNmtState manually when ready */

#ifdef CO_CONFIG_RESET_COMM_POST_CMD
    resetCommPostInd(CO_REDCY_PARA);
#endif /* CO_CONFIG_RESET_COMM_POST_CMD */

    CO_RESET_COMM_END;
}


#ifdef CONFIG_NO_ERROR_BEHAVIOR
#else /* CONFIG_NO_ERROR_BEHAVIOR */
/*******************************************************************
*
* setupCommErrorBehavior - setup communication error behavior
*
* \internal
*
* This function read the object 1029 if exist
* and save the error behavior on internal variable.
*
* If the object 1029 doesn't exist,
* set the error behavior to CHANGE TO PREOP
*
* \retval
* nothing
*/
void setupCommErrorBehavior(
	CO_LINE_PARA_DECL
    )
{
UNSIGNED8	*pErrBehavior;
UNSIGNED32	size;
RET_T		ret;

    /* get the address of error behavior */
    ret = getObjAddr(ERROR_BEHAVIOR_INDEX, 1u, &pErrBehavior, &size CO_COMMA_LINE_PARA);
    if (ret == CO_OK) {
	GL_ARRAY(commErrorBehavior) = *pErrBehavior;

    } else {

        /* set default behavior */
        GL_ARRAY(commErrorBehavior) = EB_CHANGE_TO_PREOP;
    }
}


/*******************************************************************
*
* execCommErrorBehavior - eval communication error behavior
*
* \internal
*
* This function react on the error behavior for communication errors
* depending on the state of object 1029.
*
* \retval
* nothing
*/
void execCommErrorBehavior(
	CO_REDCY_PARA_DECL
    )
{
    /* switch only, if actual state is OPERATIONAL */
    if (getNodeState(CO_REDCY_PARA) == OPERATIONAL)  {

    switch (GL_ARRAY(commErrorBehavior)) {
	case EB_CHANGE_TO_PREOP:
	    setNodeState(PRE_OPERATIONAL CO_COMMA_REDCY_PARA);
	    break;

	case EB_CHANGE_TO_STOP:
	    setNodeState(STOPPED CO_COMMA_REDCY_PARA);
            break;
	case EB_CHANGE_NONE:
	default:
	    /* don't change */
	    break;
    }
    }
}

#endif /* CONFIG_NO_ERROR_BEHAVIOR */

/*______________________________________________________________________EOF_*/
