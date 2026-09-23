/*
 * drv - defines for driver interface
 *
 * Copyright (c) 2001-2017 port GmbH Halle/Saale
 *------------------------------------------------------------------
 */

/*
DESCRIPTION

The file contains definitions of structures and complex data types
for driver interface

*/

#ifndef PCO_DRV_H__
# define PCO_DRV_H__

#include <co_def.h>
# ifdef CONFIG_REDUNDANCY_SUPPORT
#  include "drv_rdcy.h"
# endif /* CONFIG_REDUNDANCY_SUPPORT */

# if defined(CONFIG_MULT_LINES) && defined(CONFIG_MULT_CANCONTROL_TYPE)
#  include "drv_mc.h"
# endif /* defined(CONFIG_MULT_LINES) && defined(CONFIG_MULT_CANCONTROL_TYPE)*/

#include <co_drv.h>
#include <co_drvif.h>


/* external variable declarations */



/* are the macros not defined use the standard functions */
# ifndef DEFINE_COB
#   define DEFINE_COB		Define_COB
# endif /* DEFINE_COB */

# ifndef SET_COB_ID
#  ifdef CONFIG_NO_GLOBAL_VARS
#   define SET_COB_ID(par1, par2, par3)	\
	Set_COB_ID(par1, par2, par3 CO_COMMA_LINE_PARA)
#  else /* CONFIG_NO_GLOBAL_VARS */
#   define SET_COB_ID		Set_COB_ID
#  endif /* CONFIG_NO_GLOBAL_VARS */
# endif /* SET_COB_ID */

# ifndef TRANSMIT_COB
#  ifdef CONFIG_NO_GLOBAL_VARS
#   define TRANSMIT_COB(par1, par2) Transmit_COB(par1, par2 CO_COMMA_LINE_PARA)
#  else /* CONFIG_NO_GLOBAL_VARS */
#   define TRANSMIT_COB		Transmit_COB
#  endif /* CONFIG_NO_GLOBAL_VARS */
# endif /* TRANSMIT_COB */

# ifndef UPDATE_COB
#   define UPDATE_COB		Update_COB
# endif /* UPDATE_COB */

# ifndef CLEAR_RX_BUFFER
#   define CLEAR_RX_BUFFER	clearRxBuffer
# endif /* CLEAR_RX_BUFFER */

# ifndef CLEAR_TX_BUFFER
#   define CLEAR_TX_BUFFER	clearTxBuffer
# endif /* CLEAR_TX_BUFFER */



/* function prototypes */
void init_canDriverPtr(void *pCobList, void *pIdxList, UNSIGNED16 cobCnt
	CO_COMMA_LINE_PARA_DECL);


#endif /* PCO_DRV_H__ */

/* end of source */
