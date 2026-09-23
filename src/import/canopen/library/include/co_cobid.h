/*
 * co_cobid - cob id define for CANopen
 *
 * Copyright (c) 2002-2017 port GmbH Halle/Saale
 *------------------------------------------------------------------
 */

/**
*  \file co_cobid.h
*++ Defines for COB-ID
*-- Definitionen von COB-ID
*  \author port GmbH Halle (Saale)
*
*++ This file contains definitions of CAN identifiers
*++ used for CANopen services.
*-- Diese Datei enthält Definitionen von CAN-Identifier,
*-- die von den CANopen Diensten benutzt werden.
*/

#ifndef __CO_COBID_H
# define __CO_COBID_H

/* predefined connection set from ds 301 */

/* NMT */
#define CO_COBID_NMT		0u

/* Flying Master */
#define CO_COBID_FLYMA		0x70
#define CO_COBID_FLYMA_IDENT	0x71	/* master ident */
#define CO_COBID_FLYMA_TRIGGER	0x72	/* start trigger timeslot */
#define CO_COBID_FLYMA_REQ_ACTIVE 0x73	/* request active master */
#define CO_COBID_FLYMA_CAP_RESP	0x74	/* respone capable */
#define CO_COBID_FLYMA_CAP_REQ	0x75	/* request capable */
#define CO_COBID_FLYMA_FORCE_NEGO 0x76	/* force Master negotiation */
#define CO_COBID_FLYMA_END	CO_COBID_FLYMA_FORCE_NEGO

/* Redundancy */
#define CO_COBID_REDCY		0x7f

/* SYNC */
#define CO_COBID_SYNC		0x80

/* EMCY */
#define CO_COBID_EMCY		0x80
#define CO_COBID_EMCY_FIRST	(CO_COBID_EMCY + 1)
#define CO_COBID_EMCY_LAST	(CO_COBID_EMCY + 0x7f)

/* TIME */
#define CO_COBID_TIME		0x100

/* SRDO */
#define CO_COBID_SRDO		0x100
#define CO_COBID_SRDO_FIRST	(CO_COBID_SRDO + 1)
#define CO_COBID_SRDO_LAST	(CO_COBID_SRDO_FIRST + 0x7f)

/* PDOs */
#define CO_COBID_PDO		0x180
#define CO_COBID_PDO_FIRST	(CO_COBID_PDO + 1)
#define CO_COBID_PDO_LAST	(CO_COBID_PDO + 0x3ff)

#define CO_COBID_TPDO1		(CO_COBID_PDO )
#define CO_COBID_TPDO2		(CO_COBID_PDO + 0x100)
#define CO_COBID_TPDO3		(CO_COBID_PDO + 0x200)
#define CO_COBID_TPDO4		(CO_COBID_PDO + 0x300)

#define CO_COBID_RPDO1		(CO_COBID_PDO + 0x080)
#define CO_COBID_RPDO2		(CO_COBID_PDO + 0x180)
#define CO_COBID_RPDO3		(CO_COBID_PDO + 0x280)
#define CO_COBID_RPDO4		(CO_COBID_PDO + 0x380)

/* SDO */
#define CO_COBID_SDO		0x580u
#define CO_COBID_SSDO		CO_COBID_SDO
#define CO_COBID_CSDO		(CO_COBID_SDO + 0x80u)
#define CO_COBID_SDO_FIRST	(CO_COBID_SDO + 1u)
#define CO_COBID_SDO_LAST	(CO_COBID_SDO + 0xffu)

#define CO_COBID_SSDO_FIRST	(CO_COBID_SSDO + 1u)
#define CO_COBID_CSDO_FIRST	(CO_COBID_CSDO + 1u)

/* NMTERR */
#define CO_COBID_NMTERR		0x700u
#define CO_COBID_NMTERR_FIRST	(CO_COBID_NMTERR + 1u)
#define CO_COBID_NMTERR_LAST	(CO_COBID_NMTERR + 0x7fu)

#define CO_COBID_SDOREQ		1760u

#define CO_COBID_LSS_REQ	2021u	/* Request COB Id for LSS */
#define CO_COBID_LSS_CON	2020u	/* Confirmation COB Id for LSS */


#endif		/*  __CO_COBID_H */
/* end of source */

