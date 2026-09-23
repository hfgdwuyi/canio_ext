/*
 * co_gcc.h - special defines for the GCC Compiler
 *
 * Copyright (c) 2004-2017 port GmbH Halle/Saale
 *------------------------------------------------------------------
 */

/**
* \file co_gcc.h
* \author port GmbH
*
*++ This header file supports the GCC Compiler.
*-- Diese Header-Datei enthält Defines zur Verwendung
*-- mit dem GCC-Compiler.
*
*/

#ifndef __CO_GCC_H
#  define __CO_GCC_H

#  include <stddef.h>

/**************************************************************************
* Begin of special settings
**************************************************************************/
#  ifdef CONFIG_COMPILER_GCC_WINAVR
   /* special WINAVR settings */
#  endif /* CONFIG_COMPILER_GCC_WINAVR */

#  ifdef CONFIG_CPU_FAMILY_LPC24
/* special GCC - LPC24 settings */
#    define CONFIG_ISR_POSTSTRING_DECL __attribute__ ((interrupt ("IRQ")))
#  endif /* CONFIG_CPU_FAMILY_LPC24 */

#  ifdef CONFIG_CPU_FAMILY_AT91SAM7X
/* CrossStudio Settings */
//# define CONFIG_CAN_ISR_PRESTRING void
//# define CONFIG_CAN_ISR_PRESTRING __attribute__((interrupt("IRQ"), section(".arm"))) void
//# define CONFIG_CAN_ISR_POSTSTRING
//# define CONFIG_CAN_ISR_POSTSTRING_DECL __attribute__ ((interrupt ("IRQ")))
//# define CONFIG_TIMER_ISR_PRESTRING __attribute__((interrupt("IRQ"), section(".arm"))) void
//# define CONFIG_TIMER_ISR_PRESTRING void
//# define CONFIG_TIMER_ISR_POSTSTRING
//# define CONFIG_TIMER_ISR_POSTSTRING_DECL __attribute__ ((interrupt ("IRQ")))
#    define CONFIG_ISR_PRESTRING void
#    define CONFIG_ISR_POSTSTRING

#    ifndef DI_FLAG
#      define DI_FLAG(flagfunction)	do{\
  	        Disable_CPU_Interrupts();	\
  	        (flagfunction);	\
	        Restore_CPU_Interrupts();	\
	        }while(0)
#    endif /* DI_FLAG */

extern unsigned char coLibFlags;
#    define SET_COLIB_FLAG(FLAG)   DI_FLAG(GL_ARRAY(coLibFlags) |= (FLAG))
#    define RESET_COLIB_FLAG(FLAG) DI_FLAG(GL_ARRAY(coLibFlags) &= ~(FLAG))
#    define TEST_COLIB_FLAG(FLAG)  (GL_ARRAY(coLibFlags) & (FLAG))
#    define SET_COLIB_FLAG_ISR(FLAG)   (GL_ARRAY(coLibFlags) |= (FLAG))
#    define RESET_COLIB_FLAG_ISR(FLAG) (GL_ARRAY(coLibFlags) &= ~(FLAG))

extern unsigned char coCanFlags;
#    define SET_CAN_FLAG(FLAG)    DI_FLAG(GL_ARRAY(coCanFlags) |= (FLAG))
#    define RESET_CAN_FLAG(FLAG)  DI_FLAG(GL_ARRAY(coCanFlags) &= ~(FLAG))
#    define TEST_CAN_FLAG(FLAG)   (GL_ARRAY(coCanFlags) & (FLAG))
#    define SET_CAN_FLAG_ISR(FLAG)    (GL_ARRAY(coCanFlags) |= (FLAG))
#    define RESET_CAN_FLAG_ISR(FLAG)  (GL_ARRAY(coCanFlags) &= ~(FLAG))


#  endif /* CONFIG_CPU_FAMILY_AT91SAM7X */


#  if defined(CONFIG_CPU_FAMILY_STM32)    ||        \
      defined(CONFIG_CPU_FAMILY_STM32_F0) ||        \
      defined(CONFIG_CPU_FAMILY_STM32_F2) ||        \
      defined(CONFIG_CPU_FAMILY_STM32_F4) ||        \
      defined(CONFIG_CPU_FAMILY_STM32_F7) ||        \
      defined(CONFIG_CPU_FAMILY_STM32_FREERTOS)
/* save atomic bit operation for this ARM cortex cpu */
#    ifndef DI_FLAG
extern void Disable_CPU_Interrupts(void);
extern void Restore_CPU_Interrupts(void);
#      define DI_FLAG(flagfunction)	do{\
            Disable_CPU_Interrupts();	\
  	        (flagfunction);	\
	        Restore_CPU_Interrupts();	\
	   }while(0)
#    endif /* DI_FLAG */

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

#  endif /* CONFIG_CPU_FAMILY_STM32 */

#  if defined(CONFIG_CPU_FAMILY_KINETIS_K2X) ||     \
	  defined(CONFIG_CPU_FAMILY_KINETIS_K60X) ||    \
	  defined(CONFIG_CPU_FAMILY_KINETIS_K10X)  ||   \
	  defined(CONFIG_CPU_FAMILY_KINETIS_KE06)
/* save atomic bit operation for cortex M cpu */
#    ifndef DI_FLAG
extern void Disable_CPU_Interrupts(void);
extern void Restore_CPU_Interrupts(void);
#      define DI_FLAG(flagfunction)	do{\
            Disable_CPU_Interrupts();	\
  	        (flagfunction);	\
	        Restore_CPU_Interrupts();	\
	   }while(0)
#    endif /* DI_FLAG */

extern unsigned char coLibFlags;
#    define SET_COLIB_FLAG(FLAG)   DI_FLAG(GL_ARRAY(coLibFlags) |= (FLAG))
#    define RESET_COLIB_FLAG(FLAG) DI_FLAG(GL_ARRAY(coLibFlags) &= ~(FLAG))
#    define TEST_COLIB_FLAG(FLAG)  (GL_ARRAY(coLibFlags) & (FLAG))
#    define SET_COLIB_FLAG_ISR(FLAG)   DI_FLAG(GL_ARRAY(coLibFlags) |= (FLAG))
#    define RESET_COLIB_FLAG_ISR(FLAG) DI_FLAG(GL_ARRAY(coLibFlags) &= ~(FLAG))

extern unsigned char coCanFlags;
#    define TEST_CAN_FLAG(FLAG)   (GL_ARRAY(coCanFlags) & (FLAG))
#    define SET_CAN_FLAG_ISR(FLAG)    DI_FLAG(GL_ARRAY(coCanFlags) |= (FLAG))
#    define RESET_CAN_FLAG_ISR(FLAG)  DI_FLAG(GL_ARRAY(coCanFlags) &= ~(FLAG))
#    define SET_CAN_FLAG(FLAG)    DI_FLAG(GL_ARRAY(coCanFlags) |= (FLAG))
#    define RESET_CAN_FLAG(FLAG)  DI_FLAG(GL_ARRAY(coCanFlags) &= ~(FLAG))

/* used in some freescale cpu Peripheral Access Layer */
#    define DIRECT DIRECT

#  endif /* CONFIG_CPU_FAMILY_KINETIS_K2X */

#  if defined(CONFIG_CPU_FAMILY_XMC4000)   ||     \
	  defined(CONFIG_CPU_FAMILY_AURIX)     ||     \
	  defined(CONFIG_CPU_FAMILY_FM3)
/* save atomic bit operation for some infineon, cypress cpu's */
#    ifndef DI_FLAG
extern void Disable_CPU_Interrupts(void);
extern void Restore_CPU_Interrupts(void);
#      define DI_FLAG(flagfunction)	do{\
            Disable_CPU_Interrupts();	\
  	        (flagfunction);	\
	        Restore_CPU_Interrupts();	\
	   }while(0)
#    endif /* DI_FLAG */

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

#  endif

/**************************************************************************
* End of special settings
**************************************************************************/

#  if defined(CONFIG_CPU_FAMILY_QNX)
#    if defined(CONFIG_CPU_TYPE_XZYNC_ULTRASCALE_QNX)
    /* special data types QNX 64bit system */
#     define	INTEGER16_T	    short
#     define	INTEGER32_T     int
#     define	UNSIGNED32_T    unsigned int
#    endif
#  endif

#  ifndef REGISTER
#    define REGISTER 	register
#  endif /* REGISTER */

#  ifndef RESTRICT
#    define RESTRICT 	restrict
#  endif /* RESTRICT */

#ifndef NULL
#define NULL __null
#endif

#  ifdef CONFIG_USE_64_BIT_DATATYPES
#    define PTR_DATA_TYPE_T	unsigned long int
#    define UNSIGNED64_T	unsigned long int
#    define UNSIGNED32_T	unsigned int
#    define UNSIGNED16_T	unsigned short int
#    define INTEGER32_T	signed int
#    define INTEGER16_T	signed short int
#  endif /* CONFIG_USE_64_BIT_DATATYPES */

/* default Compiler specific settings */
#  include <co_default.h>
#endif /* __CO_GCC_H */
