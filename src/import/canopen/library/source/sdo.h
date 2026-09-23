/*
 * sdo - defines for sdo usage
 *
 * Copyright (c) 2001-2017 port GmbH Halle/Saale
 *------------------------------------------------------------------

DESCRIPTION

The file contains definitions of structures and data types for sdo usage

*/

# include <co_stru.h>
# include <co_sdo.h>
# include <co_timer.h>
# include "timer.h"

#ifdef CONFIG_CFG_MANAGER
# include "cfg_man.h"
#endif /* CONFIG_CFG_MANAGER */


#ifndef PCO_SDO_H__
# define PCO_SDO_H__

/* structure of a CMS domain */

#ifdef CONFIG_EXTENDED_DATA_TYPES
# define MAX_SDO_SAVE_LEN	8u	/* max data length for saving value */
#else /* CONFIG_EXTENDED_DATA_TYPES */
# define MAX_SDO_SAVE_LEN	4u	/* max data length for saving value */
#endif /* CONFIG_EXTENDED_DATA_TYPES */


struct CMS_DOMAIN {
	COB_T       *pTrCOB;	     /* COB for Request/Indication */
	COB_T       *pRecCOB;	     /* COB for Response/Confirmation */
	UNSIGNED32  domSize;         /* size of whole domain */
	UNSIGNED32  restSize;        /* size of not transfered data */
	UNSIGNED32  timeOut;         /* time out value in 1/10 ms */
	UNSIGNED16  index;	     /* index */
	UNSIGNED8   subIndex;	     /* subIndex */
	USER_T      userType;        /* Client <-> Server */
	UNSIGNED8   toggleBit;       /* toggle bit */
	UNSIGNED8   *pDomData;       /* pointer to data location */
	UNSIGNED8   *pActualDomData; /* pointer to actual data position for upload */
	UNSIGNED8   num;             /* code number of domain */
	UNSIGNED8   state;	     /* actual sdo state */
	FLAG_T	    flags;	     /* additional sdo flags */
	UNSIGNED16  attr;	     /* object attributes */

#ifdef CONFIG_SDO_BLOCKTRANSFER
# ifdef CONFIG_BLOCK_CRC
	UNSIGNED16  blkCrcSum;	     /* CRC Checksum */
        UNSIGNED16  blkCrcSumAtBlkStart; /* CRC Checksum fallback for recalculation */
# endif /* CONFIG_BLOCK_CRC */
	UNSIGNED8   pData[8];	     /* data buffer */
	UNSIGNED8   blkSegSize;	     /* block segment size */
	UNSIGNED8   blkSegDefaultSize;	     /* block default segment size */
	UNSIGNED8   blkSegNr;	     /* actual block segment number */
	BOOL_T	    blkCRC;	     /* block transfer uses CRC check */
        UNSIGNED8   lastSegSize;     /* number of valid bytes in the last tranmitted block segment */
# ifdef CO_CONFIG_BLOCKTRANSFER_INHIBITED_SEND
        INHIBIT_EVENT_T inhibit;   /* inhibit structure */
        UNSIGNED16  inhibitTime;   /* inhibit time, unit: 100us */
# endif /* CO_CONFIG_BLOCKTRANSFER_INHIBITED_SEND */
#endif /* CONFIG_SDO_BLOCKTRANSFER */
	UNSIGNED8   oldVar[MAX_SDO_SAVE_LEN];/* buffer for saving former value*/

# ifdef CONFIG_DOMAIN_INDICATION_SIZE
	UNSIGNED32  lastBorder;	     /* last saved border */
	UNSIGNED32  maxBufNum;        /* number of loaded bytes into
	                              * the intermediate domain buffer */
	UNSIGNED32  actBufNum;        /* number of transmitted bytes
	                              * from the intermediate domain buffer */
	UNSIGNED32  domSizeBuffered;  /* number of loaded bytes into the
	                              * intermediate domain buffer
	                              * from the domain object */
	UNSIGNED8   overByteNum;       /* number of surplus bytes in
	                              * the intermediate domain buffer */
	UNSIGNED8   overByteBuf[7];    /* buffer for surplus bytes */
# endif /* CONFIG_DOMAIN_INDICATION_SIZE */

#ifdef CONFIG_REDUNDANCY_SUPPORT
	UNSIGNED8   commLine;		/* 	communication line */
#endif /* CONFIG_REDUNDANCY_SUPPORT */

	BOOL_T	    saved;	      /* flag signs if old value is stored */
#if defined(CONFIG_BIG_ENDIAN) || defined(CONFIG_16BIT_CPU)
	BOOL_T	    numeric;         /* flag shows numeric contents of SDO
					only for BIG_ENDIAN/16bit devices */
#endif /* defined(CONFIG_BIG_ENDIAN) || defined(CONFIG_16BIT_CPU) */
#if defined(CONFIG_16BIT_CPU)
	BOOL_T	    halfWord;	     /* half word not processed */
#endif /* defined(CONFIG_16BIT_CPU) */
#ifdef CO_CONFIG_SDO_EXPEDITED_NO_VALID_SIZE_BIT
        BOOL_T      expedited_sdo_with_valid_size_bit; /* set CO_SIZE_VALID bit in expedited transfer */
#endif /* CO_CONFIG_SDO_EXPEDITED_NO_VALID_SIZE_BIT */
        BOOL_T      lastSegment;     /* received last segment in a segmented domain transfer */
};

typedef struct CMS_DOMAIN 	SDO_T;

struct SDO_CLIENT {
	TIMER_EVENT_T timer;	/* timer structure for timeout monitoring*/
	SDO_T	    sdo;	/* sdo data */
	UNSIGNED32  timeOut;	/* time out value in 1/10 ms */
	UNSIGNED32  sdoConf;	/* SDO confirmation value */
#ifdef CONFIG_DOMAIN_CONFIRMATION
	UNSIGNED32  domainIndSize;/* domainCnt * 7 for indication */
	UNSIGNED32  nextDomainIndBorder;/* next indication border */
# ifdef CONFIG_SDO_BLOCKTRANSFER
	UNSIGNED8   *pBufferStart; /* pointer to buffer start */
# endif /* CONFIG_SDO_BLOCKTRANSFER */
#endif /* CONFIG_DOMAIN_CONFIRMATION */
	UNSIGNED8   upDnType;	/* upload = 0, download = 1 */
#ifdef CONFIG_CFG_MANAGER
	CFG_MANAGER_T	cfg;
#endif /* CONFIG_CFG_MANAGER */
};

typedef struct SDO_CLIENT 	SDO_CLIENT_T;


# define CO_SIZE_VALID	1u

/* defines for upDnType */
#define SDO_UPLOAD	0u	/* sdo client read */
#define SDO_DOWNLOAD	1u	/* sdo cleint write */

#define EXPED_TRANSFER		0x02u	/* field e is setting to 1 */


  /* C C S   =   C l i e n t   C o m m a n d   S p e c i f i e r */

#define CO_SDO_CCS_MASK		0xe0u	/* client command specifier */

#define CCS_INI_DN_LD_REQ	0x20u	/* Initiate Download Request */
#define CCS_DN_LD_SEG_REQ	0x00u	/* Download Segment Request */
#define CCS_INI_UP_LD_REQ	0x40u	/* Initiate Upload Request */
#define CCS_UP_LD_SEG_REQ	0x60u	/* Upload Segment Request */

  /* S C S   =   S e r v e r C o m m a n d   S p e c i f i e r */

#define SCS_INI_DN_LD_RES	0x60u	/* Initiate Download Response */
#define SCS_DN_LD_SEG_RES	0x20u	/* Download Segment Response */
#define SCS_INI_UP_LD_RES	0x40u	/* Initiate Upload Response */
#define SCS_UP_LD_SEG_RES	0x00u	/* Upload Segment Response */

  /* C S   =   C o m m a n d   S p e c i f i e r */

#define CS_ABORT_TRANSFER	0x80u	/* Abort Transfer Request */

#define SDO_TOGGLE_BIT		0x10u	/* SDO Toggle Bit */

#define CO_SDO_SIZE_TYPE_MASK	0x3u	/* sdo size type mask (e + s bit) */
#define CO_SDO_SCS_MASK		0xe0u	/* client command specifier mask */

#define CO_SDO_MORE		0u	/* more data for down/uploading */
#define CO_SDO_LAST		1u	/* no more data for down/uploading */

/* SDO states */
#define SDOSTATE_DISABLED	0x00u
#define SDOSTATE_IND_BUSY	0x10u
#define SDOSTATE_READY		0x20u
#define SDOSTATE_DNLD		0x80u
#define SDOSTATE_UPLD		0x40u
#define SDOSTATE_DNLD_INIT	(SDOSTATE_DNLD + 1u)
#define SDOSTATE_DNLD_SEG	(SDOSTATE_DNLD + 2u)
#define SDOSTATE_DNLD_BLK_INIT	(SDOSTATE_DNLD + 3u)
#define SDOSTATE_DNLD_BLK_SEG	(SDOSTATE_DNLD + 4u)
#define SDOSTATE_DNLD_BLK_END	(SDOSTATE_DNLD + 5u)
#define SDOSTATE_UPLD_INIT	(SDOSTATE_UPLD + 1u)
#define SDOSTATE_UPLD_SEG	(SDOSTATE_UPLD + 2u)
#define SDOSTATE_UPLD_BLK_INIT	(SDOSTATE_UPLD + 3u)
#define SDOSTATE_UPLD_BLK_SEG	(SDOSTATE_UPLD + 4u)
#define SDOSTATE_UPLD_BLK_END	(SDOSTATE_UPLD + 5u)

/* SDO flags (bitcoded) */
#define SDOFLAG_TR_COB_VALID	1u
#define SDOFLAG_REC_COB_VALID	2u
#define SDOFLAG_COBS_VALID	(SDOFLAG_TR_COB_VALID | SDOFLAG_REC_COB_VALID)

/* defines for domain indication */
#ifdef CONFIG_BOOT_LOADER
# ifndef CONFIG_DOMAIN_INDICATION_SIZE
#  define CONFIG_DOMAIN_INDICATION_SIZE		128
# endif /* CONFIG_DOMAIN_INDICATION_SIZE */
#endif /* CONFIG_BOOT_LOADER */

				/* n byte border */
#define NEXT_BORDER(val)	(((val) / CONFIG_DOMAIN_INDICATION_SIZE) \
					* CONFIG_DOMAIN_INDICATION_SIZE)
				/* following 7 border */
#define NEXT_7_BORDER(val)	((val) ? ((val) + (7 - ((val) % 7))) : 0)


/* external data declarations */
#ifdef CONFIG_DYN_SDO_CONNECTION
extern UNSIGNED8	coSdoConError	CO_LINE_PARA_ARRAY_DEF;
#endif /* CONFIG_DYN_SDO_CONNECTION */

extern INTEGER8		co_sdoServerCnt CO_LINE_PARA_ARRAY_DEF;
# ifdef CONFIG_DYN_MEM_ALLOC
extern SDO_T		*p_co_sdoServer[];
extern UNSIGNED8	*p_co_sdoServerNrList[];
extern UNSIGNED8	*p_co_sdoServerCobIdxList[];
extern SDO_CLIENT_T	*p_co_sdoClient[];
extern UNSIGNED8	*p_co_sdoClientNrList[];
extern UNSIGNED8	*p_co_sdoClientCobIdxList[];
extern UNSIGNED16	co_maxSdoServer;
extern UNSIGNED16	co_maxSdoClient;
#  ifdef CONFIG_MULT_LINES
extern UNSIGNED8	co_sdoClientLineCnts CO_LINE_PARA_ARRAY_DEF;
extern UNSIGNED8	co_sdoServerLineCnts CO_LINE_PARA_ARRAY_DEF;
#  endif /* CONFIG_MULT_LINES */
# else /* CONFIG_DYN_MEM_ALLOC */
extern SDO_T		co_sdoServer[];
extern UNSIGNED8	co_sdoServerNrList[];
extern UNSIGNED8	co_sdoServerCobIdxList[];
extern SDO_CLIENT_T	co_sdoClient[];
extern UNSIGNED8	co_sdoClientNrList[];
extern UNSIGNED8	co_sdoClientCobIdxList[];
#  ifdef CONFIG_MULT_LINES
extern CO_CONST UNSIGNED8	co_sdoClientLineCnts CO_LINE_PARA_ARRAY_DEF;
extern CO_CONST UNSIGNED8	co_sdoServerLineCnts CO_LINE_PARA_ARRAY_DEF;
#  endif /* CONFIG_MULT_LINES */
# endif /* CONFIG_DYN_MEM_ALLOC */
extern INTEGER8		co_sdoClientCnt CO_LINE_PARA_ARRAY_DEF;
# ifdef CONFIG_MULT_LINES
extern UNSIGNED16	co_sdoServerLineOffs CO_LINE_PARA_ARRAY_DEF;
extern UNSIGNED16	co_sdoClientLineOffs CO_LINE_PARA_ARRAY_DEF;
# endif /* CONFIG_MULT_LINES */

#endif		/*  PCO_SDO_H__ */


#ifdef CONFIG_WITHOUT_PROTOTYPES
#else /* CONFIG_WITHOUT_PROTOTYPES */

# ifndef PCO_SDO__PROTOTYPES_H__
#  define PCO_SDO__PROTOTYPES_H__

/* function prototypes */
void		sdoServerMsgInd(SDO_T *pSdo, CAN_MSG_T *canMsg CO_COMMA_LINE_PARA_DECL);
void		sdoClientMsgCon(SDO_CLIENT_T *pSdo, CAN_MSG_T *canMsg
			CO_COMMA_GLOBVARS_PARA_DECL);
SDO_T		*searchForServerSdoCobId(COB_IDENT_T cobId CO_COMMA_LINE_PARA_DECL);
SDO_T		*searchForServerSdoNr(UNSIGNED8	sdoNr CO_COMMA_LINE_PARA_DECL);
SDO_CLIENT_T	*searchForClientSdoCobId(COB_IDENT_T cobId CO_COMMA_LINE_PARA_DECL);
SDO_CLIENT_T	*searchForClientSdoNr(UNSIGNED8	sdoNr CO_COMMA_LINE_PARA_DECL);
RET_T		initUpDnLd_req(SDO_CLIENT_T *pClientSdo, UNSIGNED8 *pDomDat, UNSIGNED32 dSize,
			UNSIGNED8 bCmd CO_COMMA_LINE_PARA_DECL);
RET_T		abortSdoTransf_Req(SDO_T *pCurSdo, RET_T commonRet CO_COMMA_LINE_PARA_DECL);
void		dnLdBlk_ind(SDO_T *pSdo, UNSIGNED8 *canBuf CO_COMMA_LINE_PARA_DECL);
void		initDnLdBlk_ind(SDO_T *pCurSdo, UNSIGNED8 *canBuf CO_COMMA_LINE_PARA_DECL);
void		endDnLdBlk_ind(SDO_T *pSdo, UNSIGNED8 *canBuf CO_COMMA_LINE_PARA_DECL);
void		initUpLdBlk_ind(SDO_T *pCurSdo, UNSIGNED8 *canBuf CO_COMMA_LINE_PARA_DECL);
void		dnLdBlkEnd_req(SDO_CLIENT_T *pClientSdo CO_COMMA_LINE_PARA_DECL);
void		upLdBlkEnd_req(SDO_T *pSdo CO_COMMA_LINE_PARA_DECL);
void		initUpLd_res(SDO_T *pCurSdo CO_COMMA_LINE_PARA_DECL);
void		upLdSeg_ind(SDO_T *pCurSdo CO_COMMA_LINE_PARA_DECL);
RET_T		initUpLd_ind(SDO_T *pCurSdo, UNSIGNED8 *canBuf
			CO_COMMA_LINE_PARA_DECL);
void		dnLdBlk_con	(SDO_CLIENT_T *pClientSdo, UNSIGNED8 *canBuf
			CO_COMMA_LINE_PARA_DECL);
void		upLdBlk_con	(SDO_T *pClientSdo, UNSIGNED8 *canBuf CO_COMMA_LINE_PARA_DECL);
void		upLdBlk_ind	(SDO_CLIENT_T *pCurSdo, UNSIGNED8 *canBuf CO_COMMA_LINE_PARA_DECL);
void		initUpLdBlk_con	(SDO_CLIENT_T *pClientSdo, UNSIGNED8 *canBuf CO_COMMA_LINE_PARA_DECL);
RET_T		setSdoCobId(UNSIGNED8 sdoNr, UNSIGNED32 cobId, USER_T kind, COB_KIND_T cobType
			CO_COMMA_LINE_PARA_DECL);
void		sdoTimeOut(TIMER_EVENT_T *pTimer CO_COMMA_LINE_PARA_DECL);
void		resetAllSdos(CO_LINE_PARA_DECL);
UNSIGNED8	sdoSrdMsgReceived(SDO_T *pSdo, UNSIGNED32 errReason
			CO_COMMA_LINE_PARA_DECL);
void		initSdoVars(CO_LINE_PARA_DECL);
RET_T		checkCommParAccess(UNSIGNED16 index, UNSIGNED8  subIndex
			CO_COMMA_LINE_PARA_DECL);

UNSIGNED8	cfgManagerSdoEvent(SDO_CLIENT_T *pSdo, UNSIGNED32 reason
			CO_COMMA_LINE_PARA_DECL);

RET_T pcoSetSdoPtrCobId(SDO_T *pSdo, UNSIGNED32 cobId, USER_T kind, COB_KIND_T cobType
			CO_COMMA_LINE_PARA_DECL);

#  ifdef CONFIG_DOMAIN_INDICATION_SIZE
RET_T pcoUpLdBufUpdate(SDO_T *pCurSdo CO_COMMA_LINE_PARA_DECL);
#  endif /* CONFIG_DOMAIN_INDICATION_SIZE */


# endif /* PCO_SDO__PROTOTYPES_H__ */
#endif /* CONFIG_WITHOUT_PROTOTYPES */

/* end of source */

