/*
 * - defines for codec routines
 *
 * Copyright (c) 2001-2017 port GmbH Halle/Saale
 *------------------------------------------------------------------
 */

/*
DESCRIPTION

The file contains definitions of structures and data types for
codec functions
*/

#ifndef PCO_CMSCODEC_H__
# define PCO_CMSCODEC_H__

# include "pdo.h"

/* defines for function compiling */
   /* copy from CAN buffer to index/subindex */
#  define CMS_SdoDecode(index, subIndex, pData) 		\
	index = (UNSIGNED16)(((UNSIGNED16)pData[2]) << 8u) | pData[1];	\
	subIndex = pData[3]

   /* copy from index/subindex to CAN buffer */
#  define CMS_SdoEncode(index, subIndex, pData) 	\
	pData[1] = (UNSIGNED8)(index & 0xffu);			\
	pData[2] = (UNSIGNED8)((index >> 8u) & 0xffu);		\
	pData[3] = subIndex


/* external data declarations */

/* function prototypes */


UNSIGNED8	CMS_MapEncode (PDO_MAP_T *pMapList, UNSIGNED8 *pData, UNSIGNED8 cnt CO_COMMA_LINE_PARA_DECL);
UNSIGNED8	CMS_MapDecode (PDO_MAP_T *pMapList, UNSIGNED8 *pData, UNSIGNED8 cnt CO_COMMA_LINE_PARA_DECL);

#endif		/*  PCO_CMSCODEC_H__ */

/* end of source */

