/*
 * nmt_s - defines for slave nmt services
 *
 * Copyright (c) 2001-2017 port GmbH Halle/Saale
 *------------------------------------------------------------------
 */

/*
DESCRIPTION

The file contains definitions of structures and complex data types
for nmt slave services

*/

#ifndef PCO_NMT_S_H__
# define PCO_NMT_S_H__


/* Parameter for resetObjDir */
#define CO_RESTORE_COMM_FLAG	1	/* restore comm. part */
#define CO_RESTORE_APPL_FLAG	2	/* restore application part */
#define CO_RESTORE_ALL_FLAG	(CO_RESTORE_COMM_FLAG | CO_RESTORE_APPL_FLAG)

/* external variable declarations */


/* function prototypes */

void 	resetCommMsg(CO_REDCY_PARA_DECL);
void 	resetNodeMsg(CO_REDCY_PARA_DECL);


#endif /* PCO_NMT_S_H__ */
