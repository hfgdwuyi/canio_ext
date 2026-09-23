/*
 * cfg_man - defines for configuration manager usage
 *
 * Copyright (c) 2014-2017 port GmbH Halle/Saale
 *------------------------------------------------------------------
 */

/*
DESCRIPTION

The file contains definitions of structures and data types for configuratin manager usage

*/

#include <co_stru.h>
#include <co_cfgman.h>


#ifndef PCO_CFG_MAN_H__
# define PCO_CFG_MAN_H__

typedef struct {
	UNSIGNED8	*addr;		/* pointer to consize config */
	UNSIGNED32	size;		/* byte size of consize config */
	UNSIGNED32	cnt;		/* number of configuration entries */
	UNSIGNED8	node;		/* node number */
	FLAG_T		flags;		/* check and update */
} CFG_MANAGER_T;


/* flags */
#define CFGMAN_FLAG_CHK_UPD	1	/* check and update */
#define CFGMAN_FLAG_DATE	2	/* send configuration date */
#define CFGMAN_FLAG_TIME	4	/* send configuration time */

/* external data declarations */


#endif		/*  PCO_CFG_MAN_H__ */


#ifdef CONFIG_WITHOUT_PROTOTYPES
#else /* CONFIG_WITHOUT_PROTOTYPES */

# ifndef PCO_CFG_MAN__PROTOTYPES_H__
#  define PCO_CFG_MAN__PROTOTYPES_H__

/* function prototypes */


# endif /* PCO_CFG_MAN__PROTOTYPES_H__ */

#endif /* CONFIG_WITHOUT_PROTOTYPES */


/* end of source */

