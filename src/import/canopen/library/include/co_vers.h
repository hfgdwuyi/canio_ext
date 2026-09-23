/*
 * version - CANopen library version information
 *
 * Copyright (c) 2003-2017 port GmbH Halle/Saale
 *------------------------------------------------------------------
 */

/**
*  \file co_vers.h
*++ CANopen library version information
*-- Versionsinformation der CANopen library
*  \author port GmbH Halle (Saale)
*
*++ The file contains version information for the actual library.
*-- Dieses File enthält die Versions-Information der aktuellen Library.
*
*/

#ifndef __CO_VERS_H
# define __CO_VERS_H

# define CO_LIBRARY_VERSION	4.5
# define CO_LIBRARY_PATCH	14

# ifdef CO_CONFIG_VERSION_STRING

#  define CO_CREATE_STRING(value) PCO_CREATE_STRING(value)
#  define PCO_CREATE_STRING(value) #value

CO_LIB_CONST_VAR UNSIGNED8 coLibVerStr[] = CO_CREATE_STRING(CO_LIBRARY_VERSION)"."CO_CREATE_STRING(CO_LIBRARY_PATCH);

# endif /* CO_CONFIG_VERSION_STRING */

#endif		/*  __CO_VERS_H */
/* end of source */
