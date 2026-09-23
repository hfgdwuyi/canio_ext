/*
 * utility - public defines for utility
 *
 * Copyright (c) 2001-2017 port GmbH Halle/Saale
 *------------------------------------------------------------------
 */

/**
*  \file co_util.h
*++ Defines for utility functions
*-- Definitionen für utility Funktionen
*  \author port GmbH Halle (Saale)
*
* The file contains definitions of structures and data types for utilities
*
*/

#ifndef __CO_UTIL_H
# define __CO_UTIL_H

# include <co_type.h>


/* external data declarations */

/* function prototypes */

RET_T	setCobId(UNSIGNED16 index, UNSIGNED8 subIndex, UNSIGNED32 cobId	CO_COMMA_LINE_PARA_DECL);
RET_T   coCheckRestrictedCobId(UNSIGNED16 index,UNSIGNED32 newCobId CO_COMMA_LINE_PARA_DECL);
UNSIGNED32 waitForSdoRes(UNSIGNED8 sdoNr CO_COMMA_LINE_PARA_DECL);

void		coWait(UNSIGNED32 waitingTime CO_COMMA_GLOBVARS_PARA_DECL);
UNSIGNED8	crc8Calc(UNSIGNED8 *, UNSIGNED8, UNSIGNED32);

#ifdef CONFIG_16BIT_CPU
void		pack_memcpy(UNSIGNED8 *dest, UNSIGNED8 *src, UNSIGNED32 size, UNSIGNED8);
void		unpack_memcpy(UNSIGNED8 *dest, UNSIGNED8 *src, UNSIGNED32 size,	UNSIGNED8);
UNSIGNED16	crc16Calc(UNSIGNED8 *, UNSIGNED16, UNSIGNED32, BOOL_T);
#else /* CONFIG_16BIT_CPU */
UNSIGNED16	crc16Calc(UNSIGNED8 *, UNSIGNED16, UNSIGNED32);
#endif /* CONFIG_16BIT_CPU */

#endif		/*  __CO_UTIL_H */
/* end of source */
