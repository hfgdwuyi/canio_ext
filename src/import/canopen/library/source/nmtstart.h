/*
 * nmtstart - defines for the NMT Startup Manager
 *
 * Copyright (c) 2005-2017 port GmbH Halle/Saale
 *------------------------------------------------------------------
 */

/*
DESCRIPTION

The file contains definitions of structures and complex data types
for the startup process.
*/
#include <co_nmtstart.h>


#ifndef PCO_NMTSTART_H__
# define PCO_NMTSTART_H__



/*==========================================================================*/
/* NMT STARTUP MASTER-SPECIFIC DEFINITIONS                                  */
/*==========================================================================*/

/* defines for internal function numbers */
#define NMTS_MFCT_RESET_COMM			1
#define NMTS_MFCT_START_BOOT_TIMER		2
#define NMTS_MFCT_START_SLAVES			3
#define NMTS_MFCT_START_MASTER			4
#define	NMTS_MFCT_START_ALL_NODES		5
#define NMTS_MFCT_WAIT_START_MASTER		6
#define NMTS_MFCT_POLL_OPTIONAL_SLAVES		7
#define NMTS_MFCT_STANDBY			8
#define NMTS_MFCT_STOPPED			9

/*==========================================================================*/
/* NMT STARTUP SLAVE-SPECIFIC DEFINITIONS                                   */
/*==========================================================================*/

/*--- defines for the NMT Startup process controlling ---*/

#define NMTS_SFLAG_SDO_IN_USE	0x01	/* SDO in use */
#define NMTS_SFLAG_SDO_ABORT	0x02	/* SDO Abort message received */
#define NMTS_SFLAG_SDO_TIMEOUT	0x04	/* SDO timeout occurred*/
#define NMTS_SFLAG_NODE_ERROR	0x40
#define NMTS_SFLAG_FINISHED	0x80	/* boot slave is finished */

/*--- internal function numbers ---*/
#define NMTS_SFCT_NO					0x80
#define NMTS_SFCT_START					0x01
#define NMTS_SFCT_REQUEST_DEVICE_TYPE			0x02
#define NMTS_SFCT_CHECK_DEVICE_TYPE			0x03
#define NMTS_SFCT_REQUEST_IDENTITY_VENDOR		0x04
#define NMTS_SFCT_CHECK_IDENTITY_VENDOR			0x05
#define NMTS_SFCT_REQUEST_IDENTITY_PRODUCT_CODE		0x06
#define NMTS_SFCT_CHECK_IDENTITY_PRODUCT_CODE		0x07
#define NMTS_SFCT_REQUEST_IDENTITY_REVISION		0x08
#define NMTS_SFCT_CHECK_IDENTITY_REVISION		0x09
#define NMTS_SFCT_REQUEST_IDENTITY_SERIAL_NUMBER	0x0A
#define NMTS_SFCT_CHECK_IDENTITY_SERIAL_NUMBER		0x0B
#define NMTS_SFCT_RESET_COMM				0x0C
#define NMTS_SFCT_START_ERROR_CONTROL			0x10
#define NMTS_SFCT_FINISHED_OK				0x11
#define NMTS_SFCT_CONFIG				0x13
#define NMTS_SFCT_CHECK_CONFIG1				0x14
#define NMTS_SFCT_CHECK_CONFIG2				0x15
#define NMTS_SFCT_UPDATE_CONFIG				0x16
#define NMTS_SFCT_START_NODE				0x17
#define NMTS_SFCT_WAIT_CONFIG				0x18
#define NMTS_SFCT_SOFTWARE_UPDATE			0x19
#define NMTS_SFCT_WAIT_SOFTWARE_UPDATE			0x1a
#define NMTS_SFCT_ERROR_OCCURED				0x1b


#define NMT_RET_OK			0	/* return status ok */
#define NMT_RET_BUSY			1	/* process is running */
#define NMT_RET_ERROR			2	/* wait for timer event */
#define NMT_RET_WAIT4TIMER		3	/* wait for timer event */
#define NMT_RET_GUARD_INIT_ERROR	4	/* guarding init error */
#define NMT_RET_SDO_ABORT		5	/* SDO Abort received */
#define NMT_RET_SDO_TIMEOUT		6	/* no SDO response received */
#define NMT_RET_SDO_OK			7	/* SDO response received */
#define NMT_RET_SDO_ERROR		8	/* SDO transfer error */

#define NMT_ERRCTRL_BOOTUP_RECEIVED	10	/* boot-up message received */
#define NMT_ERRCTRL_GUARD_RECEIVED	11	/* Node Guarding response */
#define NMT_ERRCTRL_HB_LOST		12	/* error ctrl heartbeat lost */
#define NMT_ERRCTRL_LOST_GUARDING	13	/* Node Guarding lost  */
#define NMT_ERRCTRL_NODE_STATE		14	/* error ctrl unexpected state*/
#define NMT_ERRCTRL_HB_STARTED		15	/* heartbeat started */
#define NMT_ERRCTRL_RECEIVED		16	/* errctrl received */

#define NMTSLAVE_TYPE_UNKNOWN	0	/* slave type unknown */
#define NMTSLAVE_TYPE_SLAVE	1	/* slave type without bootup */
#define NMTSLAVE_TYPE_OPTIONAL	2	/* slave type optional */
#define NMTSLAVE_TYPE_MANDATORY	3	/* slave type mandatory */

/*==========================================================================*/
/* GENERAL DEFINITIONS                                                      */
/*==========================================================================*/

/*==========================================================================*/
/* STRUCTURE DEFINITIONS                                                    */
/*==========================================================================*/

/* for internal data handling for the NMT master */
typedef struct{
	TIMER_EVENT_T bootTimer;	/* for monitoring of the boot time */
	UNSIGNED32	obj1F80;	/* value of object 0x1F80 */
	UNSIGNED8	fct;		/* function of the actual program step*/
	UNSIGNED8	si;		/* actual slave index */
	UNSIGNED8	slaveCnt;	/* number of slave */

	UNSIGNED8	globalRstCommAllowed;	/* global reset allowed */
	UNSIGNED8	mandatorySlaveCnt;	/* number of mandatory slaves */
	UNSIGNED8	mandatorySlaveFinish;	/* mandatory slaves processed*/
	UNSIGNED8	optionalSlaveCnt;	/* number of mandatory slaves */
	UNSIGNED8	optionalSlaveFinish;	/* mandatory slaves processed*/
	UNSIGNED8	mandatorySlaveFail;
	UNSIGNED32	bootTime;		/* overall boot time */
} NMTS_MASTER_STARTUP_T;


typedef struct {
	TIMER_EVENT_T	timer;
	UNSIGNED32	obj1F81;/* value of object 0x1F81 */
	UNSIGNED32	objVal; /* value to compare with slave value */
	UNSIGNED32	rxBuf;	/* receive buffer for values of
    				 * the slave's object dictionary */
	UNSIGNED8	nodeId;	/* node id */
	UNSIGNED8	typ;	/* node type - mandatory, optional, unknown */
	UNSIGNED8	fct;	/* function code of the actual program step */
	FLAG_T		flags;	/* control flags */
} NMT_SLAVE_STARTUP_T;


/* external data declarations */
#  ifdef CONFIG_DYN_MEM_ALLOC
extern NMT_SLAVE_STARTUP_T	*p_nmtStartupSlave[];
extern UNSIGNED8		*p_nmtStartupSlaveIdxList[];
extern UNSIGNED16		co_maxNmtStartupSlaves;
#  endif /* CONFIG_DYN_MEM_ALLOC */

#endif /* PCO_NMTSTART_H__ */

#ifdef CONFIG_WITHOUT_PROTOTYPES
#else /* CONFIG_WITHOUT_PROTOTYPES */

# ifndef __NMTSTART_PROTOTYPES_H
#  define __NMTSTART_PROTOTYPES_H

/* function prototypes */

void	nmtStartupProcess(CO_REDCY_PARA_DECL);
RET_T	setNmtStartupPara(UNSIGNED16 index, UNSIGNED8 subIndex, UNSIGNED32 objVal
		CO_COMMA_REDCY_PARA_DECL);
void	nmtStartupSdoEvent(UNSIGNED8 eventCode, UNSIGNED8 sdoNr
		CO_COMMA_REDCY_PARA_DECL);
void	nmtsTimerEvent(TIMER_EVENT_T *pTimer CO_COMMA_REDCY_PARA_DECL);
void	nmtsEventHandler(UNSIGNED8 eventCode, UNSIGNED8 nodeId CO_COMMA_REDCY_PARA_DECL);

#  ifdef CO_CONFIG_NMTSTART_NO_DEVICETYPE
BOOL_T coUserNmtStartupNoDeviceType( UNSIGNED8 nodeId, UNSIGNED16 *repeatTimeout
                CO_COMMA_LINE_PARA_DECL);
#  endif /* CO_CONFIG_NMTSTART_NO_DEVICETYPE */
#  ifdef CO_CONFIG_NMTSTART_SDO_TIMEOUT_IND
RET_T coUserNmtStartupSdoTimeInd( UNSIGNED8 nodeId, UNSIGNED16 index, UNSIGNED8 subIndex,
        UNSIGNED16 *timeOut CO_COMMA_LINE_PARA_DECL );
#  endif /* CO_CONFIG_NMTSTART_SDO_TIMEOUT_IND */

# endif /* __NMTSTART_PROTOTYPES_H */
#endif /* CONFIG_WITHOUT_PROTOTYPES */

/* end of source */

