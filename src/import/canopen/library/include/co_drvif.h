/*
 * co_drvif - declarations for public driver interface between lib and driver
 *
 * Copyright (c) 2002-2017 port GmbH Halle/Saale
 *------------------------------------------------------------------
 */

/**
*  \file co_drvif.h
*++ Defines for public driver interface between lib and driver
*-- Definitionen für die öffentliche API zwischen Treiber und Bibliothek
*  \author port GmbH Halle (Saale)
*
*++ The file contains definitions of structures and complex data types
*++ for the driver interface between library and driver.
*++ It is normally not designed for user applications
*++ and shouldn't be used by them.
*-- Diese Datei enthält Definitionen von Strukturen und Datentypen
*-- die die Schnittstelle zwischen Treiber und CANopen Bibliothek
*-- darstellen.
*-- Normalerweise wird diese Datei nicht von Anwendungen benutzt.
*/

#ifndef __CO_DRVIF_H
# define __CO_DRVIF_H

#include <co_drv.h>

# ifndef FLAG_IDENTIFICATION
#   define FLAG_IDENTIFICATION	flagIdentification
# endif /* FLAG_IDENTIFICATION */

# ifdef MSG_IDENTIFICATION
# else /* MSG_IDENTIFICATION */
#   define MSG_IDENTIFICATION(par1)  msgIdentification(par1 CO_COMMA_REDCY_PARA)
# endif /* MSG_IDENTIFICATION */


/* external variable declarations */



extern volatile UNSIGNED8	coTimerTicks CO_LINE_PARA_ARRAY_DEF;	/* CANopen timer ticks */

/* function prototypes */

void	msgIdentification(CAN_MSG_T *canMsg CO_COMMA_REDCY_PARA_DECL);
void	flagIdentification(CO_REDCY_PARA_DECL);
RET_T	Transmit_COB(COB_T *, UNSIGNED8 * CO_COMMA_GLOBVARS_PARA_DECL);
RET_T	Set_COB_ID(COB_T *, UNSIGNED32, COB_KIND_T CO_COMMA_GLOBVARS_PARA_DECL);
COB_T	*Define_COB(COB_KIND_T, UNSIGNED8 CO_COMMA_REDCY_PARA_DECL);

#endif /* __CO_DRVIF_H */

/* end of source */
