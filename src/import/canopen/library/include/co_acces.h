/*
 * access - defines for access to object dictionary
 *
 * Copyright (c) 2001-2017 port GmbH Halle/Saale
 *------------------------------------------------------------------
 */

/**
*  \file co_acces.h
*++ Defines for access to object dictionary
*-- Definitionen für den Zugriff auf das Objectverzeichnis
*  \author port GmbH Halle (Saale)
*/

#ifndef __CO_ACCES_H
# define __CO_ACCES_H

#include <co_def.h>


/* value description for each entry */
typedef struct  {
    BOOL_T	minRange;	/* min. range of objectelement */
    BOOL_T	maxRange;	/* max. range of objectelement */
} LIMIT_BOOL_T;

typedef struct  {
    UNSIGNED8	minRange;	/* min. range of objectelement */
    UNSIGNED8	maxRange;	/* max. range of objectelement */
} LIMIT_U8_T;

typedef struct  {
    UNSIGNED16	minRange;	/* min. range of objectelement */
    UNSIGNED16	maxRange;	/* max. range of objectelement */
} LIMIT_U16_T;

typedef struct  {
    UNSIGNED32	minRange;	/* min. range of objectelement */
    UNSIGNED32	maxRange;	/* max. range of objectelement */
} LIMIT_U32_T;

typedef struct  {
    INTEGER8	minRange;	/* min. range of objectelement */
    INTEGER8	maxRange;	/* max. range of objectelement */
} LIMIT_I8_T;

typedef struct  {
    INTEGER16	minRange;	/* min. range of objectelement */
    INTEGER16	maxRange;	/* max. range of objectelement */
} LIMIT_I16_T;

typedef struct  {
    INTEGER32	minRange;	/* min. range of objectelement */
    INTEGER32	maxRange;	/* max. range of objectelement */
} LIMIT_I32_T;

typedef struct  {
    REAL32	minRange;	/* min. range of objectelement */
    REAL32	maxRange;	/* max. range of objectelement */
} LIMIT_R32_T;

typedef struct  {
    UNSIGNED32	actLen;		/* actual string length */
    UNSIGNED32	maxLen;		/* max. string length */
} STRING_DATA_T;

typedef struct  {
    UNSIGNED32	len;		/* domain length */
} DOMAIN_DATA_T;


#ifdef CONFIG_EXTENDED_DATA_TYPES
typedef struct  {
    UNSIGNED24	minRange;	/* min. range of objectelement */
    UNSIGNED24	maxRange;	/* max. range of objectelement */
} LIMIT_U24_T;

typedef struct  {
    UNSIGNED40	minRange;	/* min. range of objectelement */
    UNSIGNED40	maxRange;	/* max. range of objectelement */
} LIMIT_U40_T;

typedef struct  {
    UNSIGNED48	minRange;	/* min. range of objectelement */
    UNSIGNED48	maxRange;	/* max. range of objectelement */
} LIMIT_U48_T;

typedef struct  {
    UNSIGNED56	minRange;	/* min. range of objectelement */
    UNSIGNED56	maxRange;	/* max. range of objectelement */
} LIMIT_U56_T;

typedef struct  {
    UNSIGNED64	minRange;	/* min. range of objectelement */
    UNSIGNED64	maxRange;	/* max. range of objectelement */
} LIMIT_U64_T;

typedef struct  {
    INTEGER64	minRange;	/* min. range of objectelement */
    INTEGER64	maxRange;	/* max. range of objectelement */
} LIMIT_I64_T;
#endif /* CONFIG_EXTENDED_DATA_TYPES */


typedef struct
{
    UNSIGNED8	*pDefaultVal;	/* default value or size for domains */
#ifdef CONFIG_LIMITS_CHECK
    LIMIT_U8_T	*pLimits;	/* pointer to limits */
#endif
    UNSIGNED8	varType;	/* variable type */
    UNSIGNED16  attribute;	/* domain type = 1, short desc = 2,
				   float = 4, num_val = 0x10,
				   read permitted = 0x20,
				   write permitted = 0x40,
				   pdoMAPPING allowed = 0x80 bitcoded ! */
} VALUE_DESC_T;

#ifdef CO_CONFIG_ENABLE_OBJ_CALLBACK

# ifdef CO_CONFIG_ENABLE_EXTOBJ_CALLBACK
typedef struct
{
	UNSIGNED8 *pData;
	UNSIGNED32 dataSize;
}CO_OBJ_CB_TYPE_EXTOBJ_T;
# endif /* CO_CONFIG_ENABLE_EXTOBJ_CALLBACK */

typedef struct
{
    UNSIGNED16 reason;
    UNSIGNED16 serviceNbr;
# ifdef CO_CONFIG_ENABLE_EXTOBJ_CALLBACK
	CO_OBJ_CB_TYPE_EXTOBJ_T objAccess;
# endif /* CO_CONFIG_ENABLE_EXTOBJ_CALLBACK */
}CO_OBJ_CB_TYPE_T;

# ifdef CONFIG_NO_GLOBAL_VARS
/* The structure used in CO_COMMA_LINE_PARA_DECL contains this type */
                              /*index,    subindex,  reason,    ,    CANopenDattayType */
typedef RET_T (*CO_OBJ_CB_T)(UNSIGNED16, UNSIGNED8, CO_OBJ_CB_TYPE_T , void* );
# else /* ifdef CONFIG_NO_GLOBAL_VARS */
                              /*index,    subindex,  reason,    , canline*/
typedef RET_T (*CO_OBJ_CB_T)(UNSIGNED16, UNSIGNED8, CO_OBJ_CB_TYPE_T CO_COMMA_LINE_PARA_DECL);
# endif /* ifdef CONFIG_NO_GLOBAL_VARS */



#define CO_OBJ_CB_TYPE_PRE_SDO_READ   1u
#define CO_OBJ_CB_TYPE_POST_SDO_READ  2u
#define CO_OBJ_CB_TYPE_PRE_SDO_WRITE  3u
#define CO_OBJ_CB_TYPE_POST_SDO_WRITE 4u
#define CO_OBJ_CB_TYPE_PRE_PDO_READ   5u
#define CO_OBJ_CB_TYPE_POST_PDO_READ  6u
#define CO_OBJ_CB_TYPE_PRE_PDO_WRITE  7u
#define CO_OBJ_CB_TYPE_POST_PDO_WRITE 8u

/* Error are between 50 and 99 */
#define CO_OBJ_CB_TYPE_POST_SDO_READ_ABORT   52u
#define CO_OBJ_CB_TYPE_POST_SDO_WRITE_ABORT  54u


/* This callback types are only for completness */
#define CO_OBJ_CB_TYPE_PRE_SRDO_READ   20u
#define CO_OBJ_CB_TYPE_POST_SRDO_READ  21u
#define CO_OBJ_CB_TYPE_PRE_SRDO_WRITE  22u
#define CO_OBJ_CB_TYPE_POST_SRDO_WRITE 23u


#define CO_OBJ_CB_TYPE_PRE_APPL_READ   24u
#define CO_OBJ_CB_TYPE_POST_APPL_READ  25u
#define CO_OBJ_CB_TYPE_PRE_APPL_WRITE  26u
#define CO_OBJ_CB_TYPE_POST_APPL_WRITE 27u


#endif /*CO_CONFIG_ENABLE_OBJ_CALLBACK*/

/* defines for variable type in VALUE_DESC_T */
#define CO_TYPEDESC_BOOL		1u
#define CO_TYPEDESC_UNSIGNED8		2u
#define CO_TYPEDESC_UNSIGNED16		3u
#define CO_TYPEDESC_UNSIGNED24		4u
#define CO_TYPEDESC_UNSIGNED32		5u
#define CO_TYPEDESC_UNSIGNED40		6u
#define CO_TYPEDESC_UNSIGNED48		7u
#define CO_TYPEDESC_UNSIGNED56		8u
#define CO_TYPEDESC_UNSIGNED64		9u
#define CO_TYPEDESC_INTEGER8		10u
#define CO_TYPEDESC_INTEGER16		11u
#define CO_TYPEDESC_INTEGER32		12u
#define CO_TYPEDESC_INTEGER64		13u
#define	CO_TYPEDESC_VISSTRING		14u
#define	CO_TYPEDESC_OCTETSTRING		15u
#define	CO_TYPEDESC_DOMAIN		16u
#define	CO_TYPEDESC_REAL32		17u


/* The LIST_ELEMENT is a type for a element of the object dictionary.
   It describes the features of the dataobject which ist stored at
   the address pObj */

typedef struct
{
    UNSIGNED8    *pObj;         /* pointer to data */
    VALUE_DESC_T *pValDesc;     /* value description */
    UNSIGNED16   index;		/* index of object */
    UNSIGNED8    numOfElem; 	/* number of elements */
#ifdef CO_CONFIG_ENABLE_OBJ_CALLBACK
    CO_OBJ_CB_T  pObjCallback;  /* obj function pointer */
#endif /* CO_CONFIG_ENABLE_OBJ_CALLBACK */
} LIST_ELEMENT_T;


#ifdef CONFIG_CONST_OBJDIR
# define OBJDIR_T LIST_ELEMENT_T CO_CONST
#else
  typedef LIST_ELEMENT_T OBJDIR_T;
#endif


/* definition of object dictionary ranges */

#define START_OBJ_DIC           0x1000u
#define END_OBJ_DIC             0xFFFFu
#define START_COM_PROF          0x1000u
#define END_COM_PROF            0x1FFFu
#define START_MANU_PROF         0x2000u
#define END_MANU_PROF           0x5FFFu
#define START_DEVICE_PROF       0x6000u
#define END_DEVICE_PROF		0x9FFFu


/* defines for special in VALUE_DESC */

#define CO_UP_DN_LD_DOMAIN              ((UNSIGNED16)0x0001u)  /* domain type for up and down load */
#define CO_SHORT_ARRAY_DESC             ((UNSIGNED16)0x0002u)  /* array elements all equal to subindex 1*/
#define CO_UP_DN_LD_STRING              ((UNSIGNED16)0x0004u)  /* string type for up and down load */
#define CO_CONST_PERM                   ((UNSIGNED16)0x0008u)  /* const value */
#define CO_NUM_VAL                      ((UNSIGNED16)0x0010u)  /* numeric value (for byte swapping) */
#define CO_READ_PERM                    ((UNSIGNED16)0x0020u)  /* read permission */
#define CO_WRITE_PERM                   ((UNSIGNED16)0x0040u)  /* write permission */
#define CO_MAP_PERM                     ((UNSIGNED16)0x0080u)  /* pdo mapping permission */
#define CO_UP_DN_LD_DOMAIN_SIZELESS     ((UNSIGNED16)0x0100u)  /* domain type for up and down load */
#define CO_OBJ_ATTR_SAVE                ((UNSIGNED16)0x1000u)  /* object should be saved */


/* external variable declarations */

#endif /* __CO_ACCES_H */


#ifdef CONFIG_WITHOUT_PROTOTYPES
#else /* CONFIG_WITHOUT_PROTOTYPES */

# ifndef __CO_ACCES_PROTOTYPES_H
#  define __CO_ACCES_PROTOTYPES_H

/* function prototypes */

RET_T 		getObjEntry(UNSIGNED16, UNSIGNED8, UNSIGNED8 *, UNSIGNED32 *,
			BOOL_T CO_COMMA_LINE_PARA_DECL);
RET_T 		getObjAddr(UNSIGNED16, UNSIGNED8, UNSIGNED8 **, UNSIGNED32 *
			CO_COMMA_LINE_PARA_DECL);
UNSIGNED16  	getObjAttr(UNSIGNED16, UNSIGNED8 CO_COMMA_LINE_PARA_DECL);
BOOL_T     	setObjAttr(UNSIGNED16, UNSIGNED8, UNSIGNED16
			CO_COMMA_LINE_PARA_DECL);
UNSIGNED32 	getDomainSize(UNSIGNED16, UNSIGNED8 CO_COMMA_LINE_PARA_DECL);
BOOL_T     	setDomainSize(UNSIGNED16, UNSIGNED8, UNSIGNED32
			CO_COMMA_LINE_PARA_DECL);
BOOL_T     	getStringSize(UNSIGNED16, UNSIGNED8, UNSIGNED32* CO_COMMA_LINE_PARA_DECL);
BOOL_T     	setStringSize(UNSIGNED16, UNSIGNED8, UNSIGNED32 CO_COMMA_LINE_PARA_DECL);
UNSIGNED8 	*getDomainAddr(UNSIGNED16, UNSIGNED8 CO_COMMA_LINE_PARA_DECL);
BOOL_T     	setDomainAddr(UNSIGNED16, UNSIGNED8, UNSIGNED8 *
			CO_COMMA_LINE_PARA_DECL);
UNSIGNED8 	getNumOfElem(UNSIGNED16 CO_COMMA_LINE_PARA_DECL);
RET_T 		putObj(UNSIGNED16, UNSIGNED8 , UNSIGNED8 *, UNSIGNED32, BOOL_T
			CO_COMMA_LINE_PARA_DECL);
UNSIGNED8	getOvDataTypeLen(UNSIGNED16 index);
RET_T		setDefaultOdVal(UNSIGNED16 index, UNSIGNED8 subIndex
			CO_COMMA_LINE_PARA_DECL);
RET_T		getObjLimits(UNSIGNED16	index, UNSIGNED8 subIdx,
			UNSIGNED8 **pMinVal, UNSIGNED8 **pMaxVal
			CO_COMMA_LINE_PARA_DECL);

RET_T 		getVirtualObjAddr(UNSIGNED16, UNSIGNED8, UNSIGNED8 **,
			UNSIGNED32 * CO_COMMA_LINE_PARA_DECL);
UNSIGNED16	getVirtualObjAttr(UNSIGNED16 index, UNSIGNED8 subIndex
			CO_COMMA_LINE_PARA_DECL);
void		setNewOdPtr(OBJDIR_T *pDir, UNSIGNED16	*pCnt
			CO_COMMA_LINE_PARA_DECL);



UNSIGNED16      getMaxObjDicElements( CO_LINE_PARA_DECL );

RET_T		getObjPtrAtIndex(UNSIGNED16 index, OBJDIR_T **pObj
			CO_COMMA_LINE_PARA_DECL);

UNSIGNED8	getObjPtrNumElem(OBJDIR_T *pObj, UNSIGNED16 index CO_COMMA_LINE_PARA_DECL);

RET_T 		getObjPtrEntry(OBJDIR_T *pObj, UNSIGNED16 index, UNSIGNED8, UNSIGNED8 *, UNSIGNED32 *,
			BOOL_T CO_COMMA_LINE_PARA_DECL);

RET_T      	getObjPtrAttr(OBJDIR_T *pObj, UNSIGNED16 index, UNSIGNED8 subIndex, UNSIGNED16 *attribute
			CO_COMMA_LINE_PARA_DECL);

RET_T 		getObjPtrAddr(OBJDIR_T *pObj, UNSIGNED16 index, UNSIGNED8 subIndex,
			UNSIGNED8 **pData, UNSIGNED32 *pSize CO_COMMA_LINE_PARA_DECL );


UNSIGNED16 getObjPtrIndexValue(OBJDIR_T *pObj CO_COMMA_LINE_PARA_DECL);


RET_T 		getObjStoreEnableReq(UNSIGNED16 index, UNSIGNED8 subIndex
			CO_COMMA_LINE_PARA_DECL);

RET_T 		getObjPtrStoreEnableReq(OBJDIR_T *pObj, UNSIGNED16 index, UNSIGNED8 subIndex
        		CO_COMMA_LINE_PARA_DECL );

RET_T           putObjPtr( OBJDIR_T *pObj, UNSIGNED16 index, UNSIGNED8  subIndex,
                        UNSIGNED8  *pData, UNSIGNED32 size, BOOL_T local CO_COMMA_LINE_PARA_DECL );



#  ifdef CO_CONFIG_ENABLE_OBJ_CALLBACK
CO_OBJ_CB_T 	getObjFuncPtr( UNSIGNED16 index
			CO_COMMA_LINE_PARA_DECL);

CO_OBJ_CB_T 	getObjPtrFuncPtr(OBJDIR_T *pObj, UNSIGNED16 index
			CO_COMMA_LINE_PARA_DECL);

CO_OBJ_CB_T 	*getObjFuncPtrAddr( UNSIGNED16 index
			CO_COMMA_LINE_PARA_DECL);

CO_OBJ_CB_T 	*getObjPtrFuncPtrAddr(OBJDIR_T *pObj, UNSIGNED16 index
			CO_COMMA_LINE_PARA_DECL);

RET_T 		setObjFuncPtr( UNSIGNED16 index, CO_OBJ_CB_T pNewFunc
			CO_COMMA_LINE_PARA_DECL);
#  endif /* CO_CONFIG_ENABLE_OBJ_CALLBACK */

# endif /* __CO_ACCES_PROTOTYPES_H */
#endif /* CONFIG_WITHOUT_PROTOTYPES */

/* end of source */
