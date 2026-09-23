/*
 *++ globvars.c - Contains all global variables
 *-- globvars.c - Beinhaltet die globalen Variablen
 *
 * Copyright (c) 2014-2017 port GmbH Halle (Saale)
 *------------------------------------------------------------------
 */


/* header of project specific types */


/* constant definitions
---------------------------------------------------------------------------*/

/* local defined data types
---------------------------------------------------------------------------*/

/* list of external used functions, if not in headers
---------------------------------------------------------------------------*/

/* list of global defined functions
---------------------------------------------------------------------------*/

/* list of local defined functions
---------------------------------------------------------------------------*/

/* external variables
---------------------------------------------------------------------------*/

/* global variables
---------------------------------------------------------------------------*/
#ifdef CONFIG_NO_GLOBAL_VARS

/**********************************************************************
* Driver
**********************************************************************/
UNSIGNED8	coCanFlags CO_REDCY_PARA_ARRAY_DEF;

UNSIGNED8	*canDrvPtr CO_REDCY_PARA_ARRAY_DEF;

UNSIGNED8	*cpuDrvPtr;	/* pointer to global cpu driver data */

/* the following two variables are used in example applications */
UNSIGNED8	lNodeId;
UNSIGNED16	bitRate;

/**********************************************************************
* access_o
**********************************************************************/
OBJDIR_T	*pObjDir;
UNSIGNED16	*pMaxObjDicElements;

/**********************************************************************
* NMT
**********************************************************************/
UNSIGNED8	coNodeId;

LOCAL_NODE_T	co_Node;
# ifdef CONFIG_REDUNDANCY_SUPPORT
LOCAL_NODE_T	co_redcyNode;
# endif /* CONFIG_REDUNDANCY_SUPPORT */

COB_T		*co_pNMT_COB;
UNSIGNED32	co_nmtStartUp;

# ifdef CONFIG_REDUNDANCY_SUPPORT
UNSIGNED8	coLibFlags [2];
# else /* CONFIG_REDUNDANCY_SUPPORT */
UNSIGNED8	coLibFlags;
# endif /* CONFIG_REDUNDANCY_SUPPORT */

# ifdef CONFIG_MASTER
#  ifdef CONFIG_NMT_SLAVE_CNT
REMOTE_NODE_T	nmtSlaveList[CONFIG_NMT_SLAVE_CNT];

#   ifdef CONFIG_FAST_SORT
UNSIGNED8	nmtSlaveIdxList[CONFIG_NMT_SLAVE_CNT];
#   endif /* CONFIG_FAST_SORT */
#  endif /* CONFIG_NMT_SLAVE_CNT */
# endif /* defined(CONFIG_MASTER) */

# ifdef CONFIG_NO_ERROR_BEHAVIOR
# else /* CONFIG_NO_ERROR_BEHAVIOR */
UNSIGNED8	commErrorBehavior CO_LINE_PARA_ARRAY_DEF;
# endif /* CONFIG_NO_ERROR_BEHAVIOR */



/**********************************************************************
* NMT-Error
**********************************************************************/
# ifdef CONFIG_HEARTBEAT_CONSUMER
HB_CONS_T	hbConsList[CONFIG_HEARTBEAT_CONSUMER];

#  ifdef CONFIG_REDUNDANCY_SUPPORT
HB_CONS_T	redcyHbConsList[CONFIG_HEARTBEAT_CONSUMER];
#  endif /* CONFIG_REDUNDANCY_SUPPORT */

#  ifdef CONFIG_FAST_SORT
UNSIGNED8	hbIdxList[CONFIG_HEARTBEAT_CONSUMER];
#  endif /* CONFIG_FAST_SORT */
# endif /* CONFIG_HEARTBEAT_CONSUMER */

# ifdef CONFIG_REDUNDANCY_SUPPORT
UNSIGNED8	nmtErrFailed[16] [2];
UNSIGNED8	nmtErrStarted[16] [2];
UNSIGNED8	nmtErrConfig[16] [2];
UNSIGNED8	nmtErr3HBok[16];
#  ifdef CONFIG_MARITIME_SUPPORT
UNSIGNED8	nmtErrRedundancy[16];
#  endif /* CONFIG_MARITIME_SUPPORT */
# else /* CONFIG_REDUNDANCY_SUPPORT */
UNSIGNED8	nmtErrFailed[16];
UNSIGNED8	nmtErrStarted[16];
UNSIGNED8	nmtErrConfig[16];
# endif /* CONFIG_REDUNDANCY_SUPPORT */

# ifdef CONFIG_MASTER
#  ifdef CONFIG_NODE_GUARDING
GUARDING_T	guardSlaveList[CONFIG_GUARD_SLAVE_CNT];

#   ifdef CONFIG_FAST_SORT
UNSIGNED8	guardSlaveIdxList[CONFIG_GUARD_SLAVE_CNT];
#   endif /* CONFIG_FAST_SORT */
#  endif /* CONFIG_NODE_GUARDING */
# endif /* CONFIG_NODE_GUARDING */


/**********************************************************************
* SDO
**********************************************************************/
# ifdef CONFIG_SDO_CLIENT
SDO_CLIENT_T	co_sdoClient[CONFIG_SDO_CLIENT];
INTEGER8	co_sdoClientCnt;

#  ifdef CONFIG_FAST_SORT
UNSIGNED8	co_sdoClientNrList[CONFIG_SDO_CLIENT];
UNSIGNED8	co_sdoClientCobIdxList[CONFIG_SDO_CLIENT];
#  endif /* CONFIG_FAST_SORT */
# endif /* CONFIG_SDO_CLIENT */

# ifdef CONFIG_SDO_SERVER
SDO_T		co_sdoServer[CONFIG_SDO_SERVER];
INTEGER8	co_sdoServerCnt CO_LINE_PARA_ARRAY_DEF;

#  ifdef CONFIG_FAST_SORT
UNSIGNED8	co_sdoServerNrList[CONFIG_SDO_SERVER];
UNSIGNED8	co_sdoServerCobIdxList[CONFIG_SDO_SERVER];
#  endif /* CONFIG_FAST_SORT */
# endif /* CONFIG_SDO_SERVER */


/**********************************************************************
* PDO
**********************************************************************/
# ifdef CONFIG_PDO_PRODUCER
PDO_T		co_trPdo[CONFIG_PDO_PRODUCER];
INTEGER16	co_trPdoCnt;

#  ifdef CONFIG_PDO_FAST_SORT
UNSIGNED16	co_trPdoNrIdxList[CONFIG_PDO_PRODUCER];
UNSIGNED16	co_trPdoCobIdxList[CONFIG_PDO_PRODUCER];
#  endif /* CONFIG_PDO_FAST_SORT */
# endif /* CONFIG_PDO_PRODUCER */

# ifdef CONFIG_PDO_CONSUMER
PDO_T		co_recPdo[CONFIG_PDO_CONSUMER];	/* receive pdo structures */
INTEGER16	co_recPdoCnt;

#  ifdef CONFIG_PDO_FAST_SORT
UNSIGNED16	co_recPdoNrIdxList[CONFIG_PDO_CONSUMER];
UNSIGNED16	co_recPdoCobIdxList[CONFIG_PDO_CONSUMER];
#  endif /* CONFIG_PDO_FAST_SORT */
# endif /* CONFIG_PDO_CONSUMER */


# if defined(CONFIG_PDO_CONSUMER) || defined(CONFIG_PDO_PRODUCER)
PDO_MAP_T	co_mappingTable[CONFIG_MAPPING_CNT];
UNSIGNED16	co_mappingCnt;
# endif /* defined(CONFIG_PDO_CONSUMER) || defined(CONFIG_PDO_PRODUCER) */


/**********************************************************************
* Emergency
**********************************************************************/
# ifdef CONFIG_EMCY_CONSUMER
EMCY_CONS_T	emcyConsList[CONFIG_EMCY_CONSUMER];
FLAG_T		emcyConsFlags;

#  ifdef CONFIG_FAST_SORT
UNSIGNED8	emcyConsIdxList[CONFIG_EMCY_CONSUMER];
UNSIGNED8	emcyConsCobIdxList[CONFIG_EMCY_CONSUMER];
#  endif /* CONFIG_FAST_SORT */
# endif /* CONFIG_EMCY_CONSUMER */

# ifdef CONFIG_EMCY_PRODUCER
EMCY_T		co_EmcyProd;
# endif /* CONFIG_EMCY_PRODUCER */


/**********************************************************************
* SYNC
**********************************************************************/
# if defined(CONFIG_SYNC_PRODUCER) || defined(CONFIG_SYNC_CONSUMER)
SYNC_T		co_Sync;
UNSIGNED8	co_syncCnt;
# endif /* defined(CONFIG_SYNC_PRODUCER) || defined(CONFIG_SYNC_CONSUMER) */

/**********************************************************************
* Time
**********************************************************************/
# if defined(CONFIG_TIME_PRODUCER) || defined(CONFIG_TIME_CONSUMER)
CO_TIME_T	co_Time;
TIME_OF_DAY_T 	stdTime;
# endif /* defined(CONFIG_TIME_PRODUCER) || defined(CONFIG_TIME_CONSUMER) */


/**********************************************************************
* Timer
**********************************************************************/

volatile UNSIGNED8  coTimerTicks;
UNSIGNED16	coTimerPulse;

TIMER_EVENT_T	*co_timerList;
INHIBIT_EVENT_T	*co_inhibitList;

# ifdef CONFIG_EVA_VERSION
TIMER_EVENT_T	evaTimer;
# endif /* CONFIG_EVA_VERSION */


/**********************************************************************
* LED
**********************************************************************/
# ifdef CONFIG_CO_LED
CO_LED_T	coLed;

#  ifdef CONFIG_REDUNDANCY_SUPPORT
CO_LED_T		redcyCoLed;
#  endif /* CONFIG_REDUNDANCY_SUPPORT */
# endif /* CONFIG_CO_LED */


/**********************************************************************
* LSS
**********************************************************************/
# if defined(CONFIG_LSS_MASTER) || defined(CONFIG_LSS_SLAVE)
					/* pointer to COB structs */
COB_T		*pLss_TrCOB;
COB_T		*pLss_RecCOB;
FLAG_T	 	lssFlags;
IDENTITY_T	*pIdentity;
TIMER_EVENT_T	lssTimer;
UNSIGNED32	fastScanBits[4];
UNSIGNED32	fastScanIdent[4];
UNSIGNED8       lssFastPos;
# endif /* defined(CONFIG_LSS_MASTER) || defined(CONFIG_LSS_SLAVE) */

# ifdef CONFIG_LSS_MASTER
UNSIGNED8	lssExpectAnswer;
UNSIGNED8	lssSub;
UNSIGNED8	bitChecked;
UNSIGNED8	lssNext;
UNSIGNED32	lssId;
# endif /* CONFIG_LSS_MASTER */

# ifdef CONFIG_LSS_SLAVE
UNSIGNED8	lssBitrateSwitchState;
LSS_IDENT_T lssIdent;
UNSIGNED8	lssState;
# endif /* CONFIG_LSS_SLAVE */

# if defined(CONFIG_LSS_SLAVE)
UNSIGNED8	bitrateErr;
# endif /* defined(CONFIG_LSS_MASTER) || defined(CONFIG_LSS_SLAVE) */

/**********************************************************************
* flyma
**********************************************************************/
# ifdef CONFIG_FLYING_MASTER
UNSIGNED8 	co_activeMaster;

INTEGER8	priorFlyMaster CO_LINE_PARA_ARRAY_DEF;
TIMER_EVENT_T	flymaNegoTimer CO_LINE_PARA_ARRAY_DEF;
UNSIGNED16	nodeTimeSlot  CO_LINE_PARA_ARRAY_DEF,
		priorTimeSlot CO_LINE_PARA_ARRAY_DEF;
FLYMA_T		detectMCap CO_LINE_PARA_ARRAY_DEF;
FLYMA_T		responseMCap CO_LINE_PARA_ARRAY_DEF;
FLYMA_T		detectActM CO_LINE_PARA_ARRAY_DEF;
FLYMA_T		masterIdent CO_LINE_PARA_ARRAY_DEF;
FLYMA_T		masterNegotiation CO_LINE_PARA_ARRAY_DEF; /* trigger time slot */
FLYMA_T		forceMasterNegotiation CO_LINE_PARA_ARRAY_DEF;
FLYMA_T		triggerTimeSlot CO_LINE_PARA_ARRAY_DEF;
FLYMA_T		forceResetComm CO_LINE_PARA_ARRAY_DEF;
FLAG_T	 	flymaFlags CO_LINE_PARA_ARRAY_DEF;
TIMER_EVENT_T	negoTimeDelay CO_LINE_PARA_ARRAY_DEF;		/* timer values for trigger */


#  ifdef CONFIG_REDUNDANCY_SUPPORT
UNSIGNED8	co_redcyFlymaLine;
#  endif /* CONFIG_REDUNDANCY_SUPPORT */
# endif /* CONFIG_FLYING_MASTER */

/**********************************************************************
* startup manager
**********************************************************************/
# if defined(CONFIG_MASTER) && defined(CONFIG_NMT_STARTUP_MANAGER)
NMTS_MASTER_STARTUP_T	nmtsMaster;
NMT_SLAVE_STARTUP_T	nmtStartupSlave[CONFIG_NMT_SLAVE_CNT];

#  ifdef CONFIG_FAST_SORT
UNSIGNED8	nmtStartupSlaveIdxList[CONFIG_NMT_SLAVE_CNT];
#  endif /* CONFIG_FAST_SORT */
# endif /* defined(CONFIG_MASTER) && defined(CONFIG_NMT_STARTUP_MANAGER) */

/**********************************************************************
* redundancy
**********************************************************************/
# ifdef CONFIG_REDUNDANCY_SUPPORT
UNSIGNED8	co_redcyNmtLine;
UNSIGNED8	co_redcyActiveLine;
UNSIGNED8	co_redcyInActiveLine;
UNSIGNED8	co_redcyReceivedLine;
UNSIGNED8	co_redcySdoLine;
FLAG_T		co_redcyFlags;
UNSIGNED8	co_redcyMaxDelayTimeTicks;

COB_T		*pRedcy_RecCOB, *pRedcy_TrCOB;
TIMER_EVENT_T	redcyHbEvalTimer;
UNSIGNED32	redcyEvalTime_powerOn;
UNSIGNED32	redcyEvalTime_rstComm;
UNSIGNED8	*pRedcyErrorCnt;
UNSIGNED8	redcyErrorCntTreshold;
BOOL_T		redcyStopHeartbeat;
#  ifdef CONFIG_MASTER
TIMER_EVENT_T	redcyRSTTimer;
UNSIGNED8	resetCommLine;
#  endif /* CONFIG_MASTER */
# endif /* CONFIG_REDUNDANCY_SUPPORT */

#endif /* CONFIG_NO_GLOBAL_VARS */

