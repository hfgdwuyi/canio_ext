/*
 * co_globvars - public defines for global variables
 *
 * Copyright (c) 2014-2017 port GmbH Halle/Saale
 *------------------------------------------------------------------
 */

/**
*  \file co_globvars.h
*++ Defines for global variables
*-- Definitionen für globale Variablen
*  \author port GmbH Halle (Saale)
*
*++ The file contains the definition for global variables structure
*/

#ifndef __CO_GLOBVARS_H
# define __CO_GLOBVARS_H


# ifdef CONFIG_NO_GLOBAL_VARS


# define CONFIG_WITHOUT_PROTOTYPES

# include "../../canopen/source/access.h"
# include "../../canopen/source/nmt.h"
# include "../../canopen/source/timer.h"

#if defined(CONFIG_PDO_CONSUMER) || defined(CONFIG_PDO_PRODUCER)
# include "../../canopen/source/pdo.h"
#endif /* defined(CONFIG_PDO_CONSUMER) || defined(CONFIG_PDO_PRODUCER) */
#if defined(CONFIG_SDO_CLIENT) || defined(CONFIG_SDO_SERVER)
# include "../../canopen/source/sdo.h"
#endif /* defined(CONFIG_SDO_CLIENT) || defined(CONFIG_SDO_SERVER)  */
#ifdef CONFIG_EMCY_CONSUMER
# include "../../canopen/source/emerg.h"
#endif /* CONFIG_EMCY_CONSUMER */
#if defined(CONFIG_SYNC_PRODUCER) || defined(CONFIG_SYNC_CONSUMER)
# include "../../canopen/source/sync.h"
#endif /* defined(CONFIG_SYNC_PRODUCER) || defined(CONFIG_SYNC_CONSUMER) */
#ifdef CONFIG_HEARTBEAT_CONSUMER
# include "../../canopen/source/heartbt.h"
#endif /* CONFIG_HEARTBEAT_CONSUMER */
#if defined(CONFIG_EMCY_PRODUCER) || defined(CONFIG_EMCY_CONSUMER)
# include "../../canopen/source/emerg.h"
#endif /* defined(CONFIG_SYNC_PRODUCER) || defined(CONFIG_SYNC_CONSUMER) */
#ifdef CONFIG_FLYING_MASTER
# include "../../canopen/source/flyma.h"
#endif /* CONFIG_FLYING_MASTER */
#if defined(CONFIG_TIME_PRODUCER) || defined(CONFIG_TIME_CONSUMER)
# include "../../canopen/source/time_lib.h"
#endif /* defined(CONFIG_TIME_PRODUCER) || defined(CONFIG_TIME_CONSUMER) */
#if defined(CONFIG_LSS_MASTER) || defined(CONFIG_LSS_SLAVE)
# include "../../canopen/source/lss.h"
#endif /*defined(CONFIG_LSS_MASTER) || defined(CONFIG_LSS_SLAVE)*/


#ifndef CO_USER_DATA_PTR
# define CO_USER_DATA_PTR
#endif /* CO_USER_DATA_PTR */

/* global struct for all CANopen library variables */
typedef struct {

#include "../../canopen/source/globvars.c"

	/* user data pointer */
	CO_USER_DATA_PTR

} CANOPEN_DATA_T;

# undef CONFIG_WITHOUT_PROTOTYPES

# endif /* CONFIG_NO_GLOBAL_VARS */
#endif /* __CO_GLOBVARS_H */

/* end of source */
