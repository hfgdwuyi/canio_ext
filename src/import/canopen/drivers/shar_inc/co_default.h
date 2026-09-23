/*
 * co_default.h - default Compiler settings
 *
 * Copyright (c) 2008-2017 port GmbH Halle/Saale
 *------------------------------------------------------------------
 *
 */

/**
* \file co_default.h
*++ Default Compiler Definitions
*-- Default Compiler Definitionen
* \author port GmbH
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


#ifndef CO_DEFAULT_H
#define CO_DEFAULT_H 1

/* datatype size_t */
/* # include <stddef.h> */

/*----------------------------------------------------------*/
/* Default values */
/*----------------------------------------------------------*/
/**
* \def INTERRUPT
*++ typical value is: interrupt
*-- Typischer Wert: interrupt
*
*++ It should be used to set \c CONFIG_CAN_ISR_PRESTRING and
*++ \c CONFIG_TIMER_ISR_PRESTRING.
*-- Dieses Makro wird für die Definition von \c CONFIG_CAN_ISR_PRESTRING
*-- und \c CONFIG_TIMER_ISR_PRESTRING benötigt.
*
*/
# ifndef INTERRUPT
#  define INTERRUPT
# endif

/**
* \def REGISTER
*++ compiler key \c register, if supported
*-- Compiler Schlüsselwort \c register, wenn unterstützt
*/
# ifndef REGISTER
#  define REGISTER register
# endif

/**
* \def CO_BIT
*++ datatype with value 0 or 1
*-- Datentyp mit dem Wertebereich [0,1]
*/
#  ifndef CO_BIT
#    define CO_BIT UNSIGNED8
#  endif

/**
* \def CONFIG_ISR_PRESTRING
*++ If all interrupt function have the same value before the function name,
*++ e.g. void, this define can be used to set this value.
*++
*++ Note: The interrupt function itself use \c CONFIG_CAN_ISR_PRESTRING and
*++ \c CONFIG_TIMER_ISR_PRESTRING, but you can use this definition
*++ to set this special defines.
*-- \c CONFIG_ISR_PRESTRING definiert einen Wert,
*-- der allen Interrupt Funktionen vorrangestellt wird.
*--
*-- Achtung: Dieses Define wird gegebenenfalls für die Definition
*-- von \c CONFIG_CAN_ISR_PRESTRING und \c CONFIG_TIMER_ISR_PRESTRING
*-- benutzt.
*/
# ifndef CONFIG_ISR_PRESTRING
/* #  define CONFIG_ISR_PRESTRING */
#  define CONFIG_ISR_PRESTRING void
# endif

/**
* \def CONFIG_ISR_PRESTRING_DECL
*++ same like CONFIG_ISR_PRESTRING, but used for the function declaration
*-- entspricht CONFIG_ISR_PRESTRING in der Funktionsdeklaration
*/
# ifndef CONFIG_ISR_PRESTRING_DECL
#  define CONFIG_ISR_PRESTRING_DECL CONFIG_ISR_PRESTRING
# endif

/**
* \def CONFIG_ISR_POSTSTRING
*++ If all interrupt function have the same value
*++ after the function declaration,
*++ e.g. __irq, this define can be used to set this value.
*++
*++ Note: The interrupt function itself use \c CONFIG_CAN_ISR_POSTSTRING and
*++ \c CONFIG_TIMER_ISR_POSTSTRING, but you can use this definition
*++ to set this special defines.
*-- \c CONFIG_ISR_POSTSTRING definiert einen String,
*-- der allen Interruptfunktionen nachgestellt wird.
*--
*-- Achtung: Dieses Define wird gegebenenfalls für die Definition
*-- von \c CONFIG_CAN_ISR_POSTSTRING und \c CONFIG_TIMER_ISR_POSTSTRING
*-- benutzt.
*/
# ifndef CONFIG_ISR_POSTSTRING
#  define CONFIG_ISR_POSTSTRING
# endif

/**
* \def CONFIG_ISR_POSTSTRING_DECL
*++ If all interrupt function have the same value
*++ after the function declaration,
*++ e.g. __irq, this define can be used to set this value,
*++ if it is not possible at the function definition.
*++
*-- \c CONFIG_ISR_POSTSTRING_DECL definiert einen String,
*-- der der Deklaration der Interruptfunktionen nachgestellt wird,
*-- wenn die Angabe bei der Definition nicht möglich ist.
*/
# ifndef CONFIG_ISR_POSTSTRING_DECL
#  define CONFIG_ISR_POSTSTRING_DECL CONFIG_ISR_POSTSTRING
# endif

/**
* \def CO_CONST
*++ \brief typical used to define constants
*++ If this define is empty initialized variables are use.
*++ If this value is const, constants are used.
*++ If this value a memory specifire, this memory block (e.g. code)
*++ is used.
*-- \brief typischerweise benutzt für die Definition von Konstanten
*-- Ist dieses Define leer, werden initialisierte Variable benutzt.
*-- Ist dieses Define const, werden Konstante benutzt.
*-- Wird \c CO_CONST mit einem Memory Spezifier belegt,
*-- wird der entsprechende Speicherbereich für die Variable benutzt.
*/
#  ifndef CO_CONST
#    define CO_CONST const
#  endif

/**
* \def CO_DATA
*++ \c data memory for 8051
*-- \c data Speicherbereich für den 8051
*/
#  ifndef CO_DATA
#    define CO_DATA
#  endif

/**
* \def IDATA
*++ \c idata memory for 8051
*-- \c idata Speicherbereich des 8051
*/
#  ifndef IDATA
#    define IDATA
#  endif

/**
* \def PDATA
*++ \c pdata memory for 8051
*-- \c pdata Speicherbereich des 8051
*/
#  ifndef PDATA
#    define PDATA
#  endif

/**
* \def CO_CODE
*++ \c code memory for 8051
*-- \c code Speicherbereich des 8051
*/
#  ifndef CO_CODE
#    define CO_CODE
#  endif

/**
* \def FAR
*++ used for large pointer, e.g. far, __far or huge
*-- benutzt für large Zeiger, z.B. far, __far oder huge
*/
#  ifndef FAR
#    define FAR
#  endif

/**
* \def NEAR
*++ used for near pointers
*-- benutzt für near Zeiger
*
*++ Near-pointers are used for access to variables located
*++ in RAM and Stack.
*-- Near-Zeiger werden für die Adressierung von Variablen
*-- benutzt, die sich im Ram und auf dem Stack befinden.
*/
#  ifndef NEAR
#    define NEAR
#  endif

/**
* \def XDATA
*++ \c xdata memory for 8051
*-- \c xdata Speicherbereich des 8051
*/
#  ifndef XDATA
#    define XDATA
#  endif

/**
* \def DIRECT
*++ direct accessable memory of Fujitsu CPUs
*-- direct accessable memory der Fujitsu CPUs
*
*++ Note: Not useable for function parameter!
*-- Achtung: \c DIRECT darf nicht für Funktions Parameter benutzt werden.
*/
#  ifndef DIRECT
#    define DIRECT
#  endif

/**
 * \def CO_MEM_QUICKRAM
 *++ quick access RAM
 *++ You cannot access this memory with pointers!
 *-- Besonders schnell addressierbarer RAM.
 *-- Es können keine Pointer auf die hier abgelegten
 *-- Variablen gebildet werden!
 *
 */
# ifndef CO_MEM_QUICKRAM
#  define CO_MEM_QUICKRAM
/* #  define CO_MEM_QUICKRAM CO_DATA DIRECT */
# endif

/**
 * \def CO_MEM_RAM
 *++ RAM memory specifier
 *-- immer RAM
 * CO_MEM_RAM UNSIGNED8 * ram_ptr;
 */
# ifndef CO_MEM_RAM
#  define CO_MEM_RAM
/* e.g. #  define CO_MEM_RAM XDATA NEAR */
# endif

/**
 * \def CO_MEM_CAN
 *++ CAN memory specifier
 *++ (experimental version, yet)
 *++ Should be __far, if the CAN is outside of the current memory model
 *-- Memorybereich des CAN Controllers
 *-- z.B. __far
 *
 */
# ifndef CO_MEM_CAN
#  define CO_MEM_CAN
/* e.g. #  define CO_MEM_CAN FAR */
# endif

/**
* \def VOLATILE
*++ volatile compiler key
*-- volatile Kompiler Schlüsselwort
*/
#  ifndef VOLATILE
#    define VOLATILE 	volatile
#  endif

/**
* \def RESTRICT
*++ restrict compiler key, if supported
*-- restrict Kompiler Schlüsselwort, wenn unterstützt
*/
# ifndef RESTRICT
#  define RESTRICT
# endif

/**
* \def CO_INLINE
*++ inline compiler key, if very flexible supported (like gcc)
*-- inline Kompiler Schlüsselwort, wenn sehr flexibel unterstützt (analog gcc)
*/
# ifndef INLINE
	/* old */
#  define INLINE
# endif

# ifndef CO_INLINE
#  define CO_INLINE
# endif


#endif /* CO_DEFAULT_H */
