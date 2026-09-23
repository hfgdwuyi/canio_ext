/*
 * co_dynmem - public defines for dynmic memory usage
 *
 * Copyright (c) 2014-2017 port GmbH Halle/Saale
 *------------------------------------------------------------------
 */

/**
*  \file co_dynmem.h
*++ Defines for dynamic memory usage
*-- Definitionen für dynamische Speicher Nutzung
*  \author port GmbH Halle (Saale)
*
*++ The file contains the definition for dynamic memory usage
*/

#ifndef __CO_DYNMEM_H
# define __CO_DYNMEM_H


RET_T initDynamicServices(
	UNSIGNED8	sdoServCnt,	/**< number of server sdos (max 127) */
	UNSIGNED8	sdoClientCnt,	/**< number of client sdos (max 127) */
	UNSIGNED16	pdoConsCnt,	/**< number of PDO Consumer (max 512) */
	UNSIGNED16	pdoProdCnt,	/**< number of PDO Producer (max 512) */
	UNSIGNED16	mapCnt,		/**< number of mappings */
	UNSIGNED8	hbCons,		/**< number of heartbeat consumers */
	UNSIGNED8	emcyCons,	/**< number of emergency consumers */
	UNSIGNED8	nmtSlaveCnt,	/**< number of NMT slaves */
	UNSIGNED8	nmtGuardCnt	/**< number of Guarding slaves */
	CO_COMMA_LINE_PARA_DECL
);

#endif /* __CO_DYNMEM_H */

/* end of source */
