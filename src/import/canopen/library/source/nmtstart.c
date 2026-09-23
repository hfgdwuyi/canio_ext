/*
 *++ nmtstart - NMT Startup Manager
 *-- nmtstart - NMT Startup Manager
 *
 * Copyright (c) 2006-2017 port GmbH Halle (Saale)
 *------------------------------------------------------------------
 */


/****************************************************************************/
/**
*  \file nmtstart.c
*++ Contains service routines for NMT startup manager
*-- Beinhaltet Funktionen für NMT Startup Manager
*  \author port GmbH Halle (Saale)
*
*++ This module contains functions for the NMT Startup Manager.
*++ according to DSP-302-2, V4.0.
*++ The NMT Startup Manager must be the NMT master in the network
*++ and does not integrate any Flying master functionality
*++ or any LSS service.
*++ During the NMT Startup process of the network the content of
*++ the objects 0x1F80 - 0x1F91 should be constant.
*++ This module does not support the verification of the slave software
*++ and the automatic update of the slave software.
*++ Each connected slave has to support Heartbeat or Node Guarding.
*-- Dieses Modul enthält Funktionen für einen NMT Startup Manager
*-- nach dem DSP-302-2, V4.0.
*-- Der NMT Startup Manager muss der NMT Master im Netzwerk sein.
*-- Er unterstützt zur Zeit keine Flying Master Funktionalität
*-- und keinen LSS Service.
*-- Während des NMT Startup Prozesses muß der Dateninhalt der
*-- Objekte 0x1F80 - 0x1F91 konstant sein.
*-- Dieses Modul unterstützt nicht die Prüfung und automatische
*-- Aktualisierung der Slave Software.
*-- Die eingetragenenen Slaves müssen Heartbeat oder Node Guarding
*-- unterstützen.
*
* \par
*++ All defined functions are only valid on a master device.
*-- Alle definierten Funktionen sind nur für Masterapplikationen gültig.
*/

/* header of standard C - libraries */

#include <string.h>
#include <stdio.h>

#ifdef NMTSTARTUP_DEBUG
#  include <stdarg.h>
#endif /* NMTSTARTUP_DEBUG */
/* header of project specific types */

#include <cal_conf.h>
#include <co_odidx.h>
#include <co_cobid.h>
#include <co_flag.h>
#include <co_nmt_m.h>
#include <co_guard.h>
#include <co_setcp.h>
#include "access.h"
#include "sdo.h"
#include "heartbt.h"
#include "drv.h"
#include "utility.h"
#include "nmt_m.h"
#include "nmterr.h"
#include "nmtstart.h"

/* constant definitions
---------------------------------------------------------------------------*/
#define CONFIG_NMT_STARTUP_SLAVE_CNT	CONFIG_NMT_SLAVE_CNT

#ifndef CONFIG_NMT_STARTUP_RESETCOMM_TIMEOUT
# define CONFIG_NMT_STARTUP_RESETCOMM_TIMEOUT	50000
#endif /* CONFIG_NMT_STARTUP_RESETCOMM_TIMEOUT */

/* This defines is the CiA 302 cycle time for the SDO retries */
#ifndef CO_CONFIG_NMTSTART_CYCL_TIME
# define CO_CONFIG_NMTSTART_CYCL_TIME 10000
# endif /* CO_CONFIG_NMTSTART_CYCL_TIME*/



#ifdef NMTSTARTUP_DEBUG
#define MY_PRINTF	debug_print
/* debug levels */
#define	DBGLVL_MASTER	0x1		/* master loop */
#define	DBGLVL_SLAVE	0x2		/* slave loop */
#define	DBGLVL_DEVTYPE	0x4
#define	DBGLVL_VENDOR	0x8
#define	DBGLVL_RSTCOMM	0x10
#define DBGLVL_SWUPDATE	0x20
#define DBGLVL_CFG	0x40
#define DBGLVL_ERRCTRL	0x80
#define	DBGLVL_NMT	0x100
#define	DBGLVL_EVENT	0x200
#endif /* NMTSTARTUP_DEBUG */

#ifdef CONFIG_DYN_MEM_ALLOC
# define NMT_STARTUP_SLAVE_CNT		co_maxNmtStartupSlaves
#else /* CONFIG_DYN_MEM_ALLOC */
# define NMT_STARTUP_SLAVE_CNT		CONFIG_NMT_STARTUP_SLAVE_CNT
#endif /* CONFIG_DYN_MEM_ALLOC */

/* local defined data types
---------------------------------------------------------------------------*/

/* list of external used functions, if not in headers
---------------------------------------------------------------------------*/

/* list of global defined functions
---------------------------------------------------------------------------*/

/* list of local defined functions
---------------------------------------------------------------------------*/
#if defined(CONFIG_MASTER) && defined(CONFIG_NMT_STARTUP_MANAGER)
static UNSIGNED8 nmtStartupResetComm(CO_REDCY_PARA_DECL);
static UNSIGNED8 nmtStartBootTimer(CO_LINE_PARA_DECL);
static UNSIGNED8 nmtStartupStartSlaves(BOOL_T CO_COMMA_REDCY_PARA_DECL);
static UNSIGNED8 getNmtStartupSlaveIndex(UNSIGNED8 nodeId
	CO_COMMA_LINE_PARA_DECL);
static UNSIGNED8 nmtStartMaster(CO_REDCY_PARA_DECL);
static UNSIGNED8 nmtStartAllNodes(CO_REDCY_PARA_DECL);
static UNSIGNED8 nmtsSResetComm(UNSIGNED16 CO_COMMA_REDCY_PARA_DECL);
static UNSIGNED8 nmtsSUpdateSoftware(UNSIGNED16 CO_COMMA_LINE_PARA_DECL);
static UNSIGNED8 nmtsSConfigureSlave(UNSIGNED16 CO_COMMA_LINE_PARA_DECL);
static UNSIGNED8 nmtsSStartErrorControl(UNSIGNED16 CO_COMMA_LINE_PARA_DECL);
static UNSIGNED8 nmtsStartSlave(UNSIGNED16 CO_COMMA_REDCY_PARA_DECL);
static UNSIGNED8 nmtsSRequestDeviceType(UNSIGNED16 CO_COMMA_LINE_PARA_DECL);
static UNSIGNED8 nmtsSCheckDeviceType(UNSIGNED16 CO_COMMA_LINE_PARA_DECL);
static UNSIGNED8 nmtsSRequestVendorId(UNSIGNED16 CO_COMMA_LINE_PARA_DECL);
static UNSIGNED8 nmtsSCheckVendorId(UNSIGNED16 CO_COMMA_LINE_PARA_DECL);
static UNSIGNED8 nmtsSRequestProductCode(UNSIGNED16 CO_COMMA_LINE_PARA_DECL);
static UNSIGNED8 nmtsSCheckProductCode(UNSIGNED16 CO_COMMA_LINE_PARA_DECL);
static UNSIGNED8 nmtsSRequestRevision(UNSIGNED16 sIdx CO_COMMA_LINE_PARA_DECL);
static UNSIGNED8 nmtsSCheckRevision(UNSIGNED16 CO_COMMA_LINE_PARA_DECL);
static UNSIGNED8 nmtsSRequestSerialNumber(UNSIGNED16 CO_COMMA_LINE_PARA_DECL);
static UNSIGNED8 nmtsSCheckSerialNumber(UNSIGNED16 CO_COMMA_LINE_PARA_DECL);
static UNSIGNED8 nmtsSStartNode(UNSIGNED16 CO_COMMA_REDCY_PARA_DECL);
static UNSIGNED8 nmtsSUpdateConfigSlave(UNSIGNED16 CO_COMMA_LINE_PARA_DECL);
static UNSIGNED8 nmtsSCheckConfigSlave1(UNSIGNED16 idx CO_COMMA_LINE_PARA_DECL);
static UNSIGNED8 nmtsSCheckConfigSlave2(UNSIGNED16 idx CO_COMMA_LINE_PARA_DECL);
static UNSIGNED8 nmtsSFinishedOk(UNSIGNED16 idx CO_COMMA_LINE_PARA_DECL);
static void nmtsErrorHandler(UNSIGNED8 nodeId CO_COMMA_REDCY_PARA_DECL);
static RET_T readSdo(UNSIGNED16 sidx, UNSIGNED16 index, UNSIGNED8 subIndex,
	UNSIGNED8 *buf, UNSIGNED8 len CO_COMMA_LINE_PARA_DECL);
static void restartBootNmtSlave(UNSIGNED16 idx, UNSIGNED8 typ
	CO_COMMA_REDCY_PARA_DECL);

/* helper functions */
static RET_T pcoNmtStartCheckSetSdoCob( SDO_CLIENT_T *pCSdo, COB_KIND_T cobType,
		UNSIGNED32 newCob, UNSIGNED8  sdoNr CO_COMMA_LINE_PARA_DECL );



# ifdef NMTSTARTUP_DEBUG
/* debug function */
static void debug_print(UNSIGNED16 level, char *pfmt, ...);
static char *getCmdStrg(UNSIGNED8);
# endif /* NMTSTARTUP_DEBUG */
#endif /* CONFIG_NMT_STARTUP_MANAGER */

/* external variables
---------------------------------------------------------------------------*/

/* global variables
---------------------------------------------------------------------------*/
#ifdef CONFIG_NO_GLOBAL_VARS
#else /* CONFIG_NO_GLOBAL_VARS */

# if defined(CONFIG_MASTER) && defined(CONFIG_NMT_STARTUP_MANAGER)
CO_LIB_UNINIT_VAR NMTS_MASTER_STARTUP_T	nmtsMaster CO_LINE_PARA_ARRAY_DEF;
#  ifdef CONFIG_DYN_MEM_ALLOC
CO_LIB_UNINIT_VAR NMT_SLAVE_STARTUP_T	*p_nmtStartupSlave[1];
CO_LIB_UNINIT_VAR UNSIGNED16		co_maxNmtStartupSlaves;
#  else /* CONFIG_DYN_MEM_ALLOC */
CO_LIB_UNINIT_VAR NMT_SLAVE_STARTUP_T	nmtStartupSlave[CONFIG_NMT_STARTUP_SLAVE_CNT];
#  endif /* CONFIG_DYN_MEM_ALLOC */

# endif /* defined(CONFIG_MASTER) && defined(CONFIG_NMT_STARTUP_MANAGER) */
#endif /* CONFIG_NO_GLOBAL_VARS */

/* local defined variables
---------------------------------------------------------------------------*/
#ifdef CONFIG_RCS_IDENT
CO_LIB_INIT_VAR static char _rcsid[] = "$Id: nmtstart.c,v 2.46 2016/11/17 16:31:19 rli Exp $";
#endif /* CONFIG_RCS_IDENT */

#ifdef CONFIG_NO_GLOBAL_VARS
#else /* CONFIG_NO_GLOBAL_VARS */

# if defined(CONFIG_MASTER) && defined(CONFIG_NMT_STARTUP_MANAGER)
#  ifdef CONFIG_FAST_SORT
#   ifdef CONFIG_DYN_MEM_ALLOC
CO_LIB_UNINIT_VAR UNSIGNED8	*p_nmtStartupSlaveIdxList[1];
#   else /* CONFIG_DYN_MEM_ALLOC */
CO_LIB_UNINIT_VAR static UNSIGNED8	nmtStartupSlaveIdxList[CONFIG_NMT_STARTUP_SLAVE_CNT];
#   endif /* CONFIG_DYN_MEM_ALLOC */
#  endif /* CONFIG_FAST_SORT */

#  ifdef NMTSTARTUP_DEBUG
CO_LIB_INIT_VAR static UNSIGNED32 debugLevel = 0
#ifdef xxx
	| DBGLVL_DEVTYPE
	| DBGLVL_VENDOR
	| DBGLVL_SWUPDATE
	| DBGLVL_MASTER
	| DBGLVL_RSTCOMM
	| DBGLVL_CFG
	| DBGLVL_SLAVE
#else
	| DBGLVL_ERRCTRL
	| DBGLVL_EVENT
#endif
	;
#  endif /* NMTSTARTUP_DEBUG */
# endif /* defined(CONFIG_MASTER) && defined(CONFIG_NMT_STARTUP_MANAGER) */
#endif /* CONFIG_NO_GLOBAL_VARS */


#if defined(CONFIG_MASTER) && defined(CONFIG_NMT_STARTUP_MANAGER)

/****************************************************************************/
/**
*++ \brief defineNmtStartup - initialize the NMT Startup process
*-- \brief defineNmtStartup - initialisiert den NMT Startup Prozess
*
* \retval CO_OK
*++ success
*-- Erfolg
* \retval CO_E_NO_MASTER
*++ device is not a NMT master (see object 0x1F80, bit 0)
*-- das Gerät ist kein NMT Master (siehe Objekt 0x1F80, Bit 0)
* \retval CO_E_NO_ACCESS
*++ no access to object 0x1F80
*-- Kein Zugriff auf das Objekt 0x1F80
* \retval CO_E_BAD_SERVICE
*++ configured functionality is not supported
*-- die gewünschte Funktionalität wird nicht unterstützt
*/
RET_T defineNmtStartup(
	CO_REDCY_PARA_DECL	/**< number of CAN line 0..CO_MAX_CAN_LINES-1 */
    )
{
UNSIGNED32 size;	/* current size of data in bytes */
UNSIGNED8 i;		/* slave index */
UNSIGNED16 sIdx;	/* slave index in nmtSlaveList */
UNSIGNED32 objVal;	/* object value */
RET_T ret;		/* return value */
OBJDIR_T *pObj = NULL;  /* pointer to object */

UNSIGNED8	slaveCnt;

    /*--- initialize global variables for master ---*/
    GL_ARRAY(nmtsMaster).fct = NMTS_MFCT_STANDBY;

# ifdef CONFIG_MULT_LINES
    GL_ARRAY(nmtsMaster).slaveCnt = co_nmtSlaveLineCnts[CO_LINE_PARA];
# else
    GL_ARRAY(nmtsMaster).slaveCnt = NMT_STARTUP_SLAVE_CNT;
# endif

    /* values are updated for each slave */
    GL_ARRAY(nmtsMaster).mandatorySlaveCnt = 0;
    GL_ARRAY(nmtsMaster).optionalSlaveCnt = 0;
    GL_ARRAY(nmtsMaster).globalRstCommAllowed = 0;

    /* get NMT master object */
    ret = getObjEntry(NMT_MASTER_INDEX, 0, (UNSIGNED8 *)&objVal, &size,
	CO_TRUE CO_COMMA_LINE_PARA);
    if (ret != CO_OK) {
	return (ret);
    }

    /* setup internal variables */
    ret = setNmtStartupPara(NMT_MASTER_INDEX, 0, objVal CO_COMMA_REDCY_PARA);
    if (ret != CO_OK) {
	return (ret);
    }

    /* bootup time */
    ret = getObjEntry(NMT_BOOT_TIME_INDEX, 0, (UNSIGNED8 *)&objVal, &size,
	CO_TRUE CO_COMMA_LINE_PARA);
    if (ret != CO_OK) {
	return (ret);
    }

    /* setup internal variables */
    ret = setNmtStartupPara(NMT_BOOT_TIME_INDEX, 0, objVal CO_COMMA_REDCY_PARA);
    if (ret != CO_OK) {
	return (ret);
    }

    /*--- initialize global variables for slave managing ---*/
    for (i = 0; i < GL_ARRAY(nmtsMaster).slaveCnt; i++)
    {
	sIdx = i
#ifdef CONFIG_MULT_LINES
	    + GL_ARRAY(co_nmtSlaveLineOffs)
#endif /* CONFIG_MULT_LINES */
	    ;

	GL_PVAR(nmtStartupSlave)[sIdx].nodeId = 0;
	GL_PVAR(nmtStartupSlave)[sIdx].typ = NMTSLAVE_TYPE_UNKNOWN;
	GL_PVAR(nmtStartupSlave)[sIdx].fct = NMTS_SFCT_NO;
	GL_PVAR(nmtStartupSlave)[sIdx].flags = 0;
    }

    /* setup slave settings from OD */
    ret = getObjPtrAtIndex(NMT_SLAVE_ASSIGNMENT_INDEX, &pObj
		CO_COMMA_LINE_PARA);
    if ( CO_OK != ret ) {
        return ret;
    }


    /* get number of slave count from od */
    ret = getObjPtrEntry(pObj, NMT_SLAVE_ASSIGNMENT_INDEX, 0, &slaveCnt, &size,
	    CO_TRUE CO_COMMA_LINE_PARA);
    if (ret != CO_OK) {
	return(ret);
    }

    for (i = 1; i <= slaveCnt; i++)  {

	/* get od value */
	ret = getObjPtrEntry(pObj,NMT_SLAVE_ASSIGNMENT_INDEX, i, (UNSIGNED8 *)&objVal,
		&size, CO_TRUE CO_COMMA_LINE_PARA);
	if (ret != CO_OK) {
	    return (CO_E_NO_ACCESS);
	}
	ret = setNmtStartupPara(NMT_SLAVE_ASSIGNMENT_INDEX, i, objVal
		CO_COMMA_REDCY_PARA);
	if (ret != CO_OK) {
	    return (ret);
	}
    }

    return(CO_OK);
}


/****************************************************************************/
/**
*++ \brief nmtStartupReq - start the NMT Startup process
*-- \brief nmtStartupReq - start den NMT Startup Prozess
*
* \retval CO_OK
*++ success
*-- Erfolg
* \retval CO_E_NO_MASTER
*/

RET_T nmtStartupReq(
	CO_REDCY_PARA_DECL	/**< number of CAN line 0..CO_MAX_CAN_LINES-1 */
    )
{
UNSIGNED8	i;
UNSIGNED16	idx;
RET_T		ret;
#ifdef CO_CONFIG_NMTSTART_INIT_SDO_REQ
UNSIGNED16      sdoNr = 0;
SDO_CLIENT_T   *pCSdo = NULL;
#endif /* CO_CONFIG_NMTSTART_INIT_SDO_REQ */

    /* if startup master bit is set */
    if ((GL_ARRAY(nmtsMaster).obj1F80 & NMT_STARTUP_MASTER_BIT) == 0)  {
	return(CO_E_BAD_SERVICE);
    }

    /*--- start the execution of the NMT Startup process ---*/
    GL_ARRAY(nmtsMaster).si = 0;
    GL_ARRAY(nmtsMaster).mandatorySlaveFinish = 0;
    GL_ARRAY(nmtsMaster).mandatorySlaveFail = 0;
    GL_ARRAY(nmtsMaster).optionalSlaveFinish = 0;

    for (i = 0; i < GL_ARRAY(nmtsMaster).slaveCnt; i++)  {

	idx = i;
#ifdef CONFIG_MULT_LINES
	idx += GL_ARRAY(co_nmtSlaveLineOffs);
#endif /* CONFIG_MULT_LINES */

	if (GL_PVAR(nmtStartupSlave)[idx].nodeId != 0)  {
	    ret = setNmtStartupPara(NMT_SLAVE_ASSIGNMENT_INDEX,
		    GL_PVAR(nmtStartupSlave)[idx].nodeId,
		    GL_PVAR(nmtStartupSlave)[idx].obj1F81
		    CO_COMMA_REDCY_PARA);
	    if (ret != CO_OK) {
		return (ret);
	    }
#ifdef CO_CONFIG_NMTSTART_INIT_SDO_REQ
            pCSdo = searchForClientSdoNr(sdoNr + 1 CO_COMMA_LINE_PARA);
            if ( NULL != pCSdo ) {
                if ( SDOSTATE_READY != pCSdo->sdo.state) {
                     if ( SDOSTATE_DISABLED != pCSdo->sdo.state) {
                         continue;
                     }
                }

                /* configure the sdo if necessary */
                (void)pcoNmtStartCheckSetSdoCob( pCSdo, CO_COB_SDO_TX,
		    CO_COBID_CSDO + GL_PVAR(nmtStartupSlave)[idx].nodeId, sdoNr
                    CO_COMMA_LINE_PARA );

                /* configure the sdo if necessary */
                (void)pcoNmtStartCheckSetSdoCob( pCSdo, CO_COB_SDO_RX,
		    CO_COBID_SSDO + GL_PVAR(nmtStartupSlave)[idx].nodeId, sdoNr
                    CO_COMMA_LINE_PARA );

                sdoNr++;

            }
#endif /* CO_CONFIG_NMTSTART_INIT_SDO_REQ */
	}
    }

    GL_ARRAY(nmtsMaster).fct = NMTS_MFCT_RESET_COMM;
    SET_COLIB_FLAG(COFLAG_NMT_STARTUP_MANAGER);

    return (CO_OK);
}


/****************************************************************************/
/**
* setNmtStartupPara - set NMT Startup process variables
*
* \internal
* This function updates the internal variables for the NMT Startup process.
* This function is called by set_comm.c/setCommPar().
*
* \retval CO_OK
* success
* \retval CO_E_NO_MASTER
* device is not a NMT master (see object 0x1F80, bit 0)
* \retval CO_E_BAD_SERVICE
* configured functionality is not supported
* \retval CO_E_NOT_EXIST
* NMT slave does not exist
*/
RET_T setNmtStartupPara(
	UNSIGNED16 index,	/* index of the object */
	UNSIGNED8 subIndex,	/* subindex of the object */
	UNSIGNED32 objVal	/* new object value */
	CO_COMMA_REDCY_PARA_DECL
    )
{
UNSIGNED8	i;
UNSIGNED16	idx;	        /* index in the nmtSlaveList */
NMT_SLAVE_STARTUP_T	*pSlave = NULL;

    /*--- object 0x1F80 ---*/
    if (index == NMT_MASTER_INDEX)
    {
	/* bit 0 */
	/* The node is no longer the NMT master.
	 * Stop the NMT Startup process. */
	if ((objVal & NMT_STARTUP_MASTER_BIT) == 0)
	{
	    GL_ARRAY(nmtsMaster).fct = NMTS_MFCT_STOPPED;
	    RESET_COLIB_FLAG(COFLAG_NMT_STARTUP_MANAGER);

	    nmtStartupNetworkInd(NMT_NETWORK_STARTUP_STOPPED CO_COMMA_LINE_PARA);
	    return (CO_E_NO_MASTER);
	}

	/* bit 5 */
	/* The NMT Startup process does not support these functionalities.
	 * The required value of each bit is 0. */
	else if ((objVal & NMT_STARTUP_FLYING_MASTER_BIT) != 0)
	{
	    return (CO_E_BAD_SERVICE);
	}

	/* update the internal variable */
	GL_ARRAY(nmtsMaster).obj1F80 = objVal;
    }

    /*--- object 0x1F81 ---*/
    else if (index == NMT_SLAVE_ASSIGNMENT_INDEX)  {
	/* subindex 0 isn't writable */
	/* search index in the nmtSlaveList */
	i = getNmtStartupSlaveIndex(subIndex CO_COMMA_LINE_PARA);
        if (i != 0xFF) {

	    idx = i;
# ifdef CONFIG_MULT_LINES
	    idx += GL_ARRAY(co_nmtSlaveLineOffs);
# endif /* CONFIG_MULT_LINES */

	    /* entry found, delete all entries */
	    pSlave = &GL_PVAR(nmtStartupSlave)[idx];
	    removeTimerEvent(&pSlave->timer CO_COMMA_LINE_PARA);
	    pSlave->nodeId = 0;
	    if (pSlave->typ == NMTSLAVE_TYPE_MANDATORY)  {
		GL_ARRAY(nmtsMaster).mandatorySlaveCnt --;
	    } else if (pSlave->typ == NMTSLAVE_TYPE_OPTIONAL)  {
		GL_ARRAY(nmtsMaster).optionalSlaveCnt --;
	    }
	    pSlave->typ = NMTSLAVE_TYPE_UNKNOWN;
	    pSlave->fct = NMTS_SFCT_NO;
	    pSlave->flags = 0;
	    if ((pSlave->obj1F81 & NMT_RESET_COMMUNICATION_BIT) != 0)  {
		GL_ARRAY(nmtsMaster).globalRstCommAllowed --;
	    }
	} else {
	    if ((objVal & NMT_SLAVE_BIT) == 0)  {
		return(CO_OK);
	    }

	    /* node-id not found in nmtSlaveList */
	    /* search for free entry at list */
	    i = 0;
	    while (i < GL_ARRAY(nmtsMaster).slaveCnt)  {
		idx = i;
#ifdef CONFIG_MULT_LINES
		idx += GL_ARRAY(co_nmtSlaveLineOffs);
#endif /* CONFIG_MULT_LINES */

		if (GL_PVAR(nmtStartupSlave)[idx].nodeId == 0)  {
		    pSlave = &GL_PVAR(nmtStartupSlave)[idx];
		    break;
		}
		i++;
	    }

	    if (i == GL_ARRAY(nmtsMaster).slaveCnt)  {
		return (CO_E_NOT_EXIST);
	    }
        }

	/* slave present in network ? */
	if ((objVal & NMT_SLAVE_BIT) == 0)  {
	    return(CO_OK);
	}

	/* set node id */
	pSlave->nodeId = subIndex;

	/* boot this node ? */
	if ((objVal & NMT_BOOT_SLAVE_BIT) != 0)  {
	    /* mandatory or optional slave ? */
	    if ((objVal & NMT_MANDATORY_BIT) != 0)  {
		pSlave->typ = NMTSLAVE_TYPE_MANDATORY;
		GL_ARRAY(nmtsMaster).mandatorySlaveCnt ++;
	    } else {
		pSlave->typ = NMTSLAVE_TYPE_OPTIONAL;
		GL_ARRAY(nmtsMaster).optionalSlaveCnt ++;
	    }
	    pSlave->fct = NMTS_SFCT_START;
	} else {
	    pSlave->typ = NMTSLAVE_TYPE_SLAVE;
	    pSlave->fct = NMTS_SFCT_START_ERROR_CONTROL;
	}

	pSlave->flags = 0;

	if ((objVal & NMT_RESET_COMMUNICATION_BIT) != 0)  {
	    GL_ARRAY(nmtsMaster).globalRstCommAllowed ++;
	}

	/* update the internal variable */
        pSlave->obj1F81 = objVal;

#  ifdef CONFIG_FAST_SORT
	/* sort node-id list */
	sortNodeIdList(
#   ifdef CONFIG_MULT_LINES
	    &GL_PVAR(nmtStartupSlaveIdxList)[GL_ARRAY(co_nmtSlaveLineOffs)],
	    &GL_PVAR(nmtStartupSlave)[GL_ARRAY(co_nmtSlaveLineOffs)].nodeId,
#   else /* CONFIG_MULT_LINES */
	    GL_PVAR(nmtStartupSlaveIdxList),
	    &GL_PVAR(nmtStartupSlave)[0].nodeId,
#   endif /* CONFIG_MULT_LINES */
	    sizeof(NMT_SLAVE_STARTUP_T), GL_ARRAY(nmtsMaster).slaveCnt);
#  endif /* CONFIG_FAST_SORT */

#  ifdef CONFIG_NODE_GUARDING
	/* if nodeguarding is configured, setup it */
	if (((objVal & 0xffff0000) != 0) && ((objVal & 0x0000ff00) != 0)) {
	    if (setGuardTimePara(pSlave->nodeId,
		    (UNSIGNED16)(objVal >> 16),
		    (UNSIGNED8)((objVal >> 8) & 0xff)
		    CO_COMMA_LINE_PARA) != CO_OK)  {
		/* not found in guarging list, try to append it */
		if (addGuardingSlave(pSlave->nodeId,
			(UNSIGNED16)(objVal >> 16),
			(UNSIGNED8)((objVal >> 8) & 0xff)
			CO_COMMA_LINE_PARA) != CO_OK)  {
		    /* not possible - return error */
		    return(CO_E_MEM);
		}
	    }
	}
#  endif /* CONFIG_NODE_GUARDING */
    }

    else if (index == NMT_BOOT_TIME_INDEX) {
	GL_ARRAY(nmtsMaster).bootTime = objVal * 10;
    }

    return (CO_OK);
}

/*==========================================================================*/
/* NMT STARTUP MASTER-SPECIFIC FUNCTIONS                                    */
/*==========================================================================*/

/****************************************************************************/
/**
*++ \brief nmtStartupProcess - process the NMT startup
*-- \brief nmtStartupProcess - fährt das NMT Startup aus
*
*++ This function starts and monitors the whole network
*++ and re-starts optional slaves after rebooting.
*-- Diese Funktion startet und überwacht das gesamte Netzwerk
*-- und startet optionale Slaves nach Ausfall erneut.
*
* \return
*++ nothing
*-- keine
*/
void nmtStartupProcess(
	CO_REDCY_PARA_DECL	/* number of CAN line 0..CO_MAX_CAN_LINES-1 */
    )
{
UNSIGNED8	ret = NMT_RET_ERROR;

    /* return values for all functions are defined as:
     * NMT_RET_OK		- ok, switch to next state
     * NMT_RET_BUSY	- ok, stay at the current state
     * NMT_RET_ERROR	- error, finish master bootup
     */
    switch (GL_ARRAY(nmtsMaster).fct)
    {
	/* generate Reset Communication */
	case NMTS_MFCT_RESET_COMM:

#ifdef NMTSTARTUP_DEBUG
	    MY_PRINTF(DBGLVL_MASTER, "MASTER_LOOP: ResetComm\n");
#endif /* NMTSTARTUP_DEBUG */

	    ret = nmtStartupResetComm(CO_REDCY_PARA);
	    if (ret == NMT_RET_OK)  {
		GL_ARRAY(nmtsMaster).fct = NMTS_MFCT_START_BOOT_TIMER;
	    }
	    break;

	/* start boot time monitoring */
	case NMTS_MFCT_START_BOOT_TIMER:

#ifdef NMTSTARTUP_DEBUG
	    MY_PRINTF(DBGLVL_MASTER, "MASTER_LOOP: startBootTimer\n");
#endif /* NMTSTARTUP_DEBUG */

	    ret = nmtStartBootTimer(CO_LINE_PARA);
	    if (ret == NMT_RET_OK)  {
		GL_ARRAY(nmtsMaster).fct = NMTS_MFCT_START_SLAVES;
	    }
	    break;

	/* startup NMT slaves */
	case NMTS_MFCT_START_SLAVES:

#ifdef NMTSTARTUP_DEBUG
	    MY_PRINTF(DBGLVL_MASTER, "MASTER_LOOP: startupSlaves\n");
#endif /* NMTSTARTUP_DEBUG */

	    ret = nmtStartupStartSlaves(CO_TRUE CO_COMMA_REDCY_PARA);
	    /* startup slaves finished ? */
	    if (ret == NMT_RET_OK)  {
		/* inform application */
		nmtStartupNetworkInd(NMT_NETWORK_ALL_SLAVES_BOOTED
			CO_COMMA_LINE_PARA);
		GL_ARRAY(nmtsMaster).fct = NMTS_MFCT_START_MASTER;
	    }
	    /* startup slaves are waiting for timer */
	    else if (ret == NMT_RET_WAIT4TIMER)  {
		/* disable startup manager calls */
		ret = NMT_RET_ERROR;
	    }
	    /* mandatory slave error */
	    else if (ret == NMT_SLAVE_MANDATORY_ERROR) {
		/* disable boot timer */
		removeTimerEvent(&GL_ARRAY(nmtsMaster).bootTimer
                        CO_COMMA_LINE_PARA);
		/* inform application */
		nmtStartupNetworkInd(NMT_NETWORK_STARTUP_STOPPED
				CO_COMMA_LINE_PARA);
		/* stop boot process */
		GL_ARRAY(nmtsMaster).fct = NMTS_MFCT_STOPPED;
	    }
	    break;

	/* start NMT master */
	case NMTS_MFCT_START_MASTER:

#ifdef NMTSTARTUP_DEBUG
	    MY_PRINTF(DBGLVL_MASTER, "MASTER_LOOP: startMaster\n");
#endif /* NMTSTARTUP_DEBUG */

	    ret = nmtStartMaster(CO_REDCY_PARA);
	    if (ret == NMT_RET_BUSY)  {
		/* if the application has called nmtStartupCont() yet,
		 * then the next state for nmtsMaster.fct is set
		 * don't change it */
		if (GL_ARRAY(nmtsMaster).fct == NMTS_MFCT_START_MASTER) {
		    GL_ARRAY(nmtsMaster).fct = NMTS_MFCT_WAIT_START_MASTER;
		}
	    } else
	    /* next state will be set in nmtStartMaster(); */
	    if (ret == NMT_RET_OK)  {
		GL_ARRAY(nmtsMaster).fct = NMTS_MFCT_START_ALL_NODES;
	    }
	    break;

	case NMTS_MFCT_WAIT_START_MASTER:
	    /* wait until master can be started */
	    /* this is a substate - wait until nmtStartupContReq() is called */
	    /* there the next state is set */

	    /* call slave startup for non mandatory devices */
	    ret = nmtStartupStartSlaves(CO_FALSE CO_COMMA_REDCY_PARA);
	    if (ret != NMT_RET_BUSY)  {
		ret = NMT_RET_ERROR;
	    }
	    break;

	/* start NMT slaves */
	case NMTS_MFCT_START_ALL_NODES:

#ifdef NMTSTARTUP_DEBUG
	    MY_PRINTF(DBGLVL_MASTER, "MASTER_LOOP: startAllNodes\n");
#endif /* NMTSTARTUP_DEBUG */

	    ret = nmtStartAllNodes(CO_REDCY_PARA);
	    if (ret == NMT_RET_OK)  {
		/* inform application */
		nmtStartupNetworkInd(NMT_NETWORK_ALL_SLAVES_STARTED
			CO_COMMA_LINE_PARA);
		GL_ARRAY(nmtsMaster).fct = NMTS_MFCT_POLL_OPTIONAL_SLAVES;
	    }
	    break;

	/* monitor optional slaves during normal operation */
	case NMTS_MFCT_POLL_OPTIONAL_SLAVES:

#ifdef NMTSTARTUP_DEBUG
	    MY_PRINTF(DBGLVL_MASTER, "MASTER_LOOP: poll optional slaves\n");
#endif /* NMTSTARTUP_DEBUG */

	    ret = nmtStartupStartSlaves(CO_FALSE CO_COMMA_REDCY_PARA);
	    if (ret != NMT_RET_BUSY)  {
		ret = NMT_RET_ERROR;
		GL_ARRAY(nmtsMaster).fct = NMTS_MFCT_STANDBY;

#ifdef NMTSTARTUP_DEBUG
		MY_PRINTF(DBGLVL_MASTER, "MASTER: go to standby\n");
#endif /* NMTSTARTUP_DEBUG */

	    }
	    break;

	case NMTS_MFCT_STANDBY:
	    /* this state can be set back to NMTS_MFCT_POLL_OPTIONAL_SLAVES
		at some events */
	case NMTS_MFCT_STOPPED:
	    ret = NMT_RET_ERROR;
	    break;

	/* do nothing */
	default:
	    break;
    }

    if (ret == NMT_RET_ERROR)  {
	/* disable bootupmanager calls */
	RESET_COLIB_FLAG(COFLAG_NMT_STARTUP_MANAGER);
    }
}


/****************************************************************************/
/**
* nmtStartupResetComm - initiate global Reset Communication
*
* \internal
* This function sends a Reset Communication command to each
* NMT slave which keep alive bit is not set.
*
* \return
* nothing
*/
static UNSIGNED8 nmtStartupResetComm(
	CO_REDCY_PARA_DECL	/* number of CAN line 0..CO_MAX_CAN_LINES-1 */
    )
{
UNSIGNED8 i;			/* slave nr */
UNSIGNED16 sIdx;		/* slave index in nmtSlaveList */
BOOL_T	sendCmd = CO_TRUE;

    /* if global reset comm allowed use it, */
    if (GL_ARRAY(nmtsMaster).globalRstCommAllowed == 0)  {
	resetCommReq(128 CO_COMMA_REDCY_PARA);
    } else {
	/* start reset comm for each node individually */

	/* for all slaves in the network
	 * incl. all nodes there are not part of 0x1f81 */
	for (i = 1; i < 128; i++) {
	    sendCmd = CO_TRUE;

	    /* if node in startup network list ? */
	    sIdx = getNmtStartupSlaveIndex(i CO_COMMA_LINE_PARA);
	    if (sIdx != 0xff)  {
		/* node is in list */
# ifdef CONFIG_MULT_LINES
		sIdx += GL_ARRAY(co_nmtSlaveLineOffs);
# endif /* CONFIG_MULT_LINES */

		/* if keep alive bit is set ? */
		if ((GL_PVAR(nmtStartupSlave)[sIdx].obj1F81
			    & NMT_RESET_COMMUNICATION_BIT)
			!= 0) {
		    /* node state OPERATIONAL ? */
		    if (getRemoteNodeState(i CO_COMMA_REDCY_PARA) == OPERATIONAL) {
			sendCmd = CO_FALSE;
		    }
		}
	    } else {
		/* node is not in list */
		/* ignore own node id */
		if (i == GL_ARRAY(coNodeId))  {
		    sendCmd = CO_FALSE;
		}
	    }

	    /* can we send command ? */
	    if (sendCmd == CO_TRUE)  {
		resetCommReq(0x80 + i CO_COMMA_REDCY_PARA);
	    }
	}
    }

    return(NMT_RET_OK);
}


/****************************************************************************/
/**
* nmtStartBootTimer - start boot time monitoring
*
* \internal
* This function starts the boot time monitoring if the boot time is
* unequal 0 and there are mandatory NMT slaves in the network.
*
* \return
* nothing
*/
static UNSIGNED8 nmtStartBootTimer(
	CO_LINE_PARA_DECL	/* number of CAN line 0..CO_MAX_CAN_LINES-1 */
    )
{
    /* start boot timer if there are any mandatory slave
     * and the boot time is unequal 0 */
    if ((GL_ARRAY(nmtsMaster).mandatorySlaveCnt != 0)
     && (GL_ARRAY(nmtsMaster).bootTime != 0))  {
	addTimerEvent(&GL_ARRAY(nmtsMaster).bootTimer,
	    GL_ARRAY(nmtsMaster).bootTime,
	    CO_TIMER_TYPE_NMT_BOOT_TIME CO_COMMA_LINE_PARA);
    }

    return(NMT_RET_OK);
}


/****************************************************************************/
/**
* nmtStartupStartSlaves - startup of NMT slaves
*
* \internal
* This function initiates the startup process for each NMT slave.
*
* \return
*	NMT_RET_OK	 - all mandatory or optional are finished (see checkMan)
*	NMT_RET_BUSY	 - call function again
*	NMT_RET_WAIT4TIMER - function wait for timer, no call necessary
*/
static UNSIGNED8 nmtStartupStartSlaves(
	BOOL_T	checkMandatorySlaves	/* check mandatory or optional slaves */
	CO_COMMA_REDCY_PARA_DECL
    )
{
UNSIGNED8	retVal = NMT_RET_BUSY;	/* return value from subroutine */
UNSIGNED16	sIdx;
BOOL_T		found = CO_FALSE;

    /* all slaves processed ? */
    if (GL_ARRAY(nmtsMaster).si == GL_ARRAY(nmtsMaster).slaveCnt)  {
	/* all slaves are called, start again */
	GL_ARRAY(nmtsMaster).si = 0;
	retVal = NMT_RET_WAIT4TIMER;

	/* which type of slaves should be checked ? */
	if (checkMandatorySlaves == CO_TRUE)  {
	    /* are all mandatory slaves finished ? */
	    if (GL_ARRAY(nmtsMaster).mandatorySlaveFinish
	     >= GL_ARRAY(nmtsMaster).mandatorySlaveCnt)  {

#ifdef NMTSTARTUP_DEBUG
		MY_PRINTF(DBGLVL_SLAVE, "nmtStartupStartSlaves: all mandatory slaves finished\n");
#endif /* NMTSTARTUP_DEBUG */

		/* all slaves are processed, return with ok */
		if (GL_ARRAY(nmtsMaster).mandatorySlaveFail != 0) {
		    return(NMT_SLAVE_MANDATORY_ERROR);
		} else {
		    return(NMT_RET_OK);
		}
	    }
	} else {
	    /* check optional slaves */
	    /* are all mandatory slaves finished ? */
	    if (GL_ARRAY(nmtsMaster).optionalSlaveFinish
	     >= GL_ARRAY(nmtsMaster).optionalSlaveCnt)  {

#ifdef NMTSTARTUP_DEBUG
		MY_PRINTF(DBGLVL_SLAVE, "nmtStartupStartSlaves: all optional slaves finished\n");
#endif /* NMTSTARTUP_DEBUG */

		/* all slaves are processed, return with ok */
		return(NMT_RET_OK);
	    }
	}
    }

    /* search for next mandatory or optional slave */
    while (GL_ARRAY(nmtsMaster).si < GL_ARRAY(nmtsMaster).slaveCnt)  {

	/*--- calculate slave index in nmtSlaveList */
	sIdx = GL_ARRAY(nmtsMaster).si;
# ifdef CONFIG_MULT_LINES
	sIdx += GL_ARRAY(co_nmtSlaveLineOffs);
# endif /* CONFIG_MULT_LINES */

	/* is this a mandatory or optional slave
		and bootup not finished */
	if ((GL_PVAR(nmtStartupSlave)[sIdx].fct & NMTS_SFCT_NO) == 0) {
	    found = CO_TRUE;
	    break;
	}

	/*--- select next slave node ---*/
	GL_ARRAY(nmtsMaster).si++;
    }

    if (found == CO_TRUE)  {

	/* work slave */
	/*--- execute slave-specific startup function ---*/
	retVal = nmtsStartSlave(sIdx CO_COMMA_REDCY_PARA);
	if (retVal != NMT_RET_BUSY)  {
	    /* this node has finished intialization */
	    GL_PVAR(nmtStartupSlave)[sIdx].flags |= NMTS_SFLAG_FINISHED;

	    if (GL_PVAR(nmtStartupSlave)[sIdx].typ == NMTSLAVE_TYPE_MANDATORY){
		GL_ARRAY(nmtsMaster).mandatorySlaveFinish++;
		/* count errornoues slaves */
		if (retVal != NMT_RET_OK)  {
		    GL_ARRAY(nmtsMaster).mandatorySlaveFail++;
		}
	    }
	    if (GL_PVAR(nmtStartupSlave)[sIdx].typ == NMTSLAVE_TYPE_OPTIONAL) {
		GL_ARRAY(nmtsMaster).optionalSlaveFinish++;
	    }
	}

	GL_ARRAY(nmtsMaster).si++;

	return(NMT_RET_BUSY);
    }

    return(retVal);
}


/****************************************************************************/
/**
* nmtStartMaster - start NMT master
*
* \internal
* This function changes the NMT master into the state OPERATIONAL
* or initiates the switching into the state OPERATIONAL by the
* application.
*
* \return
* nothing
*/
static UNSIGNED8 nmtStartMaster(
	CO_REDCY_PARA_DECL	/* number of CAN line 0..CO_MAX_CAN_LINES-1 */
    )
{
#ifdef NMTSTARTUP_DEBUG
    MY_PRINTF(DBGLVL_MASTER, "nmtStartMaster: ");
#endif /* NMTSTARTUP_DEBUG */

    /* stop boot timer */
    removeTimerEvent(&GL_ARRAY(nmtsMaster).bootTimer CO_COMMA_LINE_PARA);

    /* The application is responsible for the changing of the NMT master
     * into the state OPERATIONAL. */
    if ((GL_ARRAY(nmtsMaster).obj1F80 & NMT_STARTUP_MASTER_START_BIT) != 0)
    {

#ifdef NMTSTARTUP_DEBUG
	MY_PRINTF(DBGLVL_MASTER, "wait for application\n");
#endif /* NMTSTARTUP_DEBUG */

	/* The application needs further resources until the NMT
	 * master can be changed into the state OPERATIONAL.
	 * The NMT Startup process has to wait until the application
	 * is ready */

	/* set it here, may be the application calls the cont-function
	   immediately */

	nmtStartupMasterInd(NMT_MASTER_READY4START CO_COMMA_LINE_PARA);

	return(NMT_RET_BUSY);
    }

    /* Selfstarting is active. Change the NMT master into OPERATIONAL. */
    else
    {
#ifdef NMTSTARTUP_DEBUG
	MY_PRINTF(DBGLVL_MASTER, "myself\n");
#endif /* NMTSTARTUP_DEBUG */

	startRemoteNodeReq(GL_ARRAY(coNodeId) CO_COMMA_REDCY_PARA);
	/* inform application */
	nmtStartupMasterInd(NMT_MASTER_IS_STARTED CO_COMMA_LINE_PARA);
    }

    return(NMT_RET_OK);
}


/****************************************************************************/
/**
* nmtStartAllNodes - start all nodes
*
* \internal
* This function starts all slave nodes
*
* \return
* nothing
*/
static UNSIGNED8 nmtStartAllNodes(
	CO_REDCY_PARA_DECL	/* number of CAN line 0..CO_MAX_CAN_LINES-1 */
    )
{
UNSIGNED8	i;
UNSIGNED16	idx;

#ifdef NMTSTARTUP_DEBUG
    MY_PRINTF(DBGLVL_MASTER, "nmtStartAllNodes: ");
#endif /* NMTSTARTUP_DEBUG */

    /* should we start the nodes ? */
    if ((GL_ARRAY(nmtsMaster).obj1F80 & NMT_STARTUP_NOT_START_NODE_BIT) != 0)  {
#ifdef NMTSTARTUP_DEBUG
	MY_PRINTF(DBGLVL_MASTER, "not allowed to start slaves (1f80:3)");
#endif /* NMTSTARTUP_DEBUG */

	/* no, return */
	return(NMT_RET_OK);
    }

    /* start all nodes with global command allowed */
    if ((GL_ARRAY(nmtsMaster).obj1F80 & NMT_STARTUP_START_ALL_NODE_BIT) == 0) {
	/* no, return */
#ifdef NMTSTARTUP_DEBUG
	MY_PRINTF(DBGLVL_MASTER, "not allowed with global cmd (1f80:1)");
#endif /* NMTSTARTUP_DEBUG */
	return(NMT_RET_OK);
    }

    /* all optional slaves are started ? */
    if (GL_ARRAY(nmtsMaster).optionalSlaveFinish
		      == GL_ARRAY(nmtsMaster).optionalSlaveCnt)  {

#ifdef NMTSTARTUP_DEBUG
	MY_PRINTF(DBGLVL_MASTER, "global for all nodes\n");
#endif /* NMTSTARTUP_DEBUG */

	/* start all remote slaves */
	startRemoteNodeReq(0 CO_COMMA_REDCY_PARA);
    } else {
	/* start each node individually */
	for (i = 0; i < GL_ARRAY(nmtsMaster).slaveCnt; i++) {

	    idx = i;
#ifdef CONFIG_MULT_LINES
	    idx += GL_ARRAY(co_nmtSlaveLineOffs);
#endif /* CONFIG_MULT_LINES */

#ifdef NMTSTARTUP_DEBUG
	    MY_PRINTF(DBGLVL_MASTER, "for node %d\n", i);
#endif /* NMTSTARTUP_DEBUG */

	    /* for all mandatory and optional slaves */
	    if (GL_PVAR(nmtStartupSlave)[idx].typ != NMTSLAVE_TYPE_UNKNOWN)  {
		/* if startup was finished without errors */
		if ((GL_PVAR(nmtStartupSlave)[idx].flags
		     & (NMTS_SFLAG_NODE_ERROR | NMTS_SFLAG_FINISHED))
		    == NMTS_SFLAG_FINISHED)  {
		    startRemoteNodeReq(0x80 + GL_PVAR(nmtStartupSlave)[idx].nodeId
			CO_COMMA_REDCY_PARA);
		}
	    }
	}
    }
    return(NMT_RET_OK);
}


/****************************************************************************/
/**
* nmtStartupBootTimeElapsed - boot time is elapsed
*
* \internal
* This function is called if the boot timer is timed up.
*
* \return
* nothing
*/
void nmtStartupBootTimeElapsed(
	CO_LINE_PARA_DECL	/* number of CAN line 0..CO_MAX_CAN_LINES-1 */
    )
{
UNSIGNED8	i;
UNSIGNED16	idx;

    /* disable all mandatory slaves */
    for (i = 0; i < GL_ARRAY(nmtsMaster).slaveCnt; i++)  {

	idx = i;
# ifdef CONFIG_MULT_LINES
	idx += GL_ARRAY(co_nmtSlaveLineOffs);
# endif /* CONFIG_MULT_LINES */

	/* is this a mandatory and bootup not finished */
	if (GL_PVAR(nmtStartupSlave)[idx].typ == NMTSLAVE_TYPE_MANDATORY)  {

	    /* inform application */
	    if ((GL_PVAR(nmtStartupSlave)[idx].fct & ~NMTS_SFCT_NO) != 0) {
		nmtStartupSlaveInd(GL_PVAR(nmtStartupSlave)[idx].nodeId,
			NMT_SLAVE_MANDATORY_ERROR
			CO_COMMA_LINE_PARA);
		/* disable actual function */
		GL_PVAR(nmtStartupSlave)[idx].fct = NMTS_SFCT_NO;
		GL_ARRAY(nmtsMaster).mandatorySlaveFail++;
	    }
	}
    }

    nmtStartupNetworkInd(NMT_NETWORK_BOOT_TIMEOUT CO_COMMA_LINE_PARA);

    GL_ARRAY(nmtsMaster).fct = NMTS_MFCT_STOPPED;
}


/****************************************************************************/
/**
* nmtsTimerEvent - handle the boot timer
*
* \internal
* This function is called if a boot timer is timed up.
*
* \return
* nothing
*/
void nmtsTimerEvent(
	TIMER_EVENT_T *pTimer	/* pointer at the timer for boot monitoring */
	CO_COMMA_REDCY_PARA_DECL
    )
{
NMT_SLAVE_STARTUP_T	*pSlave;

    if (&GL_ARRAY(nmtsMaster).bootTimer == pTimer)
    {
	nmtStartupBootTimeElapsed(CO_LINE_PARA);
    } else {
	pSlave = (NMT_SLAVE_STARTUP_T *)pTimer;
	/* if any function is enabled, delete waiting bit */
	if ((pSlave->fct & ~NMTS_SFCT_NO) != 0)  {
	    pSlave->fct &= ~NMTS_SFCT_NO;
	    if (GL_ARRAY(nmtsMaster).fct == NMTS_MFCT_STANDBY)  {
		GL_ARRAY(nmtsMaster).fct = NMTS_MFCT_POLL_OPTIONAL_SLAVES;
	    }
	    SET_COLIB_FLAG(COFLAG_NMT_STARTUP_MANAGER);
	}
    }
}


/****************************************************************************/
/**
* nmtStartupSdoEvent - handle events for SDO Startup process
*
* \return
* nothing
*/
void nmtStartupSdoEvent(
	UNSIGNED8 eventCode,	/* code of the event */
	UNSIGNED8 sdoNr		/* sdo nr for this call */
	CO_COMMA_REDCY_PARA_DECL
    )
{
UNSIGNED16 idx;	        /* index in the nmtSlaveList */

    idx = sdoNr - 1;
#ifdef CONFIG_MULT_LINES
    idx += GL_ARRAY(co_nmtSlaveLineOffs);
#endif /* CONFIG_MULT_LINES */

    /* check, if we have started this transfer */
    if ((GL_PVAR(nmtStartupSlave)[idx].flags & NMTS_SFLAG_SDO_IN_USE) == 0)  {
	return;
    }

#ifdef NMTSTARTUP_DEBUG
    MY_PRINTF(DBGLVL_EVENT, "nmtStartupSdoEvent: event 0x%x sdoNr %d\n",
		eventCode, sdoNr);
#endif /* NMTSTARTUP_DEBUG */

    GL_PVAR(nmtStartupSlave)[idx].flags &= (FLAG_T)~NMTS_SFLAG_SDO_IN_USE;

    /* continue bootup only if function is availble */
    if ((GL_PVAR(nmtStartupSlave)[idx].fct & ~NMTS_SFCT_NO) != 0)  {
	GL_PVAR(nmtStartupSlave)[idx].fct &= ~NMTS_SFCT_NO;

	if (GL_ARRAY(nmtsMaster).fct == NMTS_MFCT_STANDBY)  {
	    GL_ARRAY(nmtsMaster).fct = NMTS_MFCT_POLL_OPTIONAL_SLAVES;
	}
	SET_COLIB_FLAG(COFLAG_NMT_STARTUP_MANAGER);
    }

    switch (eventCode) {
	/*--- SDO abort code received ---*/
	case NMT_RET_SDO_ABORT:
            GL_PVAR(nmtStartupSlave)[idx].flags &= (FLAG_T)~NMTS_SFLAG_SDO_TIMEOUT;
            GL_PVAR(nmtStartupSlave)[idx].flags |= NMTS_SFLAG_SDO_ABORT;
            break;

	/*--- SDO timeout occurred ---*/
	case NMT_RET_SDO_TIMEOUT:

#ifdef NMTSTARTUP_DEBUG
	    MY_PRINTF(DBGLVL_EVENT, "nmtSdoEvent: timeout sdo %d\n", sdoNr);
#endif /* NMTSTARTUP_DEBUG */

            GL_PVAR(nmtStartupSlave)[idx].flags |= NMTS_SFLAG_SDO_TIMEOUT;
            GL_PVAR(nmtStartupSlave)[idx].flags &= (FLAG_T)~NMTS_SFLAG_SDO_ABORT;
            break;

	case NMT_RET_SDO_OK:
            GL_PVAR(nmtStartupSlave)[idx].flags &= (FLAG_T)~NMTS_SFLAG_SDO_TIMEOUT;
            GL_PVAR(nmtStartupSlave)[idx].flags &= (FLAG_T)~NMTS_SFLAG_SDO_ABORT;
            break;

	default:

#ifdef NMTSTARTUP_DEBUG
	    MY_PRINTF(0, "*** nmtStartupSdoEvent: event 0x%x sdoNr %d unknown \n", eventCode, sdoNr);
#endif /* NMTSTARTUP_DEBUG */
	    break;

    }
}


/****************************************************************************/
/**
* nmtsEventHandler - handle events for NMT Startup process
*
* \return
* nothing
*/
void nmtsEventHandler(
	UNSIGNED8 eventCode,	/* code of the event */
	UNSIGNED8 nodeId	/* node-id of the slave, 0 for network */
	CO_COMMA_REDCY_PARA_DECL
    )
{
UNSIGNED16	idx;	        /* index in the nmtSlaveList */

    /* search index in the nmtSlaveList */
    idx = getNmtStartupSlaveIndex(nodeId CO_COMMA_LINE_PARA);
    if (idx == 0xFF) {
	/* node-id not found in nmtSlaveList */
	return;
    }

# ifdef CONFIG_MULT_LINES
    idx += GL_ARRAY(co_nmtSlaveLineOffs);
# endif /* CONFIG_MULT_LINES */

#ifdef NMTSTARTUP_DEBUG
    MY_PRINTF(DBGLVL_EVENT, "nmtsEventHandler: node %d - (%d) ", nodeId, eventCode);
#endif /* NMTSTARTUP_DEBUG */

    switch (eventCode)
    {
	/*--- Heartbeat lost ---*/
	case NMT_ERRCTRL_HB_LOST:
#ifdef CO_CONFIG_NMTSTART_RESTART_LOST_CON
        case NMT_ERRCTRL_LOST_GUARDING:
#endif /* CO_CONFIG_NMTSTART_RESTART_LOST_CON */
#ifdef NMTSTARTUP_DEBUG
	    MY_PRINTF(DBGLVL_EVENT, "lost HB\n");
#endif /* NMTSTARTUP_DEBUG */

	    /* during NMT startup - nmt error control was already started */
	    if ((GL_PVAR(nmtStartupSlave)[idx].fct
		    == (NMTS_SFCT_START_NODE | NMTS_SFCT_NO))
	     && (GL_PVAR(nmtStartupSlave)[idx].typ == NMTSLAVE_TYPE_MANDATORY)){
		/* abort slave startup sequence */
		GL_PVAR(nmtStartupSlave)[idx].fct = NMTS_SFCT_ERROR_OCCURED;
		SET_COLIB_FLAG(COFLAG_NMT_STARTUP_MANAGER);
            }

            /* after starting during normal operation */
	    else
	    {
		/* inform application */
		nmtStartupSlaveInd(nodeId, NMT_SLAVE_E_NO_HEARTBEAT_STATE
			CO_COMMA_LINE_PARA);
		/* execute CiA-302-2 Error Handler */
		nmtsErrorHandler(nodeId CO_COMMA_REDCY_PARA);
	    }
	    break;

	/*--- Heartbeat started ---*/
	case NMT_ERRCTRL_HB_STARTED:
	case NMT_ERRCTRL_GUARD_RECEIVED:

#ifdef NMTSTARTUP_DEBUG
	    MY_PRINTF(DBGLVL_EVENT, "Guarding/HB started\n");
#endif /* NMTSTARTUP_DEBUG */

	    if ((GL_PVAR(nmtStartupSlave)[idx].fct
			== (NMTS_SFCT_START_NODE | NMTS_SFCT_NO))
	     || (GL_PVAR(nmtStartupSlave)[idx].fct
			== (NMTS_SFCT_SOFTWARE_UPDATE | NMTS_SFCT_NO))
	     || (GL_PVAR(nmtStartupSlave)[idx].fct
			== (NMTS_SFCT_RESET_COMM | NMTS_SFCT_NO))) {
		GL_PVAR(nmtStartupSlave)[idx].fct &= ~NMTS_SFCT_NO;
		if (GL_ARRAY(nmtsMaster).fct == NMTS_MFCT_STANDBY)  {
		    GL_ARRAY(nmtsMaster).fct = NMTS_MFCT_POLL_OPTIONAL_SLAVES;
		}
		SET_COLIB_FLAG(COFLAG_NMT_STARTUP_MANAGER);
	    }

	    /* not used for device detection
	     * all mandatory slaves has to be availble during bootup process
	     * after that, the device is configured and ready or failed
	     * There is no automatically way to boot this kind of nodes again
	     * All optionally slaves are requested by sdo -
	     * if the node answers, the startup begins automatically
	     */
            break;

	case NMT_ERRCTRL_NODE_STATE:

#ifdef NMTSTARTUP_DEBUG
	    MY_PRINTF(DBGLVL_EVENT, "bad node state\n");
#endif /* NMTSTARTUP_DEBUG */

	    /* execute CiA-302-2 Error Handler */
	    nmtsErrorHandler(nodeId CO_COMMA_REDCY_PARA);
	    break;

	/* next errctrl received */
	case NMT_ERRCTRL_RECEIVED:

#ifdef NMTSTARTUP_DEBUG
	    MY_PRINTF(DBGLVL_EVENT, "next error ctrl received\n");
#endif /* NMTSTARTUP_DEBUG */

	    if ((GL_PVAR(nmtStartupSlave)[idx].fct
			== (NMTS_SFCT_FINISHED_OK | NMTS_SFCT_NO))
	     || (GL_PVAR(nmtStartupSlave)[idx].fct
			== (NMTS_SFCT_RESET_COMM | NMTS_SFCT_NO))) {

		GL_PVAR(nmtStartupSlave)[idx].fct &= ~NMTS_SFCT_NO;

		if (GL_ARRAY(nmtsMaster).fct == NMTS_MFCT_STANDBY)  {
		    GL_ARRAY(nmtsMaster).fct = NMTS_MFCT_POLL_OPTIONAL_SLAVES;
		}
		SET_COLIB_FLAG(COFLAG_NMT_STARTUP_MANAGER);
	    }
	    break;

	case NMT_ERRCTRL_BOOTUP_RECEIVED:
	    if (GL_PVAR(nmtStartupSlave)[idx].fct
			== (NMTS_SFCT_SOFTWARE_UPDATE | NMTS_SFCT_NO)) {
		/* remove timer */
		removeTimerEvent(&GL_PVAR(nmtStartupSlave)[idx].timer
		    CO_COMMA_LINE_PARA);

		GL_PVAR(nmtStartupSlave)[idx].fct &= ~NMTS_SFCT_NO;
		if (GL_ARRAY(nmtsMaster).fct == NMTS_MFCT_STANDBY)  {
		    GL_ARRAY(nmtsMaster).fct = NMTS_MFCT_POLL_OPTIONAL_SLAVES;
		}
		SET_COLIB_FLAG(COFLAG_NMT_STARTUP_MANAGER);
	    }
	    break;

	default:
#ifdef NMTSTARTUP_DEBUG
	    MY_PRINTF(DBGLVL_EVENT, "unknown event: 0x%x\n", eventCode);
#endif /* NMTSTARTUP_DEBUG */
	    break;
    }
}

/****************************************************************************/
/**
* nmtsErrorHandler - error handler of the CiA-302-2
*
* \internal
* This functions realizes the Error Handler from the CiA-302-2 for
* error control events of NMT slaves.
*
* \return
* nothing
*/
static void nmtsErrorHandler(
	UNSIGNED8 nodeId	/* node-id of the slave (1, 127) */
	CO_COMMA_REDCY_PARA_DECL
    )
{
UNSIGNED16	idx;	        /* index in the nmtSlaveList */

    /*--- search index in the nmtSlaveList ---*/
    idx = getNmtStartupSlaveIndex(nodeId CO_COMMA_LINE_PARA);
    if (idx == 0xFF) {
	/* node-id not found in nmtSlaveList */
	return;
    }

# ifdef CONFIG_MULT_LINES
    idx += GL_ARRAY(co_nmtSlaveLineOffs);
# endif /* CONFIG_MULT_LINES */

    /*--- NMT slave is optional ---*/
    if (GL_PVAR(nmtStartupSlave)[idx].typ == NMTSLAVE_TYPE_OPTIONAL) {
	/* restart boot NMT SLave */
	restartBootNmtSlave(idx, NMTSLAVE_TYPE_OPTIONAL CO_COMMA_REDCY_PARA);

	return;
    }

    if (GL_PVAR(nmtStartupSlave)[idx].typ != NMTSLAVE_TYPE_MANDATORY) {
	return;
    }


    /* this part is only valid for mandatory slaves */
    /*
     * That is why stop(node) is used. */
    if ((GL_ARRAY(nmtsMaster).obj1F80 & NMT_STARTUP_STOP_ALL_NODES_BIT) != 0) {
	/* send stopped to all nodes */
	stopRemoteNodeReq(128 CO_COMMA_REDCY_PARA);

	/* inform application */
	nmtStartupNetworkInd(NMT_NETWORK_STARTUP_STOPPED CO_COMMA_LINE_PARA);
	/* stop boot process */
	GL_ARRAY(nmtsMaster).fct = NMTS_MFCT_STOPPED;
    } else {
	/* stop all nodes not allowed */
	if ((GL_ARRAY(nmtsMaster).obj1F80 & NMT_STARTUP_RESET_ALL_NODES_BIT)
		!= 0) {
	    resetNodeReq(128 CO_COMMA_REDCY_PARA);
	    /* inform application */
	    nmtStartupNetworkInd(NMT_NETWORK_STARTUP_STOPPED CO_COMMA_LINE_PARA);
	    /* stop boot process */
	    GL_ARRAY(nmtsMaster).fct = NMTS_MFCT_STOPPED;
	} else {
	    resetNodeReq(0x80 + nodeId CO_COMMA_REDCY_PARA);

	    /* restart boot process for this node */
	    restartBootNmtSlave(idx, NMTSLAVE_TYPE_MANDATORY
			CO_COMMA_REDCY_PARA);
	}
    }
}


static void restartBootNmtSlave(
	UNSIGNED16	idx,	/* slave index */
	UNSIGNED8	typ	/* type of slave */
	CO_COMMA_REDCY_PARA_DECL
    )
{
    if ((GL_PVAR(nmtStartupSlave)[idx].flags & NMTS_SFLAG_FINISHED) != 0) {
	if (typ == NMTSLAVE_TYPE_MANDATORY)  {
	    GL_ARRAY(nmtsMaster).mandatorySlaveFinish--;
	} else {
	    GL_ARRAY(nmtsMaster).optionalSlaveFinish--;
	}
    }

    GL_PVAR(nmtStartupSlave)[idx].flags = 0;
    GL_PVAR(nmtStartupSlave)[idx].fct = NMTS_SFCT_START;

    if (GL_ARRAY(nmtsMaster).fct == NMTS_MFCT_STANDBY)  {
	if (typ == NMTSLAVE_TYPE_MANDATORY)  {
	    GL_ARRAY(nmtsMaster).fct = NMTS_MFCT_START_SLAVES;
	} else {
	    GL_ARRAY(nmtsMaster).fct = NMTS_MFCT_POLL_OPTIONAL_SLAVES;
	}
	SET_COLIB_FLAG(COFLAG_NMT_STARTUP_MANAGER);
    }
}


/*==========================================================================*/
/* NMT STARTUP SLAVE-SPECIFIC FUNCTIONS                                     */
/*==========================================================================*/


/****************************************************************************/
/**
* nmtsStartSlave - starts a single NMT slave
*
* \retval NMT_RET_OK
* NMT startup process finished successful for this slave
*
* \retval NMT_RET_BUSY
* NMT startup process is running for this slave
*
* \retval E_NMT_A_OBJ_NOT_LISTED
* slave was removed from the network list (see object 0x1F81/bit 0)
*
* \retval E_NMT_B_NO_DEVICE_TYPE
* slave has not sent its device type by SDO
*
* \retval E_NMT_C_WRONG_DEVICE_TYPE
* slave has sent an unexpected device type
*
* \retval E_NMT_D_WRONG_VENDOR_ID
* slave has sent an unexpected vendor-id
*
* \retval E_NMT_E_NO_HEARTBEAT_STATE
* the slave has not sent a Heartbeat message
*
* \retval E_NMT_F_NO_NODE_GUARDING_RES
* the slave has not sent a Node Guarding message
*
* \retval E_NMT_M_WRONG_PRODUCT_CODE
* slave has sent an unexpected product code
*
* \retval E_NMT_N_WRONG_REVISION
* slave has sent an unexpected revision number
*
* \retval E_NMT_O_WRONG_SERIAL_NUMBER
* slave has sent an unexpected serial number
*/
static UNSIGNED8 nmtsStartSlave(
	UNSIGNED16 sIdx		/* index in the nmtSlaveList */
	CO_COMMA_REDCY_PARA_DECL
    )
{
UNSIGNED8	retVal;		/* return value from subroutine */

#ifdef NMTSTARTUP_DEBUG
    MY_PRINTF(DBGLVL_SLAVE, "nmtsStartSlave: %d - cmd: %s\n", sIdx,
	getCmdStrg(
	GL_PVAR(nmtStartupSlave)[sIdx].fct));
#endif /* NMTSTARTUP_DEBUG */

    /*--- execute NMT Startup slave function ---*/
    switch (GL_PVAR(nmtStartupSlave)[sIdx].fct) {
	case NMTS_SFCT_START:
	    /* inform application about start boot */
	    nmtStartupSlaveInd(GL_PVAR(nmtStartupSlave)[sIdx].nodeId,
		NMT_SLAVE_BOOT_STARTED CO_COMMA_LINE_PARA);

	    GL_PVAR(nmtStartupSlave)[sIdx].fct = NMTS_SFCT_REQUEST_DEVICE_TYPE;
	    retVal = NMT_RET_BUSY;
	    break;

	/* check device type */
	case NMTS_SFCT_REQUEST_DEVICE_TYPE:
	    retVal = nmtsSRequestDeviceType(sIdx CO_COMMA_LINE_PARA);
	    break;
	case NMTS_SFCT_CHECK_DEVICE_TYPE:
	    retVal = nmtsSCheckDeviceType(sIdx CO_COMMA_LINE_PARA);
	    break;

	/* check vendor-id */
	case NMTS_SFCT_REQUEST_IDENTITY_VENDOR:
	    retVal = nmtsSRequestVendorId(sIdx CO_COMMA_LINE_PARA);
	    break;
	case NMTS_SFCT_CHECK_IDENTITY_VENDOR:
	    retVal = nmtsSCheckVendorId(sIdx CO_COMMA_LINE_PARA);
	    break;

	/* check product code */
	case NMTS_SFCT_REQUEST_IDENTITY_PRODUCT_CODE:
	    retVal = nmtsSRequestProductCode(sIdx CO_COMMA_LINE_PARA);
	    break;
	case NMTS_SFCT_CHECK_IDENTITY_PRODUCT_CODE:
	    retVal = nmtsSCheckProductCode(sIdx CO_COMMA_LINE_PARA);
	    break;

	/* check revision number */
	case NMTS_SFCT_REQUEST_IDENTITY_REVISION:
	    retVal = nmtsSRequestRevision(sIdx CO_COMMA_LINE_PARA);
	    break;
	case NMTS_SFCT_CHECK_IDENTITY_REVISION:
	    retVal = nmtsSCheckRevision(sIdx CO_COMMA_LINE_PARA);
	    break;

	/* check serial number */
	case NMTS_SFCT_REQUEST_IDENTITY_SERIAL_NUMBER:
	    retVal = nmtsSRequestSerialNumber(sIdx CO_COMMA_LINE_PARA);
	    break;
	case NMTS_SFCT_CHECK_IDENTITY_SERIAL_NUMBER:
	    retVal = nmtsSCheckSerialNumber(sIdx CO_COMMA_LINE_PARA);
	    break;

	/* generate Reset Communication for NMT slaves whose keep
	 * alive bit is set and whose communication state is not
	 * OPERATIONAL */
	case NMTS_SFCT_RESET_COMM:
	    retVal = nmtsSResetComm(sIdx CO_COMMA_REDCY_PARA);
	    break;

	/* update software */
	case NMTS_SFCT_SOFTWARE_UPDATE:
	    retVal = nmtsSUpdateSoftware(sIdx CO_COMMA_LINE_PARA);
	    break;

	/* configure NMT slave */
	case NMTS_SFCT_CONFIG:
	    retVal = nmtsSConfigureSlave(sIdx CO_COMMA_LINE_PARA);
	    break;

	case NMTS_SFCT_CHECK_CONFIG1:
	    retVal = nmtsSCheckConfigSlave1(sIdx CO_COMMA_LINE_PARA);
	    break;

	case NMTS_SFCT_CHECK_CONFIG2:
	    retVal = nmtsSCheckConfigSlave2(sIdx CO_COMMA_LINE_PARA);
	    break;

	case NMTS_SFCT_UPDATE_CONFIG:
	    retVal = nmtsSUpdateConfigSlave(sIdx CO_COMMA_LINE_PARA);
	    break;

	/* start error control */
	case NMTS_SFCT_START_ERROR_CONTROL:
	    retVal = nmtsSStartErrorControl(sIdx CO_COMMA_LINE_PARA);
	    break;

	/* start NMT slave */
	case NMTS_SFCT_START_NODE:
	    retVal = nmtsSStartNode(sIdx CO_COMMA_REDCY_PARA);
	    break;

	/* finish bootup procedure with ok */
	case NMTS_SFCT_FINISHED_OK:
	    retVal = nmtsSFinishedOk(sIdx CO_COMMA_LINE_PARA);
	    break;

	/* error event occured */
	case NMTS_SFCT_ERROR_OCCURED:
	    retVal = NMT_SLAVE_K_NO_HB;
	    break;

	default:
	    retVal = NMT_RET_BUSY;
	    break;
    }

    /* call user indication for errors */
    if ((retVal != NMT_RET_BUSY) && (retVal != NMT_RET_OK))  {
	nmtStartupSlaveInd(GL_PVAR(nmtStartupSlave)[sIdx].nodeId, retVal
		CO_COMMA_LINE_PARA);
	GL_PVAR(nmtStartupSlave)[sIdx].flags |= NMTS_SFLAG_NODE_ERROR;
    }

    return (retVal);
}


/****************************************************************************/
/**
* nmtsSRequestDeviceType - request the device type from the NMT slave
*
* \retval E_NMT_B_NO_DEVICE_TYPE
* NMT master cannot send SDO to the NMT slave
* \retval NMT_RET_BUSY
* slave is ready for the next step of the NMT Startup process
*/
static UNSIGNED8 nmtsSRequestDeviceType(
	UNSIGNED16 idx	        /* index in the nmtSlaveList */
	CO_COMMA_LINE_PARA_DECL	/* number of CAN line 0..CO_MAX_CAN_LINES-1 */
    )
{
RET_T	retVal;

#ifdef NMTSTARTUP_DEBUG
    MY_PRINTF(DBGLVL_DEVTYPE, "nmtsSRequestDeviceType: %d - ", idx);
#endif /* NMTSTARTUP_DEBUG */

    retVal = readSdo(idx, DEVICE_TYPE_INDEX, 0,
	(UNSIGNED8 *)&GL_PVAR(nmtStartupSlave)[idx].rxBuf, 4 CO_COMMA_LINE_PARA);
    if (retVal == CO_OK)  {
	GL_PVAR(nmtStartupSlave)[idx].fct =
		NMTS_SFCT_CHECK_DEVICE_TYPE | NMTS_SFCT_NO;
    } else {
	/* ignore busy error and try again next step */
	if (retVal != CO_E_BUSY) {
	    /* inform application */

#ifdef NMTSTARTUP_DEBUG
	    MY_PRINTF(DBGLVL_DEVTYPE, " error\n");
#endif /* NMTSTARTUP_DEBUG */

	    GL_PVAR(nmtStartupSlave)[idx].fct = NMTS_SFCT_NO;
	    return (NMT_SLAVE_B_NO_DEVICE_TYPE);
	}
    }

#ifdef NMTSTARTUP_DEBUG
    MY_PRINTF(DBGLVL_DEVTYPE, " ok\n");
#endif /* NMTSTARTUP_DEBUG */

    return (NMT_RET_BUSY);
}


/****************************************************************************/
/**
* nmtsSCheckDeviceType - check the device type from the NMT slave
*
* \retval E_NMT_B_NO_DEVICE_TYPE
* NMT master did not receive an SDO response from the NMT slave
* \retval E_NMT_C_WRONG_DEVICE_TYPE
* the device type of the NMT slave is not the expected device type
* \retval NMT_RET_BUSY
* slave is ready for the next step of the NMT Startup process
*/
static UNSIGNED8 nmtsSCheckDeviceType(
	UNSIGNED16 idx	        /* index in the nmtSlaveList */
	CO_COMMA_LINE_PARA_DECL	/* number of CAN line 0..CO_MAX_CAN_LINES-1 */
    )
{
UNSIGNED32 size;	/* current size of data in bytes */
#ifdef CO_CONFIG_NMTSTART_NO_DEVICETYPE
UNSIGNED16 repeatTimeout = 0u;
#endif /* CO_CONFIG_NMTSTART_NO_DEVICETYPE */

#ifdef NMTSTARTUP_DEBUG
    MY_PRINTF(DBGLVL_DEVTYPE, "nmts checkDeviceType: %d - ", idx);
#endif /* NMTSTARTUP_DEBUG */

    /* SDO abort received */
    if ((GL_PVAR(nmtStartupSlave)[idx].flags & NMTS_SFLAG_SDO_ABORT) != 0) {
#ifdef NMTSTARTUP_DEBUG
	MY_PRINTF(DBGLVL_DEVTYPE, " sdo aborted\n");
#endif /* NMTSTARTUP_DEBUG */

#ifdef CO_CONFIG_NMTSTART_NO_DEVICETYPE
        if ( CO_FALSE == coUserNmtStartupNoDeviceType(GL_PVAR(nmtStartupSlave)[idx].nodeId, &repeatTimeout CO_COMMA_LINE_PARA) ) {
#endif /* CO_CONFIG_NMTSTART_NO_DEVICETYPE */
	    GL_PVAR(nmtStartupSlave)[idx].fct = NMTS_SFCT_NO;
	    return (NMT_SLAVE_B_NO_DEVICE_TYPE);
#ifdef CO_CONFIG_NMTSTART_NO_DEVICETYPE
        } else {
            UNSIGNED32 timeOut = 0u;
            /* request SDO again in 1 sec */
            GL_PVAR(nmtStartupSlave)[idx].fct =
            NMTS_SFCT_REQUEST_DEVICE_TYPE | NMTS_SFCT_NO;

            /* check if user doesn't want the default */
            if ( repeatTimeout == 0u ) {
                timeOut = CO_CONFIG_NMTSTART_CYCL_TIME;
            } else {
                timeOut = repeatTimeout * 10u;
            }

            addTimerEvent(&GL_PVAR(nmtStartupSlave)[idx].timer, timeOut,
            CO_TIMER_TYPE_NMT_BOOT_TIME CO_COMMA_LINE_PARA);

            return (NMT_RET_BUSY);
        }
#endif /* CO_CONFIG_NMTSTART_NO_DEVICETYPE */
    }

    /* SDO timeout occurred */
    if ((GL_PVAR(nmtStartupSlave)[idx].flags & NMTS_SFLAG_SDO_TIMEOUT) != 0) {

#ifdef NMTSTARTUP_DEBUG
	MY_PRINTF(DBGLVL_DEVTYPE, "timeout\n", idx);
#endif /* NMTSTARTUP_DEBUG */

	/* request SDO again in 1 sec */
	GL_PVAR(nmtStartupSlave)[idx].fct =
		NMTS_SFCT_REQUEST_DEVICE_TYPE | NMTS_SFCT_NO;

	addTimerEvent(&GL_PVAR(nmtStartupSlave)[idx].timer, CO_CONFIG_NMTSTART_CYCL_TIME,
	    CO_TIMER_TYPE_NMT_BOOT_TIME CO_COMMA_LINE_PARA);

	return (NMT_RET_BUSY);
    }

    /* SDO response received */
    GL_PVAR(nmtStartupSlave)[idx].objVal = 0;

    if (getObjEntry(NMT_DEVICE_IDENT_INDEX,
		GL_PVAR(nmtStartupSlave)[idx].nodeId,
		(UNSIGNED8 *)&GL_PVAR(nmtStartupSlave)[idx].objVal, &size, CO_TRUE
		CO_COMMA_LINE_PARA)
	    == CO_OK) {

	if (GL_PVAR(nmtStartupSlave)[idx].objVal != 0)  {
	    if (GL_PVAR(nmtStartupSlave)[idx].objVal != GL_PVAR(nmtStartupSlave)[idx].rxBuf) {
		GL_PVAR(nmtStartupSlave)[idx].fct = NMTS_SFCT_NO;

#ifdef NMTSTARTUP_DEBUG
		MY_PRINTF(DBGLVL_DEVTYPE, "bad type\n");
#endif /* NMTSTARTUP_DEBUG */

		return (NMT_SLAVE_C_WRONG_DEVICE_TYPE);
	    }
	}
    }

#ifdef NMTSTARTUP_DEBUG
    MY_PRINTF(DBGLVL_DEVTYPE, "ok\n");
#endif /* NMTSTARTUP_DEBUG */

    GL_PVAR(nmtStartupSlave)[idx].fct = NMTS_SFCT_REQUEST_IDENTITY_VENDOR;
    return (NMT_RET_BUSY);
}


/****************************************************************************/
/**
* nmtsSRequestVendorId - request the vendor-ID from the NMT slave
*
* \retval E_NMT_D_WRONG_VENDOR_ID
* NMT master cannot send SDO to the NMT slave
* \retval NMT_RET_BUSY
* slave is ready for the next step of the NMT Startup process
*/
static UNSIGNED8 nmtsSRequestVendorId(
	UNSIGNED16 idx	        /* index in the nmtSlaveList */
	CO_COMMA_LINE_PARA_DECL	/* number of CAN line 0..CO_MAX_CAN_LINES-1 */
    )
{
UNSIGNED32 size;	/* current size of data in bytes */
RET_T	retVal;

#ifdef NMTSTARTUP_DEBUG
    MY_PRINTF(DBGLVL_VENDOR, "nmtsSRequestVendorId: %d : ", idx);
#endif /* NMTSTARTUP_DEBUG */

    GL_PVAR(nmtStartupSlave)[idx].objVal = 0;
    /* if there is no object for comparision, skip the checking of the
     * vendor-id */
    if (getObjEntry(NMT_VENDOR_IDENT_INDEX,
		GL_PVAR(nmtStartupSlave)[idx].nodeId,
		(UNSIGNED8*)&GL_PVAR(nmtStartupSlave)[idx].objVal, &size
		, CO_TRUE CO_COMMA_LINE_PARA)
	!= CO_OK) {
	GL_PVAR(nmtStartupSlave)[idx].fct =
		NMTS_SFCT_REQUEST_IDENTITY_PRODUCT_CODE;

#ifdef NMTSTARTUP_DEBUG
	MY_PRINTF(DBGLVL_VENDOR, "ignored\n");
#endif /* NMTSTARTUP_DEBUG */
    }

    /* the value of the object is 0, do not compare the object
     * value */
    else if (GL_PVAR(nmtStartupSlave)[idx].objVal == 0) {
	GL_PVAR(nmtStartupSlave)[idx].fct =
		NMTS_SFCT_REQUEST_IDENTITY_PRODUCT_CODE;

#ifdef NMTSTARTUP_DEBUG
	MY_PRINTF(DBGLVL_VENDOR, " is zero - ignored\n");
#endif /* NMTSTARTUP_DEBUG */
    }

    /* request the vendor-id from the slave */
    else
    {
	retVal = readSdo(idx, IDENTITY_INDEX, 1,
	    (UNSIGNED8*)&GL_PVAR(nmtStartupSlave)[idx].rxBuf, 4
	    CO_COMMA_LINE_PARA);
	if (retVal == CO_OK)  {
	    GL_PVAR(nmtStartupSlave)[idx].fct =
		NMTS_SFCT_CHECK_IDENTITY_VENDOR | NMTS_SFCT_NO;
        } else {
	    /* ignore busy error and try again next step */
	    if (retVal != CO_E_BUSY) {
#ifdef NMTSTARTUP_DEBUG
		MY_PRINTF(DBGLVL_VENDOR, "sdo request error\n");
#endif /* NMTSTARTUP_DEBUG */

		GL_PVAR(nmtStartupSlave)[idx].fct = NMTS_SFCT_NO;
		return (NMT_SLAVE_D_WRONG_VENDOR_ID);
	    }
        }

#ifdef NMTSTARTUP_DEBUG
	MY_PRINTF(DBGLVL_VENDOR, "ok\n");
#endif /* NMTSTARTUP_DEBUG */
    }
    return (NMT_RET_BUSY);
}


/****************************************************************************/
/**
* nmtsSCheckVendorId - check the vendor-ID of the NMT slave
*
* \retval E_NMT_D_WRONG_VENDOR_ID
* NMT master did not receive an SDO response from the NMT slave or
* the vendor-id of the NMT slave is not the expected vendor-id
* \retval NMT_RET_BUSY
* slave is ready for the next step of the NMT Startup process
*/
static UNSIGNED8 nmtsSCheckVendorId(
	UNSIGNED16 idx	        /* index in the nmtSlaveList */
	CO_COMMA_LINE_PARA_DECL	/* number of CAN line 0..CO_MAX_CAN_LINES-1 */
    )
{
#ifdef NMTSTARTUP_DEBUG
    MY_PRINTF(DBGLVL_VENDOR, "nmtsSCheckVendorId: %d - ", idx);
#endif /* NMTSTARTUP_DEBUG */

    /* SDO timeout occurred or SDO abort received */
    if ((GL_PVAR(nmtStartupSlave)[idx].flags & NMTS_SFLAG_SDO_TIMEOUT) != 0) {

#ifdef NMTSTARTUP_DEBUG
	MY_PRINTF(DBGLVL_VENDOR, "timeout\n");
#endif /* NMTSTARTUP_DEBUG */

	GL_PVAR(nmtStartupSlave)[idx].fct = NMTS_SFCT_NO;
	return (NMT_SLAVE_D_WRONG_VENDOR_ID);
    }

    /* check value */
    if (GL_PVAR(nmtStartupSlave)[idx].objVal != 0)  {
	if (GL_PVAR(nmtStartupSlave)[idx].objVal != GL_PVAR(nmtStartupSlave)[idx].rxBuf) {

#ifdef NMTSTARTUP_DEBUG
	    MY_PRINTF(DBGLVL_VENDOR, "bad vendor\n");
#endif /* NMTSTARTUP_DEBUG */

	    GL_PVAR(nmtStartupSlave)[idx].fct = NMTS_SFCT_NO;
	    return (NMT_SLAVE_D_WRONG_VENDOR_ID);
	}
    }

#ifdef NMTSTARTUP_DEBUG
    MY_PRINTF(DBGLVL_VENDOR, "ok\n");
#endif /* NMTSTARTUP_DEBUG */

    GL_PVAR(nmtStartupSlave)[idx].fct = NMTS_SFCT_REQUEST_IDENTITY_PRODUCT_CODE;
    return (NMT_RET_BUSY);
}


/****************************************************************************/
/**
* nmtsSRequestProductCode - request the product code from the NMT slave
*
* \retval E_NMT_M_WRONG_PRODUCT_CODE
* NMT master cannot send SDO to the NMT slave
* \retval NMT_RET_BUSY
* slave is ready for the next step of the NMT Startup process
*/
static UNSIGNED8 nmtsSRequestProductCode(
	UNSIGNED16 idx	        /* index in the nmtSlaveList */
	CO_COMMA_LINE_PARA_DECL	/* number of CAN line 0..CO_MAX_CAN_LINES-1 */
    )
{
UNSIGNED32 size;	/* current size of data in bytes */
RET_T	retVal;

#ifdef NMTSTARTUP_DEBUG
    MY_PRINTF(DBGLVL_VENDOR, "nmtsSRequestProductCode: %d - ", idx);
#endif /* NMTSTARTUP_DEBUG */

    GL_PVAR(nmtStartupSlave)[idx].objVal = 0;
    /* there is no object for comparision, skip the checking of the
     * product code */
    if (getObjEntry(NMT_PRODUCT_CODE_INDEX,
		GL_PVAR(nmtStartupSlave)[idx].nodeId,
		(UNSIGNED8*)&GL_PVAR(nmtStartupSlave)[idx].objVal, &size, CO_TRUE CO_COMMA_LINE_PARA)
	!= CO_OK) {

#ifdef NMTSTARTUP_DEBUG
	MY_PRINTF(DBGLVL_VENDOR, "skipped\n");
#endif /* NMTSTARTUP_DEBUG */

        GL_PVAR(nmtStartupSlave)[idx].fct = NMTS_SFCT_REQUEST_IDENTITY_REVISION;
    }

    /* the value of the object is 0, do not compare the object
     * value */
    else if (GL_PVAR(nmtStartupSlave)[idx].objVal == 0)
    {
        GL_PVAR(nmtStartupSlave)[idx].fct = NMTS_SFCT_REQUEST_IDENTITY_REVISION;

#ifdef NMTSTARTUP_DEBUG
	MY_PRINTF(DBGLVL_VENDOR, "zero - skipped\n");
#endif /* NMTSTARTUP_DEBUG */
    }

    /* request the product code from the slave for comparision */
    else
    {
        retVal = readSdo(idx, IDENTITY_INDEX, 2,
	    (UNSIGNED8*)&GL_PVAR(nmtStartupSlave)[idx].rxBuf, 4
	    CO_COMMA_LINE_PARA);
	if (retVal == CO_OK) {
	    GL_PVAR(nmtStartupSlave)[idx].fct =
		NMTS_SFCT_CHECK_IDENTITY_PRODUCT_CODE | NMTS_SFCT_NO;
        } else {
	    /* ignore busy error and try again next step */
	    if (retVal != CO_E_BUSY) {

#ifdef NMTSTARTUP_DEBUG
		MY_PRINTF(DBGLVL_VENDOR, "sdo request error\n");
#endif /* NMTSTARTUP_DEBUG */

		GL_PVAR(nmtStartupSlave)[idx].fct = NMTS_SFCT_NO;
		return (NMT_SLAVE_M_WRONG_PRODUCT_CODE);
	    }
        }

#ifdef NMTSTARTUP_DEBUG
	MY_PRINTF(DBGLVL_VENDOR, "ok \n");
#endif /* NMTSTARTUP_DEBUG */
    }
    return (NMT_RET_BUSY);
}


/****************************************************************************/
/**
* nmtsSCheckProductCode - check the product code from the NMT slave
*
* \retval E_NMT_M_WRONG_PRODUCT_CODE
* NMT master did not receive an SDO response from the NMT slave or
* the product code of the NMT slave is not the expected product code
* \retval NMT_RET_BUSY
* slave is ready for the next step of the NMT Startup process
*/
static UNSIGNED8 nmtsSCheckProductCode(
	UNSIGNED16 idx	        /* index in the nmtSlaveList */
	CO_COMMA_LINE_PARA_DECL	/* number of CAN line 0..CO_MAX_CAN_LINES-1 */
    )
{
#ifdef NMTSTARTUP_DEBUG
    MY_PRINTF(DBGLVL_VENDOR, "nmtsSCheckProductCode: %d - ", idx);
#endif /* NMTSTARTUP_DEBUG */

    if ((GL_PVAR(nmtStartupSlave)[idx].flags & NMTS_SFLAG_SDO_TIMEOUT) != 0) {

#ifdef NMTSTARTUP_DEBUG
	MY_PRINTF(DBGLVL_VENDOR, "timeout\n");
#endif /* NMTSTARTUP_DEBUG */

	GL_PVAR(nmtStartupSlave)[idx].fct = NMTS_SFCT_NO;
	return (NMT_SLAVE_M_WRONG_PRODUCT_CODE);
    }

    /* SDO abort received: subindex is optional */
    else if ((GL_PVAR(nmtStartupSlave)[idx].flags & NMTS_SFLAG_SDO_ABORT) != 0)
    {
#ifdef NMTSTARTUP_DEBUG
	MY_PRINTF(DBGLVL_VENDOR, "abort - ignored\n");
#endif /* NMTSTARTUP_DEBUG */

        GL_PVAR(nmtStartupSlave)[idx].fct = NMTS_SFCT_REQUEST_IDENTITY_REVISION;
    }

    else
    {
	if (GL_PVAR(nmtStartupSlave)[idx].objVal != 0)  {
	    if (GL_PVAR(nmtStartupSlave)[idx].objVal != GL_PVAR(nmtStartupSlave)[idx].rxBuf) {

#ifdef NMTSTARTUP_DEBUG
		MY_PRINTF(DBGLVL_VENDOR, "bad product code %x - %x\n",
		    GL_PVAR(nmtStartupSlave)[idx].objVal, GL_PVAR(nmtStartupSlave)[idx].rxBuf);
#endif /* NMTSTARTUP_DEBUG */

		GL_PVAR(nmtStartupSlave)[idx].fct = NMTS_SFCT_NO;
		return (NMT_SLAVE_M_WRONG_PRODUCT_CODE);
	    }
	}

	GL_PVAR(nmtStartupSlave)[idx].fct = NMTS_SFCT_REQUEST_IDENTITY_REVISION;

#ifdef NMTSTARTUP_DEBUG
	MY_PRINTF(DBGLVL_VENDOR, "ok\n");
#endif /* NMTSTARTUP_DEBUG */
    }

    return (NMT_RET_BUSY);
}


/****************************************************************************/
/**
* nmtsSRequestRevision - request the revision number from the NMT slave
*
* \retval E_NMT_N_WRONG_REVISION
* NMT master cannot send SDO to the NMT slave
* \retval NMT_RET_BUSY
* slave is ready for the next step of the NMT Startup process
*/
static UNSIGNED8 nmtsSRequestRevision(
	UNSIGNED16 idx	        /* index in the nmtSlaveList */
	CO_COMMA_LINE_PARA_DECL	/* number of CAN line 0..CO_MAX_CAN_LINES-1 */
    )
{
UNSIGNED32 size;	/* current size of data in bytes */
RET_T	retVal;

#ifdef NMTSTARTUP_DEBUG
    MY_PRINTF(DBGLVL_VENDOR, "nmtsSRequestRevision: %d - ", idx);
#endif /* NMTSTARTUP_DEBUG */

    /* there is no object for comparision, skip the checking of the
     * revision number */
    GL_PVAR(nmtStartupSlave)[idx].objVal = 0;

    if (getObjEntry(NMT_REVISION_NUMBER_INDEX,
	GL_PVAR(nmtStartupSlave)[idx].nodeId,
	(UNSIGNED8 *)&GL_PVAR(nmtStartupSlave)[idx].objVal, &size, CO_TRUE CO_COMMA_LINE_PARA)
		!= CO_OK) {

#ifdef NMTSTARTUP_DEBUG
	MY_PRINTF(DBGLVL_VENDOR, "skipped\n");
#endif /* NMTSTARTUP_DEBUG */

        GL_PVAR(nmtStartupSlave)[idx].fct =
		NMTS_SFCT_REQUEST_IDENTITY_SERIAL_NUMBER;
    }

    /* the value of the object is 0, do not compare the object
     * value */
    else if (GL_PVAR(nmtStartupSlave)[idx].objVal == 0)
    {

#ifdef NMTSTARTUP_DEBUG
	MY_PRINTF(DBGLVL_VENDOR, "zero - skipped\n");
#endif /* NMTSTARTUP_DEBUG */

        GL_PVAR(nmtStartupSlave)[idx].fct =
		NMTS_SFCT_REQUEST_IDENTITY_SERIAL_NUMBER;
    }

    /* request the revision number from the slave */
    else
    {
	retVal = readSdo(idx, IDENTITY_INDEX, 3,
	    (UNSIGNED8*)&GL_PVAR(nmtStartupSlave)[idx].rxBuf, 4
	    CO_COMMA_LINE_PARA);
	if (retVal == CO_OK) {
	    GL_PVAR(nmtStartupSlave)[idx].fct =
		NMTS_SFCT_CHECK_IDENTITY_REVISION | NMTS_SFCT_NO;
	} else {
	    /* ignore busy error and try again next step */
	    if (retVal != CO_E_BUSY) {

#ifdef NMTSTARTUP_DEBUG
		MY_PRINTF(DBGLVL_VENDOR, "sdo req error\n");
#endif /* NMTSTARTUP_DEBUG */

		GL_PVAR(nmtStartupSlave)[idx].fct = NMTS_SFCT_NO;
		return (NMT_SLAVE_N_WRONG_REVISION );
	    }
	}

#ifdef NMTSTARTUP_DEBUG
	MY_PRINTF(DBGLVL_VENDOR, "ok\n");
#endif /* NMTSTARTUP_DEBUG */
    }
    return (NMT_RET_BUSY);
}


/****************************************************************************/
/**
* nmtsSCheckRevision - check the revision number from the NMT slave
*
* \retval E_NMT_N_WRONG_REVISION
* NMT master did not receive an SDO response from the NMT slave or
* the revision number of the NMT slave is not the expected revision number
* \retval NMT_RET_BUSY
* slave is ready for the next step of the NMT Startup process
*/
static UNSIGNED8 nmtsSCheckRevision(
	UNSIGNED16 idx	        /* index in the nmtSlaveList */
	CO_COMMA_LINE_PARA_DECL	/* number of CAN line 0..CO_MAX_CAN_LINES-1 */
    )
{
#ifdef NMTSTARTUP_DEBUG
    MY_PRINTF(DBGLVL_VENDOR, "nmtsSCheckRevision: %d - ", idx);
#endif /* NMTSTARTUP_DEBUG */

    /* SDO timeout occurred */
    if ((GL_PVAR(nmtStartupSlave)[idx].flags & NMTS_SFLAG_SDO_TIMEOUT) != 0) {

#ifdef NMTSTARTUP_DEBUG
	MY_PRINTF(DBGLVL_VENDOR, "timeout\n");
#endif /* NMTSTARTUP_DEBUG */

	GL_PVAR(nmtStartupSlave)[idx].fct = NMTS_SFCT_NO;
	return (NMT_SLAVE_N_WRONG_REVISION );
    }

    /* SDO abort received: subindex is optional */
    else if ((GL_PVAR(nmtStartupSlave)[idx].flags & NMTS_SFLAG_SDO_ABORT) != 0) {

#ifdef NMTSTARTUP_DEBUG
	MY_PRINTF(DBGLVL_VENDOR, "skipped\n");
#endif /* NMTSTARTUP_DEBUG */

        GL_PVAR(nmtStartupSlave)[idx].fct = NMTS_SFCT_REQUEST_IDENTITY_SERIAL_NUMBER;
    }

    else
    {
	if (GL_PVAR(nmtStartupSlave)[idx].objVal  != 0)  {
	    if (GL_PVAR(nmtStartupSlave)[idx].objVal != GL_PVAR(nmtStartupSlave)[idx].rxBuf) {
		GL_PVAR(nmtStartupSlave)[idx].fct = NMTS_SFCT_NO;

#ifdef NMTSTARTUP_DEBUG
		MY_PRINTF(DBGLVL_VENDOR, "bad revison\n");
#endif /* NMTSTARTUP_DEBUG */

		return (NMT_SLAVE_N_WRONG_REVISION );
	    }
	}
	GL_PVAR(nmtStartupSlave)[idx].fct =
		NMTS_SFCT_REQUEST_IDENTITY_SERIAL_NUMBER;

#ifdef NMTSTARTUP_DEBUG
	MY_PRINTF(DBGLVL_VENDOR, "ok\n");
#endif /* NMTSTARTUP_DEBUG */

    }
    return (NMT_RET_BUSY);
}


/****************************************************************************/
/**
* nmtsSRequestSerialNumber - request the serial number from the NMT slave
*
* \retval E_NMT_O_WRONG_SERIAL_NUMBER
* NMT master cannot send SDO to the NMT slave
* \retval NMT_RET_BUSY
* slave is ready for the next step of the NMT Startup process
*/
static UNSIGNED8 nmtsSRequestSerialNumber(
	UNSIGNED16 idx	        /* index in the nmtSlaveList */
	CO_COMMA_LINE_PARA_DECL	/* number of CAN line 0..CO_MAX_CAN_LINES-1 */
    )
{
UNSIGNED32 size;	/* current size of data in bytes */
RET_T	retVal;

#ifdef NMTSTARTUP_DEBUG
    MY_PRINTF(DBGLVL_VENDOR, "nmtsSRequestSerialNumber: %d - ", idx);
#endif /* NMTSTARTUP_DEBUG */

    /* there is no object for comparision, skip the checking of the
     * serial number */
    GL_PVAR(nmtStartupSlave)[idx].objVal = 0;

    if (getObjEntry(NMT_SERIAL_NUMBER_INDEX,
	GL_PVAR(nmtStartupSlave)[idx].nodeId,
	(UNSIGNED8 *)&GL_PVAR(nmtStartupSlave)[idx].objVal, &size, CO_TRUE CO_COMMA_LINE_PARA) != CO_OK)
    {
#ifdef NMTSTARTUP_DEBUG
	MY_PRINTF(DBGLVL_VENDOR, "skipped\n");
#endif /* NMTSTARTUP_DEBUG */

	GL_PVAR(nmtStartupSlave)[idx].fct = NMTS_SFCT_RESET_COMM;
    }

    /* the value of the object is 0, do not compare the object
     * value */
    else if (GL_PVAR(nmtStartupSlave)[idx].objVal == 0)
    {
#ifdef NMTSTARTUP_DEBUG
	MY_PRINTF(DBGLVL_VENDOR, "zero - skipped\n");
#endif /* NMTSTARTUP_DEBUG */

        GL_PVAR(nmtStartupSlave)[idx].fct = NMTS_SFCT_RESET_COMM;
    }

    /* request the serial number from the slave */
    else
    {
        retVal = readSdo(idx, IDENTITY_INDEX, 4,
	    (UNSIGNED8*)&GL_PVAR(nmtStartupSlave)[idx].rxBuf, 4
	    CO_COMMA_LINE_PARA);
	if (retVal == CO_OK) {
	    GL_PVAR(nmtStartupSlave)[idx].fct =
		NMTS_SFCT_CHECK_IDENTITY_SERIAL_NUMBER | NMTS_SFCT_NO;
	} else {
	    /* ignore busy error and try again next step */
	    if (retVal != CO_E_BUSY) {

#ifdef NMTSTARTUP_DEBUG
		MY_PRINTF(DBGLVL_VENDOR, "sdo req error\n");
#endif /* NMTSTARTUP_DEBUG */

		GL_PVAR(nmtStartupSlave)[idx].fct = NMTS_SFCT_NO;
		return (NMT_SLAVE_O_WRONG_SERIAL_NUMBER);
	    }
	}

#ifdef NMTSTARTUP_DEBUG
	MY_PRINTF(DBGLVL_VENDOR, "ok\n");
#endif /* NMTSTARTUP_DEBUG */

    }
    return (NMT_RET_BUSY);
}


/****************************************************************************/
/**
* nmtsSCheckSerialNumber - check the serial number from the NMT slave
*
* \retval E_NMT_O_WRONG_SERIAL_NUMBER
* NMT master did not receive an SDO response from the NMT slave or
* the serial number of the NMT slave is not the expected serial number
* \retval NMT_RET_BUSY
* slave is ready for the next step of the NMT Startup process
*/
static UNSIGNED8 nmtsSCheckSerialNumber(
	UNSIGNED16 idx	        /* index in the nmtSlaveList */
	CO_COMMA_LINE_PARA_DECL	/* number of CAN line 0..CO_MAX_CAN_LINES-1 */
    )
{
#ifdef NMTSTARTUP_DEBUG
    MY_PRINTF(DBGLVL_VENDOR, "nmtsSCheckSerialNumber: %d - ", idx);
#endif /* NMTSTARTUP_DEBUG */

    /* SDO timeout occurred */
    if ((GL_PVAR(nmtStartupSlave)[idx].flags & NMTS_SFLAG_SDO_TIMEOUT) != 0) {

#ifdef NMTSTARTUP_DEBUG
	MY_PRINTF(DBGLVL_VENDOR, "timeout\n");
#endif /* NMTSTARTUP_DEBUG */

	GL_PVAR(nmtStartupSlave)[idx].fct = NMTS_SFCT_NO;
	return (NMT_SLAVE_O_WRONG_SERIAL_NUMBER);
    }

    /* SDO abort received: subindex is optional */
    else if ((GL_PVAR(nmtStartupSlave)[idx].flags & NMTS_SFLAG_SDO_ABORT) != 0)
    {

#ifdef NMTSTARTUP_DEBUG
	MY_PRINTF(DBGLVL_VENDOR, "zero - skipped\n");
#endif /* NMTSTARTUP_DEBUG */

        GL_PVAR(nmtStartupSlave)[idx].fct = NMTS_SFCT_RESET_COMM;
    }

    else
    {
	if (GL_PVAR(nmtStartupSlave)[idx].objVal != 0)  {
	    if (GL_PVAR(nmtStartupSlave)[idx].objVal != GL_PVAR(nmtStartupSlave)[idx].rxBuf) {

#ifdef NMTSTARTUP_DEBUG
		MY_PRINTF(DBGLVL_VENDOR, "bad serial number\n");
#endif /* NMTSTARTUP_DEBUG */

		GL_PVAR(nmtStartupSlave)[idx].fct = NMTS_SFCT_NO;
		return (NMT_SLAVE_O_WRONG_SERIAL_NUMBER);
	    }
	}
	GL_PVAR(nmtStartupSlave)[idx].fct = NMTS_SFCT_RESET_COMM;

#ifdef NMTSTARTUP_DEBUG
	MY_PRINTF(DBGLVL_VENDOR, "ok\n");
#endif /* NMTSTARTUP_DEBUG */

    }
    return (NMT_RET_BUSY);
}


/****************************************************************************/
/**
* nmtsSResetComm - generate Reset Communication
*
* \retval NMT_RET_BUSY
* slave is ready for the next step of the NMT Startup process
*/
static UNSIGNED8 nmtsSResetComm(
	UNSIGNED16 idx	        /* index in the nmtSlaveList */
	CO_COMMA_REDCY_PARA_DECL
    )
{
NODE_STATE_T	hbState;

#ifdef NMTSTARTUP_DEBUG
    MY_PRINTF(DBGLVL_RSTCOMM, "nmtsSResetComm: %d - ", idx);
#endif /* NMTSTARTUP_DEBUG */

    /* resetComm if node is OPERATIONAL ? */
    if ((GL_PVAR(nmtStartupSlave)[idx].obj1F81 & NMT_RESET_COMMUNICATION_BIT)
		!= 0) {

#ifdef NMTSTARTUP_DEBUG
	MY_PRINTF(DBGLVL_RSTCOMM, "force reset comm for OPER\n");
#endif /* NMTSTARTUP_DEBUG */

	/* did we receive a heartbeat from this node ?*/
	hbState = getHbNodeState(GL_PVAR(nmtStartupSlave)[idx].nodeId
		CO_COMMA_REDCY_PARA);
	if (hbState == UNKNOWN)  {
	    /* no, try to start heartbeat for this node */
	    startHeartBeatReq(GL_PVAR(nmtStartupSlave)[idx].nodeId
		CO_COMMA_LINE_PARA);
	    setHeartBeatSignaling(GL_PVAR(nmtStartupSlave)[idx].nodeId
		CO_COMMA_LINE_PARA);

	    /* wait until node has sent HB state */
	    GL_PVAR(nmtStartupSlave)[idx].fct |= NMTS_SFCT_NO;

	} else {
	    /* we know the actual state */
	    if (hbState == OPERATIONAL)  {
		/* go to startErrorControl */
		GL_PVAR(nmtStartupSlave)[idx].fct = NMTS_SFCT_START_ERROR_CONTROL;
	    } else {
		/* send reset comm for this node */
		resetCommReq(0x80 + GL_PVAR(nmtStartupSlave)[idx].nodeId
			CO_COMMA_REDCY_PARA);

		/* wait until node has been booted */
		(void)addTimerEvent(&GL_PVAR(nmtStartupSlave)[idx].timer,
		    CONFIG_NMT_STARTUP_RESETCOMM_TIMEOUT,
		    CO_TIMER_TYPE_NMT_BOOT_TIME CO_COMMA_LINE_PARA);

		/* timeout can be interrupted by receive bootup message */
		GL_PVAR(nmtStartupSlave)[idx].fct =
			NMTS_SFCT_SOFTWARE_UPDATE | NMTS_SFCT_NO;
	    }
	}
    }
    else {

#ifdef NMTSTARTUP_DEBUG
	MY_PRINTF(DBGLVL_RSTCOMM, "nothing to do\n");
#endif /* NMTSTARTUP_DEBUG */

        GL_PVAR(nmtStartupSlave)[idx].fct = NMTS_SFCT_SOFTWARE_UPDATE;
    }
    return (NMT_RET_BUSY);
}


/****************************************************************************/
/**
* nmtsSUpdateSoftware - update software on the NMT slave
*
* \retval NMT_RET_BUSY
* slave is ready for the next step of the NMT Startup process
*/
static UNSIGNED8 nmtsSUpdateSoftware(
	UNSIGNED16 idx	        /* index in the nmtSlaveList */
	CO_COMMA_LINE_PARA_DECL	/* number of CAN line 0..CO_MAX_CAN_LINES-1 */
    )
{
#ifdef NMTSTARTUP_DEBUG
    MY_PRINTF(DBGLVL_SWUPDATE, "nmtsSUpdateSoftware: %d - ", idx);
#endif /* NMTSTARTUP_DEBUG */

    /* check for software version necessary ? */
    if ((GL_PVAR(nmtStartupSlave)[idx].obj1F81 & NMT_SOFTWARE_VERSION_BIT) != 0)  {
	/* The application is responsible for the update of the software.
	 * The NMT Startup process has to wait at the end of the software
	 * update. */

#ifdef NMTSTARTUP_DEBUG
	MY_PRINTF(DBGLVL_SWUPDATE, "call user indication\n", idx);
#endif /* NMTSTARTUP_DEBUG */

	GL_PVAR(nmtStartupSlave)[idx].fct =
		NMTS_SFCT_WAIT_SOFTWARE_UPDATE | NMTS_SFCT_NO;
	nmtStartupSlaveInd(GL_PVAR(nmtStartupSlave)[idx].nodeId,
		NMT_SLAVE_UPDATE_SOFTWARE
		CO_COMMA_LINE_PARA);
    } else {
	/* no version check necessary */
	GL_PVAR(nmtStartupSlave)[idx].fct = NMTS_SFCT_CONFIG;

#ifdef NMTSTARTUP_DEBUG
	MY_PRINTF(DBGLVL_SWUPDATE, "none\n", idx);
#endif /* NMTSTARTUP_DEBUG */

    }
    return (NMT_RET_BUSY);
}


/****************************************************************************/
/**
* nmtsSConfigureSlave - configure NMT slave
*
* \retval NMT_RET_BUSY
* slave is ready for the next step of the NMT Startup process
*/
static UNSIGNED8 nmtsSConfigureSlave(
	UNSIGNED16 idx	        /* index in the nmtSlaveList */
	CO_COMMA_LINE_PARA_DECL	/* number of CAN line 0..CO_MAX_CAN_LINES-1 */
    )
{
UNSIGNED32	date, time, size;
RET_T		retVal;

#ifdef NMTSTARTUP_DEBUG
    MY_PRINTF(DBGLVL_CFG, "nmtsSConfigureSlave: %d - ", idx);
#endif /* NMTSTARTUP_DEBUG */

    retVal = getObjEntry(EXPECTED_CONFIG_DATE_INDEX,
	GL_PVAR(nmtStartupSlave)[idx].nodeId,
	(UNSIGNED8 *)&date, &size, CO_TRUE CO_COMMA_LINE_PARA);
    if (retVal != CO_OK)  {
	date = 0;
    }
    retVal = getObjEntry(EXPECTED_CONFIG_TIME_INDEX,
	GL_PVAR(nmtStartupSlave)[idx].nodeId,
	(UNSIGNED8 *)&time, &size, CO_TRUE CO_COMMA_LINE_PARA);
    if (retVal != CO_OK)  {
	time = 0;
    }

    /* are the expected configuration date and time == 0 */
    if ((date == 0) && (time == 0)) {

#ifdef NMTSTARTUP_DEBUG
	MY_PRINTF(DBGLVL_CFG, "no cfg available\n");
#endif /* NMTSTARTUP_DEBUG */

	/* call user function update config */
	GL_PVAR(nmtStartupSlave)[idx].fct = NMTS_SFCT_UPDATE_CONFIG;
	return (NMT_RET_BUSY);
    }


    /* expected configuration date and time is != 0 */

    /* request the entries from the slave */
    retVal = readSdo(idx, VERIFY_CONFIG_INDEX, 1,
	(UNSIGNED8*)&GL_PVAR(nmtStartupSlave)[idx].rxBuf, 4 CO_COMMA_LINE_PARA);
    if (retVal == CO_OK) {
	GL_PVAR(nmtStartupSlave)[idx].fct =
		NMTS_SFCT_CHECK_CONFIG1 | NMTS_SFCT_NO;
    } else {
	/* ignore busy error and try again next step */
	if (retVal != CO_E_BUSY) {

#ifdef NMTSTARTUP_DEBUG
	    MY_PRINTF(DBGLVL_CFG, "sdo req error\n");
#endif /* NMTSTARTUP_DEBUG */

	    GL_PVAR(nmtStartupSlave)[idx].fct = NMTS_SFCT_NO;
	    return (NMT_SLAVE_J_CONFIG_DOWNLOAD_FAILED);
	}
    }

#ifdef NMTSTARTUP_DEBUG
    MY_PRINTF(DBGLVL_CFG, "request config 1 from device\n", idx);
#endif /* NMTSTARTUP_DEBUG */

    return (NMT_RET_BUSY);
}


/****************************************************************************/
/**
* nmtsSCheckConfigSlave1 - check config 1 NMT slave
*
* \retval NMT_RET_BUSY
* slave is ready for the next step of the NMT Startup process
*/
static UNSIGNED8 nmtsSCheckConfigSlave1(
	UNSIGNED16 idx	        /* index in the nmtSlaveList */
	CO_COMMA_LINE_PARA_DECL	/* number of CAN line 0..CO_MAX_CAN_LINES-1 */
    )
{
UNSIGNED32	objVal, size;
RET_T		retVal;

#ifdef NMTSTARTUP_DEBUG
    MY_PRINTF(DBGLVL_CFG, "nmtsSCheckConfigSlave1: %d - ", idx);
#endif /* NMTSTARTUP_DEBUG */

    /* SDO timeout occurred */
    if ((GL_PVAR(nmtStartupSlave)[idx].flags & NMTS_SFLAG_SDO_TIMEOUT) != 0) {

#ifdef NMTSTARTUP_DEBUG
	MY_PRINTF(DBGLVL_CFG, "timeout\n");
#endif /* NMTSTARTUP_DEBUG */

	GL_PVAR(nmtStartupSlave)[idx].fct = NMTS_SFCT_NO;
	return (NMT_SLAVE_J_CONFIG_DOWNLOAD_FAILED);
    }

    /* SDO abort received: */
    else if ((GL_PVAR(nmtStartupSlave)[idx].flags & NMTS_SFLAG_SDO_ABORT) != 0)
    {

#ifdef NMTSTARTUP_DEBUG
	MY_PRINTF(DBGLVL_CFG, "zero - skipped\n");
#endif /* NMTSTARTUP_DEBUG */

	/* call user function update config */
	GL_PVAR(nmtStartupSlave)[idx].fct = NMTS_SFCT_UPDATE_CONFIG;
    }

    else
    {
	(void)getObjEntry(EXPECTED_CONFIG_DATE_INDEX,
		GL_PVAR(nmtStartupSlave)[idx].nodeId,
		(UNSIGNED8*)&objVal, &size, CO_TRUE CO_COMMA_LINE_PARA);

	if (objVal != 0)  {
	    if (objVal != GL_PVAR(nmtStartupSlave)[idx].rxBuf) {

#ifdef NMTSTARTUP_DEBUG
		MY_PRINTF(DBGLVL_CFG, "bad config date\n");
#endif /* NMTSTARTUP_DEBUG */

		/* call user function update config */
		GL_PVAR(nmtStartupSlave)[idx].fct = NMTS_SFCT_UPDATE_CONFIG;
		return (NMT_RET_BUSY);
	    }
	}

#ifdef NMTSTARTUP_DEBUG
	MY_PRINTF(DBGLVL_CFG, "ok\n");
#endif /* NMTSTARTUP_DEBUG */

    }

    /* request the entries from the slave */
    retVal = readSdo(idx, VERIFY_CONFIG_INDEX, 2,
	(UNSIGNED8*)&GL_PVAR(nmtStartupSlave)[idx].rxBuf, 4 CO_COMMA_LINE_PARA);
    if (retVal == CO_OK) {
	GL_PVAR(nmtStartupSlave)[idx].fct =
		NMTS_SFCT_CHECK_CONFIG2 | NMTS_SFCT_NO;
    } else {
	/* ignore busy error and try again next step */
	if (retVal != CO_E_BUSY) {

#ifdef NMTSTARTUP_DEBUG
	    MY_PRINTF(DBGLVL_CFG, "sdo req error\n");
#endif /* NMTSTARTUP_DEBUG */

	    GL_PVAR(nmtStartupSlave)[idx].fct = NMTS_SFCT_NO;
	    return (NMT_SLAVE_J_CONFIG_DOWNLOAD_FAILED);
	}
    }

#ifdef NMTSTARTUP_DEBUG
    MY_PRINTF(DBGLVL_CFG, "request config 2 from device\n", idx);
#endif /* NMTSTARTUP_DEBUG */

    return (NMT_RET_BUSY);
}


/****************************************************************************/
/**
* nmtsSCheckConfigSlave2 - check config 2 NMT slave
*
* \retval NMT_RET_BUSY
* slave is ready for the next step of the NMT Startup process
*/
static UNSIGNED8 nmtsSCheckConfigSlave2(
	UNSIGNED16 idx	        /* index in the nmtSlaveList */
	CO_COMMA_LINE_PARA_DECL	/* number of CAN line 0..CO_MAX_CAN_LINES-1 */
    )
{
UNSIGNED32	objVal, size;

#ifdef NMTSTARTUP_DEBUG
    MY_PRINTF(DBGLVL_CFG, "nmtsSCheckConfigSlave2: %d - ", idx);
#endif /* NMTSTARTUP_DEBUG */

    /* SDO timeout occurred */
    if ((GL_PVAR(nmtStartupSlave)[idx].flags & NMTS_SFLAG_SDO_TIMEOUT) != 0) {

#ifdef NMTSTARTUP_DEBUG
	MY_PRINTF(DBGLVL_CFG, "timeout\n");
#endif /* NMTSTARTUP_DEBUG */

	GL_PVAR(nmtStartupSlave)[idx].fct = NMTS_SFCT_NO;
	return (NMT_SLAVE_J_CONFIG_DOWNLOAD_FAILED);
    }

    /* SDO abort received: */
    else if ((GL_PVAR(nmtStartupSlave)[idx].flags & NMTS_SFLAG_SDO_ABORT) != 0)
    {

#ifdef NMTSTARTUP_DEBUG
	MY_PRINTF(DBGLVL_CFG, "zero - skipped\n");
#endif /* NMTSTARTUP_DEBUG */

	/* call user function update config */
	GL_PVAR(nmtStartupSlave)[idx].fct = NMTS_SFCT_UPDATE_CONFIG;
    }

    else
    {
	(void)getObjEntry(EXPECTED_CONFIG_TIME_INDEX,
		GL_PVAR(nmtStartupSlave)[idx].nodeId,
		(UNSIGNED8*)&objVal, &size, CO_TRUE CO_COMMA_LINE_PARA);

	if (objVal != 0)  {
	    if (objVal != GL_PVAR(nmtStartupSlave)[idx].rxBuf) {

#ifdef NMTSTARTUP_DEBUG
		MY_PRINTF(DBGLVL_CFG, "bad config date\n");
#endif /* NMTSTARTUP_DEBUG */

		/* call user function update config */
		GL_PVAR(nmtStartupSlave)[idx].fct = NMTS_SFCT_UPDATE_CONFIG;
		return (NMT_RET_BUSY);
	    }
	}
	GL_PVAR(nmtStartupSlave)[idx].fct = NMTS_SFCT_START_ERROR_CONTROL;

#ifdef NMTSTARTUP_DEBUG
	MY_PRINTF(DBGLVL_CFG, "ok\n");
#endif /* NMTSTARTUP_DEBUG */

    }

    return (NMT_RET_BUSY);
}


/****************************************************************************/
/**
* nmtsSUpdateConfigSlave - Update configure NMT slave
*
* \retval NMT_RET_BUSY
* slave is ready for the next step of the NMT Startup process
*/
static UNSIGNED8 nmtsSUpdateConfigSlave(
	UNSIGNED16 idx	        /* index in the nmtSlaveList */
	CO_COMMA_LINE_PARA_DECL	/* number of CAN line 0..CO_MAX_CAN_LINES-1 */
    )
{
    /* The application is responsible for the configuration of each NMT
     * slave. The NMT Startup process for this NMT slave has to wait at
     * the end of the configuration. */

#ifdef NMTSTARTUP_DEBUG
    MY_PRINTF(DBGLVL_CFG, "nmtsSUpdateConfigSlave: %d - wait for user\n", idx);
#endif /* NMTSTARTUP_DEBUG */

    GL_PVAR(nmtStartupSlave)[idx].fct = NMTS_SFCT_WAIT_CONFIG | NMTS_SFCT_NO;

    nmtStartupSlaveInd(GL_PVAR(nmtStartupSlave)[idx].nodeId,
	NMT_SLAVE_UPDATE_CONFIG CO_COMMA_LINE_PARA);

    return (NMT_RET_BUSY);
}


/****************************************************************************/
/**
* nmtsSStartErrorControl - start error control
*
* \retval NMT_RET_OK
* NMT slave is started (status L)
* \retval NMT_RET_BUSY
* slave is ready for the next step of the NMT Startup process
*/
static UNSIGNED8 nmtsSStartErrorControl(
	UNSIGNED16 idx	        /* index in the nmtSlaveList */
	CO_COMMA_LINE_PARA_DECL	/* number of CAN line 0..CO_MAX_CAN_LINES-1 */
    )
{
# ifdef CONFIG_HEARTBEAT_CONSUMER
UNSIGNED8	hbIdx;	/* index of object 0x1016 for node */
HB_CONS_T	*pHBCons;
# endif

#ifdef NMTSTARTUP_DEBUG
    MY_PRINTF(DBGLVL_ERRCTRL, "nmtsSStartErrorControl: %d - ", idx);
#endif /* NMTSTARTUP_DEBUG */

    /* Heartbeat */
# ifdef CONFIG_HEARTBEAT_CONSUMER
    hbIdx = getHeartBeatIndex(GL_PVAR(nmtStartupSlave)[idx].nodeId
	    CO_COMMA_LINE_PARA);
    if (hbIdx != 0xFF) {

	pHBCons = &GL_PVAR(hbConsList)[hbIdx
# ifdef CONFIG_MULT_LINES
	    + GL_ARRAY(co_hbConsLineOffs)
# endif /* CONFIG_MULT_LINES */
 	];

	/* check, if HB time unequal 0 */
	if (pHBCons->timer.timerVal != 0)  {
	    /* check, if heartbeat is active */
	    if ((pHBCons->mflags & GUARDFLAG_HB_ACTIVE) != 0) {
		/* heartbeat is already active */
		GL_PVAR(nmtStartupSlave)[idx].fct = NMTS_SFCT_START_NODE;

#ifdef NMTSTARTUP_DEBUG
		MY_PRINTF(DBGLVL_ERRCTRL, "HB ok\n");
#endif /* NMTSTARTUP_DEBUG */

	    } else {
		startHeartBeatReq(GL_PVAR(nmtStartupSlave)[idx].nodeId
			CO_COMMA_LINE_PARA);
		/* wait for next heartbeat */
		GL_PVAR(nmtStartupSlave)[idx].fct =
			NMTS_SFCT_START_NODE | NMTS_SFCT_NO;

#ifdef NMTSTARTUP_DEBUG
		MY_PRINTF(DBGLVL_ERRCTRL, "warte auf HB\n");
#endif /* NMTSTARTUP_DEBUG */

	    }
	    return (NMT_RET_BUSY);
	}
    } else
# endif
    {
	/* Node Guarding */
# ifdef CONFIG_NODE_GUARDING
	/* guarding para available ? */
	if ((GL_PVAR(nmtStartupSlave)[idx].obj1F81 & 0xFFFF0000) != 0u) {
	    if (setGuardTimePara(GL_PVAR(nmtStartupSlave)[idx].nodeId,
		(UNSIGNED16)(GL_PVAR(nmtStartupSlave)[idx].obj1F81 >> 16),
		(UNSIGNED8)((GL_PVAR(nmtStartupSlave)[idx].obj1F81 >> 8) & 0xff)
		CO_COMMA_LINE_PARA) != CO_OK)  {
		return (NMT_RET_GUARD_INIT_ERROR);
	    }
	    startNodeGuardReq(GL_PVAR(nmtStartupSlave)[idx].nodeId
			CO_COMMA_LINE_PARA);

	    /* wait for guarding */
	    GL_PVAR(nmtStartupSlave)[idx].fct =
		NMTS_SFCT_START_NODE | NMTS_SFCT_NO;

#ifdef NMTSTARTUP_DEBUG
	    MY_PRINTF(DBGLVL_ERRCTRL, "warte auf Guarding\n");
#endif /* NMTSTARTUP_DEBUG */
	    return (NMT_RET_BUSY);

	} else {
	    /* heartbeat is already active */

#ifdef NMTSTARTUP_DEBUG
	    MY_PRINTF(DBGLVL_ERRCTRL, "Guarding ok\n");
#endif /* NMTSTARTUP_DEBUG */

	}
# endif
    }

    GL_PVAR(nmtStartupSlave)[idx].fct = NMTS_SFCT_START_NODE;

#ifdef NMTSTARTUP_DEBUG
    MY_PRINTF(DBGLVL_NMT, "none\n");
#endif /* NMTSTARTUP_DEBUG */

    return (NMT_RET_BUSY);
}


/****************************************************************************/
/**
* nmtsSStartNode - change NMT slave into OPERATIONAL
*
* \retval NMT_RET_OK
* NMT slave is started
*/
static UNSIGNED8 nmtsSStartNode(
	UNSIGNED16 idx	        /* index in the nmtSlaveList */
	CO_COMMA_REDCY_PARA_DECL
    )
{
#ifdef NMTSTARTUP_DEBUG
    MY_PRINTF(DBGLVL_NMT, "nmtsSStartNode: %d ", idx);
#endif /* NMTSTARTUP_DEBUG */

    /* enter OPERATIONAL only if bit 3 is not set */
    if ((GL_ARRAY(nmtsMaster).obj1F80 & NMT_STARTUP_NOT_START_NODE_BIT) == 0) {
	/* start node individually only if global start flag is unset
	 * or the master is in OPERATIONAL */
	if (((GL_ARRAY(nmtsMaster).obj1F80 & NMT_STARTUP_START_ALL_NODE_BIT)
		!= 0)
	 &&
	    (GL_ARRAY(co_Node).eState != OPERATIONAL))  {
	    /* dont start any slave here */
	} else {

#ifdef NMTSTARTUP_DEBUG
	    MY_PRINTF(DBGLVL_NMT, "set to OPERATIONAL %d\n", idx);
	    MY_PRINTF(DBGLVL_NMT, "node %d is in %d",
		    GL_PVAR(nmtStartupSlave)[idx].nodeId,
		    getRemoteNodeState(GL_PVAR(nmtStartupSlave)[idx].nodeId));
#endif /* NMTSTARTUP_DEBUG */

	    startRemoteNodeReq(GL_PVAR(nmtStartupSlave)[idx].nodeId | 0x80
		    CO_COMMA_REDCY_PARA);

#ifdef NMTSTARTUP_DEBUG
	    MY_PRINTF(DBGLVL_NMT, " - and now should be in %d\n",
		    getRemoteNodeState(GL_PVAR(nmtStartupSlave)[idx].nodeId));
	    MY_PRINTF(DBGLVL_NMT, "*** waiting.....\n");
#endif /* NMTSTARTUP_DEBUG */

	    /* wait until node has sent HB state OPERATIONAL */
	    GL_PVAR(nmtStartupSlave)[idx].fct =
		NMTS_SFCT_FINISHED_OK | NMTS_SFCT_NO;

	    setHeartBeatSignaling(GL_PVAR(nmtStartupSlave)[idx].nodeId
		CO_COMMA_LINE_PARA);

	    return (NMT_RET_BUSY);
	}
    }

    GL_PVAR(nmtStartupSlave)[idx].fct = NMTS_SFCT_FINISHED_OK;

    return (NMT_RET_BUSY);
}


/****************************************************************************/
/**
* nmtsSFinishedOk - finish Bootup process
*
* \retval NMT_RET_OK
* NMT slave bootup was finished
*/
static UNSIGNED8 nmtsSFinishedOk(
	UNSIGNED16 idx	        /* index in the nmtSlaveList */
	CO_COMMA_LINE_PARA_DECL	/* number of CAN line 0..CO_MAX_CAN_LINES-1 */
    )
{
#ifdef NMTSTARTUP_DEBUG
    MY_PRINTF(DBGLVL_NMT, "nmtsSFinished: %d ", idx);
#endif /* NMTSTARTUP_DEBUG */

    GL_PVAR(nmtStartupSlave)[idx].fct = NMTS_SFCT_NO;

#ifdef NMTSTARTUP_DEBUG
    MY_PRINTF(DBGLVL_NMT, "startup finished\n", idx);
#endif /* NMTSTARTUP_DEBUG */

    nmtStartupSlaveInd(GL_PVAR(nmtStartupSlave)[idx].nodeId,
	NMT_SLAVE_BOOT_FINISHED CO_COMMA_LINE_PARA);

    return (NMT_RET_OK);
}


/****************************************************************************/
/**
* readSdo - start sdo read request
*
* check if sdo is not busy
* setup sdo cob-ids
* start sdo read request
*
* \return
*	RET_T
*/
static RET_T readSdo(
	UNSIGNED16	sIdx,		/* index at slave list */
	UNSIGNED16	index,		/* index */
	UNSIGNED8	subIndex,	/* subIndex */
	UNSIGNED8	*buf,		/* pointer to buffer */
	UNSIGNED8	len		/* buffer size */
	CO_COMMA_LINE_PARA_DECL	/* number of CAN line 0..CO_MAX_CAN_LINES-1 */
    )
{
SDO_CLIENT_T	*pSdo;		/* pointer to current sdo */
UNSIGNED8	sdoNr;
RET_T		retVal;
UNSIGNED32      sdoTimeOut = CONFIG_NMT_STARTUP_SDO_TIMEOUT * 10u;
#ifdef CO_CONFIG_NMTSTART_SDO_TIMEOUT_IND
UNSIGNED16      userTimeOut = 0;
#endif /* CO_CONFIG_NMTSTART_SDO_TIMEOUT_IND */


    /* we use the sdo-nr depending on the index of our nmt slave list */
#ifdef NMTSTARTUP_DEBUG
    MY_PRINTF(DBGLVL_VENDOR, "\nreadSdo: idx %d - ", sIdx);
#endif /* NMTSTARTUP_DEBUG */

    /* calculate sdo number */
#ifdef CONFIG_MULT_LINES
    sdoNr = (UNSIGNED8)(sIdx - GL_ARRAY(co_nmtSlaveLineOffs));
#else /* CONFIG_MULT_LINES */
    sdoNr = (UNSIGNED8)sIdx;
#endif /* CONFIG_MULT_LINES */

    /* check if sdo is not busy */
    pSdo = searchForClientSdoNr(sdoNr + 1 CO_COMMA_LINE_PARA);
    if (pSdo == NULL) {
	return(CO_E_NOT_EXIST);
    }

    /* test for active transfer */
    if (pSdo->sdo.state != SDOSTATE_READY)  {
	/* test disable flag */
	if (pSdo->sdo.state != SDOSTATE_DISABLED)  {
	    return(CO_E_BUSY);
	}
    }

    /* configure the sdo if necessary */
    retVal = pcoNmtStartCheckSetSdoCob( pSdo, CO_COB_SDO_TX,
		CO_COBID_CSDO + GL_PVAR(nmtStartupSlave)[sIdx].nodeId, sdoNr
                CO_COMMA_LINE_PARA );
    if ( CO_OK != retVal ) {
        return retVal;
    }

    /* configure the sdo if necessary */
    retVal = pcoNmtStartCheckSetSdoCob( pSdo, CO_COB_SDO_RX,
		CO_COBID_SSDO + GL_PVAR(nmtStartupSlave)[sIdx].nodeId, sdoNr
                CO_COMMA_LINE_PARA );
    if ( CO_OK != retVal ) {
        return retVal;
    }

#ifdef CO_CONFIG_NMTSTART_SDO_TIMEOUT_IND
    retVal = coUserNmtStartupSdoTimeInd(GL_PVAR(nmtStartupSlave)[sIdx].nodeId, index, subIndex
        ,&userTimeOut CO_COMMA_LINE_PARA );
    if ( CO_OK != retVal ) {
        /* next try with next flushMbox */
        return(CO_E_BUSY);
    }

    /* if the user doesnt change the value use default */
    if ( 0u != userTimeOut ) {
        sdoTimeOut = userTimeOut * 10u;
    }
#endif /* CO_CONFIG_NMTSTART_SDO_TIMEOUT_IND */

    /* start sdo read request */
    retVal = readSdoReq(CO_NUM_SDO | (sdoNr + 1), index, subIndex,  buf, len,
	sdoTimeOut CO_COMMA_LINE_PARA);

    if (retVal == CO_OK)  {
	/* mark we have started this transfer */
	GL_PVAR(nmtStartupSlave)[sIdx].flags |= NMTS_SFLAG_SDO_IN_USE;
    }

    return(retVal);
}

/****************************************************************************/
/**
* \brief pcoNmtStartCheckSetSdoCob - checks if cob already set
*
* This function tests if the cob is alredy set and the SDO is usable.
* If not it will configure the SDO for the new cob.
*
* \return
*
*/

static RET_T pcoNmtStartCheckSetSdoCob(
	SDO_CLIENT_T *pCSdo,    /* pointer to client sdo */
	COB_KIND_T cobType,	/* kind of use */
	UNSIGNED32 newCob,      /* new Cob */
        UNSIGNED8  sdoNr	/* sdo number to edit the object */
	CO_COMMA_LINE_PARA_DECL )
{
OBJDIR_T 	*pObj = NULL;
UNSIGNED8 	subIndex = 0u;
COB_T    	*pCob = NULL;
RET_T    	retVal = CO_OK;
UNSIGNED32      *pData = NULL;
UNSIGNED32      size = 0u;


     (void)getObjPtrAtIndex( CSDO_PARA_BASE_INDEX + sdoNr, &pObj CO_COMMA_LINE_PARA);
     /* is it the TX cob */
     if ( CO_COB_SDO_TX == cobType ) {

         subIndex = 1u;
         pCob = pCSdo->sdo.pTrCOB;

     } else if ( CO_COB_SDO_RX == cobType ){

         pCob = pCSdo->sdo.pRecCOB;
         subIndex = 2u;

     } else {
         return CO_E_NOT_EXIST;
     }

     if ( (pCob->cobId != newCob) || (pCSdo->sdo.state == SDOSTATE_DISABLED) ) {

        /* first disable */
        retVal = getObjPtrAddr(pObj,CSDO_PARA_BASE_INDEX + sdoNr, subIndex, (UNSIGNED8 **)&pData, &size
            CO_COMMA_LINE_PARA);
        if ( CO_OK != retVal ) {
            return retVal;
        }

        *pData = SDO_NO_VALID_BIT;

        retVal = pcoSetSdoPtrCobId(&pCSdo->sdo, *pData, CLIENT, cobType
                CO_COMMA_LINE_PARA);
        if ( CO_OK != retVal ) {
            return retVal;
        }

        /* now enable with new cob */
        *pData = newCob;

        retVal = pcoSetSdoPtrCobId(&pCSdo->sdo, *pData, CLIENT, cobType
            CO_COMMA_LINE_PARA);
        if ( CO_OK != retVal ) {
            return retVal;
        }
     }

     return retVal;

}


/****************************************************************************/
/**
*++ \brief nmtStartupContReq - continue request
*-- \brief nmtStartupContReq - Fortsetzung anfordern
*
*++ The NMT Startup process can be continued after an application-specific
*++ NMT Startup action is finished.
*++ The following action codes are supported:
*++     E_NMT_MASTER_START: The NMT master is started.
*++     NMT_SLAVE_UPDATE_SOFTWARE: The software is updated.
*++     NMT_SLAVE_UPDATE_CONFIG: The NMT slave with node-ID is configured.
*-- Der NMT Startup Prozess kann nach der Ausführung applikations-spezifischer
*-- Aktionen fortgesetzt werden.
*-- Die folgenden Aktionen werden unterstützt:
*--     E_NMT_MASTER_START: Der NMT Master wurde gestartet.
*--     NMT_SLAVE_UPDATE_SOFTWARE: Die Software wurde aktualisiert.
*--     NMT_SLAVE_UPDATE_CONFIG: Der NMT Slave mit der Node-ID wurde konfiguriert.
*
* \return
*++ nothing
*-- keine
*/
void nmtStartupContReq(
	UNSIGNED8 nodeId,	/**< node-id */
	UNSIGNED8 actionCode	/**< code of the action */
	CO_COMMA_REDCY_PARA_DECL
    )
{
UNSIGNED8  i;
UNSIGNED16 idx;	        /* index in the nmtSlaveList */

    /*--- continue after the finishing of an action ---*/
    if (actionCode == NMT_CONT_START_MASTER)  {
	/* the NMT master can changed into the state OPERATIONAL */
	if ((GL_ARRAY(nmtsMaster).fct == NMTS_MFCT_WAIT_START_MASTER)
	 || (GL_ARRAY(nmtsMaster).fct == NMTS_MFCT_START_MASTER)) {

#ifdef NMTSTARTUP_DEBUG
	    MY_PRINTF(DBGLVL_MASTER, "nmtStartupContReq: start master now\n");
#endif /* NMTSTARTUP_DEBUG */

	    startRemoteNodeReq(GL_ARRAY(coNodeId) CO_COMMA_REDCY_PARA);

	    GL_ARRAY(nmtsMaster).fct = NMTS_MFCT_START_ALL_NODES;
	    SET_COLIB_FLAG(COFLAG_NMT_STARTUP_MANAGER);
	}
    } else {

	i = getNmtStartupSlaveIndex(nodeId CO_COMMA_LINE_PARA);
	if (i == 0xFF) {
	    /* node-id not found in nmtSlaveList */
            return;
        }

	idx = i;
#ifdef CONFIG_MULT_LINES
	idx += GL_ARRAY(co_nmtSlaveLineOffs);
#endif /* CONFIG_MULT_LINES */

	/* software update on the NMT slave is finished */
	if  (actionCode == NMT_CONT_UPDATE_SOFTWARE)  {
	    if (GL_PVAR(nmtStartupSlave)[idx].fct
		!= (NMTS_SFCT_WAIT_SOFTWARE_UPDATE | NMTS_SFCT_NO))  {
		return;
	    }

#ifdef NMTSTARTUP_DEBUG
	    MY_PRINTF(DBGLVL_SWUPDATE, "nmtStartupContReq: sw update ok %d\n", idx);
#endif /* NMTSTARTUP_DEBUG */

	    GL_PVAR(nmtStartupSlave)[idx].fct = NMTS_SFCT_CONFIG;
	} else

	/* configuration of the NMT slave is finished */
	if  (actionCode == NMT_CONT_UPDATE_CONFIG)  {
	    if (GL_PVAR(nmtStartupSlave)[idx].fct
		!= (NMTS_SFCT_WAIT_CONFIG | NMTS_SFCT_NO))  {
		return;
	    }

#ifdef NMTSTARTUP_DEBUG
	    MY_PRINTF(DBGLVL_CFG, "nmtStartupContReq: cfg ok %d\n", idx);
#endif /* NMTSTARTUP_DEBUG */

	    GL_PVAR(nmtStartupSlave)[idx].fct = NMTS_SFCT_START_ERROR_CONTROL;
	}

	/* continue bootup only if function is availble */
	if (GL_ARRAY(nmtsMaster).fct == NMTS_MFCT_STANDBY)  {
	    GL_ARRAY(nmtsMaster).fct = NMTS_MFCT_POLL_OPTIONAL_SLAVES;
	}
	SET_COLIB_FLAG(COFLAG_NMT_STARTUP_MANAGER);
    }
}


/*******************************************************************
*
* getNmtStartupSlaveIndex - searches for NMT Slave entry
*
* \internal
*
* This function checks the NMT Startup Slave list
* and returns the index at the list.
* if the node isn't at the list, 0xff is returned
*
* RETURNS
* \retval subIndex
*	subindex for the given node
* \retval 0xff
*	no entry found and no more entries free
*
*/

static UNSIGNED8 getNmtStartupSlaveIndex(
	UNSIGNED8 nodeId	/* node id */
	CO_COMMA_LINE_PARA_DECL	/* number of CAN line 0..CO_MAX_CAN_LINES-1 */
    )
{
NMT_SLAVE_STARTUP_T	*pSlaveList;

#  ifdef CONFIG_FAST_SORT
INTEGER8 	found = 0;
INTEGER16	low, mid = 0, high;
UNSIGNED8	*pIdxList;
#  else /* CONFIG_FAST_SORT */
UNSIGNED8	i;		/* loop variable */
#  endif /* CONFIG_FAST_SORT */


#  ifdef CONFIG_MULT_LINES
    pSlaveList = &GL_PVAR(nmtStartupSlave)[GL_ARRAY(co_nmtSlaveLineOffs)];
#  else /* CONFIG_MULT_LINES */
    pSlaveList = &GL_PVAR(nmtStartupSlave)[0];
#  endif /* CONFIG_MULT_LINES */

    /* ignore node 0 */
    if (nodeId == 0)  {
	return(0xff);
    }

#  ifdef CONFIG_FAST_SORT

    high = GL_ARRAY(nmtsMaster).slaveCnt - 1;
    low = 0;
#   ifdef CONFIG_MULT_LINES
    pIdxList = &GL_PVAR(nmtStartupSlaveIdxList)[GL_ARRAY(co_nmtSlaveLineOffs)];
#   else /* CONFIG_MULT_LINES */
    pIdxList = &GL_PVAR(nmtStartupSlaveIdxList)[0];
#   endif /* CONFIG_MULT_LINES */

    while (found == 0)  {
	if (high >= low) {
	    mid = (high + low) / 2;
	    if (pSlaveList[pIdxList[mid]].nodeId == nodeId)  {
		found = 1;
	    } else {
		if (pSlaveList[pIdxList[mid]].nodeId > nodeId) {
		    high = mid - 1;
		} else  {
		    low = mid + 1;
		}
	    }
	} else {
	    found = -1;
	}
    }
    if (found < 0)  {
	return(0xff);
    } else {
	return(pIdxList[mid]);
    }
#  else /* CONFIG_FAST_SORT */

    for (i = 0; i < GL_ARRAY(nmtsMaster).slaveCnt; i++)  {
	/* get the entry */
	if (pSlaveList[i].nodeId == nodeId)  {
	    return(i);
	}
    }
    return(0xff);
#  endif /* CONFIG_FAST_SORT */
}


/*******************************************************************/
/**
*++ \brief getNmtStartupSdoNr - returns used sdo number for NMT Slave
*-- \brief getNmtStartupSdoNr - liefert genutzte SDO Nummer für NMT Slave
*
*++ This function returns the sdo number,
*++ which is used for the access of the NMT slave <nodeId>
*++ from the startup-manager.
*++ The application should use the same sdo channel.
*-- Diese Funktion liefert die SDO Nummer,
*-- die für den Zugriff auf den NMT-Slave <nodeId>
*-- vom Startup-Manager genutzt wird.
*-- Die Applikation sollte nach Möglichkeit denselben SDO Kanal benutzen.
*
* RETURNS
* \retval 1..127
*++	sdo channel
*--	SDO Kanal
* \retval 0x0
*++	no entry found
*--	kein Eintrag gefunden
*
*/
UNSIGNED8 getNmtStartupSdoNr(
	UNSIGNED8 nodeId	/**< node id */
	CO_COMMA_LINE_PARA_DECL	/**< number of CAN line 0..CO_MAX_CAN_LINES-1 */
    )
{
    return(getNmtStartupSlaveIndex(nodeId CO_COMMA_LINE_PARA) + 1);
}


/*******************************************************************/
/**
*++ \brief getNmtStartupNodeId - returns used Node Id for this SDO
*-- \brief getNmtStartupNodeId - liefert die Knotennummer für das genutzte SDO
*
*++ This function returns the node Id
*++ which is addressed by this SDO
*++ from the startup-manager.
*++ The application should use the same sdo channel.
*-- Diese Funktion liefert die Knotennummer
*-- welche vom Startup-Manager für dieses SDO genutzt wird.
*-- Die Applikation sollte nach Möglichkeit denselben SDO Kanal benutzen.
*
* RETURNS
* \retval 1..127
*++	node Id
*--	Node-ID
* \retval 0
*++	no entry found
*--	kein Eintrag gefunden
*
*/
UNSIGNED8 getNmtStartupNodeId(
	UNSIGNED8	sdoNr	/**< SDO Number */
	CO_COMMA_LINE_PARA_DECL /**< number of CAN line 0..CO_MAX_CAN_LINES-1 */
    )
{
UNSIGNED16	sIdx;

    /* check correct value range for sdoNr */
    if ((sdoNr < 1) || (sdoNr > GL_ARRAY(nmtsMaster).slaveCnt)) {
	return(0);
    }

    sIdx = sdoNr - 1;
#ifdef CONFIG_MULT_LINES
    sIdx += GL_ARRAY(co_nmtSlaveLineOffs);
#endif /* CONFIG_MULT_LINES */

    return(GL_PVAR(nmtStartupSlave)[sIdx].nodeId);
}

#endif /* CONFIG_MASTER && CONFIG_NMT_STARTUP_MANAGER */



#ifdef NMTSTARTUP_DEBUG
/*
* print debug information depending on debug level
*/
#include <sys/time.h>
static void debug_print(
	UNSIGNED16 level,	/* debug level */
        char *pfmt,             /* format string */
        ...                     /* optional arguments */
    )
{
va_list args;
struct timeval tv;

    if ((level & debugLevel) == 0)  {
        va_start(args, pfmt);
        va_end(args);
        return;
    }

    gettimeofday(&tv, NULL);
    printf("%d.%06d: ", (int)tv.tv_sec, (int)tv.tv_usec);

    va_start(args, pfmt);
    vprintf(pfmt, args);
    va_end(args);
}


static char *getCmdStrg(
	UNSIGNED8	cmd
    )
{
typedef struct {
	UNSIGNED8	fct;
	char		*strg;
} CMD_STRING;
CMD_STRING cmdStrg[] = {
    { NMTS_SFCT_START, "start" },
    { NMTS_SFCT_REQUEST_DEVICE_TYPE, "request device type" },
    { NMTS_SFCT_REQUEST_IDENTITY_VENDOR, "request vendor" },
    { NMTS_SFCT_CHECK_IDENTITY_VENDOR, "check vendor" },
    { NMTS_SFCT_REQUEST_IDENTITY_PRODUCT_CODE, "request product" },
    { NMTS_SFCT_CHECK_IDENTITY_PRODUCT_CODE, "check product" },
    { NMTS_SFCT_REQUEST_IDENTITY_REVISION, "request revision" },
    { NMTS_SFCT_CHECK_IDENTITY_REVISION, "check revision" },
    { NMTS_SFCT_REQUEST_IDENTITY_SERIAL_NUMBER, "request serial" },
    { NMTS_SFCT_CHECK_IDENTITY_SERIAL_NUMBER, "check serial" },
    { NMTS_SFCT_RESET_COMM, "reset comm" },
    { NMTS_SFCT_SOFTWARE_UPDATE, "SW update" },
    { NMTS_SFCT_CONFIG, "config" },
    { NMTS_SFCT_CHECK_CONFIG1, "check config 1" },
    { NMTS_SFCT_CHECK_CONFIG2, "check config 2" },
    { NMTS_SFCT_UPDATE_CONFIG, "update config" },
    { NMTS_SFCT_START_ERROR_CONTROL, "start err ctrl" },
    { NMTS_SFCT_START_NODE, "start node" },
    { NMTS_SFCT_ERROR_OCCURED, "error occured" },
    { NMTS_SFCT_FINISHED_OK, "finish with ok" },
};
UNSIGNED8	i;

    i = 20;
    while (i > 0)  {
	i--;
	if (cmdStrg[i].fct == cmd)  {
	    return(cmdStrg[i].strg);
	}
    }
    return(NULL);
}
#endif /* NMTSTARTUP_DEBUG */

/*______________________________________________________________________EOF_*/
