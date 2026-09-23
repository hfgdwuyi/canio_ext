/*
 * sdomain - defines for internal sdo usage
 *
 * Copyright (c) 2001-2017 port GmbH Halle/Saale
 *------------------------------------------------------------------
 */

/*
DESCRIPTION

The file contains definitions of structures and data types for sdo usage

*/

#ifndef PCO_SDOMAIN_H__
# define PCO_SDOMAIN_H__


#include "sdo.h"


/* function prototypes */
void	sdoMsgReceived(CAN_MSG_T *canMsg CO_COMMA_REDCY_PARA_DECL);
void	printSdoState(char *fctName, SDO_T *pSdo);


#endif		/*  PCO_SDOMAIN_H__ */

/* end of source */

