/*
 * co_stor - defines for parameter storage
 *
 * Copyright (c) 2001-2017 port GmbH Halle/Saale
 *------------------------------------------------------------------
 */

/**
*  \file co_stor.h
*++ Defines for storing data
*-- Definitionen für die Speicherung von Daten
*  \author port GmbH Halle (Saale)
*
*++ The file contains definitions of structures and data types for
*++ storing data via CANopen
*-- Diese Datei enthält Definitionen von Strukturen und Datentypen
*-- zur Speicherung von Daten mit CANopen
*/

#ifndef __CO_STOR_H
# define __CO_STOR_H

# include <co_type.h>

/* signature for store, restore clear non volatile memory */
#define SAVE_SIGNATURE  0x65766173UL /* save */
#define CLEAR_SIGNATURE 0x6c6c696bUL /* kill */
#define LOAD_SIGNATURE  0x64616f6cUL /* load */

/* constants for load/save/clean non volatile memory */

#define CO_RESTORE_MODE_BOOTUP		1
#define CO_RESTORE_MODE_RESETCOMM	2
#define CO_RESTORE_MODE_SDO		3

/* const for read the storage indexes */

#define STORE_PARA_ON_COMMAND		1
#define STORE_PARA_AUTONOMOUSLY		2
#define RESTORE_PARA_ON_COMMAND		1

/* Storage structure for 0x1010 and 0x1011 */

typedef struct {
	UNSIGNED8   numOfEntries;          /* number of entries in record */
	UNSIGNED32  segment[1];	           /* segment of memory */
} STORAGE1_T;

typedef struct {
	UNSIGNED8   numOfEntries;          /* number of entries in record */
	UNSIGNED32  segment[2];	           /* segment of memory */
} STORAGE2_T;

typedef struct {
	UNSIGNED8   numOfEntries;          /* number of entries in record */
	UNSIGNED32  segment[3];	           /* segment of memory */
} STORAGE3_T;

typedef struct {
	UNSIGNED8   numOfEntries;          /* number of entries in record */
	UNSIGNED32  segment[4];	           /* segment of memory */
} STORAGE4_T;

typedef struct {
	UNSIGNED8   numOfEntries;          /* number of entries in record */
	UNSIGNED32  segment[5];	           /* segment of memory */
} STORAGE5_T;

typedef struct {
	UNSIGNED8   numOfEntries;          /* number of entries in record */
	UNSIGNED32  segment[6];	           /* segment of memory */
} STORAGE6_T;

typedef struct {
	UNSIGNED8   numOfEntries;          /* number of entries in record */
	UNSIGNED32  segment[7];	           /* segment of memory */
} STORAGE7_T;

typedef struct {
	UNSIGNED8   numOfEntries;          /* number of entries in record */
	UNSIGNED32  segment[8];	           /* segment of memory */
} STORAGE8_T;

/* external data declarations */

/* function prototypes */

BOOL_T		loadParameterInd(UNSIGNED8, UNSIGNED8 CO_COMMA_LINE_PARA_DECL);
BOOL_T 		saveParameterInd(UNSIGNED8 CO_COMMA_LINE_PARA_DECL);
BOOL_T 		clearParameterInd(UNSIGNED8 CO_COMMA_LINE_PARA_DECL);

#endif		/*  __CO_STOR_H */

/* end of source */

