/*
 * co_emcy - public defines for emergnecy usage
 *
 * Copyright (c) 2001-2017 port GmbH Halle/Saale
 *------------------------------------------------------------------
 */

/**
*  \file co_emcy.h
*++ Defines for emergency usage
*-- Definitionen für die Verwendung des Emergency-Dienstes
*  \author port GmbH Halle (Saale)
*
*++ The file contains public definitions of structures and data types for
*++ emergency (EMCY) usage.
*-- Diese Datei enthält Definitionen von Strukturen und Datentypen
*-- zur Verwendung des Emergency-Dienstes (EMCY).
*/

#ifndef __CO_EMCY_H
# define __CO_EMCY_H

# include <co_def.h>		/* include canopen definition */
# include <co_cobid.h>

/* Emergency object DS 301 Version 4.0 */

typedef struct
{
    UNSIGNED16	errCode;	/**< error code */
    UNSIGNED8	errReg;		/**< error register */
    UNSIGNED8	manu[5];	/**< manufacturer specific */
} EMERGENCY_T;


/* Bitmask for enable/disable emcy */
#define EMCY_NOT_VALID_BIT	0x80000000UL

/* emergency error codes */
#define ERRCODE_COMM_ERROR	0x8100	/* communication error */
#define ERRCODE_CAN_OVERRUN	0x8110	/* CAN overrun */
#define ERRCODE_CAN_PASSIVE	0x8120	/* CAN in error passive */
#define ERRCODE_HB_ERROR	0x8130	/* HB or life guard error */
#define ERRCODE_CAN_RECOVER_BOFF 0x8140	/* CAN recoverd from bus-off */
#define ERRCODE_BAD_PDOPARA	0x8210	/* PDO not processed due the length */
#define ERRCODE_BAD_PDOLEN	0x8220	/* PDO length exceeded */

/* external data declarations */

#endif		/*  __CO_EMCY_H */



#ifdef CONFIG_WITHOUT_PROTOTYPES
#else /* CONFIG_WITHOUT_PROTOTYPES */

# ifndef __CO_EMCY_PROTOTYPES_H
#  define __CO_EMCY_PROTOTYPES_H

/* function prototypes */

RET_T 	defineEmcy(CO_USER_T CO_COMMA_LINE_PARA_DECL);
RET_T	setEmcyConsumerCobId(UNSIGNED8	nodeId, UNSIGNED32 cobId
		CO_COMMA_LINE_PARA_DECL);
RET_T 	writeEmcyReq(UNSIGNED16, UNSIGNED8 * CO_COMMA_LINE_PARA_DECL);
RET_T 	eraseErr(UNSIGNED16 CO_COMMA_LINE_PARA_DECL);
void 	emcyInd(UNSIGNED8, EMERGENCY_T * CO_COMMA_LINE_PARA_DECL);

# endif /* __CO_EMCY_PROTOTYPES_H */
#endif /* CONFIG_WITHOUT_PROTOTYPES */
/* end of source */

