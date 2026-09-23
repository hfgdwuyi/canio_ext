/*
 * lss - defines for lss usage
 *
 * Copyright (c) 2002-2017 port GmbH Halle/Saale
 *------------------------------------------------------------------
 */

/*
DESCRIPTION

The file contains definitions of structures and data types for lss usage

*/

#ifndef __LSS_H
# define __LSS_H

# include <co_lss.h>

#define LSS_FLAGS_MASTER	1	/* work as lss master */
#define LSS_FLAGS_WAITING	2	/* wait for answer from slave */
#define LSS_FLAGS_NODEID_CHANGED 4	/* node id was changed */
#define LSS_FLAGS_FAST_SCAN	8	/* fast scan active */
#define LSS_FLAGS_FAST_SCAN_ANSWER 0x10	/* fast scan answer received */


/* LSS states */
#define LSS_STATE_NONE		0	/* node is not in LSS FSA */
#define LSS_STATE_WAITING	1	/* node is in waiting mode */
#define LSS_STATE_CONFIG	2	/* node is in config mode */


/* LSS command specifier */
#define LSS_CS_SWITCH_GLOBAL	04	/* switch global */
#define LSS_CS_SET_NODEID	17	/* set node id */
#define LSS_CS_SET_BITRATE	19	/* set bit rate */
#define LSS_CS_ACTIVATE_BITRATE	21	/* activate bit rate */
#define LSS_CS_STORE_CFG	23	/* store configuration*/
#define LSS_CS_SWITCH_SEL_VENDOR 64	/* switch selektive vendor */
#define LSS_CS_SWITCH_SEL_PROD	65	/* switch selektive product */
#define LSS_CS_SWITCH_SEL_REV	66	/* switch revision */
#define LSS_CS_SWITCH_SEL_SNR	67	/* switch snr */
#define LSS_CS_SWITCH_SEL	68	/* switch selektive */
#define LSS_CS_IDENT_SLAVE_VENDOR	70	/* ident slave vendor */
#define LSS_CS_IDENT_SLAVE_PRODUCT	71	/* ident slave product */
#define LSS_CS_IDENT_SLAVE_REV_LOW	72	/* ident slave rev low */
#define LSS_CS_IDENT_SLAVE_REV_HIGH	73	/* ident slave rev high */
#define LSS_CS_IDENT_SLAVE_SNR_LOW	74	/* ident slave snr low */
#define LSS_CS_IDENT_SLAVE_SNR_HIGH	75	/* ident slave snr high */
#define LSS_CS_IDENT_SLAVE_NOCFG	76	/* ident slave without cfg */
#define LSS_CS_IDENT_SLAVE	79	/* ident slave */
#define LSS_CS_IDENT_SLAVE_CFG	80	/* ident slave without config */
#define LSS_CS_FAST_SCAN	81	/* LSS Fast Scaan */
#define LSS_CS_INQ_VENDOR	90	/* inquire vendor */
#define LSS_CS_INQ_PRODUCT	91	/* inquire product */
#define LSS_CS_INQ_REV		92	/* inquire revision */
#define LSS_CS_INQ_SNR		93	/* inquire snr */
#define LSS_CS_INQ_NODEID	94	/* inquire node id */



typedef struct {
	UNSIGNED32 vendor;	/* vendor id */
	UNSIGNED32 product;	/* product id */
	UNSIGNED32 rev_low;	/* revision number low */
	UNSIGNED32 rev_high;	/* revision number high */
	UNSIGNED32 snr_low;	/* serial number low */
	UNSIGNED32 snr_high;	/* serial number high */
} LSS_IDENT_T;


#define TRANS_LSS_DATA(cs, var) 			\
	bData[0] = (UNSIGNED8) cs;			\
	bData[1] = (UNSIGNED8) (var & 0xff);		\
	bData[2] = (UNSIGNED8) ((var >> 8) & 0xff);	\
	bData[3] = (UNSIGNED8) ((var >> 16) & 0xff);	\
	bData[4] = (UNSIGNED8) ((var >> 24) & 0xff);	\
	(void) TRANSMIT_COB(GL_ARRAY(pLss_TrCOB), &bData[0]);

				/* LSS master waits for an answer from slave
				 * answer - expected answer from slave */
#define LSS_WAIT_FOR_ANSWER(answer)	\
	GL_ARRAY(lssFlags) |= LSS_FLAGS_WAITING;	\
	(void) addTimerEvent(&GL_ARRAY(lssTimer), CON_TIMEOUT,	\
		CO_TIMER_TYPE_LSS_MSTR CO_COMMA_LINE_PARA); \
	GL_ARRAY(lssExpectAnswer) = answer;

/* external data declarations */

#endif		/*  __LSS_H */


/* function prototypes */

#ifdef CONFIG_WITHOUT_PROTOTYPES
#else /* CONFIG_WITHOUT_PROTOTYPES */

# ifndef PCO_LSS_PROTOTYPES_H__
#  define PCO_LSS_PROTOTYPES_H__



void	lssMsgReceived(CAN_MSG_T *canMsg CO_COMMA_LINE_PARA_DECL);
void	lssReqMsgReceived(CAN_MSG_T *canMsg CO_COMMA_LINE_PARA_DECL);
void	lssConMsgReceived(CAN_MSG_T *canMsg CO_COMMA_LINE_PARA_DECL);
void	lssSwitchTimeEvent(CO_LINE_PARA_DECL);
void	setLssState(UNSIGNED8 mode CO_COMMA_LINE_PARA_DECL);
void	lssTimeOut(CO_LINE_PARA_DECL);
void	initLss(CO_LINE_PARA_DECL);

# endif /* PCO_LSS_PROTOTYPES_H__ */
#endif /* CONFIG_WITHOUT_PROTOTYPES */


/* end of source */
