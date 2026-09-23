/*
 * co_cfg_man - public defines for config manager usage
 *
 * Copyright (c) 2014-2017 port GmbH Halle/Saale
 *------------------------------------------------------------------
 */

/**
*  \file co_cfgman.h
*++ Defines for Configuration manager usage
*-- Definitionen für die Verwendung des Configuration Manager
*  \author port GmbH Halle (Saale)
*
*++ The file contains definitions of structures and data types for Configuration manager usage
*-- Diese Datei enthält Definitionen von Strukturen und Datentypen
*-- zur Verwendung des Configuration Manager

*/

#ifndef __CO_CFGMAN_H
# define __CO_CFGMAN_H

#include <co_def.h>		/* include canopen definition */


/* convert modi */
#define DCF_CONVERT	1	/* standard convert */
#define DCF_MAPPING	2	/* actualize mapping counters */
#define DCF_PDOCOB	3	/* update PDO cobs */


#define CFG_MANAGER_OK		 	0	/* configuration ok */
#define CFG_MANAGER_DATA_MISSING 	1	/* not enough data available */
#define CFG_MANAGER_SDO_ERROR	 	2	/* sdo error */
#define CFG_MANAGER_SDO_TIMEOUT 	3	/* sdo timeout */
#define CFG_MANAGER_SDO_ABORT	 	4	/* sdo abort answer */
#define CFG_MANAGER_SDO_ABORT_CFG_INFO	5	/* sdo abort config info*/
#define	CFG_MANAGER_DATE_CHECK_FAIL	6	/* date check failed */
#define	CFG_MANAGER_TIME_CHECK_FAIL	7	/* time check failed */
#define CFG_MANAGER_START_UPDATE_FAIL	8	/* start update failed */
#define CFG_MANAGER_START_UPDATE	9	/* start update */
#define CFG_MANAGER_WRITE_CFG_DATE_FAIL 10	/* write config date/time fail*/
#define	CFG_MANAGER_CFG_OK		11	/* configuration ok */

#define DCFCONVERT_OK			0	/* concise convert ok */
#define DCFCONVERT_BRACKET_ERROR	1	/* bracket error */
#define DCFCONVERT_BAD_INDEX		2	/* invalid index */
#define DCFCONVERT_BAD_TYPE		3	/* invalid var type or value */
#define DCFCONVERT_BUF_TO_SMALL		4	/* concise buffer to small */

/* external data declarations */

#endif		/*  __CO_CFGMAN_H */

/* function prototypes */

#ifdef CONFIG_WITHOUT_PROTOTYPES
#else /* CONFIG_WITHOUT_PROTOTYPES */

# ifndef __CO_CFGMAN_PROTOTYPES_H
#  define __CO_CFGMAN_PROTOTYPES_H

UNSIGNED8 convertToConciseDcf(char *pDcfData, UNSIGNED32 *pDcfLen,
	    UNSIGNED32 *pDcfOffs, char *pData, UNSIGNED32 dataLen,
	    UNSIGNED8  mode);
BOOL_T	cfgManagerInd(UNSIGNED8, UNSIGNED8 CO_COMMA_LINE_PARA_DECL);
RET_T	updateRemoteNodeConfig(UNSIGNED8 nodeId, UNSIGNED8 sdoNr
	    CO_COMMA_LINE_PARA_DECL);
RET_T	handleRemoteNodeConfig(UNSIGNED8 nodeId, UNSIGNED8 sdoNr
	    CO_COMMA_LINE_PARA_DECL);
RET_T	checkRemoteNodeConfig(UNSIGNED8 nodeId, UNSIGNED8 sdoNr
	    CO_COMMA_LINE_PARA_DECL);

# endif /* __CO_CFGMAN_PROTOTYPES_H */
#endif /* CONFIG_WITHOUT_PROTOTYPES */


/* end of source */
