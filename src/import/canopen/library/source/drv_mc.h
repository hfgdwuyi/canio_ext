/*
 * drv_mc - defines for driver interface for multi can controller types
 *
 * Copyright (c) 2001-2017 port GmbH Halle/Saale
 *------------------------------------------------------------------
 */

/*
DESCRIPTION

The file contains definitions of structures and complex data types
for driver interface for multi can controller types

*/

#ifndef __DRV_MULT_CAN_H
# define __DRV_MULT_CAN_H

# if defined(CONFIG_MULT_LINES) && defined(CONFIG_MULT_CANCONTROL_TYPE)
/*******************************************************************
*
*++ For using of more CAN controllers from different type
*++ the functions are called depended from the can line.
*++ A adaption for the used hardware is necessary.
*++
*++ The description is available at the rigth driver.
*-- Für die Nutzung von mehreren CAN-Controllern von unterschiedlichem Typ
*-- muss beim Aufruf die entsprechende Funktion abhängig von der CAN-Linie
*-- aufgerufen werden.
*-- Eine Anpassung an die jeweilige Hardware ist in jedem Fall notwendig.
*--
*-- Die Beschreibung der Funktionen erfolgt beim jeweiligen Treiber
*/

#define DEFINE_COB		Define_COB_fctTab[canLine]
#define SET_COB_ID(par1, par2)	Set_COB_ID_fctTab[par1->canLine](par1, par2)
#define UPDATE_COB(par1, par2)	Update_COB_fctTab[par1->canLine](par1, par2)
#define GETNEXT_TX_REQUEST(par1) GetNext_TX_Request_fctTab[par1](par1)
#define TRANSMIT_COB(par1, par2) Transmit_COB_fctTab[par1->canLine](par1,par2)

#define MSG_IDENTIFICATION(par1)	  msgIdentification(par1, canLine)
#define FLAG_IDENTIFICATION(CO_LINE_PARA) flagIdentification(CO_LINE_PARA)

#define CLEAR_RX_BUFFER		clearRxBuffer
#define CLEAR_TX_BUFFER		clearTxBuffer

typedef COB_T	*(Define_COB_t)(COB_KIND_T, UNSIGNED8, UNSIGNED8);
typedef void	(Set_COB_ID_t)(COB_T *, UNSIGNED16);
typedef void	(Update_COB_t)(COB_T *, UNSIGNED8 *);
typedef void	(GetNext_TX_Request_t)(UNSIGNED8) RTX51_MODIFIER;
typedef void	(Transmit_COB_t)(COB_T *, UNSIGNED8 *);

extern Define_COB_t	*Define_COB_fctTab[CO_MAX_CAN_LINES];
extern Set_COB_ID_t	*Set_COB_ID_fctTab[CO_MAX_CAN_LINES];
extern Update_COB_t	*Update_COB_fctTab[CO_MAX_CAN_LINES];
extern GetNext_TX_Request_t *GetNext_TX_Request_fctTab[CO_MAX_CAN_LINES];
extern Transmit_COB_t	*Transmit_COB_fctTab[CO_MAX_CAN_LINES];

# endif /* defined(CONFIG_MULT_LINES) && defined(CONFIG_MULT_CANCONTROL_TYPE) */

#endif /* __DRV_MULT_CAN_H */

/* end of source */

