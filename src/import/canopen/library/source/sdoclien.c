/*
 *++ sdoclien - subroutines needed for sdo client transfers
 *-- sdoclien - Unterprogramme für SDO Client Transfer
 *
 * Copyright (c) 2001-2017 port GmbH Halle (Saale)
 *------------------------------------------------------------------
 */

/****************************************************************************/
/**
*  \file sdoclien.c
*++ Subroutines needed for sdo client transfers
*-- Unterprogramme für SDO Client Transfer
*  \author port GmbH Halle (Saale)
*
*++ This module contains support functions for sdo client transfers.
*-- Dieses Modul enthält Hilfsfunktionen für den SDO Client Transfer.
*/

/* header of standard C - libraries */

#include <string.h>
#include <stdio.h>

/* header of project specific types */

#include <cal_conf.h>
#include <co_mcpy.h>
#include <co_debug.h>
#include <co_def.h>
#include "sdo.h"
#include "sdomain.h"
#include "cmscodec.h"
#include "nmt.h"
#include "drv.h"
#include "timer.h"
#include "utility.h"

#ifdef CONFIG_SDO_BLOCKTRANSFER
#include "sdoblock.h"
#endif /* CONFIG_SDO_BLOCKTRANSFER */

#ifdef CONFIG_DYN_SDO_CONNECTION_MANAGER
#include "sdomgr.h"
#endif /* CONFIG_DYN_SDO_CONNECTION_MANAGER */

#ifdef CONFIG_REDUNDANCY_SUPPORT
#include "reduncy.h"
#endif /* CONFIG_REDUNDANCY_SUPPORT */

#ifdef CONFIG_NMT_STARTUP_MANAGER
# include "nmtstart.h"
#endif /* CONFIG_NMT_STARTUP_MANAGER */

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
RET_T dnLdSeg_req(SDO_CLIENT_T  *pDomInUse CO_COMMA_LINE_PARA_DECL);
void initUpLd_con(SDO_CLIENT_T	*pCurSdo, UNSIGNED8 *canBuf
	CO_COMMA_LINE_PARA_DECL);
void upLdSeg_con(SDO_CLIENT_T	*pCurSdo, UNSIGNED8 *canBuf
	CO_COMMA_LINE_PARA_DECL);
RET_T upLdSeg_req(SDO_CLIENT_T	*pClientSdo CO_COMMA_LINE_PARA_DECL);

#ifdef CONFIG_SDO_CLIENT
static RET_T pcoSdoUtilCheckIndexSub(SDO_CLIENT_T *pClientSdo,
	UNSIGNED8 *canBuf CO_COMMA_LINE_PARA_DECL  );
#endif /* CONFIG_SDO_CLIENT */

/* external variables
---------------------------------------------------------------------------*/

/* global variables
---------------------------------------------------------------------------*/

/* local defined variables
---------------------------------------------------------------------------*/
#ifdef CONFIG_RCS_IDENT
CO_LIB_INIT_VAR static char _rcsid[] = "$Id: sdoclien.c,v 2.50 2016/11/17 16:31:06 rli Exp $";
#endif /* CONFIG_RCS_IDENT */


#if defined(CONFIG_SDO_CLIENT)

/*****************************************************************
*
* sdoClientMsgCon - sdo confirmation for client
*
* \internal
*
* \retval
*	nothing
*
*/
void sdoClientMsgCon(
	SDO_CLIENT_T	*pClientSdo,	/* pointer to actual client sdo */
	CAN_MSG_T	*canMsg		/* Pointer to CAN Message */
	CO_COMMA_GLOBVARS_PARA_DECL
    )
{
UNSIGNED32	dReason = 0;		/* error reason */
SDO_T		*pSdo = &pClientSdo->sdo;/* pointer to actual sdo */
UNSIGNED8	state;
# ifdef CONFIG_MULT_LINES
UNSIGNED8	canLine;    /* number of CAN line 0..CO_MAX_CAN_LINES-1 */

    canLine = pSdo->pRecCOB->canLine;
# endif /* CONFIG_MULT_LINES */

    /* accept messages only if we have started the transfer */
    if (pSdo->state == SDOSTATE_READY) {
	return;
    }

# ifdef CONFIG_CO_DEBUG
    printSdoState("sdoClientMsgInd", pSdo);
# endif /* CONFIG_CO_DEBUG */


# ifdef CONFIG_SDO_BLOCKTRANSFER
    if (pSdo->state == SDOSTATE_UPLD_BLK_SEG)  {
	/* remove Timer */
	removeTimerEvent(&pClientSdo->timer CO_COMMA_LINE_PARA);
	upLdBlk_ind(pClientSdo, canMsg->pData CO_COMMA_LINE_PARA);
	return;
    }
# endif /* CONFIG_SDO_BLOCKTRANSFER */

    switch (canMsg->pData[0] & CO_SDO_CCS_MASK) {
	case SCS_INI_DN_LD_RES : /* client Initiate Download Confirmation */
				 /* (exp + segm. write req) */
	    if (pSdo->state != SDOSTATE_DNLD_INIT)  {
# ifdef CONFIG_CO_DEBUG
		BDEBUG(CO_DEBUG_SDO, "Bad State for received Message\n");
# endif /* CONFIG_CO_DEBUG */
		return;		/* ignore message */
	    }
	    if ( CO_OK != pcoSdoUtilCheckIndexSub(pClientSdo, canMsg->pData CO_COMMA_LINE_PARA )) {
# ifdef CONFIG_CO_DEBUG
		BDEBUG(CO_DEBUG_SDO, "Index and SubIndex don't fit to Confirmation\n");
# endif /* CONFIG_CO_DEBUG */
		return;
            }

# ifdef CONFIG_CO_DEBUG
	    BDEBUG(CO_DEBUG_SDO, "Init Download Confirmation\n");
# endif /* CONFIG_CO_DEBUG */
            /* remove Timer */
            removeTimerEvent(&pClientSdo->timer CO_COMMA_LINE_PARA);

            /* make sure attr is set */
            pSdo->attr = getObjAttr(pSdo->index, pSdo->subIndex CO_COMMA_LINE_PARA);

	    (void) dnLdSeg_req(pClientSdo CO_COMMA_LINE_PARA);
	    break;

# ifdef CONFIG_SEG_SDO
	case SCS_DN_LD_SEG_RES : /* CLIENT Download Segment Confirmation */

	    if (pSdo->state != SDOSTATE_DNLD_SEG)  {
#  ifdef CONFIG_CO_DEBUG
		BDEBUG(CO_DEBUG_SDO, "Bad State for received Message\n");
#  endif /* CONFIG_CO_DEBUG */
		return;		/* ignore message */
	    }

	    /* if toggle not the same ignore message */
	    if (pSdo->toggleBit != (canMsg->pData[0] & SDO_TOGGLE_BIT)) {
		return;
	    }

#  ifdef CONFIG_CO_DEBUG
	    BDEBUG(CO_DEBUG_SDO, "Download Segment Confirmation\n");
#  endif /* CONFIG_CO_DEBUG */
	    /* change toggle */
	    pSdo->toggleBit ^= SDO_TOGGLE_BIT;

            /* remove Timer */
            removeTimerEvent(&pClientSdo->timer CO_COMMA_LINE_PARA);

            (void) dnLdSeg_req(pClientSdo CO_COMMA_LINE_PARA);
	    break;
# endif /* CONFIG_SEG_SDO */

	case SCS_INI_UP_LD_RES: /* CLIENT Initiate Upload Confirmation */
	    if ((pSdo->state != SDOSTATE_UPLD_INIT)  &&
	        (pSdo->state != SDOSTATE_UPLD_BLK_INIT))  {
# ifdef CONFIG_CO_DEBUG
		BDEBUG(CO_DEBUG_SDO, "Bad State for received Message\n");
# endif /* CONFIG_CO_DEBUG */
		return;		/* ignore message */
	    }
# ifdef CONFIG_CO_DEBUG
	    BDEBUG(CO_DEBUG_SDO, "Init Upload Confirmation\n");
# endif /* CONFIG_CO_DEBUG */
	    if ( CO_OK != pcoSdoUtilCheckIndexSub(pClientSdo, canMsg->pData CO_COMMA_LINE_PARA )) {
# ifdef CONFIG_CO_DEBUG
		BDEBUG(CO_DEBUG_SDO, "Index and SubIndex don't fit to Confirmation\n");
# endif /* CONFIG_CO_DEBUG */
		return;
            }

            /* remove Timer */
            removeTimerEvent(&pClientSdo->timer CO_COMMA_LINE_PARA);

	    initUpLd_con(pClientSdo, &canMsg->pData[0] CO_COMMA_LINE_PARA);
	    break;

# ifdef CONFIG_SEG_SDO

	case SCS_UP_LD_SEG_RES : /* CLIENT Upload Segment Confirmation */
	    if (pSdo->state != SDOSTATE_UPLD_SEG)  {
#  ifdef CONFIG_CO_DEBUG
		BDEBUG(CO_DEBUG_SDO, "Bad State for received Message\n");
#  endif /* CONFIG_CO_DEBUG */
		return;		/* ignore message */
	    }

	    /* if toggle not the same ignore message */
	    if (pSdo->toggleBit != (canMsg->pData[0] & SDO_TOGGLE_BIT)) {
		return;
	    }

#  ifdef CONFIG_CO_DEBUG
	    BDEBUG(CO_DEBUG_SDO, "Upload Segment Confirmation\n");
#  endif /* CONFIG_CO_DEBUG */

	    /* change toggle */
	    pSdo->toggleBit ^= SDO_TOGGLE_BIT;

            /* remove Timer */
            removeTimerEvent(&pClientSdo->timer CO_COMMA_LINE_PARA);

	    upLdSeg_con(pClientSdo, &canMsg->pData[0] CO_COMMA_LINE_PARA);
	    break;
# endif /* not CONFIG_SEG_SDO */

# ifdef CONFIG_SDO_BLOCKTRANSFER
	case CO_SDOBLK_SCS_DOWN: /* CLIENT Block Download Confirmation */
	    if ((pSdo->state != SDOSTATE_DNLD_BLK_INIT) &&
		(pSdo->state != SDOSTATE_DNLD_BLK_SEG) &&
		(pSdo->state != SDOSTATE_DNLD_BLK_END))  {

#  ifdef CONFIG_CO_DEBUG
		BDEBUG(CO_DEBUG_SDO, "Bad State for received Message\n");
#  endif /* CONFIG_CO_DEBUG */

		/* Abort Transfer */
		(void) abortSdoTransf_Req(pSdo, CO_E_SDO_CMD_SPEC_INVALID CO_COMMA_LINE_PARA);
		return;
	    }

            /* remove Timer */
            removeTimerEvent(&pClientSdo->timer CO_COMMA_LINE_PARA);

	    if (pSdo->state != SDOSTATE_DNLD_BLK_END)  {

#  ifdef CONFIG_CO_DEBUG
		BDEBUG(CO_DEBUG_SDO, "Download Block Confirmation\n");
#  endif /* CONFIG_CO_DEBUG */
		dnLdBlk_con(pClientSdo, canMsg->pData CO_COMMA_LINE_PARA);

	    } else  {

#  ifdef CONFIG_CO_DEBUG
		BDEBUG(CO_DEBUG_SDO, "Download Block End Confirmation\n");
#  endif /* CONFIG_CO_DEBUG */

		pSdo->state = SDOSTATE_READY;

#  ifdef CONFIG_DYN_SDO_CONNECTION_MANAGER
		if ((pSdo->num >= FIRST_RESERVED_SDO)
		 && (pSdo->num <= LAST_RESERVED_SDO)) {
		    sdoMgrMsgReceived(pSdo, 0UL CO_COMMA_LINE_PARA);
		} else
#  endif /* CONFIG_DYN_SDO_CONNECTION_MANAGER */
#  ifdef CONFIG_DYN_SDO_CONNECTION_SLAVE
		if (sdoSrdMsgReceived(pSdo, 0UL CO_COMMA_LINE_PARA) == 0)
#  endif /* CONFIG_DYN_SDO_CONNECTION_SLAVE */
		{
#  ifdef CONFIG_CFG_MANAGER
		    if (cfgManagerSdoEvent(pClientSdo, 0 CO_COMMA_LINE_PARA)
				== 0)
#  endif /* CONFIG_CFG_MANAGER */
		    {
			pClientSdo->sdoConf = E_SDO_NO_ERROR;
			sdoWrCon(pSdo->num, E_SDO_NO_ERROR CO_COMMA_LINE_PARA);
		    }
		}
	    }
	    break;

	case  CO_SDOBLK_SCS_UP: /* CLIENT Block Upload Confirmation */

	    if ((pSdo->state != SDOSTATE_UPLD_BLK_INIT) &&
	        (pSdo->state != SDOSTATE_UPLD_BLK_END)) {
#  ifdef CONFIG_CO_DEBUG
		BDEBUG(CO_DEBUG_SDO, "Bad State for received Message\n");
#  endif /* CONFIG_CO_DEBUG */
		/* Abort Transfer */
		(void) abortSdoTransf_Req(pSdo, CO_E_SDO_CMD_SPEC_INVALID CO_COMMA_LINE_PARA);
		return;
	    }

#  ifdef CONFIG_CO_DEBUG
	    BDEBUG(CO_DEBUG_SDO, "Upload Block Confirmation\n");
#  endif /* CONFIG_CO_DEBUG */

            /* remove Timer */
            removeTimerEvent(&pClientSdo->timer CO_COMMA_LINE_PARA);

	    initUpLdBlk_con(pClientSdo, &canMsg->pData[0] CO_COMMA_LINE_PARA);
	    break;
# endif /* CONFIG_SDO_BLOCKTRANSFER */

	case CS_ABORT_TRANSFER:		/* Abort Transfer */
	    if (pSdo->state == SDOSTATE_READY)  {
		/* ignore abort */
		return;
	    }

            /* remove Timer */
            removeTimerEvent(&pClientSdo->timer CO_COMMA_LINE_PARA);

# ifdef CONFIG_SDO_BLOCKTRANSFER
	    /* init block download abort ? */
	    if (pSdo->state == SDOSTATE_DNLD_BLK_INIT)  {
		pSdo->state = SDOSTATE_READY;
		/* blocktransfer failed, try to init a segmented transfer */
		if (initUpDnLd_req(pClientSdo, pSdo->pDomData, pSdo->restSize,
			CCS_INI_DN_LD_REQ CO_COMMA_LINE_PARA
		    ) == CO_OK)  {
		    /* ok, return */
		    return;
		}
	    }
	    /* init block upload abort ? */
	    if (pSdo->state == SDOSTATE_UPLD_BLK_INIT)  {
		pSdo->state = SDOSTATE_READY;
		/* blocktransfer failed, try to init a segmented transfer */
		if (initUpDnLd_req(pClientSdo, pSdo->pDomData, pSdo->restSize,
			CCS_INI_UP_LD_REQ CO_COMMA_LINE_PARA
		    ) == CO_OK)  {
		    /* ok, return */
		    return;
		}
	    }
# endif /* CONFIG_SDO_BLOCKTRANSFER */

	    CO_PACK_MEMCPY((UNSIGNED8 *)&dReason, &canMsg->pData[4], 4, CO_TRUE);

	    /* check if reason != 0, because it mean success */
	    if (dReason == 0L)  {
		dReason = E_SDO_INTERNAL | E_SDO_ZERO_ERROR;
	    }

	    state = pSdo->state;
	    pSdo->state = SDOSTATE_READY;

#  ifdef CONFIG_DYN_SDO_CONNECTION_MANAGER
	    if ((pSdo->num >= FIRST_RESERVED_SDO)
	     && (pSdo->num <= LAST_RESERVED_SDO)) {
		sdoMgrMsgReceived(pSdo, dReason CO_COMMA_LINE_PARA);
	    } else
#  endif /* CONFIG_DYN_SDO_CONNECTION_MANAGER */
#  ifdef CONFIG_DYN_SDO_CONNECTION_SLAVE
	    if (sdoSrdMsgReceived(pSdo, 0UL CO_COMMA_LINE_PARA) == 0)
#  endif /* CONFIG_DYN_SDO_CONNECTION_SLAVE */
	    {
		/* select read or write confirmation */
		if ((state & SDOSTATE_DNLD) != 0) {
#  ifdef CONFIG_CFG_MANAGER
		    if (cfgManagerSdoEvent(pClientSdo, dReason
			CO_COMMA_LINE_PARA) == 0)
#  endif /* CONFIG_CFG_MANAGER */
		    {
			pClientSdo->sdoConf = dReason;
 			sdoWrCon(pSdo->num, dReason CO_COMMA_LINE_PARA);
		    }
		} else {
#  ifdef CONFIG_NMT_STARTUP_MANAGER
		    if (dReason != 0) {
			nmtStartupSdoEvent(NMT_RET_SDO_ABORT, pSdo->num
			    CO_COMMA_REDCY_PARA);
		    }
#  endif /* CONFIG_NMT_STARTUP_MANAGER */
#  ifdef CONFIG_CFG_MANAGER
	   	    if (cfgManagerSdoEvent(pClientSdo, dReason CO_COMMA_LINE_PARA)
			== 0)
#  endif /* CONFIG_CFG_MANAGER */
		    {
			pClientSdo->sdoConf = dReason;
			sdoRdCon(pSdo->num, dReason CO_COMMA_LINE_PARA);
		    }
		}
	    }
	    break;

	default:	/* unknown error */
            /* remove Timer */
            removeTimerEvent(&pClientSdo->timer CO_COMMA_LINE_PARA);

	    (void) abortSdoTransf_Req(pSdo, CO_E_SDO_CMD_SPEC_INVALID
		CO_COMMA_LINE_PARA);
	    break;
    } /* end switch */
}


/*****************************************************************
*
* initUpDnLd_req - shared function for initiate up- and download
*
* \internal
*
* This function initiates a domain transfer
* this is done by comparing the domain handles.
*
* \retval CO_OK
*++ succes
*-- Erfolg
* \retval CO_E_DISABLED
*++ sdo disabled
*-- sdo disabled
* \retval CO_E_BUSY
*++ sdo busy
*-- sdo in Arbeit
* \retval CO_E_STATE
*++ node in wrong state
*-- Knoten nicht in gültigem Status (nicht PRE-Op und nicht OP)
*
*
*/

RET_T initUpDnLd_req(
	SDO_CLIENT_T	*pClientSdo,	/* pointer to client sdo */
	UNSIGNED8	*pDomDat,	/* pointer to domain data */
	UNSIGNED32	dSize,		/* domain size */
	UNSIGNED8	bCmd		/* command specifier */
	CO_COMMA_LINE_PARA_DECL	/* number of CAN line 0..CO_MAX_CAN_LINES-1 */
    )
{
NODE_STATE_T	actState;
UNSIGNED8	tData[8];		/* transmit buffer */
SDO_T		*pSdo;
RET_T		retVal;

    pSdo = &pClientSdo->sdo;

    /* check communication state */
#ifdef CONFIG_REDUNDANCY_SUPPORT
    /* check node state of received line */
    if (pSdo->commLine == CAN_DEFAULT_LINE)  {
	actState = GL_ARRAY(co_Node).eState;
    } else {
	actState = GL_VAR(co_redcyNode).eState;
    }
#else /* CONFIG_REDUNDANCY_SUPPORT */
    actState = GL_ARRAY(co_Node).eState;
#endif /* CONFIG_REDUNDANCY_SUPPORT */

    if ((actState != OPERATIONAL) && (actState != PRE_OPERATIONAL)) {
	 return(CO_E_STATE);
    }

    /* test disable flag */
    if (pSdo->state == SDOSTATE_DISABLED)  {
	return(CO_E_DISABLED);
    }

    /* save size */
    pSdo->domSize = dSize;
    memset(tData, (int)0, (size_t)8);
    CMS_SdoEncode(pSdo->index, pSdo->subIndex, tData);

    if ((bCmd == CCS_INI_UP_LD_REQ)
# ifdef CONFIG_SDO_BLOCKTRANSFER
     || (bCmd == CO_SDOBLK_CCS_UP)
# endif /* CONFIG_SDO_BLOCKTRANSFER */
	)  {

	/* upload request */

# ifdef CONFIG_SDO_BLOCKTRANSFER
	if ((dSize < CONFIG_BLOCK_MIN_DATASIZE) || (bCmd == CCS_INI_UP_LD_REQ)){
# endif /* CONFIG_SDO_BLOCKTRANSFER */
	    /* normal (expetited/segmented) transfer */
	    tData[0] = CCS_INI_UP_LD_REQ;
	    pSdo->state = SDOSTATE_UPLD_INIT;

# ifdef CONFIG_SDO_BLOCKTRANSFER

	}  else  {
	    /* blocktransfer */

#  ifdef CONFIG_CO_DEBUG
	    BDEBUG(CO_DEBUG_SDOBLOCK, "Start SDO Block Upload\n");
#  endif /* CONFIG_CO_DEBUG */

	    /* try to use blocktransfer */
	    tData[0] = CO_SDOBLK_CCS_UP;
#  ifdef CONFIG_BLOCK_CRC
	    tData[0] |= CO_SDOBLOCK_USE_CRC;
	    pSdo->blkCRC = CO_TRUE;
#  else /* CONFIG_BLOCK_CRC */
	    pSdo->blkCRC = CO_FALSE;
#  endif /* CONFIG_BLOCK_CRC */
	    tData[4] = pSdo->blkSegDefaultSize;
	    tData[5] = CONFIG_BLOCK_MIN_DATASIZE;
	    pSdo->state = SDOSTATE_UPLD_BLK_INIT;
	}
# endif /* BLOCK_TRANSFER */
	pSdo->restSize = dSize;
    }
    else  {
	/* download request */
	if (dSize < 5)  {		/* expedited transfer ? */
	    /* expedited transfer */
	    tData[0] = (UNSIGNED8) (
                        CCS_INI_DN_LD_REQ
			+ ((4u - (UNSIGNED8)dSize) << 2u)
# ifndef CO_CONFIG_SDO_EXPEDITED_NO_VALID_SIZE_BIT
			+ CO_SIZE_VALID
 # endif /* CO_CONFIG_SDO_EXPEDITED_NO_VALID_SIZE_BIT */
                        + EXPED_TRANSFER
                        );

# ifdef CO_CONFIG_SDO_EXPEDITED_NO_VALID_SIZE_BIT
            if (pSdo->expedited_sdo_with_valid_size_bit == CO_TRUE)
            {
                tData[0] |= CO_SIZE_VALID;
            }
# endif /* CO_CONFIG_SDO_EXPEDITED_NO_VALID_SIZE_BIT */

	    CO_UNPACK_MEMCPY(&tData[4], pDomDat, (size_t)dSize, pSdo->numeric);
	    pSdo->restSize = 0UL;
	    pSdo->state = SDOSTATE_DNLD_INIT;
	}
	else  {
	    /* segmented transfer */
	    pSdo->restSize = dSize;

	    /* copy size into message buffer */
	    CO_UNPACK_MEMCPY(&tData[4], (UNSIGNED8 *)&dSize, 4, 1);
	    /* block transfer ? */
# ifdef CONFIG_SDO_BLOCKTRANSFER
	    if ((bCmd == CCS_INI_DN_LD_REQ)
	     || (dSize < CONFIG_BLOCK_MIN_DATASIZE))  {
# endif /* CONFIG_SDO_BLOCKTRANSFER */
		/* segmented transfer */
		tData[0] = CCS_INI_DN_LD_REQ + CO_SIZE_VALID;
		pSdo->state = SDOSTATE_DNLD_INIT;
# ifdef CONFIG_SDO_BLOCKTRANSFER
	    } else  {
		/* Block Transfer */
#  ifdef CONFIG_CO_DEBUG
		BDEBUG(CO_DEBUG_SDOBLOCK, "Start SDO Block Download\n");
#  endif /* CONFIG_CO_DEBUG */

		tData[0] = CO_SDOBLK_CCS_DOWN + CO_SDOBLOCK_SIZE_VALID;
		pSdo->blkSegSize = 0;
#  ifdef CONFIG_BLOCK_CRC
		tData[0] |= CO_SDOBLOCK_USE_CRC;
		pSdo->blkCRC = CO_TRUE;
#  else /* CONFIG_BLOCK_CRC */
		pSdo->blkCRC = CO_FALSE;
#  endif /* CONFIG_BLOCK_CRC */
		pSdo->state = SDOSTATE_DNLD_BLK_INIT;
	    }
# endif /* CONFIG_SDO_BLOCKTRANSFER */
	}
    } /* end download */

    /* set state flags */
    pSdo->toggleBit = 0;
    pSdo->pDomData = pDomDat;
    pSdo->pActualDomData = pDomDat;
# ifdef CONFIG_DOMAIN_CONFIRMATION
#  ifdef CONFIG_SDO_BLOCKTRANSFER
    pClientSdo->pBufferStart = pDomDat;
#  endif /* CONFIG_SDO_BLOCKTRANSFER */
# endif /* CONFIG_DOMAIN_CONFIRMATION */

# ifdef CONFIG_BLOCK_CRC
    pSdo->blkCrcSum = 0;
# endif

    /* transmit */
# ifdef CONFIG_REDUNDANCY_SUPPORT
    GL_VAR(co_redcySdoLine) = pSdo->commLine;
# endif /* CONFIG_REDUNDANCY_SUPPORT */

    retVal = TRANSMIT_COB(pSdo->pTrCOB, tData);
    /* error at driver occured ? */
    if (retVal != CO_OK) {
	/* set sdo back to unused */
	pSdo->state = SDOSTATE_READY;
    } else {

#  ifdef CONFIG_16BIT_CPU
	pSdo->halfWord = CO_FALSE;
#  endif /* CONFIG_16BIT_CPU */

	/* start timeout */
	if (addTimerEvent(&pClientSdo->timer, pClientSdo->timeOut,
		    CO_TIMER_TYPE_SDO CO_COMMA_LINE_PARA)
	    != 0)
        {
            (void) abortSdoTransf_Req(pSdo, CO_E_SDO_TIMEOUT CO_COMMA_LINE_PARA);
	    return(CO_E_SDO_TIMEOUT);
	}
    }

    return(retVal);
}


/*******************************************************************
*
* dnLdSeg_req - requests the service domain download segment
*
* \internal
*
* Der Client lädt ein Segment der Domain in den Server.
* (exp. and domain write transfers)
*
* CiA301: download segment request
*
* \retval CO_OK
*-- success
*++ Erfolg
*
*/

RET_T dnLdSeg_req(
        SDO_CLIENT_T *pClientSdo   /* pointer to SDO structure */
        CO_COMMA_LINE_PARA_DECL    /* number of CAN line 0..CO_MAX_CAN_LINES-1 */
    )
{
# ifdef CONFIG_SEG_SDO
UNSIGNED8 tData[8];    /* transmit buffer */
UNSIGNED8 contFlag;    /* continious flag */
UNSIGNED8 bSize;       /* size */
# endif /* CONFIG_SEG_SDO */
SDO_T *pSdo = &pClientSdo->sdo;/* pointer to actual sdo */
RET_T retVal = CO_OK;
# ifdef CO_CONFIG_DOMAIN_UNKNOWN_SIZE
BOOL_T needEndMessage = CO_FALSE;
# endif /* CO_CONFIG_DOMAIN_UNKNOWN_SIZE */

# ifdef CO_CONFIG_DOMAIN_UNKNOWN_SIZE
    /* sizeless: ask application for restsize */
    /* don't ask if we already know that we're at the end */
    /* happens when last segment was between 0-7 bytes */
    if (pSdo->restSize != 0u)
    {
        if ((pSdo->attr & CO_UP_DN_LD_DOMAIN_SIZELESS) != 0)
        {
            if (pSdo->restSize <= 7)
            {
                /* printf("dnLdSeg_req: ask for size\n"); */
                pSdo->restSize = coUserSdoDomainSizeInd(pSdo->num CO_COMMA_LINE_PARA);
            }
            else
            {
                /* printf("dnLdSeg_req: size bigger than 1 message: %lu\n",pSdo->restSize); */
            }
        }
        if (pSdo->restSize == 0u)
        /* set by application */
        /* need extra empty message to cause server to write to OD */
        {
            needEndMessage = CO_TRUE;
        }
    }
# endif /* CO_CONFIG_DOMAIN_UNKNOWN_SIZE */

    /* no more data */
# ifdef CO_CONFIG_DOMAIN_UNKNOWN_SIZE
    if ((pSdo->restSize == 0UL) && (needEndMessage == CO_FALSE))
# else  /* CO_CONFIG_DOMAIN_UNKNOWN_SIZE */
    if (pSdo->restSize == 0UL)
# endif /* CO_CONFIG_DOMAIN_UNKNOWN_SIZE */
    {
        pSdo->state = SDOSTATE_READY;

# ifdef CONFIG_DYN_SDO_CONNECTION_MANAGER
        if ((pSdo->num >= FIRST_RESERVED_SDO)
            && (pSdo->num <= LAST_RESERVED_SDO)) {
            sdoMgrMsgReceived(pSdo, 0 CO_COMMA_LINE_PARA);
        } else
# endif /* CONFIG_DYN_SDO_CONNECTION_MANAGER */
# ifdef CONFIG_DYN_SDO_CONNECTION_SLAVE
        if (sdoSrdMsgReceived(pSdo, 0UL CO_COMMA_LINE_PARA) == 0)
# endif /* CONFIG_DYN_SDO_CONNECTION_SLAVE */
        {
# ifdef CONFIG_CFG_MANAGER
	    if (cfgManagerSdoEvent(pClientSdo, 0 CO_COMMA_LINE_PARA) == 0)
# endif /* CONFIG_CFG_MANAGER */
            {
                pClientSdo->sdoConf = E_SDO_NO_ERROR;
                sdoWrCon(pSdo->num, E_SDO_NO_ERROR CO_COMMA_LINE_PARA);
            }
        }
        return(CO_OK);
    }

# ifdef CONFIG_SEG_SDO
    /* save address */
    if (pSdo->restSize > 7UL) {
        bSize = 7;
        contFlag = CO_SDO_MORE;

#ifdef CONFIG_DEBUG_SDO_CLIENT_BREAK_CNT
        {
        /* send only CONFIG_DEBUG_SDO_CLIENT_BREAK_CNT bytes
            and don't fill all bytes at last message */
UNSIGNED32 actCnt;
UNSIGNED8 rest;

            actCnt = pSdo->domSize - pSdo->restSize;
            rest = ((actCnt + 7) % CONFIG_DEBUG_SDO_CLIENT_BREAK_CNT);
            if (rest < 7) {
                bSize -= rest;
                /* printf("bisher %d (%d * %d), bSize = %d, danach sind %d\n", */
                /* actCnt, actCnt / CONFIG_DEBUG_SDO_CLIENT_BREAK_CNT, */
                    /* CONFIG_DEBUG_SDO_CLIENT_BREAK_CNT, bSize,actCnt + bSize); */
            }
        }
#endif

    } else  {
        /* restSize less than or equal 7 bytes */
        contFlag = CO_SDO_LAST;
        bSize = (UNSIGNED8)pSdo->restSize;
        /* fill with 0 */
        memset(&tData[1], (int)0, (size_t)7);
        /* pCurSdo->restSize = 0UL; */
    }

#  ifdef CONFIG_16BIT_CPU
    /* if domain download */
    if (pSdo->numeric == CO_TRUE)  {
        pSdo->pActualDomData =
            unpack_oddmemcpy(tData + 1, pSdo->pActualDomData,
                             bSize, &pSdo->halfWord);
    } else
#  endif /* CONFIG_16BIT_CPU */
    {
        CO_MEMCPY(tData + 1, pSdo->pActualDomData, bSize);
        pSdo->pActualDomData += ((UNSIGNED32)bSize);
    }
    pSdo->restSize -= (UNSIGNED32)bSize;

    tData[0] = (UNSIGNED8)(CCS_DN_LD_SEG_REQ | pSdo->toggleBit
                    | ((7u - bSize) << 1u) | contFlag);

# ifdef CONFIG_REDUNDANCY_SUPPORT
    GL_VAR(co_redcySdoLine) = pSdo->commLine;
# endif /* CONFIG_REDUNDANCY_SUPPORT */
    retVal = TRANSMIT_COB(pSdo->pTrCOB, tData);

    pSdo->state = SDOSTATE_DNLD_SEG;

#  ifdef CONFIG_DOMAIN_CONFIRMATION
    if (pClientSdo->domainIndSize != 0)  {
        /* next border reached ? */
        if (pSdo->restSize == pClientSdo->nextDomainIndBorder)  {
            /* call indication function */
            if (sdoDomainWrCon(pSdo->num CO_COMMA_LINE_PARA) != CO_OK)  {
                /* abort transfer */
                abortSdoTransf_Req(pSdo, CO_E_HARDWARE_FAULT CO_COMMA_LINE_PARA);
                /* FIXME missing return, change abort reason */
            }
            /* reset pointer */
            pSdo->pActualDomData = pSdo->pDomData;
            /* calculate next border */
            if (pClientSdo->domainIndSize < pSdo->restSize)  {
                pClientSdo->nextDomainIndBorder =
                    pSdo->restSize - pClientSdo->domainIndSize;
            }
        }
    }
#  endif /* CONFIG_DOMAIN_CONFIRMATION */
# endif /* not CONFIG_SEG_SDO */

    /* start timeout again */
    if (addTimerEvent(&pClientSdo->timer, pClientSdo->timeOut,
                        CO_TIMER_TYPE_SDO CO_COMMA_LINE_PARA) != 0)
    {
        return(CO_E_SDO_TIMEOUT);
    }

    return(retVal);
}


/*******************************************************************
*
*++ upLdSeg_req - request the service domain upload segment
*-- upLdSeg_req - fordert die Übertragung eines Domain Segmentes an
*
* \internal
*
*-- Der Client fordert mit dieser Funktion
*-- die Übertragung eines Segmentes
*-- der Domain mit der Hodenummer \f2hNum\f1 an.
*++ With this function the client requests the transmission
*++ of a upload segment.
*
* \retval CO_OK
*++ success
*-- Erfolg
*
*/
RET_T upLdSeg_req(
	SDO_CLIENT_T	*pClientSdo	/* pointer to client sdo */
	CO_COMMA_LINE_PARA_DECL	/* number of CAN line 0..CO_MAX_CAN_LINES-1 */
    )
{
UNSIGNED8	pData[8];
RET_T		retVal;

    memset(&pData[1], (int)0, (size_t)7);
    pData[0] = CCS_UP_LD_SEG_REQ | pClientSdo->sdo.toggleBit;

# ifdef CONFIG_REDUNDANCY_SUPPORT
    GL_VAR(co_redcySdoLine) = pClientSdo->sdo.commLine;
# endif /* CONFIG_REDUNDANCY_SUPPORT */
    retVal = TRANSMIT_COB(pClientSdo->sdo.pTrCOB, pData);

    /* start timeout again */
    if (addTimerEvent(&pClientSdo->timer, pClientSdo->timeOut,
		CO_TIMER_TYPE_SDO CO_COMMA_LINE_PARA) != 0)  {
	return(CO_E_SDO_TIMEOUT);
    }

    return(retVal);
}


/*******************************************************************
*
*++ initUpLd_con - Upload SDO Response
*-- initUpLd_con - SDO-Antowrt auf Upload
*
* \internal
*
* This function responses a upload domain request of a CANopen server.
*
* RETURNS
* .TP
* nothing
*
*/

void initUpLd_con(
	SDO_CLIENT_T	*pClientSdo,	/* Pointer to SDO */
	UNSIGNED8	*canBuf		/* Pointer to CAN buffer */
	CO_COMMA_LINE_PARA_DECL	/* number of CAN line 0..CO_MAX_CAN_LINES-1 */
   )
{
UNSIGNED32	dSize;			/* data size */
UNSIGNED8	*pData = NULL;		/* pointer to data */
SDO_T		*pSdo = &pClientSdo->sdo;/* pointer to actual sdo */

    dSize = 0UL;

    /* select the subcommand */
    switch (canBuf[0] & CO_SDO_SIZE_TYPE_MASK) {
	/*
	 * normal transfer
	 * d contains number of data bytes
	*/
	case 1:
	    CO_PACK_MEMCPY((UNSIGNED8 *)&dSize, &canBuf[4], 4, CO_TRUE);
	    pData = NULL;
	    break;
	case 2:
	    /* if no size indicator size always 4 */
	    /* dSize = 4; */
	    /* use the given buffer size */
	    dSize = pSdo->domSize;
	    pData = canBuf+4;
	    break;
	/*
	 * expedited transfer
	 * d contains data
	 * n contains data number
	 */
	case 3:
	    dSize = 4 - ((canBuf[0] & 0x0C) >> 2);
	    pData = canBuf+4;
	    break;
	default:
	    break;
    }

    /* test for space for data */
    if ((pSdo->domSize < dSize) || (pSdo->pDomData == NULL)) {
	(void) abortSdoTransf_Req(pSdo, CO_E_MEM CO_COMMA_LINE_PARA);

# ifdef CONFIG_DYN_SDO_CONNECTION_MANAGER
	if ((pSdo->num >= FIRST_RESERVED_SDO)
	 && (pSdo->num <= LAST_RESERVED_SDO)) {
	    sdoMgrMsgReceived(pSdo, 1 CO_COMMA_LINE_PARA);
	} else
# endif /* CONFIG_DYN_SDO_CONNECTION_MANAGER */
# ifdef CONFIG_DYN_SDO_CONNECTION_SLAVE
	if (sdoSrdMsgReceived(pSdo, 0UL CO_COMMA_LINE_PARA) == 0)
# endif /* CONFIG_DYN_SDO_CONNECTION_SLAVE */
	{
	    /* error: not enough memory */
# ifdef CONFIG_NMT_STARTUP_MANAGER
	    nmtStartupSdoEvent(NMT_RET_SDO_ABORT, pSdo->num
	        CO_COMMA_REDCY_PARA);
# endif /* CONFIG_NMT_STARTUP_MANAGER */
# ifdef CONFIG_CFG_MANAGER
	    if (cfgManagerSdoEvent(pClientSdo,
		E_SDO_INTERNAL | E_SDO_NO_RESSOURCES
		CO_COMMA_LINE_PARA) == 0)
# endif /* CONFIG_CFG_MANAGER */
	    {
		pClientSdo->sdoConf = E_SDO_INTERNAL | E_SDO_NO_RESSOURCES;
		sdoRdCon(pSdo->num, E_SDO_INTERNAL | E_SDO_NO_RESSOURCES
		    CO_COMMA_LINE_PARA);
	    }
	}
	return;
    }

    /* set string termination, if
       expected size (upLdDom_req) > real size
       typical for string up load */
# ifdef CONFIG_DOMAIN_CONFIRMATION
    if (pClientSdo->domainIndSize == 0)
# endif /* CONFIG_DOMAIN_CONFIRMATION */
    {
	if (pSdo->domSize > dSize)  {
	    pSdo->pDomData[(UNSIGNED16)dSize] = 0x0;
	}
    }

    /* set expected size to real size */
    pSdo->domSize = dSize;
    pSdo->restSize = dSize;
    if ((dSize < 5) && (pData != NULL)) {
	/* copy to user's destination for automatic transfer */
	CO_PACK_MEMCPY(pSdo->pDomData, pData, (size_t)dSize,
		pSdo->numeric);

	pSdo->state = SDOSTATE_READY;

# ifdef CONFIG_DYN_SDO_CONNECTION_MANAGER
	if ((pSdo->num >= FIRST_RESERVED_SDO)
	 && (pSdo->num <= LAST_RESERVED_SDO)) {
	    sdoMgrMsgReceived(pSdo, 0 CO_COMMA_LINE_PARA);
	} else
# endif /* CONFIG_DYN_SDO_CONNECTION_MANAGER */
# ifdef CONFIG_DYN_SDO_CONNECTION_SLAVE
	if (sdoSrdMsgReceived(pSdo, 0UL CO_COMMA_LINE_PARA) == 0)
# endif /* CONFIG_DYN_SDO_CONNECTION_SLAVE */
	{
# ifdef CONFIG_NMT_STARTUP_MANAGER
	    nmtStartupSdoEvent(NMT_RET_SDO_OK, pSdo->num CO_COMMA_REDCY_PARA);
# endif /* CONFIG_NMT_STARTUP_MANAGER */
# ifdef CONFIG_CFG_MANAGER
	    if (cfgManagerSdoEvent(pClientSdo, 0L CO_COMMA_LINE_PARA) == 0)
# endif /* CONFIG_CFG_MANAGER */
	    {
		pClientSdo->sdoConf = E_SDO_NO_ERROR;
		sdoRdCon(pSdo->num, E_SDO_NO_ERROR CO_COMMA_LINE_PARA);
	    }
	}
	return;
    }

# ifdef CONFIG_SEG_SDO
    /* common initiate -> continue with segmented transfer */
    pSdo->state = SDOSTATE_UPLD_SEG;
    (void) upLdSeg_req(pClientSdo CO_COMMA_LINE_PARA);

#  ifdef CONFIG_DOMAIN_CONFIRMATION
    /* calculate next border */
    pClientSdo->nextDomainIndBorder = pSdo->restSize - pClientSdo->domainIndSize;
#  endif /* CONFIG_DOMAIN_CONFIRMATION */
# else /* CONFIG_SEG_SDO */
    abortSdoTransf_Req(pSdo, CO_E_INVALID_TRANSMODE CO_COMMA_LINE_PARA);

#  ifdef CONFIG_DYN_SDO_CONNECTION_MANAGER
    if ((pSdo->num >= FIRST_RESERVED_SDO)
     && (pSdo->num <= LAST_RESERVED_SDO)) {
	sdoMgrMsgReceived(pSdo, 1 CO_COMMA_LINE_PARA);
    } else
#  endif /* CONFIG_DYN_SDO_CONNECTION_MANAGER */
# ifdef CONFIG_DYN_SDO_CONNECTION_SLAVE
    if (sdoSrdMsgReceived(pSdo, 0UL CO_COMMA_LINE_PARA) == 0)
# endif /* CONFIG_DYN_SDO_CONNECTION_SLAVE */
    {
	/* error: service not available */
# ifdef CONFIG_NMT_STARTUP_MANAGER
	nmtStartupSdoEvent(E_NMT_SDO_ABORT, pSdo->num CO_COMMA_LINE_PARA);
# endif /* CONFIG_NMT_STARTUP_MANAGER */
# ifdef CONFIG_CFG_MANAGER
	if (cfgManagerSdoEvent(pSdo, E_SDO_INTERNAL | E_SDO_SERVICE
		CO_COMMA_LINE_PARA) == 0)
# endif /* CONFIG_CFG_MANAGER */
	{
	    pClientSdo->sdoConf = E_SDO_INTERNAL | E_SDO_SERVICE;
	    sdoRdCon(pSdo->num, E_SDO_INTERNAL | E_SDO_SERVICE CO_COMMA_LINE_PARA);
	}
    }
    return;
# endif /* CONFIG_SEG_SDO */

}


/*******************************************************************
*
* upLdSeg_con - Upload SDO Response for CANopen
*
* \internal
*
* This function handles the upload domain response of a CANopen server.
*
* RETURNS
* .TP
* nothing
*
*/

void upLdSeg_con(
	SDO_CLIENT_T	*pClientSdo,	/* Pointer to SDO */
	UNSIGNED8	*canBuf		/* Pointer to CAN buffer */
	CO_COMMA_LINE_PARA_DECL	/* number of CAN line 0..CO_MAX_CAN_LINES-1 */
   )
{
UNSIGNED8	dSize;			/* actual data size */
SDO_T		*pSdo = &pClientSdo->sdo;/* pointer to actual sdo */

    /* get size in Byte */
    dSize = 7 - ((canBuf[0] >> 1) & 7);

    /* check for enough data at buffer */
    if (dSize > pSdo->restSize)  {
	/* abort transfer */
	(void) abortSdoTransf_Req(pSdo, CO_E_MEM CO_COMMA_LINE_PARA);
	return;
    }

    /* copy data */
# ifdef CONFIG_16BIT_CPU
    /* if numeric data */
    if (pSdo->numeric == CO_TRUE)  {
	pSdo->pActualDomData =
	    pack_oddmemcpy(pSdo->pActualDomData, canBuf + 1,
		dSize, &pSdo->halfWord);
    } else
# endif /* CONFIG_16BIT_CPU */
    {
	CO_PACK_MEMCPY(pSdo->pActualDomData, &canBuf[1], dSize, pSdo->numeric);

	/* increment actual pointer */
	pSdo->pActualDomData += dSize;
    }

#  ifdef CONFIG_DOMAIN_CONFIRMATION
   /* FIXME outside ifdef? */
    pSdo->restSize -= dSize;

    if (pClientSdo->domainIndSize != 0)  {
	/* next border reached ? */
	if (pSdo->restSize == pClientSdo->nextDomainIndBorder)  {
	    /* call indication function */
	    if (sdoDomainRdCon(pSdo->num CO_COMMA_LINE_PARA) != CO_OK)  {
		/* abort transfer */
		abortSdoTransf_Req(pSdo, CO_E_HARDWARE_FAULT CO_COMMA_LINE_PARA);
		return;
	    }
	    /* reset pointer */
	    pSdo->pActualDomData = pSdo->pDomData;
	    /* calculate next border */
	    /* pClientSdo->nextDomainIndBorder += pClientSdo->domainIndSize; */
	    if (pClientSdo->domainIndSize < pSdo->restSize)  {
		pClientSdo->nextDomainIndBorder =
			pSdo->restSize - pClientSdo->domainIndSize;
	    }
	}
    }
#  endif /* CONFIG_DOMAIN_CONFIRMATION */

    /* continue if more data */
    if ((*canBuf & CO_SDO_LAST) != CO_SDO_LAST) {
	(void) upLdSeg_req(pClientSdo CO_COMMA_LINE_PARA);
    } else {

	/* delete timer */
# ifdef CONFIG_CFG_MANAGER
	if (cfgManagerSdoEvent(pClientSdo, 0L CO_COMMA_LINE_PARA) == 0)
# endif /* CONFIG_CFG_MANAGER */
	{
	    pClientSdo->sdoConf = E_SDO_NO_ERROR;
	    pSdo->state = SDOSTATE_READY;
	    sdoRdCon(pSdo->num, E_SDO_NO_ERROR CO_COMMA_LINE_PARA);
	}
# ifdef CONFIG_CFG_MANAGER
        else {
	    pSdo->state = SDOSTATE_READY;
        }
# endif /* CONFIG_CFG_MANAGER */

#ifdef CONFIG_DYN_SDO_CONNECTION_MANAGER
	if ((pSdo->num >= FIRST_RESERVED_SDO)
	 && (pSdo->num <= LAST_RESERVED_SDO)) {
	    sdoMgrMsgReceived(pSdo, 0 CO_COMMA_LINE_PARA);
	} else
# endif /* CONFIG_DYN_SDO_CONNECTION_MANAGER */
# ifdef CONFIG_DYN_SDO_CONNECTION_SLAVE
	if (sdoSrdMsgReceived(pSdo, 0UL CO_COMMA_LINE_PARA) == 0)
	{
#  ifdef CONFIG_CFG_MANAGER
	    if (cfgManagerSdoEvent(pSdo, 0L CO_COMMA_LINE_PARA) == 0)
#  endif /* CONFIG_CFG_MANAGER */
	    {
		pClientSdo->sdoConf = E_SDO_NO_ERROR;
		sdoRdCon(pSdo->num, E_SDO_NO_ERROR CO_COMMA_LINE_PARA);
	    }
	}
# endif /* CONFIG_DYN_SDO_CONNECTION_SLAVE */
    }
}


/****************************************************************************/
/*
*++ \brief sdoTimeOut - timeout function for sdo access
*-- \brief sdoTimeOut for SDO Zugriffe
*
* \internal
*
*++ This function is called when the timeout for
*-- Diese Funktion wird nach dem Ablauf des Timeout nach einem
* \em writeSdoReq(2)
*++ or
*-- bzw.
* \em readSdoReq(2)
*++ is over.
*-- aufgerufen.
*++ First an
*-- Es wird ein
* \b "Abort domain Transfer"
*++ is sent.
*-- gesendet und die Indikation Funktion
*++ Then the indication function
* \em sdoWrCon()
*++ or
*-- bzw.
* \em sdoRdCon()
*++ with the parameter
*-- mit dem Parameter
* \b CO_E_TIMEOUT
*++ is called.
*-- aufgerufen.
*
* \retval
*	none
*/

void sdoTimeOut(
	TIMER_EVENT_T	*pTimer	/**< pointet to timer structure */
	CO_COMMA_LINE_PARA_DECL	/**< number of CAN line 0..CO_MAX_CAN_LINES-1 */
    )
{
SDO_CLIENT_T	*pCurSdo;		/* pointer to current SDO */

    pCurSdo = (SDO_CLIENT_T *)pTimer;

    /* send abort telegram */
    (void) abortSdoTransf_Req(&pCurSdo->sdo, CO_E_SDO_TIMEOUT
		CO_COMMA_LINE_PARA);

    /* call indication function */
    if (pCurSdo->upDnType == SDO_DOWNLOAD)  {
# ifdef CONFIG_CFG_MANAGER
	if (cfgManagerSdoEvent(pCurSdo, E_SDO_TIMEOUT CO_COMMA_LINE_PARA) == 0)
# endif /* CONFIG_CFG_MANAGER */
	{
	    pCurSdo->sdoConf = E_SDO_TIMEOUT;
	    sdoWrCon(pCurSdo->sdo.num, E_SDO_TIMEOUT CO_COMMA_LINE_PARA);
	}
    } else {
# ifdef CONFIG_NMT_STARTUP_MANAGER
	nmtStartupSdoEvent(NMT_RET_SDO_TIMEOUT, pCurSdo->sdo.num
	    CO_COMMA_REDCY_PARA);
# endif /* CONFIG_NMT_STARTUP_MANAGER */
# ifdef CONFIG_CFG_MANAGER
	if (cfgManagerSdoEvent(pCurSdo, E_SDO_TIMEOUT CO_COMMA_LINE_PARA) == 0)
# endif /* CONFIG_CFG_MANAGER */
	{
	    pCurSdo->sdoConf = E_SDO_TIMEOUT;
	    sdoRdCon(pCurSdo->sdo.num, E_SDO_TIMEOUT CO_COMMA_LINE_PARA);
	}
    }

    return;
}

/* Helper function to compare message index and sub with sdo structure */
static RET_T pcoSdoUtilCheckIndexSub(
	SDO_CLIENT_T *pClientSdo,
	UNSIGNED8 *canBuf
	CO_COMMA_LINE_PARA_DECL
    )
{
RET_T retVal = CO_E_PARA_INCOMP;
UNSIGNED16 index    = 0u; /* temporary buffer for index from sdo msg */
UNSIGNED8  subindex = 0u; /* temporary buffer for sub from sdo msg */

# ifdef CONFIG_MULT_LINES
    CO_INTERNAL_NOT_USED(CO_LINE_PARA);
# endif /* CONFIG_MULT_LINES */

    /* get the index and the subindex from the message */
    CMS_SdoDecode( index, subindex, canBuf );

    /* Check if this message belogs to this SDO struct */
    if ( (index == pClientSdo->sdo.index)
	 && (subindex == pClientSdo->sdo.subIndex) ) {

        retVal = CO_OK;
    }
    /* for compatibilty reason to older version of the stack */
    if ( (index == 0u) && (subindex == 0u) ) {
        retVal = CO_OK;
    }

    return retVal;
}





#endif /* defined(CONFIG_SDO_CLIENT) */

/*______________________________________________________________________EOF_*/

