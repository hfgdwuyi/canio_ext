/*
 * sdoblock - CANopen's Sdo block transfer routines
 *
 * Copyright (c) 2000-2017 port GmbH Halle (Saale)
 *------------------------------------------------------------------
 */


/*
*  \file sdoblock.c
*  \author port GmbH Halle (Saale)
*
*++ This modul contains all CANopen's indication and confirmation functions for
*++ sdo block transfer.
*-- Dieses Modul enthält Funktionen, um den SDO-Blocktransfer zu nutzen.
*
*++ All of the functions are only called from within the library
*++ and not from the library user.
*++ Therefore there are no manual entries of the functions available.
*-- Alle hier enthaltenen Funktionen werden nur innerhalb der Library
*-- aufgerufen.
*-- Daher sind keine Funktionsbeschreibungen verfügbar.
*
*/

/* header of standard C - libraries */

#include <stdio.h>
#include <string.h>

/* header of project specific types */

#include <cal_conf.h>
#include <co_flag.h>
#include <co_mcpy.h>
#include <co_debug.h>
#include <co_drv.h>
#include "sdo.h"
#include "drv.h"
#include "cmscodec.h"
#include "access.h"
#include "sdoblock.h"

#ifdef CONFIG_16BIT_CPU
#include "utility.h"
#endif /*  CONFIG_16BIT_CPU */

#ifdef CONFIG_REDUNDANCY_SUPPORT
# include "reduncy.h"
#endif /* CONFIG_REDUNDANCY_SUPPORT */

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
void sdoBlockTrans(SDO_T *pCurSdo, SDO_CLIENT_T *
	CO_COMMA_LINE_PARA_DECL         /* canline */ );


/* external variables
---------------------------------------------------------------------------*/

/* global variables
---------------------------------------------------------------------------*/
#ifdef CONFIG_BLOCK_CRC
UNSIGNED8 test = 2;
#endif /* CONFIG_BLOCK_CRC */

/* local defined variables
---------------------------------------------------------------------------*/
#ifdef CONFIG_RCS_IDENT
CO_LIB_INIT_VAR static char _rcsid[] = "$Id: sdoblock.c,v 2.49 2016/10/06 16:44:04 rli Exp $";
#endif /* CONFIG_RCS_IDENT */

#ifdef CONFIG_SDO_BLOCKTRANSFER
/*******************************************************************
*
* sdoBlockTrans - Block transmission
*
* send block data
*
* NOMANUAL
*
*
* \retval
*	nothing
*
*/
void sdoBlockTrans(
    SDO_T         *pSdo,       /* pointer to current sdo */
    SDO_CLIENT_T  *pClientSdo  /* pointer to client sdo */
    CO_COMMA_LINE_PARA_DECL    /* canline */
    )
{
UNSIGNED32	size;			/* transmit size for actual sdo */
UNSIGNED8	tData[8];		/* transmit buffer */

    /* max datasize for one message */
    size = 7;

# ifdef CONFIG_REDUNDANCY_SUPPORT
    canLine = pSdo->commLine;
# endif /* CONFIG_REDUNDANCY_SUPPORT */

    /* send block */
    while (pSdo->blkSegNr <= pSdo->blkSegSize)  {

        /* is transmit buffer empty ? */
        if (checkTxBuffer(CO_REDCY_PARA) == CO_FALSE)  {
            /* no, break and call it again in flagIdentification () */
            SET_COLIB_FLAG(COFLAG_SDO_BLOCKTRANS);
            break;
        }

# ifdef CO_CONFIG_BLOCKTRANSFER_INHIBITED_SEND
        /* check if inhibit timer is active */
        if (pSdo->inhibit.ticks > 0u)
        {
            /* yes, break and call this function again in in flagIdentification() */
            SET_COLIB_FLAG(COFLAG_SDO_BLOCKTRANS);
            break;
        }
        else
        {
            /* no, restart it */
            startInhibitTimer(&(pSdo->inhibit), pSdo->inhibitTime CO_COMMA_LINE_PARA);
        }
# endif /* CO_CONFIG_BLOCKTRANSFER_INHIBITED_SEND */

# ifdef CONFIG_DOMAIN_INDICATION_SIZE
        if ((pSdo->attr & CO_UP_DN_LD_DOMAIN) != 0) {
            /* update intermediate domain buffer if necessary */
            (void)pcoUpLdBufUpdate(pSdo CO_COMMA_LINE_PARA);
        }
# endif /* CONFIG_DOMAIN_INDICATION_SIZE */

        /* set block number */
        tData[0] = pSdo->blkSegNr;

#  ifdef CO_CONFIG_DOMAIN_UNKNOWN_SIZE
        /* ask application for restsize */
        if ((pSdo->attr & CO_UP_DN_LD_DOMAIN_SIZELESS) != 0)
        {
            if (pSdo->restSize <= 7)
            {
                /* printf("sdoBlockTrans: ask for size\n"); */
                size = coUserSdoDomainSizeInd(pSdo->num CO_COMMA_LINE_PARA);
                pSdo->restSize = size;
            }
            else
            {
                /* printf("sdoBlockTrans: size bigger than 1 message: %lu\n",pSdo->restSize); */
                size = pSdo->restSize;
            }
        }
        else
#  endif /* CO_CONFIG_DOMAIN_UNKNOWN_SIZE */
        {
            /* printf("SETTING SIZE = RESTSIZE\n"); */
            size = pSdo->restSize;
        }
        /* printf("sdoTransfBlk size %lu restSize %lu\n",size,pSdo->restSize); */

        if (pSdo->restSize <= 7)  {
            /* last block */
            tData[0] |= CO_SDOBLOCK_NO_MORE_BLKS;
        }

        if (size > 7)  {
            size = 7;
        }

        if (size == 0)
        {
            /* all data was already transmitted -> do send empty block or the sdo hangs*/

            /* force end of transmission */
            pSdo->restSize = 0;
            pSdo->lastSegSize = 7;                /* last segment must have been full of data */
            tData[0] |= CO_SDOBLOCK_NO_MORE_BLKS;
        }

        if (size < 7)
        {
            /* this is the last segment of data */
            tData[0] |= CO_SDOBLOCK_NO_MORE_BLKS;
            /* force end of transmission */
            pSdo->restSize = size;
        }

        pSdo->lastSegSize = size;
        /* printf("sdoTransfBlk size %lu restSize %lu\n",size,pSdo->restSize); */

        /* copy data to transmit buffer */
# ifdef CONFIG_16BIT_CPU
        if (pSdo->numeric == CO_TRUE)  {
            pSdo->pActualDomData =
                unpack_oddmemcpy(&tData[1], pSdo->pActualDomData,
                                size, &pSdo->halfWord);
        } else
# endif /* CONFIG_16BIT_CPU */
        {
# ifdef CONFIG_DOMAIN_INDICATION_SIZE
            if (pSdo->overByteNum != 0) {
                CO_MEMCPY(&tData[1], pSdo->overByteBuf, pSdo->overByteNum);
            }
            /* size - overByteNum can turn negative */
            if (size >= pSdo->overByteNum)
            {
                /* enough size to transfer all overBytes */
                CO_MEMCPY(&tData[1 + pSdo->overByteNum], pSdo->pActualDomData,
                            (size - pSdo->overByteNum));
                /* points to next valid block segment */
                pSdo->pActualDomData += (size - pSdo->overByteNum);
                pSdo->actBufNum += (size - pSdo->overByteNum);
                pSdo->overByteNum = 0;
            }
            else
            {
                /* size < overbyteNum -> buffered more data than belongs to sdo */
                /* data already copied */
                /* adjust pointers */ /* only called for last segment -> no effect? */
                pSdo->pActualDomData += 0;                       /* no new data bytes */
                pSdo->actBufNum += 0;                            /* no new data bytes */
                pSdo->overByteNum -= (pSdo->overByteNum - size);
            }
# else /* CONFIG_DOMAIN_INDICATION_SIZE */
            CO_MEMCPY(&tData[1], pSdo->pActualDomData, size);
            pSdo->pActualDomData += size;
# endif /* CONFIG_DOMAIN_INDICATION_SIZE */
        }

# ifdef CONFIG_BLOCK_CRC
        pSdo->blkCrcSum = crc16Calc(&tData[1], pSdo->blkCrcSum, size
#  ifdef CONFIG_16BIT_CPU
                                    , CO_FALSE
#  endif /* CONFIG_16BIT_CPU */
                                    );
        /* printf("calculated CRC %x\n",pSdo->blkCrcSum); */
# endif /* CONFIG_BLOCK_CRC */

# ifdef CONFIG_REDUNDANCY_SUPPORT
        GL_VAR(co_redcySdoLine) = pSdo->commLine;
# endif /* CONFIG_REDUNDANCY_SUPPORT */
        (void) TRANSMIT_COB(pSdo->pTrCOB, tData);

        /* set new rest size */
        pSdo->restSize -= (UNSIGNED8)size;
        pSdo->blkSegNr++;

        /* printf("sdoTransfBlk restSize %lu blkSegNr %lu\n", pSdo->restSize, pSdo->blkSegNr); */

        /* all data was transmitted */
        if (pSdo->restSize == 0)  {
            /* return; */
            break;
        }

# ifdef CONFIG_SDO_CLIENT
#  ifdef CONFIG_DOMAIN_CONFIRMATION
        if (pClientSdo != NULL)  {
            if (pClientSdo->domainIndSize != 0)  {
                /* next border reached ? */
                if (pSdo->restSize == pClientSdo->nextDomainIndBorder)  {
                    /* call indication function */
                    if (sdoDomainWrCon(pSdo->num CO_COMMA_LINE_PARA) != CO_OK)  {
                        /* abort transfer */
                        abortSdoTransf_Req(pSdo, CO_E_HARDWARE_FAULT);
                        break;
                    }
                    /* reset pointer */
                    pSdo->pActualDomData = pClientSdo->pBufferStart;
                    /* calculate next border */
                    if (pClientSdo->domainIndSize < pSdo->restSize)  {
                    pClientSdo->nextDomainIndBorder =
                        pSdo->restSize - pClientSdo->domainIndSize;
                    }
                }
            }
        }
#  endif /* CONFIG_DOMAIN_CONFIRMATION */
# endif /* CONFIG_SDO_CLIENT */
    }

# ifdef CONFIG_SDO_CLIENT
    /* are all messages transmitted, wait for answer */
    if (pClientSdo != NULL) {
        if ((pSdo->blkSegNr > pSdo->blkSegSize)
             || (pSdo->restSize == 0))  {
            /* start timeout again */
            (void) addTimerEvent(&pClientSdo->timer, pClientSdo->timeOut,
                            CO_TIMER_TYPE_SDO CO_COMMA_LINE_PARA);
        }
    }
# else /* CONFIG_SDO_CLIENT */
    /* to avoid compiler warnings */
    pClientSdo = pClientSdo;
# endif /* CONFIG_SDO_CLIENT */
}


/*******************************************************************
*
* sdoContBlockTrans - continue transmit Block Download
*
* NOMANUAL
*
* This function continues transmit sdo segments for block transfer,
* if the transfer was interrupted because the transmit buffer
* was not empty.
* This function is called from flagIdentification() from file utility.c
*
* \retval
*	nothing
*
*/
void sdoContBlockTrans(
	CO_LINE_PARA_DECL	/* number of CAN line 0..CO_MAX_CAN_LINES-1 */
    )
{
UNSIGNED8	nr;

# ifdef CONFIG_SDO_SERVER
    /* for all server sdos */
    for (nr = 0; nr < (UNSIGNED8)GL_ARRAY(co_sdoServerCnt); nr++)  {
        if ((GL_PVAR(co_sdoServer) [nr
# ifdef CONFIG_MULT_LINES
                + GL_ARRAY(co_sdoServerLineOffs)
# endif /* CONFIG_MULT_LINES */
                ].state == SDOSTATE_DNLD_BLK_SEG)
                || (GL_PVAR(co_sdoServer) [nr
# ifdef CONFIG_MULT_LINES
                + GL_ARRAY(co_sdoServerLineOffs)
# endif /* CONFIG_MULT_LINES */
                ].state == SDOSTATE_UPLD_BLK_SEG)) {
            /* send the next block */
            sdoBlockTrans(&GL_PVAR(co_sdoServer) [nr
# ifdef CONFIG_MULT_LINES
                            + GL_ARRAY(co_sdoServerLineOffs)
# endif /* CONFIG_MULT_LINES */
                            ], NULL CO_COMMA_LINE_PARA);
        }
    }
# endif /* CONFIG_SDO_SERVER */

# ifdef CONFIG_SDO_CLIENT
    /* for all client sdos */
    for (nr = 0; nr < (UNSIGNED8)GL_ARRAY(co_sdoClientCnt); nr++)  {
        if ((GL_PVAR(co_sdoClient) [nr
# ifdef CONFIG_MULT_LINES
                + GL_ARRAY(co_sdoClientLineOffs)
# endif /* CONFIG_MULT_LINES */
                ].sdo.state == SDOSTATE_DNLD_BLK_SEG)
                || (GL_PVAR(co_sdoClient) [nr
# ifdef CONFIG_MULT_LINES
                + GL_ARRAY(co_sdoClientLineOffs)
# endif /* CONFIG_MULT_LINES */
                ].sdo.state == SDOSTATE_UPLD_BLK_SEG)) {
            /* send the next block */
            sdoBlockTrans(&GL_PVAR(co_sdoClient) [nr
# ifdef CONFIG_MULT_LINES
                        + GL_ARRAY(co_sdoClientLineOffs)
# endif /* CONFIG_MULT_LINES */
                        ].sdo,
                        &GL_PVAR(co_sdoClient) [nr
# ifdef CONFIG_MULT_LINES
                        + GL_ARRAY(co_sdoClientLineOffs)
# endif /* CONFIG_MULT_LINES */
                        ] CO_COMMA_LINE_PARA);
        }
    }
# endif /* CONFIG_SDO_CLIENT */
}
#endif /* defined(CONFIG_SDO_BLOCKTRANSFER) */


#if defined(CONFIG_SDO_SERVER) && defined(CONFIG_SDO_BLOCKTRANSFER)

/*******************************************************************
*
* initDnLdBlk_ind - Initiate Block Download Response
*
* NOMANUAL
*
* This function is indication and response of block download init on server side.
*
* \retval
*	nothing
*
*/
void initDnLdBlk_ind(
	SDO_T		*pCurSdo,	/* pointer to actual sdo */
	UNSIGNED8	*canBuf		/* Pointer to CAN buffer */
	CO_COMMA_LINE_PARA_DECL	/* number of CAN line 0..CO_MAX_CAN_LINES-1 */
    )
{
RET_T 		commonRet;	/* common return value */
UNSIGNED8	*pAddr;		/* pointer to data address */
UNSIGNED32      objSize;	/* size of data */
UNSIGNED16 	index;		/* index */
UNSIGNED8	subIndex;	/* subindex */
UNSIGNED32	sdoSize = 0;	/* SDO size */
UNSIGNED8	attr;		/* flag if object is a domain */
UNSIGNED8	tData[8];	/* transmit data */

#  ifdef CONFIG_REDUNDANCY_SUPPORT
    pCurSdo->commLine = co_redcyReceivedLine;/* communication line */
#  endif /* CONFIG_REDUNDANCY_SUPPORT */

    /* get index and subIndex */
    CMS_SdoDecode(pCurSdo->index, pCurSdo->subIndex, canBuf);

    /* for faster access */
    index = pCurSdo->index;
    subIndex = pCurSdo->subIndex;

    /* get target address */
    commonRet = getObjAddr(index, subIndex, &pAddr, &objSize
		 CO_COMMA_LINE_PARA);

    /* test target space size against data size and for valid address */
    if (commonRet == CO_OK) {
	attr = getObjAttr(index, subIndex CO_COMMA_LINE_PARA);

	/* write permission ok ? */
	if ((attr & CO_WRITE_PERM) == 0)  {
	    (void) abortSdoTransf_Req(pCurSdo, CO_E_NO_WRITE_PERM CO_COMMA_LINE_PARA);
	    return;
	}

        /* if size available */
        if ((*canBuf & CO_SDOBLOCK_SIZE_VALID) != 0)
        {
            /* copy the size from the can buffer */
            CO_PACK_MEMCPY((UNSIGNED8 *)&sdoSize, canBuf + 4, 4, CO_TRUE);
        }
        else
        {
# ifdef CONFIG_DOMAIN_UPDNLD
	    /* set the domain size as transfer size */
	    if ((attr & CO_UP_DN_LD_DOMAIN) != 0)  {
		sdoSize = getDomainSize(index, subIndex CO_COMMA_LINE_PARA);
	    } else
# endif /* CONFIG_DOMAIN_UPDNLD */
	    {
		sdoSize = objSize;
	    }
	}

# ifdef CONFIG_DOMAIN_UPDNLD
	/*  exception for domains
	    the default value entry of the type descripion
	    contains the true size value and
	    the object entry contains only a pointer to the domain
	 */
	if ((attr & CO_UP_DN_LD_DOMAIN) != 0) {
	    pAddr = getDomainAddr(index, subIndex CO_COMMA_LINE_PARA);
	    if (sdoSize > getDomainSize(index, subIndex CO_COMMA_LINE_PARA)
			|| pAddr == NULL) {
		commonRet = CO_E_WRONG_SIZE;
	    }

#  ifdef CONFIG_16BIT_CPU
	    /* set flag for domain download */
	    pCurSdo->numeric = CO_TRUE;
#  endif /* CONFIG_16BIT_CPU */

	} else
# endif /* CONFIG_DOMAIN_UPDNLD */
	{

# ifdef CONFIG_16BIT_CPU
	    /* set flag for domain download */
	    pCurSdo->numeric = CO_FALSE;
# endif /* CONFIG_16BIT_CPU */

	    if ((sdoSize > objSize) || (pAddr == NULL))  {
		commonRet = CO_E_WRONG_SIZE;
	    }
# ifdef CO_CONFIG_SDO_SHORT_STRINGS
            else
            {
                /* if object is a string, set actual length */
                if ((attr & CO_UP_DN_LD_STRING) != 0u)
                {
                    setStringSize(index, subIndex, sdoSize);
                }
            }
# endif /* CO_CONFIG_SDO_SHORT_STRINGS */
	}
    }

# ifdef CONFIG_VALUE_CHECK_FUNCTION
    if (commonRet == CO_OK) {
        /* user function for value test */
        commonRet = testSdoValue(index, subIndex, (void *)pAddr, sdoSize
                CO_COMMA_LINE_PARA);
    }
# endif /* CONFIG_VALUE_CHECK_FUNCTION */

    /*
      If size of indicated SDO to large, target address invalid
      or access error to Object Dictionary
      return with error (Abort Domain Transfer)
    */

    if (commonRet != CO_OK) {
	(void) abortSdoTransf_Req(pCurSdo, commonRet CO_COMMA_LINE_PARA);
	return;
    }

    /* set block size, init size information  */
    pCurSdo->blkSegSize = pCurSdo->blkSegDefaultSize;
    pCurSdo->domSize = sdoSize;
    pCurSdo->restSize = sdoSize;
# ifdef CONFIG_16BIT_CPU
    pCurSdo->halfWord = CO_FALSE;
# endif /* CONFIG_16BIT_CPU */
    pCurSdo->blkSegNr = 1;

    /* CRC supported ? */
# ifdef CONFIG_BLOCK_CRC
    pCurSdo->blkCRC = (*canBuf & CO_SDOBLOCK_USE_CRC) ? CO_TRUE : CO_FALSE;
    pCurSdo->blkCrcSum = 0;
    pCurSdo->blkCrcSumAtBlkStart = 0;
# else /* CONFIG_BLOCK_CRC */
    pCurSdo->blkCRC = CO_FALSE;
# endif /* CONFIG_BLOCK_CRC */

    /* set target address */
    pCurSdo->pDomData = pCurSdo->pActualDomData = pAddr;

    /* generate a response */
    CMS_SdoEncode(pCurSdo->index, pCurSdo->subIndex, tData);
    tData[0] = CO_SDOBLK_SCS_DOWN;

# ifdef CONFIG_BLOCK_CRC
    /* add crc informion */
    if (pCurSdo->blkCRC == CO_TRUE)  {
	tData[0] += CO_SDOBLOCK_USE_CRC;
    }
# endif /* CONFIG_BLOCK_CRC */

    /* set block size */
    tData[4] = pCurSdo->blkSegSize;
    /* delete unused bytes */
    memset(&tData[5], (int)0, (size_t)3);

# ifdef CONFIG_REDUNDANCY_SUPPORT
    GL_VAR(co_redcySdoLine) = pCurSdo->commLine;
# endif /* CONFIG_REDUNDANCY_SUPPORT */
    (void) TRANSMIT_COB(pCurSdo->pTrCOB, tData);

    /* set internal sdo state */
    pCurSdo->state = SDOSTATE_DNLD_BLK_SEG;
}


/*******************************************************************
*
* dnLdBlk_ind - Block Download Indication
*
* NOMANUAL
*
* This function responses a upload block request of a CANopen server.
*
* \retval
*	nothing
*
*/
void dnLdBlk_ind(
	SDO_T		*pSdo,		/* pointer to current sdo */
	UNSIGNED8	*canBuf		/* Pointer to CAN buffer */
	CO_COMMA_LINE_PARA_DECL
    )
{
UNSIGNED8	segNr;			/* segment number */
UNSIGNED32	size;			/* sdo size */
# ifdef CONFIG_DOMAIN_INDICATION_SIZE
UNSIGNED8	overSize;	/* bytes more than 128 */
UNSIGNED32	actCnt;		/* actual size */
UNSIGNED32	lb, b128;	/* next 128 byte limit */
UNSIGNED32	g7;		/* next 7 limit after 128 byte limit */
RET_T		commonRet;
# endif /* CONFIG_DOMAIN_INDICATION_SIZE */
# ifdef CONFIG_SDO_BLOCK_INDICATION
RET_T		retVal;
# endif /* CONFIG_SDO_BLOCK_INDICATION */
UNSIGNED8	tData[8];	/* transmit buffer */

# ifdef CONFIG_MULT_LINES
    (void)canLine;
# endif /* CONFIG_MULT_LINES */
    /* get segment number */
    segNr = *canBuf & ~CO_SDOBLOCK_CONT_FLAG;

    /* for testing only: creates transmission error */
    /* if (test > 0)
    {
        if (test > 1)
        {
            if (segNr == 9)    // ignore a segment with id 9
            {
                test--;
                return;
            }
        }

        if (test == 1)
        {
            if (segNr == 3)     // ignore a sgement with id 3
            {
                test--;
                return;
            }
        }
    } */

    /* test for valid block number */
    if (segNr == pSdo->blkSegNr)  {
	/* ok, copy data into buffer */
	size = pSdo->restSize;
	if (size > 7)  {
	    size = 7;
	}
	/* update size */
	pSdo->restSize -= size;

	/* if last block don't copy the data into buffer
	 * the size of valid data will be transfered with the next message
	 * we save the data in the pSdo later
	 */
	if ((*canBuf & CO_SDOBLOCK_CONT_FLAG) == 0)  {

# ifdef CONFIG_16BIT_CPU
	    /* if domain download */
	    if (pSdo->numeric == CO_TRUE)  {
		pSdo->pActualDomData =
		    pack_oddmemcpy(pSdo->pActualDomData, canBuf + 1,
			    size, &pSdo->halfWord);
	    } else

# endif /* CONFIG_16BIT_CPU */

	    {
		CO_MEMCPY(pSdo->pActualDomData, &canBuf[1], size);

		/* update address and size */
		pSdo->pActualDomData += size;
	    }

# ifdef CONFIG_BLOCK_CRC
            /* printf("previous calculated CRC %x\n",pSdo->blkCrcSum); */
	    pSdo->blkCrcSum = crc16Calc(&canBuf[1], pSdo->blkCrcSum, size
#  ifdef CONFIG_16BIT_CPU
		, CO_FALSE
#  endif /* CONFIG_16BIT_CPU */
		);
            /* printf("calculated CRC %x\n",pSdo->blkCrcSum); */
# endif /* CONFIG_BLOCK_CRC */

# ifdef CONFIG_DOMAIN_INDICATION_SIZE
	    /* set actual byte count */
	    actCnt = pSdo->domSize - pSdo->restSize;
	    /* look for next border limit */
	    b128 = NEXT_BORDER(actCnt);
	    /* get next 7 byte limit after border byte limit */
	    g7 = NEXT_7_BORDER(b128);

	    /* new border-7 byte limit reached ? */
	    if ((actCnt != 0) && (actCnt == g7))  {
		/* get last 128-7 limit */
		lb = NEXT_7_BORDER(b128 - CONFIG_DOMAIN_INDICATION_SIZE);

		overSize = actCnt % CONFIG_DOMAIN_INDICATION_SIZE;
		/* call indication function */
		commonRet = sdoDomainInd(pSdo->index, pSdo->subIndex,
		    pSdo->pDomData, g7 - lb - overSize, overSize
		    CO_COMMA_LINE_PARA);
		if (commonRet != CO_OK)  {
		    abortSdoTransf_Req(pSdo, commonRet);
		    return;
		}
		/* reset pointer */
		pSdo->pActualDomData = pSdo->pDomData;
	    }
# endif /* CONFIG_DOMAIN_INDICATION_SIZE */
	}

	/* prepare next segment */
	pSdo->blkSegNr++;
    }

    /* if block count reached or last block transmitted */
    if ((segNr >= pSdo->blkSegSize)
     || ((*canBuf & CO_SDOBLOCK_CONT_FLAG) != 0))  {

# ifdef CONFIG_SDO_BLOCK_INDICATION
	retVal = sdoBlockInd(pSdo->index, pSdo->subIndex,
		pSdo->domSize - pSdo->restSize CO_COMMA_LINE_PARA);
	if (retVal != CO_OK)  {
	    abortSdoTransf_Req(pSdo, retVal);
	    return;
	}
# endif /* CONFIG_SDO_BLOCK_INDICATION */

	/* send block response */
	tData[0] = CO_SDOBLK_SCS_DOWN | CO_SDOBLK_SS_BLK_DL;
	tData[1] = pSdo->blkSegNr - 1;
	tData[2] = pSdo->blkSegSize;
	memset(&tData[3], (int)0, (size_t)5);

# ifdef CONFIG_REDUNDANCY_SUPPORT
	GL_VAR(co_redcySdoLine) = pSdo->commLine;
# endif /* CONFIG_REDUNDANCY_SUPPORT */
	(void) TRANSMIT_COB(pSdo->pTrCOB, tData);

	/* reset block counter */
	if ((segNr + 1) == pSdo->blkSegNr)  {
	    pSdo->blkSegNr = 1;
	}
    }

    /* if last block transmitted finish block download */
    if ((*canBuf & CO_SDOBLOCK_CONT_FLAG) != 0)  {
	/* save the data from can buffer to pSdo */
	CO_MEMCPY(pSdo->pData, canBuf, 8);
	pSdo->state = SDOSTATE_DNLD_BLK_END;
    }
}


/*******************************************************************
*
* endDnLdBlk_ind - End Block Download Response
*
* NOMANUAL
*
* This function responses a end download block request of a CANopen server.
*
* \retval
*	nothing
*
*/
void endDnLdBlk_ind(
	SDO_T		*pSdo,		/* pointer to actual sdo */
	UNSIGNED8	*canBuf		/* Pointer to CAN buffer */
	CO_COMMA_LINE_PARA_DECL
    )
{
UNSIGNED8	len;			/* actual sdo length */
RET_T		retVal;			/* return value */
# ifdef CONFIG_DOMAIN_INDICATION_SIZE
UNSIGNED8	overSize;	/* bytes more than 128 */
UNSIGNED32	actCnt;		/* actual size */
UNSIGNED32	lb, b128;	/* next 128 byte limit */
UNSIGNED32	g7, lr, rest;	/* next 7 limit after 128 byte limit */
RET_T		commonRet;
# endif /* CONFIG_DOMAIN_INDICATION_SIZE */
# ifdef CONFIG_BLOCK_CRC
UNSIGNED16	blkCrcSum;
# endif /* CONFIG_BLOCK_CRC */

    /* copy the data from last segment */
    len = 7 - (((*canBuf) >> 2) & 0x7);

# ifdef CONFIG_16BIT_CPU
    /* if domain download */
    if (pSdo->numeric == CO_TRUE)  {
	pack_oddmemcpy(pSdo->pActualDomData, pSdo->pData + 1, len, &pSdo->halfWord);
    } else

# endif /* CONFIG_16BIT_CPU */
    {
	CO_MEMCPY(pSdo->pActualDomData, &pSdo->pData[1], len);
    }

    /* CRC supported ? */
# ifdef CONFIG_BLOCK_CRC
    if (pSdo->blkCRC == CO_TRUE)  {

        /* printf("previous calculated CRC %x\n",pSdo->blkCrcSum); */
	pSdo->blkCrcSum = crc16Calc(&pSdo->pData[1], pSdo->blkCrcSum, len
#  ifdef CONFIG_16BIT_CPU
	    , CO_FALSE
#  endif /* CONFIG_16BIT_CPU */
	    );

	/* check crc */
	CO_PACK_MEMCPY((UNSIGNED8*)&blkCrcSum, canBuf + 1, 2, CO_TRUE);
        /* printf("end: calculated CRC %x\nend: received CRC %x\n",pSdo->blkCrcSum,blkCrcSum); */
	if (blkCrcSum != pSdo->blkCrcSum)
        {
	    abortSdoTransf_Req(pSdo, CO_E_SDO_INVALID_BLKCRC);
	    return;
	}
    }

# endif /* CONFIG_BLOCK_CRC */

# ifdef CONFIG_DOMAIN_INDICATION_SIZE
	/* set actual byte count */
	actCnt = pSdo->domSize;

	/* look for next border limit */
	b128 = NEXT_BORDER(actCnt);
	/* get next 7 byte limit after border byte limit */
	g7 = NEXT_7_BORDER(b128);
	/* new border-7 byte limit reached ? */
	if (actCnt == g7)  {
	    /* get last 128-7 limit */
	    lb = NEXT_7_BORDER(b128 - CONFIG_DOMAIN_INDICATION_SIZE);

	    overSize = actCnt % CONFIG_DOMAIN_INDICATION_SIZE;
	    /* call indication function */
	    commonRet = sdoDomainInd(pSdo->index, pSdo->subIndex,
		    pSdo->pDomData, g7 - lb - overSize, overSize
		    CO_COMMA_LINE_PARA);
	    if (commonRet != CO_OK)  {
		abortSdoTransf_Req(pSdo, commonRet);
		return;
	    }
	} else {
	    /* get last g7 border */
	    if (actCnt < g7)  {
		/* calculate new value */
		b128 = NEXT_BORDER(b128 - CONFIG_DOMAIN_INDICATION_SIZE);
		g7 = NEXT_7_BORDER(b128);
	    }

	    /* not yet saved data */
	    rest = actCnt - g7;
	    /* rest from last save */
	    lr = g7 % CONFIG_DOMAIN_INDICATION_SIZE;

	    /* greater than max indication size, split it */
	    if ((lr + rest) > CONFIG_DOMAIN_INDICATION_SIZE)  {
		overSize = lr + rest - CONFIG_DOMAIN_INDICATION_SIZE;
		commonRet = sdoDomainInd(pSdo->index, pSdo->subIndex,
			pSdo->pDomData, rest - overSize, overSize
			CO_COMMA_LINE_PARA);
	    } else {
		commonRet = sdoDomainInd(pSdo->index, pSdo->subIndex,
			pSdo->pDomData, rest, 0 CO_COMMA_LINE_PARA);
	    }
	    if (commonRet != CO_OK)  {
		abortSdoTransf_Req(pSdo, commonRet);
		return;
	    }
	}
	commonRet = sdoDomainInd(pSdo->index, pSdo->subIndex,
	    pSdo->pDomData, 0, 0 CO_COMMA_LINE_PARA);
	if (commonRet != CO_OK)  {
	    abortSdoTransf_Req(pSdo, commonRet);
	    return;
	}
# endif /* CONFIG_DOMAIN_INDICATION_SIZE */

    /* informs application about new value */
# ifdef CONFIG_SPLIT_INDICATION
    retVal = sdoWrInd(pSdo->index, pSdo->subIndex, pSdo->num CO_COMMA_LINE_PARA);
# else /* CONFIG_SPLIT_INDICATION */
    retVal = sdoWrInd(pSdo->index, pSdo->subIndex CO_COMMA_LINE_PARA);
# endif /* CONFIG_SPLIT_INDICATION */
    if (retVal != CO_OK)  {
	(void) abortSdoTransf_Req(pSdo, retVal CO_COMMA_LINE_PARA);
	return;
    }

    /* sdoWrInd returned CO_OK */
    pSdo->pData[0] = CO_SDOBLK_SCS_DOWN | CO_SDOBLK_SS_END;
    memset(&pSdo->pData[1], (int)0, (size_t)7);

# ifdef CONFIG_REDUNDANCY_SUPPORT
    GL_VAR(co_redcySdoLine) = pSdo->commLine;
# endif /* CONFIG_REDUNDANCY_SUPPORT */
    (void) TRANSMIT_COB(pSdo->pTrCOB, pSdo->pData);

    pSdo->state = SDOSTATE_READY;
}


/*******************************************************************
*
* initUpLdBlk_ind - Initiate Block Upload Indication
*
* NOMANUAL
*
* This function responses a init upload block request in a CANopen server.
* In CiA-301, this function is initiate block upload indication AND response.
* It's also the confirmation of a block segment response ...
* ... the indication of the start block upload request ...
* ... and the confirmation of a block upload end response ...
*
* \retval
*	nothing
*
*/
void initUpLdBlk_ind(
    SDO_T      *pCurSdo,    /* pointer to actual sdo */
    UNSIGNED8  *canBuf      /* Pointer to CAN buffer */
    CO_COMMA_LINE_PARA_DECL /* number of CAN line 0..CO_MAX_CAN_LINES-1 */
    )
{
UNSIGNED8	segNr;			/* sdo segment number */
# ifdef CONFIG_BLOCK_CRC
UNSIGNED8       i;
# endif /* CONFIG_BLOCK_CRC */

    /* select client subcommand */
    if (((*canBuf) & CO_SDOBLK_CS_MASK) == CO_SDOBLK_CS_UP_START)  {
	/* upload start */
	sdoBlockTrans(pCurSdo, NULL CO_COMMA_LINE_PARA);
	pCurSdo->state = SDOSTATE_UPLD_BLK_SEG;
	return;

    } else if (((*canBuf) & CO_SDOBLK_CS_MASK) == CO_SDOBLK_CS_UP_RESP)  {
	/* segment confirmation */

	/* test was transmitting ok */
	segNr = canBuf[1];
	if (segNr != (pCurSdo->blkSegNr - 1))  {
	    /* transmission failed */
	    /* blocksize > 127 isn't allowed - abort for conformance test */
	    if (segNr > 127) {
		(void) abortSdoTransf_Req(pCurSdo, CO_E_SDO_INVALID_BLK_SEQ CO_COMMA_LINE_PARA);
	    }
	    pCurSdo->blkSegNr = segNr + 1;
	    pCurSdo->pActualDomData = pCurSdo->pDomData + segNr * 7;
	    pCurSdo->restSize = pCurSdo->domSize - segNr * 7;

#  ifdef CONFIG_BLOCK_CRC
            /* printf("recalculating CRC Sum\n"); */
            pCurSdo->blkCrcSum = pCurSdo->blkCrcSumAtBlkStart;
            /* printf("last stored CRC %x\n",pCurSdo->blkCrcSum); */
            for (i = 0; i < segNr; i++)
            {
                pCurSdo->blkCrcSum = crc16Calc(pCurSdo->pDomData + i * 7, pCurSdo->blkCrcSum, 7
                #  ifdef CONFIG_16BIT_CPU
                    , CO_FALSE
                #  endif /* CONFIG_16BIT_CPU */
                                           );
                 /* printf("recalculated CRC step %x %x\n", i, pCurSdo->blkCrcSum); */
            }
#  endif /* CONFIG_BLOCK_CRC */

	}
        else
        {
            /* transmission ok */
            /* move the pointer if not end of transmission */
            if (pCurSdo->restSize != 0)
            {
#  ifndef CONFIG_DOMAIN_INDICATION_SIZE
                pCurSdo->pDomData = pCurSdo->pActualDomData;
#  endif /* CONFIG_DOMAIN_INDICATION_SIZE */
                pCurSdo->domSize = pCurSdo->restSize;
            }
            else
            {
                /* all transmissions was ok, send end block */
                upLdBlkEnd_req(pCurSdo CO_COMMA_LINE_PARA);
                return;
            }
            pCurSdo->blkSegNr = 1;
# ifdef CONFIG_BLOCK_CRC
            pCurSdo->blkCrcSumAtBlkStart = pCurSdo->blkCrcSum;
# endif /* CONFIG_BLOCK_CRC */

            /* set new block size */
            pCurSdo->blkSegSize = canBuf[2];
        }
        sdoBlockTrans(pCurSdo, NULL CO_COMMA_LINE_PARA);

    } else if ((*canBuf & CO_SDOBLK_CS_MASK) == CO_SDOBLK_CS_UP_END)  {
	/* upload end */
	pCurSdo->state = SDOSTATE_READY;
	return;

    } else if ((*canBuf & CO_SDOBLK_CS_MASK) == CO_SDOBLK_CS_INIT)  {
	/* upload init */
	if (initUpLd_ind(pCurSdo, canBuf CO_COMMA_LINE_PARA) != CO_OK)  {
	    return;
	}

	/* test the min datasize and go back to the normal upload protocol */
	if (pCurSdo->restSize <= canBuf[5])  {
	    pCurSdo->toggleBit = SDO_TOGGLE_BIT;
	    initUpLd_res(pCurSdo CO_COMMA_LINE_PARA);
	    return;
	}

	/* check block size - must be < 128 */
	if (canBuf[4] > 127)  {
	    /* abort for conformance test */
	    (void) abortSdoTransf_Req(pCurSdo, CO_E_SDO_INVALID_BLKSIZE CO_COMMA_LINE_PARA);
	    return;
	}

	/* now block transfer is ok */
	pCurSdo->blkSegSize = canBuf[4];
	pCurSdo->blkSegNr = 1;

	/* CRC supported ? */
# ifdef CONFIG_BLOCK_CRC
	pCurSdo->blkCRC = (*canBuf & CO_SDOBLOCK_USE_CRC) ? CO_TRUE : CO_FALSE;
	pCurSdo->blkCrcSum = 0;
# else /* CONFIG_BLOCK_CRC */
	pCurSdo->blkCRC = CO_FALSE;
# endif /* CONFIG_BLOCK_CRC */

	/* generate a response */
	CMS_SdoEncode(pCurSdo->index, pCurSdo->subIndex, pCurSdo->pData);
        pCurSdo->pData[0] = CO_SDOBLK_SCS_UP | CO_SDOBLOCK_SIZE_VALID;

# ifdef CONFIG_BLOCK_CRC
	if (pCurSdo->blkCRC == CO_TRUE)  {
	    pCurSdo->pData[0] |= CO_SDOBLOCK_USE_CRC;
	}
# endif /* CONFIG_BLOCK_CRC */

	CO_UNPACK_MEMCPY(&pCurSdo->pData[4], (UNSIGNED8 *)&pCurSdo->restSize,
		4, CO_TRUE);

# ifdef CONFIG_REDUNDANCY_SUPPORT
	GL_VAR(co_redcySdoLine) = pCurSdo->commLine;
# endif /* CONFIG_REDUNDANCY_SUPPORT */
	(void) TRANSMIT_COB(pCurSdo->pTrCOB, pCurSdo->pData);

	pCurSdo->state = SDOSTATE_UPLD_BLK_INIT;
    }
}


/*******************************************************************
*
* upLdBlkEnd_req - Upload block transfer end sequence
*
* NOMANUAL
*
* This function send the end block upload sequence
*
* \retval
*	nothing
*
*/
void upLdBlkEnd_req(
	SDO_T *pSdo			/* pointer to current sdo */
        CO_COMMA_LINE_PARA_DECL         /* canline */
    )
{
UNSIGNED8	rest;			/* restsize of actual sdo data */

# ifdef CONFIG_MULT_LINES
    (void)canLine;
# endif /* CONFIG_MULT_LINES */

    /* send block response */
    pSdo->pData[0] = CO_SDOBLK_SCS_UP | CO_SDOBLK_SS_END;
    /* valid bytes in last transferred segment */
    rest = pSdo->lastSegSize;

    /* printf("upLdBlkEnd_req:rest: %d\n", rest); */

    pSdo->pData[0] |= ((7 - rest) & 0x7) << 2;

# ifdef CONFIG_BLOCK_CRC
    if (pSdo->blkCRC == CO_TRUE)
    {
        CO_UNPACK_MEMCPY(&pSdo->pData[1], (UNSIGNED8*)&pSdo->blkCrcSum, 2,CO_TRUE);
        /* printf("upLdBlkEnd_req:blkCrcSum: %X\n", pSdo->blkCrcSum); */
    }
    else
    {
	pSdo->pData[1] = 0;
	pSdo->pData[2] = 0;
    }
    memset(&pSdo->pData[3], (int)0, (size_t)5);
# else /* CONFIG_BLOCK_CRC */
    pSdo->pData[1] = pSdo->pData[2] = 0;
    memset(&pSdo->pData[1], (int)0, (size_t)7);
# endif /* CONFIG_BLOCK_CRC */

# ifdef CONFIG_REDUNDANCY_SUPPORT
    GL_VAR(co_redcySdoLine) = pSdo->commLine;
# endif /* CONFIG_REDUNDANCY_SUPPORT */
    (void) TRANSMIT_COB(pSdo->pTrCOB, pSdo->pData);

    pSdo->state = SDOSTATE_UPLD_BLK_END;
}

#endif /* defined(CONFIG_SDO_SERVER) && defined(CONFIG_SDO_BLOCKTRANSFER) */


#if defined(CONFIG_SDO_CLIENT) && defined(CONFIG_SDO_BLOCKTRANSFER)

/*******************************************************************
*
* dnLdBlk_con - Block Download Confirmation
*
* NOMANUAL
*
* This function react for the download block confirmation of a CANopen server.
* This is the block download init confirmation and the block segment confirmation on client side.
*
* \retval
*	nothing
*
*/
void dnLdBlk_con(
	SDO_CLIENT_T	*pClientSdo,	/* pointer to client sdo */
	UNSIGNED8	*canBuf		/* Pointer to CAN Message */
	CO_COMMA_LINE_PARA_DECL
    )
{
UNSIGNED8	segNr;			/* segment number */
SDO_T		*pSdo;
# ifdef CONFIG_BLOCK_CRC
UNSIGNED8       i;
# endif /* CONFIG_BLOCK_CRC */

    pSdo = &pClientSdo->sdo;

    /* server subcommand */
    /* init sdo block download con ? */
    if ((*canBuf & CO_SDOBLK_SS_MASK) == 0)  {

	if (pSdo->state != SDOSTATE_DNLD_BLK_INIT)  {
	    return;
	}

	/* set Blocksize */
	pSdo->blkSegSize = canBuf[4];
	if (pSdo->blkSegSize == 0)  {
	    (void) abortSdoTransf_Req(pSdo, CO_E_SDO_INVALID_BLKSIZE CO_COMMA_LINE_PARA);
	    return;
	}

# ifdef CONFIG_BLOCK_CRC
	if ((*canBuf & CO_SDOBLOCK_USE_CRC) == 0)  {
	    pSdo->blkCRC = CO_FALSE;
	    pSdo->blkCrcSum = 0;
            pSdo->blkCrcSumAtBlkStart = 0;
	}
# endif /* CONFIG_BLOCK_CRC */
	pSdo->blkSegNr = 1;
	pSdo->pActualDomData = pSdo->pDomData;

	pSdo->state = SDOSTATE_DNLD_BLK_SEG;

        /* make sure attr is set */
        pSdo->attr = getObjAttr(pSdo->index, pSdo->subIndex CO_COMMA_LINE_PARA);

    } else if ((*canBuf & CO_SDOBLK_SS_MASK) == 2)  {
	/* block download segment confirm */

	/* transmission successful? */
	segNr = canBuf[1];
	if (segNr != (pSdo->blkSegNr - 1))  {
	    /* transmission failed */
# ifdef CONFIG_DOMAIN_CONFIRMATION
	    /*  domain confirmation can be different from block size
		therefore we can't handle the different pointers
		and have to abort the transfer
	    */
	    sdoWrCon(pSdo->num, CO_E_SDO_OTHER CO_COMMA_LINE_PARA);
	    abortSdoTransf_Req(pSdo, CO_E_SDO_OTHER);
# else /* CONFIG_DOMAIN_CONFIRMATION */
            /* reset transmission to first not successfully transmited segment */
	    pSdo->blkSegNr = segNr + 1;
	    pSdo->pActualDomData = pSdo->pDomData + segNr * 7;
	    pSdo->restSize = pSdo->domSize - (UNSIGNED32)segNr * 7;

#  ifdef CONFIG_BLOCK_CRC
            /* printf("recalculating CRC Sum\n"); */
            pSdo->blkCrcSum = pSdo->blkCrcSumAtBlkStart;
            /* printf("last stored CRC %x\n",pSdo->blkCrcSum); */
            for (i = 0; i < segNr; i++)
            {
                pSdo->blkCrcSum = crc16Calc(pSdo->pDomData + i * 7, pSdo->blkCrcSum, 7
                #  ifdef CONFIG_16BIT_CPU
                    , CO_FALSE
                #  endif /* CONFIG_16BIT_CPU */
                                           );
                /* printf("recalculated CRC step %x %x\n", i, pSdo->blkCrcSum); */
            }
#  endif /* CONFIG_BLOCK_CRC */

# endif /* CONFIG_DOMAIN_CONFIRMATION */
	} else  {
	    /* transmission ok */
# ifdef CONFIG_BLOCK_CRC
            pSdo->blkCrcSumAtBlkStart = pSdo->blkCrcSum;
# endif /* CONFIG_BLOCK_CRC */
	    /* move the pointer if not end of transmission */
	    if (pSdo->restSize != 0)  {
		pSdo->pDomData = pSdo->pActualDomData;
		pSdo->domSize = pSdo->restSize;
	    } else {
		/* all data is transmitted */
		dnLdBlkEnd_req(pClientSdo CO_COMMA_LINE_PARA);
		return;
	    }
	    pSdo->blkSegNr = 1;

	    /* set new block size */
	    pSdo->blkSegSize = canBuf[2];
	}
    }

    sdoBlockTrans(pSdo, pClientSdo CO_COMMA_LINE_PARA);
}


/*******************************************************************
*
* dnLdBlkEnd_req - download block transfer end sequence
*
* NOMANUAL
*
* This function starts the block download end sequence
*
* \retval
*	nothing
*
*/
void dnLdBlkEnd_req(
	SDO_CLIENT_T	*pClientSdo	/* pointer to current sdo */
	CO_COMMA_LINE_PARA_DECL	/* number of CAN line 0..CO_MAX_CAN_LINES-1 */
    )
{
UNSIGNED8	rest;			/* data length of a actual sdo */
UNSIGNED8	tData[8];		/* transmit buffer */
SDO_T		*pSdo;

    pSdo = &pClientSdo->sdo;

    /* send block response */
    tData[0] = CO_SDOBLK_CCS_DOWN | CO_SDOBLK_SS_END;
    /* save the unused data */
    /* at domSize are the data count from last block */
    /* rest = (UNSIGNED8)(pSdo->domSize % 7);
    if (rest == 0) { */
	/* the last message contained 7 valid databytes */
    /*	rest = 7;
    } */
    /* valid bytes in last transferred segment */
    rest = pSdo->lastSegSize;

    /* printf("dnLdBlkEnd_req:rest: %d\n", rest); */
    tData[0] |= ((7 - rest) & 0x7) << 2;

# ifdef CONFIG_BLOCK_CRC
    CO_UNPACK_MEMCPY(&tData[1], (UNSIGNED8*)&pSdo->blkCrcSum, 2, CO_TRUE);
    memset(&tData[3], (int)0, (size_t)5);
# else /* CONFIG_BLOCK_CRC */
    memset(&tData[1], (int)0, (size_t)7);
# endif /* CONFIG_BLOCK_CRC */

# ifdef CONFIG_REDUNDANCY_SUPPORT
    GL_VAR(co_redcySdoLine) = pSdo->commLine;
# endif /* CONFIG_REDUNDANCY_SUPPORT */
    (void) TRANSMIT_COB(pSdo->pTrCOB, tData);

    pSdo->state = SDOSTATE_DNLD_BLK_END;

    /* start timeout again */
    (void) addTimerEvent(&pClientSdo->timer, pClientSdo->timeOut,
		CO_TIMER_TYPE_SDO CO_COMMA_LINE_PARA);
}


/*******************************************************************
*
* initUpLdBlk_con - init upload block transfer confirmation
*
* NOMANUAL
*
* Depending on server subcommand this function is either the
* SDO block upload end indication and response, or the
* SDO block upload init confirmation and request of first block.
*
* \retval
*	nothing
*
*/
void initUpLdBlk_con(
	SDO_CLIENT_T	*pClientSdo,	/* pointer of actual client sdo */
	UNSIGNED8	*canBuf		/* pointer to can buffer */
	CO_COMMA_LINE_PARA_DECL	/* number of CAN line 0..CO_MAX_CAN_LINES-1 */
    )
{
SDO_T		*pSdo;			/* pointer of sdo */
UNSIGNED32	size;			/* complete object data size */
UNSIGNED8	len;			/* actual sdo data len */
# ifdef CONFIG_BLOCK_CRC
UNSIGNED16	crc;			/* calculated crc */
# endif /* CONFIG_BLOCK_CRC */

    pSdo = &pClientSdo->sdo;

    if ((*canBuf & 1) != 0)  {
	/* end sdo block */
        /* SDO block upload end indication and response */

	/* copy the data from last segment */
	len = 7 - (((*canBuf) >> 2) & 0x7);

# ifdef CONFIG_16BIT_CPU
	/* if domain download */
	if (pSdo->numeric == CO_TRUE)  {
	    pSdo->pActualDomData =
		pack_oddmemcpy(pSdo->pActualDomData, &pSdo->pData[1],
		    len, &pSdo->halfWord);
	} else
# endif /* CONFIG_16BIT_CPU */
	{
	    CO_MEMCPY(pSdo->pActualDomData, &pSdo->pData[1], len);
	}

# ifdef CONFIG_BLOCK_CRC
	if (pSdo->blkCRC == CO_TRUE)  {

	    pSdo->blkCrcSum = crc16Calc(&pSdo->pData[1], pSdo->blkCrcSum, len
#  ifdef CONFIG_16BIT_CPU
		, CO_FALSE
#  endif /* CONFIG_16BIT_CPU */
		);

	    CO_PACK_MEMCPY((UNSIGNED8 *)&crc, canBuf + 1, 2, CO_TRUE);
            /* printf("received CRC: %x\n",crc); */
            /* printf("calculated CRC: %x\n",pSdo->blkCrcSum); */
	    if (crc != pSdo->blkCrcSum)  {
		/* informs application about this */
		sdoRdCon(pSdo->num,
		    E_SDO_SERVICE | E_SDO_ILLEG_PARA | E_SDO_A_CRC_INVALID
#  ifdef CONFIG_MULT_LINES
		    ,pSdo->pRecCOB->canLine
#  endif /* CONFIG_MULT_LINES */
		    );
		/* Abort Sdo Transfer */
		abortSdoTransf_Req(pSdo, CO_E_SDO_INVALID_BLKCRC CO_COMMA_LINE_PARA);
		return;
	    }
	}
# endif /* CONFIG_BLOCK_CRC */

	/* informs application about new value */
	sdoRdCon( pSdo->num, 0 CO_COMMA_LINE_PARA );

	pSdo->pData[0] = CO_SDOBLK_CCS_UP | CO_SDOBLK_CS_UP_END;
	memset(&pSdo->pData[1], (int)0, (size_t)7);

# ifdef CONFIG_REDUNDANCY_SUPPORT
	GL_VAR(co_redcySdoLine) = pSdo->commLine;
# endif /* CONFIG_REDUNDANCY_SUPPORT */
	(void) TRANSMIT_COB(pSdo->pTrCOB, pSdo->pData);

	pSdo->state = SDOSTATE_READY;
	return;
    }

    /* init block */
    /* SDO block upload init confirmation + request of first block */
# ifdef CONFIG_BLOCK_CRC
    if ((*canBuf & CO_SDOBLOCK_USE_CRC) == 0)  {
	pSdo->blkCRC = CO_FALSE;
    }
# endif /* CONFIG_BLOCK_CRC */

    /* test for buffer length */
    if ((*canBuf & CO_SDOBLOCK_SIZE_VALID) != 0)  {
	CO_PACK_MEMCPY((UNSIGNED8 *)&size, &canBuf[4], 4, CO_TRUE);
	if (pSdo->domSize < size)  {
	    /* buffer to small, abort */
	    (void) abortSdoTransf_Req(pSdo, CO_E_WRONG_SIZE CO_COMMA_LINE_PARA);

	    /* informs application about this */
	    sdoRdCon(pSdo->num,
		    E_SDO_ACCESS | E_SDO_TYPE_CONFLICT | E_SDO_A_INVALID_VAL
                    CO_COMMA_LINE_PARA );
	    return;
	}
	/* correct size information */
	pSdo->domSize = size;

#  ifdef CONFIG_DOMAIN_CONFIRMATION
	pSdo->restSize = size;

	/* calculate next border */
	pClientSdo->nextDomainIndBorder =
		pSdo->restSize - pClientSdo->domainIndSize;
#  endif /* CONFIG_DOMAIN_CONFIRMATION */
    }

    pSdo->blkSegNr = 1;
    pSdo->blkSegSize = pSdo->blkSegDefaultSize;

    /* start upload */
    pSdo->pData[0] = CO_SDOBLK_CCS_UP | CO_SDOBLK_CS_UP_START;
    memset(&pSdo->pData[1], (int)0, (size_t)7);

# ifdef CONFIG_REDUNDANCY_SUPPORT
    GL_VAR(co_redcySdoLine) = pSdo->commLine;
# endif /* CONFIG_REDUNDANCY_SUPPORT */
    (void) TRANSMIT_COB(pSdo->pTrCOB, pSdo->pData);

    pSdo->state = SDOSTATE_UPLD_BLK_SEG;

    /* start timeout */
    (void) addTimerEvent(&pClientSdo->timer, pClientSdo->timeOut,
	    CO_TIMER_TYPE_SDO CO_COMMA_LINE_PARA);
}


/*******************************************************************
*
* upLdBlk_ind - Block Upload Indication
*
* NOMANUAL
*
* This function responses a upload block request of a CANopen server.
*
* \retval
*	nothing
*
*/
void upLdBlk_ind(
	SDO_CLIENT_T	*pClientSdo,	/* pointer to current client sdo */
	UNSIGNED8	*canBuf		/* Pointer to CAN buffer */
	CO_COMMA_LINE_PARA_DECL	/* number of CAN line 0..CO_MAX_CAN_LINES-1 */
    )
{
SDO_T		*pSdo;
UNSIGNED8	segNr;			/* block segment number */
UNSIGNED32	size;			/* data size */

    pSdo = &pClientSdo->sdo;
    /* get segment number */
    segNr = *canBuf & 0x7f;


    /* printf("segNr: 0x%02x\n",segNr); */
    /* for testing only: creates transmission error */
    /* if (test > 0)
    {
        if (test > 1)
        {
            if (segNr == 8)    // ignore a segment with id 9
            {
                test--;
                return;
            }
        }

        if (test == 1)
        {
            if (segNr == 9)     // ignore a sgement with id 3
            {
                test--;
                return;
            }
        }
    }*/


    /* test for valid block number */
    if (segNr == pSdo->blkSegNr)  {
	/* ok, copy data into buffer */
	size = pSdo->restSize;
	if (size > 7)  {
	    size = 7;
	}
	/* don't copy the last message - the valid length will be transmitted
	 * with the next message */
	if ((*canBuf & CO_SDOBLOCK_CONT_FLAG) == 0) {

# ifdef CONFIG_16BIT_CPU
	    /* if domain download */
	    if (pSdo->numeric == CO_TRUE)  {
		pSdo->pActualDomData =
		    pack_oddmemcpy(pSdo->pActualDomData, canBuf + 1,
			size, &pSdo->halfWord);
	    } else
# endif /* CONFIG_16BIT_CPU */
	    {
		CO_MEMCPY(pSdo->pActualDomData, &canBuf[1], size);
		/* actualise address and size */
		pSdo->pActualDomData += size;
	    }
	    pSdo->restSize -= size;

# ifdef CONFIG_BLOCK_CRC
	    pSdo->blkCrcSum = crc16Calc(&canBuf[1], pSdo->blkCrcSum, size
#  ifdef CONFIG_16BIT_CPU
		, CO_FALSE
#  endif /* CONFIG_16BIT_CPU */
		);
# endif

# ifdef CONFIG_DOMAIN_CONFIRMATION
	    if (pClientSdo->domainIndSize != 0)  {
		/* next border reached ? */
		if (pSdo->restSize == pClientSdo->nextDomainIndBorder)  {
		    /* call indication function */
		    if (sdoDomainRdCon(pSdo->num CO_COMMA_LINE_PARA) != CO_OK)  {
			/* abort transfer */
			abortSdoTransf_Req(pSdo, CO_E_HARDWARE_FAULT);
			return;
		    }
		    /* reset pointer */
		    /* pSdo->pActualDomData = pSdo->pDomData; */
		    pSdo->pActualDomData = pClientSdo->pBufferStart;
		    /* calculate next border */
		    if (pClientSdo->domainIndSize < pSdo->restSize)  {
			pClientSdo->nextDomainIndBorder =
				pSdo->restSize - pClientSdo->domainIndSize;
		    }
		}
	    }
# endif /* CONFIG_DOMAIN_CONFIRMATION */
	}
	/* prepare next segment */
	pSdo->blkSegNr++;
    }

    /* if block count reached or last block transmitted */
    if ((segNr >= pSdo->blkSegSize)
     || ((*canBuf & CO_SDOBLOCK_CONT_FLAG) != 0))  {
	/* send block response */
	pSdo->pData[0] = CO_SDOBLK_CCS_UP | CO_SDOBLK_CS_UP_RESP;
	pSdo->pData[1] = pSdo->blkSegNr - 1;
	pSdo->pData[2] = pSdo->blkSegSize;

# ifdef CONFIG_REDUNDANCY_SUPPORT
	GL_VAR(co_redcySdoLine) = pSdo->commLine;
# endif /* CONFIG_REDUNDANCY_SUPPORT */
	(void) TRANSMIT_COB(pSdo->pTrCOB, pSdo->pData);

	/* reset block counter */
	if ((segNr + 1) == pSdo->blkSegNr)  {
	    pSdo->blkSegNr = 1;
	}
    }

    /* if last block transmitted finish block download */
    if ((*canBuf & CO_SDOBLOCK_CONT_FLAG) != 0)  {
	/* save the last data */
	CO_MEMCPY(pSdo->pData, canBuf, 8);
	pSdo->state = SDOSTATE_UPLD_BLK_END;
    }

    /* start timeout */
    (void) addTimerEvent(&pClientSdo->timer, pClientSdo->timeOut,
	    CO_TIMER_TYPE_SDO CO_COMMA_LINE_PARA);
}

#endif /* defined(CONFIG_SDO_CLIENT) && defined(CONFIG_SDO_BLOCKTRANSFER) */
/*______________________________________________________________________EOF_*/
