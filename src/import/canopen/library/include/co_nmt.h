/*
 * co_nmt - public defines for nmt usage
 *
 * Copyright (c) 2001-2017 port GmbH Halle/Saale
 *------------------------------------------------------------------
 */

/**
*  \file co_nmt.h
*++ Defines for NMT usage
*-- Definitionen für die NMT-Dienste
*  \author port GmbH Halle (Saale)
*
*++ The file contains definitions of structures and data types for
*++ NMT services.
*-- Diese Datei enthält Definitionen von Strukturen und Datentypen
*-- zur Verwendung des NMT-Dienstes.
*/

#ifndef __CO_NMT_H
# define __CO_NMT_H

# include <co_def.h>		/* include canopen definition */

/** Defines the legth of a valid nmt-msg */
#ifndef PCO_VALID_MESSAGE_LENGTH_NMT
# define PCO_VALID_MESSAGE_LENGTH_NMT 2
#endif /* PCO_VALID_MESSAGE_LENGTH_NMT */

#define MEM_SEG_ALL_PARAMETERS		1u
#define MEM_SEG_COM_PARAMETERS		2u
#define MEM_SEG_APPL_PARAMETERS		3u

/* external data declarations */

#endif		/*  __CO_NMT_H */


#ifdef CONFIG_WITHOUT_PROTOTYPES
#else /* CONFIG_WITHOUT_PROTOTYPES */

# ifndef __CO_NMT_PROTOTYPES_H
#  define __CO_NMT_PROTOTYPES_H

/* function prototypes */

RET_T		deleteNodeReq(CO_LINE_PARA_DECL);
UNSIGNED8 	sGuardErrorInd(ERROR_SPEC_T CO_COMMA_LINE_PARA_DECL);

#if defined(CONFIG_MASTER)
RET_T		createNodeReq(BOOL_T, BOOL_T, BOOL_T CO_COMMA_LINE_PARA_DECL);
#else /*  defined(CONFIG_MASTER) */
RET_T		createNodeReq(BOOL_T, BOOL_T CO_COMMA_LINE_PARA_DECL);
#endif /*  defined(CONFIG_MASTER) */

RET_T		setNodePREOP(CO_REDCY_PARA_DECL);
RET_T		setNodeSTOPPED(CO_REDCY_PARA_DECL);
RET_T		coSetNodeOPERATIONAL(CO_REDCY_PARA_DECL);
NODE_STATE_T	getNodeState(CO_REDCY_PARA_DECL);
BOOL_T		newStateInd(NODE_STATE_T CO_COMMA_REDCY_PARA_DECL);
#ifdef CO_CONFIG_USER_NMT_MSG_IND
RET_T coUserNmtMsgInd(UNSIGNED8 CO_COMMA_LINE_PARA_DECL);
#endif /* CO_CONFIG_USER_NMT_MSG_IND */

#ifdef CO_CONFIG_RESET_COMM_PRE_CMD
void		resetCommPreInd(CO_REDCY_PARA_DECL);
#endif /* CO_CONFIG_RESET_COMM_PRE_CMD */

#ifdef CO_CONFIG_RESET_COMM_POST_CMD
void		resetCommPostInd(CO_REDCY_PARA_DECL);
#endif /* CO_CONFIG_RESET_COMM_POST_CMD */

void 		resetCommInd(CO_REDCY_PARA_DECL);
void		resetApplPreInd(CO_REDCY_PARA_DECL);
void 		resetApplInd(CO_REDCY_PARA_DECL);
void 		mGuardErrorInd(UNSIGNED8,ERROR_SPEC_T CO_COMMA_REDCY_PARA_DECL);
NODE_STATE_T	getRemoteNodeState(UNSIGNED8  nodeNr CO_COMMA_REDCY_PARA_DECL);

#ifdef CO_CONFIG_REPORT_ANY_HB
void coUserHbReceived(UNSIGNED8, NODE_STATE_T CO_COMMA_REDCY_PARA_DECL);
#endif

# endif /* __CO_NMT_PROTOTYPES_H */
#endif /* CONFIG_WITHOUT_PROTOTYPES */




/* end of source */

