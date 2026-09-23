/*
 * co_nmtstart - public defines for the NMT Startup manager
 *
 * Copyright (c) 2014-2017 port GmbH Halle/Saale
 *------------------------------------------------------------------
 */

/**
*  \file co_nmtstart.h
*++ Defines for the bootup manager
*-- Definitionen für den Bootupmanager
*  \author port GmbH Halle (Saale)
*
*++ The file contains definitions of structures and data types for
*++ the boot manager.
*-- Diese Datei enthält Definitionen von Strukturen und Datentypen
*-- für den Bootupmanager.
*/

#ifndef __CO_NMTSTART_H
# define __CO_NMTSTART_H

/* definitions for the bits in object 0x1F80 ---*/
#define NMT_STARTUP_MASTER_BIT			0x00000001
#define NMT_STARTUP_START_ALL_NODE_BIT		0x00000002
#define NMT_STARTUP_MASTER_START_BIT		0x00000004
#define NMT_STARTUP_NOT_START_NODE_BIT		0x00000008
#define NMT_STARTUP_RESET_ALL_NODES_BIT		0x00000010
#define NMT_STARTUP_FLYING_MASTER_BIT		0x00000020
#define NMT_STARTUP_STOP_ALL_NODES_BIT		0x00000040

/* definitions for the bits in object 0x1F81 */
#define NMT_SLAVE_BIT				0x00000001
#define NMT_BOOT_SLAVE_BIT			0x00000004
#define NMT_MANDATORY_BIT			0x00000008
#define NMT_RESET_COMMUNICATION_BIT		0x00000010
#define NMT_SOFTWARE_VERSION_BIT		0x00000020
#define NMT_SOFTWARE_UPDATE_BIT			0x00000040


/*=== event codes ===========================*/
/* #define NMT_SLAVE_A_OBJ_NOT_LISTED         	0x41 */
#define NMT_SLAVE_B_NO_DEVICE_TYPE	2
#define NMT_SLAVE_C_WRONG_DEVICE_TYPE	3
#define NMT_SLAVE_D_WRONG_VENDOR_ID	4
#define NMT_SLAVE_E_NO_HEARTBEAT_STATE	5
/* #define NMT_SLAVE_F_NO_NODE_GUARDING_RES    0x46 */
/* #define NMT_SLAVE_G_NO_PROGRAM_DOWNLOAD_TIME     0x47 */
/* #define NMT_SLAVE_H_PROGRAM_DOWNLOAD_NOT_ALLOWED 0x48 */
/* #define NMT_SLAVE_I_PROGRAM_DOWNLOAD_FAILED	0x49 */
#define NMT_SLAVE_J_CONFIG_DOWNLOAD_FAILED	10
#define NMT_SLAVE_K_NO_HB		11
#define NMT_SLAVE_M_WRONG_PRODUCT_CODE	12
#define NMT_SLAVE_N_WRONG_REVISION	13
#define NMT_SLAVE_O_WRONG_SERIAL_NUMBER	14
#define NMT_SLAVE_BOOT_STARTED		20	/* slave starts booting */
#define	NMT_SLAVE_BOOT_FINISHED		21	/* slave booting finished */
#define NMT_SLAVE_MANDATORY_ERROR	22	/* error during NMT Startup */
#define NMT_SLAVE_UPDATE_CONFIG		23	/* * start the configuration */
#define NMT_SLAVE_UPDATE_SOFTWARE	24	/* * start the sw update */

#define NMT_MASTER_READY4START		30	/* wait for signal from appl */
#define NMT_MASTER_IS_STARTED		31	/* master is started */

#define NMT_NETWORK_ALL_SLAVES_BOOTED	40	/* all slaves booted */
#define NMT_NETWORK_ALL_SLAVES_STARTED	41	/* all mandatory slaves are
						 * in OPERATIONAL */
#define NMT_NETWORK_STARTUP_STOPPED	42	/* fatal error during NMT */
#define NMT_NETWORK_BOOT_TIMEOUT       	43	/* boot time is elapsed */

#define NMT_CONT_START_MASTER		50	/* continue start master */
#define NMT_CONT_UPDATE_SOFTWARE	51	/* continue update software */
#define NMT_CONT_UPDATE_CONFIG		52	/* continue update config */

#endif		/* _CO_NMTSTART_H */


/*=== function prototypes ============================================*/
#ifdef CONFIG_WITHOUT_PROTOTYPES
#else /* CONFIG_WITHOUT_PROTOTYPES */

# ifndef __CO_NMTSTART_PROTOTYPES_H
#  define __CO_NMTSTART_PROTOTYPES_H

RET_T	defineNmtStartup(CO_REDCY_PARA_DECL);
RET_T	nmtStartupReq(CO_REDCY_PARA_DECL);
void	nmtStartupContReq(UNSIGNED8, UNSIGNED8 CO_COMMA_REDCY_PARA_DECL);
void	nmtStartupMasterInd(UNSIGNED8 CO_COMMA_REDCY_PARA_DECL);
BOOL_T	nmtStartupNetworkInd(UNSIGNED8 CO_COMMA_LINE_PARA_DECL);
void	nmtStartupSlaveInd(UNSIGNED8, UNSIGNED8 CO_COMMA_REDCY_PARA_DECL);
UNSIGNED8 getNmtStartupSdoNr(UNSIGNED8 nodeId CO_COMMA_LINE_PARA_DECL);
UNSIGNED8 getNmtStartupNodeId(UNSIGNED8	sdoNr CO_COMMA_LINE_PARA_DECL);

# endif/* __CO_NMTSTART_PROTOTYPES_H */
#endif /* CONFIG_WITHOUT_PROTOTYPES */
/* end of source */

