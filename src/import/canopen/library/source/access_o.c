/*
 *++ access_o - contains routines for the access to the object dictionary
 *-- access_o - beinhaltet Rotuinen zum Zugriff auf das Objektverzeichnis
 *
 * Copyright (c) 1997-2017 port GmbH Halle (Saale)
 *------------------------------------------------------------------
 */

/**
*  \file access_o.c
*++ Functions for accessing the object dictionary
*-- Funktionen für den Zugriff auf das Objektverzeichnis
*  \author port GmbH Halle (Saale)
*
*++ This module contains functions for accessing
*++ objects in the object dictionary.
*++ The values and the properties of the object dictionary entries
*++ can be read and written via these functions.
*-- Dieses Modul enthält Zugriffsfunktionen zum Objektverzeichnis.
*-- Mit diesen Funktionen können Inhalt und Eigenschaften
*-- von Elementen des Objektverzeichnisses gelesen oder
*-- geschrieben werden.
*
*/

/* header of standard C - libraries */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/* header of project specific types */
#include <cal_conf.h>

#include <co_mcpy.h>
#include "access.h"
#include <co_acces.h>
#include <co_def.h>

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
#ifdef CONFIG_LIMITS_CHECK
static RET_T checkObjLimits(LIST_ELEMENT_T *curObj, UNSIGNED8 subIndex,
		UNSIGNED8 *pData);
#ifdef CONFIG_EXTENDED_DATA_TYPES
static INTEGER8 compareData(UNSIGNED8 *pVal1, UNSIGNED8	*pVal2, UNSIGNED8 len,
		UNSIGNED8 sign);
# endif /* CONFIG_EXTENDED_DATA_TYPES */
#endif /* CONFIG_LIMITS_CHECK */

/* external variables
---------------------------------------------------------------------------*/

/* global variables
---------------------------------------------------------------------------*/
#ifdef CONFIG_NO_GLOBAL_VARS
#else /* CONFIG_NO_GLOBAL_VARS */

# ifdef CONFIG_MULT_LINES
CO_LIB_INIT_VAR OBJDIR_T	**co_objDirMan = objDirMan;/* pointer to od for single line  */
CO_LIB_INIT_VAR UNSIGNED16	*co_maxObjDicElements = &maxObjDicElements[0];
CO_LIB_INIT_VAR OBJDIR_T	**pObjDirMan = objDirMan;/* pointer to od for multi line */
CO_LIB_INIT_VAR UNSIGNED16	*pMaxObjDicElements = &maxObjDicElements[0];
# else /* CONFIG_MULT_LINES */
CO_LIB_INIT_VAR OBJDIR_T	*co_objDir = &objDir[0];/* pointer to od for single line  */
CO_LIB_INIT_VAR UNSIGNED16	*co_maxObjDicElements = &maxObjDicElements;
CO_LIB_INIT_VAR OBJDIR_T	*pObjDir = &objDir[0];	/* pointer to od for single line  */
CO_LIB_INIT_VAR UNSIGNED16	*pMaxObjDicElements = &maxObjDicElements;
# endif /* CONFIG_MULT_LINES */
#endif /* CONFIG_NO_GLOBAL_VARS */

/* local defined variables
---------------------------------------------------------------------------*/
#ifdef CONFIG_RCS_IDENT
CO_LIB_INIT_VAR static char _rcsid[] = "$Id: access_o.c,v 2.76 2016/10/04 14:24:46 rli Exp $";
#endif /* CONFIG_RCS_IDENT */



/****************************************************************************/
/**
* \public
*
*++ \brief getObjEntry - get an object entry from the local object dictionary
*-- \brief getObjEntry - liefert einen Eintrag des lokalen Objektverzeichnisses
*
*++ The function copies the object value and the data size of a numerical object
*++ referenced by \em index and \em subIndex to the pointers given
*++ as function parameters.
*++ \em Size
*++ is the object size of a single element in bytes.
*++ In case of arrays or records only a single sub-index
*++ is allowed to address an object.
*++ The function tests the limits of indices and the read permission.
*-- Die Funktion kopiert den Wert und Datengröße eines
*-- über \em index und \em subIndex referenzierten nummerischen Objektes
*-- des Objektverzeichnisses zu den als Funktionsparametern angegebenen
*-- Zeigern.
*-- \em Size
*-- ist die Größe eines Elementes des Objektes in Bytes
*-- (bei Arrays oder Records bezieht sich das auf einen einzelnen Sub-Index).
*-- Der übergebene Index wird auf das zulässige Grenzwerte überprüft
*-- und es erfolgt eine Überprüfung der Zulässigkeit des Lesezugriffs.
*
* \par Endianess
*++ On BIG_ENDIAN machines it converts the data, if the parameter
*++ \b local
*++ is set to \c CO_FALSE .
*-- Bei BIG_ENDIAN Prozessoren erfolgt eine Datenwandlung,
*-- wenn der Parameter
*-- \b local == \c CO_FALSE gesetzt ist.
*
* \code
* UNSIGNED8 data[4];
* UNSIGNED32 size;
*
* // get value and size of Object 0x2000:1
* getObjEntry(0x2000, 1, &data[0], &size, CO_TRUE);
* \endcode
*
* \retval OK
*++ success
*-- Erfolg
* \retval CO_E_NONEXIST_OBJECT
*++ object doesn't exist
*-- Das angegebene Objekt existiert nicht
* \retval CO_E_NO_READ_PERM
*++ no read permission
*-- Keine Leseerlaubnis für dieses Objekt
* \retval CO_E_NONEXIST_SUBINDEX
*++ sub-index doesn't exist
*-- Der angegebene Subindex existiert nicht
*
*/
RET_T getObjEntry(
	UNSIGNED16 index,	/**< main-index */
	UNSIGNED8  subIndex,	/**< sub-index */
	UNSIGNED8  *pData,	/**< destination for data */
	UNSIGNED32 *pSize,	/**< destination for data size */
	BOOL_T	   local	/**< data only for local usage */
	CO_COMMA_LINE_PARA_DECL	/**< number of CAN line 0..CO_MAX_CAN_LINES-1 */
    )
{
LIST_ELEMENT_T *curObj;		/* pointer to current object */
RET_T		ret;		/* retval */

    /* index value exceeds the physical limitations */
    curObj = searchObj(index CO_COMMA_LINE_PARA);

    ret = getObjPtrEntry( curObj, index, subIndex, pData, pSize, local CO_COMMA_LINE_PARA );

    return ret;
}


/****************************************************************************/
/**
* \public
*
*++ \brief getObjAddr - get the address of the object in the local object dictionary
*-- \brief getObjAddr - liefert Adresse eines Eintrages im lokalen Objektverzeichnis
*
*++ The function delivers the address of an object
*++ referenced by \em index and \em subIndex.
*++ It tests only the limit of the provided indices.
*-- Die Funktion ermittelt die Adresse des über \em index und \em subIndex
*-- referenzierten Objektes des Objektverzeichnisses.
*-- Der übergebene Index wird auf das zulässige Grenzwerte überprüft.
*-- Wenn das \c #define \c CONFIG_VIRTUAL_OBJECTS gesetzt ist,
*-- und das Objekt \b nicht
*-- im herstellerspezifischen Bereich des Objektverzeichnis
*-- vorhanden ist, wird die Funktion \em getVirtualObjectAddr() aufgerufen.
*++ If \c CONFIG_VIRTUAL_OBJECTS  is \c #defined
*++ and the adressed object is \b not in the manufacturer specific area
*++ of the object dictionary then the function
*++ getVirtualObjectAddr() is called.
*
*-- Sie besitzt dieselben Parameter und Rückgabewerte wie \em getObjAddr(),
*-- ermöglicht dem Anwender aber die Nutzung virtueller Objekte.
*-- Der Anwender ist für die korrekte Artbeitsweise der Funktion verantwortlich.
*++ This function has the same parameters and return values as \em getObjAddr(),
*++ but enables the user to have so-called virtual objects
*++ in the object dictionary.
*++ The user is responsible for correct coding of this function.
*
* \code
* UNSIGNED8 data[4];
* UNSIGNED8 *pData;
* UNSIGNED32 size;
*
* // get address and size of Object 0x2000:1
* getObjAddr(0x2000, 1, &pData, &size);
* // copy Object 0x2000 to local array data[]
* memcpy(&data[0], pData, size);
* \endcode
*
* \retval OK
*++ success
*-- Erfolg
* \retval CO_E_NONEXIST_OBJECT
*++ object doesn't exist
*-- Das angegebene Objekt existiert nicht
* \retval CO_E_NONEXIST_SUBINDEX
*++ sub-index doesn't exist
*-- Der angegebene Subindex existiert nicht
*
*/

RET_T getObjAddr(
	UNSIGNED16 index,	/**< main-index */
	UNSIGNED8  subIndex,	/**< sub-index */
	UNSIGNED8  **pData,	/**< destination for data address*/
	UNSIGNED32 *pSize	/**< destination for data size */
	CO_COMMA_LINE_PARA_DECL	/**< number of CAN line 0..CO_MAX_CAN_LINES-1 */
    )
{
LIST_ELEMENT_T *curObj;		/* pointer to current object */

    /* index value exceeds the physical limitations */
    curObj = searchObj(index CO_COMMA_LINE_PARA);

    return getObjPtrAddr(curObj, index, subIndex, pData, pSize CO_COMMA_LINE_PARA );

}


/****************************************************************************/
/**
* \public
*
*++ \brief putObj - put an object to the local object dictionary
*-- \brief putObj - Schreiben eines Elementes im lokalen Objektverzeichnis
*
*++ The function copies the data into the object
*++ referenced by \em index and \em subIndex.
*++ The limits of indices are tested and also
*++ the write permission for remote access to the object.
*++ If the \em sub-index equals zero
*++ then the first element or the whole structure/array
*++ will be put into the dictionary.
*++ The parameter
*++ \b size specifies the size of the data in bytes.
*-- Die Funktion kopiert die übergebenen Daten in das
*-- durch \em index und \em subIndex angegebene Objekt.
*-- Sie testet den übergebenen Index auf die Grenzwerte und
*-- bei \em remote Zugriffen die Schreiberlaubnis
*-- für das Objekt.
*-- Der Parameter \b size gibt die Größe, der zu kopierenden
*-- Daten, in Bytes an.
*
* \par Endianess
*++ On BIG_ENDIAN machines it converts the data,
*++ if parameter
*++ \b local == \c CO_FALSE.
*-- Bei BIG_ENDIAN Prozessoren erfolgt eine Datenwandlung,
*-- wenn der Parameter
*-- \b local == \c CO_FALSE gesetzt ist.
*
*-- Wenn das \c #define \c CONFIG_VIRTUAL_OBJECTS gesetzt ist,
*-- und das Objekt nicht im herstellerspezifischen Bereich des Objektverzeichnis
*-- vorhanden ist, wird die Funktion getVirtualObjAddr()
*-- aufgerufen.
*-- Sie besitzt dieselben Parameter und Rückgabewerte wie getObjAddr(),
*-- ermöglicht dem Anwender aber die Nutzung virtueller Objekte.
*-- Der Anwender ist für die korrekte Artbeitsweise der Funktion verantwortlich.
*-- In  putObj() wird auf die Adresse
*-- des virtuellen Objekts
*-- die mit \em pData übergebenen Daten geschrieben.
*-- Als Datenlänge wird immer die von getVirtualObjAddr()
*-- erhaltene Länge genutzt.
*-- Der Anwender ist dafür verantwortlich,
*-- dass die übergebene Datenlänge korrekt ist.
*
*++ If \c CONFIG_VIRTUAL_OBJECTS  is \c #defined
*++ and the adressed object is \b not in the manufacturer specific area
*++ of the object dictionary then the function
*++ getVirtualObjAddr() is called.
*++ This function has the same parameters and return values as getObjAddr(),
*++ but enables the user to have so-called virtual objects
*++ in the object dictionary.
*++ The user is responsible for correct coding of this function.
*++ putObj() uses the address and data size information
*++ of an \em virtual \em object returned by getVirtualObjAddr()
*++ to write the data \em pData is pointing to to this address.
*++ The user is responsible for correct data size information.
*
* \retval CO_OK
*++ success
*-- Erfolg
* \retval CO_E_NONEXIST_OBJECT
*++ object doesn't exist
*-- das angegebene Objekt existiert nicht
* \retval CO_E_NO_WRITE_PERM
*++ no write permission
*-- keine Schreiberlaubnis für dieses Objekt
* \retval CO_E_NONEXIST_SUBINDEX
*++ sub-index doesn't exist
*-- der angegebene Subindex existiert nicht
* \retval CO_E_VALUE_TO_LOW
*++ value is too low
*-- der zu schreibende Wert liegt unter dem Limit
* \retval CO_E_VALUE_TO_HIGH
*++ value is too high
*-- der zu schreibende Wert liegt über dem Limit
* \retval CO_E_WRONG_SIZE
*++ size of data has wrong size
*-- falsche Datengröße
*
*/

RET_T putObj(
	UNSIGNED16 index,	/**< main-index */
	UNSIGNED8  subIndex,	/**< sub-index */
	UNSIGNED8  *pData,	/**< data address */
	UNSIGNED32 size,	/**< data size */
	BOOL_T     local	/**< data only for local usage */
	CO_COMMA_LINE_PARA_DECL	/**< number of CAN line 0..CO_MAX_CAN_LINES-1 */
    )
{
LIST_ELEMENT_T 	*curObj = NULL;	/* pointer to current object */
RET_T           retVal;         /* retval */

    curObj = searchObj(index CO_COMMA_LINE_PARA);

    retVal = putObjPtr( curObj, index, subIndex, pData, size, local CO_COMMA_LINE_PARA);

    return retVal;
}





#ifdef CONFIG_LIMITS_CHECK
/*****************************************************************************/
/*
* \private
*
* checkObjLimits - check object limits
*
* This function checks the limits for the given object
*
* return
* object size
*
*/
static RET_T checkObjLimits(
	LIST_ELEMENT_T	*pObj,		/* pointer to current object */
	UNSIGNED8	subIdx,		/* sub-index */
	UNSIGNED8	*pData		/* address of data to be written to the object */
    )
{
UNSIGNED16	attr;		/* object attributs */
UNSIGNED8	varType;
#ifdef CONFIG_EMULATE_U64
UNSIGNED64 limit;
INTEGER64 limitI64;
#endif /* CONFIG_EMULATE_U64 */


    if ((subIdx != 0u)
     && ((CO_READ_OD_DESC_ATTR(pObj, 0) & CO_SHORT_ARRAY_DESC) != 0)) {
	subIdx = 1u;
    }

    /* test the limits for numerical values */
    attr = CO_READ_OD_DESC_ATTR(pObj, subIdx);
    if ((attr & CO_NUM_VAL) == 0) {
	return(CO_OK);
    }

    varType = CO_READ_OD_DESC_U8(pObj, subIdx, varType);
    if (subIdx == 0u)  {
	/* if there are more subIndizes ,
	 * sub-index 0 has per definition only 1 byte */
	if (CO_READ_OD8(pObj->numOfElem) > 1)  {
	    varType = CO_TYPEDESC_UNSIGNED8;
	}
    }

    if (varType == CO_TYPEDESC_BOOL)  {
	if (*pData > CO_READ_OD_LIMIT_U8(pObj, subIdx, maxRange)) {
	    return(CO_E_VALUE_TO_HIGH);
	}

    } else if (varType == CO_TYPEDESC_UNSIGNED8)  {
	if (*pData < CO_READ_OD_LIMIT_U8(pObj, subIdx, minRange)) {
	    return(CO_E_VALUE_TO_LOW);
	}
	if (*pData > CO_READ_OD_LIMIT_U8(pObj, subIdx, maxRange)) {
	    return(CO_E_VALUE_TO_HIGH);
	}

    } else if (varType == CO_TYPEDESC_UNSIGNED16)  {
	UNSIGNED16	var;

	CO_NUM_MEMCPY((UNSIGNED8 *)&var, pData, 2, CO_NUM_VAL);
	if (var < CO_READ_OD_LIMIT_U16(pObj,subIdx,minRange)) {
	    return(CO_E_VALUE_TO_LOW);
	}
	if (var > CO_READ_OD_LIMIT_U16(pObj,subIdx,maxRange)) {
	    return(CO_E_VALUE_TO_HIGH);
	}

    } else if (varType == CO_TYPEDESC_UNSIGNED32)  {
	UNSIGNED32	var;

	CO_NUM_MEMCPY((UNSIGNED8 *)&var, pData, 4, CO_NUM_VAL);
	if (var < CO_READ_OD_LIMIT_U32(pObj,subIdx,minRange)) {
	    return(CO_E_VALUE_TO_LOW);
	}
	if (var > CO_READ_OD_LIMIT_U32(pObj,subIdx,maxRange)) {
	    return(CO_E_VALUE_TO_HIGH);
	}

    } else if (varType == CO_TYPEDESC_INTEGER8)  {
	if (*(INTEGER8 *)pData < CO_READ_OD_LIMIT_I8(pObj, subIdx, minRange)) {
	    return(CO_E_VALUE_TO_LOW);
	}
	if (*(INTEGER8 *)pData > CO_READ_OD_LIMIT_I8(pObj, subIdx, maxRange)) {
	    return(CO_E_VALUE_TO_HIGH);
	}

    } else if (varType == CO_TYPEDESC_INTEGER16)  {
	INTEGER16	var;

	CO_NUM_MEMCPY((UNSIGNED8 *)&var, pData, 2, CO_NUM_VAL);
	if (var < CO_READ_OD_LIMIT_I16(pObj,subIdx,minRange)) {
	    return(CO_E_VALUE_TO_LOW);
	}
	if (var > CO_READ_OD_LIMIT_I16(pObj,subIdx,maxRange)) {
	    return(CO_E_VALUE_TO_HIGH);
	}

    } else if (varType == CO_TYPEDESC_INTEGER32)  {
	INTEGER32	var;

	CO_NUM_MEMCPY((UNSIGNED8 *)&var, pData, 4, CO_NUM_VAL);
	if (var < CO_READ_OD_LIMIT_I32(pObj,subIdx,minRange)) {
	    return(CO_E_VALUE_TO_LOW);
	}
	if (var > CO_READ_OD_LIMIT_I32(pObj,subIdx,maxRange)) {
	    return(CO_E_VALUE_TO_HIGH);
	}

# ifdef CONFIG_FLOAT_VALUES
    } else if (varType == CO_TYPEDESC_REAL32)  {
	REAL32	var;

	CO_NUM_MEMCPY((UNSIGNED8 *)&var, pData, 4, CO_NUM_VAL);
	/* test for float values (4 bytes) */
	if (var < CO_READ_OD_LIMIT_R32(pObj,subIdx,minRange)) {
	    return(CO_E_VALUE_TO_LOW);
	}
	if (var > CO_READ_OD_LIMIT_R32(pObj,subIdx,maxRange)) {
	    return(CO_E_VALUE_TO_HIGH);
	}
# endif /* CONFIG_FLOAT_VALUES */

#ifdef CONFIG_EXTENDED_DATA_TYPES
    } else if (varType == CO_TYPEDESC_UNSIGNED24)  {
	UNSIGNED24	val, min, max;

	/* test for U24 values (3 bytes) */
	CO_NUM_MEMCPY((UNSIGNED8 *)&val, pData, 3, CO_NUM_VAL);
	val = val & 0xffffff;
	min = CO_READ_OD_LIMIT_U24(pObj,subIdx,minRange) & 0xffffff;
	max = CO_READ_OD_LIMIT_U24(pObj,subIdx,maxRange) & 0xffffff;

	if (val < min)  {
	    return(CO_E_VALUE_TO_LOW);
	}
	if (val > max)  {
	    return(CO_E_VALUE_TO_HIGH);
	}

    } else if (varType == CO_TYPEDESC_UNSIGNED40)  {
	/* test for U40 values (5 bytes) */
	UNSIGNED40	val;

	val = CO_READ_OD_LIMIT_U40(pObj,subIdx,minRange);
	if (compareData(pData, (UNSIGNED8 *)&val, 5, 0) < 0)  {
	    return(CO_E_VALUE_TO_LOW);
	}
	val = CO_READ_OD_LIMIT_U40(pObj,subIdx,maxRange);
	if (compareData(pData, (UNSIGNED8 *)&val, 5, 0) > 0)  {
	    return(CO_E_VALUE_TO_HIGH);
	}

    } else if (varType == CO_TYPEDESC_UNSIGNED48)  {
	/* test for U48 values (6 bytes) */
	UNSIGNED48	val;

	val = CO_READ_OD_LIMIT_U48(pObj,subIdx,minRange);
	if (compareData(pData, (UNSIGNED8 *)&val, 6, 0) < 0)  {
	    return(CO_E_VALUE_TO_LOW);
	}
	val = CO_READ_OD_LIMIT_U48(pObj,subIdx,maxRange);
	if (compareData(pData, (UNSIGNED8 *)&val, 6, 0) > 0)  {
	    return(CO_E_VALUE_TO_HIGH);
	}

    } else if (varType == CO_TYPEDESC_UNSIGNED56)  {
	/* test for U56 values (7 bytes) */
	UNSIGNED56	val;

	val = CO_READ_OD_LIMIT_U56(pObj,subIdx,minRange);
	if (compareData(pData, (UNSIGNED8 *)&val, 7, 0) < 0)  {
	    return(CO_E_VALUE_TO_LOW);
	}
	val = CO_READ_OD_LIMIT_U56(pObj,subIdx,maxRange);
	if (compareData(pData, (UNSIGNED8 *)&val, 7, 0) > 0)  {
	    return(CO_E_VALUE_TO_HIGH);
	}

    } else if (varType == CO_TYPEDESC_UNSIGNED64)  {

# ifdef CONFIG_EMULATE_U64
	limit = CO_READ_OD_LIMIT_U64(pObj, subIdx, minRange);
	if (compareData(pData, (UNSIGNED8 *)&limit, 8, 0) < 0) {
# else /* CONFIG_EMULATE_U64 */
	if (*(UNSIGNED64 *)pData < CO_READ_OD_LIMIT_U64(pObj,subIdx,minRange)) {
# endif /* CONFIG_EMULATE_U64 */
	    return(CO_E_VALUE_TO_LOW);
	}
# ifdef CONFIG_EMULATE_U64
	limit = CO_READ_OD_LIMIT_U64(pObj, subIdx, maxRange);
	if (compareData(pData, (UNSIGNED8 *)&limit, 8, 0) > 0)  {
# else /* CONFIG_EMULATE_U64 */
	if (*(UNSIGNED64 *)pData > CO_READ_OD_LIMIT_U64(pObj,subIdx,maxRange)) {
# endif /* CONFIG_EMULATE_U64 */
	    return(CO_E_VALUE_TO_HIGH);
	}

    } else if (varType == CO_TYPEDESC_INTEGER64)  {

# ifdef CONFIG_EMULATE_U64
	limitI64 = CO_READ_OD_LIMIT_I64(pObj, subIdx, minRange);
	if (compareData(pData, (UNSIGNED8 *)&limitI64, 8, 1) < 0) {
# else /* CONFIG_EMULATE_U64 */
	if (*(INTEGER64 *)pData < CO_READ_OD_LIMIT_I64(pObj,subIdx,minRange)) {
# endif /* CONFIG_EMULATE_U64 */
	    return(CO_E_VALUE_TO_LOW);
	}
# ifdef CONFIG_EMULATE_U64
	limitI64 = CO_READ_OD_LIMIT_I64(pObj, subIdx, maxRange);
	if (compareData(pData, (UNSIGNED8 *)&limitI64, 8, 1) > 0)  {
# else /* CONFIG_EMULATE_U64 */
	if (*(INTEGER64 *)pData > CO_READ_OD_LIMIT_I64(pObj,subIdx,maxRange)) {
# endif /* CONFIG_EMULATE_U64 */
	    return(CO_E_VALUE_TO_HIGH);
	}
#endif /* CONFIG_EXTENDED_DATA_TYPES */

    }

    return(CO_OK);
}


/*****************************************************************************/
/**
* \public
*
*++ \brief getObjLimits - get object limits
*
*++ This function returns pointer to the limits for the given object
*++ It doesn't work for virtual objects.
*-- Diese Funktion liefert Zeiger auf die Min- und Max-Werte
*-- für das geforderte Objekt.
*-- Diese Funktion kann nicht für virtuelle Objekte aufgerufen werden.
* \code
*   UNSIGNED8 *low;     // Take care of the objects data type !
*   UNSIGNED8 *high;
*   UNSIGNED8 retVal;
*   retVal = getObjLimits(0x2000, 0,  &low, &high);
*   printf("current limits  %d - %d\n",  *low, *high);
*
*   *low += 10;
*   *high += 10;
*
*   retVal = getObjLimits(0x2000, 0,  &low, &high);
*   printf("current limits  %d - %d\n",  *low, *high);
* \endcode
*++ In case you like to use these pointers to change limits,
*++ take care that you have put them into RAM instead of code or Flash space.
*++ (CANopen Design Tool setting).
*-- Falls die Pointer auch zum ändern der Limits genutzt werden sollen
*-- muss man dafür Sorge tragen dass die Limits auch im RAM
*-- und nicht im Code oder Flash Bereich liegen.
*-- (CANopen Design Tool Einstellung).
*
* \retval CO_OK
*++ success	pointer to min and max limits are valid
*-- Erfolg	Zeiger zu Min- und Max-Werten sind gültig
* \retval CO_E_PARA_INCOMP
*++ error	pointers are invalid
*-- Fehler	Zeiger sind ungültig
* \retval CO_E_NONEXIST_OBJECT
*++ object does not exist
*-- Objekt nicht im Objektverzeichnis vorhanden
* \retval CO_E_NONEXIST_SUBINDEX
*++ sub-index does not exist
*-- Subindex existiert nicht
* \retval CO_E_TYPE
*++ non-numerical object
*-- nicht nummerisches Objekt
*/
RET_T getObjLimits(
	UNSIGNED16	index,		/**< [in] index */
	UNSIGNED8	subIdx,		/**< [in] sub-index */
	UNSIGNED8	**pMinVal,	/**< [out] pointer to the min limit */
	UNSIGNED8	**pMaxVal	/**< [out] pointer to the max limit */
	CO_COMMA_LINE_PARA_DECL	/**< [in] number of CAN line 0..CO_MAX_CAN_LINES-1 */
    )
{
UNSIGNED16	attr;		/* object attributs */
LIST_ELEMENT_T 	*pObj;	/* pointer to current object */
UNSIGNED8	varType;

    pObj = searchObj(index CO_COMMA_LINE_PARA);
    if (pObj == NULL)	{		/* object doesn't exist */
	return(CO_E_NONEXIST_OBJECT);
    }

    if (CO_READ_OD8(pObj->numOfElem) <= subIdx)  {
	/* sub-index does not exist */
	return(CO_E_NONEXIST_SUBINDEX);
    }

/* TODO - check for valid pointers */

    if ((subIdx != 0)
     && ((CO_READ_OD_DESC_ATTR(pObj, 0) & CO_SHORT_ARRAY_DESC) != 0)) {
	subIdx = 1;
    }

    attr = CO_READ_OD_DESC_ATTR(pObj, subIdx);
    if ((attr & CO_NUM_VAL) == 0) {
	return(CO_E_TYPE);
    }

    varType = CO_READ_OD_DESC_U8(pObj, subIdx, varType);
    if (subIdx == 0)  {
	/* if there are more subIndizes ,
	 * sub-index 0 has per definition only 1 byte */
	if (CO_READ_OD8(pObj->numOfElem) > 1)  {
	    varType = CO_TYPEDESC_UNSIGNED8;
	}
    }

    if (varType == CO_TYPEDESC_UNSIGNED8)  {
	*pMinVal = &CO_READ_OD_LIMIT_U8(pObj, subIdx, minRange);
	*pMaxVal = &CO_READ_OD_LIMIT_U8(pObj, subIdx, maxRange);

    } else if (varType == CO_TYPEDESC_UNSIGNED16)  {
	*pMinVal = (UNSIGNED8 *)&CO_READ_OD_LIMIT_U16(pObj,subIdx,minRange);
	*pMaxVal = (UNSIGNED8 *)&CO_READ_OD_LIMIT_U16(pObj,subIdx,maxRange);

    } else if (varType == CO_TYPEDESC_UNSIGNED32)  {
	*pMinVal = (UNSIGNED8 *)&CO_READ_OD_LIMIT_U32(pObj,subIdx,minRange);
	*pMaxVal = (UNSIGNED8 *)&CO_READ_OD_LIMIT_U32(pObj,subIdx,maxRange);

    } else if (varType == CO_TYPEDESC_INTEGER8)  {
	*pMinVal = (UNSIGNED8 *)&CO_READ_OD_LIMIT_I8(pObj, subIdx, minRange);
	*pMaxVal = (UNSIGNED8 *)&CO_READ_OD_LIMIT_I8(pObj, subIdx, maxRange);

    } else if (varType == CO_TYPEDESC_INTEGER16)  {
	*pMinVal = (UNSIGNED8 *)&CO_READ_OD_LIMIT_I16(pObj,subIdx,minRange);
	*pMaxVal = (UNSIGNED8 *)&CO_READ_OD_LIMIT_I16(pObj,subIdx,maxRange);

    } else if (varType == CO_TYPEDESC_INTEGER32)  {
	*pMinVal = (UNSIGNED8 *)&CO_READ_OD_LIMIT_I32(pObj,subIdx,minRange);
	*pMaxVal = (UNSIGNED8 *)&CO_READ_OD_LIMIT_I32(pObj,subIdx,maxRange);

# ifdef CONFIG_FLOAT_VALUES
    } else if (varType == CO_TYPEDESC_REAL32)  {
	/* test for float values (4 bytes) */
	*pMinVal = (UNSIGNED8 *)&CO_READ_OD_LIMIT_R32(pObj,subIdx,minRange);
	*pMaxVal = (UNSIGNED8 *)&CO_READ_OD_LIMIT_R32(pObj,subIdx,maxRange);
# endif /* CONFIG_FLOAT_VALUES */

# ifdef CONFIG_EXTENDED_DATA_TYPES
    } else if (varType == CO_TYPEDESC_UNSIGNED24)  {
	*pMinVal = (UNSIGNED8 *)&CO_READ_OD_LIMIT_U24(pObj,subIdx,minRange);
	*pMaxVal = (UNSIGNED8 *)&CO_READ_OD_LIMIT_U24(pObj,subIdx,maxRange);

    } else if (varType == CO_TYPEDESC_UNSIGNED40)  {
	*pMinVal = (UNSIGNED8 *)&CO_READ_OD_LIMIT_U40(pObj,subIdx,minRange);
	*pMaxVal = (UNSIGNED8 *)&CO_READ_OD_LIMIT_U40(pObj,subIdx,maxRange);

    } else if (varType == CO_TYPEDESC_UNSIGNED48)  {
	*pMinVal = (UNSIGNED8 *)&CO_READ_OD_LIMIT_U48(pObj,subIdx,minRange);
	*pMaxVal = (UNSIGNED8 *)&CO_READ_OD_LIMIT_U48(pObj,subIdx,maxRange);

    } else if (varType == CO_TYPEDESC_UNSIGNED56)  {
	*pMinVal = (UNSIGNED8 *)&CO_READ_OD_LIMIT_U56(pObj,subIdx,minRange);
	*pMaxVal = (UNSIGNED8 *)&CO_READ_OD_LIMIT_U56(pObj,subIdx,maxRange);

    } else if (varType == CO_TYPEDESC_UNSIGNED64)  {
	*pMinVal = (UNSIGNED8 *)&CO_READ_OD_LIMIT_U64(pObj,subIdx,minRange);
	*pMaxVal = (UNSIGNED8 *)&CO_READ_OD_LIMIT_U64(pObj,subIdx,maxRange);

    } else if (varType == CO_TYPEDESC_INTEGER64)  {
	*pMinVal = (UNSIGNED8 *)&CO_READ_OD_LIMIT_I64(pObj,subIdx,minRange);
	*pMaxVal = (UNSIGNED8 *)&CO_READ_OD_LIMIT_I64(pObj,subIdx,maxRange);
# endif /* CONFIG_EXTENDED_DATA_TYPES */
    } else {
	*pMinVal = NULL;
	*pMaxVal = NULL;
    }

    return(CO_OK);
}


# ifdef CONFIG_EXTENDED_DATA_TYPES
/****************************************************************************/
/**
* \private
*
* compareData- compares datastream
*
* This function compares to values for non-standard data size
*
* return
* -1	val1 < val2
* 0	val1 = val2
* 1	val1 > val2
*
*/
static INTEGER8 compareData(
	UNSIGNED8	*pVal1,		/* pointer to value 1 */
	UNSIGNED8	*pVal2,		/* pointer to value 2 */
	UNSIGNED8	len,		/* byte counter */
	UNSIGNED8	sign		/* sign/unsigned values */
    )
{
UNSIGNED8 sign1 = 0;
UNSIGNED8 sign2 = 0;

    /* printf("compareData sign: %d value1: %d value2: %d \n",sign,*pVal1,*pVal2); */

    if (sign == 0)  {
	/* unsigned values */
	while (len > 0)  {
	    len--;
	    if (pVal1[len] == pVal2[len])  {
		/* equal, next byte */
		if (len == 0)  {
		    /* was last byte, return equal */
		    return(0);
		}
	    } else {
		/* greater or less */
		if (pVal1[len] > pVal2[len])  {
		    /* its greater */
		    return(1);
		} else {
		    return(-1);
		}
	    }
	}
    }
    else
    /* signed values */
    {
        /* determine signs in first byte */
        sign1 = pVal1[len-1] & 0x80;
        sign2 = pVal2[len-1] & 0x80;

        /* printf("signs (0 == pos, anything else == neg) (value, limit)  %d %d\n",sign1,sign2); */

        if ((sign1 == 0) && (sign2 == 0))   /* both positive */
        {
            return compareData(pVal1,pVal2,len,0); /* compare as unsigned */
        }
        else if ((sign1 != 0) && (sign2 != 0)) /* both negative */
        {
            return compareData(pVal1,pVal2,len,0); /* also compare as unsigned */
        }
        else if ((sign1 == 0) && (sign2 != 0)) /* Val1 pos., Val2 neg. */
        {
            return 1;
        }
        else if ((sign1 != 0) && (sign2 == 0)) /* Val1 neg., Val2 pos. */
        {
            return -1;
        }
    }
    return(0);
}
# endif /* CONFIG_EXTENDED_DATA_TYPES */
#endif /* CONFIG_LIMITS_CHECK */


/*****************************************************************************/
/**
* \public
*
*++ \brief getobjSize - get size of object
*-- \brief getobjSize - Objektgröße abfragen
*
*++ This function returns the actual size of an object.
*++ Please note, use the right sub-index for short array description !
*-- Diese Funktion gibt die aktuelle Größe des Objektes zurück.
*-- Bitte beachten Sie, den richtgen Sub-Index für Short Arrays anzugeben.
*
* \return
*++ object size in bytes
*-- Objektgröße in Bytes
*
*/
CO_INLINE UNSIGNED32 getObjSize(
    CO_CONST LIST_ELEMENT_T *curObj,   /**< pointer to current object */
    UNSIGNED8 subIndex                 /**< actual sub-index */
    )
{
UNSIGNED32	size;

    switch (CO_READ_OD_DESC_U8(curObj, subIndex, varType)) {
	case CO_TYPEDESC_BOOL:
	case CO_TYPEDESC_UNSIGNED8:
	case CO_TYPEDESC_INTEGER8:
	    size = 1u;
	    break;
	case CO_TYPEDESC_UNSIGNED16:
	case CO_TYPEDESC_INTEGER16:
	    size = 2u;
	    break;
	case CO_TYPEDESC_UNSIGNED32:
	case CO_TYPEDESC_INTEGER32:
	    size = 4u;
	    break;
	case CO_TYPEDESC_UNSIGNED24:
	    size = 3u;
	    break;
	case CO_TYPEDESC_UNSIGNED40:
	    size = 5u;
	    break;
	case CO_TYPEDESC_UNSIGNED48:
	    size = 6u;
	    break;
	case CO_TYPEDESC_UNSIGNED56:
	    size = 7u;
	    break;
	case CO_TYPEDESC_UNSIGNED64:
        case CO_TYPEDESC_INTEGER64:
	    size = 8u;
	    break;
	case CO_TYPEDESC_VISSTRING:
	case CO_TYPEDESC_OCTETSTRING:
            if ( 0u == subIndex ) {
                UNSIGNED16 attr;
	        attr = CO_READ_OD_DESC_ATTR(curObj, subIndex);
	        if ((attr & CO_NUM_VAL) != 0) {
                    /* if sub-index 0 is a string and a number at the same time it is a array */
                    size = 1;
                } else {
	            size = ((STRING_DATA_T *)CO_READ_OD_DESC_PTR(curObj, subIndex, pDefaultVal))->maxLen;
                }
            } else {
	        size = ((STRING_DATA_T *)CO_READ_OD_DESC_PTR(curObj, subIndex, pDefaultVal))->maxLen;
            }
	    break;
	case CO_TYPEDESC_DOMAIN:
	    if ( 0u == subIndex ) {
	        UNSIGNED16 attr;
		attr = CO_READ_OD_DESC_ATTR(curObj, subIndex);
		if ((attr & CO_NUM_VAL) != 0) {
		    size = 1;
		} else {
	            size = ((DOMAIN_DATA_T *)CO_READ_OD_DESC_PTR(curObj, subIndex, pDefaultVal))->len;
		}

	    } else {
	         size = ((DOMAIN_DATA_T *)CO_READ_OD_DESC_PTR(curObj, subIndex, pDefaultVal))->len;
	    }
	    break;
#ifdef CONFIG_FLOAT_VALUES
	case CO_TYPEDESC_REAL32:
	    size = 4u;
	    break;
#endif /* CONFIG_FLOAT_VALUES */
	default:
	    size = 0u;
            break;
    }
    return(size);
}


/*****************************************************************************/
/**
* \public
*
*++ \brief getobjDefaultVal - get default value of object
*-- \brief getobjDefaultVal - Vorbelegungswert eines Objektes ausgeben
*
*++ This function returns the default value of a numerical object.
*++ For all non-numerical data types a NULL pointer is returned.
*-- Diese Funktion gibt den Vorbelegungswert eined nummerischen Objektes zurück.
*-- Für nicht nummerische Datentypen wird ein NULL Zeiger zurückgegeben.
*
* \return
*++ default value of the object
*-- Vorbelegungswert des Objektes
*
*/
UNSIGNED8 *getObjDefaultVal(
    CO_CONST LIST_ELEMENT_T *curObj,   /**< pointer to current object */
    UNSIGNED8 subIndex	               /**< actual sub-index */
    )
{
UNSIGNED8 *pVal;

    switch (CO_READ_OD_DESC_U8(curObj, subIndex, varType)) {
	case CO_TYPEDESC_BOOL:
	case CO_TYPEDESC_UNSIGNED8:
	case CO_TYPEDESC_UNSIGNED16:
	case CO_TYPEDESC_UNSIGNED24:
	case CO_TYPEDESC_UNSIGNED32:
	case CO_TYPEDESC_UNSIGNED40:
	case CO_TYPEDESC_UNSIGNED48:
	case CO_TYPEDESC_UNSIGNED56:
	case CO_TYPEDESC_UNSIGNED64:
	case CO_TYPEDESC_INTEGER8:
	case CO_TYPEDESC_INTEGER16:
	case CO_TYPEDESC_INTEGER32:
        case CO_TYPEDESC_INTEGER64:
	case CO_TYPEDESC_REAL32:
	    pVal = CO_READ_OD_DESC_PTR(curObj, subIndex, pDefaultVal);
	    break;
	/* case CO_TYPEDESC_VISSTRING: */
	    /* size = */
	    /* break; */
	default:
	    pVal = NULL;
	    break;
    }
    return(pVal);
}

/*****************************************************************************/
/**
* \public
*
*++ \brief getobjBasicDataType - get datatype of object
*-- \brief getobjBasicDataType - Datentyp des Objektes abfragen
*
*++ This function returns the basic data type of an object (see BASIC_DATA_T)
*++ Please note, use the right sub-index for short array description !
*-- Diese Funktion gibt den Datentyp des Objektes zurück.
*-- Bitte beachten Sie, bei mit Short Array optimierten Feldern den richtigen
*-- Sub-Index anzugeben.
*
* \return
*++ object data type
*-- Datentyp des Objektes
*
*/
CO_INLINE BASIC_DATA_T getObjBasicDataType(
    UNSIGNED16 index,         /**< main-index of the object */
    UNSIGNED8 subIndex        /**< sub-index of the object */
    CO_COMMA_LINE_PARA_DECL   /**< number of CAN line 0..CO_MAX_CAN_LINES-1 */
    )
{
LIST_ELEMENT_T *pObjEntry;	/* pointer to object entry */
#ifdef CONFIG_VIRTUAL_OBJECTS
UNSIGNED16	attr;
#endif /* CONFIG_VIRTUAL_OBJECTS */
BASIC_DATA_T	dataType;
UNSIGNED8	subIndexDesc;

    pObjEntry = searchObj(index CO_COMMA_LINE_PARA);
    if (pObjEntry == NULL) {

#ifdef CONFIG_VIRTUAL_OBJECTS
# ifdef CONFIG_DYN_MEM_ALLOC
# else /* CONFIG_DYN_MEM_ALLOC */
	/* virtual objects are allowed only in manufacturer or device profile area */
	if ((index < START_MANU_PROF) || (index > END_DEVICE_PROF)) {
	    return(CO_NIL);
	} else
# endif /* CONFIG_DYN_MEM_ALLOC */
	{
	    /* we can't really determine the datatype for virtuell objects */
	    attr = getVirtualObjAttr(index, subIndex CO_COMMA_LINE_PARA);
	    if ((attr & CO_NUM_VAL) != 0u)  {
		return(CO_UNSIGNED);
	    } else {
		return(CO_STRING);
	    }
	}
#else /* CONFIG_VIRTUAL_OBJECTS */
	return(CO_NIL);
#endif /* CONFIG_VIRTUAL_OBJECTS */
    }

    if (CO_READ_OD8(pObjEntry->numOfElem) <= subIndex)  {
	/* sub-index does not exist */
	return(CO_INVALID);
    }

    /* test for short arrays */
    if ((subIndex != 0u)
     && ((CO_READ_OD_DESC_ATTR(pObjEntry, 0u) & CO_SHORT_ARRAY_DESC) != 0u)){
	subIndexDesc = 1u;
    } else  {
	subIndexDesc = subIndex;
    }

    switch (CO_READ_OD_DESC_U8(pObjEntry, subIndexDesc, varType)) {
	case CO_TYPEDESC_BOOL:
	    dataType = CO_BOOLEAN;
	    break;
	case CO_TYPEDESC_UNSIGNED8:
	case CO_TYPEDESC_UNSIGNED16:
	case CO_TYPEDESC_UNSIGNED24:
	case CO_TYPEDESC_UNSIGNED32:
	case CO_TYPEDESC_UNSIGNED40:
	case CO_TYPEDESC_UNSIGNED48:
	case CO_TYPEDESC_UNSIGNED56:
	case CO_TYPEDESC_UNSIGNED64:
	    dataType = CO_UNSIGNED;
	    break;
	case CO_TYPEDESC_INTEGER8:
	case CO_TYPEDESC_INTEGER16:
	case CO_TYPEDESC_INTEGER32:
	case CO_TYPEDESC_INTEGER64:
	    dataType = CO_INTEGER;
	    break;
	case CO_TYPEDESC_VISSTRING:
	case CO_TYPEDESC_OCTETSTRING:
	case CO_TYPEDESC_DOMAIN:
	    dataType = CO_STRING;
	    break;
#ifdef CONFIG_FLOAT_VALUES
	case CO_TYPEDESC_REAL32:
	    dataType = CO_NIL;
	    break;
#endif /* CONFIG_FLOAT_VALUES */
	default:
	    dataType = CO_INVALID;
            break;
    }
    return(dataType);
}


#ifdef CONFIG_VARIABLES_ALIGNMENT
#else /* CONFIG_VARIABLES_ALIGNMENT */
/*****************************************************************************/
/**
* \public
*
*++ \brief getSubIndexAddr - search the address of an object
*-- \brief getSubIndexAddr - ermittelt die Subindex Adresse eines Objekts
*
*++ This function searches for the object address
*++ referenced by
*++ \em subIndex .
*-- Die Funktion sucht nach der über den \em subIndex referenzierten
*-- Adresse eines Objektes.
*
* \return
*++ address of object
*++ if successful
*-- Objektadresse
*-- bei Erfolg
*
*/

void *getSubIndexAddr(
    CO_CONST LIST_ELEMENT_T *curObj,    /**< pointer to object entry */
    UNSIGNED8 subIndex                  /**< sub-index of the object */
    )
{
UNSIGNED8	i; 		/* loop counter */
UNSIGNED8	size;		/* size of sub-index element */
UNSIGNED8	*pAddr;		/* pointer to sub-index elements */
UNSIGNED8	sIdxDesc;	/* sub-index description */
UNSIGNED8	shortDesc;	/* short desc on/off */
UNSIGNED16	attr;		/* sub-index attributes */
#if CONFIG_ALIGNMENT > 1u
UNSIGNED8	nextSize;	/* size of sub-index element */
# if CONFIG_ALIGNMENT == 2u
# else /* CONFIG_ALIGNMENT == 2 */
UNSIGNED8	rest;
# endif /* CONFIG_ALIGNMENT == 2 */
#endif /* CONFIG_ALIGNMENT > 1 */

    /* load base address */
    pAddr = CO_READ_ODP(curObj->pObj);

    /* check for short array description */
    if ((subIndex != 0u)
     && ((CO_READ_OD_DESC_ATTR(curObj, 0) & CO_SHORT_ARRAY_DESC) != 0u)) {
	shortDesc = 1u;
    } else {
	shortDesc = 0u;
    }

    /* get address of sub-index */
    for (i = 0u ; i < subIndex; i++) {
	/* check for short array description */
	if ((i != 0u) && (shortDesc != 0u))  {
	    sIdxDesc = 1u;
	} else {
	    sIdxDesc = i;
	}

	/* add size for last element */
	attr = CO_READ_OD_DESC_ATTR(curObj, sIdxDesc);
	if ((attr & CO_UP_DN_LD_DOMAIN)	!= 0u) {
	    size = (UNSIGNED8)(sizeof(UNSIGNED8 *));
	} else  {
	    size = (UNSIGNED8)getObjSize(curObj, sIdxDesc);
	}

#ifdef CONFIG_16BIT_CPU
	if ((attr & CO_NUM_VAL) != 0) {
	    size = (size + 1) >> 1;
	}
#endif /* CONFIG_16BIT_CPU */

        /* This function only gets called for records and arrays,
           and if sub-index 0 has the data type != UNSIGNED8 its a array. */
        if ( i == 0u ) {
            UNSIGNED8 varType = CO_READ_OD_DESC_U8(curObj, i, varType);
            switch (varType) {
	    case CO_TYPEDESC_VISSTRING:
            case CO_TYPEDESC_OCTETSTRING:
                /* Here we fix the byte count of sub-index 0 , because the used value for strings
                   was the number of array entries. */
	        size =  (UNSIGNED8)getObjSize(curObj, 1u);
	        break;
	    default:
	        break;
            }
        }


	/* if no Byte Alignment */
	pAddr += size;

#if CONFIG_ALIGNMENT > 1u
	/*
	   test whether address is a multiple of the CONFIG_ALIGNMENT
	   and the size of the next element must be greater or
	   equal ALIGNMENT, because the compiler set array elements
	   linear in the memory
	 */

	/* set actual size to 1 for non numeric values */
	if ((attr & CO_NUM_VAL) == 0u) {
	    size = 1u;
	}

	/* get size of next element */
	nextSize = size;

	/* test for short arrays */
	if (shortDesc != 0u)  {
	    sIdxDesc = 1u;
	} else {
	    sIdxDesc = i + 1u;
	}

	attr = CO_READ_OD_DESC_ATTR(curObj, sIdxDesc);
	if ((attr & CO_NUM_VAL) != 0u) {
	    nextSize = (UNSIGNED8)getObjSize(curObj, sIdxDesc);

# ifdef CONFIG_16BIT_CPU
	    nextSize = (nextSize + 1) >> 1;
# endif /* CONFIG_16BIT_CPU */
	}
	if ((attr & CO_UP_DN_LD_DOMAIN) != 0u) {
	    nextSize = (UNSIGNED8)(sizeof(UNSIGNED8 *));
	}

	if (nextSize > size)  {
	    if (nextSize > CONFIG_ALIGNMENT)  {
		nextSize = CONFIG_ALIGNMENT;
	    }
# if CONFIG_ALIGNMENT == 2u
	    if (((int)pAddr & 1) != 0)  {
		pAddr++;
	    }
# else
	    rest = (UNSIGNED8)((PTR_DATA_TYPE)pAddr % nextSize);
	    if (rest != 0u) {
		pAddr += (nextSize - rest);
	    }
# endif
	}
#endif /* CONFIG_ALIGNMENT > 1 */

    }
    return(pAddr);
}
#endif /* CONFIG_VARIABLES_ALIGNMENT */


/****************************************************************************/
/**
* \public
*
*++ \brief searchObj - search the object in the local dictionary
*-- \brief searchObj - sucht nach einem Objekt im lokalen Objektverzeichnis
*
*++ This function searches for the object referenced by
*++ \em index
*++ in the object dictionary
*++ and returns its address.
*-- Die Funktion sucht nach dem über den \em index referenzierten
*-- Objekt im Objektverzeichnis und liefert dessen Adresse.
*
* \return
*++ address of object
*++ if successful
*-- Objektadresse
*-- bei Erfolg
* \retval NULL
*++ searching failed
*-- Objekt nicht gefunden
*
*/

LIST_ELEMENT_T *searchObj(
	UNSIGNED16	index   /**< index to search for */
	CO_COMMA_LINE_PARA_DECL	/**< number of CAN line 0..CO_MAX_CAN_LINES-1 */
    )
{
LIST_ELEMENT_T *retVal = NULL;
INTEGER8  found = 0;
INTEGER16 low = 0, mid = 0;
UNSIGNED16 lIndex; /* local OD index */
#ifdef CONFIG_MULT_LINES
OBJDIR_T	*pDir = GL_ARRAY(pObjDirMan);
INTEGER16 high = (INTEGER16)GL_ARRAY(pMaxObjDicElements) - 1;
#else
OBJDIR_T	*pDir = GL_VAR(pObjDir);
INTEGER16 high = (INTEGER16)*GL_VAR(pMaxObjDicElements) - 1;
#endif /* CONFIG_MULT_LINES */

    while (found == 0)
    {
	if (high >= low) {
	    mid = (high + low) / 2;

	    lIndex = CO_READ_OD16(pDir[mid].index);
	    if (lIndex == index)  {
		found = 1;
	    } else {
		if (lIndex > index) {
		    high = mid - 1;
		} else  {
		    low = mid + 1;
		}
	    }
	} else {
	    found = -1;
	}
    }

    if (found >= 0)  {
	retVal = (LIST_ELEMENT_T *)&pDir[mid];
    }

    return(retVal);
}


/****************************************************************************/
/**
* \public
*
*++ \brief getObjAttr - delivers the attributes of an object
*-- \brief getObjAttr - ermittelt die Attribute eines Objektes
*
*++ This function delivers the attributes of the object
*++ referenced by \em index and \em subIndex.
*++ The return values are \b OR-ed combinations from the
*++ listed values below.
*-- Die Funktion ermittelt die Attribute des über
*-- \em index und \em subIndex referenzierten
*-- Objektes.
*-- Die Rückgabewerte sind \b OR-Verknüpfungen der unten gelisteten
*-- Werte.
*
*-- Wird ein \em virtuelles Objekt im herstellerspezifischen oder profilespezifischem Bereich
*-- adressiert, wird die User-Funktion
*++ If an \em virtual object in the manufacturer specific part or a device profile specific part
*++ of the object dictionary is addressed,
*++ the function
* \em getVirtualObjectAddr()
*++ is called
*-- aufgerufen.
*
*-- Sie besitzt dieselben Parameter und Rückgabewerte wie
*++ This function has the same parameters and return values as
* \em getObjAttr().
*-- Der Anwender ist für die korrekte Artbeitsweise der Funktion verantwortlich.
*++ The user is responsible for correct coding of this function.
*
*++ \par Attributes
*-- \par Attribute
*
* \arg \c CO_MAP_PERM
*++ PDO mapping permission
*-- PDO Mapping ist erlaubt
* \arg \c CO_READ_PERM
*++ read access permission
*-- Lesezugriffe sind erlaubt
* \arg \c CO_WRITE_PERM
*++ write access permission
*-- Schreibzugriffe sind erlaubt
* \arg \c CO_NUM_VAL
*++ object has numerical type
*-- Objekt hat einen numerischer Typ
* \arg \c CO_UP_DN_LD_DOMAIN
*++ object is a domain type
*-- Objekttyp ist Domain
*
* \retval 0
*++ object doesn't exist or attributte is zero
*-- Objekt existiert nicht oder das Attribut ist 0
* \retval CO_MAP_PERM
*++ PDO mapping permission
*-- PDO Mapping ist erlaubt
* \retval CO_READ_PERM
*++ read access permission
*-- Lesezugriffe sind erlaubt
* \retval CO_WRITE_PERM
*++ write access permission
*-- Schreibzugriffe sind erlaubt
* \retval CO_NUM_VAL
*++ object has numerical type
*-- Objekt hat einen numerischer Typ
* \retval CO_UP_DN_LD_DOMAIN
*++ object is a domain type
*-- Objekttyp ist Domain
*
*/

UNSIGNED16 getObjAttr(
	UNSIGNED16 index,	/**< index of object */
	UNSIGNED8  subIndex	/**< sub-index of object */
	CO_COMMA_LINE_PARA_DECL	/**< number of CAN line 0..CO_MAX_CAN_LINES-1 */
    )
{
UNSIGNED16 retVal = 0u;
LIST_ELEMENT_T *pObjEntry;	/* pointer to object entry */

    pObjEntry = searchObj(index CO_COMMA_LINE_PARA);
    if (pObjEntry == NULL) {

#ifdef CONFIG_VIRTUAL_OBJECTS
# ifdef CONFIG_DYN_MEM_ALLOC
# else /* CONFIG_DYN_MEM_ALLOC */
	/* virtual objects are allowed only in manufacturer or device profile area */
	if ((index < START_MANU_PROF) || (index > END_DEVICE_PROF)) {
	    return((UNSIGNED16)0);
	} else
# endif /* CONFIG_DYN_MEM_ALLOC */
	{
	    return(getVirtualObjAttr(index, subIndex CO_COMMA_LINE_PARA));
	}
#else /* CONFIG_VIRTUAL_OBJECTS */
	return((UNSIGNED16)0);
#endif /* CONFIG_VIRTUAL_OBJECTS */
    }


    /* test for short arrays */
    if ((subIndex != 0u)
     && ((CO_READ_OD_DESC_ATTR(pObjEntry, 0u) & CO_SHORT_ARRAY_DESC)
		!= 0u)) {
	retVal = CO_READ_OD_DESC_ATTR(pObjEntry, 1u);
    } else {
	retVal = CO_READ_OD_DESC_ATTR(pObjEntry, subIndex);
    }
	return retVal;
}


/****************************************************************************/
/**
* \public
*
*++ \brief setObjAttr - sets the attributes of an object
*-- \brief setObjAttr - setzt die Attribute eines Objektes
*
*++ This function sets the attributes of the object
*++ referenced by \em index and \em sub-index.
*++ Possible attributes can be any \b OR-ed combination from the
*++ listed values below.
*-- Die Funktion setzt die Attribute des über
*-- \em index und \em sub-index referenzierten
*-- Objektes.
*-- Mögliche Attribute sind beliebige \b OR-Verknüpfungen
*-- der unten gelisteten Werte.
*
*++ \par Attributes
*-- \par Attribute
*
* \arg \c CO_MAP_PERM
*++ PDO mapping permission
*-- PDO Mapping ist erlaubt
* \arg \c CO_READ_PERM
*++ read access permission
*-- Lesezugriffe sind erlaubt
* \arg \c CO_WRITE_PERM
*++ write access permission
*-- Schreibzugriffe sind erlaubt
* \arg \c CO_NUM_VAL
*++ object has numerical type
*-- Objekt hat einen numerischer Typ
* \arg \c CO_UP_DN_LD_DOMAIN
*++ object is a domain type
*-- Objekttyp ist Domain
*
* \retval CO_TRUE
*++ success
*-- Erfolg
* \retval CO_FALSE
*++ object doesn't exist
*-- Objekt existiert nicht
*/

BOOL_T setObjAttr(
	UNSIGNED16 index,    /**< index of object */
	UNSIGNED8  subIndex, /**< sub-index of object */
	UNSIGNED16  att       /**< attribute of object */
	CO_COMMA_LINE_PARA_DECL	/**< number of CAN line 0..CO_MAX_CAN_LINES-1 */
   )
{
LIST_ELEMENT_T *pObjEntry;	/* pointer to object entry */

    pObjEntry = searchObj(index CO_COMMA_LINE_PARA);
    if (pObjEntry == NULL) {
	return(CO_FALSE);
    }

    if (subIndex > (CO_READ_OD8(pObjEntry->numOfElem) - 1u))  {
	return(CO_FALSE);
    }

    /* test for short arrays */
    if ((subIndex != 0u)
     && ((CO_READ_OD_DESC_ATTR(pObjEntry, 0u) & CO_SHORT_ARRAY_DESC)
		!= 0u)) {
	CO_WRITE_OD_ATTR(pObjEntry, 1u, att);
    } else {
	CO_WRITE_OD_ATTR(pObjEntry, subIndex, att);
    }
    return(CO_TRUE);
}


/* function for CANopen domain transfer */
#ifdef CONFIG_DOMAIN_UPDNLD
/****************************************************************************/
/**
* \public
*
*++ \brief getDomainSize - get the size information of a domain object
*-- \brief getDomainSize - ermittelt die Größeninformation eines Domainobjektes
*
*++ This function gets the size information of the domain object
*++ referenced by \em index.
*++ This is necessary, because the user is responsible for
*++ the domain target location and size.
*-- Die Funktion ermittelt die Größeninformation des über
*-- \em index referenzierten Domainobjektes.
*-- Diese Funktion ist notwendig, da der Anwender für den
*-- Domainspeicherbereich verantwortlich ist.
*
* \retval 0
*++ object doesn't exist or size is zero
*-- Objekt existiert nicht oder Größe ist 0
* \retval size
*++ size of domain location
*-- Größe des Domainspeicherbereiches
*
*/

UNSIGNED32 getDomainSize(
	UNSIGNED16 index,	/**< index of object */
	UNSIGNED8  subIndex	/**< sub-index of object */
	CO_COMMA_LINE_PARA_DECL	/**< number of CAN line 0..CO_MAX_CAN_LINES-1 */
    )
{
LIST_ELEMENT_T *pObj;		/* pointer to object entry */

    pObj = searchObj(index CO_COMMA_LINE_PARA);
    if (pObj == NULL) {
	return(0u);
    }

    /* test for short arrays */
    if ((subIndex != 0u) && ((CO_READ_OD_DESC_ATTR(pObj, 0) & CO_SHORT_ARRAY_DESC) != 0))
    {
        return ((DOMAIN_DATA_T *)CO_READ_OD_DESC_PTR(pObj, 1, pDefaultVal))->len;
    }
    else
    {
        return ((DOMAIN_DATA_T *)CO_READ_OD_DESC_PTR(pObj, subIndex, pDefaultVal))->len;
    }
}


/****************************************************************************/
/**
* \public
*
*++ \brief setDomainSize - set the size information of a domain object
*-- \brief setDomainSize - setzt die Größeninformation eines Domainobjektes
*
*++ This function sets the size information of the domain object
*++ referenced by \em index.
*++ This is necessary, because the user is responsible for
*++ the domain target location and size.
*-- Die Funktion setzt die Größeninformation des über
*-- \em index und referenzierten Domainobjektes.
*-- Diese Funktion ist notwendig, da der Anwender für den
*-- Domainspeicherbereich und dessen Größe verantwortlich ist.
*
* \retval CO_TRUE
*++ success
*-- Erfolg
* \retval CO_FALSE
*++ object doesn't exist
*-- Objekt existiert nicht
*
*/

BOOL_T setDomainSize(
       UNSIGNED16 index,    /**< index of object */
       UNSIGNED8  subIndex, /**< sub-index of object */
       UNSIGNED32 size      /**< attribute of object */
	CO_COMMA_LINE_PARA_DECL	/**< number of CAN line 0..CO_MAX_CAN_LINES-1 */
       )
{
LIST_ELEMENT_T *pObjEntry;	/* pointer to object entry */

    pObjEntry = searchObj(index CO_COMMA_LINE_PARA);
    if (pObjEntry == NULL) {
	return(CO_FALSE);
    }

    /* test for short arrays */
    if ((subIndex != 0u) && ((CO_READ_OD_DESC_ATTR(pObjEntry, 0) & CO_SHORT_ARRAY_DESC)	!= 0))
    {
	((DOMAIN_DATA_T *)CO_READ_OD_DESC_PTR(pObjEntry, 1, pDefaultVal))->len = size;
    }
    else
    {
	((DOMAIN_DATA_T *)CO_READ_OD_DESC_PTR(pObjEntry, subIndex, pDefaultVal))->len = size;
    }

    return(CO_TRUE);
}

#ifdef CO_CONFIG_SDO_SHORT_STRINGS
/****************************************************************************/
/**
* \public
*
*++ \brief getStringSize - get the actual size information of a string
*-- \brief getStringSize - ermittelt die echte Größeninformation eines Strings
*
*++ This function gets the size information of the string object
*++ referenced by \em index.
*++ When sending this object over sdo, this is used to send only the needed bytes
*++ instead of always the full length.
*-- Die Funktion ermittelt die Größeninformation des über
*-- \em index referenzierten Strings.
*-- Wenn das Objekt über SDO versendent wird, werden somit nur die genutzten
*-- bytes versendet anstelle der vollen Länge des Objektes.
*
* \retval 0
*++ object doesn't exist or size is zero
*-- Objekt existiert nicht oder Größe ist 0
* \retval size
*++ size of domain location
*-- Größe des Stringspeicherbereiches
*
*/
BOOL_T getStringSize(
    UNSIGNED16 index,        /**< index of object */
    UNSIGNED8  subIndex,     /**< sub-index of object */
    UNSIGNED32* pSize        /**< actual size of string */
    CO_COMMA_LINE_PARA_DECL  /**< number of CAN line 0..CO_MAX_CAN_LINES-1 */
)
{
LIST_ELEMENT_T *pObjEntry;      /* pointer to object entry */

    pObjEntry = searchObj(index CO_COMMA_LINE_PARA);
    if (pObjEntry == NULL) {
        return(CO_FALSE);
    }

    /* test for short arrays */
    if ((subIndex != 0u)
     && ((CO_READ_OD_DESC_ATTR(pObjEntry, 0) & CO_SHORT_ARRAY_DESC)
                != 0))  {
        *pSize = ((STRING_DATA_T *)CO_READ_OD_DESC_PTR(pObjEntry, 1, pDefaultVal))->actLen;
    } else  {
        *pSize = ((STRING_DATA_T *)CO_READ_OD_DESC_PTR(pObjEntry, subIndex, pDefaultVal))->actLen;
    }

    /* printf("%hx %u string size get %lu\n",index,subIndex,*pSize); */

    return(CO_TRUE);
}
#endif /* CO_CONFIG_SDO_SHORT_STRINGS */

#ifdef CO_CONFIG_SDO_SHORT_STRINGS
/****************************************************************************/
/**
* \public
*
*++ \brief setStringSize - set the actual size information of a string object
*-- \brief setStringSize - setzt die echte Größeninformation eines Domainobjektes
*
*++ This function sets the size information of the string object
*++ referenced by \em index.
*-- Die Funktion setzt die Größeninformation des über
*-- \em index und referenzierten Stringobjektes.
*
* \retval CO_TRUE
*++ success
*-- Erfolg
* \retval CO_FALSE
*++ object doesn't exist
*-- Objekt existiert nicht
*
*/
BOOL_T setStringSize(
    UNSIGNED16 index,    /**< index of object */
    UNSIGNED8  subIndex, /**< sub-index of object */
    UNSIGNED32 size      /**< actual size of string */
    CO_COMMA_LINE_PARA_DECL /**< number of CAN line 0..CO_MAX_CAN_LINES-1 */
    )
{
LIST_ELEMENT_T *pObjEntry;      /* pointer to object entry */

    pObjEntry = searchObj(index CO_COMMA_LINE_PARA);
    if (pObjEntry == NULL)
    {
        return(CO_FALSE);
    }

    /* test for short arrays */
    if ((subIndex != 0u) && ((CO_READ_OD_DESC_ATTR(pObjEntry, 0) & CO_SHORT_ARRAY_DESC) != 0))
    {
        ((STRING_DATA_T *)CO_READ_OD_DESC_PTR(pObjEntry, 1, pDefaultVal))->actLen = size;
    }
    else
    {
        ((STRING_DATA_T *)CO_READ_OD_DESC_PTR(pObjEntry, subIndex, pDefaultVal))->actLen = size;
    }

    /* printf("%hx %u string size set to %lu\n",index,subIndex,((STRING_DATA_T *)CO_READ_OD_DESC_PTR(pObjEntry, subIndex, pDefaultVal))->actLen); */

    return(CO_TRUE);
}
#endif /* CO_CONFIG_SDO_SHORT_STRINGS */

/****************************************************************************/
/**
* \public
*
*++ \brief getDomainAddr - get the address of a domain object
*-- \brief getDomainAddr - ermittelt die Adresse eines Domainobjektes
*
*++ This function gets the address of the domain object
*++ referenced by \em index and \em subIndex.
*++ This is necessary, because the user is responsible for
*++ the domain target location.
*-- Die Funktion ermittelt die Adresse des über
*-- \em index und \em subIndex referenzierten Domainobjektes.
*-- Diese Funktion ist notwendig, da der Anwender für den
*-- Domainspeicherbereich verantwortlich ist.
*
* \retval NULL
*++ object doesn't exist or address is NULL
*-- Objekt existiert nicht oder Adresse ist 0
* \retval address
*++ address of domain location
*-- Adresse des Domainspeicherbereiches
*
*/

UNSIGNED8 *getDomainAddr(
    UNSIGNED16 index,        /**< index of object */
    UNSIGNED8 subIndex       /**< sub-index of object */
    CO_COMMA_LINE_PARA_DECL  /**< number of CAN line 0..CO_MAX_CAN_LINES-1 */
    )
{
UNSIGNED8  *pAddr;	/* destination for data address*/
UNSIGNED32 size ;	/* destination for data size */

    /* check for domain entry */
    if (((getObjAttr(index, subIndex CO_COMMA_LINE_PARA) & CO_UP_DN_LD_DOMAIN))
		== 0)  {
	return(NULL);
    }

    /* get pointer to address */
    if (getObjAddr(index, subIndex, &pAddr, &size CO_COMMA_LINE_PARA) != CO_OK){
	return(NULL);
    }

    /* DOMAIN_T is pointer of void !! */
    return((UNSIGNED8 *)(*((DOMAIN_T *)pAddr)));
}


/****************************************************************************/
/**
* \public
*
*++ \brief setDomainAddr - set the address of a domain object
*-- \brief setDomainAddr - setzt die Adresse eines Domainobjektes
*
*++ This function sets the address of the domain object
*++ referenced by \em index and \em subIndex.
*++ This is necessary, because the user is responsible for
*++ the domain target location.
*-- Die Funktion setzt die Adresse des über
*-- \em index und \em subIndex referenzierten Domainobjektes.
*-- Diese Funktion ist notwendig, da der Anwender für den
*-- Domainspeicherbereich verantwortlich ist.
*
* \retval CO_TRUE
*++ success
*-- Erfolg
* \retval CO_FALSE
*++ object doesn't exist
*-- Objekt existiert nicht
*
*/
BOOL_T setDomainAddr(
	UNSIGNED16 index,    /**< index of object */
	UNSIGNED8  subIndex, /**< sub-index of object */
	UNSIGNED8  *addr     /**< address of object */
	CO_COMMA_LINE_PARA_DECL	/**< number of CAN line 0..CO_MAX_CAN_LINES-1 */
       )
{
UNSIGNED8  *pAddr;	/* destination for data address*/
UNSIGNED32 size;	/* destination for data size */

    /* check for domain entry */
    if (((getObjAttr(index, subIndex CO_COMMA_LINE_PARA) & CO_UP_DN_LD_DOMAIN))
		== 0)  {
	return(CO_FALSE);
    }

    /* get pointer to address */
    if (getObjAddr(index, subIndex, &pAddr, &size CO_COMMA_LINE_PARA) != CO_OK){
	return(CO_FALSE);
    }

    *(void **)pAddr = addr;

    return(CO_TRUE);
}
#endif /* CONFIG_DOMAIN_UPDNLD */


/****************************************************************************/
/**
* \public
*
*++ \brief getNumOfElem - gets the number of elements of an array or struct
*-- \brief getNumOfElem - liefert Anzahl der Elemente eines Arrays oder Struktur
*
*++ The number of elements of the object referenced by \em index
*++ of an array or record is returned.
*-- Es wird die Anzahl der Elemente eines Array- oder Record-Objektes
*-- im Objektverzeichnis ermittelt.
*-- Das Objekt wird über den angegebenen \em index adressiert.
*
* \retval != 0
*++ number of elements if > 0
*-- Anzahl der Elemente, wenn > 0
* \retval 0
*++ object doesn't exist
*-- Objekt existiert nicht
*
*/

UNSIGNED8 getNumOfElem (
	UNSIGNED16 index   /**< main index of variable */
	CO_COMMA_LINE_PARA_DECL	/**< number of CAN line 0..CO_MAX_CAN_LINES-1 */
    )
{
UNSIGNED8 retVal = 0u;
LIST_ELEMENT_T *pObjEntry;	/* pointer to object entry */
#ifdef CONFIG_DYN_MEM_ALLOC
# ifdef CONFIG_VIRTUAL_OBJECTS
UNSIGNED32 size = 0u;
# endif /* CONFIG_VIRTUAL_OBJECTS */
#endif /* CONFIG_DYN_MEM_ALLOC */

    pObjEntry = searchObj(index CO_COMMA_LINE_PARA);
    if (pObjEntry != NULL) {
	retVal = CO_READ_OD8(pObjEntry->numOfElem);
    }
#ifdef CONFIG_DYN_MEM_ALLOC
# ifdef CONFIG_VIRTUAL_OBJECTS
    else /* fallback: try reading length via subindex 0 */
    {
        if (getObjEntry(index, 0u, &retVal, &size, CO_TRUE CO_COMMA_LINE_PARA) != CO_OK)
        {
            return 0;
        }
        retVal += 1;   /* count subindex 0 */
    }
# endif /* CONFIG_VIRTUAL_OBJECTS */
#endif /* CONFIG_DYN_MEM_ALLOC */
    return(retVal);
}


/*******************************************************************/
/**
* \public
*
*++ \brief setDefaultOvVal - set the default value for this object
*-- \brief setDefaultOvVal - setzt den Default Wert für ien Objekt
*
*++ This function sets the default value for the given object.
*++ It uses the default value of the object dictionary description structure
*++ and writes it to the actual c- variable.
*-- Diese Funktion setzt den Default Wert für das übergebene Objekt.
*-- Dazu wird der Default Wert aus der Objektverzeichnis-Beschreibungs-Struktur
*-- auf die aktuelle C-Variable geschrieben.
*
* \retval CO_OK
*++ success
*-- Erfolg
* \retval CO_E_NONEXIST_OBJECT
*++ object doesn't exist
*-- das angegebene Objekt existiert nicht
* \retval CO_E_NONEXIST_SUBINDEX
*++ sub-index doesn't exist
*-- Der angegebene Subindex existiert nicht
*
*/
RET_T setDefaultOdVal(
	UNSIGNED16	index,		/**< main-index */
	UNSIGNED8	subIndex	/**< sub-index */
	CO_COMMA_LINE_PARA_DECL	/**< number of CAN line 0..CO_MAX_CAN_LINES-1 */
    )
{
RET_T		retVal = CO_OK;
UNSIGNED8	*pDefaultVal;	/* default value */
UNSIGNED32	size;		/* size */
LIST_ELEMENT_T	*pObjEntry;	/* pointer to object entry */
UNSIGNED8	subIndexDesc;

    /* look for object */
    pObjEntry = searchObj(index CO_COMMA_LINE_PARA);
    if (pObjEntry == NULL)
    {
	retVal = CO_E_NONEXIST_OBJECT;
    }
    else
    {
        /* test for short array description */
        if ((subIndex != 0u)
         && ((CO_READ_OD_DESC_ATTR(pObjEntry, 0) & CO_SHORT_ARRAY_DESC)
		!= 0u))
        {
	    subIndexDesc = 1u;
        }
        else
        {
    	    subIndexDesc = subIndex;
        }

        /* get object size */
        size = getObjSize(pObjEntry, subIndexDesc);

        /* get default value */
        pDefaultVal = getObjDefaultVal(pObjEntry, subIndexDesc);

#ifdef CO_CODE_COPY
        /* pDefaultVal will be changed to a local buffer! */
        CO_CODE_COPY(pDefaultVal, size);
#endif /* CO_CODE_COPY */

        /* put default value to variable */
        retVal = putObj(index, subIndex, pDefaultVal, size, CO_TRUE
		CO_COMMA_LINE_PARA);
    }
    return(retVal);
}


/*******************************************************************/
/**
* \public
*
*++ \brief getOvDataTypeLen - get length of standard data types
*-- \brief getOvDataTypeLen - gibt die Länge von Standard Daten Typen zurück
*
*++ This function returns the length of standard data types.
*++ It is only valid for data types 1..7 (index 1..7).
*++ If it is called with an invalid \em index,
*++ it returns a length of 0.
*++ The returned length is in bits.
*-- Diese Funktion liefert die Länge von Standard Datentypen zurück.
*-- Sie kann nur für die Datentypen 1..7 (index 1..7) verwendet werden.
*-- Wenn ein ungültiger \em index übergeben wurde,
*-- wird die Länge 0 zurückgeliefert.
*-- Die Länge wird in Bits angeben.
*
* \return
*++ length of standard data types in bit
*-- Länge von Standard Daten Typ in Bit
*
*/
UNSIGNED8 getOvDataTypeLen(
    UNSIGNED16 index         /**< main-index */
    )
{
UNSIGNED8 retVal = 0u; /* bad index */
static CO_CONST UNSIGNED8 lenTab[] = { 0u, 1u, 8u, 16u, 32u, 8u, 16u, 32u };

    /* only index from 1..7 are allowed */
    if (index < 8u) {
	retVal = lenTab[index];
    }

    return retVal;
}


#ifdef CONFIG_NO_GLOBAL_VARS
/*******************************************************************/
/**
* \public
*
*++ \brief setupNewOd - setup new object dictionary
*-- \brief setupNewOd - setzt neues OD
*
*++ This function setup the given pointer as the start of the object dictionary.
*-- Diese Funktion setzt den übergebenen Pointer als Start des OV
*
*/
void setNewOdPtr(
	OBJDIR_T	*pDir,	/**< pointer to start od */
	UNSIGNED16	*pCnt	/**< number of elements of od */
	CO_COMMA_LINE_PARA_DECL	/**< number of CAN line 0..CO_MAX_CAN_LINES-1 */
    )
{
    GL_VAR(pObjDir) = pDir;
    GL_VAR(pMaxObjDicElements) = pCnt;
}
#endif /* CONFIG_NO_GLOBAL_VARS */


/*******************************************************************/
/**
* \public
*
*++ \brief getMaxObjDicElements - gets the object count
*-- \brief getMaxObjDicElements - Anzahl der Objekte
*
*++ This function return the number of objects of the selected line.
*-- Diese Funktion gibt die Anzahl der Objekte der ausgewaehlten Linie
*-- zurueck.
*
*++ \return Number of objects of the selected line.
*-- \return Anzahl der Objekte in der ausgewaehlten Linie
*
*/
UNSIGNED16 getMaxObjDicElements(
    CO_LINE_PARA_DECL   /**< number of CAN line 0..CO_MAX_CAN_LINES-1 */
    )
{
#ifdef CONFIG_MULT_LINES
    return GL_ARRAY(pMaxObjDicElements);
#else /* CONFIG_MULT_LINES */
    return *GL_ARRAY(pMaxObjDicElements);
#endif /* CONFIG_MULT_LINES */
}


/*******************************************************************/
/**
* \public
*
*++ \brief getObjPtrAtIndex - gets the object pointer at index
*-- \brief getObjPtrAtIndex - sucht den Objektpointer für einen index
*
*++ This function searches for an opbject pointer for the given index,
*++ and puts it in the given pointer.
*-- Diese Funktion sucht den Objektpointer für den uebergebenen index,
*-- und fuellt damit den uebergebenen Pointer.
*
* \retval CO_OK
*++ success
*-- Erfolg
* \retval CO_E_NONEXIST_OBJECT
*++ object does not exist
*-- Objekt nicht im Objektverzeichnis vorhanden
*/

RET_T getObjPtrAtIndex(
    UNSIGNED16 index,       /**< main-index */
    OBJDIR_T **pObj         /**< */
    CO_COMMA_LINE_PARA_DECL /**< number of CAN line 0..CO_MAX_CAN_LINES-1 */
    )
{
RET_T retVal = CO_E_NONEXIST_OBJECT;

    *pObj = searchObj(index CO_COMMA_LINE_PARA);

    if ( *pObj != NULL ) {
        retVal = CO_OK;
    }

    return retVal;
}


/*******************************************************************/
/**
* \public
*
*++ \brief getObjPtrNumElem - gets the number of sub-index from object pointer
*-- \brief getObjPtrNumElem - gibt die Anzahl der Subindexe eines Objektpointers zurueck
*
*++ This function return the number of sub-indices of an object pointer.
*-- Diese Funktion gibt die Anzahl der Subindizes eines Objektpointers zurueck.
*
*++ \return Number of subindecies.
*-- \return Anzahl der Subindexe.
*
*/

UNSIGNED8 getObjPtrNumElem(
	OBJDIR_T *pObj,
	UNSIGNED16 index	/**< main-index */
	CO_COMMA_LINE_PARA_DECL
	)
{
UNSIGNED8 retVal = 0u;

#ifdef CONFIG_MULT_LINES
    CO_INTERNAL_NOT_USED(CO_LINE_PARA);
#endif /* CONFIG_MULT_LINES */
    CO_INTERNAL_NOT_USED(index);

    if (pObj != NULL) {
	retVal = CO_READ_OD8(pObj->numOfElem);
    }
    return retVal;
}

/*******************************************************************/
/**
* \private
*
* getObjPtrAttr - get pointer at object attributes
*
* This function gets a pointer at the object attribute.
* This pointer is only valid if the return value of this function is CO_OK.
*
* return CANopen return value
*/
RET_T getObjPtrAttr(
    OBJDIR_T *pObj,           /* poiter at object entry in object dictionary */
    UNSIGNED16 index,         /* main-index of the object */
    UNSIGNED8 subIndex,       /* sub-index of the object */
    UNSIGNED16 *pAttribute    /* pointer at object attributes */
    CO_COMMA_LINE_PARA_DECL   /* number of CAN line 0..CO_MAX_CAN_LINES-1 */
    )
{
RET_T retVal = CO_OK;

    if ( pAttribute == NULL ) {
        retVal = CO_E_PARA_INCOMP;
        return retVal;
    }

    if ( pObj == NULL ) {
#ifdef CONFIG_VIRTUAL_OBJECTS
# ifdef CONFIG_DYN_MEM_ALLOC
# else /* CONFIG_DYN_MEM_ALLOC */
	/* virtual objects are allowed only in manufacturer or device profile area */
	if ((index < START_MANU_PROF) || (index > END_DEVICE_PROF)) {
	    *pAttribute = 0u;
            retVal = CO_OK;
            return retVal;
	} else
# endif /* CONFIG_DYN_MEM_ALLOC */
	{
	    *pAttribute = getVirtualObjAttr(index, subIndex CO_COMMA_LINE_PARA);
            retVal = CO_OK;
            return retVal;
	}
#else /* CONFIG_VIRTUAL_OBJECTS */
        retVal = CO_E_PARA_INCOMP;
        return retVal;
#endif /* CONFIG_VIRTUAL_OBJECTS */
    }

    if ( (subIndex + 1u) > getObjPtrNumElem(pObj, index CO_COMMA_LINE_PARA) ) {
        retVal = CO_E_NONEXIST_SUBINDEX;
        return retVal;
    }

    /* test for short arrays */
    if ((subIndex != 0u)
     && ((CO_READ_OD_DESC_ATTR(pObj, 0u) & CO_SHORT_ARRAY_DESC)
		!= 0u)) {
	*pAttribute = CO_READ_OD_DESC_ATTR(pObj, 1u);
    } else {
	*pAttribute = CO_READ_OD_DESC_ATTR(pObj, subIndex);
    }

    return retVal;
}

/****************************************************************************/
/**
* \public
*
*++ \brief getObjPtrIndexValue - function to return the index of a object pointer
*-- \brief getObjPtrIndexValue - Funktion gibt Index eines Objektpointers zurück
*
*++ The function delivers the index of an object pointer.
*-- Diese Funktion gibt den Index eines Objektpointers zurück.
*
* \return
* index number
*/
UNSIGNED16 getObjPtrIndexValue(
    OBJDIR_T *pObj           /**< poiter at object entry in object dictionary */
    CO_COMMA_LINE_PARA_DECL  /**< number of CAN line 0..CO_MAX_CAN_LINES-1 */
    )
{
UNSIGNED16 index = 0u;

#ifdef CONFIG_MULT_LINES
    CO_INTERNAL_NOT_USED(CO_LINE_PARA);
#endif /* CONFIG_MULT_LINES */

    if ( pObj != NULL ) {
	index = CO_READ_OD16(pObj->index);
    }

    return index;
}



/****************************************************************************/
/**
* \public
*
*++ \brief putObjPtr - alternativ function to putObj
*-- \brief putObjPtr - alternative Funktion zu putObj
*
*++ The function copies the data into the object
*++ referenced by \em index and \em subIndex.
*++ The limits of indices are tested and also
*++ the write permission for remote access to the object.
*++ If the \em sub-index equals zero
*++ then the first element or the whole structure/array
*++ will be put into the dictionary.
*++ The parameter
*++ \b size specifies the size of the data in bytes.
*-- Die Funktion kopiert die übergebenen Daten in das
*-- durch \em index und \em subIndex angegebene Objekt.
*-- Sie testet den übergebenen Index auf die Grenzwerte und
*-- bei \em remote Zugriffen die Schreiberlaubnis
*-- für das Objekt.
*-- Der Parameter \b size gibt die Größe, der zu kopierenden
*-- Daten, in Bytes an.
*
* \par Endianess
*++ On BIG_ENDIAN machines it converts the data,
*++ if parameter
*++ \b local == \c CO_FALSE.
*-- Bei BIG_ENDIAN Prozessoren erfolgt eine Datenwandlung,
*-- wenn der Parameter
*-- \b local == \c CO_FALSE gesetzt ist.
*
*-- Wenn das \c #define \c CONFIG_VIRTUAL_OBJECTS gesetzt ist,
*-- und das Objekt nicht im herstellerspezifischen oder profilspezifischem Bereich des Objektverzeichnis
*-- vorhanden ist, wird die Funktion getVirtualObjAddr()
*-- aufgerufen.
*-- Sie besitzt dieselben Parameter und Rückgabewerte wie getObjAddr(),
*-- ermöglicht dem Anwender aber die Nutzung virtueller Objekte.
*-- Der Anwender ist für die korrekte Artbeitsweise der Funktion verantwortlich.
*-- In  putObj() wird auf die Adresse
*-- des virtuellen Objekts
*-- die mit \em pData übergebenen Daten geschrieben.
*-- Als Datenlänge wird immer die von getVirtualObjAddr()
*-- erhaltene Länge genutzt.
*-- Der Anwender ist dafür verantwortlich,
*-- dass die übergebene Datenlänge korrekt ist.
*
*++ If \c CONFIG_VIRTUAL_OBJECTS  is \c #defined
*++ and the adressed object is \b not in the manufacturer specific or profile specific area
*++ of the object dictionary then the function
*++ getVirtualObjAddr() is called.
*++ This function has the same parameters and return values as getObjAddr(),
*++ but enables the user to have so-called virtual objects
*++ in the object dictionary.
*++ The user is responsible for correct coding of this function.
*++ putObj() uses the address and data size information
*++ of an \em virtual \em object returned by getVirtualObjAddr()
*++ to write the data \em pData is pointing to to this address.
*++ The user is responsible for correct data size information.
*
* \retval CO_OK
*++ success
*-- Erfolg
* \retval CO_E_NONEXIST_OBJECT
*++ object doesn't exist
*-- das angegebene Objekt existiert nicht
* \retval CO_E_NO_WRITE_PERM
*++ no write permission
*-- keine Schreiberlaubnis für dieses Objekt
* \retval CO_E_NONEXIST_SUBINDEX
*++ sub-index doesn't exist
*-- der angegebene Subindex existiert nicht
* \retval CO_E_VALUE_TO_LOW
*++ value is too low
*-- der zu schreibende Wert liegt unter dem Limit
* \retval CO_E_VALUE_TO_HIGH
*++ value is too high
*-- der zu schreibende Wert liegt über dem Limit
* \retval CO_E_WRONG_SIZE
*++ size of data has wrong size
*-- falsche Datengröße
*
*/
RET_T putObjPtr(
	OBJDIR_T *pObj,		/**< pointer to object */
	UNSIGNED16 index,	/**< main-index */
	UNSIGNED8  subIndex,	/**< sub-index */
	UNSIGNED8  *pData,	/**< data address */
	UNSIGNED32 size,	/**< data size */
	BOOL_T     local	/**< data only for local usage */
	CO_COMMA_LINE_PARA_DECL	/**< number of CAN line 0..CO_MAX_CAN_LINES-1 */
	)
{
	UNSIGNED8  	*address;	/* adress pointer */
	UNSIGNED8	subIndexDesc;	/* Subindex for description */
	UNSIGNED16	attr;		/* object attributs */
#if defined(CONFIG_LIMITS_CHECK) || defined(CONFIG_VIRTUAL_OBJECTS)
	RET_T		retVal;		/* return value */
#endif /* defined(CONFIG_LIMITS_CHECK) || defined(CONFIG_VIRTUAL_OBJECTS) */
#ifdef CONFIG_VIRTUAL_OBJECTS
	UNSIGNED32	vSize;		/* object size */
#endif /* CONFIG_VIRTUAL_OBJECTS */
#if defined(CONFIG_BIG_ENDIAN) || defined(CONFIG_16BIT_CPU)
	/* buffer for converted data, if BIG_ENDIAN machine */
	static UNSIGNED8	convBuffer[CO_MAX_NUMDATA_SIZE];
#endif /* defined(CONFIG_BIG_ENDIAN) || defined(CONFIG_16BIT_CPU) */

#ifdef CONFIG_MULT_LINES
	CO_INTERNAL_NOT_USED(canLine);
#endif /* CONFIG_MULT_LINES */

	if (pObj == NULL)	{		/* object doesn't exist */

#ifdef CONFIG_VIRTUAL_OBJECTS
# ifdef CONFIG_DYN_MEM_ALLOC
# else /* CONFIG_DYN_MEM_ALLOC */
		/* virtual objects are allowed only in manufacturer or device profile area */
		if ((index < START_MANU_PROF) || (index > END_DEVICE_PROF)) {
			return(CO_E_NONEXIST_OBJECT);
		}
		else
# endif /* CONFIG_DYN_MEM_ALLOC */
		{
			/* transfer given size to this function */
			vSize = size;
			retVal = getVirtualObjAddr(index, subIndex, &address, &vSize
				CO_COMMA_LINE_PARA);
			if (retVal == CO_OK)  {
				/* copy data */
# if defined(CONFIG_BIG_ENDIAN) || defined(CONFIG_16BIT_CPU)
				if (local == CO_TRUE) {
					CO_NUM_MEMCPY(address, pData, vSize, CO_NUM_VAL);
				}
				else {
					CO_PACK_MEMCPY(address, pData, vSize, CO_NUM_VAL);
				}
# else /* defined(CONFIG_BIG_ENDIAN) || defined(CONFIG_16BIT_CPU) */
				CO_MEMCPY(address, pData, vSize);
# endif /* defined(CONFIG_BIG_ENDIAN) || defined(CONFIG_16BIT_CPU) */
			}
			return(retVal);
		}
#else /* CONFIG_VIRTUAL_OBJECTS */
		return(CO_E_NONEXIST_OBJECT);
#endif /* CONFIG_VIRTUAL_OBJECTS */
	}

	if (CO_READ_OD8(pObj->numOfElem) <= subIndex)  {
		/* sub-index does not exist */
		return(CO_E_NONEXIST_SUBINDEX);
	}

	/* test for short arrays */
	if ((subIndex != 0u)
		&& ((CO_READ_OD_DESC_ATTR(pObj, 0) & CO_SHORT_ARRAY_DESC) != 0u)){
		subIndexDesc = 1u;
	}
	else  {
		subIndexDesc = subIndex;
	}

	attr = CO_READ_OD_DESC_ATTR(pObj, subIndexDesc);

	/* security checks only for remote access */
	if (local == CO_FALSE) {

		/* test the write permission */
		if ((attr & CO_WRITE_PERM) != CO_WRITE_PERM) {
			return(CO_E_NO_WRITE_PERM);
		}

# if defined(CONFIG_BIG_ENDIAN) || defined(CONFIG_16BIT_CPU)
		/* convert data into internal format for numerical values */
		if ((attr & CO_NUM_VAL) != 0) {
			/* special handling for signed 1 byte values */
			if (CO_READ_OD_DESC_U8(pObj, 0, varType) == CO_TYPEDESC_INTEGER8){
				CO_PACK_MEMCPY(convBuffer, pData, size, CO_8BIT_SIGNED_VAL);
				pData = convBuffer;
			}
			else {
#ifdef CONFIG_16BIT_CPU
				if (CO_READ_OD_DESC_U8(pObj, subIndex, varType) != CO_TYPEDESC_UNSIGNED64)
#endif /* CONFIG_16BIT_CPU */
				{
					CO_PACK_MEMCPY(convBuffer, pData, size, attr & CO_NUM_VAL);
					pData = convBuffer;
				}
			}
		}
# endif /* defined(CONFIG_BIG_ENDIAN) || defined(CONFIG_16BIT_CPU) */

#ifdef CONFIG_LIMITS_CHECK
		retVal = checkObjLimits( (LIST_ELEMENT_T *)pObj, subIndexDesc, pData);
		if (retVal != CO_OK)  {
			return(retVal);
		}
#endif /* CONFIG_LIMITS_CHECK */
	}

	/* get address of sub-index */
	if (subIndex == 0u)  {
		address = CO_READ_ODP(pObj->pObj);

		/* if there are more subIndizes ,
		* sub-index 0 has per definition only 1 byte */
#ifdef CONFIG_BIG_ENDIAN
		if (pObj->numOfElem > 1)  {
			address = CO_READ_ODP(pObj->pObj) + getObjSize(pObj, 0) - 1;
		}
#endif /* CONFIG_BIG_ENDIAN */

	}
	else  {
		address = (UNSIGNED8 *)getSubIndexAddr(pObj, subIndex);
	}

	if ((CO_READ_OD_DESC_ATTR(pObj, subIndexDesc)
		& CO_UP_DN_LD_DOMAIN) != 0u) {
		/* DOMAIN_T is pointer of void !! */
		address = (UNSIGNED8 *)(*(DOMAIN_T *)address);
	}

	/* allocate security mechanism for object dictionary consistency */
	if (index < START_MANU_PROF) {
		CO_COM_PART_ALLOC(CO_LINE_PARA);
	}
	else {
		CO_APPL_PART_ALLOC(CO_LINE_PARA);
	}

        /* if object is a string, update actual length */
        if ((CO_READ_OD_DESC_ATTR(pObj, subIndexDesc)
                & CO_UP_DN_LD_STRING) != 0u)
        {
#ifdef CO_CONFIG_SDO_SHORT_STRINGS
            setStringSize(index,subIndexDesc,size CO_COMMA_LINE_PARA);
#endif /* CO_CONFIG_SDO_SHORT_STRINGS */
        }
	/* copy to dictionary */
	CO_NUM_MEMCPY(address, pData, size, attr & CO_NUM_VAL);

	/* release security mechanism for object dictionary consistency */
	if (index < START_MANU_PROF) {
		CO_COM_PART_RELEASE(CO_LINE_PARA);
	}
	else {
		CO_APPL_PART_RELEASE(CO_LINE_PARA);
	}

	return(CO_OK);
}


/****************************************************************************/
/**
* \private
*
* getObjPtrEntry - ???
*
*/
RET_T getObjPtrEntry(
	OBJDIR_T *pObj,
	UNSIGNED16 index,	/* main-index */
	UNSIGNED8  subIndex,	/* sub-index */
	UNSIGNED8  *pData,	/* destination for data */
	UNSIGNED32 *pSize,	/* destination for data size */
	BOOL_T	   local	/* data only for local usage */
	CO_COMMA_LINE_PARA_DECL	/* number of CAN line 0..CO_MAX_CAN_LINES-1 */
)
{
RET_T retVal = CO_OK;
UNSIGNED16 attr;
UNSIGNED8	*ppData;	/* pointer to pointer of data */

#ifndef CONFIG_DYN_MEM_ALLOC
    if ( pObj == NULL ) {
        retVal = CO_E_NONEXIST_OBJECT;
        return retVal;
    }
#endif /* CONFIG_DYN_MEM_ALLOC */

    retVal = getObjPtrAddr(pObj, index, subIndex, &ppData, pSize CO_COMMA_LINE_PARA);
    if (retVal != CO_OK)  {
        retVal = CO_E_NONEXIST_OBJECT;
	return retVal;
    }

    retVal = getObjPtrAttr( pObj, index, subIndex, &attr CO_COMMA_LINE_PARA );
    if ( retVal != CO_OK ) {
        return retVal;
    }

    /* security checks only for remote access */
    if (local == CO_FALSE) {
	if ((attr & CO_READ_PERM) != CO_READ_PERM) {
	    return(CO_E_NO_READ_PERM);
        }
    }


    if ((attr & CO_UP_DN_LD_DOMAIN) != 0u) {
	pData = ppData;
    } else {
	/* allocate security mechanism for object dictionary consistency */
	if (index < START_MANU_PROF) {
	    CO_COM_PART_ALLOC(CO_LINE_PARA);
	} else {
	    CO_APPL_PART_ALLOC(CO_LINE_PARA);
	}
	/* copy only numeric values */
	if ((attr & CO_NUM_VAL) != 0u) {
	    /* get address of sub-index */
# if defined(CONFIG_BIG_ENDIAN) || defined(CONFIG_16BIT_CPU)
	    if (local == CO_TRUE) {
		CO_NUM_MEMCPY(pData, ppData, *pSize, attr & CO_NUM_VAL);
	    } else {
		CO_UNPACK_MEMCPY(pData, ppData, *pSize, attr & CO_NUM_VAL);
	    }
# else /* defined(CONFIG_BIG_ENDIAN) || defined(CONFIG_16BIT_CPU) */
	    CO_MEMCPY(pData, ppData, *pSize);
# endif /* defined(CONFIG_BIG_ENDIAN) || defined(CONFIG_16BIT_CPU) */
	} else {
	    /* return pointer */
	    pData = ppData;
	}
	/* release security mechanism for object dictionary consistency */
	if(index < START_MANU_PROF) {
	    CO_COM_PART_RELEASE(CO_LINE_PARA);
	} else {
	    CO_APPL_PART_RELEASE(CO_LINE_PARA);
	}
    }

    retVal = CO_OK;
    return retVal;

}


/****************************************************************************/
/**
* \public
*
*++ \brief getObjPtrAddr - alternative function to getObjAddr if the pointer to the object is known
*-- \brief getObjPtrAddr - Alternativfunktion zu getObjAddr wenn der Pointer zum Objekt schon bekannt ist
*
*++ The function delivers the address of an object
*++ referenced by \em pObj and \em subIndex.
*++ It tests only the limit of the provided indices.
*-- Die Funktion ermittelt die Adresse des über \em pObj und \em subIndex
*-- referenzierten Objektes des Objektverzeichnisses.
*-- Der übergebene Index wird auf das zulässige Grenzwerte überprüft.
*-- Wenn das \c #define \c CONFIG_VIRTUAL_OBJECTS gesetzt ist,
*-- und das Objekt \b nicht
*-- im herstellerspezifischen oder profilspezifischem Bereich des Objektverzeichnis
*-- vorhanden ist, wird die Funktion \em getVirtualObjectAddr() aufgerufen.
*++ If \c CONFIG_VIRTUAL_OBJECTS  is \c #defined
*++ and the adressed object is \b not in the manufacturer specific or device profile specific area
*++ of the object dictionary then the function
*++ getVirtualObjectAddr() is called.
*
*-- Sie besitzt dieselben Parameter und Rückgabewerte wie \em getObjAddr(),
*-- ermöglicht dem Anwender aber die Nutzung virtueller Objekte.
*-- Der Anwender ist für die korrekte Artbeitsweise der Funktion verantwortlich.
*++ This function has the same parameters and return values as \em getObjAddr(),
*++ but enables the user to have so-called virtual objects
*++ in the object dictionary.
*++ The user is responsible for correct coding of this function.
*
* \code
* UNSIGNED8 data[4];
* UNSIGNED8 *pData;
* UNSIGNED32 size;
* OBJDIR_T *pointerToObj;
*
* // get address and size of Object 0x2000:1
* pointerToObj =  searchObj(0x2000 CO_COMMA_LINE_PARA);
*
* getObjAddr( pointerToObj ,0x2000, 1, &pData, &size);
* // copy Object 0x2000 to local array data[]
* memcpy(&data[0], pData, size);
* \endcode
*
* \retval OK
*++ success
*-- Erfolg
* \retval CO_E_NONEXIST_OBJECT
*++ object doesn't exist
*-- Das angegebene Objekt existiert nicht
* \retval CO_E_NONEXIST_SUBINDEX
*++ sub-index doesn't exist
*-- Der angegebene Subindex existiert nicht
*
*/

RET_T getObjPtrAddr(
	OBJDIR_T *pObj,	 	/**< pointer to main object */
	UNSIGNED16 index,	/**< main-index */
	UNSIGNED8  subIndex,	/**< sub-index */
	UNSIGNED8  **pData,	/**< destination for data address*/
	UNSIGNED32 *pSize	/**< destination for data size */
	CO_COMMA_LINE_PARA_DECL	/**< number of CAN line 0..CO_MAX_CAN_LINES-1 */
    )
{
UNSIGNED8    subIndexDesc;	/* Subindex for description */
UNSIGNED8    varType = 0;

#ifdef CONFIG_MULT_LINES
    CO_INTERNAL_NOT_USED(canLine);
#endif /* CONFIG_MULT_LINES */


    if (pObj == NULL)	{
	/* object doesn't exist */

#ifdef CONFIG_VIRTUAL_OBJECTS
# ifdef CONFIG_DYN_MEM_ALLOC
	return(getVirtualObjAddr(index, subIndex, pData, pSize
		CO_COMMA_LINE_PARA));
# else /* CONFIG_DYN_MEM_ALLOC */
	/* virtual objects are allowed only in manufacturer and device areas */
	if ((index < START_MANU_PROF) || (index > END_DEVICE_PROF)) {
	    return(CO_E_NONEXIST_OBJECT);
	} else  {
	    return(getVirtualObjAddr(index, subIndex, pData, pSize
		CO_COMMA_LINE_PARA));
	}
# endif /* CONFIG_DYN_MEM_ALLOC */
#else /* CONFIG_VIRTUAL_OBJECTS */
	return(CO_E_NONEXIST_OBJECT);
#endif /* CONFIG_VIRTUAL_OBJECTS */
    }

    CO_INTERNAL_NOT_USED(index);
    CO_INTERNAL_NOT_USED(varType);

    if (CO_READ_OD8(pObj->numOfElem) <= subIndex)  {
	/* sub-index does not exist*/
	return(CO_E_NONEXIST_SUBINDEX);
    }

    /* test for short array description */
    if ((subIndex != 0u)
     && ((CO_READ_OD_DESC_ATTR(pObj, 0u) & CO_SHORT_ARRAY_DESC) != 0u)) {
	subIndexDesc = 1u;
    } else {
	subIndexDesc = subIndex;
    }

    *pSize = getObjSize(pObj, subIndexDesc);

    /* get address of sub-index */
    if (subIndex == 0u)  {

	*pData = CO_READ_ODP(pObj->pObj);

	/* if there are more subIndizes */
	if (CO_READ_OD8(pObj->numOfElem) > 1u)  {
#ifdef CONFIG_BIG_ENDIAN
	    *pData = CO_READ_ODP(pObj->pObj) + *pSize - 1;
#endif /* CONFIG_BIG_ENDIAN */
	    /* sub-index 0 has per definition only 1 byte */
	    *pSize = 1u;
	    /* port - workaround for real32 datatype */
#ifdef CONFIG_FLOAT_VALUES
            varType = CO_READ_OD_DESC_U8(pObj, 0u, varType);
            if (varType == CO_TYPEDESC_REAL32)  {
                UNSIGNED8 u8_num = 0u;
                UNSIGNED8 *pRealData = (UNSIGNED8 *)getSubIndexAddr(pObj, 0u);
                u8_num = (CO_READ_OD8(pObj->numOfElem)) - 1;
                memset(pRealData, u8_num, sizeof(REAL32_T));
            }
#endif /* CONFIG_FLOAT_VALUES */
            /* port - workaround end */
	}

    } else  {
	/* sub-index > 0 */
	*pData = (UNSIGNED8 *)getSubIndexAddr(pObj, subIndex);
    }

    return(CO_OK);
}


/*******************************************************************/
/**
* \public
*
*++ \brief getObjStoreEnableReq - get nonvolatile store information
*-- \brief getObjStoreEnableReq - Eigenschaft nichtflüchtige Speicherung abfragen
*
*++ This function returns the property "Nonvolatile storage".
*++ This property is specified in the CANopen Design Tool
*++ about Line / Object Dictionary / * Segment / object sub-index / tab Structure /
*++ Nonvolatile storage and is included as object attribute in the generated
*++ object dictionary. The application is responsible for the nonvolatile storage
*++ of the object.
*-- Mit dieser Funktion kann die Eigenschaft "Nichtflüchtige Speicherung" des Objektes
*-- abgefragt werden. Diese Eigenschaft kann mit dem CANopen Design Tool eingestellt
*-- werden über: Line / Object Dictionary / * Segment / object sub-index / tab Structure /
*-- Nonvolatile storage. Diese Einstellung wird als Attribut des Objektes im
*-- Objektverzeichnis bei der Generierung hinterlegt. Die Appliakation ist für die
*-- nichtflüchtige Abspeicherung des Objektes verantwortlich.
*
*++ \return CO_OK: the object is marked for store
*++ \return CO_E_NONEXIST_OBJECT: the object does not exist
*++ \return CO_E_NONEXIST_SUBINDEX: the sub-index does not exist
*++ \return CO_E_NO_DATA_AVAILABLE: the object is not marked for store
*-- \return CO_OK: das Objekt wird nichtflüchtig gespeichert
*-- \return CO_E_NONEXIST_OBJECT: das Objekt mit dem Main-Index existiert nicht
*-- \return CO_E_NONEXIST_SUBINDEX: der Sub-Index des Objektes existiert nicht
*-- \return CO_E_NO_DATA_AVAILABLE: das Objekt wird nicht nichtflüchtig gespeichert
*
*/
RET_T getObjStoreEnableReq(
    UNSIGNED16 index,             /**< main-index of the object */
    UNSIGNED8 subIndex            /**< sub-index of the object */
    CO_COMMA_LINE_PARA_DECL       /**< number of CAN line 0..CO_MAX_CAN_LINES-1 */
    )
{
RET_T retVal = CO_E_NONEXIST_OBJECT;
LIST_ELEMENT_T *curObj;		/* pointer to current object */

    /* index value exceeds the physical limitations */
    curObj = searchObj(index CO_COMMA_LINE_PARA);

    if ( curObj == NULL ) {
        return retVal;
    }

    retVal = getObjPtrStoreEnableReq(curObj, index, subIndex CO_COMMA_LINE_PARA );

    return retVal;
}


/*******************************************************************/
/**
* \private
*
* getObjPtrStoreEnableReq - get nonvolatile store information
*
* Alternative function to getObjStoreEnableReq, if the object pointer
* is already known.
*
* return CO_OK, the should be stored
* return CO_E_NONEXIST_OBJECT, the object does not exist
* return CO_E_NONEXIST_SUBINDEX, the sunindex does not exist
* return CO_E_NO_DATA_AVAILABLE, the object is not marked for store
*
*/
RET_T getObjPtrStoreEnableReq(
        OBJDIR_T *pObj,           /* pointer at the object in the object dictionary */
        UNSIGNED16 index,         /* main-index of the object */
        UNSIGNED8 subIndex        /* sub-index of the object */
        CO_COMMA_LINE_PARA_DECL   /* number of CAN line 0..CO_MAX_CAN_LINES-1 */
   )
{
RET_T retVal = CO_E_NONEXIST_OBJECT;
UNSIGNED16 attr = 0u;

    CO_INTERNAL_NOT_USED(index);
#ifdef CONFIG_MULT_LINES
    CO_INTERNAL_NOT_USED(canLine);
#endif /* CONFIG_MULT_LINES */

    if ( pObj == NULL ) {
        return retVal;
    }

    if (CO_READ_OD8(pObj->numOfElem) <= subIndex)  {
	/* sub-index does not exist*/
        retVal = CO_E_NONEXIST_SUBINDEX;
	return retVal;
    }

    attr = CO_READ_OD_DESC_ATTR(pObj, subIndex);

    if ( ( attr & CO_OBJ_ATTR_SAVE ) > 0u ) {
        retVal = CO_OK;
    } else {
        retVal = CO_E_NO_DATA_AVAILABLE;
    }

    return retVal;
}




#ifdef CO_CONFIG_ENABLE_OBJ_CALLBACK
/*******************************************************************/
/**
* \private
*
* getObjFuncPtr - searches for function pointer of object
*
* This function searches for the callback function pointer of an object.
*
* return The function pointer
*
*/
CO_OBJ_CB_T getObjFuncPtr(
	UNSIGNED16 index          /* main-index of the object */
	CO_COMMA_LINE_PARA_DECL   /* number of CAN line 0..CO_MAX_CAN_LINES-1 */
    )
{
LIST_ELEMENT_T	*pObjEntry;	/* pointer to object entry */
CO_OBJ_CB_T retVal = NULL;

    /* look for object */
    pObjEntry = searchObj(index CO_COMMA_LINE_PARA);
    if (pObjEntry != NULL) {
	retVal = pObjEntry->pObjCallback;
    }

    return retVal;
}
#endif /*CO_CONFIG_ENABLE_OBJ_CALLBACK*/


#ifdef CO_CONFIG_ENABLE_OBJ_CALLBACK
/*******************************************************************/
/**
* \private
*
* getObjFuncPtr - searches for function pointer of object
*
* This function searches for the function pointer of an object.
*
* return
*
*/
CO_OBJ_CB_T getObjPtrFuncPtr(
	OBJDIR_T *pObj,           /* pointer at the object in the object dictionary */
	UNSIGNED16 index          /* main-index of the object */
	CO_COMMA_LINE_PARA_DECL   /* number of CAN line 0..CO_MAX_CAN_LINES-1 */
    )
{
CO_OBJ_CB_T retVal = NULL;

    CO_INTERNAL_NOT_USED(index); /* Not used yet */
#ifdef CONFIG_MULT_LINES
    CO_INTERNAL_NOT_USED(canLine);
#endif /* CONFIG_MULT_LINES */

    if (pObj != NULL) {
	retVal = pObj->pObjCallback;
    }

    return retVal;
}
#endif /*CO_CONFIG_ENABLE_OBJ_CALLBACK*/


#ifdef CO_CONFIG_ENABLE_OBJ_CALLBACK
/*******************************************************************/
/**
* \private
*
* getObjFuncPtrAddr - searches for function pointer of object
*
* This function searches for the function pointer of an object and
* returns the address.
*
* return
*
*/
CO_OBJ_CB_T *getObjFuncPtrAddr(
	UNSIGNED16 index          /* main-index of the object */
	CO_COMMA_LINE_PARA_DECL   /* number of CAN line 0..CO_MAX_CAN_LINES-1 */
    )
{
LIST_ELEMENT_T	*pObjEntry;	/* pointer to object entry */
CO_OBJ_CB_T *retVal = NULL;

    /* look for object */
    pObjEntry = searchObj(index CO_COMMA_LINE_PARA);
    if (pObjEntry != NULL) {
	retVal = &pObjEntry->pObjCallback;
    }

    return retVal;
}
#endif /*CO_CONFIG_ENABLE_OBJ_CALLBACK*/


#ifdef CO_CONFIG_ENABLE_OBJ_CALLBACK
/*******************************************************************/
/**
* \private
*
* getObjPtrFuncPtrAddr - searches for function pointer of object
*
* This function searches for the function pointer of an object and
* returns the address.
*
*/
CO_OBJ_CB_T *getObjPtrFuncPtrAddr(
	OBJDIR_T *pObj,
	UNSIGNED16 index           /* main-index of the object */
	CO_COMMA_LINE_PARA_DECL    /* number of CAN line 0..CO_MAX_CAN_LINES-1 */
    )
{
CO_OBJ_CB_T *retVal = NULL;

    CO_INTERNAL_NOT_USED(index); /* Not used yet */
#ifdef CONFIG_MULT_LINES
    CO_INTERNAL_NOT_USED(canLine);
#endif /* CONFIG_MULT_LINES */

    if (pObj != NULL) {
	retVal = (CO_OBJ_CB_T *)&pObj->pObjCallback;
    }

    return retVal;
}
#endif /*CO_CONFIG_ENABLE_OBJ_CALLBACK*/


#ifdef CO_CONFIG_ENABLE_OBJ_CALLBACK
/*******************************************************************/
/**
* \private
*
* setObjFuncPtr - sets function pointer of object
*
* This function sets the function pointer of an object.
*
*/
RET_T setObjFuncPtr(
	UNSIGNED16 index,          /* main-index of the object */
        CO_OBJ_CB_T pNewFunc       /* new function pointer for object */
	CO_COMMA_LINE_PARA_DECL    /* number of CAN line 0..CO_MAX_CAN_LINES-1 */
    )
{
LIST_ELEMENT_T	*pObjEntry;	/* pointer to object entry */
RET_T retVal = CO_E_NONEXIST_OBJECT;

    /* look for object */
    pObjEntry = searchObj(index CO_COMMA_LINE_PARA);
    if (pObjEntry != NULL) {
	pObjEntry->pObjCallback = pNewFunc;
        retVal = CO_OK;
    }

    return retVal;
}
#endif /*CO_CONFIG_ENABLE_OBJ_CALLBACK*/

/* \public */
/*______________________________________________________________________EOF_*/
