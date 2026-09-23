/*
 * co_lss - public defines for lss usage
 *
 * Copyright (c) 2002-2017 port GmbH Halle/Saale
 *------------------------------------------------------------------
 */

/**
*  \file co_lss.h
*++ Defines for lss usage
*-- Definitionen für die Verwendung des LSS-Dienstes
*  \author port GmbH Halle (Saale)
*
*++ The file contains definitions of structures and data types for lss usage
*-- Diese Datei enthält Definitionen von Strukturen und Datentypen
*-- zur Verwendung des LSS-Dienstes
*/

#ifndef __CO_LSS_H
# define __CO_LSS_H

#include <co_def.h>		/* include canopen definition */

#define LSS_MASTER		1
#define LSS_SLAVE		2

/* types for lssSlaveInd() */
#define LSS_IND_NODEID		1	/* LSS indication Mode Node id */
#define LSS_IND_STORE		2	/* LSS indication Mode store config */
#define LSS_IND_BITRATE		3	/* LSS indication Mode new bitrate */
#define LSS_IND_BITRATE_SWITCH	4	/* LSS indication Mode switch received*/
#define LSS_IND_BITRATE_SET	5	/* LSS indication Mode set bitrate */
#define LSS_IND_BITRATE_ACTIVE	6	/* LSS indication Mode bitrate activated*/
#define LSS_IND_MODE_CFG	7	/* LSS indication config mode entered */
#define LSS_IND_MODE_WAITING	8	/* LSS indication waiting mode entered*/

/* types for lssMasterCon() */
#define LSS_CON_TIMEOUT		1	/* LSS confirmation time out */
#define LSS_CON_ANSWER_OK	2	/* LSS confirmation answer ok */
#define LSS_CON_ANSWER_ERROR	3	/* LSS confirmation answer error */
#define LSS_CON_ANSWER_DATA	4	/* LSS confirmation answer with data */
#define LSS_CON_ANSWER_NODEID	5	/* LSS confirmation answer node id */
#define LSS_CON_UNCONFIG_NODE	6	/* LSS confirmation unconfigured node */
#define LSS_CON_FAST_SCAN_NO_NODE 7	/* LSS confirmation fastscan no nodes*/
#define LSS_CON_FAST_SCAN_DATA	8	/* LSS confirmation fastscan node data*/

/* inquiry modes */
#define LSS_VENDOR		1	/* inquiry mode vendor */
#define LSS_PRODUCT		2	/* inquiry mode product */
#define LSS_REVISION		3	/* inquiry mode revision */
#define LSS_SNR			4	/* inquiry mode serial  number */
#define LSS_NODEID		5	/* inquiry mode node id */

/* defines for switch mode global */
#define LSS_SWITCH_MODE_WAIT	0	/* switch mode global waiting */
#define LSS_SWITCH_MODE_CFG	1	/* switch mode global config */
#define LSS_SWITCH_MODE_WAIT_C	2	/* switch to wait, if cfg not entered */


/* lss slave answer error codes */
#define LSS_ERROR_OK		0	/* set node id: succesful */
#define LSS_ERROR_NODEID_RANGE	1	/* set node id: out of range */
#define LSS_ERROR_NODEID_SPEC	255	/* set node id: specific error code */
#define LSS_ERROR_STORE_SUPPORT	1	/* store: not supported */
#define LSS_ERROR_STORE_MEDIA	2	/* store: media error */
#define LSS_ERROR_STORE_SPEC	255	/* store: specific error code */
#define LSS_ERROR_BITTIME_SUPPORT 1	/* bittime: not supported */
#define LSS_ERROR_BITTIME_SPEC	255	/* bittime: specific error code */


/* lss slave state for getState function */
#define CO_LSS_STATE_NONE       0u      /* lss slave state machine has no valid state */
#define CO_LSS_STATE_WAIT       1u      /* lss slave state machine is in state waiting */
#define CO_LSS_STATE_CFG        2u      /* lss slave state machine is in state config */

/* external data declarations */

#endif		/*  __CO_LSS_H */


/* function prototypes */


#ifdef CONFIG_WITHOUT_PROTOTYPES
#else /* CONFIG_WITHOUT_PROTOTYPES */

# ifndef __CO_LSS_PROTOTYPES_H
#  define __CO_LSS_PROTOTYPES_H


RET_T defineLss(UNSIGNED8  kind CO_COMMA_LINE_PARA_DECL);
RET_T setLssMode(UNSIGNED8  kind CO_COMMA_LINE_PARA_DECL);

# ifdef CONFIG_LSS_MASTER
RET_T writeLssSwitchModeReq(UNSIGNED32  vendor, UNSIGNED32  product,
	UNSIGNED32 revision, UNSIGNED32 snr, UNSIGNED8 mode
	CO_COMMA_LINE_PARA_DECL);
RET_T writeLssConfigNodeIdReq(UNSIGNED8 nodeId CO_COMMA_LINE_PARA_DECL);
RET_T writeLssInquiryReq(UNSIGNED8 mode CO_COMMA_LINE_PARA_DECL);
RET_T writeLssStoreReq(CO_LINE_PARA_DECL);
RET_T writeLssIdentNonCfgReq(CO_LINE_PARA_DECL);
RET_T writeLssIdentityReq(UNSIGNED32 vendor, UNSIGNED32 product,
	UNSIGNED32 revision_low, UNSIGNED32 revision_high,
	UNSIGNED32 snr_low, UNSIGNED32 snr_high CO_COMMA_LINE_PARA_DECL);
RET_T writeLssConfigBitrateReq(UNSIGNED8   table, UNSIGNED8   index
	CO_COMMA_LINE_PARA_DECL);
RET_T writeLssActivateBitrateReq(UNSIGNED16 switchDelay CO_COMMA_LINE_PARA_DECL);
RET_T writeLssFastScanReq(UNSIGNED32 vendor, UNSIGNED32 product,
	UNSIGNED32 revision, UNSIGNED32 snr CO_COMMA_LINE_PARA_DECL);

RET_T lssCheckState(CO_LINE_PARA_DECL);
void lssMasterCon(UNSIGNED8 mode, UNSIGNED8 *par1, UNSIGNED8 *par2
	CO_COMMA_LINE_PARA_DECL);
# endif /* CONFIG_LSS_MASTER */

# ifdef CONFIG_LSS_SLAVE
UNSIGNED8 lssSlaveInd(UNSIGNED8 mode, UNSIGNED8 para1, UNSIGNED8 para2
	CO_COMMA_LINE_PARA_DECL);
RET_T	writeLssNonConfigSlaveReq(CO_LINE_PARA_DECL);
UNSIGNED8 lssGetLocalSlaveState(CO_LINE_PARA_DECL);
# endif /* CONFIG_LSS_SLAVE */

# endif /* __CO_LSS_PROTOTYPES_H */
#endif /* CONFIG_WITHOUT_PROTOTYPES */



/* end of source */
