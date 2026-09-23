/*
 * pdo - public defines for pdo usage
 *
 * Copyright (c) 2001-2017 port GmbH Halle/Saale
 *------------------------------------------------------------------
 */

/**
*  \file co_pdo.h
*++ Defines for PDO usage
*-- Definitionen für die Verwendung des PDO-Dienstes
*  \author port GmbH Halle (Saale)
*
*++ The file contains definitions of structures and data types for PDO usage
*-- Diese Datei enthält Definitionen von Strukturen und Datentypen
*-- zur Verwendung des PDO-Dienstes
#include "pdo.h"

*/

#ifndef __CO_PDO_H
# define __CO_PDO_H

#include <co_def.h>		/* include canopen definition */
#include <co_cobid.h>		/* include cobid definition */

#define TRANSMIT_PDO 		0u
#define RECEIVE_PDO  		1u

#define PDO_NO_RTR_ALLOWED_BIT  0x40000000UL
#define PDO_NO_VALID_BIT        0x80000000UL

#define PDO_LEN_TO_SHORT	1	/* PDO length to short */
#define PDO_LEN_TO_LONG		2	/* PDO length to long */

/* FLAG defines for sendPdoInd */
#define SEND_PDO_IND_RTR        0x01u
#define SEND_PDO_IND_TIME       0x02u
#define SEND_PDO_IND_SYNC       0x04u
#define SEND_PDO_IND_ACYC       0x08u



/* variants of PDO parameter structure in order to save RAM */

typedef struct {
	UNSIGNED8   numOfEntries;          /* number of entries in record */
	UNSIGNED32  cobId;                 /* COB-ID */
	UNSIGNED8   transType;             /* transmission type */
} PDO_COMM_PAR2_T;

typedef struct {
	UNSIGNED8   numOfEntries;          /* number of entries in record */
	UNSIGNED32  cobId;                 /* COB-ID */
	UNSIGNED8   transType;             /* transmission type */
	UNSIGNED16  inhibitTime;           /* inhibit time */
} PDO_COMM_PAR3_T;

typedef struct {
	UNSIGNED8   numOfEntries;          /* number of entries in record */
	UNSIGNED32  cobId;                 /* COB-ID */
	UNSIGNED8   transType;             /* transmission type */
	UNSIGNED16  inhibitTime;           /* inhibit time */
	UNSIGNED8   cmsPriorityGroup;      /* Compatibility Entry */
} PDO_COMM_PAR4_T;

typedef struct {
	UNSIGNED8   numOfEntries;          /* number of entries in record */
	UNSIGNED32  cobId;                 /* COB-ID */
	UNSIGNED8   transType;             /* transmission type */
	UNSIGNED16  inhibitTime;           /* inhibit time */
	UNSIGNED8   cmsPriorityGroup;      /* Compatibility Entry */
	UNSIGNED16  eventTimer;		   /* Event Timer */
} PDO_COMM_PAR5_T;

typedef struct {
	UNSIGNED8   numOfEntries;          /* number of entries in record */
	UNSIGNED32  cobId;                 /* COB-ID */
	UNSIGNED8   transType;             /* transmission type */
	UNSIGNED16  inhibitTime;           /* inhibit time */
	UNSIGNED8   cmsPriorityGroup;      /* Compatibility Entry */
	UNSIGNED16  eventTimer;		   /* Event Timer */
	UNSIGNED8   syncStartVal;	   /* Sync start value */
} PDO_COMM_PAR6_T;

/* external data declarations */

#endif		/*  __CO_PDO_H */

/* function prototypes */

#ifdef CONFIG_WITHOUT_PROTOTYPES
#else /* CONFIG_WITHOUT_PROTOTYPES */

# ifndef __CO_PDO_PROTOTYPES_H
#  define __CO_PDO_PROTOTYPES_H

RET_T 	definePdo(UNSIGNED8, UNSIGNED16, BOOL_T CO_COMMA_LINE_PARA_DECL);
RET_T 	writePdoReq(UNSIGNED16 CO_COMMA_LINE_PARA_DECL);
RET_T	updatePdoReq(UNSIGNED16 CO_COMMA_LINE_PARA_DECL);
RET_T 	readPdoReq(UNSIGNED16 CO_COMMA_LINE_PARA_DECL);
void 	*getMapObjAddr(UNSIGNED16, UNSIGNED8 CO_COMMA_LINE_PARA_DECL);
void	*getPdoMapObjAddr(UNSIGNED8  kind, UNSIGNED16 pdoNr, UNSIGNED8  mapNr
	    CO_COMMA_LINE_PARA_DECL);
BOOL_T	checkPdoInhibitTime(UNSIGNED16 pdoNr CO_COMMA_LINE_PARA_DECL);

void 	pdoInd(UNSIGNED16 CO_COMMA_LINE_PARA_DECL);
void 	pdoTimerInd(UNSIGNED16 CO_COMMA_LINE_PARA_DECL);
void	pdoEventTimerInd(UNSIGNED16 CO_COMMA_LINE_PARA_DECL);
void	rtrPdoInd(UNSIGNED16 CO_COMMA_LINE_PARA_DECL);
RET_T 	pdoLenInd(UNSIGNED16 pdoNr, UNSIGNED8 info CO_COMMA_LINE_PARA_DECL);
void	delSyncShadowBuffer(CO_LINE_PARA_DECL);
UNSIGNED8 *getPdoDataPtr(CO_LINE_PARA_DECL);

#ifdef CO_CONFIG_PDO_SEND_IND
# ifdef CONFIG_PDO_PRODUCER
void sendPdoInd(UNSIGNED16 pdoNr, UNSIGNED8 reason, RET_T returnCode
	CO_COMMA_LINE_PARA_DECL);
# endif /*CONFIG_PDO_PRODUCER*/
#endif /*CO_CONFIG_PDO_SEND_IND*/

#ifdef CONFIG_VIRTUAL_OBJECTS_PDO
# ifdef CONFIG_PDO_PRODUCER
RET_T coUserVirtualTpdoInd(UNSIGNED16 pdoNr,UNSIGNED8* pBuf CO_COMMA_LINE_PARA_DECL);
# endif /*CONFIG_PDO_PRODUCER*/
# ifdef CONFIG_PDO_CONSUMER
RET_T coUserVirtualRpdoInd(UNSIGNED16 pdoNr, UNSIGNED8* pBuf CO_COMMA_LINE_PARA_DECL);
# endif /*CONFIG_PDO_CONSUMER*/
#endif /* CONFIG_VIRTUAL_OBJECTS_PDO */


# endif /* __CO_PDO_PROTOTYPES_H */
#endif /* CONFIG_WITHOUT_PROTOTYPES */


/* end of source */
