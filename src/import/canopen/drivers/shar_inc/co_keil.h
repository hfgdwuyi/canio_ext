/*
 * co_keil.h - special defines for the Keil Compiler
 *
 * Copyright (c) 2002-2017 port GmbH Halle/Saale
 *------------------------------------------------------------------
 *
 * $Header: /z2/cvsroot/common_include/co_keil.h,v 1.36 2015/05/22 13:22:36 jsc Exp $
 *
 *------------------------------------------------------------------
 *
 */

/**
* \file co_keil.h
*++ Definitions for the Keil C Compiler
*-- Definitionen für den Keil C Compiler
* \author port GmbH
* $Revision: 1.36 $
* $Date: 2015/05/22 13:22:36 $
*
*
*++ This header file supports the Keil Compiler.
*-- Diese Header-Datei enthält Defines zur Verwendung
*-- mit dem Keil-Compiler.
*
*/
#  ifdef DOXYGEN
	/* this part is only used for the generation of the documentation */
#   define XDATA
#   undef XDATA
#   define IDATA
#   undef IDATA
#   define CO_CODE
#   undef CO_CODE
#   define CO_DATA
#   undef CO_DATA
#   define FAR
#   undef FAR
#   define NEAR
#   undef NEAR
#   define DIRECT
#   undef DIRECT
#   define RESTRICT 
#   undef RESTRICT 
#   define INTERRUPT
#   undef INTERRUPT
#   define CONFIG_ISR_PRESTRING
#   undef CONFIG_ISR_PRESTRING
#   define CONFIG_ISR_POSTSTRING
#   undef CONFIG_ISR_POSTSTRING
#  endif


#ifndef CO_KEIL_H
#define CO_KEIL_H 1

/* datatype size_t */
# include <stddef.h>

/* Keil is a C90 Compiler with support a handful of C99 
 * options. BDEBUG(...) in the file co_debug.h is not supported. */
#define BDEBUG	//

/* --------------------------------------------------------------- */
# ifdef CONFIG_CPU_FAMILY_XC166
/* --------------------------------------------------------------- */
#  ifndef CO_MEM_CAN
#    define CO_MEM_CAN  FAR
#  endif
#  ifndef CO_CONST
#    define CO_CONST	const
#  endif
#  ifndef CO_DATA
	/* only useable for 8051 */
#    define CO_DATA
#  endif
#  ifndef CO_CODE
#    define CO_CODE 	
#  endif
#  ifndef FAR
	/* FAR ist depend on the Memory Modell - LARGE/far XLARGE/huge */
#    define FAR huge	
/* #    define FAR  */
#  endif

#  ifndef NEAR
#    define NEAR
	/* near only possible, if Stack and Ram in the same 16K segment */
/* #    define NEAR near */
#  endif

#  ifndef XDATA
#    define XDATA
#  endif

/*
* type 'long long int' not supported
* UNSIGNED64 must be simulate
*/
#  ifdef  CONFIG_EXTENDED_DATA_TYPES
#   define CONFIG_EMULATE_U64
#  endif

/* interupt settings for Keil XC166 */
#  ifndef CONFIG_TIMER_ISR_PRESTRING 
#   define CONFIG_TIMER_ISR_PRESTRING void
#  endif /* CONFIG_TIMER_ISR_PRESTRING  */
#  ifndef CONFIG_TIMER_ISR_POSTSTRING 
#   define CONFIG_TIMER_ISR_POSTSTRING \
		CONFIG_TIMER_ISR_NUMBER CONFIG_TIMER_ISR_REGISTERBANK
#  endif /* CONFIG_TIMER_ISR_POSTSTRING  */

#  ifndef CONFIG_CAN_ISR_PRESTRING 
#   define CONFIG_CAN_ISR_PRESTRING void
#  endif /* CONFIG_CAN_ISR_PRESTRING  */
#  ifndef CONFIG_CAN_ISR_POSTSTRING 
#   define CONFIG_CAN_ISR_POSTSTRING \
		CONFIG_CAN_ISR_NUMBER CONFIG_CAN_ISR_REGISTERBANK
#  endif /* CONFIG_CAN_ISR_POSTSTRING  */

/* --------------------------------------------------------------- */
# endif /* CONFIG_CPU_FAMILY_XC166 */
/* --------------------------------------------------------------- */

/* --------------------------------------------------------------- */
# ifdef CONFIG_CPU_FAMILY_XE166
/* --------------------------------------------------------------- */
#  ifndef CO_CONST
#    define CO_CONST	const
#  endif
#  ifndef CO_DATA
	/* only useable for 8051 */
#    define CO_DATA
#  endif
#  ifndef CO_CODE
#    define CO_CODE 	
#  endif
#  ifndef FAR
	/* FAR ist depend on the Memory Modell - LARGE/far XLARGE/huge */
#    define FAR huge	
/* #    define FAR  */
#  endif

#  ifndef NEAR
#    define NEAR
	/* near only possible, if Stack and Ram in the same 16K segment */
/* #    define NEAR near */
#  endif

#  ifndef XDATA
#    define XDATA
#  endif

/*
* type 'long long int' not supported
* UNSIGNED64 must be simulate
*/
#  ifdef  CONFIG_EXTENDED_DATA_TYPES
#   define CONFIG_EMULATE_U64
#  endif

/* interupt settings for Keil XC166 */
#  ifndef CONFIG_TIMER_ISR_PRESTRING 
#   define CONFIG_TIMER_ISR_PRESTRING void
#  endif /* CONFIG_TIMER_ISR_PRESTRING  */
#  ifndef CONFIG_TIMER_ISR_POSTSTRING 
#   define CONFIG_TIMER_ISR_POSTSTRING \
		CONFIG_TIMER_ISR_NUMBER CONFIG_TIMER_ISR_REGISTERBANK
#  endif /* CONFIG_TIMER_ISR_POSTSTRING  */

#  ifndef CONFIG_CAN_ISR_PRESTRING 
#   define CONFIG_CAN_ISR_PRESTRING void
#  endif /* CONFIG_CAN_ISR_PRESTRING  */
#  ifndef CONFIG_CAN_ISR_POSTSTRING 
#   define CONFIG_CAN_ISR_POSTSTRING \
		CONFIG_CAN_ISR_NUMBER CONFIG_CAN_ISR_REGISTERBANK
#  endif /* CONFIG_CAN_ISR_POSTSTRING  */

/* --------------------------------------------------------------- */
# endif /* CONFIG_CPU_FAMILY_XE166 */
/* --------------------------------------------------------------- */

/* --------------------------------------------------------------- */
# ifdef CONFIG_CPU_FAMILY_C166
/* --------------------------------------------------------------- */
#  ifndef CO_CONST
#    define CO_CONST	const
#  endif
#  ifndef CO_DATA
	/* only useable for 8051 */
#    define CO_DATA
#  endif
#  ifndef CO_CODE
#    define CO_CODE 	
#  endif
#  ifndef FAR
	/* FAR ist depend on the Memory Modell - LARGE/far XLARGE/huge */
#   if __MODEL__ < 4	
#    define FAR far
#   else
/* #    define FAR huge */
	/* default Pointers are far or huge pointer without extention */
#   define FAR
#   endif
#  endif
#  ifndef NEAR
#    define NEAR
	/* near only possible, if Stack and Ram in the same 16K segment */
/* #    define NEAR near */
#  endif
#  ifndef XDATA
#    define XDATA
#  endif

# ifndef XHUGE
#  define XHUGE xhuge
# endif

# ifndef HUGE
#  define HUGE huge
# endif

/*
* type 'long long int' not supported
* UNSIGNED64 must be simulate
*/
#  ifdef  CONFIG_EXTENDED_DATA_TYPES
#   define CONFIG_EMULATE_U64
#  endif

/* interupt settings for Keil C166 */
#  ifndef CONFIG_TIMER_ISR_PRESTRING 
#   define CONFIG_TIMER_ISR_PRESTRING void
#  endif /* CONFIG_TIMER_ISR_PRESTRING  */
#  ifndef CONFIG_TIMER_ISR_POSTSTRING 
#   define CONFIG_TIMER_ISR_POSTSTRING \
		CONFIG_TIMER_ISR_NUMBER CONFIG_TIMER_ISR_REGISTERBANK
#  endif /* CONFIG_TIMER_ISR_POSTSTRING  */

#  ifndef CONFIG_CAN_ISR_PRESTRING 
#   define CONFIG_CAN_ISR_PRESTRING void
#  endif /* CONFIG_CAN_ISR_PRESTRING  */
#  ifndef CONFIG_CAN_ISR_POSTSTRING 
#   define CONFIG_CAN_ISR_POSTSTRING \
		CONFIG_CAN_ISR_NUMBER CONFIG_CAN_ISR_REGISTERBANK
#  endif /* CONFIG_CAN_ISR_POSTSTRING  */

/* --------------------------------------------------------------- */
# endif /* CONFIG_CPU_FAMILY_C166 */
/* --------------------------------------------------------------- */

/* --------------------------------------------------------------- */
/* # if defined(CONFIG_CPU_FAMILY_8051) || defined(CONFIG_CPU_FAMILY_XC800) */
# ifdef CONFIG_COMPILER_KEIL_C51
/* --------------------------------------------------------------- */
#  ifndef CO_CONST
#    define CO_CONST	code
#  endif
#  ifndef CO_DATA
#    define CO_DATA 	data
#  endif
#  ifndef IDATA
#    define IDATA 	idata
#  endif
#  ifndef PDATA
#    define PDATA 	pdata
#  endif
#  ifndef CO_CODE
#    define CO_CODE 	code
#  endif
#  ifndef FAR
#    define FAR 
#  endif
#  ifndef NEAR
#    define NEAR
#  endif
#  ifndef XDATA
#    define XDATA 	xdata
#  endif
#  ifndef CO_BIT
#    define CO_BIT bit
#  endif
#  ifndef RTX51_MODIFIER 
#   define RTX51_MODIFIER
#  endif

/*
* type 'long long int' not supported
* UNSIGNED64 must be simulate
*/
#  ifdef  CONFIG_EXTENDED_DATA_TYPES
#   define CONFIG_EMULATE_U64
#  endif

#endif

# if defined(CONFIG_CPU_FAMILY_8051)
/* interupt settings for 8051 + Keil C51 */
#  ifndef CONFIG_TIMER_ISR_PRESTRING 
#   define CONFIG_TIMER_ISR_PRESTRING void
#  endif /* CONFIG_TIMER_ISR_PRESTRING  */
#  ifndef CONFIG_TIMER_ISR_POSTSTRING 
#   define CONFIG_TIMER_ISR_POSTSTRING \
		CONFIG_TIMER_ISR_NUMBER CONFIG_TIMER_ISR_REGISTERBANK
#  endif /* CONFIG_TIMER_ISR_POSTSTRING  */

#  ifndef CONFIG_CAN_ISR_PRESTRING 
#   define CONFIG_CAN_ISR_PRESTRING void
#  endif /* CONFIG_CAN_ISR_PRESTRING  */
#  ifndef CONFIG_CAN_ISR_POSTSTRING 
#   define CONFIG_CAN_ISR_POSTSTRING \
		CONFIG_CAN_ISR_NUMBER CONFIG_CAN_ISR_REGISTERBANK
#  endif /* CONFIG_CAN_ISR_POSTSTRING  */



/* --------------------------------------------------------------- */
# endif /* CONFIG_CPU_FAMILY_8051 */
/* --------------------------------------------------------------- */

# ifdef CONFIG_COMPILER_KEIL_ARM9
#  define CONFIG_COMPILER_KEIL_ARM7
# endif

/* --------------------------------------------------------------- */
# if defined(CONFIG_COMPILER_KEIL_ARM7) || \
	defined(CONFIG_COMPILER_KEIL_CORTEXM3) || \
	defined(CONFIG_COMPILER_KEIL_ARMCC)
/* --------------------------------------------------------------- */
#  ifndef DI_FLAG
/*extern void Disable_CAN_Interrupts(void);*/
extern void Disable_CPU_Interrupts(void);
/*extern void Restore_CAN_Interrupts(void);*/
extern void Restore_CPU_Interrupts(void);

#   define DI_FLAG(flagfunction)	do{\
		Disable_CPU_Interrupts();	\
		(flagfunction);	\
		Restore_CPU_Interrupts();	\
	}while(0)
#  endif /* DI_FLAG */

#if defined (CONFIG_MULT_LINES) || defined(CONFIG_REDUNDANCY_SUPPORT)
#   ifdef CONFIG_MULT_LINES
extern unsigned char coLibFlags [CO_MAX_CAN_LINES];/* CONFIG_MULT_LINES */
    #else
extern unsigned char coLibFlags [2]; /* CONFIG_REDUNDANCY_SUPPORT */
    #endif
#   define SET_COLIB_FLAG(FLAG)   DI_FLAG(coLibFlags[canLine] |= (FLAG))
#   define RESET_COLIB_FLAG(FLAG) DI_FLAG(coLibFlags[canLine] &= ~(FLAG))
#   define TEST_COLIB_FLAG(FLAG)  (coLibFlags[canLine] & (FLAG))
#   define SET_COLIB_FLAG_ISR(FLAG)   DI_FLAG(coLibFlags[canLine] |= (FLAG))
#   define RESET_COLIB_FLAG_ISR(FLAG) DI_FLAG(coLibFlags[canLine] &= ~(FLAG))
#  else /* (CONFIG_MULT_LINES) || defined(CONFIG_REDUNDANCY_SUPPORT) */
    
extern unsigned char coLibFlags;
#   define SET_COLIB_FLAG(FLAG)   DI_FLAG(GL_ARRAY(coLibFlags) |= (FLAG))
#   define RESET_COLIB_FLAG(FLAG) DI_FLAG(GL_ARRAY(coLibFlags) &= ~(FLAG))
#   define TEST_COLIB_FLAG(FLAG)  (GL_ARRAY(coLibFlags) & (FLAG))
#   define SET_COLIB_FLAG_ISR(FLAG)   DI_FLAG(GL_ARRAY(coLibFlags) |= (FLAG))
#   define RESET_COLIB_FLAG_ISR(FLAG) DI_FLAG(GL_ARRAY(coLibFlags) &= ~(FLAG))
#  endif /* (CONFIG_MULT_LINES) || defined(CONFIG_REDUNDANCY_SUPPORT) */


#if defined (CONFIG_MULT_LINES) || defined(CONFIG_REDUNDANCY_SUPPORT)
#  ifdef CONFIG_MULT_LINES
extern unsigned char coCanFlags [CO_MAX_CAN_LINES]; /* CONFIG_MULT_LINES */
#  else /* CONFIG_MULT_LINES */
extern unsigned char coCanFlags[2]; /* CONFIG_REDUNDANCY_SUPPORT */
#  endif
#   define SET_CAN_FLAG(FLAG)       DI_FLAG(coCanFlags[canLine] |= (FLAG))
#   define RESET_CAN_FLAG(FLAG)     DI_FLAG(coCanFlags[canLine] &= ~(FLAG))
#   define TEST_CAN_FLAG(FLAG)      (coCanFlags [canLine] & (FLAG))
#   define SET_CAN_FLAG_ISR(FLAG)    DI_FLAG(coCanFlags[canLine] |= (FLAG))
#   define RESET_CAN_FLAG_ISR(FLAG)  DI_FLAG(coCanFlags[canLine] &= ~(FLAG))

#  else /* (CONFIG_MULT_LINES) || defined(CONFIG_REDUNDANCY_SUPPORT) */

extern unsigned char coCanFlags;

#   define SET_CAN_FLAG(FLAG)       DI_FLAG(GL_ARRAY(coCanFlags) |= (FLAG))
#   define RESET_CAN_FLAG(FLAG)     DI_FLAG(GL_ARRAY(coCanFlags) &= ~(FLAG))
#   define TEST_CAN_FLAG(FLAG)      (GL_ARRAY(coCanFlags) & (FLAG))
#   define SET_CAN_FLAG_ISR(FLAG)    DI_FLAG(GL_ARRAY(coCanFlags) |= (FLAG))
#   define RESET_CAN_FLAG_ISR(FLAG)  DI_FLAG(GL_ARRAY(coCanFlags) &= ~(FLAG))
#  endif /* (CONFIG_MULT_LINES) || defined(CONFIG_REDUNDANCY_SUPPORT)*/

#  ifndef CO_CONST
#    define CO_CONST	const
#  endif
#  ifndef FAR
#    define FAR 
#  endif
#  ifndef VOLATILE
#    define VOLATILE 	volatile
#  endif

#  ifndef CONFIG_ISR_PRESTRING
#   define CONFIG_ISR_PRESTRING void
#  endif
#  ifndef CONFIG_ISR_POSTSTRING
#   define CONFIG_ISR_POSTSTRING __irq
#  endif

#  ifndef CONFIG_CAN_ISR_PRESTRING 
#   define CONFIG_CAN_ISR_PRESTRING  CONFIG_ISR_PRESTRING
#  endif /* CONFIG_CAN_ISR_PRESTRING  */

#  ifndef CONFIG_CAN_ISR_POSTSTRING 
#   define CONFIG_CAN_ISR_POSTSTRING CONFIG_ISR_POSTSTRING
#  endif /* CONFIG_CAN_ISR_POSTSTRING  */

/* --------------------------------------------------------------- */
# endif /* CONFIG_CPU_FAMILY_ARM */
/* --------------------------------------------------------------- */

/*----------------------------------------------------------*/
/* Real Time operating system extensions                    */
# ifdef CONFIG_OS_RTX51
/*----------------------------------------------------------*/
#  ifndef RTX51_MODIFIER 
#   define RTX51_MODIFIER compact reentrant
#  endif
/*----------------------------------------------------------*/
# endif /* CONFIG_OS_RTX51 */
/*----------------------------------------------------------*/

/* default atomar Flag handling
---------------------------------------------------------------*/
# ifdef CONFIG_CPU_FAMILY_C166
/* #  include <flag_167.h> */
# endif

/* default Compiler specific settings 
---------------------------------------------------------------*/
# include <co_default.h>
#endif /* __CO_KEIL_H */
