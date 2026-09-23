/*
 * co_sdo - public defines for sdo usage
 *
 * Copyright (c) 2001-2017 port GmbH Halle/Saale
 *------------------------------------------------------------------
 */

/**
*  \file co_sdo.h
*++ Defines for SDO usage
*-- Definitionen für die Verwendung von SDO
*  \author port GmbH Halle (Saale)
*
*++ The file contains definitions of structures and data types for SDO usage
*-- Diese Datei enthält Definitionen von Strukturen und Datentypen
*-- zur Verwendung des SDO-Dienstes
*/

#ifndef __CO_SDO_H
# define __CO_SDO_H

/* # include <co_stru.h> */
# include <co_def.h>		/* include canopen definition */
# include <co_cobid.h>		/* include cobid definition */

typedef struct {
	UNSIGNED8   numOfEntries;          /**< number of entries in record */
	UNSIGNED32  cobIdReqInd;	   /**< COB ID request or indication */
	UNSIGNED32  cobIdResCon;	   /**< COB ID response or confirmation */
	UNSIGNED8   nodeId;		   /* write permission */
} SDO_COMM_PAR_T;

#ifdef CO_CONFIG_SDO_CLIENT_CALLBACK
typedef struct
{
    UNSIGNED32 reason;   /* SDO Error Code */
    UNSIGNED32 size;     /* Size of transmission */
    UNSIGNED8  sdoNum;
}CO_SDOC_CB_TYPE_T;


typedef RET_T (*CO_SDO_C_CB_T)(UNSIGNED16, UNSIGNED8, CO_SDOC_CB_TYPE_T, void* CO_COMMA_LINE_PARA_DECL);
#endif /* CO_CONFIG_SDO_CLIENT_CALLBACK */


#define SDO_NO_VALID_BIT        0x80000000UL

/* numeric bit for SDO type length on big endian machines */
# define CO_NUM_SDO		0x80u
# define CO_NONUM_SDO		0x7Fu


/* SDO Error defines DS 301 V 4.0 */

/* error class */
#define E_SDO_TIMEOUT         	0xFFFFFFFFUL
#define E_SDO_NO_ERROR         	0x00000000UL
#define E_SDO_SERVICE          	0x05000000UL
#define E_SDO_ACCESS           	0x06000000UL
#define E_SDO_OTHER		0x08000000UL
/* error code */
#define E_SDO_UNSUPP_ACCESS    	0x00010000UL
#define E_SDO_NONEXIST_OBJECT  	0x00020000UL
#define E_SDO_INCONS_PARA      	0x00030000UL
#define E_SDO_ILLEG_PARA       	0x00040000UL
#define E_PDO_MAPPING	  	0x00040000UL
#define E_SDO_HARDWARE_FAULT   	0x00060000UL
#define E_SDO_TYPE_CONFLICT    	0x00070000UL
#define E_SDO_INCONS_OBJ_ATTR  	0x00090000UL
#define E_SDO_RES_NOT_AVAIL 	0x000a0000UL

/* additional Error Codes for SDO */
#define E_SDO_A_NO_DETAILS		0u
#define E_SDO_A_CMD_SPEC_INVALID	0x01u
#define E_SDO_A_NO_READ_PERM		0x01u
#define E_SDO_A_SIZE_INVALID		0x02u
#define E_SDO_A_NO_WRITE_PERM		0x02u
#define E_SDO_A_SEQ_INVALID		0x03u
#define E_SDO_A_CRC_INVALID		0x04u
#define E_SDO_A_OUT_OF_MEM		0x05u
#define E_SDO_A_INVALID_VAL		0x10u
#define E_SDO_A_NONEXIST_SUBINDEX	0x11u
#define E_SDO_A_LENGTH_TO_HIGH		0x12u
#define E_SDO_A_LENGTH_TO_LOW		0x13u
#define E_SDO_A_NO_EXECUTION		0x20u
#define E_SDO_A_INVALID_TRANSMODE	0x20u
#define E_SDO_A_UNDER_LOCAL_CONTROL	0x21u
#define E_SDO_A_WRONG_STATE		0x22u
#define E_SDO_A_SDO_CONN		0x23u
#define E_SDO_A_DICTIONARY_ERROR	0x23u
#define E_SDO_A_NO_DATA_AVAILABLE	0x24u
#define E_SDO_A_VALUE_RANGE_EXCEED	0x30u
#define E_SDO_A_VALUE_TO_HIGH		0x31u
#define E_SDO_A_VALUE_TO_LOW		0x32u
#define E_SDO_A_MAX_LESS_MIN		0x36u
#define E_SDO_A_INCOMP			0x40u
#define E_SDO_A_NO_MAPPING		0x41u
#define E_SDO_A_PDO_LENGTH_EXCEED	0x42u
#define E_SDO_A_GENERAL_PARA_INCOMP	0x43u
#define E_SDO_A_GENERAL_INTERNAL_INCOMP	0x47u

/* error class */
#define E_SDO_INTERNAL		0xFF000000UL
/* error code */
#define E_SDO_NO_RESSOURCES    	0x00FF0000UL /* no or not enough memory for upload   */
#define E_SDO_ZERO_ERROR        0x00FE0000UL /* abortDomainTransfer has error code 0 */

/* additional SDO confirmation values */
#define E_SDO_IN_USE 0xFFFFFFFEUL


/* external data declarations */
#endif		/*  __CO_SDO_H */


#ifdef CONFIG_WITHOUT_PROTOTYPES
#else /* CONFIG_WITHOUT_PROTOTYPES */

# ifndef __CO_SDO__PROTOTYPES_H
#  define __CO_SDO__PROTOTYPES_H

/* function prototypes */

RET_T	defineSdo(UNSIGNED8, USER_T CO_COMMA_LINE_PARA_DECL);
RET_T	writeSdoReq(UNSIGNED8, UNSIGNED16, UNSIGNED8, UNSIGNED8 *,
			UNSIGNED32, UNSIGNED32 CO_COMMA_LINE_PARA_DECL);
RET_T	writeSdoSegReq(UNSIGNED8, UNSIGNED16, UNSIGNED8, UNSIGNED8 *,
			UNSIGNED32, UNSIGNED32 CO_COMMA_LINE_PARA_DECL);
RET_T	readSdoReq(UNSIGNED8, UNSIGNED16, UNSIGNED8, UNSIGNED8 *,
				UNSIGNED32, UNSIGNED32 CO_COMMA_LINE_PARA_DECL);
RET_T	readSdoSegReq(UNSIGNED8, UNSIGNED16, UNSIGNED8, UNSIGNED8 *,
				UNSIGNED32, UNSIGNED32 CO_COMMA_LINE_PARA_DECL);
UNSIGNED32      getSdoSize(UNSIGNED8, USER_T CO_COMMA_LINE_PARA_DECL);
UNSIGNED32      co_getSdoRestSize(UNSIGNED8, USER_T CO_COMMA_LINE_PARA_DECL);
UNSIGNED8       getActualSdo(UNSIGNED16, UNSIGNED8 CO_COMMA_LINE_PARA_DECL);

#ifdef CONFIG_SPLIT_INDICATION
RET_T	sdoRdInd(UNSIGNED16, UNSIGNED8, UNSIGNED8 CO_COMMA_LINE_PARA_DECL);
RET_T	sdoWrInd(UNSIGNED16, UNSIGNED8, UNSIGNED8 CO_COMMA_LINE_PARA_DECL);
#else /* CONFIG_SPLIT_INDICATION */
RET_T	sdoRdInd(UNSIGNED16, UNSIGNED8 CO_COMMA_LINE_PARA_DECL);
RET_T	sdoWrInd(UNSIGNED16, UNSIGNED8 CO_COMMA_LINE_PARA_DECL);
#endif /* CONFIG_SPLIT_INDICATION */
#ifdef CO_CONFIG_DOMAIN_UNKNOWN_SIZE
UNSIGNED32 coUserSdoDomainSizeInd(UNSIGNED8 CO_COMMA_LINE_PARA_DECL);
#endif /* CO_CONFIG_DOMAIN_UNKNOWN_SIZE */
void	sdoRdCon(UNSIGNED8, UNSIGNED32 CO_COMMA_LINE_PARA_DECL);
void	sdoWrCon(UNSIGNED8, UNSIGNED32 CO_COMMA_LINE_PARA_DECL);
RET_T	testSdoValue(UNSIGNED16, UNSIGNED8, void *, UNSIGNED32
		CO_COMMA_LINE_PARA_DECL);

RET_T	sdoDomainInd(UNSIGNED16 index, UNSIGNED8 subIndex,
		UNSIGNED8 *pData, UNSIGNED32 actSize,
		UNSIGNED8 overSize
#ifdef CO_CONFIG_DOMAIN_INDICATION_DEFERRABLE
                , UNSIGNED8 sdoNr,
                BOOL_T    pausable
#endif /* CO_CONFIG_DOMAIN_INDICATION_DEFERRABLE */
                CO_COMMA_LINE_PARA_DECL);
RET_T	coUserSdoDomainUploadInd(UNSIGNED16 index, UNSIGNED8 subIndex,
		UNSIGNED8 *pData, UNSIGNED32 *pSize CO_COMMA_LINE_PARA_DECL);
RET_T	sdoDomainWrCon(UNSIGNED8 sdo CO_COMMA_LINE_PARA_DECL);
RET_T	sdoDomainRdCon(UNSIGNED8 sdo CO_COMMA_LINE_PARA_DECL);

RET_T	sdoBlockInd(UNSIGNED16 index, UNSIGNED8 subIndex,
		UNSIGNED32 actSize CO_COMMA_LINE_PARA_DECL);

RET_T	writeSdoDomainReq(UNSIGNED8 sdoNr, UNSIGNED16 index, UNSIGNED8 subIndex,
		UNSIGNED8 *pData, UNSIGNED32  length, UNSIGNED32 domSizeCnt,
		UNSIGNED32 timeOut CO_COMMA_LINE_PARA_DECL);
RET_T	readSdoDomainReq(UNSIGNED8 sdoNr, UNSIGNED16 index, UNSIGNED8 subIndex,
		UNSIGNED8 *pData, UNSIGNED32  length, UNSIGNED32 domSizeCnt,
		UNSIGNED32 timeOut CO_COMMA_LINE_PARA_DECL);

RET_T	finishSdoWrInd(UNSIGNED8 sdoNr, RET_T retCode CO_COMMA_LINE_PARA_DECL);
RET_T	finishSdoRdInd(UNSIGNED8 sdoNr, RET_T retCode CO_COMMA_LINE_PARA_DECL);
RET_T   finishSdoDomainInd(UNSIGNED8 sdoNr, RET_T retCode CO_COMMA_LINE_PARA_DECL);
RET_T	abortSdoReq(UNSIGNED8 sdoNr, USER_T kindOfUse, RET_T abortCode
		CO_COMMA_LINE_PARA_DECL);
void	sdoServerAbortInd(UNSIGNED16 index, UNSIGNED8 subIndex,
		UNSIGNED32 errReason CO_COMMA_LINE_PARA_DECL);

RET_T 	getClientSdoNumFromNodeId(UNSIGNED8 nodeNum, UNSIGNED8* pSdoNum
		CO_COMMA_LINE_PARA_DECL );


# endif /* __CO_SDO__PROTOTYPES_H */
#endif /* CONFIG_WITHOUT_PROTOTYPES */


/* end of source */

