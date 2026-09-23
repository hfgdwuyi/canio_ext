/*
 * cmscodec - modul for cms decoder and encoder functions
 *
 * Copyright (c) 1996-2017 port GmbH Halle (Saale)
 *------------------------------------------------------------------
 */


/*
* \file cmscodec.c
* CAN based message specification (CMS) decoder and encoder functions
* \author port GmbH, Halle
*
* This modul contains functions for encoding and coding of CMS objects.
* The function CMS_Encode transforms implementation dependent data structures
* into a transfer syntax corresponding the encoding rules
* defined by CiA in CiA/DS202-3 p. 2.
* CMS_Decode works in the opposite direction. It transforms
* a received message back into the defined data structure.
*
* the data types are stored as followed.
*
* \code
* - BOOLEAN         unsigned char (Bit 0 byte aligned)
* - INTEGER(1..8)   char (byte aligned)
* - INTEGER(9..16)  short (word aligned)
* - INTEGER(17..32) long (word aligned)
* - INTEGER(33..64) two long (Word Aligned, first LOW double Word)
* - UNSIGNED(x..y)  like INTEGER(x..y) with signed instead unsigned
* - FLOAT	    like INTEGER(32) (not specified yet)
* - DUMMY_SPACE     not stored, but contains information length
* - NIL             not stored
* \endcode
*
* The compiler directive CONFIG_BIT_ENCODING enables/disables bitewise
* or bytewise encoding/decoding
* bytewise encoding/decoding has a better performance
* (code space) and run time behaviour.
*
* All of these functions are only called from within the library
* and not from the library user.
* Therefore there are no manual entries of the functions available.
*
*/


/* header of standard C - libraries */

#include <string.h>
#include <stdio.h>

/* header of project specific types */

#include <cal_conf.h>

#include <co_mcpy.h>
#include <co_def.h>
#include "cmscodec.h"
#include "pdo.h"
#include "access.h"

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
CO_LIB_INIT_VAR static char _rcsid[] = "$Id: cmscodec.c,v 2.31 2016/06/23 16:42:51 rli Exp $";
#endif /* CONFIG_RCS_IDENT */



#ifdef CONFIG_PDO_PRODUCER
/*******************************************************************
*
* CMS_MapEncode - encode CMS objects for Mapping
*
* NOMANUAL
*
* CMS_MapEncode transforms pdo mapping data
* into a transfer syntax corresponding the encoding rules
* defined by CiA in CiA/DS202-3 p. 2.
*
* \retval
* telegram length
*
*/

UNSIGNED8 CMS_MapEncode(
	PDO_MAP_T   *pMapList,	/* pointer to mapping list for PDOs */
	UNSIGNED8   *pData,	/* pointer to CAN message buffer */
	UNSIGNED8   cnt		/* mapping count */
	CO_COMMA_LINE_PARA_DECL	/* number of CAN line 0..CO_MAX_CAN_LINES-1 */
    )
{
# ifdef CONFIG_BIT_ENCODING
UNSIGNED8	destBits = 0;	/* count of destination bits */
UNSIGNED8	bitSize,	/* actual bitsize */
		destShift,	/* count of shifted bits */
		destByte,	/* destination byte counter */
		srcByte;	/* source byte counter */
UNSIGNED8	tmpBuf[8]={0};	/* unshifted value */
UNSIGNED8	tmpU8 = 0u;
# else /* CONFIG_BIT_ENCODING */
UNSIGNED8	mapLen = 0;	/* mapping length */
# endif /* CONFIG_BIT_ENCODING */
UNSIGNED8	actLen;		/* length (in bytes) of mapping data */
# ifdef CO_CONFIG_ENABLE_OBJ_CALLBACK
CO_OBJ_CB_TYPE_T callBackReason;
# endif /* CO_CONFIG_ENABLE_OBJ_CALLBACK */

# ifndef CO_CONFIG_ENABLE_OBJ_CALLBACK
#  if defined(CONFIG_MULT_LINES) || defined(CONFIG_NO_GLOBAL_VARS)
CO_INTERNAL_NOT_USED(CO_LINE_PARA);
#  endif /* CONFIG_MULT_LINES || CONFIG_NO_GLOBAL_VARS */
# endif /* CO_CONFIG_ENABLE_OBJ_CALLBACK */


/* until all mappings are processed */
    while (cnt > 0)
    {

        /* abort for invalid entries */
        if (pMapList->eBasicType == CO_INVALID)
        {
            break;
        }

# ifdef CONFIG_BIT_ENCODING
        /* length of actual mapping entry */
        bitSize = pMapList->bBitSize;
# endif /* CONFIG_BIT_ENCODING */
        /* set actual len of mapping entry */
        actLen = pMapList->bBitSize >> 3;
        if ((pMapList->bBitSize % 8) != 0)
        {
            actLen++;
        }

# ifdef CO_CONFIG_ENABLE_OBJ_CALLBACK
#  ifdef CO_CONFIG_OBJ_CB_PRE_PDO_READ
        if ( pMapList->ppObjCallback != NULL )
        {
            if( *(*(pMapList->ppObjCallback)) != NULL )
            {
                callBackReason.serviceNbr = pMapList->cbServiceNum;
                callBackReason.reason = CO_OBJ_CB_TYPE_PRE_PDO_READ;
#   ifdef CO_CONFIG_ENABLE_EXTOBJ_CALLBACK
                callBackReason.objAccess.pData = pMapList->pAddress;
                callBackReason.objAccess.dataSize = actLen;
#   endif /* CO_CONFIG_ENABLE_EXTOBJ_CALLBACK */

                (void)((CO_OBJ_CB_T)(*(pMapList->ppObjCallback)))( pMapList->objIndex,
#   ifdef CONFIG_NO_GLOBAL_VARS
                    pMapList->objSubindex, callBackReason ,(void*)CO_LINE_PARA
#   else /* CONFIG_NO_GLOBAL_VARS */
                    pMapList->objSubindex, callBackReason CO_COMMA_LINE_PARA
#   endif /* CONFIG_NO_GLOBAL_VARS */
                    );
            }
        }
#  endif /* CO_CONFIG_OBJ_CB_POST_PRE_READ */
# endif /* CO_CONFIG_ENABLE_OBJ_CALLBACK */

# ifdef CONFIG_BIT_ENCODING
        /* determine alignment */
        if (((bitSize % 8) == 0) && ((destBits % 8) == 0))
        {
            /* copy bytewise */
            if ( NULL != pMapList->pAddress )
            {
#  ifdef CONFIG_16BIT_CPU
                if (pMapList->eBasicType == CO_INTEGER)
                {
                    CO_UNPACK_MEMCPY(&pData[destBits >> 3],
                            (UNSIGNED8 *)pMapList->pAddress,
                            actLen,
                            CO_8BIT_SIGNED_VAL);
                }
                else
#  endif /* CONFIG_16BIT_CPU */
                {
                    CO_UNPACK_MEMCPY(&pData[destBits >> 3],
                            (UNSIGNED8 *)pMapList->pAddress,
                            actLen,
                            (pMapList->eBasicType == CO_UNSIGNED));
                }
            }

            destBits += bitSize;
        }
        else
        {
            /* copy data to temp buffer as bytes */
            if (NULL != pMapList->pAddress)
            {
#  ifdef CONFIG_16BIT_CPU
                if (pMapList->eBasicType == CO_INTEGER)
                {
                    CO_UNPACK_MEMCPY(&tmpBuf[0],
                                    (UNSIGNED8 *)pMapList->pAddress,
                                    actLen,
                                    CO_8BIT_SIGNED_VAL);
		}
                else
#  endif /* CONFIG_16BIT_CPU */
                {
                    CO_UNPACK_MEMCPY(&tmpBuf[0],
                                    (UNSIGNED8 *)pMapList->pAddress,
                                    actLen,
                                    (pMapList->eBasicType == CO_UNSIGNED));
		}
	    }

	    srcByte = 0;

            while (bitSize > 0)
            {
#  ifdef CONFIG_VIRTUAL_OBJECTS_PDO
                UNSIGNED8 bitMask = 0xFF;
#  endif /* CONFIG_VIRTUAL_OBJECTS_PDO */
                /* process low part */

                /* how many bits are shifted to left */
		destShift = destBits & 7;
		destByte = destBits >> 3;

		/* make local copy */
		tmpU8 = tmpBuf[srcByte];
		/* shift left (low part of this byte) */
		tmpU8 <<= destShift;
		/* put into can msg buffer, but only if obj is not virtual */
		if (NULL != pMapList->pAddress)
                {
#  ifdef CONFIG_VIRTUAL_OBJECTS_PDO
                    if (bitSize < 8u)
                    {
			bitMask >>= (8 - bitSize);
		    }

                    bitMask <<= destShift;
                    pData[destByte] &= (~bitMask);
#  endif /* CONFIG_VIRTUAL_OBJECTS_PDO */
                    pData[destByte] |= tmpU8;
                }

		/* shift right (high part of this byte) */
		/* but only, are there more bits available */
		if (bitSize > (8 - destShift))
                {
		    tmpU8 = tmpBuf[srcByte];
		    tmpU8 >>= (8 - destShift);

                    /* put into can msg buffer, but only if obj is not virtual */
                    if (NULL != pMapList->pAddress)
                    {
#  ifdef CONFIG_VIRTUAL_OBJECTS_PDO
                        bitMask = 0xFF;
                        bitMask >>= (8 - destShift);
                        pData[destByte + 1] &= (~bitMask);
#  endif /* CONFIG_VIRTUAL_OBJECTS_PDO */
                        pData[destByte + 1] |= tmpU8;
                    }
                }

		if (bitSize > 8)
                {
                    bitSize -= 8;
                    destBits += 8;
		}
                else
                {
                    destBits += bitSize;
                    bitSize = 0;
                }

                srcByte++;
            }
        }
# else /* CONFIG_BIT_ENCODING */
        if ( NULL !=  pMapList->pAddress )
        {
#  ifdef CONFIG_16BIT_CPU
	    if (pMapList->eBasicType == CO_INTEGER)
            {
	        CO_UNPACK_MEMCPY(pData, (UNSIGNED8 *)pMapList->pAddress, actLen,
		    CO_8BIT_SIGNED_VAL);
            }
            else
#  endif /* CONFIG_16BIT_CPU */
            {
                /* without dummy mapping */
                CO_UNPACK_MEMCPY(pData, (UNSIGNED8 *)pMapList->pAddress, actLen,
		        (pMapList->eBasicType == CO_UNSIGNED));
            }
        }
# endif /* CONFIG_BIT_ENCODING */

# ifdef CONFIG_BIT_ENCODING
# else /* CONFIG_BIT_ENCODING */
        /* incr data pointer */
	pData += actLen;
	/* actualize mapping length */
	mapLen += actLen;
# endif /* CONFIG_BIT_ENCODING */

# ifdef CO_CONFIG_ENABLE_OBJ_CALLBACK
#  ifdef CO_CONFIG_OBJ_CB_POST_PDO_READ
        if ( pMapList->ppObjCallback != NULL )
        {
            if( *(*(pMapList->ppObjCallback)) != NULL )
            {
	        callBackReason.serviceNbr = pMapList->cbServiceNum;
		callBackReason.reason = CO_OBJ_CB_TYPE_POST_PDO_READ;
#   ifdef CO_CONFIG_ENABLE_EXTOBJ_CALLBACK
		callBackReason.objAccess.pData = pMapList->pAddress;
		callBackReason.objAccess.dataSize = actLen;
#   endif /* CO_CONFIG_ENABLE_EXTOBJ_CALLBACK */

	        (void)((CO_OBJ_CB_T)(*(pMapList->ppObjCallback)))( pMapList->objIndex,
#   ifdef CONFIG_NO_GLOBAL_VARS
			pMapList->objSubindex, callBackReason ,(void*)CO_LINE_PARA
#   else /* CONFIG_NO_GLOBAL_VARS */
			pMapList->objSubindex, callBackReason CO_COMMA_LINE_PARA
#   endif /* CONFIG_NO_GLOBAL_VARS */
			);
	    }
	}
#  endif /* CO_CONFIG_OBJ_CB_POST_PDO_READ */
# endif /* CO_CONFIG_ENABLE_OBJ_CALLBACK */
	/* get next address if mapping */
	pMapList ++;
	cnt--;
    }

# ifdef CONFIG_BIT_ENCODING
    /* calculate bytecount */
    bitSize = destBits >> 3;
    if ((destBits % 8) != 0)  {
	bitSize++;
    }
    return(bitSize);
# else /* CONFIG_BIT_ENCODING */
    return(mapLen);
# endif /* CONFIG_BIT_ENCODING */
}
#endif /* PDO_PRODUCER */


#ifdef CONFIG_PDO_CONSUMER
/*******************************************************************
*
* CMS_MapDecode - decode CMS objects for Mapping
*
* NOMANUAL
*
* CMS_MapDecode transforms a transfer syntax corresponding the encoding rules
* defined by CiA in CiA/DS202-3 p. 2
* into pdo data structures.
*
* \retval
*	necessary byte count
*
*/

UNSIGNED8 CMS_MapDecode(
	PDO_MAP_T   *pMapList,	/* pointer to mapping list for PDOs */
	UNSIGNED8   *pData,	/* pointer to CAN message buffer */
	UNSIGNED8   cnt		/* mapping count */
	CO_COMMA_LINE_PARA_DECL	/* number of CAN line 0..CO_MAX_CAN_LINES-1 */
    )
{
UNSIGNED8	actLen;		/* length in bytes of actual data item */
# ifdef CONFIG_BIT_ENCODING
UNSIGNED8	sourceBits=0u;	/* count of source bits */
UNSIGNED8	bitSize,	/* actual bitsize */
		sourceShift,	/* count of shifted bits */
		sourceByte,	/* source byte counter */
		byteCnt;	/* byte count */
UNSIGNED8	tmpBuf[8]={0u};	/* unshifted value */
UNSIGNED8	tmpU8 = 0u;
# else /* CONFIG_BIT_ENCODING */
UNSIGNED8	mapLen = 0u;	/* whole mapping length size in bytes */
# endif /* CONFIG_BIT_ENCODING */
# ifdef CO_CONFIG_ENABLE_OBJ_CALLBACK
CO_OBJ_CB_TYPE_T callBackReason;
# endif /* CO_CONFIG_ENABLE_OBJ_CALLBACK */

# ifndef CO_CONFIG_ENABLE_OBJ_CALLBACK
#  if defined(CONFIG_MULT_LINES) || defined(CONFIG_NO_GLOBAL_VARS)
    CO_INTERNAL_NOT_USED(CO_LINE_PARA);
#  endif /* CONFIG_MULT_LINES || CONFIG_NO_GLOBAL_VARS */
# endif /* CO_CONFIG_ENABLE_OBJ_CALLBACK */

    /* until all mappings are processed */
    while (cnt > 0u)
    {
	/* abort for invalid entries */
	if (pMapList->eBasicType == CO_INVALID)
        {
            break;
	}

# ifdef CONFIG_BIT_ENCODING
	/* length of actual mapping entry */
	bitSize = pMapList->bBitSize;
# endif /* CONFIG_BIT_ENCODING */
	/* set actual len of mapping entry */
	actLen = pMapList->bBitSize >> 3;
	if (actLen == 0u)
        {
	    actLen++;
        }

# ifdef CO_CONFIG_ENABLE_OBJ_CALLBACK
#  ifdef CO_CONFIG_OBJ_CB_PRE_PDO_WRITE
	if ( pMapList->ppObjCallback != NULL )
        {
	    if( *(*(pMapList->ppObjCallback)) != NULL )
            {
                callBackReason.serviceNbr = pMapList->cbServiceNum;
                callBackReason.reason = CO_OBJ_CB_TYPE_PRE_PDO_WRITE;
#   ifdef CO_CONFIG_ENABLE_EXTOBJ_CALLBACK
                callBackReason.objAccess.pData = pMapList->pAddress;
                callBackReason.objAccess.dataSize = actLen;
#   endif /* CO_CONFIG_ENABLE_EXTOBJ_CALLBACK */
	        (void)((CO_OBJ_CB_T)(*(pMapList->ppObjCallback)))( pMapList->objIndex,
#   ifdef CONFIG_NO_GLOBAL_VARS
			pMapList->objSubindex, callBackReason ,(void*)CO_LINE_PARA
#   else /* CONFIG_NO_GLOBAL_VARS */
			pMapList->objSubindex, callBackReason CO_COMMA_LINE_PARA
#   endif /* CONFIG_NO_GLOBAL_VARS */
			);
	    }
	}
#  endif /* CO_CONFIG_OBJ_CB_POST_PRE_WRITE */
# endif /* CO_CONFIG_ENABLE_OBJ_CALLBACK */


	/* ignore dummy mapping */
        if (pMapList->eBasicType != CO_DUMMY_SPACE)
        {

# ifdef CONFIG_BIT_ENCODING
            /* determine alignment */
            if (((bitSize % 8) == 0) && ((sourceBits % 8) == 0))
            {

                /* copy bytewise */
                if (NULL != pMapList->pAddress)
                {
#  ifdef CONFIG_16BIT_CPU
                    if (pMapList->eBasicType == CO_INTEGER)
                    {
                        CO_PACK_MEMCPY((UNSIGNED8 *)pMapList->pAddress,
                                        &pData[sourceBits >> 3],
                                        bitSize >> 3,
                                        CO_8BIT_SIGNED_VAL);
                    }
                    else
#  endif /* CONFIG_16BIT_CPU */
                    {
                        CO_PACK_MEMCPY((UNSIGNED8 *)pMapList->pAddress,
                                        &pData[sourceBits >> 3],
                                        bitSize >> 3,
                                        (pMapList->eBasicType == CO_UNSIGNED));
                    }
	        }
    	        sourceBits += bitSize;
            } else
            {
    	        byteCnt = 0;
                while (bitSize > 0)
                {
                    /* process low part */

                    /* how many bits are shifted to left */
                    sourceShift = sourceBits & 7;
                    sourceByte = sourceBits >> 3;

                    /* copy complete byte (contains low-part) */
                    tmpBuf[byteCnt] = pData[sourceByte];
                    /* shift to right border */
                    tmpBuf[byteCnt] >>= sourceShift;
                    /* delete not used bits */
                    /* tmpBuf[byteCnt] &= (0xff >> sourceShift); */

                    /* recalc sourcebit count */
                    if (bitSize > (8 - sourceShift))
                    {
                        sourceShift = 8 - (sourceBits & 7);
                        /* higher part is saved in next source byte */
                        tmpU8 = pData[sourceByte + 1];
                        tmpU8 <<= sourceShift;
                        tmpBuf[byteCnt] |= tmpU8;
                    }

	            if (bitSize > 7)
                    {
                        sourceBits += 8;
                        bitSize -= 8;
                    } else
                    {
                        /* mask unused bits */
                        tmpBuf[byteCnt] &= (0xff >> (8 - bitSize));
                        sourceBits += bitSize;
                        bitSize = 0;
	            }
		    byteCnt++;
	        }

                /* The pointer should only be NULL with virtual objects */
                if (NULL != pMapList->pAddress) {
#  ifdef CONFIG_16BIT_CPU
	            if (pMapList->eBasicType == CO_INTEGER)
                    {
				CO_PACK_MEMCPY((UNSIGNED8 *)pMapList->pAddress,
				                &tmpBuf[0],
                                                byteCnt,
				                CO_8BIT_SIGNED_VAL);
		    }
		    else
#  endif /* CONFIG_16BIT_CPU */
		    {
	                        CO_PACK_MEMCPY((UNSIGNED8 *)pMapList->pAddress,
                                                &tmpBuf[0],
                                                byteCnt,
				                (pMapList->eBasicType == CO_UNSIGNED));
		    }
	        }
	    }
        } else
        {
            /* dummy */
            sourceBits += bitSize;
# else /* CONFIG_BIT_ENCODING */
            /* The pointer should only be NULL with virtual objects */
            if ( NULL != pMapList->pAddress )
            {
#  ifdef CONFIG_16BIT_CPU
	        if (pMapList->eBasicType == CO_INTEGER)  {
	            CO_PACK_MEMCPY((UNSIGNED8 *)pMapList->pAddress,
                                    pData,
                                    actLen,
                                    CO_8BIT_SIGNED_VAL);
	        } else
#  endif /* CONFIG_16BIT_CPU */
	        {
	            CO_PACK_MEMCPY((UNSIGNED8 *)pMapList->pAddress,
                                    pData,
                                    actLen,
	                            (pMapList->eBasicType == CO_UNSIGNED));
	        }
            }
# endif /* CONFIG_BIT_ENCODING */
        }

# ifdef CONFIG_BIT_ENCODING
# else /* CONFIG_BIT_ENCODING */
        /* incr data pointer */
        pData += actLen;
        /* actualize mapping length */
        mapLen += actLen;
# endif /* CONFIG_BIT_ENCODING */

# ifdef CO_CONFIG_ENABLE_OBJ_CALLBACK
#  ifdef CO_CONFIG_OBJ_CB_POST_PDO_WRITE
        if ( pMapList->ppObjCallback != NULL )
        {
	    if( *(*(pMapList->ppObjCallback)) != NULL )
            {
	        callBackReason.serviceNbr = pMapList->cbServiceNum;
		callBackReason.reason = CO_OBJ_CB_TYPE_POST_PDO_WRITE;
#   ifdef CO_CONFIG_ENABLE_EXTOBJ_CALLBACK
		callBackReason.objAccess.pData = pMapList->pAddress;
		callBackReason.objAccess.dataSize = actLen;
#   endif /* CO_CONFIG_ENABLE_EXTOBJ_CALLBACK */

	        (void)((CO_OBJ_CB_T)(*(pMapList->ppObjCallback)))( pMapList->objIndex,
#   ifdef CONFIG_NO_GLOBAL_VARS
		    pMapList->objSubindex, callBackReason ,(void*)CO_LINE_PARA
#   else /* CONFIG_NO_GLOBAL_VARS */
		    pMapList->objSubindex, callBackReason CO_COMMA_LINE_PARA
#   endif /* CONFIG_NO_GLOBAL_VARS */
		    );
	    }
	}
#  endif /* CO_CONFIG_OBJ_CB_POST_PDO_WRITE */
# endif /* CO_CONFIG_ENABLE_OBJ_CALLBACK */

	/* get next address if PDO mapping */
	pMapList ++;
	cnt --;
    }

# ifdef CONFIG_BIT_ENCODING
    /* calculate bytecount */
    bitSize = sourceBits >> 3;
    if ((sourceBits % 8) != 0)  {
        bitSize++;
    }
    return(bitSize);
# else /* CONFIG_BIT_ENCODING */
    return(mapLen);
# endif /* CONFIG_BIT_ENCODING */
}
#endif /* PDO_CONSUMER */

