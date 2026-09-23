/*
 * co_setcp - defines for set communication parameter
 *
 * Copyright (c) 2001-2017 port GmbH Halle/Saale
 *------------------------------------------------------------------
 */

/**
*  \file co_setcp.h
*++ Defines for set communication parameter
*-- Definitionen für das Setzen von Kommunikationsparametern
*  \author port GmbH Halle (Saale)
*
*++ The file contains definitions of structures and data types for
*++ set communication parameter
*-- Diese Datei enthält Definitionen von Strukturen und Datentypen
*-- zum aktualisieren der CANopen internen Variablen.
*/

#ifndef __CO_SETCP_H
# define __CO_SETCP_H
# include <co_def.h>		/* include canopen definition */

/* external data declarations */

/* function prototypes */
RET_T	setCommPar(UNSIGNED16, UNSIGNED8 CO_COMMA_LINE_PARA_DECL);
RET_T	resetObjDir(UNSIGNED8 CO_COMMA_LINE_PARA_DECL);
void	setDefaultParameter(UNSIGNED8 CO_COMMA_LINE_PARA_DECL);

#endif		/*  __CO_SETCP_H */
/* end of source */
