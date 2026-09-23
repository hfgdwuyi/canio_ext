/*
 * dynmem_var - variables for dynamic memory usage
 *
 * Copyright (c) 2014-2017 port GmbH Halle/Saale
 *------------------------------------------------------------------
 */

/*************************************************************
* variables for dynmem usage
*
* macro LIB_VAR:
*	pointer-var
*	var for number of elements
*	data type
*	function parameter cnt
*	static cnt
*	number of cobs
*
*/

#ifdef CONFIG_SDO_SERVER
LIB_VAR(p_co_sdoServer[0],co_maxSdoServer,SDO_T,sdoServCnt,sdoServ,2);

# ifdef CONFIG_FAST_SORT
LIB_VAR(p_co_sdoServerNrList[0],tmpVar,UNSIGNED8,sdoServCnt,sdoServ,0);
LIB_VAR(p_co_sdoServerCobIdxList[0],tmpVar,UNSIGNED8,sdoServCnt,sdoServ,0);
# endif /* CONFIG_PDO_FAST_SORT */
#endif /* CONFIG_SDO_SERVER */


#ifdef CONFIG_SDO_CLIENT
LIB_VAR(p_co_sdoClient[0],co_maxSdoClient,SDO_CLIENT_T,sdoClientCnt,sdoClient,2);

# ifdef CONFIG_FAST_SORT
LIB_VAR(p_co_sdoClientNrList[0],tmpVar,UNSIGNED8,sdoClientCnt,sdoClient,0);
LIB_VAR(p_co_sdoClientCobIdxList[0],tmpVar,UNSIGNED8,sdoClientCnt,sdoClient,0);
# endif /* CONFIG_PDO_FAST_SORT */
#endif /* CONFIG_SDO_CLIENT */


#ifdef CONFIG_PDO_PRODUCER
LIB_VAR(p_co_trPdo[0],co_pdoProdCnt,PDO_T,pdoProdCnt,pdoProd,1);

# ifdef CONFIG_FAST_SORT
LIB_VAR(p_co_trPdoNrIdxList[0],tmpVar,UNSIGNED16,pdoProdCnt,pdoProd,0);
LIB_VAR(p_co_trPdoCobIdxList[0],tmpVar,UNSIGNED16,pdoProdCnt,pdoProd,0);
# endif /* CONFIG_PDO_FAST_SORT */
#endif /* CONFIG_PDO_PRODUCER */


#ifdef CONFIG_PDO_CONSUMER
LIB_VAR(p_co_recPdo[0],co_pdoConsCnt,PDO_T,pdoConsCnt,pdoCons,1);

# ifdef CONFIG_FAST_SORT
LIB_VAR(p_co_recPdoNrIdxList[0],tmpVar,UNSIGNED16,pdoConsCnt,pdoCons,0);
LIB_VAR(p_co_recPdoCobIdxList[0],tmpVar,UNSIGNED16,pdoConsCnt,pdoCons,0);
# endif /* CONFIG_PDO_FAST_SORT */
#endif /* CONFIG_PDO_CONSUMER */

#if defined(CONFIG_PDO_CONSUMER) || defined(CONFIG_PDO_PRODUCER)
LIB_VAR(p_co_mappingTable[0],co_maxMappingCnt,PDO_MAP_T,mapCnt,map,0);
#endif /* defined(CONFIG_PDO_CONSUMER) || defined(CONFIG_PDO_PRODUCER) */


#ifdef CONFIG_HEARTBEAT_CONSUMER
LIB_VAR(p_hbConsList[0],co_maxHbConsCnt,HB_CONS_T,hbCnt,hb,1);
# ifdef CONFIG_REDUNDANCY_SUPPORT
LIB_VAR(p_redcyHbConsList[0],tmpVar,HB_CONS_T,hbCnt,hb,0);
# endif /* CONFIG_REDUNDANCY_SUPPORT */

# ifdef CONFIG_FAST_SORT
LIB_VAR(p_hbIdxList[0],tmpVar,UNSIGNED8,hbCnt,hb,0);
# endif /* CONFIG_PDO_FAST_SORT */
#endif /* CONFIG_HEARTBEAT_CONSUMER */


#ifdef CONFIG_EMCY_CONSUMER
LIB_VAR(p_emcyConsList[0],co_maxEmcyConsCnt,EMCY_CONS_T,emcyCnt,emcy,1);

# ifdef CONFIG_FAST_SORT
LIB_VAR(p_emcyConsIdxList[0],tmpVar,UNSIGNED8,emcyCnt,emcy,0);
LIB_VAR(p_emcyConsCobIdxList[0],tmpVar,UNSIGNED8,emcyCnt,emcy,0);
# endif /* CONFIG_FAST_SORT */
#endif /* CONFIG_EMCY_CONSUMER */


#ifdef CONFIG_MASTER
# ifdef CONFIG_NMT_SLAVE_CNT
LIB_VAR(p_nmtSlaveList[0],co_maxNmtSlaves,REMOTE_NODE_T,nmtSlaveCnt,nmtSlave,0);

#  ifdef CONFIG_FAST_SORT
LIB_VAR(p_nmtSlaveIdxList[0],tmpVar,UNSIGNED8,nmtSlaveCnt,nmtSlave,0);
#  endif /* CONFIG_FAST_SORT */
# endif /* CONFIG_NMT_SLAVE_CNT */


# ifdef CONFIG_NODE_GUARDING
LIB_VAR(p_guardSlaveList[0],co_maxGuardSlaves,GUARDING_T,nmtGuardCnt,nmtGuard,1);

#  ifdef CONFIG_FAST_SORT
LIB_VAR(p_guardSlaveIdxList[0],tmpVar,UNSIGNED8,nmtGuardCnt,nmtGuard,0);
#  endif /* CONFIG_FAST_SORT */
# endif /* CONFIG_NODE_GUARDING */


# ifdef CONFIG_NMT_STARTUP_MANAGER
LIB_VAR(p_nmtStartupSlave[0],co_maxNmtStartupSlaves,NMT_SLAVE_STARTUP_T,nmtSlaveCnt,nmtSlave,0);

#  ifdef CONFIG_FAST_SORT
LIB_VAR(p_nmtStartupSlaveIdxList[0],tmpVar,UNSIGNED8,nmtSlaveCnt,nmtSlave,0);
#  endif /* CONFIG_FAST_SORT */
# endif /* CONFIG_NMT_STARTUP_MANAGER */
#endif /* CONFIG_MASTER */

