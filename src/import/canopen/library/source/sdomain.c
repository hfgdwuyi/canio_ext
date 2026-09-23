/*
 *++ sdomain - subroutines needed for domain transfers
 *-- sdomain - Unterprogramme für Domain Transfer
 *
 * Copyright (c) 2001-2017 port GmbH Halle (Saale)
 *------------------------------------------------------------------
 */

/*
*  \file sdomain.c
*++ Subroutines needed for domain transfers
*-- Unterprogramme für Domain Transfer
*  \author port GmbH Halle (Saale)
*
*++ This module contains support functions for domain transfer.
*-- Dieses Modul enthält Hilfsfunktionen für den Domaintransfer.
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

#include <string.h>
#include <stdio.h>

/* header of project specific types */

#include <cal_conf.h>
#include <co_debug.h>
#include <co_odidx.h>
#include <co_acces.h>
#include <co_mcpy.h>
#include <co_def.h>
#include "sdomain.h"
#include "cmscodec.h"
#include "sdo.h"
#include "nmt.h"
#include "drv.h"
#include "utility.h"

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

/* external variables
---------------------------------------------------------------------------*/

/* global variables
---------------------------------------------------------------------------*/

/* local defined variables
---------------------------------------------------------------------------*/
#ifdef CONFIG_RCS_IDENT
CO_LIB_INIT_VAR static char _rcsid[] = "$Id: sdomain.c,v 2.45 2016/11/01 20:03:08 rli Exp $";
#endif /* CONFIG_RCS_IDENT */


#if defined(CONFIG_SDO_SERVER) || defined(CONFIG_SDO_CLIENT)
/*******************************************************************
*
* sdoMsgReceived - sdo receive function
*
* \internal
*
* this function calls the corresponding coding or
* encoding functions to transfer a multiplexed domain.
* it calls the response or confirmation primitives too.
*
* \retval
*	nothing
*/

void sdoMsgReceived(
	CAN_MSG_T *canMsg	/* Pointer to CAN Message */
	CO_COMMA_REDCY_PARA_DECL
    )
{
# ifdef CONFIG_SDO_SERVER
SDO_T *pSdo;			/* pointer to actual sdo */
# endif /* CONFIG_SDO_SERVER */
# ifdef CONFIG_SDO_CLIENT
SDO_CLIENT_T *pClientSdo;	/* pointer to actual sdo */
# endif /* CONFIG_SDO_CLIENT */
NODE_STATE_T actState;

# ifdef CONFIG_REDUNDANCY_SUPPORT
    /* check node state of received line */
    if (canLine == CAN_DEFAULT_LINE)  {
	actState = GL_ARRAY(co_Node).eState;
    } else {
	actState = GL_VAR(co_redcyNode.eState);
    }
# else /* CONFIG_REDUNDANCY_SUPPORT */
    actState = GL_ARRAY(co_Node).eState;
# endif /* CONFIG_REDUNDANCY_SUPPORT */

    if ((actState != OPERATIONAL) && (actState != PRE_OPERATIONAL)) {
	 return;
    }

# ifdef CONFIG_SDO_SERVER
    pSdo = searchForServerSdoCobId(canMsg->cobId CO_COMMA_LINE_PARA);
    if (pSdo != NULL)  {
	/* SDO disabled */
	if (pSdo->state != SDOSTATE_DISABLED)  {
	    sdoServerMsgInd(pSdo, canMsg CO_COMMA_LINE_PARA);
	}

	return;
    }
# endif 	/* defined(SDO_SERVER) */

# if defined(CONFIG_SDO_CLIENT)
    pClientSdo = searchForClientSdoCobId(canMsg->cobId CO_COMMA_LINE_PARA);
    if (pClientSdo != NULL)  {
	/* SDO disabled */
	if (pClientSdo->sdo.state != SDOSTATE_DISABLED)  {
	    sdoClientMsgCon(pClientSdo, canMsg CO_COMMA_GLOBVARS_PARA);
	}

	return;
    }
# endif /* defined(SDO_CLIENT) */
}


/*******************************************************************
*
* setSdoCobId - sets the COB-ID of SDO
*
* \internal
*
* This function sets cob-ids for SDO
* If the disable bit at one of the 2 cobs is set,
* the sdo will be disabled.
*
* \retval CO_OK
*	success
* \retval CO_E_NOT_EXIST
*	internal communication object doesn't exist
* \retval CO_E_RANGE
*	COB-ID is out of the range (1..1760)
* \retval CO_E_TRANS_TYPE
*	bad transtype
*
*/

RET_T setSdoCobId(
	UNSIGNED8  sdoNr,	/* sdo number */
	UNSIGNED32 cobId,	/* new COB-ID */
	USER_T	   kind,	/* kind of SDO SERVER/CLIENT */
	COB_KIND_T cobType	/* kind of use */
	CO_COMMA_LINE_PARA_DECL	/* number of CAN line 0..CO_MAX_CAN_LINES-1 */
    )
{
SDO_T		*pSdo = NULL;
# ifdef CONFIG_SDO_CLIENT
SDO_CLIENT_T	*pClientSdo;
# endif /* CONFIG_SDO_CLIENT */

#ifndef CO_CONFIG_DONT_CHECK_RESTRICTED_COBID
RET_T retVal = CO_OK;
     retVal = coCheckRestrictedCobId(SSDO_PARA_BASE_INDEX, cobId CO_COMMA_LINE_PARA); /* SDO checks all restricted cobIds but the cobIds reserved for SDOs */
     if (retVal != CO_OK)  {
         return(retVal);
     }
#endif


# ifdef CONFIG_SDO_SERVER
    if (kind == SERVER)  {
	pSdo = searchForServerSdoNr(sdoNr CO_COMMA_LINE_PARA);
	if (pSdo == NULL)  {
	    return(CO_E_NOT_EXIST);
	}
    } else
# endif /* CONFIG_SDO_SERVER */
    {
# ifdef CONFIG_SDO_CLIENT
	pClientSdo = searchForClientSdoNr(sdoNr CO_COMMA_LINE_PARA);
	if (pClientSdo == NULL)  {
	    return(CO_E_NOT_EXIST);
	}
	pSdo = &pClientSdo->sdo;
# endif /* CONFIG_SDO_CLIENT */
    }

    return pcoSetSdoPtrCobId(pSdo, cobId, kind, cobType CO_COMMA_LINE_PARA);

}

/*******************************************************************
*
* pcoSetSdoCobId - sets the COB-ID of SDO
*
* \internal
*
* This function sets cob-ids for pointer SDO
* If the disable bit at one of the 2 cobs is set,
* the sdo will be disabled.
*
* \retval CO_OK
*	success
* \retval CO_E_NOT_EXIST
*	internal communication object doesn't exist
* \retval CO_E_RANGE
*	COB-ID is out of the range (1..1760)
* \retval CO_E_TRANS_TYPE
*	bad transtype
*
*/

RET_T pcoSetSdoPtrCobId(
	SDO_T  	   *pSdo,	/* sdo pointer */
	UNSIGNED32 cobId,	/* new COB-ID */
	USER_T	   kind,	/* kind of SDO SERVER/CLIENT */
	COB_KIND_T cobType	/* kind of use */
	CO_COMMA_LINE_PARA_DECL	/* number of CAN line 0..CO_MAX_CAN_LINES-1 */
    )
{
RET_T		retVal;

    if ( pSdo == NULL ) {
        return(CO_E_NOT_EXIST);
    }

    /* change cob-id is not allowed for active transfers */
    if ((pSdo->state != SDOSTATE_READY) && (pSdo->state != SDOSTATE_DISABLED)) {
	return(CO_E_BUSY);
    }

# ifdef CONFIG_LSS_SLAVE
    /* if the node unconfigured */
    if (GL_ARRAY(coNodeId) == 255)  {
	cobId |= SDO_NO_VALID_BIT;
    }
# endif /* CONFIG_LSS_SLAVE */

    /* set to invalid */
    if ((cobId & SDO_NO_VALID_BIT) != 0u)  {
	pSdo->state = SDOSTATE_DISABLED;

	if (cobType == CO_COB_SDO_RX)  {
	    pSdo->flags &= (FLAG_T)~SDOFLAG_REC_COB_VALID;
	    retVal = SET_COB_ID(pSdo->pRecCOB, cobId & ~SDO_NO_VALID_BIT,
		(COB_KIND_T)(cobType | CO_COB_DISABLED));
	} else {
	    pSdo->flags &= (FLAG_T)~SDOFLAG_TR_COB_VALID;
	    retVal = SET_COB_ID(pSdo->pTrCOB, cobId & ~SDO_NO_VALID_BIT,
		(COB_KIND_T)(cobType | CO_COB_DISABLED));
	}
	return(retVal);
    }

    /* changing cob id is only allowed if sdo is disabled */
    if (pSdo->state != SDOSTATE_DISABLED)  {
	return(CO_E_STATE);
    }

    if (cobType == CO_COB_SDO_RX)  {
	retVal = SET_COB_ID(pSdo->pRecCOB, cobId, CO_COB_SDO_RX);
	pSdo->flags |= SDOFLAG_REC_COB_VALID;
    } else {
	retVal = SET_COB_ID(pSdo->pTrCOB, cobId, CO_COB_SDO_TX);
	pSdo->flags |= SDOFLAG_TR_COB_VALID;
    }

    if ((pSdo->flags & SDOFLAG_COBS_VALID) == SDOFLAG_COBS_VALID)  {
	pSdo->state = SDOSTATE_READY;

# ifdef CONFIG_FAST_SORT
	/* sort list again */
	if (kind == SERVER)  {
	    sortCobIdList(
#  ifdef CONFIG_MULT_LINES
		&GL_PVAR(co_sdoServerCobIdxList)[GL_ARRAY(co_sdoServerLineOffs)],
		&GL_PVAR(co_sdoServer)[GL_ARRAY(co_sdoServerLineOffs)].pRecCOB,
#  else /* CONFIG_MULT_LINES */
		&GL_PVAR(co_sdoServerCobIdxList)[0],
		&GL_PVAR(co_sdoServer)[0].pRecCOB,
#  endif /* CONFIG_MULT_LINES */
		sizeof(SDO_T),
		GL_ARRAY(co_sdoServerCnt));
#  ifdef CONFIG_SDO_CLIENT
	} else {
	    sortCobIdList(
#   ifdef CONFIG_MULT_LINES
		&GL_PVAR(co_sdoClientCobIdxList)[GL_ARRAY(co_sdoClientLineOffs)],
		&GL_PVAR(co_sdoClient)[GL_ARRAY(co_sdoClientLineOffs)].sdo.pRecCOB,
#   else /* CONFIG_MULT_LINES */
		 &GL_PVAR(co_sdoClientCobIdxList)[0],
		&GL_PVAR(co_sdoClient)[0].sdo.pRecCOB,
#   endif /* CONFIG_MULT_LINES */
		sizeof(SDO_CLIENT_T),
		GL_ARRAY(co_sdoClientCnt));
#  endif /* CONFIG_SDO_CLIENT */
	}
# endif /* CONFIG_FAST_SORT */
    }

    return(retVal);
}



/*******************************************************************
*
* resetAllSdos - reset all SDOs at resetComm
*
* \internal
*
* The function resets all SDOs at resetComm command
* The COB-Id was set by resetObjDir
*
*
*/
void resetAllSdos(
	CO_LINE_PARA_DECL	/* number of CAN line 0..CO_MAX_CAN_LINES-1 */
    )
{
UNSIGNED8	nr;
UNSIGNED16	sdoNr;
UNSIGNED32	cobId, size;
RET_T		commonRet;
# ifdef CONFIG_SDO_SERVER
SDO_T		*pSdo;
# endif /* CONFIG_SDO_SERVER */
# ifdef CONFIG_SDO_CLIENT
SDO_CLIENT_T	*pClientSdo;
# endif /* CONFIG_SDO_CLIENT */


# ifdef CONFIG_SDO_SERVER
    /* for all yet defined sdos */
    nr = (UNSIGNED8)GL_ARRAY(co_sdoServerCnt);
    while (nr > 0u)  {

	/* get address of sdo */
	pSdo = &GL_PVAR(co_sdoServer)[(nr - 1u)
#  ifdef CONFIG_MULT_LINES
		+ GL_ARRAY(co_sdoServerLineOffs)
#  endif /* CONFIG_MULT_LINES */
		];

	/* check if sdo is busy */
	if ((pSdo->state != SDOSTATE_READY)
	 && (pSdo->state != SDOSTATE_DISABLED)) {
	    /* yes, is busy - set to ready */
	    pSdo->state = SDOSTATE_READY;
	}

	/* get sdo number */
	sdoNr = pSdo->num;

	if (sdoNr == 1u)  {
	    /* first server sdo */
	    cobId = CO_COBID_CSDO + GL_ARRAY(coNodeId);
	    commonRet = CO_OK;
	} else {
	    commonRet = getObjEntry(SSDO_PARA_BASE_INDEX + sdoNr - 1u, 1u,
		(UNSIGNED8 *)&cobId, &size, CO_TRUE  CO_COMMA_LINE_PARA);
	}
	if (commonRet == CO_OK)  {
	    if ((cobId & SDO_NO_VALID_BIT) == 0u) {
		/* disable sdo first */
		(void)setSdoCobId(nr, SDO_NO_VALID_BIT, SERVER, CO_COB_SDO_RX
		    CO_COMMA_LINE_PARA);
	    }
	    (void)setSdoCobId(nr, cobId, SERVER, CO_COB_SDO_RX CO_COMMA_LINE_PARA);
	}

	if (sdoNr == 1u)  {
	    /* first server sdo */
	    cobId = CO_COBID_SSDO + GL_ARRAY(coNodeId);
	    commonRet = CO_OK;
	} else {
	    commonRet = getObjEntry(SSDO_PARA_BASE_INDEX + sdoNr - 1u, 2u,
		(UNSIGNED8 *)&cobId, &size, CO_TRUE  CO_COMMA_LINE_PARA);
	}
	if (commonRet == CO_OK)  {
	    if ((cobId & SDO_NO_VALID_BIT) == 0u) {
		/* disable sdo first */
		(void)setSdoCobId(nr, SDO_NO_VALID_BIT, SERVER, CO_COB_SDO_TX
		    CO_COMMA_LINE_PARA);
	    }
	    (void)setSdoCobId(nr, cobId, SERVER, CO_COB_SDO_TX CO_COMMA_LINE_PARA);
	}

	nr--;
    }
# endif /* CONFIG_SDO_SERVER */

# ifdef CONFIG_SDO_CLIENT
    nr = 1;
    /* for all yet defined pdos */
    while (nr <= (UNSIGNED8)GL_ARRAY(co_sdoClientCnt))  {
	pClientSdo = &GL_PVAR(co_sdoClient)[(nr - 1)
#  ifdef CONFIG_MULT_LINES
		+ GL_ARRAY(co_sdoClientLineOffs)
#  endif /* CONFIG_MULT_LINES */
		];

	/* check if sdo is busy */
	if ((pClientSdo->sdo.state != SDOSTATE_READY)
	 && (pClientSdo->sdo.state != SDOSTATE_DISABLED)) {
	    /* yes, is busy - stop timer */
	    removeTimerEvent(&pClientSdo->timer CO_COMMA_LINE_PARA);
	    pClientSdo->sdo.state = SDOSTATE_READY;
	}

	sdoNr = pClientSdo->sdo.num;
	commonRet = getObjEntry(CSDO_PARA_BASE_INDEX + sdoNr - 1, 1,
	    (UNSIGNED8 *)&cobId, &size, CO_TRUE  CO_COMMA_LINE_PARA);
	if (commonRet == CO_OK)  {
	    if ((cobId & SDO_NO_VALID_BIT) == 0) {
		/* disable sdo first */
		(void) setSdoCobId(nr, SDO_NO_VALID_BIT, CLIENT, CO_COB_SDO_TX
		    CO_COMMA_LINE_PARA);
	    }
	    (void) setSdoCobId(nr, cobId, CLIENT, CO_COB_SDO_TX CO_COMMA_LINE_PARA);
	}

	commonRet = getObjEntry(CSDO_PARA_BASE_INDEX + sdoNr - 1, 2,
	    (UNSIGNED8 *)&cobId, &size, CO_TRUE  CO_COMMA_LINE_PARA);
	if (commonRet == CO_OK)  {
	    if ((cobId & SDO_NO_VALID_BIT) == 0) {
		/* disable sdo first */
		(void) setSdoCobId(nr, SDO_NO_VALID_BIT, CLIENT, CO_COB_SDO_RX
		    CO_COMMA_LINE_PARA);
	    }
	    (void) setSdoCobId(nr, cobId, CLIENT, CO_COB_SDO_RX CO_COMMA_LINE_PARA);
	}

	nr++;
    }
# endif /* CONFIG_SDO_CLIENT */
}


# ifdef CONFIG_SDO_SERVER
/*******************************************************************
*
* searchForServerSdoNr - search server SDO by SDO number
*
* \internal
*
* The function returns the pointer of an SDO
* with the given SDO number
*
* \retval
*	index at SDO list if SDO number exists
* else -1
*
*/
SDO_T *searchForServerSdoNr(
	UNSIGNED8	sdoNr	/* SDO number */
	CO_COMMA_LINE_PARA_DECL	/**< number of CAN line 0..CO_MAX_CAN_LINES-1 */
    )
{
# ifdef CONFIG_MULT_LINES
SDO_T	*pSdo = &GL_PVAR(co_sdoServer) [GL_ARRAY(co_sdoServerLineOffs)];
# else /* CONFIG_MULT_LINES */
SDO_T	*pSdo = GL_PVAR(co_sdoServer);
# endif /* CONFIG_MULT_LINES */

# ifdef CONFIG_FAST_SORT
#  ifdef CONFIG_MULT_LINES
UNSIGNED8 *pNrList = &GL_PVAR(co_sdoServerNrList) [GL_ARRAY(co_sdoServerLineOffs)];
#  else /* CONFIG_MULT_LINES */
UNSIGNED8 *pNrList = GL_PVAR(co_sdoServerNrList);
#  endif /* CONFIG_MULT_LINES */
INTEGER8 found = 0;
INTEGER8 low = 0, mid = 0, high = GL_ARRAY(co_sdoServerCnt) - 1;

    while (found == 0)  {
	if (high >= low) {
	    mid = (high + low) / 2;
	    if (pSdo[pNrList[mid]].num == sdoNr)  {
		found = 1;
	    } else {
		if (pSdo[pNrList[mid]].num > sdoNr) {
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
	return(NULL);
    } else {
	return(&pSdo[pNrList[mid]]);
    }
# else /* CONFIG_FAST_SORT */
INTEGER8  idx = GL_ARRAY(co_sdoServerCnt) - 1;

    while (idx >= 0)  {
	if (pSdo[idx].num == sdoNr)  {
	    return(&pSdo[idx]);
	}
	idx--;
    }
    return(NULL);
# endif /* CONFIG_FAST_SORT */
}


/*******************************************************************
*
* searchForServerSdoCobId - search server SDO by cob id
*
* \internal
*
* The function returns the address of an SDO
* with the given COB-Id
*
* \retval
*	pointer to SDO struct if cobid exists
* else NULL
*
*/
SDO_T *searchForServerSdoCobId(
	COB_IDENT_T	cobId		/* value to search for */
	CO_COMMA_LINE_PARA_DECL	/**< number of CAN line 0..CO_MAX_CAN_LINES-1 */
    )
{
# ifdef CONFIG_MULT_LINES
SDO_T	*pSdo = &GL_PVAR(co_sdoServer)[GL_ARRAY(co_sdoServerLineOffs)];
# else /* CONFIG_MULT_LINES */
SDO_T	*pSdo = GL_PVAR(co_sdoServer);
# endif /* CONFIG_MULT_LINES */

# ifdef CONFIG_FAST_SORT
INTEGER8  found = 0;
INTEGER8  low = 0, mid = 0, high = GL_ARRAY(co_sdoServerCnt) - 1;
UNSIGNED8 *pCobIdxList;

#  ifdef CONFIG_MULT_LINES
    pCobIdxList = &GL_PVAR(co_sdoServerCobIdxList)[GL_ARRAY(co_sdoServerLineOffs)];
#  else /* CONFIG_MULT_LINES */
    pCobIdxList = &GL_PVAR(co_sdoServerCobIdxList)[0];
#  endif /* CONFIG_MULT_LINES */

    while (found == 0)  {
	if (high >= low) {
	    mid = (high + low) / 2;
	    if (pSdo[pCobIdxList[mid]].pRecCOB->cobId == cobId)  {
		found = 1;
	    } else {
		if (pSdo[pCobIdxList[mid]].pRecCOB->cobId > cobId) {
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
	return(NULL);
    } else {
	return(&pSdo[pCobIdxList[mid]]);
    }
# else /* CONFIG_FAST_SORT */
INTEGER8	i = GL_ARRAY(co_sdoServerCnt) - 1;
COB_T		*pCob;

    while (i >= 0)  {
	/* the following statement doesn't work with Keil compiler:
	  therefore has to be splitted in two statements
		if (pSdo[i].pRecCOB->cobId == cobId) {
	 */
	pCob = pSdo[i].pRecCOB;
	if (pCob->cobId == cobId) {
	    return(&pSdo[i]);
	}
	i--;
    }

    return(NULL);
# endif /* CONFIG_FAST_SORT */
}
#endif /* CONFIG_SDO_SERVER */

#ifdef CONFIG_SDO_CLIENT
/*******************************************************************
*
* searchForClientSdoNr - search Client SDO by SDO number
*
* \internal
*
* The function returns the pointer of an Client SDO
* with the given SDO number
*
* \retval
*	index at sdo list if pdo number exists
* else -1
*
*/
SDO_CLIENT_T *searchForClientSdoNr(
	UNSIGNED8	sdoNr		/* SDO number */
	CO_COMMA_LINE_PARA_DECL	/* number of CAN line 0..CO_MAX_CAN_LINES-1 */
    )
{
# ifdef CONFIG_MULT_LINES
SDO_CLIENT_T	*pSdo = &GL_PVAR(co_sdoClient)[GL_ARRAY(co_sdoClientLineOffs)];
# else /* CONFIG_MULT_LINES */
SDO_CLIENT_T	*pSdo = GL_PVAR(co_sdoClient);
# endif /* CONFIG_MULT_LINES */

# ifdef CONFIG_FAST_SORT
UNSIGNED8	*pIdxList;
INTEGER8	found = 0;
INTEGER8	low, mid = 0, high;

    low = 0;
    high = GL_ARRAY(co_sdoClientCnt) - 1;
#  ifdef CONFIG_MULT_LINES
    pIdxList = &GL_PVAR(co_sdoClientNrList)
		[GL_ARRAY(co_sdoClientLineOffs)];
#  else /* CONFIG_MULT_LINES */
    pIdxList = &GL_PVAR(co_sdoClientNrList)[0];
#  endif /* CONFIG_MULT_LINES */

    sdoNr &= ~CO_NUM_SDO;

    while (found == 0)  {
	if (high >= low) {
	    mid = (high + low) / 2;
	    if (pSdo[pIdxList[mid]].sdo.num == sdoNr)  {
		found = 1;
	    } else {
		if (pSdo[pIdxList[mid]].sdo.num > sdoNr) {
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
	return(NULL);
    } else {
	return(&pSdo[pIdxList[mid]]);
    }
# else /* CONFIG_FAST_SORT */
INTEGER8 idx = GL_ARRAY(co_sdoClientCnt) - 1;

    while (idx >= 0)  {
	if (pSdo[idx].sdo.num == sdoNr)  {

	    return(&pSdo[idx]);
	}
	idx--;
    }
    return(NULL);
# endif /* CONFIG_FAST_SORT */
}


/*******************************************************************
*
* searchForClientSdoCobId - search Client sdo by cob id
*
* \internal
*
* The function returns the address of an Sdo
* with the given COB-Id
*
* \retval
*	pointer to SDO struct if cobid exists
* else NULL
*
*/
SDO_CLIENT_T *searchForClientSdoCobId(
	COB_IDENT_T	cobId		/* value to search for */
	CO_COMMA_LINE_PARA_DECL	/* number of CAN line 0..CO_MAX_CAN_LINES-1 */
    )
{
# ifdef CONFIG_MULT_LINES
SDO_CLIENT_T	*pSdo = &GL_PVAR(co_sdoClient)[GL_ARRAY(co_sdoClientLineOffs)];
# else /* CONFIG_MULT_LINES */
SDO_CLIENT_T	*pSdo = GL_PVAR(co_sdoClient);
# endif /* CONFIG_MULT_LINES */

# ifdef CONFIG_FAST_SORT
INTEGER8  found = 0;
INTEGER8 low = 0, mid = 0, high = GL_ARRAY(co_sdoClientCnt) - 1;
UNSIGNED8 *pCobIdxList;

#  ifdef CONFIG_MULT_LINES
    pCobIdxList = &GL_PVAR(co_sdoClientCobIdxList)[GL_ARRAY(co_sdoClientLineOffs)];
#  else /* CONFIG_MULT_LINES */
    pCobIdxList = &GL_PVAR(co_sdoClientCobIdxList)[0];
#  endif /* CONFIG_MULT_LINES */

    while (found == 0)  {
	if (high >= low) {
	    mid = (high + low) / 2;
	    if (pSdo[pCobIdxList[mid]].sdo.pRecCOB->cobId == cobId)  {
		found = 1;
	    } else {
		if (pSdo[pCobIdxList[mid]].sdo.pRecCOB->cobId > cobId) {
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
	return(NULL);
    } else {
	return(&pSdo[pCobIdxList[mid]]);
    }
# else /* CONFIG_FAST_SORT */
INTEGER8	i = GL_ARRAY(co_sdoClientCnt) - 1;
COB_T		*pCob;

    while (i >= 0)  {
	/* the following statement doesn't work with Keil compiler:
	  therefore has to be splitted in two statements
	    if (pSdo[i].sdo.pRecCOB->cobId == cobId) {
	 */
	pCob = pSdo[i].sdo.pRecCOB;
	if (pCob->cobId == cobId) {
	    return(&pSdo[i]);
	}
	i--;
    }
    return(NULL);
# endif /* CONFIG_FAST_SORT */

}
#endif /* CONFIG_SDO_CLIENT */


#if defined(CONFIG_SDO_SERVER) || defined(CONFIG_SDO_CLIENT)
/****************************************************************************/
/*
*++ \brief abortSdoTransf_Req - request the remote service abort domain transfer
*-- \brief abortSdoTransf_Req - Anforderung eines Abort Domain Transfers
*
*-- Der Client
*-- oder der Server der Domain
*-- versuchen den Domaintransfer wegen eines Fehlers abzubrechen.
*-- Dieser Dienst ist unbestätigt.
*-- Der Fehlercode \em errReason kann folgende Werte annehmen:
*++ Client or server of a domain try to interrupt the transmission
*++ due do a error condition.
*++ This service is un-confirmed.
*++ The error code \em errReason can have the following values:
*
* \arg \c CO_OK
* \arg \c CO_E_NONEXIST_OBJECT
* \arg \c CO_E_NONEXIST_SUBINDEX
* \arg \c CO_E_NO_READ_PERM
* \arg \c CO_E_NO_WRITE_PERM
* \arg \c CO_E_MAP
* \arg \c CO_E_DATA_LENGTH
* \arg \c CO_E_TRANS_TYPE
* \arg \c CO_E_VALUE_TO_HIGH
* \arg \c CO_E_VALUE_TO_LOW
* \arg \c CO_E_WRONG_SIZE
* \arg \c CO_E_PARA_INCOMP
* \arg \c CO_E_HARDWARE_FAULT
* \arg \c CO_E_SRD_NO_RESSOURCE
* \arg \c CO_E_SDO_CMD_SPEC_INVALID
* \arg \c CO_E_MEM
* \arg \c CO_E_SDO_INVALID_BLKSIZE
* \arg \c CO_E_SDO_INVALID_BLKCRC
* \arg \c CO_E_SDO_TIMEOUT
* \arg \c CO_E_INVALID_TRANSMODE
* \arg \c CO_E_SDO_OTHER
* \arg \c CO_E_DEVICE_STATE
*
*++ see also user manual appendix 5 - sdo abort codes
*-- siehe auch User Manual Anhang 5 - Sdo Abort Codes
*
* \retval CO_OK
*-- Erfolg
*++ Success
* \retval CO_E_NOT_EXIST
*-- Domain mit dieser Codenummer existiert nicht
*++ domain with this number does not exist
* \retval CO_E_STATE
*-- Knoten nicht im Zustand OPERATIONAL
*++ node not in state OPERATIONAL
* \internal
* input: handle, error reason
*/
RET_T abortSdoTransf_Req(
	SDO_T	*pCurSdo,
	RET_T	commonRet	/**< return error code */
	CO_COMMA_LINE_PARA_DECL
    )
{
UNSIGNED8	pData[8];	/* transmit buffer  */
UNSIGNED32	errReason;	/* error reason coded */

#ifdef CONFIG_CO_DEBUG
    BDEBUG(CO_DEBUG_SDO, "*** Abort Sdo Transfer %x\n", (UNSIGNED8)commonRet);
#endif /* CONFIG_CO_DEBUG */

#ifdef CONFIG_MULT_LINES
	CO_INTERNAL_NOT_USED(canLine);
#endif /* CONFIG_MULT_LINES */

    /* detect the error reason */
    switch (commonRet)
    {
	case CO_E_SDO_INVALID_TOGGLEBIT:/* 0x05030000 */
	    errReason = E_SDO_SERVICE | E_SDO_INCONS_PARA;
	    break;

	case CO_E_SDO_TIMEOUT:          /* 0x05040000 */
	    errReason = E_SDO_SERVICE | E_SDO_ILLEG_PARA;
	    break;

	case CO_E_SDO_CMD_SPEC_INVALID: /* 0x05040001 */
	    errReason = E_SDO_SERVICE | E_SDO_ILLEG_PARA | E_SDO_A_CMD_SPEC_INVALID;
	    break;

	case CO_E_SDO_INVALID_BLKSIZE:  /* 0x05040002 */
	    errReason = E_SDO_SERVICE | E_SDO_ILLEG_PARA | E_SDO_A_SIZE_INVALID;
	    break;

	case CO_E_SDO_INVALID_BLK_SEQ:  /* 0x05040003 */
	    errReason = E_SDO_SERVICE | E_SDO_ILLEG_PARA | E_SDO_A_SEQ_INVALID;
	    break;

	case CO_E_SDO_INVALID_BLKCRC:   /* 0x05040004 */
	    errReason = E_SDO_SERVICE | E_SDO_ILLEG_PARA | E_SDO_A_CRC_INVALID;
	    break;

	case CO_E_MEM:                  /* 0x05040005 */
	    errReason = E_SDO_SERVICE | E_SDO_ILLEG_PARA | E_SDO_A_OUT_OF_MEM;
	    break;

	case CO_E_NO_ACCESS:            /* 0x06010000 */
	    errReason = E_SDO_ACCESS | E_SDO_UNSUPP_ACCESS;
	    break;

	case CO_E_NO_READ_PERM:         /* 0x06010001 */
	    errReason = E_SDO_ACCESS | E_SDO_UNSUPP_ACCESS | E_SDO_A_NO_READ_PERM;
	    break;

	case CO_E_NO_WRITE_PERM:        /* 0x06010002 */
	    errReason = E_SDO_ACCESS | E_SDO_UNSUPP_ACCESS | E_SDO_A_NO_WRITE_PERM;
	    break;

	case CO_E_NONEXIST_OBJECT:	/* 0x06020000 */
	    errReason = E_SDO_ACCESS | E_SDO_NONEXIST_OBJECT;
	    break;

	case CO_E_MAP:			/* 0x06040041 */
	    errReason = E_SDO_ACCESS | E_PDO_MAPPING | E_SDO_A_NO_MAPPING;
	    break;

	case CO_E_DATA_LENGTH:		/* 0x06040042 */
	    errReason = E_SDO_ACCESS | E_PDO_MAPPING | E_SDO_A_PDO_LENGTH_EXCEED;
	    break;

	case CO_E_PARA_INCOMP:          /* 0x06040043 */
	    errReason = E_SDO_ACCESS | E_SDO_ILLEG_PARA	| E_SDO_A_GENERAL_PARA_INCOMP;
	    break;

	case CO_E_INTERNAL_INCOMP:      /* 0x06040047 */
	    errReason = E_SDO_ACCESS | E_SDO_ILLEG_PARA	| E_SDO_A_GENERAL_INTERNAL_INCOMP;
	    break;

	case CO_E_HARDWARE_FAULT:       /* 0x06060000 */
	    errReason = E_SDO_ACCESS | E_SDO_HARDWARE_FAULT;
	    break;

	case CO_E_WRONG_SIZE:           /* 0x06070010 */
	    errReason = E_SDO_ACCESS | E_SDO_TYPE_CONFLICT | E_SDO_A_INVALID_VAL;
	    break;

	case CO_E_SIZE_TOO_HIGH:        /* 0x06070012 */
	    errReason = E_SDO_ACCESS | E_SDO_TYPE_CONFLICT | E_SDO_A_LENGTH_TO_HIGH;
	    break;

	case CO_E_SIZE_TOO_LOW:         /* 0x06070013 */
	    errReason = E_SDO_ACCESS | E_SDO_TYPE_CONFLICT | E_SDO_A_LENGTH_TO_LOW;
	    break;

	case CO_E_NONEXIST_SUBINDEX:	/* 0x06090011 */
	    errReason = E_SDO_ACCESS | E_SDO_INCONS_OBJ_ATTR | E_SDO_A_NONEXIST_SUBINDEX;
	    break;

	case CO_E_TRANS_TYPE:		/* 0x06090030 */
	    errReason = E_SDO_ACCESS | E_SDO_INCONS_OBJ_ATTR | E_SDO_A_VALUE_RANGE_EXCEED;
	    break;

	case CO_E_VALUE_TO_HIGH:        /* 0x06090031 */
	    errReason = E_SDO_ACCESS | E_SDO_INCONS_OBJ_ATTR | E_SDO_A_VALUE_TO_HIGH;
	    break;

	case CO_E_VALUE_TO_LOW:         /* 0x06090032 */
	    errReason = E_SDO_ACCESS | E_SDO_INCONS_OBJ_ATTR | E_SDO_A_VALUE_TO_LOW;
	    break;

	case CO_E_LIMIT_ORDER:          /* 0x06090036 */
	    errReason = E_SDO_ACCESS | E_SDO_INCONS_OBJ_ATTR | E_SDO_A_MAX_LESS_MIN;
	    break;

	case CO_E_SRD_NO_RESSOURCE:     /* 0x060A0023 */
	    errReason = E_SDO_ACCESS | E_SDO_RES_NOT_AVAIL | E_SDO_A_SDO_CONN;
	    break;

	case CO_E_SDO_OTHER:            /* 0x08000000 */
	    errReason = E_SDO_OTHER;
	    break;

	case CO_E_INVALID_TRANSMODE:    /* 0x08000020 */
	    errReason = E_SDO_OTHER | E_SDO_A_INVALID_TRANSMODE;
	    break;

	case CO_E_LOCAL_CONTROL:        /* 0x08000021 */
	    errReason = E_SDO_OTHER | E_SDO_A_UNDER_LOCAL_CONTROL;
	    break;

	case CO_E_DEVICE_STATE:         /* 0x08000022 */
	    errReason = E_SDO_OTHER | E_SDO_A_WRONG_STATE;
	    break;

	case CO_E_DICTIONARY:           /* 0x08000023 */
	    errReason = E_SDO_OTHER | E_SDO_A_DICTIONARY_ERROR;
	    break;

	case CO_E_NO_DATA_AVAILABLE:    /* 0x08000024 */
	    errReason = E_SDO_OTHER | E_SDO_A_NO_DATA_AVAILABLE;
	    break;

        /* for static check: add all possible values */
        /* these could be extended if they get standard definition,
           but for now they are library internal and undefined on
           the CANbus */
        case CO_OK:
        case CO_E_NOT_EXIST:
        case CO_E_ALREADY_EXIST:
        case CO_E_STATE:
        case CO_E_TYPE:
        case CO_E_INHIBITED:
        case CO_E_NO_INITIATE:
        case CO_E_BUSY:
        case CO_E_ERROR_LENGTH:
        case CO_E_NO_NETWORK:
        case CO_E_RANGE:
        case CO_E_NAME_LENGTH:
        case CO_E_NAME_SYNTAX:
        case CO_E_NO_DATABASE:
        case CO_E_DISABLED:
        case CO_E_SYNTAX:
        case CO_E_SYNTAX_D_TYPE:
        case CO_E_SYNTAX_E_TYPE:
        case CO_E_BAD_ERROR_CTRL:
        case CO_E_BAD_CRC:
        case CO_E_BAD_SERVICE:
        case CO_E_CAN_TRANS_BUF:
        case CO_E_CAN_TRANS_ERROR:
        case CO_E_CAN_TRANS_TOUT:
        case CO_E_CAN_TRANS_TYPE:
        case CO_E_UNKNOWN_NODE:
        case CO_E_NO_MASTER:
        case CO_E_BAD_NODEID:
        case CO_E_BAD_TIMEVAL:
        case CO_MSG1:
        case CO_MSG2:
        case CO_SDO_IND_BUSY:
	default:
	    errReason = E_SDO_OTHER;    /* 0x08000000 */
    }

    CMS_SdoEncode(pCurSdo->index, pCurSdo->subIndex, pData);
    pData[0] = CS_ABORT_TRANSFER;

    /* fill in the reason bytes */
    CO_UNPACK_MEMCPY(&pData[4], (UNSIGNED8*)&errReason, 4, CO_NUM_VAL);

    /* no return value evaluation ! */
# ifdef CONFIG_REDUNDANCY_SUPPORT
  /*  pCurSdo->commLine = GL_VAR(co_redcyActiveLine); */ /* communication line */
    GL_VAR(co_redcySdoLine) = pCurSdo->commLine;
# endif /* CONFIG_REDUNDANCY_SUPPORT */
    (void)TRANSMIT_COB(pCurSdo->pTrCOB, pData);

    pCurSdo->state = SDOSTATE_READY;


# ifdef CONFIG_SDO_SERVER_ABORT_IND
    if (pCurSdo->userType == SERVER)  {
	sdoServerAbortInd(pCurSdo->index, pCurSdo->subIndex, errReason
		CO_COMMA_LINE_PARA);
    }
# endif /* CONFIG_SDO_SERVER_ABORT_IND */

    return(commonRet);
}
#endif /* defined(CONFIG_SDO_SERVER) || defined(CONFIG_SDO_CLIENT) */


#ifdef CONFIG_CO_DEBUG
/*****************************************************************
*
* printSdoState - debug print actual sdo state
*
* \internal
*
* RETURNS
* .TP
* nothing
*
*/
void printSdoState(
	char *fctName,
	SDO_T *pSdo
    )
{
    /* BDEBUG(CO_DEBUG_SDO, "%s: state = ", fctName); */
    BDEBUG(CO_DEBUG_SDO, "state = ");
    switch (pSdo->state)  {
	case SDOSTATE_DISABLED:
	    BDEBUG(CO_DEBUG_SDO, "disabled");
	    break;
	case SDOSTATE_READY:
	    BDEBUG(CO_DEBUG_SDO, "ready");
	    break;
	case SDOSTATE_DNLD_INIT:
	    BDEBUG(CO_DEBUG_SDO, "init download");
	    break;
	case SDOSTATE_DNLD_SEG:
	    BDEBUG(CO_DEBUG_SDO, "download segment");
	    break;
	case SDOSTATE_DNLD_BLK_INIT:
	    BDEBUG(CO_DEBUG_SDO, "init block download");
	    break;
	case SDOSTATE_DNLD_BLK_SEG:
	    BDEBUG(CO_DEBUG_SDO, "download block segment");
	    break;
	case SDOSTATE_DNLD_BLK_END:
	    BDEBUG(CO_DEBUG_SDO, "download block end");
	    break;
	case SDOSTATE_UPLD_BLK_INIT:
	    BDEBUG(CO_DEBUG_SDO, "init block upload");
	    break;
	case SDOSTATE_UPLD_BLK_SEG:
	    BDEBUG(CO_DEBUG_SDO, "upload block segment");
	    break;
	case SDOSTATE_UPLD_BLK_END:
	    BDEBUG(CO_DEBUG_SDO, "upload block end");
	    break;
	default:
	    BDEBUG(CO_DEBUG_SDO, "unknwon state 0x%x", pSdo->state);
    }
    BDEBUG(CO_DEBUG_SDO, "\n");
}
# endif /* CONFIG_CO_DEBUG */
#endif /* defined(CONFIG_SDO_SERVER) || defined(CONFIG_SDO_CLIENT) */
/*______________________________________________________________________EOF_*/
