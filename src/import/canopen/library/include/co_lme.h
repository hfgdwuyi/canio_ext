/*
 * co_lme - defines for lme usage
 *
 * Copyright (c) 2001-2017 port GmbH Halle/Saale
 *------------------------------------------------------------------
 */

/**
*  \file co_lme.h
*++ Defines public CANopen functions
*-- Definitionen öffentlicher CANopen Funktionen
*  \author port GmbH Halle (Saale)
*
*++ The file contains public CANopen functions.
*-- Diese Datei enthält Definitionen öffentlicher CANopen Funktionen.
*/

#ifndef __CO_LME_H
# define __CO_LME_H

#include <co_def.h>		/* include canopen definition */


/* external data declarations */


/* function prototypes */

RET_T	initCANopen(CO_GLOBVARS_PARA_DECL);
void 	leaveCANopen(CO_GLOBVARS_PARA_DECL);


#endif		/*  __CO_LME_H */

/* end of source */

