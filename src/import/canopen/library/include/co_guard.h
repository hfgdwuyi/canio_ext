/*
 * co_guard - public defines for node guarding
 *
 * Copyright (c) 2001-2017 port GmbH Halle/Saale
 *------------------------------------------------------------------
 */

/**
*  \file co_guard.h
*++ Defines for node guarding usage
*-- Definitionen für die Verwendung des Nodeguarding-Dienstes
*  \author port GmbH Halle (Saale)
*
*++ The file contains definitions of structures and data types for
*++ node guarding usage
*-- Diese Datei enthält Definitionen von Strukturen und Datentypen
*-- zur Verwendung des Nodeguarding-Dienstes
*/

#ifndef __CO_GUARD_H
# define __CO_GUARD_H


# include <co_def.h>		/* include canopen definition */


/* external data declarations */

/* function prototypes */

RET_T 	startNodeGuardReq(UNSIGNED8 CO_COMMA_LINE_PARA_DECL);
RET_T 	stopNodeGuardReq(UNSIGNED8 CO_COMMA_LINE_PARA_DECL);
RET_T 	setGuardTimePara(UNSIGNED8, UNSIGNED16, UNSIGNED8
		CO_COMMA_LINE_PARA_DECL);
RET_T	addGuardingSlave(UNSIGNED8 nodeId, UNSIGNED16 guardTime, UNSIGNED8
		CO_COMMA_LINE_PARA_DECL);


#endif		/*  __CO_GUARD_H */

/* end of source */

