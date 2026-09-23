/*
 * access - defines for access to object dictionary
 *
 * Copyright (c) 2001-2017 port GmbH Halle/Saale
 *------------------------------------------------------------------
 */

/*
DESCRIPTION

The file contains definitions of structures and complex data types
for access to object dictionary

*/

#include <co_acces.h>		/* include public definition */


#ifndef PCO_ACCESS_H__
# define PCO_ACCESS_H__



/* read values from object dictionary
 * for some controllers we need a wrapper function,
 * when the od is stored at ROM */
#ifndef CO_READ_ODP
# define CO_READ_ODP(val) val
#endif /* CO_READ_ODP */

#ifndef CO_READ_OD16
# define CO_READ_OD16(val) val
#endif /* CO_READ_OD16 */

#ifndef CO_READ_OD8
# define CO_READ_OD8(val) val
#endif /* CO_READ_OD8 */

#ifndef CO_READ_OD_DESC_U8
# define CO_READ_OD_DESC_U8(pObj, idx, member) pObj->pValDesc[idx].member
#endif /* CO_READ_OD_DESC_U8 */

#ifndef CO_READ_OD_DESC_PTR
# define CO_READ_OD_DESC_PTR(pObj, idx, member) pObj->pValDesc[idx].member
#endif /* CO_READ_OD_DESC_PTR */

#ifndef CO_READ_OD_DESC_ATTR
# define CO_READ_OD_DESC_ATTR(pObj, idx) pObj->pValDesc[idx].attribute
#endif /* CO_READ_OD_DESC_ATTR */

#ifndef CO_WRITE_OD_DESC8
# define CO_WRITE_OD_DESC8(pObj, idx, member, val) \
	do {\
	pObj->pValDesc[idx].member = (val);\
	} while(0)
#endif /* CO_WRITE_OD_DESC8 */

#ifndef CO_WRITE_OD_ATTR
# define CO_WRITE_OD_ATTR(pObj, idx, val) \
	do {\
	pObj->pValDesc[idx].attribute = (val);\
	} while(0)
#endif /* CO_WRITE_OD_ATTR */

#ifndef CO_READ_OD_LIMIT_U8
# define CO_READ_OD_LIMIT_U8(pObj, idx, member)	\
	((LIMIT_U8_T *)pObj->pValDesc[idx].pLimits)->member
#endif /* CO_READ_OD_LIMIT_U8 */

#ifndef CO_READ_OD_LIMIT_U16
# define CO_READ_OD_LIMIT_U16(pObj, idx, member)	\
	((LIMIT_U16_T *)pObj->pValDesc[idx].pLimits)->member
#endif /* CO_READ_OD_LIMIT_U16 */

#ifndef CO_READ_OD_LIMIT_U32
# define CO_READ_OD_LIMIT_U32(pObj, idx, member)	\
	((LIMIT_U32_T *)pObj->pValDesc[idx].pLimits)->member
#endif /* CO_READ_OD_LIMIT_U32 */

#ifndef CO_READ_OD_LIMIT_I8
# define CO_READ_OD_LIMIT_I8(pObj, idx, member)	\
	((LIMIT_I8_T *)pObj->pValDesc[idx].pLimits)->member
#endif /* CO_READ_OD_LIMIT_I8 */

#ifndef CO_READ_OD_LIMIT_I16
# define CO_READ_OD_LIMIT_I16(pObj, idx, member)	\
	((LIMIT_I16_T *)pObj->pValDesc[idx].pLimits)->member
#endif /* CO_READ_OD_LIMIT_I16 */

#ifndef CO_READ_OD_LIMIT_I32
# define CO_READ_OD_LIMIT_I32(pObj, idx, member)	\
	((LIMIT_I32_T *)pObj->pValDesc[idx].pLimits)->member
#endif /* CO_READ_OD_LIMIT_I32 */

#ifndef CO_READ_OD_LIMIT_R32
# define CO_READ_OD_LIMIT_R32(pObj, idx, member)	\
	((LIMIT_R32_T *)pObj->pValDesc[idx].pLimits)->member
#endif /* CO_READ_OD_LIMIT_R32 */


#ifdef CONFIG_EXTENDED_DATA_TYPES
# ifndef CO_READ_OD_LIMIT_U24
#  define CO_READ_OD_LIMIT_U24(pObj, idx, member)	\
	((LIMIT_U24_T *)pObj->pValDesc[idx].pLimits)->member
# endif /* CO_READ_OD_LIMIT_U24 */

# ifndef CO_READ_OD_LIMIT_U40
#  define CO_READ_OD_LIMIT_U40(pObj, idx, member)	\
	((LIMIT_U40_T *)pObj->pValDesc[idx].pLimits)->member
# endif /* CO_READ_OD_LIMIT_U40 */

# ifndef CO_READ_OD_LIMIT_U48
#  define CO_READ_OD_LIMIT_U48(pObj, idx, member)	\
	((LIMIT_U48_T *)pObj->pValDesc[idx].pLimits)->member
# endif /* CO_READ_OD_LIMIT_U48 */

# ifndef CO_READ_OD_LIMIT_U56
#  define CO_READ_OD_LIMIT_U56(pObj, idx, member)	\
	((LIMIT_U56_T *)pObj->pValDesc[idx].pLimits)->member
# endif /* CO_READ_OD_LIMIT_U56 */

# ifndef CO_READ_OD_LIMIT_U64
#  define CO_READ_OD_LIMIT_U64(pObj, idx, member)	\
	((LIMIT_U64_T *)pObj->pValDesc[idx].pLimits)->member
# endif /* CO_READ_OD_LIMIT_U64 */

# ifndef CO_READ_OD_LIMIT_I64
#  define CO_READ_OD_LIMIT_I64(pObj, idx, member)	\
	((LIMIT_I64_T *)pObj->pValDesc[idx].pLimits)->member
# endif /* CO_READ_OD_LIMIT_U64 */
#endif /* CONFIG_EXTENDED_DATA_TYPES */

/* external variable declarations */
/* number of objects */
extern UNSIGNED16	maxObjDicElements CO_LINE_PARA_ARRAY_DEF;
extern UNSIGNED16	*co_maxObjDicElements;
extern UNSIGNED16	*pMaxObjDicElements;
extern UNSIGNED16	*co_pMaxObjDicElements;
/* object dictionary */
#ifdef CONFIG_MULT_LINES
extern OBJDIR_T *objDirMan[];	/* array of addresses of od */
extern OBJDIR_T **pObjDirMan;	/* pointer to od pointers for multi line */
extern OBJDIR_T **co_objDirMan;	/* pointer to od for multi line */
#else
extern OBJDIR_T objDir[];	/* od for single line */
extern OBJDIR_T *pObjDir;	/* pointer to od for single line */
extern OBJDIR_T *co_objDir;	/* pointer to od for single line */
#endif


/* defines max. Datasize for convert buffer */
#ifndef CO_MAX_NUMDATA_SIZE
# define CO_MAX_NUMDATA_SIZE 8
#else /* check size */
# if (CO_MAX_NUMDATA_SIZE < 5)
#  define CO_MAX_NUMDATA_SIZE 4
# endif
#endif

#endif /* PCO_ACCESS_H__ */



#ifdef CONFIG_WITHOUT_PROTOTYPES
#else /* CONFIG_WITHOUT_PROTOTYPES */

# ifndef PCO_ACCESS__PROTOTYPES_H__
#  define PCO_ACCESS__PROTOTYPES_H__

/* function prototypes */

LIST_ELEMENT_T 	*searchObj(UNSIGNED16 index CO_COMMA_LINE_PARA_DECL);
void   	        *getSubIndexAddr( CO_CONST LIST_ELEMENT_T *curObj, UNSIGNED8 subIndex);
CO_INLINE UNSIGNED32 getObjSize(CO_CONST LIST_ELEMENT_T *curObj, UNSIGNED8 subIndex);
UNSIGNED8 *getObjDefaultVal(CO_CONST LIST_ELEMENT_T *curObj, UNSIGNED8 subIndex);
CO_INLINE BASIC_DATA_T getObjBasicDataType(UNSIGNED16 index, UNSIGNED8 subIndex
	CO_COMMA_LINE_PARA_DECL);

# endif /* PCO_ACCESS__PROTOTYPES_H__ */
#endif /* PCONFIG_WITHOUT_PROTOTYPES */

/* end of source */

