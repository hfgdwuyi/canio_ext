/*
 * co_splus - public defines for slave plus
 *
 * Copyright (c) 2001-2017 port GmbH Halle/Saale
 *------------------------------------------------------------------
 */

/**
*  \file co_splus.h
*++ Defines for slaves with NMT master capabilities
*-- Definitionen für CANopen-Slaves mit NMT-Masterfunktionalität
*  \author port GmbH Halle (Saale)
*
*++ The file contains definitions of structures and data types for
*++ slaves with nmt master capabilities and heartbeat consumer properties
*-- Diese Datei enthält Definitionen von Strukturen und Datentypen
*-- für CANopen-Slaves mit NMT-Masterfunktionalität
*/

#ifndef __CO_SLAVE_PLUS_H
# define __CO_SLAVE_PLUS_H
# include <co_def.h>		/* include canopen definition */

/* external data declarations */

/* function prototypes */

RET_T	newRemoteStateReq(NODE_STATE_T newState CO_COMMA_REDCY_PARA_DECL);
RET_T	newLocalStateReq(NODE_STATE_T newState CO_COMMA_REDCY_PARA_DECL);

#endif		/*  __CO_SLAVE_PLUS_H */

/*______________________________________________________________________EOF_*/
