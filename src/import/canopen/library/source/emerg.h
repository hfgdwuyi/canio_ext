/*
 * emerg - defines for emergency services
 *
 * Copyright (c) 2001-2017 port GmbH Halle/Saale
 *------------------------------------------------------------------
 */

/*
DESCRIPTION

The file contains definitions of structures and complex data types
for emergency services

*/

#include <co_stru.h>
#include <co_emcy.h>
#include "timer.h"


#ifndef PCO_EMERG_H__
# define PCO_EMERG_H__



struct EMCY_CONS {
	COB_T		*pCOB;		/* COB */
	UNSIGNED8	nodeId;		/* number of emergency  1..128 */
	FLAG_T   	flags;		/* EMCY-flags, EMCY disabled */
};

struct EMCY_PROD {
	COB_T		*pCOB;		/* COB for Request/Response */
	struct INHIBIT_EVENT inhibit;	/* inhibit structure */
	UNSIGNED16  	wInhibitTime;	/* inhibit time, unit: 100us */
	FLAG_T   	flags;		/* PDO-flags, PDO disabled, RTR, */
};

typedef struct EMCY_CONS EMCY_CONS_T;
typedef struct EMCY_PROD EMCY_T;

#define EMCYFLAG_DEFINED	1	/* emcy defined */
#define EMCYFLAG_ENABLED	2	/* emcy enabled */
#define EMCYFLAG_CONS_OV	4	/* consumer ov entries available */


/* external variable declarations */
#  ifdef CONFIG_DYN_MEM_ALLOC
extern UNSIGNED8		emcyConsLineCnts[];
extern UNSIGNED16		emcyConsLineOffs[];
extern EMCY_CONS_T		*p_emcyConsList[];
extern UNSIGNED16		co_maxEmcyConsCnt;
extern UNSIGNED8		*p_emcyConsIdxList[];
extern UNSIGNED8		*p_emcyConsCobIdxList[];
#  else /* CONFIG_DYN_MEM_ALLOC */
extern CO_CONST UNSIGNED8	emcyConsLineCnts[];
extern UNSIGNED16		emcyConsLineOffs[];
extern EMCY_CONS_T		emcyConsList[];
#  endif /* CONFIG_DYN_MEM_ALLOC */

#endif /* PCO_EMERG_H__ */


#ifdef CONFIG_WITHOUT_PROTOTYPES
#else /* CONFIG_WITHOUT_PROTOTYPES */

# ifndef __EMERG_PROTOTYPES_H
#  define __EMERG_PROTOTYPES_H

/* function prototypes */

void	emcyMsgReceived(CAN_MSG_T *canMsg CO_COMMA_REDCY_PARA_DECL);
void	leaveEmcyCons(CO_LINE_PARA_DECL);
RET_T	setEmcyCobId(UNSIGNED32 *pCobid CO_COMMA_LINE_PARA_DECL);
RET_T	setEmcyInhibit(UNSIGNED16 *pInhibit CO_COMMA_LINE_PARA_DECL);
void	initEmcyVars(CO_LINE_PARA_DECL);
RET_T	initEmcy(CO_USER_T kindOfUse CO_COMMA_LINE_PARA_DECL);
RET_T	checkErrorFieldAccess(UNSIGNED8	subIndex CO_COMMA_LINE_PARA_DECL);

# endif /* __EMERG_PROTOTYPES_H */
#endif /* CONFIG_WITHOUT_PROTOTYPES */


/* end of source */

