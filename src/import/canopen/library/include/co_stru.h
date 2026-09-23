/*
 * co_stru - defines common structures for CANopen
 *
 * Copyright (c) 2001-2017 port GmbH Halle/Saale
 *------------------------------------------------------------------
 */

/**
*  \file co_stru.h
*++ Defines common structures for CANopen
*-- Definitionen allgemeiner Strukturen für CANopen
*  \author port GmbH Halle (Saale)
*
*++ The file contains definitions of structures and data types
*++ for the CANopen library
*-- Diese Datei enthält allgemeine Definitionen von Strukturen und Datentypen
*-- for die CANopen Library.
*/

#ifndef __CO_STRU_H
# define __CO_STRU_H

# include <co_def.h>		/* include canopen definition */


/* CAN buffer structures */
struct CAN_MSG
{
    COB_KIND_T	cobType;		/* COB Type */
    COB_IDENT_T cobId;			/* COB Id */
    UNSIGNED8  pData[8];		/* data */
    UNSIGNED8  length;			/* if bit CO_RTR_REQ is set -> RTR */
};

typedef struct 	CAN_MSG		CAN_MSG_T;


/**
* \struct CO_COB
* Communication Object datatype
*/
struct CO_COB
{
	COB_IDENT_T	 cobId;		/**< Message Identifier */
	UNSIGNED8	 bLength;	/**< Message length */
	UNSIGNED8	 bChannel;	/* internal */
#if defined(CONFIG_MULT_LINES) || defined(CONFIG_REDUNDANCY_SUPPORT)
	UNSIGNED8	 canLine;	/**< CAN bus line */
#endif
	COB_KIND_T       eType;		/**< Type of Message */
#ifdef CONFIG_REDUNDANCY_SUPPORT
	struct CO_COB	*pNextLine;	/* internal */
#endif /* CONFIG_REDUNDANCY_SUPPORT */
};

/**
* \var COB_T
* Communication Object datatype
*/
typedef struct	CO_COB	COB_T;


/* Identity object DS 301 V 4.0 */
typedef struct {
	UNSIGNED8	numOfEntries;          /* number of entries in record */
	UNSIGNED32	vendorId;
	UNSIGNED32	productCode;
	UNSIGNED32	revisionNumber;
	UNSIGNED32	serialNumber;
} IDENTITY_T;


#endif		/*  __CO_STRU_H */

/* end of source */

