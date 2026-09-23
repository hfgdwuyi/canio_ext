/*
 * cdriver - includes for common CAN driver functions
 *
 * Copyright (c) 2000-2017 port GmbH Halle (Saale)
 *------------------------------------------------------------------
 */

/**
*  \file cdriver.h
*++ Definitions for common CAN driver functions
*-- Definitionen für den allgemeinen CAN-Treiberteil
*  \author port GmbH Halle (Saale)
*
*++ This module provides
*++ the defines and declarations for the cdriver modul.
*-- Diese Modul enthält defines und Deklarationen
*-- für das cdriver Modul.
*
*/

/**
* \def CONFIG_COLIB_FLUSHMBOX
*-- Dieses Define schaltet die allgemeingültige FlushMbox() Funktion frei.
*-- Bei Benutzung einer eigenen FlushMbox() Implementierung muss dieses
*-- Define deaktiviert werden.
*++ This define enables the FlushMbox() function.
*++ It must be disabled, if using an own FlushMBox implementation.
*/
/**
* \def CONFIG_COLIB_BUFFER
*-- Dieses Define aktiviert das Default Buffer Handling.
*-- Sollte der seltene Fall eintreten, dass ein eigenes Bufferhandling
*-- benötigt wird, müssen die Buffer Zugriffs Makros an den eigenen
*-- Bedarf angepasst werden.
*++ This define enables the default buffer handling.
*++ If another buffer handling should be used, the buffer access macros
*++ have to be adjusted.
*/
#ifdef DOXYGEN
	/* only set for generation of the documentation */
# define CONFIG_COLIB_FLUSHMBOX 1
# define CONFIG_COLIB_BUFFER 1
#endif

#ifndef CDRIVER_H
# define CDRIVER_H 1

# ifndef EXTERN
#  define EXTERN extern
# endif

/* base datatypes, like COB_T
---------------------------------------------------------------------*/
#include <co_stru.h>


/* default setting - compiler specific settings are in co_XXX.h
---------------------------------------------------------------------*/
# ifndef VOLATILE
#  define VOLATILE volatile
# endif /* VOLATILE */

# ifndef REGISTER
#  define REGISTER register
# endif

# ifndef CO_DATA
#  define CO_DATA
# endif /* CO_DATA */

# ifndef XDATA
#  define XDATA
# endif /* XDATA */

# ifndef FAR
#  define FAR
# endif /* FAR */

# ifndef NEAR
#  define NEAR
# endif /* NEAR */

# ifndef DIRECT
#  define DIRECT
# endif /* DIRECT */

/* ----- Datatypes - Memory specifier --------------------------*/

/* quick access RAM
 * you cannot access this memory with pointers
 *
 */
# ifndef CO_MEM_QUICKRAM
#  define CO_MEM_QUICKRAM
/* #  define CO_MEM_QUICKRAM CO_DATA DIRECT */
# endif

/* RAM memory specifier
 * CO_MEM_RAM UNSIGNED8 * ram_ptr;
 */
# ifndef CO_MEM_RAM
#  define CO_MEM_RAM
/* #  define CO_MEM_RAM XDATA NEAR */
# endif

/* CAN memory specifier
 * (experimental version, yet)
 * Should be __far, if the CAN is outside of the current memory model
 *
 */
# ifndef CO_MEM_CAN
#  define CO_MEM_CAN
/* #  define CO_MEM_CAN FAR */
# endif

/* ----- Datatypes - Memory specifier end -------------------------*/


# ifdef CONFIG_REDUNDANCY_SUPPORT
  /* Driver must check transmit Timeout */
#  ifndef CONFIG_CAN_TIMEOUT
#   define CONFIG_CAN_TIMEOUT 1
#  endif
# endif /* CONFIG_REDUNDANCY_SUPPORT */

/*---- includes --------------------------------------------------*/
/* are specified in hardware/driver.c, the file which includes this */

/*---- default definitions ----------------------------------------*/
# ifndef CAN_ISR_REGISTERBANK
#  define CAN_ISR_REGISTERBANK
# endif /* CAN_ISR_REGISTERBANK */

# ifndef CAN_ISRSUB_REGISTERBANK
#  define CAN_ISRSUB_REGISTERBANK
# endif /* CAN_ISRSUB_REGISTERBANK */

# ifndef CAN_ISR_NUMBER
#  define CAN_ISR_NUMBER
# endif /* CAN_ISR_NUMBER */

# ifndef CONFIG_CAN_SLOW_DOWN_IO
#  define CONFIG_CAN_SLOW_DOWN_IO
# endif


/**
* \def CONFIG_CAN_OBJECTS
*++ sum of all CAN objects for CANopen
*-- Summe aller CAN Objekte für CANopen
*/
#ifdef DOXYGEN
	/* only set for generation of the documentation */
# define CONFIG_CAN_OBJECTS 10
#endif

/**
* \def CONFIG_COB_NUMBERS
*++ sum of all CAN objects for CANopen and all additional needed
*++ CAN objects, e.g. for Debugging
*-- Summe aller CAN Objekte für CANopen und zusätzlicher CAN Objekte,
*-- z.B. für Debug Objekte
* \hideinitializer
*/

/* everytime arrays used */
# define CONFIG_COB_ARRAY	1

# ifndef CONFIG_COB_NUMBERS
#  ifdef CONFIG_COB_ADDITIONAL
#   ifdef CONFIG_MULT_LINES
#    error "CONFIG_COB_ADDITIONAL with CONFIG_MULT_LINES not supported, yet!"
#    error "Define an adapted CONFIG_COB_NUMBERS  within cal_conf.h."
#   else /* CONFIG_MULT_LINES */
#    define CONFIG_COB_NUMBERS 	(CONFIG_CAN_OBJECTS + CONFIG_COB_ADDITIONAL)
#   endif /* CONFIG_MULT_LINES */
#  else /* CONFIG_COB_ADDITIONAL */
#    define CONFIG_COB_NUMBERS 			CONFIG_CAN_OBJECTS
#    ifdef CONFIG_MULT_LINES
#      define CONFIG_COB_NUMBERS_LINECFG 	CONFIG_CAN_OBJECTS_LINECFG
#    endif
#    if defined(CONFIG_REDUNDANCY_SUPPORT) && !defined(CONFIG_NO_GLOBAL_VARS)
#      undef CONFIG_COB_NUMBERS_LINECFG
#      define CONFIG_COB_NUMBERS_LINECFG 	CONFIG_CAN_OBJECTS,CONFIG_CAN_OBJECTS
#      undef CONFIG_COB_NUMBERS
#      define CONFIG_COB_NUMBERS 		(CONFIG_CAN_OBJECTS * 2)
#    endif
#  endif /* CONFIG_COB_ADDITIONAL */
# endif /* CONFIG_COB_NUMBERS */

/*---- struct definition ---------------------------------------*/
/**
* \def CONFIG_DRIVER_FAST_SORT
*++ Use Fast Sort Algorithm for COB lists (needs more RAM).
*-- Benutzt einen schnelleren Sortieralgothmus für die COB Listen.
*-- Es wird hierfür extra RAM benötigt.
*/
# ifdef CONFIG_FAST_SORT
#  ifdef CONFIG_DRIVER_FAST_SORT
#  else
#    define CONFIG_DRIVER_FAST_SORT 1
#  endif
# endif

/**
* \def GL_DRV_VAR
*++ access to a line independend driver variable
*-- Makro für den Zugriff auf Linienunabhängige Treiber Variable
* \param var
*++ driver variable
*-- Treiber variable
*
*++ In multiline case: GL_DRV_VAR(var) -> var
*-- Mit Multiline: GL_DRV_VAR(var) -> var
*  \hideinitializer
*/
/**
* \def GL_DRV_REDCY
*++ access to a driver variable, that has a redundant line,
*++ but is line independend
*-- Makro für den Zugriff auf Treibervariable, die eine Redundante
*-- Line unterstützen, aber ansonsten Linienunabhängig sind.
*
*++ This macro is used to special handle the case Redundancy without
*++ global variables. In other cases the macro is used like ::GL_DRV_VAR.
*-- Dieses Makro behandelt den Spezialfall, dass Redundancy ohne globale
*-- Variable benutzt wird. Ansonsten verhält es sich so wie ::GL_DRV_VAR.
*
* \param var
*++ driver variable
*-- Treiber variable
*
*  \hideinitializer
*/
/**
* \def GL_DRV_ARRAY
*++ access to a line depend driver variable
*-- Makro für den Zugriff auf Linienabhängige Variable
*
* \param var
*++ driver variable
*-- Treiber variable
*
*++ In multiline case: GL_DRV_ARRAY(var) -> var[canline]
*-- Multiline: GL_DRV_ARRAY(var) -> var[canline]
*  \hideinitializer
*/
#ifdef CONFIG_NO_GLOBAL_VARS

# ifdef CONFIG_REDUNDANCY_SUPPORT
# define GL_DRV_VAR(var)	(((DRIVER_DATA_T *)(GL_VAR(canDrvPtr)))->var)
# define GL_DRV_REDCY(var)	(((DRIVER_DATA_T *)(GL_VAR(canDrvPtr[canLine])))->var)
# define GL_DRV_ARRAY(var)	(((DRIVER_DATA_T *)(GL_VAR(canDrvPtr[canLine])))->var)
# define GL_DRV_PREDCY(var)	(((DRIVER_DATA_T *)(GL_VAR(canDrvPtr[canLine])))->var)
# else /* CONFIG_REDUNDANCY_SUPPORT */
# define GL_DRV_VAR(var)	(((DRIVER_DATA_T *)(GL_VAR(canDrvPtr)))->var)
# define GL_DRV_REDCY(var)	(((DRIVER_DATA_T *)(GL_VAR(canDrvPtr)))->var)
# define GL_DRV_ARRAY(var)	(((DRIVER_DATA_T *)(GL_VAR(canDrvPtr)))->var)
# define GL_DRV_PREDCY(var)	(((DRIVER_DATA_T *)(GL_VAR(canDrvPtr)))->var)
# endif /* CONFIG_REDUNDANCY_SUPPORT */

#else /* CONFIG_NO_GLOBAL_VARS */
# ifdef CONFIG_REDUNDANCY_SUPPORT
#  define GL_DRV_VAR(var)	(GL_VAR(var))
#  define GL_DRV_REDCY(var)	(GL_VAR(var))
#  define GL_DRV_ARRAY(var)	(GL_ARRAY(var)[canLine])
#  define GL_DRV_PREDCY(var)	(GL_PVAR(var))
# else /* CONFIG_REDUNDANCY_SUPPORT */
#  define GL_DRV_VAR(var)	(GL_VAR(var))
#  define GL_DRV_ARRAY(var)	(GL_ARRAY(var))
#  define GL_DRV_REDCY(var)	(GL_VAR(var))
#  define GL_DRV_PREDCY(var)	(GL_PVAR(var))
# endif /* CONFIG_REDUNDANCY_SUPPORT */
#endif /* CONFIG_NO_GLOBAL_VARS */


# ifdef CONFIG_COLIB_BUFFER
/* common transmit and receive buffer */

/**
*++ software buffer state
*-- Software Buffer Status
*   \ingroup bufferhandling
*/
typedef enum  { EMPTY, FULL }	MEM_STAT_T;

/** software buffer type \ingroup bufferhandling */
struct BUFFER_ENTRY
{
	VOLATILE MEM_STAT_T	eStat;	/**< state */
	COB_KIND_T	eType;		/**< message type */
	COB_IDENT_T	cobId;		/**< message id */
#ifdef CONFIG_CAN_TIMEOUT
	UNSIGNED16	timeticks;	/**< timeout timer ticks */
#endif /* CONFIG_CAN_TIMEOUT */
	UNSIGNED8	bLength;	/**< message length */
	UNSIGNED8	pData[8];	/**< message data */
	UNSIGNED8	bChannel;	/* internal */
};

/**
* \brief software buffer type
* \ingroup bufferhandling
* The location can be changed or optimized by setting the macros
* ::NEAR and ::XDATA.
*/
typedef struct BUFFER_ENTRY BUFFER_ENTRY_T;

typedef BUFFER_ENTRY_T CO_MEM_RAM * BUFFER_ENTRY_PTR_T;
# endif /*CONFIG_COLIB_BUFFER*/

# if (CONFIG_COB_NUMBERS > 254) || defined(CONFIG_DYN_MEM_ALLOC)
typedef UNSIGNED16 COB_ARRAY_INDEX_T;
# else /* CONFIG_COB_NUMBERS > 254 */
    /** COB array index type */
typedef UNSIGNED8 COB_ARRAY_INDEX_T;
# endif /* CONFIG_COB_NUMBERS > 254 */

/*---- externals -------------------------------------------------*/
#ifdef CONFIG_NO_GLOBAL_VARS
#else /* CONFIG_NO_GLOBAL_VARS */

/* table for CAN addresses */
extern void CO_MEM_CAN * canAddrTab CO_REDCY_PARA_ARRAY_DEF;

extern UNSIGNED8 coCanDriverState CO_REDCY_PARA_ARRAY_DEF;

#endif /* CONFIG_NO_GLOBAL_VARS */


/*---- global variables ------------------------------------------*/
# ifdef CONFIG_COLIB_BUFFER

typedef UNSIGNED16 CO_DATA BUFFER_INDEX_T;

#ifdef CONFIG_NO_GLOBAL_VARS
#else /* CONFIG_NO_GLOBAL_VARS */
extern BUFFER_ENTRY_T CO_MEM_RAM pTX_Buffer[] CO_REDCY_PARA_ARRAY_DEF;
extern BUFFER_ENTRY_T CO_MEM_RAM pRX_Buffer[] CO_REDCY_PARA_ARRAY_DEF;

extern /*VOLATILE*/ BUFFER_INDEX_T CO_MEM_QUICKRAM
			bRX_WriteIndex CO_REDCY_PARA_ARRAY_DEF,
			bRX_ReadIndex  CO_REDCY_PARA_ARRAY_DEF,
		        bTX_WriteIndex CO_REDCY_PARA_ARRAY_DEF,
		        bTX_ReadIndex  CO_REDCY_PARA_ARRAY_DEF;
#endif /* CONFIG_NO_GLOBAL_VARS */

# ifndef CO_MEMCPY
	/* memcpy RAM <-> RAM */
#  define CO_MEMCPY(dest,src,size) memcpy((dest),(src),(size))
# endif

# ifndef CO_CAN_MEMCPY
	/* memcpy RAM <-> CAN Controller */
#  define CO_CAN_MEMCPY(dest,src,size) CO_MEMCPY((dest),(src),(size))
# endif


/*  Buffer access
-------------------------------------------------------------*/
/**
*++ software buffer address
*-- Software Buffer Adresse
*
*  \code
*  BUFFER_ENTRY_T * BUFFER_ADDR(direction, action)
*  \endcode
*
*  \param       direction 	= TX | RX
*  \param 		action		= Write | Read
*  \returns 	pointer to ::BUFFER_ENTRY_T
*
*  \hideinitializer
*  \ingroup bufferhandling
*/
/*-----------------------------------------------------------------------*/
# define BUFFER_ADDR(direction,action) \
		(&(GL_DRV_ARRAY( p##direction##_Buffer[ \
                  GL_DRV_ARRAY(b##direction##_##action##Index) ]) \
		   ))


/**
*++ initialize buffer access
*-- Initialisierungen für den nächsten Buffer Zugriff
*
*  \code
*  BUFFER_INIT_PTR(direction, action)
*  \endcode
*
*  \param       direction 	= TX | RX
*  \param 	 action		= Write | Read
*
*++ Initialize Buffer access for BUFFER_READ() and BUFFER_WRITE().
*++ In the current implementation the function that use this macro
*++ has to declare a local pointer:
*-- Dieses Makro führt Initialisierungen durch, welche für die folgenden
*-- Buffer Zugriffe per BUFFER_READ() und BUFFER_WRITE() notwendig sind.
*-- In der aktuellen Implementierung wird ein lokaler Pointer benötigt:
*
*  \code
*  BUFFER_ENTRY_T * pBuffer;
*  \endcode
*
*  \hideinitializer
*  \ingroup bufferhandling
*/
/*-----------------------------------------------------------------------*/
# define BUFFER_INIT_PTR(direction, action) 		\
		pBuffer = BUFFER_ADDR(direction,action)


# define BUFFER_MEMBER_ADDR(direction, action, member)	\
		&(pBuffer->member)

/**
*++ buffer write access check
*-- Prüfung, ob in den Softwarebuffer geschrieben werden darf
*
*  \code
*  CHECK_BUFFER_WRITE(direction, error)
*  \endcode
*
*   \param     	direction	= RX | TX
*++ \param	error		= for example ::CANFLAG_TXBUFFER_OVERFLOW
*++				  if no write permit
*-- \param	error		= zu erzeugender Fehler, wenn kein
*--				  Schreibzugriff möglich ist,
*--				  z.B. ::CANFLAG_TXBUFFER_OVERFLOW
*
*++Usage:
*--Benutzung:
*
* \code
* BUFFER_ENTRY_T * pBuffer;
* BUFFER_INIT_PTR(TX, Write);
* CHECK_BUFFER_WRITE(TX, CANFLAG_TXBUFFER_OVERFLOW)
* {
*      // Transmit buffer is free and can be filled
* }
* \endcode
*
*++ Attention: BUFFER_INIT_PTR() must called before use this macro!
*-- Achtung: Vor Benutzung dieses Makros muss BUFFER_INIT_PTR() aufgerufen
*-- werden!
*
*  \hideinitializer
*  \ingroup bufferhandling
*/
/*------------------------------------------------------------------*/
# ifdef CONFIG_CAN_ERROR_HANDLING
#  define CHECK_BUFFER_WRITE(direction,error) \
		  if (pBuffer->eStat == FULL ) { \
		      GL_DRV_ARRAY(coCanDriverState) |= (error);\
                      SET_CAN_FLAG(error); \
                      SET_COLIB_FLAG(COFLAG_CAN_EVENT); \
                  } else
# else /* CONFIG_CAN_ERROR_HANDLING */
#  define CHECK_BUFFER_WRITE(direction,error) if (pBuffer->eStat == EMPTY )
# endif /* CONFIG_CAN_ERROR_HANDLING */

/**
*++ buffer read access check
*-- Prüfung, ob vom Softwarebuffer gelesen werden kann
*
*  \code
*  CHECK_BUFFER_READ(direction)
*  \endcode
*
*  \param       	direction = RX | TX
*
*++Usage:
*--Benutzung:
*
* \code
* BUFFER_ENTRY_T * pBuffer;
*
* BUFFER_INIT_PTR(TX, Read);
*
* CHECK_BUFFER_Read(TX)
* {
*      // full receive buffer can read
* }
* \endcode
*
*++ Attention: BUFFER_INIT_PTR() must called before use this macro!
*-- Achtung: Vor Benutzung dieses Makros muss BUFFER_INIT_PTR() aufgerufen
*-- werden!
*
*  \hideinitializer
*  \ingroup bufferhandling
*/
/*-----------------------------------------------------------------------*/
# define CHECK_BUFFER_READ(direction) \
		if (pBuffer->eStat == FULL )


/*  base buffer access
-----------------------------------------------------------------------*/

/**
*++ write to current buffer
*-- Schreibzugriff auf den aktuellen Softwarebuffer
*
* \code
* BUFFER_WRITE(direction, destination, value)
* \endcode
*
* \param              direction 	= TX | RX
* \param              destination 	= item from p??_Buffer[index]
* \param              value 	        = data to write
*
*++ Attention: BUFFER_INIT_PTR() must called before use this macro!
*-- Achtung: Vor Benutzung dieses Makros muss BUFFER_INIT_PTR() aufgerufen
*-- werden!
*
*  \hideinitializer
*  \ingroup bufferhandling
*/
/*-----------------------------------------------------------------------*/
# define BUFFER_WRITE(direction,destination,value) \
			    pBuffer->destination = (value)

/**
*++ read from current buffer
*-- Lesezugriff auf den aktuellen Softwarebuffer
*
* \code
* ret BUFFER_READ(direction, source)
* \endcode
*
* \retval             ret		= read data
* \param              direction 	= TX | RX
* \param              source 	= item from p??_Buffer[index]
*
*++ Attention: BUFFER_INIT_PTR() must called before use this macro!
*-- Achtung: Vor Benutzung dieses Makros muss BUFFER_INIT_PTR() aufgerufen
*-- werden!
*
*  \hideinitializer
*  \ingroup bufferhandling
*/
/*-----------------------------------------------------------------------*/
# define BUFFER_READ(direction,source) \
			(pBuffer->source)


/**
*++ release buffer, after buffer read/write the index must increment
*-- Buffer Zugriffe beendet, der Index wird auf den nächsten Buffer gesetzt
*
*  \code
*    BUFFER_ENTRY_INCR(direction, action, status)
*  \endcode
*
* \param          	direction	= TX | RX
* \param		action		= Write | Read
* \param		status		= new FULL | EMPTY
*
*++ Attention: BUFFER_INIT_PTR() must called before use this macro!
*-- Achtung: Vor Benutzung dieses Makros muss BUFFER_INIT_PTR() aufgerufen
*-- werden!
*
*  \hideinitializer
*  \ingroup bufferhandling
*/
/*----------------------------------------------------------------------*/
#   define BUFFER_ENTRY_INCR(direction,action,status) \
	do{		\
	REGISTER BUFFER_INDEX_T tmpIndex;			\
	    pBuffer->eStat = status;				\
	    tmpIndex = 1 +					\
	    	GL_DRV_ARRAY(b##direction##_##action##Index);\
	    if(tmpIndex >= (BUFFER_INDEX_T)(CONFIG_##direction##_BUFFER_SIZE))\
	    {  								\
	        tmpIndex = (BUFFER_INDEX_T)0;				\
	    }	 							\
	    GL_DRV_ARRAY(b##direction##_##action##Index) = 	\
							     tmpIndex; \
	}while(0)


//# ifdef CONFIG_COLIB_FLUSHMBOX
/* Macro's for buffer access with direct access to CAN controller */

/**
*++ Msg-Data copy from controller to buffer
*-- Msg-Data copy vom CAN Kontroller in den Softwarebuffer
*
* \param	length	= 0..8 Byte
*
* \hideinitializer
*/
/*-----------------------------------------------------------------------*/
#  ifndef BUFFER_WRITE_CPY
#  if defined(CONFIG_CAN_USE_MEMCPY)
#   define BUFFER_WRITE_CPY(length) CO_CAN_MEMCPY(              \
          (void *)(&(BUFFER_ADDR(RX,Write)->pData[0])),	\
		(void CO_MEM_CAN *)CAN_ADDR_OBJ(bChannel,CAN_OBJ_MSG(0)),\
		(length))
#  elif defined(CONFIG_CAN_USE_FORLOOP)
#   define BUFFER_WRITE_CPY(length) do{ 		\
              UNSIGNED8 CO_MEM_RAM *pTmpd;               	\
              UNSIGNED8 CO_MEM_QUICKRAM i;            	\
              pTmpd = (UNSIGNED8 CO_MEM_RAM *)&(BUFFER_ADDR(RX,Write)->pData[0]);	 \
	      for(i = 0; i < (length); i++) {                                \
	      /* may be defined as 'SLOW_DOWN' or not */                     \
	          CONFIG_CAN_SLOW_DOWN_IO;                                   \
	          *pTmpd++ = CAN_READ_OBJ(bChannel, CAN_OBJ_MSG(i) );        \
              }  \
          }while(0)
#  elif defined (CONFIG_CAN_USE_DIRECTTRANSFER )
#   define BUFFER_WRITE_CPY(length)  do{  \
              UNSIGNED8 CO_MEM_RAM *pTmpd;		\
              UNSIGNED8 CO_MEM_CAN *pTmps;      \
              pTmpd = &(BUFFER_ADDR(RX,Write)->pData[0]);		\
	      pTmps = (UNSIGNED8 CO_MEM_CAN *)CAN_ADDR_OBJ(bChannel,CAN_OBJ_MSG(0));\
	      								\
	      *pTmpd++ = *pTmps; pTmps+= CONFIG_CAN_REGISTER_OFFSET;    \
	      *pTmpd++ = *pTmps; pTmps+= CONFIG_CAN_REGISTER_OFFSET;    \
	      *pTmpd++ = *pTmps; pTmps+= CONFIG_CAN_REGISTER_OFFSET;    \
	      *pTmpd++ = *pTmps; pTmps+= CONFIG_CAN_REGISTER_OFFSET;    \
	      if(length > 4) {          \
	          *pTmpd++ = *pTmps; pTmps+= CONFIG_CAN_REGISTER_OFFSET; \
		  *pTmpd++ = *pTmps; pTmps+= CONFIG_CAN_REGISTER_OFFSET; \
		  *pTmpd++ = *pTmps; pTmps+= CONFIG_CAN_REGISTER_OFFSET; \
		  *pTmpd   = *pTmps;    \
	      } \
	  }while(0)
#  else
#   error "No Message data transfer mode specified!"
#  endif
#  endif /* BUFFER_WRITE_CPY */

/**
*++ copy message data from buffer to controller
*-- Msg-data copy vom Softwarebuffer in den CAN Kontroller
*
* \param         length = count of bytes
*
* \hideinitializer
*/
/*------------------------------------------------------------------------*/
#  ifndef BUFFER_READ_CPY
#  if defined(CONFIG_CAN_USE_MEMCPY)
#   define BUFFER_READ_CPY(length) \
	do{ \
	    CO_CAN_MEMCPY( 						\
		(void CO_MEM_CAN *)CAN_ADDR_OBJ(bChannel,CAN_OBJ_MSG(0)),	\
          	(const void CO_MEM_RAM *)&(BUFFER_ADDR(TX,Read)->pData[0]),	\
		(size_t)(length) ); \
	}while(0)
#  elif defined(CONFIG_CAN_USE_FORLOOP)
#   define BUFFER_READ_CPY(length) \
	do{ 		\
            UNSIGNED8 *pTmp;               	\
            UNSIGNED8 CO_MEM_QUICKRAM bN;            	\
	    pTmp = (UNSIGNED8 CO_MEM_RAM *)&(BUFFER_ADDR(TX,Read)->pData[0]);	\
	    for(bN = 0; bN < (length); bN++) { 	\
		CAN_WRITE_OBJ( bChannel, CAN_OBJ_MSG(bN), *pTmp++ ); 	\
	    } 					\
	}while(0)
#  elif defined(CONFIG_CAN_USE_DIRECTTRANSFER)
#   define BUFFER_READ_CPY(length) \
	do{ \
            UNSIGNED8 CO_MEM_RAM *pTmp;      \
	    pTmp = (UNSIGNED8 CO_MEM_RAM *)&(BUFFER_ADDR(TX,Read)->pData[0]); \
	    CAN_WRITE_OBJ( bChannel, CAN_OBJ_MSG(0), *pTmp++ ); \
	    CAN_WRITE_OBJ( bChannel, CAN_OBJ_MSG(1), *pTmp++ ); \
	    CAN_WRITE_OBJ( bChannel, CAN_OBJ_MSG(2), *pTmp++ ); \
	    CAN_WRITE_OBJ( bChannel, CAN_OBJ_MSG(3), *pTmp++ ); \
	    if( length > 4) { \
	        CAN_WRITE_OBJ( bChannel, CAN_OBJ_MSG(4), *pTmp++ ); \
	        CAN_WRITE_OBJ( bChannel, CAN_OBJ_MSG(5), *pTmp++ ); \
	        CAN_WRITE_OBJ( bChannel, CAN_OBJ_MSG(6), *pTmp++ ); \
	        CAN_WRITE_OBJ( bChannel, CAN_OBJ_MSG(7), *pTmp++ ); \
	    } \
	}while(0)
#  else
#   error "No Message data transfer mode specified!"
#  endif
#  endif /* BUFFER_READ_CPY */

//# endif /* CONFIG_COLIB_FLUSHMBOX */

# endif /*CONFIG_COLIB_BUFFER*/

#ifdef CONFIG_NO_GLOBAL_VARS
#else /* CONFIG_NO_GLOBAL_VARS */
# ifdef  CONFIG_CAN_DEBUG_VARS
extern VOLATILE UNSIGNED32 cal_rx_ints CO_LINE_PARA_ARRAY_DEF;
extern VOLATILE UNSIGNED32 cal_tx_ints CO_LINE_PARA_ARRAY_DEF;
extern VOLATILE UNSIGNED32 cal_ch_ints CO_LINE_PARA_ARRAY_DEF;
# endif /* CONFIG_CAN_DEBUG_VARS */
#endif /* CONFIG_NO_GLOBAL_VARS */

# ifdef CONFIG_COB_ARRAY
#  ifdef CONFIG_COB_NUMBERS
#  else /* CONFIG_COB_NUMBERS */
#    error "Number of CAN Objects is absent (CONFIG_COB_NUMBERS)!"
#  endif /* CONFIG_COB_NUMBERS */

typedef COB_T CO_MEM_RAM * COB_PTR_T;
typedef COB_ARRAY_INDEX_T CO_MEM_RAM * COB_ARRAY_INDEX_PTR_T;

#ifdef CONFIG_NO_GLOBAL_VARS
#else
extern COB_T CO_MEM_RAM cobList[CONFIG_COB_NUMBERS];
extern COB_ARRAY_INDEX_T CO_MEM_RAM cobListNextEntry CO_REDCY_PARA_ARRAY_DEF;
#endif /* CONFIG_NO_GLOBAL_VARS */

#  ifdef CONFIG_CAN_TEST_VALID_COB
/* static in cdriver.c */
/* extern COB_ARRAY_INDEX_T cob_index_list[CONFIG_COB_NUMBERS]; */
#  endif /* CONFIG_CAN_TEST_VALID_COB */
# endif /* CONFIG_COB_ARRAY */

#ifdef CONFIG_NO_GLOBAL_VARS
typedef CAN_MSG_T * CAN_MSG_PTR_T;
#else /* CONFIG_NO_GLOBAL_VARS */
typedef CAN_MSG_T CO_MEM_RAM * CAN_MSG_PTR_T;
#endif /* CONFIG_NO_GLOBAL_VARS */

/*---- function prototypes ---------------------------------------*/
#ifdef CONFIG_WITHOUT_PROTOTYPES

#else /* CONFIG_WITHOUT_PROTOTYPES */

# ifdef CONFIG_NO_GLOBAL_VARS
void	init_canDriverPtr(UNSIGNED8 *ptr, CO_REDCY_PARA_DECL);
void	init_cpuDriverPtr(UNSIGNED8 *ptr, CO_REDCY_PARA_DECL);
# endif /* CONFIG_NO_GLOBAL_VARS */


# ifdef CONFIG_COLIB_BUFFER
RET_T Insert_TX_Request( COB_T * , UNSIGNED8 * CO_COMMA_GLOBVARS_PARA_DECL );

BUFFER_INDEX_T getNumberOfTxMessages( CO_REDCY_PARA_DECL );
BUFFER_INDEX_T getNumberOfRxMessages( CO_REDCY_PARA_DECL );

#  ifdef CONFIG_CAN_TIMEOUT
BUFFER_INDEX_T checkBufferTimeout(CO_REDCY_PARA_DECL);
#  endif /* CONFIG_CAN_TIMEOUT */

# endif /*CONFIG_COLIB_BUFFER*/

# ifdef CONFIG_REDUNDANCY_SUPPORT
void move_TxBuffer( UNSIGNED8 , UNSIGNED8 );
# endif

#ifdef CONFIG_COLIB_J1939
# include "kvcanhw.h"
void co_getNextRxMessage(kvRawCanMsg* CO_COMMA_LINE_PARA_DECL);
#endif /* CONFIG_COLIB_J1939 */

# ifdef CONFIG_CAN_NO_TEST_VALID_COB
# else /* CONFIG_CAN_NO_TEST_VALID_COB */
void createCobIdIndex( CO_REDCY_PARA_DECL );
COB_KIND_T validCobId( COB_IDENT_T, UNSIGNED8 CO_COMMA_REDCY_PARA_DECL);
# endif /* CONFIG_CAN_TEST_VALID_COB */

void initCobList( CO_REDCY_PARA_DECL );
COB_T * initCobEntry( CO_REDCY_PARA_DECL );

COB_T * getSortRxCOB( COB_ARRAY_INDEX_T cnt CO_COMMA_REDCY_PARA_DECL);
#endif /* CONFIG_WITHOUT_PROTOTYPES */


#endif /* __CDRIVER_H */

/*______________________________________________________________________EOF_*/
