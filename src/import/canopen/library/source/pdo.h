/*
 * pdo - defines for pdo usage
 *
 * Copyright (c) 2001-2017 port GmbH Halle/Saale
 *------------------------------------------------------------------
 */

/*
DESCRIPTION

The file contains definitions of structures and data types for pdo usage

*/

#ifndef PCO_PDO_H__
# define PCO_PDO_H__

#include <co_stru.h>
#include <co_pdo.h>
#include <co_acces.h>
#include "timer.h"

/* mapping object structure */
struct PDO_MAP {
        UNSIGNED8       *pAddress;      /* pointer to data */
        BASIC_DATA_T    eBasicType;     /* basic data type */
        UNSIGNED8       bBitSize;       /* length in bits */
# ifdef CO_CONFIG_ENABLE_OBJ_CALLBACK
        CO_OBJ_CB_T     *ppObjCallback; /* pointer to obj function pointer */
        UNSIGNED16      cbServiceNum;   /* the pdo number */
        UNSIGNED16      objIndex;       /* index of the object */
        UNSIGNED8       objSubindex;    /* subindex of the object */
# endif /* CO_CONFIG_ENABLE_OBJ_CALLBACK */
};

struct PDO {
# ifdef CONFIG_PDO_EVENTTIMER
        TIMER_EVENT_T   timer;          /* timer event structure */
# endif /* CONFIG_PDO_EVENTTIMER */
        INHIBIT_EVENT_T inhibit;        /* inhibit structure */
        COB_T           *pCOB;          /* COB for Request/Response */
        UNSIGNED16      mapStartIdx;    /* start index at mapping table */
        UNSIGNED8       actMapCnt;      /* actual mapping count */
        UNSIGNED8       maxMapCnt;      /* max. mapping count */
        UNSIGNED16      pdoNr;          /* number of PDO 1..512 */
        UNSIGNED16      wInhibitTime;   /* inhibit time, unit: 100us */
        UNSIGNED8       transType;      /* transmission type */
# if defined(CONFIG_SYNC_PRODUCER) || defined(CONFIG_SYNC_CONSUMER)
        FLAG_T          syncFlags;      /* PDO SYNC Flags */
        UNSIGNED8       curCount;       /* is to decrement with every SYNC */
        UNSIGNED8       shadowData[8];  /* Shadow CAN Data Buffer */
# endif /* defined(CONFIG_SYNC_PRODUCER) || defined(CONFIG_SYNC_CONSUMER) */
# ifdef CONFIG_PDO_SYNC_START_VALUE
        UNSIGNED8       syncStartValue; /* sync start value */
# endif /* CONFIG_PDO_SYNC_START_VALUE */
        FLAG_T          flags;          /* PDO-flags, PDO disabled, RTR, */
# if defined(CONFIG_MPDO_DEST) || defined(CONFIG_MPDO_SRC)
        FLAG_T          mpdoFlags;      /* flags for mpdo modes */
# endif
# ifdef CONFIG_VIRTUAL_OBJECTS
        FLAG_T          virtualObjFlags;
# endif /* CONFIG_VIRTUAL_OBJECTS */
# ifdef CO_CONFIG_PDO_INHIBITTIME_RESEND
        FLAG_T          inhibitFlags;
# endif /* CO_CONFIG_PDO_INHIBITTIME_RESEND */
};

typedef struct PDO PDO_T;
typedef struct PDO_MAP PDO_MAP_T;

/* defines for PDO flags */
#define PDOFLAG_DISABLED	1u		/* PDO disabled */
#define PDOFLAG_MAP_DISABLED	2u		/* Mapping disabled */
#define PDOFLAG_SYNC_POSSIBLE	4u		/* SYNC possible */
#define PDOFLAG_SYNC		8u		/* SYNC active */
#define PDOFLAG_CYCLIC		0x10u		/* cyclic pdo */
#define PDOFLAG_ONLY_RTR	0x20u
#define PDOFLAG_TOTRANSMIT	0x40u
#define PDOFLAG_TOUPDATE	0x80u

#define PDOSYNCFLAG_ENABLED	0x40		/* sync counter enabled */
#define PDOSYNCFLAG_SYNCSTART	0x80		/* sync start active */
/* #define PDOFLAG_OUTSTANDING	0x80 */

#define PDOFLAG_VIRTUAL_OBJ_FALSE 0x00u
#define PDOFLAG_VIRTUAL_OBJ_TRUE  0x01u

#define PDO_INHIBIT_FLAG_RETRANSMIT 1u          /* TPDO should be automatically transmitted after it was inhibited */

#define MAP_INDEX_SHIFT         16
#define MAP_INDEX_MASK          0x0000FFFFUL
#define MAP_SUBINDEX_SHIFT      8
#define MAP_SUBINDEX_MASK       0x000000FFUL
#define MAP_LENGTH_MASK 	0x000000FFUL

/* external data declarations */
extern PDO_T		co_trPdo[];
extern PDO_T		co_recPdo[];
extern INTEGER16	co_trPdoCnt CO_LINE_PARA_ARRAY_DEF;
extern INTEGER16	co_recPdoCnt CO_LINE_PARA_ARRAY_DEF;
extern PDO_MAP_T	co_mappingTable[];

# ifdef CONFIG_DYN_MEM_ALLOC
extern PDO_T		*p_co_trPdo[];
extern PDO_T		*p_co_recPdo[];
extern UNSIGNED16	co_pdoConsCnt;
extern UNSIGNED16	co_pdoProdCnt;
extern PDO_MAP_T	*p_co_mappingTable[1];
extern UNSIGNED16	co_maxMappingCnt;
#  ifdef CONFIG_MULT_LINES
extern INTEGER16	co_trPdoLineCnts[];
extern INTEGER16	co_recPdoLineCnts[];
# endif /* CONFIG_MULT_LINES */
#  ifdef CONFIG_PDO_FAST_SORT
extern UNSIGNED16	*p_co_recPdoNrIdxList[1];
extern UNSIGNED16	*p_co_recPdoCobIdxList[1];
extern UNSIGNED16	*p_co_trPdoNrIdxList[1];
extern UNSIGNED16	*p_co_trPdoCobIdxList[1];
#  endif /* CONFIG_PDO_FAST_SORT */
# endif /* CONFIG_DYN_MEM_ALLOC */

# ifdef CONFIG_MULT_LINES
extern UNSIGNED16	co_trPdoLineOffs CO_LINE_PARA_ARRAY_DEF;
extern UNSIGNED16	co_recPdoLineOffs CO_LINE_PARA_ARRAY_DEF;
# endif /* CONFIG_MULT_LINES */


#ifdef CONFIG_WITHOUT_PROTOTYPES
#else /* CONFIG_WITHOUT_PROTOTYPES */

# ifndef PCO_PDO_PROTOTYPES_H__
#  define PCO_PDO_PROTOTYPES_H__

/* function prototypes */

void	pdoMsgReceived(CAN_MSG_T *canMsg CO_COMMA_REDCY_PARA_DECL);
void	pdoRtrMsgReceived(CAN_MSG_T *canMsg CO_COMMA_LINE_PARA_DECL);
RET_T	prepareTransPdo(PDO_T *pPdo, UNSIGNED8 *pBuf CO_COMMA_LINE_PARA_DECL);
PDO_T 	*pdoExist(UNSIGNED16 num, UNSIGNED8 pdoType CO_COMMA_LINE_PARA_DECL);
void	transSyncPdo(CO_LINE_PARA_DECL);
void	updateSyncRpdo(CO_REDCY_PARA_DECL);
RET_T	updateSyncTpdo(CO_LINE_PARA_DECL);
void	eventTransPdo(TIMER_EVENT_T *pTimer CO_COMMA_LINE_PARA_DECL);
void	eventRecPdo(TIMER_EVENT_T *pTimer CO_COMMA_LINE_PARA_DECL);
RET_T	checkMappingTable(PDO_T *pPdo, UNSIGNED16 index, UNSIGNED8 kind CO_COMMA_LINE_PARA_DECL);
RET_T	checkMappingEntry(UNSIGNED32 newMapEntry, UNSIGNED8 kind CO_COMMA_LINE_PARA_DECL);
void	resetAllPdos(CO_LINE_PARA_DECL);
void	initPdoVars(CO_LINE_PARA_DECL);
void	updatePdoSyncStartValues(CO_LINE_PARA_DECL);
RET_T	setPdoCommPara(UNSIGNED16 index, UNSIGNED8 subIndex,
	    UNSIGNED8 *pData, UNSIGNED8 kind CO_COMMA_LINE_PARA_DECL);

# endif /* PCO_PDO_PROTOTYPES_H__ */
#endif /* CONFIG_WITHOUT_PROTOTYPES */

#endif		/*  PCO_PDO_H__ */

/* end of source */

