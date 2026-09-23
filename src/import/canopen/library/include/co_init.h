/*
 * co_init - defines for initializing of the CANopen library
 *
 * Copyright (c) 2004-2017 port GmbH Halle/Saale
 *------------------------------------------------------------------
 */



/**
* \file co_init.h
*++ Defines public functions for initialization of the CANopen Library
*-- Definitionen öffentlicher Funktionen für die CANopen Initialisierung
* \author port GmbH, Halle Saale
*  $Revision: 2.11 $
*  $Date: 2016/09/26 11:15:30 $
*
*++ This file contains defines for the initializing of the CANopen Library.
*-- Dieses File enthält Defines für die Initialisierung der CANopen Library.
*/

#ifndef __CO_INIT_H
# define __CO_INIT_H
# include <co_def.h>

RET_T	init_Library(CO_GLOBVARS_PARA_DECL);
RET_T	deinit_Library(CO_GLOBVARS_PARA_DECL);
RET_T	overwritePreDefConnSet(CO_GLOBVARS_PARA_DECL);

#endif		/* __CO_INIT_H */
