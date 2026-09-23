/*
 * co_usr - declarations for public functions at usr_301.c
 *
 * Copyright (c) 2001-2017 port GmbH Halle/Saale
 *------------------------------------------------------------------
 */

/**
*  \file co_usr.h
*++ Declarations for public functions at usr_301.c
*-- Funktionsdeklarationen für die Funktionen in usr_301.c
*  \author port GmbH Halle (Saale)
*
*++ The file contains definitions of structures and complex data types
*++ for the public indication functions
*-- Diese Datei enthält Funktionsdeklarationen für die öffentlichen
*-- Indikationsfunktionen.
*/

#ifndef __CO_USR
# define __CO_USR


# include <co_def.h>		/* include canopen definition */
# include <co_stru.h>

# define CO_RESET_OBJ_DIR_IND_APPL 0x01u

/* external variable declarations */

/* function prototypes */
UNSIGNED8 	getNodeId(CO_LINE_PARA_DECL);
BOOL_T 		canErrorInd(UNSIGNED8 CO_COMMA_REDCY_PARA_DECL);
RET_T		coResetObjDirInd(UNSIGNED8 reason CO_COMMA_LINE_PARA_DECL );

# ifdef CO_CONFIG_USER_MESSAGE_TEST
RET_T           coUserMessageTestInd(CAN_MSG_T* canMsg CO_COMMA_REDCY_PARA_DECL);
# endif /* CO_CONFIG_USER_MESSAGE_TEST */

#if defined(CO_CONFIG_WRONG_MSG_IND_HBC) || defined(CO_CONFIG_WRONG_MSG_IND_NMT)
RET_T           coProtocolErrorInd(UNSIGNED16 eType, CAN_MSG_T *canMsg CO_COMMA_REDCY_PARA_DECL );
#endif /* defined(CO_CONFIG_WRONG_MSG_IND_HBC) || defined(CO_CONFIG_WRONG_MSG_IND_NMT) */

#endif /* __CO_USR */

/* end of source */
