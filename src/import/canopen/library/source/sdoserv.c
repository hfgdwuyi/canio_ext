/*
 *++ sdoserv - subroutines needed for sdo server transfers
 *-- sdoserv - Unterprogramme für SDO Server Transfer
 *
 * Copyright (c) 2001-2017 port GmbH Halle (Saale)
 *------------------------------------------------------------------
 */

/**
*  \file sdoserv.c
*  \author port GmbH Halle (Saale)
*
*++ This module contains support functions for sdo server transfers.
*-- Dieses Modul enthält Hilfsfunktionen für den SDO Server Transfer
*
*/

/* header of standard C - libraries */

#include <string.h>
#include <stdio.h>

/* header of project specific types */

#include <cal_conf.h>
#include <co_debug.h>
#include <co_mcpy.h>
#include <co_setcp.h>
#include <co_odidx.h>
#include "sdo.h"
#include "cmscodec.h"
#include "access.h"
#include "nmt.h"
#include "nmt_s.h"
#include "drv.h"

#ifdef CONFIG_NON_VOLATILE_MEM
# include <co_stor.h>
#endif /* CONFIG_NON_VOLATILE_MEM */

#ifdef CONFIG_DYN_SDO_CONNECTION_MANAGER
# include "sdomgr.h"
#endif /* CONFIG_DYN_SDO_CONNECTION_MANAGER */

#ifdef CONFIG_16BIT_CPU
# include "utility.h"
#endif /* CONFIG_16BIT_CPU */

# ifdef CONFIG_SDO_BLOCKTRANSFER
# include "sdoblock.h"
# endif /* CONFIG_SDO_BLOCKTRANSFER */

#ifdef CONFIG_REDUNDANCY_SUPPORT
# include "reduncy.h"
#endif /* CONFIG_REDUNDANCY_SUPPORT */

#ifdef CO_CONFIG_ENABLE_OBJ_CALLBACK
# include <co_acces.h>
#endif /* CO_CONFIG_ENABLE_OBJ_CALLBACK */

#ifdef CO_CONFIG_SDO_SHORT_STRINGS
# include <co_acces.h>
#endif /* CO_CONFIG_SDO_SHORT_STRINGS */

/* constant definitions
---------------------------------------------------------------------------*/

/* local defined data types
---------------------------------------------------------------------------*/

/* list of external used functions, if not in headers
---------------------------------------------------------------------------*/
#ifdef CO_CONFIG_DOMAIN_UNKNOWN_SIZE
    extern UNSIGNED32 coUserSdoDomainSizeInd(UNSIGNED8 CO_COMMA_LINE_PARA_DECL);
#endif /* CO_CONFIG_DOMAIN_UNKNOWN_SIZE */

/* list of global defined functions
---------------------------------------------------------------------------*/

/* list of local defined functions
---------------------------------------------------------------------------*/
    void initDnLd_ind(SDO_T	*pCurSdo, UNSIGNED8 *canBuf CO_COMMA_LINE_PARA_DECL);
    void initDnLd_res(SDO_T	*pCurSdo CO_COMMA_GLOBVARS_PARA_DECL);
    void dnLdSeg_ind(SDO_T	*pCurSdo, UNSIGNED8 *pCanBuf CO_COMMA_LINE_PARA_DECL);
    void dnLdSeg_res(SDO_T	*pCurSdo CO_COMMA_GLOBVARS_PARA_DECL);
    static void writeSdoValue(SDO_T *pCurSdo, UNSIGNED8 *pData CO_COMMA_LINE_PARA_DECL);
#ifdef CONFIG_SPLIT_INDICATION
    void writeSdoValue_finished(SDO_T *pCurSdo CO_COMMA_LINE_PARA_DECL);
    RET_T initUpLd_ind_finish(SDO_T	*pCurSdo CO_COMMA_LINE_PARA_DECL);
#endif /* CONFIG_SPLIT_INDICATION */
#ifdef CONFIG_DOMAIN_INDICATION_SIZE
# ifdef CO_CONFIG_DOMAIN_INDICATION_DEFERRABLE
   void finish_dnLdSeg_ind(SDO_T *pCurSdo CO_COMMA_LINE_PARA_DECL);
# endif /* CO_CONFIG_DOMAIN_INDICATION_DEFERRABLE */
#endif /* CONFIG_DOMAIN_INDICATION_SIZE */

/* external variables
---------------------------------------------------------------------------*/

/* global variables
---------------------------------------------------------------------------*/

/* local defined variables
---------------------------------------------------------------------------*/
#ifdef CONFIG_RCS_IDENT
CO_LIB_INIT_VAR static char _rcsid[] = "$Id: sdoserv.c,v 2.65 2016/09/26 11:16:09 rli Exp $";
#endif /* CONFIG_RCS_IDENT */


#ifdef CONFIG_SDO_SERVER

/*****************************************************************
*
* sdoServerMsgInd - sdo indication for server
*
* NOMANUAL
*
* RETURNS
* .TP
* nothing
*
*/
void sdoServerMsgInd(
	SDO_T		*pSdo,		/* Pointer to actual SDO structure */
	CAN_MSG_T	*canMsg		/* Pointer to CAN Message */
	CO_COMMA_LINE_PARA_DECL	/* number of CAN line 0..CO_MAX_CAN_LINES-1 */
    )
{
# ifdef CONFIG_SDO_BLOCKTRANSFER
    if (pSdo->state == SDOSTATE_DNLD_BLK_SEG)  {
	/* but test before for sdo abort */
	if (canMsg->pData[0] == 0x80)  {
	    pSdo->state = SDOSTATE_READY;

# ifdef CONFIG_SDO_SERVER_ABORT_IND
	    {
	    UNSIGNED32	abortCode;

		CO_PACK_MEMCPY((UNSIGNED8 *)&abortCode, &canMsg->pData[4], 4,
		    CO_NUM_VAL);
		sdoServerAbortInd(pSdo->index, pSdo->subIndex, abortCode
		    CO_COMMA_LINE_PARA);
	    }
# endif /* CONFIG_SDO_SERVER_ABORT_IND */

	    return;
	}

	dnLdBlk_ind(pSdo, canMsg->pData CO_COMMA_LINE_PARA);
	return;
    }
# endif /* CONFIG_SDO_BLOCKTRANSFER */

    switch (canMsg->pData[0] & CO_SDO_SCS_MASK) {
	case CCS_INI_DN_LD_REQ : /* SERVER Initiate Download Indication */
	    if (pSdo->state != SDOSTATE_READY)  {
# ifdef CONFIG_CO_DEBUG
		BDEBUG(CO_DEBUG_SDO, "Bad State for received Message\n");
# endif /* CONFIG_CO_DEBUG */
		return;
	    }
	    pSdo->toggleBit = SDO_TOGGLE_BIT;

# ifdef CONFIG_CO_DEBUG
	    BDEBUG(CO_DEBUG_SDO, "init Download Indication\n");
# endif /* CONFIG_CO_DEBUG */
	    initDnLd_ind(pSdo, canMsg->pData CO_COMMA_LINE_PARA);
	    break;

# ifdef CONFIG_SEG_SDO
	case CCS_DN_LD_SEG_REQ : /* SERVER Download Segment Indication */
	    if (pSdo->state != SDOSTATE_DNLD_SEG)  {
#  ifdef CONFIG_CO_DEBUG
		BDEBUG(CO_DEBUG_SDO, "Bad State for received Message\n");
#  endif /* CONFIG_CO_DEBUG */
		return;
	    }
	    /* if toggle-bit not alternates, ignore message */
	    if (pSdo->toggleBit == (canMsg->pData[0] & SDO_TOGGLE_BIT)) {
		return;
	    }

#  ifdef CONFIG_CO_DEBUG
	    BDEBUG(CO_DEBUG_SDO, "Download Segment Indication\n");
#  endif /* CONFIG_CO_DEBUG */
	    pSdo->toggleBit = (canMsg->pData[0] & SDO_TOGGLE_BIT);
	    dnLdSeg_ind(pSdo, canMsg->pData CO_COMMA_LINE_PARA);
	    break;
# endif /* not CONFIG_SEG_SDO */

	case CCS_INI_UP_LD_REQ : /* SERVER Initiate Upload Indication */
	    if (pSdo->state != SDOSTATE_READY)  {
# ifdef CONFIG_CO_DEBUG
		BDEBUG(CO_DEBUG_SDO, "Bad State for received Message\n");
# endif /* CONFIG_CO_DEBUG */
		(void)abortSdoTransf_Req(pSdo, CO_E_SDO_CMD_SPEC_INVALID
			CO_COMMA_LINE_PARA);
		return;
	    }
	    pSdo->toggleBit = SDO_TOGGLE_BIT;

# ifdef CONFIG_CO_DEBUG
	    BDEBUG(CO_DEBUG_SDO, "init Upload Indication\n");
# endif /* CONFIG_CO_DEBUG */
	    if (initUpLd_ind(pSdo, canMsg->pData CO_COMMA_LINE_PARA) == CO_OK) {
		initUpLd_res(pSdo CO_COMMA_LINE_PARA);
	    }
	    break;

# ifdef CONFIG_SEG_SDO
	case CCS_UP_LD_SEG_REQ : /* SERVER Upload Segment Indication */
	    if (pSdo->state != SDOSTATE_UPLD_INIT)  {
#  ifdef CONFIG_CO_DEBUG
		BDEBUG(CO_DEBUG_SDO, "Bad State for received Message\n");
#  endif /* CONFIG_CO_DEBUG */
		return;
	    }
	    /* if toggle not alternates ignore message */
	    if (pSdo->toggleBit == (canMsg->pData[0] & SDO_TOGGLE_BIT)) {
		(void)abortSdoTransf_Req(pSdo, CO_E_SDO_INVALID_TOGGLEBIT
			CO_COMMA_LINE_PARA);
		return;
	    }
	    pSdo->toggleBit = (canMsg->pData[0] & SDO_TOGGLE_BIT);
#  ifdef CONFIG_CO_DEBUG
	    BDEBUG(CO_DEBUG_SDO, "Upload Segment Indication\n");
#  endif /* CONFIG_CO_DEBUG */
	    upLdSeg_ind(pSdo CO_COMMA_LINE_PARA);
	    break;
# endif /* not CONFIG_SEG_SDO */

# ifdef CONFIG_SDO_BLOCKTRANSFER
	case CO_SDOBLK_CCS_DOWN:	/* SERVER Init/End Block Download */
	    /* select init or end of block transfer */
	    if ((canMsg->pData[0] & CO_SDOBLK_SS_END) != 0) {
#  ifdef CONFIG_CO_DEBUG
		BDEBUG(CO_DEBUG_SDO, "End Download Block Indication\n");
#  endif /* CONFIG_CO_DEBUG */
		endDnLdBlk_ind(pSdo, canMsg->pData CO_COMMA_LINE_PARA);
	    } else {
#  ifdef CONFIG_CO_DEBUG
		BDEBUG(CO_DEBUG_SDO, "Init Download Block Indication\n");
#  endif /* CONFIG_CO_DEBUG */

		initDnLdBlk_ind(pSdo, canMsg->pData CO_COMMA_LINE_PARA);
	    }
	    break;

	case CO_SDOBLK_CCS_UP:	/* SERVER Init Block Upload */
	    if ((pSdo->state != SDOSTATE_UPLD_BLK_INIT) &&
	        (pSdo->state != SDOSTATE_UPLD_BLK_SEG) &&
		(pSdo->state != SDOSTATE_UPLD_BLK_END) &&
		(pSdo->state != SDOSTATE_READY)) {
#  ifdef CONFIG_CO_DEBUG
		BDEBUG(CO_DEBUG_SDO, "Bad State for received Message\n");
#  endif /* CONFIG_CO_DEBUG */
		/* Abort Transfer */
		(void)abortSdoTransf_Req(pSdo, CO_E_SDO_CMD_SPEC_INVALID
			CO_COMMA_LINE_PARA);
		return;
	    }

#  ifdef CONFIG_CO_DEBUG
	    BDEBUG(CO_DEBUG_SDO, "Upload Block Indication\n");
#  endif /* CONFIG_CO_DEBUG */
	    initUpLdBlk_ind(pSdo, canMsg->pData CO_COMMA_LINE_PARA);
	    break;
# endif /* CONFIG_SDO_BLOCKTRANSFER */

	case CS_ABORT_TRANSFER:		/* Abort Transfer */
	    /* for CANopen application will not informed */
	    pSdo->state = SDOSTATE_READY;

# ifdef CONFIG_SDO_SERVER_ABORT_IND
	    {
	    UNSIGNED32	abortCode;

		CO_PACK_MEMCPY((UNSIGNED8 *)&abortCode, &canMsg->pData[4], 4,
		    CO_NUM_VAL);
		sdoServerAbortInd(pSdo->index, pSdo->subIndex, abortCode
		    CO_COMMA_LINE_PARA);
	    }
# endif /* CONFIG_SDO_SERVER_ABORT_IND */
	    break;

	default:	/* unknown error */
	    CMS_SdoDecode(pSdo->index, pSdo->subIndex, canMsg->pData);
# ifdef CONFIG_REDUNDANCY_SUPPORT
	    pSdo->commLine = getReduncyReceivedLine(CO_GLOBVARS_PARA);
# endif /* CONFIG_REDUNDANCY_SUPPORT */
	    (void)abortSdoTransf_Req(pSdo, CO_E_SDO_CMD_SPEC_INVALID
			CO_COMMA_LINE_PARA);
	    break;
     }
}


/*******************************************************************
*
* initDnLd_ind - Download SDO indication for CANopen
*
* NOMANUAL
*
* This function indicate a download domain request of a CANopen client.
* It puts the domain data to the objectdictionary.
* A buffer will be allocated for SDOs, which contain more than 7 Bytes.
*
* RETURNS
* .TP
* nothing
*
*/

void initDnLd_ind(
	SDO_T	*pCurSdo,	/* pointer to current sdo */
	UNSIGNED8 *canBuf	/* pointer to can buffer */
	CO_COMMA_LINE_PARA_DECL	/**< number of CAN line 0..CO_MAX_CAN_LINES-1 */
     )
{
RET_T 		commonRet;	/* common return value */
UNSIGNED8	*pVar;		/* pointer to data address */
UNSIGNED32      objSize;	/* size of data */
UNSIGNED16 	index;		/* index */
UNSIGNED8	subIndex;	/* subindex */
UNSIGNED8	*pData;		/* pointer to SDO data */
UNSIGNED32	sdoSize = 0u;	/* SDO size */

# ifdef CONFIG_REDUNDANCY_SUPPORT
    pCurSdo->commLine = getReduncyReceivedLine(CO_GLOBVARS_PARA);
# endif /* CONFIG_REDUNDANCY_SUPPORT */

    CMS_SdoDecode(pCurSdo->index, pCurSdo->subIndex, canBuf);

    /* get index and subIndex */
    /* for faster access */
    index = pCurSdo->index;
    subIndex = pCurSdo->subIndex;

    /* detect the transfer typ */
    switch (*canBuf & CO_SDO_SIZE_TYPE_MASK) {
	/*
	 * normal transfer
	 * data set size is indicated
	 * d contains number of data bytes to be downloaded
	 */
	case 1:
	    CO_PACK_MEMCPY((UNSIGNED8 *)&sdoSize, canBuf + 4, 4, CO_NUM_VAL);
	    pData = NULL;
	    break;
	/*
	 * expedited transfer
	 * data set size is indicated
	 * d contains data
	 * n contains data number
	 */
	case 2:
# ifdef CONFIG_SDO_NO_SIZE_INDICATED
	    commonRet = getObjAddr(index, subIndex, &pVar, &sdoSize
			 CO_COMMA_LINE_PARA);
	    if (commonRet != CO_OK) {
		(void)abortSdoTransf_Req(pCurSdo, commonRet CO_COMMA_GLOBVARS_PARA);
		return;
	    }
# else /* CONFIG_SDO_NO_SIZE_INDICATED */
	     /* if no size indicator size always 4 */
	    sdoSize = 4u;
# endif /* CONFIG_SDO_NO_SIZE_INDICATED */

	    pData = canBuf + 4u;
	    break;

	case 3:
	    sdoSize = 4u - ((*canBuf & 0x0Cu) >> 2u);
	    pData = canBuf + 4u;
	    /* pCurSdo->state = SDOSTATE_DNLD_SEG; */
	    break;

	default:
	   return;
    }

    /* get target address */
#ifdef CONFIG_VIRTUAL_OBJECTS
    /* for virtual objects put trough the len information
       it will be overwritten by getObjAddr */
    objSize = sdoSize;
#endif /* CONFIG_VIRTUAL_OBJECTS */

    commonRet = getObjAddr(index, subIndex, &pVar, &objSize
		 CO_COMMA_LINE_PARA);
    if (commonRet != CO_OK) {
	(void)abortSdoTransf_Req(pCurSdo, commonRet CO_COMMA_LINE_PARA);
	return;
    }

    pCurSdo->attr = getObjAttr(index, subIndex CO_COMMA_LINE_PARA);

    /* test target space size against data size and for valid address */
# ifdef CONFIG_DOMAIN_UPDNLD
    /*
	exception for domains
	the default value entry of the type descripion
	contains the true size value and
	the object entry contains only a pointer to the domain
    */
    if ((pCurSdo->attr & CO_UP_DN_LD_DOMAIN) != 0) {
	if ((sdoSize > getDomainSize(index, subIndex CO_COMMA_LINE_PARA))
	 || (pVar == NULL)) {
	    commonRet = CO_E_WRONG_SIZE;
	}
	pVar = getDomainAddr(index, subIndex CO_COMMA_LINE_PARA);
	if (pVar == NULL)  {
	    commonRet = CO_E_NONEXIST_OBJECT;
	}
#  ifdef CONFIG_16BIT_CPU
	/* set flag for domain download */
	pCurSdo->numeric = CO_TRUE;
#  endif /* CONFIG_16BIT_CPU */

    } else {

# endif /* CONFIG_DOMAIN_UPDNLD */

# ifdef CONFIG_16BIT_CPU
	/* reset flag for domain download */
	pCurSdo->numeric = CO_FALSE;
# endif /* CONFIG_16BIT_CPU */

	if ((pCurSdo->attr & CO_NUM_VAL) != 0u)  {
	    /* numeric value */
	    if (sdoSize != objSize)  {
		commonRet = CO_E_WRONG_SIZE;
	    }
	} else {
	    /* not numeric value */
	    if ((sdoSize > objSize) || (pVar == NULL))  {
		commonRet = CO_E_WRONG_SIZE;
	    }
# ifdef CO_CONFIG_SDO_SHORT_STRINGS
            else
            {
                /* if object is a string, set actual length */
                if ((pCurSdo->attr & CO_UP_DN_LD_STRING) != 0u)
                {
                    setStringSize(index, subIndex, sdoSize);
                }
            }
# endif /* CO_CONFIG_SDO_SHORT_STRINGS */
	}
# ifdef CONFIG_DOMAIN_UPDNLD
    }
# endif /* CONFIG_DOMAIN_UPDNLD */

    /* at this point pVar should always contain a valid address,
        this is an additonal check prompted by static code check */
    if (pVar == NULL)
    {
        commonRet = CO_E_NONEXIST_OBJECT;
	(void)abortSdoTransf_Req(pCurSdo, commonRet CO_COMMA_LINE_PARA);
        return;
    }

    /* check write permission */
    if (commonRet == CO_OK) {
	if ((pCurSdo->attr & CO_WRITE_PERM) == 0u)  {
	    commonRet = CO_E_NO_WRITE_PERM;
	}
    }

# ifdef	CONFIG_VALUE_CHECK_FUNCTION
    if (commonRet == CO_OK) {
	/* user function for value test */
	commonRet = testSdoValue(index, subIndex, (void *)pData, sdoSize
		CO_COMMA_LINE_PARA);
    }
# endif /* CONFIG_VALUE_CHECK_FUNCTION */

    /*
      If size of indicated SDO to large, target address invalid
      or access error to Object Dictionary
      return with error (Abort Domain Transfer)
    */

    if (commonRet != CO_OK) {
	(void)abortSdoTransf_Req(pCurSdo, commonRet CO_COMMA_LINE_PARA);
	return;
    }

# ifdef CO_CONFIG_ENABLE_OBJ_CALLBACK
#  ifdef CO_CONFIG_OBJ_CB_PRE_SDO_WRITE
    /* if ( commonRet == CO_OK ) */
    else
    {
        CO_OBJ_CB_T callback = NULL;
        /* check if the object has an function pointer*/
        callback = getObjFuncPtr(pCurSdo->index CO_COMMA_LINE_PARA);
        if ( callback != NULL ) {
            CO_OBJ_CB_TYPE_T callReason;
            callReason.reason = CO_OBJ_CB_TYPE_PRE_SDO_WRITE;
            callReason.serviceNbr = pCurSdo->num;
#   ifdef CO_CONFIG_ENABLE_EXTOBJ_CALLBACK
            (void) getObjAddr(pCurSdo->index, pCurSdo->subIndex, &callReason.objAccess.pData,
                &callReason.objAccess.dataSize CO_COMMA_LINE_PARA);
#   endif /* CO_CONFIG_ENABLE_EXTOBJ_CALLBACK */

            /* call the function pointer */
#   ifdef CONFIG_NO_GLOBAL_VARS
            commonRet = (*callback)( pCurSdo->index, pCurSdo->subIndex, callReason ,(void*)CO_LINE_PARA );
#   else /* CONFIG_NO_GLOBAL_VARS */
            commonRet = (*callback)( pCurSdo->index, pCurSdo->subIndex, callReason CO_COMMA_LINE_PARA );
#   endif /* CONFIG_NO_GLOBAL_VARS */

        }
	if ( commonRet != CO_OK ) {
	    (void)abortSdoTransf_Req(pCurSdo, commonRet CO_COMMA_LINE_PARA);
	    return;
        }
    }
#  endif /* CO_CONFIG_OBJ_CB_PRE_SDO_WRITE */
# endif /* CO_CONFIG_ENABLE_OBJ_CALLBACK */


    /* save old value only for data < 5/9 byte */
    if (sdoSize <= MAX_SDO_SAVE_LEN)  {
	CO_NUM_MEMCPY(&pCurSdo->oldVar[0], pVar, sdoSize,
		pCurSdo->attr & CO_NUM_VAL);
	pCurSdo->saved = CO_TRUE;
    } /* oldSize > 4/8 */
    else {
	pCurSdo->saved = CO_FALSE;
    }

    pCurSdo->domSize = sdoSize;
    pCurSdo->restSize = sdoSize;
    pCurSdo->pDomData = pVar;

# ifdef CONFIG_DOMAIN_INDICATION_SIZE
    pCurSdo->lastBorder = 0;
# endif /* CONFIG_DOMAIN_INDICATION_SIZE */


    if (pData != NULL) {
	/* expedited transfer */
	writeSdoValue(pCurSdo, pData CO_COMMA_LINE_PARA);
	/* dnLdSeg_res will be called in writeSdoValue */
	return;
    }

# ifdef CONFIG_SEG_SDO
    /* set target address */
    pCurSdo->pActualDomData = pVar;
    initDnLd_res(pCurSdo CO_COMMA_GLOBVARS_PARA);
    pCurSdo->state = SDOSTATE_DNLD_SEG;
#  ifdef CONFIG_16BIT_CPU
    pCurSdo->halfWord = CO_FALSE;
#  endif /* CONFIG_16BIT_CPU */
# endif /* not CONFIG_SEG_SDO */
}


/*******************************************************************
*
* initUpLd_ind - Upload SDO indication for CANopen
*
* NOMANUAL
*
* This function indicates an upload domain request of a CANopen client.
* It gets the SDO data from the object dictionary.
*
* \retval CO_OK
*++ success
*-- Erfolg
* else
*++ SDO abort codes
*-- SDO Abbruch Codes
*/

RET_T initUpLd_ind(
	SDO_T	*pCurSdo,	/* pointer to current sdo */
	UNSIGNED8	*canBuf		/* pointer to can buffer */
	CO_COMMA_LINE_PARA_DECL	/**< number of CAN line 0..CO_MAX_CAN_LINES-1 */
   )
{
LIST_ELEMENT_T *curObj;	    /* pointer to current object */
RET_T 		commonRet;  /* common return value */
UNSIGNED8	*pData;	    /* pointer to data address */
UNSIGNED16 	index;      /* index */
UNSIGNED8	subIndex;   /* subindex */
UNSIGNED32	tmpSize = 0xfffffffful;    /* size of data */
#ifdef CONFIG_SPLIT_INDICATION
#else /* CONFIG_SPLIT_INDICATION */
# if defined(CONFIG_BIG_ENDIAN) || defined(CONFIG_16BIT_CPU)
static UNSIGNED8 tmpBuf[CO_MAX_NUMDATA_SIZE]; /* convert buffer */
# endif /* defined(CONFIG_BIG_ENDIAN) || defined(CONFIG_16BIT_CPU) */
#endif /* CONFIG_SPLIT_INDICATION */

# ifdef CONFIG_REDUNDANCY_SUPPORT
    pCurSdo->commLine = getReduncyReceivedLine(CO_GLOBVARS_PARA);
# endif /* CONFIG_REDUNDANCY_SUPPORT */

    CMS_SdoDecode(pCurSdo->index, pCurSdo->subIndex, canBuf);

    /* for faster access */
    index = pCurSdo->index;
    subIndex = pCurSdo->subIndex;

# ifdef CONFIG_VIRTUAL_OBJECTS
    /* special for customer support - pointer to CAN data */
    pData = canBuf;
# endif /* CONFIG_VIRTUAL_OBJECTS */

    /* index value exceeds the physical limitations */
    curObj = searchObj(index CO_COMMA_LINE_PARA);

    /* read contents of object dictionary to SDO */
    commonRet = getObjPtrAddr(curObj, index, subIndex, &pData,&tmpSize CO_COMMA_LINE_PARA);
    if (commonRet != CO_OK) {
	return(abortSdoTransf_Req(pCurSdo, commonRet CO_COMMA_LINE_PARA));
    }

    /* test read permission */
    commonRet = getObjPtrAttr(curObj, index, subIndex, &pCurSdo->attr  CO_COMMA_LINE_PARA);
    if (commonRet != CO_OK) {
	return(abortSdoTransf_Req(pCurSdo, commonRet CO_COMMA_LINE_PARA));
    }

    if ((pCurSdo->attr & CO_READ_PERM) != CO_READ_PERM) {
	return(abortSdoTransf_Req(pCurSdo, CO_E_NO_READ_PERM
			CO_COMMA_LINE_PARA));
    }

# ifdef CO_CONFIG_SDO_SHORT_STRINGS
    /* if object is a string, use actual length */
    if ((pCurSdo->attr & CO_UP_DN_LD_STRING) != 0u)
    {
        getStringSize(index, subIndex, &tmpSize);
    }
# endif /* CO_CONFIG_SDO_SHORT_STRINGS */

    /* special behavoir for some comm-profile entries */
    if (index <= END_COM_PROF) {
	commonRet = checkCommParAccess(index, subIndex CO_COMMA_LINE_PARA);
	if (commonRet != CO_OK) {
	    return(abortSdoTransf_Req(pCurSdo, commonRet
			CO_COMMA_LINE_PARA));
	}
    }

    /* inform application */
    /* transmit actual value from device to object dictionary */

# ifdef CONFIG_SPLIT_INDICATION
    commonRet = sdoRdInd(index, subIndex, pCurSdo->num CO_COMMA_LINE_PARA);
#  ifdef CO_CONFIG_ENABLE_OBJ_CALLBACK
#   ifdef CO_CONFIG_OBJ_CB_PRE_SDO_READ
    if ( commonRet == CO_OK ) {
        CO_OBJ_CB_T callback = NULL;
        /* check if the object has an function pointer*/
        callback = getObjPtrFuncPtr(curObj, index CO_COMMA_LINE_PARA);
        if ( callback != NULL ) {
            CO_OBJ_CB_TYPE_T callReason;
            callReason.reason = CO_OBJ_CB_TYPE_PRE_SDO_READ;
            callReason.serviceNbr = pCurSdo->num;
#   ifdef CO_CONFIG_ENABLE_EXTOBJ_CALLBACK
			callReason.objAccess.pData = pData;
			callReason.objAccess.dataSize = tmpSize;
#   endif /* CO_CONFIG_ENABLE_EXTOBJ_CALLBACK */

            /* call the function pointer */
#   ifdef CONFIG_NO_GLOBAL_VARS
            commonRet = (*callback)( index, subIndex, callReason ,(void*)CO_LINE_PARA );
#   else /* CONFIG_NO_GLOBAL_VARS */
            commonRet = (*callback)( index, subIndex, callReason CO_COMMA_LINE_PARA );
#   endif /* CONFIG_NO_GLOBAL_VARS */
        }
    }
#   endif /* CO_CONFIG_OBJ_CB_PRE_SDO_READ */
#  endif /* CO_CONFIG_ENABLE_OBJ_CALLBACK */
    if (commonRet != CO_OK) {
        if (commonRet != CO_SDO_IND_BUSY) {

#  ifdef CO_CONFIG_ENABLE_OBJ_CALLBACK
#   ifdef CO_CONFIG_OBJ_CB_POST_SDO_READ
			{
				CO_OBJ_CB_T callback = NULL;
				/* check if the object has an function pointer*/
				callback = getObjPtrFuncPtr(curObj, index CO_COMMA_LINE_PARA);
				if (callback != NULL) {
					CO_OBJ_CB_TYPE_T callReason;
					callReason.reason = CO_OBJ_CB_TYPE_POST_SDO_READ_ABORT;
					callReason.serviceNbr = pCurSdo->num;
#   ifdef CO_CONFIG_ENABLE_EXTOBJ_CALLBACK
					callReason.objAccess.pData = pData;
					callReason.objAccess.dataSize = tmpSize;
#   endif /* CO_CONFIG_ENABLE_EXTOBJ_CALLBACK */
					/* call the function pointer */
#   ifdef CONFIG_NO_GLOBAL_VARS
					(void)(*callback)(index, subIndex, callReason, (void*)CO_LINE_PARA);
#   else /* CONFIG_NO_GLOBAL_VARS */
					(void)(*callback)(index, subIndex, callReason CO_COMMA_LINE_PARA);
#   endif /* CONFIG_NO_GLOBAL_VARS */
				}
			}
#   endif /* CO_CONFIG_OBJ_CB_POST_SDO_READ */
#  endif /* CO_CONFIG_ENABLE_OBJ_CALLBACK */

            return(abortSdoTransf_Req(pCurSdo, commonRet CO_COMMA_LINE_PARA));
        }
        /* now we wait for finish the indication */
        /* therefore the user have to call the function finishSdoRdInd() */
        pCurSdo->state = SDOSTATE_IND_BUSY;
        return(commonRet);
    }
# else /* CONFIG_SPLIT_INDICATION */
    commonRet = sdoRdInd(index, subIndex CO_COMMA_LINE_PARA);
#  ifdef CO_CONFIG_ENABLE_OBJ_CALLBACK
#   ifdef CO_CONFIG_OBJ_CB_PRE_SDO_READ
    if ( commonRet == CO_OK ) {
        CO_OBJ_CB_T callback = NULL;
        /* check if the object has an function pointer*/
        callback = getObjPtrFuncPtr(curObj, index CO_COMMA_LINE_PARA);
        if ( callback != NULL ) {
            CO_OBJ_CB_TYPE_T callReason;
            callReason.reason = CO_OBJ_CB_TYPE_PRE_SDO_READ;
            callReason.serviceNbr = pCurSdo->num;
#   ifdef CO_CONFIG_ENABLE_EXTOBJ_CALLBACK
			callReason.objAccess.pData = pData;
			callReason.objAccess.dataSize = tmpSize;
#   endif /* CO_CONFIG_ENABLE_EXTOBJ_CALLBACK */
            /* call the function pointer */
#   ifdef CONFIG_NO_GLOBAL_VARS
            commonRet = (*callback)( index, subIndex, callReason ,(void*)CO_LINE_PARA );
#   else /* CONFIG_NO_GLOBAL_VARS */
            commonRet = (*callback)( index, subIndex, callReason CO_COMMA_LINE_PARA );
#   endif /* CONFIG_NO_GLOBAL_VARS */
        }
    }
#   endif /* CO_CONFIG_OBJ_CB_PRE_SDO_READ */
#  endif /* CO_CONFIG_ENABLE_OBJ_CALLBACK */
    if (commonRet != CO_OK) {

#  ifdef CO_CONFIG_ENABLE_OBJ_CALLBACK
#   ifdef CO_CONFIG_OBJ_CB_POST_SDO_READ
		{
			CO_OBJ_CB_T callback = NULL;
			/* check if the object has an function pointer*/
			callback = getObjPtrFuncPtr(curObj, index CO_COMMA_LINE_PARA);
			if (callback != NULL) {
				CO_OBJ_CB_TYPE_T callReason;
				callReason.reason = CO_OBJ_CB_TYPE_POST_SDO_READ_ABORT;
				callReason.serviceNbr = pCurSdo->num;
#   ifdef CO_CONFIG_ENABLE_EXTOBJ_CALLBACK
				callReason.objAccess.pData = pData;
				callReason.objAccess.dataSize = tmpSize;
#   endif /* CO_CONFIG_ENABLE_EXTOBJ_CALLBACK */
				/* call the function pointer */
#   ifdef CONFIG_NO_GLOBAL_VARS
				(void)(*callback)(index, subIndex, callReason, (void*)CO_LINE_PARA);
#   else /* CONFIG_NO_GLOBAL_VARS */
				(void)(*callback)(index, subIndex, callReason CO_COMMA_LINE_PARA);
#   endif /* CONFIG_NO_GLOBAL_VARS */
			}
		}
#   endif /* CO_CONFIG_OBJ_CB_POST_SDO_READ */
#  endif /* CO_CONFIG_ENABLE_OBJ_CALLBACK */

        return(abortSdoTransf_Req(pCurSdo, commonRet CO_COMMA_LINE_PARA));
    }
# endif /* CONFIG_SPLIT_INDICATION */


# ifdef CONFIG_SPLIT_INDICATION
    /* generate answer */
    return initUpLd_ind_finish(pCurSdo CO_COMMA_LINE_PARA);
}


/****************************************************************************/
/*
*++ \brief initUpLd_ind_finish - finish initUpLd_ind function
*-- \brief initUpLd_ind_finish - beendet die initUpLd_ind Funktion
*
* NOMANUAL
*
*-- Diese Funktion beendet die initUpLd_ind Funktion
*
* \retval
*++ success
*-- siehe initUpLd_ind
*
*/
RET_T initUpLd_ind_finish(
	SDO_T	*pCurSdo
	CO_COMMA_LINE_PARA_DECL	/* number of CAN line 0..CO_MAX_CAN_LINES-1 */
    )
{
LIST_ELEMENT_T *curObj;	    /* pointer to current object */
RET_T 		commonRet = CO_OK;  /* common return value */
UNSIGNED8	*pData;	    /* pointer to data address */
UNSIGNED16 	index;      /* index */
UNSIGNED8	subIndex;   /* subindex */
UNSIGNED32	tmpSize = 0xffffffff;    /* size of data */
#  if defined(CONFIG_BIG_ENDIAN) || defined(CONFIG_16BIT_CPU)
static UNSIGNED8 tmpBuf[CO_MAX_NUMDATA_SIZE];
#  endif /* defined(CONFIG_BIG_ENDIAN) || defined(CONFIG_16BIT_CPU) */

    /* for faster access */
    index = pCurSdo->index;
    subIndex = pCurSdo->subIndex;

    /* get attribute again */
    /* index value exceeds the physical limitations */
    curObj = searchObj(index CO_COMMA_LINE_PARA);

    getObjPtrAttr( curObj, index, subIndex, & pCurSdo->attr CO_COMMA_LINE_PARA);
# endif /* CONFIG_SPLIT_INDICATION */

# ifdef CONFIG_VIRTUAL_OBJECTS
#  ifdef CONFIG_SPLIT_INDICATION
    pData = NULL;
#  else /* CONFIG_SPLIT_INDICATION */
    /* special for customer support - pointer to CAN data */
    pData = canBuf;
#  endif /* CONFIG_SPLIT_INDICATION */
# endif /* CONFIG_VIRTUAL_OBJECTS */

    /* read object dictionary to SDO again
     *	(after change from user)
     * this can't get an error, therefore this was done before*/
    (void)getObjPtrAddr(curObj, index, subIndex, &pData, &tmpSize CO_COMMA_LINE_PARA);
# ifdef CO_CONFIG_SDO_SHORT_STRINGS
    /* if object is a string, use actual length */
    if ((pCurSdo->attr & CO_UP_DN_LD_STRING) != 0u)
    {
        getStringSize(index, subIndex, &tmpSize);
    }
# endif /* CO_CONFIG_SDO_SHORT_STRINGS */

# ifdef CONFIG_DOMAIN_UPDNLD
    /*
	exception for domains
	the default value entry of the type descripion
	contains the true size value and
	the object entry contains only a pointer to the domain
     */
    if ((pCurSdo->attr & CO_UP_DN_LD_DOMAIN) != 0) {
	pData = getDomainAddr(index, subIndex CO_COMMA_LINE_PARA);
	if (pData == NULL)  {
	    commonRet = CO_E_NONEXIST_OBJECT;
	}
#  ifdef CONFIG_16BIT_CPU
	/* set flag for domain upload */
	pCurSdo->numeric = CO_TRUE;
	pCurSdo->halfWord = CO_FALSE;
#  endif /* CONFIG_16BIT_CPU */

    } else {

# endif /* CONFIG_DOMAIN_UPDNLD */

# ifdef CONFIG_16BIT_CPU
	/* set flag for domain upload */
	pCurSdo->numeric = CO_FALSE;
# endif /* CONFIG_16BIT_CPU */

# if defined(CONFIG_BIG_ENDIAN) || defined(CONFIG_16BIT_CPU)
        if ((pCurSdo->attr & CO_NUM_VAL) == CO_NUM_VAL)
        {
	    CO_UNPACK_MEMCPY(tmpBuf, pData, tmpSize, pCurSdo->attr & CO_NUM_VAL);
	    pData = tmpBuf;
	}
# endif /* defined(CONFIG_BIG_ENDIAN) || defined(CONFIG_16BIT_CPU) */

# ifdef CONFIG_DOMAIN_UPDNLD
    }
# endif /* CONFIG_DOMAIN_UPDNLD */

    if (commonRet != CO_OK) {
	commonRet = abortSdoTransf_Req(pCurSdo, commonRet CO_COMMA_LINE_PARA);
    } else {

	pCurSdo->pActualDomData = pData;
	pCurSdo->pDomData = pData;
	pCurSdo->restSize = tmpSize;
	pCurSdo->domSize = tmpSize;

# ifdef CONFIG_DOMAIN_INDICATION_SIZE
	if ((pCurSdo->attr & CO_UP_DN_LD_DOMAIN) != 0) {
	    pCurSdo->maxBufNum = 0;
	    pCurSdo->actBufNum = 0;
	    pCurSdo->domSizeBuffered = 0;
	    pCurSdo->overByteNum = 0;

	    /* fill domain buffer */
	    commonRet = pcoUpLdBufUpdate(pCurSdo CO_COMMA_LINE_PARA);
	}
# endif /* CONFIG_DOMAIN_INDICATION_SIZE */

    }
    return(commonRet);
}


# ifdef CONFIG_SPLIT_INDICATION
/****************************************************************************/
/**
*++ \brief finishSdoRdInd - finish sdoRdInd
*-- \brief finishSdoRdInd - Abschluß der SdoRdInd-Funktion
*
*++ This function finishes the sdoRdInd call
*++ if the function has returned
*-- Diese Funktion beendet die SdoRdInd Funktion,
*-- wenn diese vorher mit dem Rückgabewert
*\c CO_SDO_IND_BUSY
*-- verlassen wurde.
*
*-- Normalerweise wird nach Beendigung der SdoRdInd Funktion
*-- die Antwort für die SDO Anfrage generiert
*-- und zum Server zurückgesendet.
*-- Wenn der Anwender die Indikation Funktion
*-- mit diesem speziellen Rückgabewert verläßt,
*-- wird die Programmabarbeitung der SDO Anfrage unterbrochen
*-- und erst mit dem Aufruf dieser Funktion fortgesetzt.
*-- Der Anwender hat somit die Möglichkeit,
*-- Antworten auf SDO Anfragen hinauszuzögern,
*-- um z. B. externe Ereignisse abzuwarten.
*++ Normally the response of a request is generated and
*++ sent back to the SDO-Server
*++ after sdoRdInd was called.
*++ If SdoRdInd returns with this special value
*++ processing of finishSdoRdInd is discontinued and is only finished if
*++ it is called a second time from the user application..
*++ By doing so the user application has the opportunity
*++ to delay a SDO-response in order to wait for an external event.
*
* \retval CO_OK
*++ success
*-- Erfolg
* \retval CO_E_NOT_EXIST
*++ SDO doesn't exist
*-- SDO existiert nicht
* \retval CO_E_STATE);
*++ SDO not up to date (Client doesn't send abort or no waiting SDO)
*-- SDO nicht mehr aktuell (Client sendet abort, oder kein wartendes SDO)
*
*/
RET_T finishSdoRdInd(
	UNSIGNED8	sdoNr,		/**< SDO number */
	RET_T		retCode		/**< returnval sdoInd */
	CO_COMMA_LINE_PARA_DECL	/**< number of CAN line 0..CO_MAX_CAN_LINES-1 */
    )
{
SDO_T	*pCurSdo;

    /* search for sdo structure */
    pCurSdo = searchForServerSdoNr(sdoNr CO_COMMA_LINE_PARA);
    if (pCurSdo == NULL)  {
	return(CO_E_NOT_EXIST);
    }

    /* check, if the indication is waiting and the client has not aborted */
    if ((pCurSdo->state & (SDOSTATE_IND_BUSY | SDOSTATE_READY))
	!= SDOSTATE_IND_BUSY)  {
	return(CO_E_STATE);
    }

    /* if retval from indication not CO_OK, start abort */
    if (retCode != CO_OK) {
	return(abortSdoTransf_Req(pCurSdo, retCode CO_COMMA_LINE_PARA));
    }

    /* correct the state */
    pCurSdo->state = SDOSTATE_READY;

    (void)initUpLd_ind_finish(pCurSdo CO_COMMA_LINE_PARA);
    initUpLd_res(pCurSdo CO_COMMA_GLOBVARS_PARA);

    return(CO_OK);
}
# endif /* CONFIG_SPLIT_INDICATION */


# ifdef CONFIG_SPLIT_INDICATION
/****************************************************************************/
/**
*++ \brief finishSdoWrInd - finish sdoWrInd
*-- \brief finishSdoWrInd - Abschluß der SdoWrInd-Funktion
*
*-- Diese Funktion beendet die SdoWrInd Funktion,
*-- wenn diese vorher mit dem Rückgabewert
*++ This function will finish the function SdoWrInd
*++ if
* \c CO_SDO_IND_BUSY
*-- verlassen wurde.
*++ was returned by SdoWrInd.
*
*-- Normalerweise wird nach Beendigung der SdoWrInd Funktion
*-- die Antwort für die SDO Anfrage generiert
*-- und zum Server zurückgesendet.
*-- Wenn der Anwender die Indikation Funktion
*-- mit diesem speziellen Rückgabewert verläßt,
*-- wird die Programmabarbeitung der SDO Anfrage unterbrochen
*-- und erst mit dem Aufruf dieser Funktion fortgesetzt.
*-- Der Anwender hat somit die Möglichkeit,
*-- Antworten auf SDO Anfragen hinauszuzögern,
*-- um z. B. externe Ereignisse abzuwarten.
*++ Normally the response of a request is generated
*++ and is sent back to the SDO-Server
*++ after SdoWrInd was called.
*++ If the SdoWrInd returns with this special value
*++ processing of finishSdoRdInd is discontinued and will only be
*++ finished if
*++ it is called a second time from the user application..
*++ By doing so the user application has the opportunity
*++ to delay a SDO-response in order to wait for an external event.
*
*-- Um diese Funktion verwenden zu können,
*-- ist das define
* CONFIG_SPLIT_INDICATION
*-- zu setzen.
*
* \retval CO_OK
*++ success
*-- Erfolg
* \retval CO_E_NOT_EXIST
*++ SDO doesn't exist
*-- SDO existiert nicht
* \retval CO_E_STATE);
*++ SDO not up to date (Client doesn't send abort or no waiting SDO)
*-- SDO nicht mehr aktuell (Client sendet abort, oder kein wartendes SDO)
*
*/
RET_T finishSdoWrInd(
	UNSIGNED8	sdoNr,		/**< SDO number */
	RET_T		retCode		/**< returnval sdoInd */
	CO_COMMA_LINE_PARA_DECL	/**< number of CAN line 0..CO_MAX_CAN_LINES-1 */
    )
{
SDO_T		*pCurSdo;		/* pointer to current sdo */
UNSIGNED8	*pVar;			/* pointer to variable address */
UNSIGNED32	oldSize;
UNSIGNED8	objAttr = 0u;		/* object attributes */


    /* search for sdo structure */
    pCurSdo = searchForServerSdoNr(sdoNr CO_COMMA_LINE_PARA);
    if (pCurSdo == NULL)  {
	return(CO_E_NOT_EXIST);
    }

    /* check, if the indication is waiting and the client has not aborted */
    if ((pCurSdo->state & (SDOSTATE_IND_BUSY | SDOSTATE_READY))
	    != SDOSTATE_IND_BUSY)  {
	return(CO_E_STATE);
    }

    /* if retval from indication not CO_OK, start abort */
    if (retCode != CO_OK) {

	/* command failed, the data would be restored */
	if (pCurSdo->saved == CO_TRUE)  {
	    if (getObjAddr(pCurSdo->index, pCurSdo->subIndex, &pVar, &oldSize
			CO_COMMA_LINE_PARA)
		    == CO_OK)  {

		objAttr = getObjAttr(pCurSdo->index, pCurSdo->subIndex
			CO_COMMA_LINE_PARA);
		CO_NUM_MEMCPY(pVar, &pCurSdo->oldVar, oldSize,
			objAttr & CO_NUM_VAL);

		/* set communication parameter back to old values */
		if (pCurSdo->index <= END_COM_PROF) {
		    setCommPar(pCurSdo->index, pCurSdo->subIndex
			CO_COMMA_LINE_PARA);
		}
	    }
	}
	(void)abortSdoTransf_Req(pCurSdo, retCode CO_COMMA_LINE_PARA);
	return(retCode);
    }

    /* correct the state */
    pCurSdo->state &= (UNSIGNED8)~SDOSTATE_IND_BUSY;

    writeSdoValue_finished(pCurSdo CO_COMMA_LINE_PARA);

    return(CO_OK);
}
# endif /* CONFIG_SPLIT_INDICATION */

# ifdef CONFIG_DOMAIN_INDICATION_SIZE
#  ifdef CO_CONFIG_DOMAIN_INDICATION_DEFERRABLE
/****************************************************************************/
/**
*++ \brief finishSdoDomainInd - finish sdoDomainInd
*
*++ This function finishes the sdoDomainInd call
*++ if the function has returned
*\c CO_SDO_IND_BUSY
*++ .
*
*++ Normally the response of a request is generated and
*++ sent back to the SDO-Server immediatly after sdoRdInd was called.
*++ If sdoDomainInd returns with this special value
*++ processing of sdoDomainInd is discontinued and is only finished when
*++ it is called a from the user application.
*++ This enables the application to delay a SDO-response in order to wait
*++ for an external event.
*
* \retval CO_OK
*++ success
* \retval CO_E_NOT_EXIST
*++ SDO doesn't exist
* \retval CO_E_STATE);
*++ SDO not up to date (Client doesn't send abort or no waiting SDO)
*/
RET_T finishSdoDomainInd(
    UNSIGNED8   sdoNr,          /**< SDO number */
    RET_T       retCode         /**< returnval sdoInd */
    CO_COMMA_LINE_PARA_DECL     /* number of CAN line 0..CO_MAX_CAN_LINES-1 */
)
{
SDO_T       *pCurSdo;

    /* search for sdo structure */
    pCurSdo = searchForServerSdoNr(sdoNr CO_COMMA_LINE_PARA);
    if (pCurSdo == NULL)  {
        return CO_E_NOT_EXIST;
    }

    /* check, if the indication is waiting and the client has not aborted */
    if ((pCurSdo->state & (SDOSTATE_IND_BUSY | SDOSTATE_READY))
        != SDOSTATE_IND_BUSY)  {
        return CO_E_STATE;
    }

    /* if retval from indication not CO_OK, start abort */
    if (retCode != CO_OK) {
        return (abortSdoTransf_Req(pCurSdo, retCode CO_COMMA_LINE_PARA));
    }

    /* correct the state */
    pCurSdo->state = SDOSTATE_DNLD_SEG;

    finish_dnLdSeg_ind(pCurSdo CO_COMMA_LINE_PARA);

    return CO_OK;
}
#  endif /* CO_CONFIG_DOMAIN_INDICATION_DEFERRABLE */
# endif /* CONFIG_DOMAIN_INDICATION_SIZE */

# ifdef CONFIG_SEG_SDO
/*******************************************************************
*
* dnLdSeg_ind - download SDO segment indication for CANopen
*
* NOMANUAL
*
* This function concatenates the received SDO segments and puts the object into the
* dictionary of the server.
*
* CiA301: download segment indication, calls response
*
* RETURNS
*	nothing
*
*/

void dnLdSeg_ind(
	SDO_T		*pCurSdo,  /* pointer to SDO */
	UNSIGNED8	*pCanBuf  /* pointer to SDO data */
	CO_COMMA_LINE_PARA_DECL	/* number of CAN line 0..CO_MAX_CAN_LINES-1 */
    )
{
#  ifdef CONFIG_DOMAIN_INDICATION_SIZE
UNSIGNED32	actCnt;		/* actual size */
RET_T		commonRet;	/* return value */
UNSIGNED16	rest;		/* restValue */
UNSIGNED16	lastRest;	/* restValue from last call */
#  endif /* CONFIG_DOMAIN_INDICATION_SIZE */
UNSIGNED8	size;		/* segmentsize */
UNSIGNED8	buffer[MAX_SDO_SAVE_LEN];

    size = 7u - ((*pCanBuf >> 1u) & 7u);
    /* copy data to target address */
    if (pCurSdo->pActualDomData == NULL)  {
	return;
    }

    /* check for to many data */
    if (pCurSdo->restSize < size)  {
	(void)abortSdoTransf_Req(pCurSdo, CO_E_MEM CO_COMMA_LINE_PARA);
	return;
    }

    pCurSdo->restSize -= size;

#  ifdef CONFIG_16BIT_CPU
    /* if domain download */
    if ((pCurSdo->attr & CO_NUM_VAL) == CO_NUM_VAL) {
        pCurSdo->pActualDomData =
            pack_oddmemcpy(pCurSdo->pActualDomData, pCanBuf + 1,
            size, &pCurSdo->halfWord);
    }
    else
#  endif /* CONFIG_16BIT_CPU */
    {
        CO_MEMCPY(pCurSdo->pActualDomData, (pCanBuf + 1), size);

        /* increment target adress pointer */
        pCurSdo->pActualDomData += size;
    }

    if ((*pCanBuf & CO_SDO_LAST) == 0)
    {
        pCurSdo->lastSegment = CO_FALSE;
    }
    else
    {
        pCurSdo->lastSegment = CO_TRUE;
    }

#  ifdef CONFIG_DOMAIN_INDICATION_SIZE
    /* set actual byte count */
    actCnt = pCurSdo->domSize - pCurSdo->restSize;

    /* calculate rest */
    rest = (actCnt % CONFIG_DOMAIN_INDICATION_SIZE);
    /* max 7 bytes over the border */
    if (rest < 7)
    {
        /* check, if this border not already signed */
        if (actCnt > (pCurSdo->lastBorder + 7u))
        {
            /* call indication */
            commonRet = sdoDomainInd(pCurSdo->index, pCurSdo->subIndex,
                    pCurSdo->pDomData,
                    actCnt - pCurSdo->lastBorder - rest, rest
#   ifdef CO_CONFIG_DOMAIN_INDICATION_DEFERRABLE
                    , pCurSdo->num,
                    CO_TRUE
#   endif /* CO_CONFIG_DOMAIN_INDICATION_DEFERRABLE */
                    CO_COMMA_LINE_PARA);
            if (commonRet != CO_OK)
            {
#   ifdef CO_CONFIG_DOMAIN_INDICATION_DEFERRABLE
                if (commonRet != CO_SDO_IND_BUSY)
#   endif /* CO_CONFIG_DOMAIN_INDICATION_DEFERRABLE */
                {
                    (void)abortSdoTransf_Req(pCurSdo, commonRet CO_COMMA_LINE_PARA);
                        return;
                }
            }
            /* save last border */
            pCurSdo->lastBorder = actCnt;

            /* reset pointer */
            pCurSdo->pActualDomData = pCurSdo->pDomData;

#   ifdef CO_CONFIG_DOMAIN_INDICATION_DEFERRABLE
            if (commonRet == CO_SDO_IND_BUSY)
            {
                pCurSdo->state = SDOSTATE_IND_BUSY;
                pCurSdo->state &= (UNSIGNED8) ~SDOSTATE_READY;
                return;
            }
#   endif /* CO_CONFIG_DOMAIN_INDICATION_DEFERRABLE */
        }
    }
#  endif /* CONFIG_DOMAIN_INDICATION_SIZE */

#  ifdef CONFIG_DOMAIN_INDICATION_SIZE
#   ifdef CO_CONFIG_DOMAIN_INDICATION_DEFERRABLE

    (void) finish_dnLdSeg_ind(pCurSdo CO_COMMA_LINE_PARA);
    return;

}

/*******************************************************************
*
* finish_dnLdSeg_ind - finishes dnLdSeg_ind if CO_CONFIG_DOMAIN_INDICATION_DEFERRABLE is set
*
* NOMANUAL
*
* This function finishes dnLdSeg_ind if CO_CONFIG_DOMAIN_INDICATION_DEFERRABLE was set.
* In case of delayed execution of the answer, it is called in finishSdoDomainInd(..), else
* it is called directly from dnLdSeg_ind.
*
* RETURNS
  nothing
*/
void finish_dnLdSeg_ind(
    SDO_T       *pCurSdo        /**< SDO  */
    CO_COMMA_LINE_PARA_DECL     /* number of CAN line 0..CO_MAX_CAN_LINES-1 */
)
{

UNSIGNED32	actCnt;		/* actual size */
RET_T		commonRet;	/* return value */
UNSIGNED16	rest;		/* restValue */
UNSIGNED16	lastRest;	/* restValue from last call */
UNSIGNED8	size;		/* segmentsize */
UNSIGNED8	buffer[MAX_SDO_SAVE_LEN];

#   endif /* CO_CONFIG_DOMAIN_INDICATION_DEFERRABLE */
#  endif /* CONFIG_DOMAIN_INDICATION_SIZE */

    /* if not last message */
    if (pCurSdo->lastSegment == CO_FALSE)
    {
        dnLdSeg_res(pCurSdo CO_COMMA_GLOBVARS_PARA);
        return;
    }

#  ifdef CONFIG_DOMAIN_INDICATION_SIZE
    /* set actual byte count */
    actCnt = pCurSdo->domSize - pCurSdo->restSize;

    /* not all bytes saved ? */
    if (actCnt != pCurSdo->lastBorder)  {
	/* count bytes to save */
	rest = actCnt - pCurSdo->lastBorder;
	/* rest from last call */
	lastRest = (pCurSdo->lastBorder % CONFIG_DOMAIN_INDICATION_SIZE);

	/* can all data be saved with one call ? */
	if ((lastRest + rest) > CONFIG_DOMAIN_INDICATION_SIZE)  {
	    commonRet = sdoDomainInd(pCurSdo->index, pCurSdo->subIndex,
		    pCurSdo->pDomData,
		    CONFIG_DOMAIN_INDICATION_SIZE - lastRest,
		    rest - CONFIG_DOMAIN_INDICATION_SIZE - lastRest
#   ifdef CO_CONFIG_DOMAIN_INDICATION_DEFERRABLE
                    , pCurSdo->num,
                    CO_FALSE
#   endif /* CO_CONFIG_DOMAIN_INDICATION_DEFERRABLE */
		    CO_COMMA_LINE_PARA);
	} else {
	    commonRet = sdoDomainInd(pCurSdo->index, pCurSdo->subIndex,
		    pCurSdo->pDomData, rest, 0
#   ifdef CO_CONFIG_DOMAIN_INDICATION_DEFERRABLE
                    , pCurSdo->num,
                    CO_FALSE
#   endif /* CO_CONFIG_DOMAIN_INDICATION_DEFERRABLE */
                    CO_COMMA_LINE_PARA);
	}
        if (commonRet != CO_OK)
        {
            (void)abortSdoTransf_Req(pCurSdo, commonRet CO_COMMA_LINE_PARA);
            return;
        }
    }

    commonRet = sdoDomainInd(pCurSdo->index, pCurSdo->subIndex,
	    pCurSdo->pDomData, 0, 0
#   ifdef CO_CONFIG_DOMAIN_INDICATION_DEFERRABLE
                    , pCurSdo->num,
                    CO_FALSE
#   endif /* CO_CONFIG_DOMAIN_INDICATION_DEFERRABLE */
                    CO_COMMA_LINE_PARA);
    if (commonRet != CO_OK)
    {
        (void)abortSdoTransf_Req(pCurSdo, commonRet CO_COMMA_LINE_PARA);
        return;
    }
#  endif /* CONFIG_DOMAIN_INDICATION_SIZE */

    if (pCurSdo->domSize <= MAX_SDO_SAVE_LEN) {
	/* data are already saved at OD
	 * but for big-endian and 16bit
	 * Therefore we copy the data into a temporary buffer
	 */
	CO_NUM_MEMCPY(buffer, pCurSdo->pDomData, pCurSdo->domSize,
	    pCurSdo->attr & CO_NUM_VAL);
    }

    writeSdoValue(pCurSdo, buffer CO_COMMA_LINE_PARA);
    /* dnLdSeg_res will be called in writeSdoValue */

    return;
}



/*******************************************************************
*
* upLdSeg_ind - upload SDO segment indication for CANopen
*
* NOMANUAL
*
* This function gets the object from the dictonary, splits off a segment and uploads it to
* the client. In CiA-301, this function is upload segment indication AND response.
*
* RETURNS
* .TP
* nothing
*
*/

void upLdSeg_ind(
	SDO_T	*pCurSdo	/* pointer to current sdo */
	CO_COMMA_LINE_PARA_DECL
    )
{
UNSIGNED8 	size;			/* size of next segment */
UNSIGNED8 	contFlag;		/* continue transfer flag */
UNSIGNED8	tData[8];		/* transmit data */

#  ifdef CO_CONFIG_DOMAIN_UNKNOWN_SIZE
    /* sizeless: ask application for restsize */
    if ((pCurSdo->attr & CO_UP_DN_LD_DOMAIN_SIZELESS) != 0)
    {
        if (pCurSdo->restSize <= 7)
        {
            /* printf("upLdSeg_ind: ask for size\n"); */
            pCurSdo->restSize = coUserSdoDomainSizeInd(pCurSdo->num CO_COMMA_LINE_PARA);
        }
        else
        {
            /* printf("upLdSeg_ind: size bigger than 1 message: %lu\n",pCurSdo->restSize); */
        }
    }
#  endif /* CO_CONFIG_DOMAIN_UNKNOWN_SIZE */

    if (pCurSdo->restSize > 7UL) {
	contFlag = CO_SDO_MORE;
	size = 7;
    } else {
	contFlag = CO_SDO_LAST;
	size = (UNSIGNED8)pCurSdo->restSize;
	pCurSdo->state = SDOSTATE_READY;
    }

    tData[0] = (UNSIGNED8)(SCS_UP_LD_SEG_RES | pCurSdo->toggleBit
		   | ((7u - size) << 1u) | contFlag);
    memset(&tData[1], (int)0, (size_t)7);

#  ifdef CONFIG_DOMAIN_INDICATION_SIZE
    if ((pCurSdo->attr & CO_UP_DN_LD_DOMAIN) != 0) {
        /* update intermediate domain buffer if necessary */
        (void)pcoUpLdBufUpdate(pCurSdo CO_COMMA_LINE_PARA);
    }
#  endif /* CONFIG_DOMAIN_INDICATION_SIZE */

#  ifdef CO_CONFIG_ENABLE_OBJ_CALLBACK
#   ifdef CO_CONFIG_OBJ_CB_POST_SDO_READ
    if ( contFlag == CO_SDO_LAST ) {
        CO_OBJ_CB_T callback = NULL;
        /* check if the object has an function pointer*/
        callback = getObjFuncPtr(pCurSdo->index CO_COMMA_LINE_PARA);
        if ( callback != NULL ) {
            CO_OBJ_CB_TYPE_T callReason;
            callReason.reason = CO_OBJ_CB_TYPE_POST_SDO_READ;
            callReason.serviceNbr = pCurSdo->num;
#    ifdef CO_CONFIG_ENABLE_EXTOBJ_CALLBACK
            (void) getObjAddr(pCurSdo->index, pCurSdo->subIndex, &callReason.objAccess.pData,
                &callReason.objAccess.dataSize CO_COMMA_LINE_PARA);
#    endif /* CO_CONFIG_ENABLE_EXTOBJ_CALLBACK */
            /* call the function pointer */
#    ifdef CONFIG_NO_GLOBAL_VARS
            (void)(*callback)( pCurSdo->index, pCurSdo->subIndex, callReason, (void*)CO_LINE_PARA );
#    else /* CONFIG_NO_GLOBAL_VARS */
            (void)(*callback)( pCurSdo->index, pCurSdo->subIndex, callReason CO_COMMA_LINE_PARA );
#    endif /* CONFIG_NO_GLOBAL_VARS */
        }
    }
#   endif /* CO_CONFIG_OBJ_CB_POST_SDO_READ */
#  endif /* CO_CONFIG_ENABLE_OBJ_CALLBACK */

#  ifdef CONFIG_DOMAIN_INDICATION_SIZE
    if (pCurSdo->overByteNum != 0)
    {
        CO_MEMCPY(&tData[1], pCurSdo->overByteBuf, pCurSdo->overByteNum);
    }
#  endif /* CONFIG_DOMAIN_INDICATION_SIZE */

#  ifdef CONFIG_16BIT_CPU
    /* if domain download */
    if (pCurSdo->numeric == CO_TRUE)  {
	pCurSdo->pActualDomData =
	    unpack_oddmemcpy(&tData[1], pCurSdo->pActualDomData,
		    size, &pCurSdo->halfWord);
    } else
#  endif /* CONFIG_16BIT_CPU */
    {
#  ifdef CONFIG_DOMAIN_INDICATION_SIZE
        /* size - overByteNum can turn negative */
        if ((size - pCurSdo->overByteNum) >= 0)
        {
            /* enough size to transfer all overBytes */
            CO_MEMCPY(&tData[1 + pCurSdo->overByteNum], pCurSdo->pActualDomData,
	    (size - pCurSdo->overByteNum));
            /* points to next valid segment */
            pCurSdo->pActualDomData += (size - pCurSdo->overByteNum);
            pCurSdo->actBufNum += (size - pCurSdo->overByteNum);
            pCurSdo->overByteNum = 0;
        }
        else
        {
            /* size < overbyteNum -> buffered more data than belongs to sdo */
            /* data already copied */
            /* adjust pointers */ /* only called for last segment -> no effect? */
            pCurSdo->pActualDomData += 0;                       /* no new data bytes */
            pCurSdo->actBufNum += 0;                            /* no new data bytes */
            pCurSdo->overByteNum -= (pCurSdo->overByteNum - size);
        }
#  else /* CONFIG_DOMAIN_INDICATION_SIZE */
	CO_MEMCPY(&tData[1], pCurSdo->pActualDomData, size);

	/* points to next valid segment */
	pCurSdo->pActualDomData += size;
#  endif /* CONFIG_DOMAIN_INDICATION_SIZE */
    }
    pCurSdo->restSize -= size;

#  ifdef CONFIG_REDUNDANCY_SUPPORT
    GL_VAR(co_redcySdoLine) = pCurSdo->commLine;
#  endif /* CONFIG_REDUNDANCY_SUPPORT */
    (void)TRANSMIT_COB(pCurSdo->pTrCOB, tData);
}


/*******************************************************************
*
* dnLdSeg_res - performs the response to the service download domain segment
*
* Mit diesem Funktions\%aufruf teilt der Client dem Server
* Erfolg oder Mißerfolg des Dienstes mit.
*
* Im Byte \fIbRemRes\fP wird das Ergebis als
* \&RES_SUCC oder RES_FAIL
* spezifiziert.
* Wird RES_FAIL angegeben
* sind die Fehlercodes
* .TP
* \&E_APPLICATION_REQ
* Anwenderanforderung
* .TP
* \&E_NO_RESSOURCES
* kein Speicher für die Daten verfügbar
*
* Der Fehlercode ist Anwenderspezifiziert für \fIbErrReason\fP = 1
* und Imlementierungsspezifisch für \fIbErrReason\fP >= 128.
*
* INTERNAL
* input: remote result, error reason
*/
void dnLdSeg_res(
	SDO_T	*pCurSdo	/* pointer to current sdo */
	CO_COMMA_GLOBVARS_PARA_DECL
    )
{
UNSIGNED8	pData[8];		/* transmit data buffer */

    pData[0] = SCS_DN_LD_SEG_RES | pCurSdo->toggleBit;
    memset(&pData[1], (int)0, (size_t)7);
# ifdef CONFIG_REDUNDANCY_SUPPORT
    GL_VAR(co_redcySdoLine) = pCurSdo->commLine;
# endif /* CONFIG_REDUNDANCY_SUPPORT */
    (void)TRANSMIT_COB(pCurSdo->pTrCOB, pData);
}
# endif /* CONFIG_SEG_SDO */


/*******************************************************************
*
* initUpLd_res - response to the service initiate upload domain
*
* NOMANUAL
*
* This function generates an answer for init upload domain request
* to to sdo client.
*
* \retval
*	nothing
*
*/
void initUpLd_res(
	SDO_T	*pCurSdo	/* pointer to current sdo */
	CO_COMMA_LINE_PARA_DECL
    )
{
UNSIGNED8	pData[8];	/* transmit buffer */
UNSIGNED32	dSize;		/* Domain size */

    dSize = pCurSdo->domSize;

    CMS_SdoEncode(pCurSdo->index, pCurSdo->subIndex, pData);

    if ((dSize != 0u) && (dSize < 5u))  { /* expedited transfer */
	pData[0] = (UNSIGNED8) (
                    SCS_INI_UP_LD_RES
                    | ((4u - dSize) << 2u)
# ifndef CO_CONFIG_SDO_EXPEDITED_NO_VALID_SIZE_BIT
                    | CO_SIZE_VALID
# endif /* CO_CONFIG_SDO_EXPEDITED_NO_VALID_SIZE_BIT */
                    | EXPED_TRANSFER
                    );

# ifdef CO_CONFIG_SDO_EXPEDITED_NO_VALID_SIZE_BIT
    if (pCurSdo->expedited_sdo_with_valid_size_bit == CO_TRUE)
    {
        pData[0] |= CO_SIZE_VALID;
    }
# endif /* CO_CONFIG_SDO_EXPEDITED_NO_VALID_SIZE_BIT */

	/* erase unused data */
	memset(&pData[4], (int)0, (size_t)4);
	/* both data fields are char fields, memcpy is possible */
	CO_MEMCPY(&pData[4], pCurSdo->pActualDomData, dSize);
# ifdef CO_CONFIG_ENABLE_OBJ_CALLBACK
#  ifdef CO_CONFIG_OBJ_CB_POST_SDO_READ
        {
            CO_OBJ_CB_T callback = NULL;
            /* check if the object has an function pointer*/
            callback = getObjFuncPtr(pCurSdo->index CO_COMMA_LINE_PARA);
            if ( callback != NULL ) {
                CO_OBJ_CB_TYPE_T callReason;
                callReason.reason = CO_OBJ_CB_TYPE_POST_SDO_READ;
                callReason.serviceNbr = pCurSdo->num;
#   ifdef CO_CONFIG_ENABLE_EXTOBJ_CALLBACK
                callReason.objAccess.pData = pCurSdo->pActualDomData;
                callReason.objAccess.dataSize = dSize;
#   endif /* CO_CONFIG_ENABLE_EXTOBJ_CALLBACK */
                /* call the function pointer */
#   ifdef CONFIG_NO_GLOBAL_VARS
                (void)(*callback)( pCurSdo->index, pCurSdo->subIndex, callReason, (void*)CO_LINE_PARA );
#   else /* CONFIG_NO_GLOBAL_VARS */
                (void)(*callback)( pCurSdo->index, pCurSdo->subIndex, callReason CO_COMMA_LINE_PARA );
#   endif /* CONFIG_NO_GLOBAL_VARS */
            }
        }
#  endif /* CO_CONFIG_OBJ_CB_POST_SDO_READ */
# endif /* CO_CONFIG_ENABLE_OBJ_CALLBACK */

    } else { /* segmented transfer */
# ifdef CONFIG_SEG_SDO
	pData[0] = SCS_INI_UP_LD_RES | CO_SIZE_VALID;
	CO_UNPACK_MEMCPY(&pData[4], (UNSIGNED8 *)&dSize, 4, CO_NUM_VAL);
	pCurSdo->state = SDOSTATE_UPLD_INIT;
# else /* CONFIG_SEG_SDO */
	(void)abortSdoTransf_Req(pCurSdo, CO_E_SDO_OTHER CO_COMMA_LINE_PARA);
	return;
# endif /* CONFIG_SEG_SDO */
    }

# ifdef CONFIG_REDUNDANCY_SUPPORT
    GL_VAR(co_redcySdoLine) = pCurSdo->commLine;
# endif /* CONFIG_REDUNDANCY_SUPPORT */
    (void)TRANSMIT_COB(pCurSdo->pTrCOB, pData);
}


/*******************************************************************
*
* initDnLd_res - response to the service initiate download domain
*
* NOMANUAL
*
* This function generates an answer for the init download domain
* request to the sdo server.
*
* \retval
*	nothing
*
*/

void initDnLd_res(
	SDO_T	*pCurSdo	/* pointer to current sdo */
	CO_COMMA_GLOBVARS_PARA_DECL
    )
{
UNSIGNED8	pData[8];	/* transmit buffer */

    CMS_SdoEncode(pCurSdo->index, pCurSdo->subIndex, pData);
    memset(&pData[4], (int)0, (size_t)4);
    pData[0] = SCS_INI_DN_LD_RES;

# ifdef CONFIG_REDUNDANCY_SUPPORT
    GL_VAR(co_redcySdoLine) = pCurSdo->commLine;
# endif /* CONFIG_REDUNDANCY_SUPPORT */
    (void)TRANSMIT_COB(pCurSdo->pTrCOB, pData);
}


/*******************************************************************
*
* writeSdoValue - write the value of a SDO to the object dictionary
*
* NOMANUAL
*
* This function writes the value of a SDO
* to the object dictionary and tests it before .
* If an error occurs it calls abortSdoTransf_Req(),
* which initiate the abort_domain_transfer,
* otherwise the initDnLd_res()
* or dnLdSeg_res() service
*
* RETURNS
* .TP
* nothing
*
*/

static void writeSdoValue(
	SDO_T	    *pCurSdo,	/* pointer to SDO */
	UNSIGNED8   *pData	/* pointer to SDO data */
	CO_COMMA_LINE_PARA_DECL	/**< number of CAN line 0..CO_MAX_CAN_LINES-1 */
    )
{
RET_T 		commonRet = CO_OK;     /* common return value */
UNSIGNED16 	index;         /* index */
UNSIGNED8	subIndex;      /* subindex */

    /* for faster access */
    index = pCurSdo->index;
    subIndex = pCurSdo->subIndex;

    if (pCurSdo->domSize <= MAX_SDO_SAVE_LEN)  {
	commonRet = putObj(index, subIndex, pData, pCurSdo->domSize, CO_FALSE
		CO_COMMA_LINE_PARA);
    }

    /* set communication parameter */
    if ((commonRet == CO_OK) && (index <= END_COM_PROF)) {

# ifdef CONFIG_NON_VOLATILE_MEM
	if (index == STORE_PARA_INDEX) {
	    /* check signature */
	    if (subIndex > 0)  {
		if (*((UNSIGNED32 *)pCurSdo->pDomData) == SAVE_SIGNATURE) {
		    if (saveParameterInd(subIndex CO_COMMA_LINE_PARA)
				== CO_FALSE) {
			commonRet = CO_E_HARDWARE_FAULT;
		    }
		}
#  ifdef OLD_NON_VOLATILE_MEMORY
		else if (*((UNSIGNED32 *)pCurSdo->pDomData) == CLEAR_SIGNATURE){
		    if (clearParameterInd(subIndex CO_COMMA_LINE_PARA)
				== CO_FALSE) {
			commonRet = CO_E_HARDWARE_FAULT;
		    }
		}
#  endif /* OLD_NON_VOLATILE_MEMORY */
		else {
		    commonRet = CO_E_INVALID_TRANSMODE;
		}
	    } else {
		commonRet = CO_E_INVALID_TRANSMODE;
	    }
	}
	else if (index == RESTORE_DEF_PARA_INDEX) {
	    if (subIndex > 0)  {
		if (*((UNSIGNED32 *)pCurSdo->pDomData) == LOAD_SIGNATURE) {
		    /* set default values */
		    /* call user indication */
#  ifdef OLD_NON_VOLATILE_MEMORY
		    if (loadParameterInd(subIndex, CO_RESTORE_MODE_SDO
			    CO_COMMA_LINE_PARA) == CO_FALSE) {
#  else /* OLD_NON_VOLATILE_MEMORY */
		    if (clearParameterInd(subIndex CO_COMMA_LINE_PARA)
				== CO_FALSE) {
#  endif /* OLD_NON_VOLATILE_MEMORY */
			commonRet = CO_E_HARDWARE_FAULT;
		    }
		} else  {
		    commonRet = CO_E_INVALID_TRANSMODE;
		}
	    } else {
		commonRet = CO_E_INVALID_TRANSMODE;
	    }
	}
	else /* neither STORE_PARA_INDEX nor STORE_PARA_INDEX */
	{
# endif /* CONFIG_NON_VOLATILE_MEM */
	    commonRet = setCommPar(index, subIndex CO_COMMA_LINE_PARA);
# ifdef CONFIG_NON_VOLATILE_MEM
	}
# endif /* CONFIG_NON_VOLATILE_MEM */
    }

# ifdef CONFIG_NON_VOLATILE_MEM
    /* remove signature from save/restore objects */
    if ((index == STORE_PARA_INDEX) || (index == RESTORE_DEF_PARA_INDEX))  {
	putObj(index, subIndex, &pCurSdo->oldVar[0], pCurSdo->domSize, CO_TRUE
		CO_COMMA_LINE_PARA);
    }
# endif /* CONFIG_NON_VOLATILE_MEM */

    /* control the behaviour for writing on a object */

    if (commonRet == CO_OK) {
# ifdef CONFIG_SPLIT_INDICATION
        commonRet = sdoWrInd(index, subIndex, pCurSdo->num CO_COMMA_LINE_PARA);
# else /* CONFIG_SPLIT_INDICATION */
        commonRet = sdoWrInd(index, subIndex CO_COMMA_LINE_PARA);
# endif /* CONFIG_SPLIT_INDICATION */
    }

# ifdef CO_CONFIG_ENABLE_OBJ_CALLBACK
#  ifdef CO_CONFIG_OBJ_CB_POST_SDO_WRITE
    {
        CO_OBJ_CB_T callback = NULL;
        /* check if the object has an function pointer*/
        callback = getObjFuncPtr(pCurSdo->index CO_COMMA_LINE_PARA);
        if ( callback != NULL ) {
            CO_OBJ_CB_TYPE_T callReason;
			callReason.serviceNbr = pCurSdo->num;
			if (commonRet == CO_OK) {
				callReason.reason = CO_OBJ_CB_TYPE_POST_SDO_WRITE;
#   ifdef CO_CONFIG_ENABLE_EXTOBJ_CALLBACK
				(void) getObjAddr(pCurSdo->index, pCurSdo->subIndex, &callReason.objAccess.pData,
					&callReason.objAccess.dataSize CO_COMMA_LINE_PARA);
#   endif /* CO_CONFIG_ENABLE_EXTOBJ_CALLBACK */
				/* call the function pointer */
#   ifdef CONFIG_NO_GLOBAL_VARS
				commonRet = (*callback)(pCurSdo->index, pCurSdo->subIndex, callReason, (void*)CO_LINE_PARA);
#   else /* CONFIG_NO_GLOBAL_VARS */
				commonRet = (*callback)(pCurSdo->index, pCurSdo->subIndex, callReason CO_COMMA_LINE_PARA);
#   endif /* CONFIG_NO_GLOBAL_VARS */
			} else {
				/* in case of an error the user should not modify the commonRet */
				callReason.reason = CO_OBJ_CB_TYPE_POST_SDO_WRITE_ABORT;
				/* call the function pointer */
#   ifdef CONFIG_NO_GLOBAL_VARS
				(void)(*callback)(pCurSdo->index, pCurSdo->subIndex, callReason, (void*)CO_LINE_PARA);
#   else /* CONFIG_NO_GLOBAL_VARS */
				(void)(*callback)(pCurSdo->index, pCurSdo->subIndex, callReason CO_COMMA_LINE_PARA);
#   endif /* CONFIG_NO_GLOBAL_VARS */
			}
        }
    }
#  endif /* CO_CONFIG_OBJ_CB_POST_SDO_WRITE */
# endif /* CO_CONFIG_ENABLE_OBJ_CALLBACK */

    if (commonRet != CO_OK) {
	if (commonRet == CO_SDO_IND_BUSY) {
	    /* now we wait for finish the indication */
	    /* therefore the user has to call the function finishSdoWrInd(*/
	    pCurSdo->state |= SDOSTATE_IND_BUSY;
	    pCurSdo->state &= (UNSIGNED8)~SDOSTATE_READY;
	    return;
	}

        /* change SDO abort code for conformance test */
        if (((index >= RPDO_MAP_BASE_INDEX)&&(index <= RPDO_MAP_LAST_INDEX))
            ||((index >= TPDO_MAP_BASE_INDEX)&&(index <= TPDO_MAP_LAST_INDEX)))
        {
            if ((subIndex == 0)&&((commonRet==CO_E_VALUE_TO_HIGH)||(commonRet==CO_E_VALUE_TO_LOW)))
            {
                commonRet = CO_E_MAP;
            }
        }

	/* if command failed, the data would be restored */
	if (pCurSdo->saved == CO_TRUE)  {
	    CO_NUM_MEMCPY(pCurSdo->pDomData, &pCurSdo->oldVar, pCurSdo->domSize,
		    pCurSdo->attr & CO_NUM_VAL);

	    /* set communication parameter back to old values */
	    if (index <= END_COM_PROF) {
		(void)setCommPar(index, subIndex CO_COMMA_LINE_PARA);
	    }
	}
	(void)abortSdoTransf_Req(pCurSdo, commonRet CO_COMMA_LINE_PARA);
	return;
    }

# ifdef CONFIG_SPLIT_INDICATION
    writeSdoValue_finished(pCurSdo CO_COMMA_LINE_PARA);
}


/*******************************************************************
*
* writeSdoValue_finished - finish write the value of a SDO to the od dictionary
*
* NOMANUAL
*
* This function finished the writeSdoValue function
* This is only a stand alone function,
* if the CONFIG_SPLIT_INDICATION is set !!
*
*
*/

void writeSdoValue_finished(
	SDO_T	*pCurSdo	/* pointer to SDO */
	CO_COMMA_LINE_PARA_DECL	/* number of CAN line 0..CO_MAX_CAN_LINES-1 */
    )
{
#  ifdef CONFIG_DYN_SDO_CONNECTION_MANAGER
RET_T 		commonRet;	/* common return value */
#  endif /* CONFIG_DYN_SDO_CONNECTION_MANAGER */

# endif /* CONFIG_SPLIT_INDICATION */


# ifdef CONFIG_DYN_SDO_CONNECTION_MANAGER
    if (pCurSdo->index == SRD_REQUEST_SDO_INDEX)  {
	commonRet = srdRequest(CO_LINE_PARA);
	if (commonRet != CO_OK) {
	    (void)abortSdoTransf_Req(pCurSdo, commonRet CO_COMMA_LINE_PARA);
	    return;
	}
    }

    if (pCurSdo->index == SRD_RELEASE_SDO_INDEX)  {
	commonRet = srdRelease(CO_LINE_PARA);
	if (commonRet != CO_OK) {
	    (void)abortSdoTransf_Req(pCurSdo, commonRet CO_COMMA_LINE_PARA);
	    return;
	}
    }
# endif /* CONFIG_DYN_SDO_CONNECTION_MANAGER */

# ifdef CONFIG_SEG_SDO
    if (pCurSdo->state == SDOSTATE_DNLD_SEG) {
	dnLdSeg_res(pCurSdo CO_COMMA_GLOBVARS_PARA);
    } else {
	initDnLd_res(pCurSdo CO_COMMA_GLOBVARS_PARA);
    }
# else /* CONFIG_SEG_SDO */
    initDnLd_res(pCurSdo CO_COMMA_GLOBVARS_PARA);
# endif /* CONFIG_SEG_SDO */

    pCurSdo->state = SDOSTATE_READY;
}


#  ifdef CONFIG_DOMAIN_INDICATION_SIZE
/*******************************************************************
*
* pcoUpLdBufUpdate - update buffer for domain upload
*
* NOMANUAL
*
* This function updates the buffer of the size CONFIG_DOMAIN_INDICATION_SIZE
* with new data for domain upload.
*
* RETURNS
* .TP
* RET_T: CANopen return value
*
*/
RET_T pcoUpLdBufUpdate(
	SDO_T	*pCurSdo	/* pointer to current sdo */
	CO_COMMA_LINE_PARA_DECL	/* number of CAN line 0..CO_MAX_CAN_LINES-1 */
     )
{
RET_T commonRet;	/* common return value */
UNSIGNED32 restBufNum;  /* actual size of the domain */
UNSIGNED32 size;        /* number of actual loaded bytes in the domain buffer */

    commonRet = CO_OK;

    /* calculate number of available bytes in the buffer for sending */
    restBufNum = pCurSdo->maxBufNum - pCurSdo->actBufNum;

    /* update buffer if there are not enough bytes in the buffer for the next
     * CAN message and if there are further bytes available in the memory */
    if ((restBufNum < 7) && ((pCurSdo->domSize - pCurSdo->domSizeBuffered) > 0))
    {
        /* save surplus bytes */
        if (restBufNum != 0)
        {
            CO_MEMCPY(pCurSdo->overByteBuf, pCurSdo->pActualDomData, restBufNum);
            pCurSdo->overByteNum = (UNSIGNED8)restBufNum;
        }

        /* update domain buffer by application */
        commonRet = coUserSdoDomainUploadInd(pCurSdo->index, pCurSdo->subIndex,
            pCurSdo->pDomData, &size CO_COMMA_LINE_PARA);
        if (commonRet != CO_OK)
        {
            (void)abortSdoTransf_Req(pCurSdo, commonRet CO_COMMA_LINE_PARA);
            return (commonRet);
        }

        if (size > CONFIG_DOMAIN_INDICATION_SIZE)
        {
            /* SDO abort is generated and the domain transfer is interrupted */
            commonRet = CO_E_MEM;
            (void)abortSdoTransf_Req(pCurSdo, commonRet CO_COMMA_LINE_PARA);
            return (commonRet);
        }
        if (size > pCurSdo->domSize)
        {
            /* SDO abort is generated and the domain transfer is interrupted */
            commonRet = CO_E_MEM;
            (void)abortSdoTransf_Req(pCurSdo, commonRet CO_COMMA_LINE_PARA);
            return (commonRet);
        }

        /* release new filled domain buffer */
        pCurSdo->maxBufNum = size;
        pCurSdo->actBufNum = 0;
        pCurSdo->domSizeBuffered += size;

        /* reset pointer to domain buffer */
        pCurSdo->pActualDomData = pCurSdo->pDomData;
    }

    return (commonRet);
}
#  endif /* CONFIG_DOMAIN_INDICATION_SIZE */

#endif /* CONFIG_SDO_SERVER */

/*______________________________________________________________________EOF_*/
