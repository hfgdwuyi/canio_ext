/*
 *++ co_debug - CANopen's functions for debug actions
 *-- co_debug - CANopen's Funktionen für debug Ausgaben
 *
 * Copyright (c) 2000-2017 port GmbH Halle (Saale)
 *------------------------------------------------------------------
 */


/*
* DESCRIPTION
*
* This file contains functions for debugging the CANopen library.
* Debug messages can be enabled/disabled at runtime
* over the variable co_debug.
* This variable is bitcoded.
*
* Functions
* .CS
* .CE
*
* INTERNAL
*
*/

/* header of standard C - libraries */
#include <cal_conf.h>
#include <co_debug.h>

#ifdef CONFIG_CO_DEBUG

# if defined(__STDC__) || defined(__BORLANDC__)
#  include <stdarg.h>
# else
#  include <varargs.h>
# endif

# if !defined(TARGET_FUJITSU_90540)
#  include <errno.h>
# endif

#endif /* CONFIG_CO_DEBUG */

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

/* external variables
---------------------------------------------------------------------------*/

/* global variables
---------------------------------------------------------------------------*/

/* local defined variables
---------------------------------------------------------------------------*/
#ifdef CONFIG_RCS_IDENT
CO_LIB_INIT_VAR static char _rcsid[] = "$Id: co_debug.c,v 2.10 2016/09/26 11:16:09 rli Exp $";
#endif


#ifdef CONFIG_CO_DEBUG

/*******************************************************************
*
* debugprint - print errormessage
*
* NOMANUAL
*
* Print a message  and return to caller.
* This version doesn't print errno and syserrormessage.
* Its use is indented in program debug messages.
*
* 	debugprint(level, fmt, arg1, arg2, ...)
*
* The string "fmt" must specify the conversion specification for any args.
*
* ARGUMENTS
* level - bitcoded error print level
* fmt - printf like format string
* .br
* arg - arguments to be formatted with informations in str
*
* RETURNS
* .TP
* nothing
*
* SEE ALSO: BDEBUG Macro
*
* INTERNAL:
* Die Übergabe der Variablen Parameter erfolgt mit Makros
* Ist __STDC__ == ANSI definiert, wird das Makropaket <stdarg.h> benutzt
* sonst <varargs.h>
*
*/
void debugprint(int level, char *fmt, ...)
{
va_list		args;

    /* should this message print out ? (use defines CO_DEBUG_xxx) */
    if ((level & co_debug) == 0)  {
	return;
    }

    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);

    fflush(stdout);
    fflush(stderr);

    return;
}

#endif /*  CO_DEBUG */
