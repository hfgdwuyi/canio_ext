/*
 * co_def - defines constants and enumerations
 *
 * Copyright (c) 2001-2017 port GmbH Halle/Saale
 * ---------------------------------------------------------------
 */

/**
*  \file co_def.h
*++ Defines for constants and enumerations
*-- Definitionen von Konstanten und Aufzählungen
*  \author port GmbH Halle (Saale)
*
*++ This file contains constants and enumerations for the CANopen library.
*-- Diese Datei enthält Definitionen und Aufzählungen,
*-- die von der CANopen Bibliothek benutzt werden.
*/

#ifndef __CO_DEF_H
# define __CO_DEF_H

#include <co_type.h>		/* include data type definition */


/* macros for single/multiple line substitution */
#ifndef CO_LINE_PARA_DECL

# ifdef CONFIG_NO_GLOBAL_VARS
#   define CO_LINE_PARA			coptr
#   define CO_LINE_PARA_DECL		CANOPEN_DATA_T *coptr
#   define CO_COMMA_LINE_PARA		,coptr
#   define CO_COMMA_LINE_PARA_DECL	,CANOPEN_DATA_T *coptr
#   define CO_LINE_PARA_ARRAY_DEF
#   define CO_LINE_PARA_ARRAY_INDEX

#  ifdef CONFIG_REDUNDANCY_SUPPORT
#   define CO_REDCY_PARA		canLine, CO_LINE_PARA
#   define CO_REDCY_PARA_DECL		UNSIGNED8 canLine, CO_LINE_PARA_DECL
#   define CO_COMMA_REDCY_PARA		,canLine, CO_LINE_PARA
#   define CO_COMMA_REDCY_PARA_DECL	,UNSIGNED8 canLine, CO_LINE_PARA_DECL
#   define CO_REDCY_PARA_ARRAY_INDEX	[canLine]
#   define CO_REDCY_PARA_ARRAY_DEF	[2]
#  else /* CONFIG_REDUNDANCY_SUPPORT */
#   define CO_REDCY_PARA		CO_LINE_PARA
#   define CO_REDCY_PARA_DECL		CO_LINE_PARA_DECL
#   define CO_COMMA_REDCY_PARA		CO_COMMA_LINE_PARA
#   define CO_COMMA_REDCY_PARA_DECL	CO_COMMA_LINE_PARA_DECL
#   define CO_REDCY_PARA_ARRAY_INDEX
#   define CO_REDCY_PARA_ARRAY_DEF
#  endif /* CONFIG_REDUNDANCY_SUPPORT */

#   define CO_GLOBVARS_PARA		CO_LINE_PARA
#   define CO_GLOBVARS_PARA_DECL	CO_LINE_PARA_DECL
#   define CO_COMMA_GLOBVARS_PARA	CO_COMMA_LINE_PARA
#   define CO_COMMA_GLOBVARS_PARA_DECL	CO_COMMA_LINE_PARA_DECL

# else /* CONFIG_NO_GLOBAL_VARS */

#  ifdef CONFIG_MULT_LINES
#   define CO_LINE_PARA			canLine
#   define CO_LINE_PARA_DECL		UNSIGNED8 canLine
#   define CO_LINE_PARA_ARRAY_DEF	[CO_MAX_CAN_LINES]
#   define CO_LINE_PARA_ARRAY_INDEX	[canLine]
#   define CO_REDCY_PARA_ARRAY_DEF	CO_LINE_PARA_ARRAY_DEF
#   define CO_REDCY_PARA_ARRAY_INDEX	CO_LINE_PARA_ARRAY_INDEX
#   define CO_REDCY_PARA_DECL		UNSIGNED8 canLine
#   define CO_REDCY_PARA		canLine
#   define CO_COMMA_LINE_PARA_DECL	,UNSIGNED8 canLine
#   define CO_COMMA_LINE_PARA		,canLine
#   define CO_COMMA_REDCY_PARA_DECL	,UNSIGNED8 canLine
#   define CO_COMMA_REDCY_PARA		,canLine
#   ifndef CO_ACT_CAN_LINES
#    define CO_ACT_CAN_LINES		CONFIG_MULT_LINES
#   endif
#  else /* CONFIG_MULT_LINES */
#   define CO_LINE_PARA_DECL		void
#   define CO_LINE_PARA
#   define CO_COMMA_LINE_PARA
#   define CO_COMMA_LINE_PARA_DECL
#   define CO_LINE_PARA_ARRAY_DEF
#   define CO_LINE_PARA_ARRAY_INDEX

#   ifdef CONFIG_REDUNDANCY_SUPPORT
#   define CO_REDCY_PARA_DECL		UNSIGNED8 canLine
#   define CO_REDCY_PARA		canLine
#   define CO_REDCY_PARA_ARRAY_DEF	[2]
#   define CO_REDCY_PARA_ARRAY_INDEX	[canLine]
#   define CO_COMMA_REDCY_PARA_DECL	,UNSIGNED8 canLine
#   define CO_COMMA_REDCY_PARA		,canLine
#   else /* CONFIG_REDUNDANCY_SUPPORT */
#   define CO_REDCY_PARA_DECL		void
#   define CO_REDCY_PARA
#   define CO_REDCY_PARA_ARRAY_DEF
#   define CO_REDCY_PARA_ARRAY_INDEX
#   define CO_COMMA_REDCY_PARA_DECL
#   define CO_COMMA_REDCY_PARA
#   endif /* CONFIG_REDUNDANCY_SUPPORT */

#  endif /* CONFIG_MULT_LINES */

#  define CO_GLOBVARS_PARA
#  define CO_GLOBVARS_PARA_DECL		void
#  define CO_COMMA_GLOBVARS_PARA
#  define CO_COMMA_GLOBVARS_PARA_DECL

# endif /* CONFIG_NO_GLOBAL_VARS */
#endif /* CO_LINE_PARA_DECL */


#ifndef GL_ARRAY
# ifdef CONFIG_NO_GLOBAL_VARS
#   define GL_VAR(var)	coptr->var
#   define GL_ARRAY(var)	coptr->var
#  ifdef CONFIG_DYN_MEM_ALLOC
#   define GL_PVAR(var)		coptr->(*p_##var)
#  else /* CONFIG_DYN_MEM_ALLOC */
#   define GL_PVAR(var)		coptr->var
#  endif /* CONFIG_DYN_MEM_ALLOC */
# else /* CONFIG_NO_GLOBAL_VARS */
#   define GL_VAR(var)		var
#   define GL_ARRAY(var)	var CO_LINE_PARA_ARRAY_INDEX
#  ifdef CONFIG_DYN_MEM_ALLOC
#   define GL_PVAR(var)		(*p_##var)
#  else /* CONFIG_DYN_MEM_ALLOC */
#   define GL_PVAR(var)		var
#  endif /* CONFIG_DYN_MEM_ALLOC */
# endif /* CONFIG_NO_GLOBAL_VARS */
#endif /* GL_ARRAY */


/* Alignment should be set at makefile or cal_conf.h */
# if !defined(CONFIG_ALIGNMENT)
#  error No Alignment set - Please define it at the Makefile or in cal_conf.h
# endif

/* if segmented SDO transfer is not allowed,
   no domain (program) up- and download is possible  */
# if !defined(CONFIG_SEG_SDO) && defined(CONFIG_DOMAIN_UPDNLD)
#  error impossible configuration: CONFIG_DOMAIN_UPDNLD is set and CONFIG_SEG_SDO is not set
# endif

/* Heartbeat or Nodeguarding is mandatory */
# if !(defined(CONFIG_NODE_GUARDING) || defined(CONFIG_HEARTBEAT_PRODUCER))
#  error impossible configuration: NODEGUARDING or HEARTBEAT is required
# endif

# if defined(CONFIG_REDUNDANCY_SUPPORT) && defined(CONFIG_NODE_GUARDING)
#  error impossible configuration: Redundancy and Nodeguarding
# endif /* CONFIG_REDUNDANCY_SUPPORT */

/* for usage global variable pointer init all static variables */
# ifdef CONFIG_NO_GLOBAL_VARS
#  ifdef CONFIG_CLEAR_CO_GLOBAL_VARS
#  else /* CONFIG_CLEAR_CO_GLOBAL_VARS */
#   define CONFIG_CLEAR_CO_GLOBAL_VARS
#  endif /* CONFIG_CLEAR_CO_GLOBAL_VARS */
# endif /* CONFIG_NO_GLOBAL_VARS */


/* define security mechanism, if not defined in cal_conf.h */

/* BC 4.5 32bit compiler has problem with empty parameters within macros
   at WIN32 console applications */

# if defined (__WIN32__) && defined(__BORLANDC__)
#  if __BORLANDC__ > 0x400 && __BORLANDC__ < 0x500
#    ifndef CONFIG_MULT_LINES
#	define CO_MACRO_DEF_EXCEPTION 1
#    endif
#  else
#   undef CO_MACRO_DEF_EXCEPTION
#  endif
# endif

# ifdef CO_MACRO_DEF_EXCEPTION

#  ifndef CO_COM_PART_ALLOC
#   define CO_COM_PART_ALLOC()
#  endif

#  ifndef CO_COM_PART_RELEASE
#   define CO_COM_PART_RELEASE()
#  endif

#  ifndef CO_APPL_PART_ALLOC
#   define CO_APPL_PART_ALLOC()
#  endif

#  ifndef CO_APPL_PART_RELEASE
#   define CO_APPL_PART_RELEASE()
#  endif

#  ifndef CO_NEW_RX_MSG
#   define CO_NEW_RX_MSG()
#  endif

# else /* CO_MACRO_DEF_EXCEPTION */

#  ifndef CO_COM_PART_ALLOC
#   define CO_COM_PART_ALLOC(CO_LINE_PARA)
#  endif

#  ifndef CO_COM_PART_RELEASE
#   define CO_COM_PART_RELEASE(CO_LINE_PARA)
#  endif

#  ifndef CO_APPL_PART_ALLOC
#   define CO_APPL_PART_ALLOC(CO_LINE_PARA)
#  endif

#  ifndef CO_APPL_PART_RELEASE
#   define CO_APPL_PART_RELEASE(CO_LINE_PARA)
#  endif

#  ifndef CO_NEW_RX_MSG
#   define CO_NEW_RX_MSG(CO_LINE_PARA)
#  endif

# endif /* CO_MACRO_DEF_EXCEPTION */

/* watchdog call */
# ifndef CO_WATCH_DOG
#  define CO_WATCH_DOG
# endif

/* call for resetComm start and end */
# ifndef CO_RESET_COMM_START
#  define CO_RESET_COMM_START
# endif /*CO_RESET_COMM_START */
# ifndef CO_RESET_COMM_END
#  define CO_RESET_COMM_END
# endif /*CO_RESET_COMM_END */

# ifdef CONFIG_MASTER_PLUS
/* prevent warnings */
#  undef  CONFIG_MASTER
#  undef  CONFIG_SLAVE
/* set both configurations */
#  define CONFIG_MASTER
#  define CONFIG_SLAVE
# endif

# ifdef CONFIG_SLAVE_PLUS
/* prevent warnings if already exist */
#  ifndef  CONFIG_SLAVE
   /* set slave configurations */
#   define CONFIG_SLAVE
#  endif
# endif

# ifdef CONFIG_FAST_SORT
#  define CONFIG_PDO_FAST_SORT
# endif /* CONFIG_FAST_SORT */

# define CO_RTR_REQ	0x80u	/* RTR sign at cantelegram length
				 * here use the bitdefinition from sja1000 */

# define CO_MAX_NODE	128u

/* Macro for not used variables too avoid compiler warnings */
# ifndef CO_INTERNAL_NOT_USED
#  define CO_INTERNAL_NOT_USED(vpar) ((void)(vpar))
# endif /* CO_INTERNAL_NOT_USED */

#endif    /* __CO_DEF_H */

/*______________________________________________________________________EOF_*/
