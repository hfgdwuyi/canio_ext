/*
 * utility - defines for utility
 *
 * Copyright (c) 2001-2017 port GmbH Halle/Saale
 *------------------------------------------------------------------
 */

/*
DESCRIPTION

The file contains definitions of structures and data types for

*/

#ifndef PCO_UTILITY_H__
# define PCO_UTILITY_H__

# include <co_util.h>


/* external data declarations */

/* function prototypes */

void sortNodeIdList(
	UNSIGNED8	pIdxList[],	/* pointer to index list */
	UNSIGNED8	*pNodeId,	/* pointer to first node at plist */
	UNSIGNED16	elementSize,	/* size of one element */
	UNSIGNED8	listLen		/* actual list len */
);
void sortCobIdList(
	UNSIGNED8	pIdxList[],	/* pointer to index list */
	COB_T		**ppCob,		/* pointer to first COB at pList */
	UNSIGNED16	elementSize,	/* size of one element */
	UNSIGNED8	listLen		/* actual list len */
);

# ifdef CONFIG_16BIT_CPU
UNSIGNED8 *unpack_oddmemcpy(UNSIGNED8 *dest, UNSIGNED8 *src, UNSIGNED32 size, BOOL_T *odd);
UNSIGNED8 *pack_oddmemcpy(UNSIGNED8 *dest, UNSIGNED8 *src, UNSIGNED32 size, BOOL_T *odd);
# endif /* CONFIG_16BIT_CPU */

#endif		/*  PCO_UTILITY_H__ */

/* end of source */

