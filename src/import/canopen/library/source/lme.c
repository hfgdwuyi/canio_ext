/*
 *++ lme - Initialisation of the Layer Management Entity
 *-- lme - Initialisierung der Layer Management Entity
 *
 * Copyright (c) 1995-2017 port GmbH Halle (Saale)
 *------------------------------------------------------------------
 */


/****************************************************************************/
/**
*  \file lme.c
*++  Initialisation of the Layer Management Entity
*--  Initialisierung der Layer Management Entity
*  \author port GmbH Halle (Saale)
*
*++ This modul contains functions to initialize
*++ or deactivate the CANopen Library.
*-- Mit den hier beschriebenen Funktionen
*-- wird die CANopen Library
*-- initialisiert bzw. beendet.
*
*++ Closely associated with this module
*++ are the functions to initialize and serve
*++ CAN controller initCAN() and CANopen timer initTimer().
*-- In engem funktionalen Zusammenhang zu diesem Modul stehen auch die
*-- hardwarenahen Funktionen zur Initialisierung und Bedienung von
*-- CAN Controller initCAN()
*-- und Timer initTimer().
*
*/


/****************************************************************************/
/**
*++ \mainpage The CANopen Library Reference Manual
*-- \mainpage Das CANopen Library Referenz Handbuch
*
*++ \section i Introduction
*++ The CANopen Protocol Library by
*++ \e port
*++ is an extensible software package.
*++ It conforms to the standard
*++ "CANopen Application Layer and Communication Profile"
*++ CiA301
*++ and other profiles of CiA e.V. and EN50325-4, respectively.
*
*++ The library is available as a slave-only and a master-or-slave version.
*++ Additionally
*++ \e port
*++ offers a multi CAN line version.
*
*++ \note
*++ \b Attention
*++    This manual contains the description of all modules and functions available. Not all of these
*++    are present in every delivery version.
*++    Besides this, many functions are presented here with the \b canLine parameter of the CANopen multi line version.
*++    If you are working with the single line version only, this parameter has to be left out.
*
*++ \section a Application
*++ The library is fully ANSI-C coded.
*
*++ Hardware specific interfaces are located in separate modules, providing an easy adaption to different systems.
*
*++ Usage of an application layer decouples
*++ the application completely from the communication system, thus making applications more portable and easier to maintain.
*
*++ \section b Description
*++ \e CANopen
*++ Library by \e port
*++ offers the following CANopen properties:
*++ \li Minimum Boot Up
*++ \li NMT services
*++ \li Service Data Object (SDO)
*++ \li Process Data Object (PDO)
*++ \li Emergency Object (EMCY)
*++ \li Synchronization Object (SYNC)
*++ \li Time Stamp (TIME)
*++ \li Node Guarding/Heartbeat
*++ \li Program Download
*++ \li SDO-Manager
*++ \li Flying Master
*++ \li Layer Setting services (LSS)
*++ \li Redundancy Support
*++ \li Safety relevant communication (SRDO)
*
*++ The standard Boot Up for CANopen Devices (Minimum Boot Up) has been
*++ implemented in the CANopen Library.
*++ That guarantees an automatic device initialization.
*++ After that the device will be forced in the state PRE_OPERATIONAL.
*++ In this state
*++ the user is able to change the CAN Object Identifier (COB-ID) of
*++ the CANopen services via SDO communication.
*
*++ Further it is possible
*++ to set the PDO parameters and their mapping (variable PDO Mapping).
*++ The implemented PDOs support the asynchronous, synchronous, cyclic
*++ and acyclic transmission modes.
*++ The number of usable CANopen Data Objects (SDO and PDO) depends only on
*++ memory restrictions of the user's target hardware.
*
*++ The Object Dictionary contains references to the user's application variables.
*++ The user variables can be included in the Object Dictionary
*++ without any changes of the application code.
*
*++ The interface to the user application is built by indication functions.
*++ With these functions the user can determine which types of reactions will be
*++ processed on an alteration of Object-Dictionary entries.
*
*++ A further highlight of this library is the scalability.
*++ Every kind of CANopen service is located in its own module e.g pdo.c, sdo.c.
*++ Therefore the user can select only the modules he actually needs.
*++ Additionally it is possible to use compiler defines
*++ to select several properties.
*++ The advantage is that the code size is proportional to
*++ the used CANopen functionalities.
*
*++ \e port
*++ supports the development, test and integration of CANopen devices
*++ with a complete set of tools.
*++ One of them is the CANopen Design Tool.
*++ This tool generates for every device
*++ the Object Dictionary implementation, the Electronic Data Sheet (EDS)
*++ and a documentation about the implemented device interface
*++ from a database.
*++ It reduces the development cycle and
*++ ensures the quality by the consistency of implementation and documentation.
*
*++ Further variants of the CANopen Library for supporting
*++ multiple CAN networks (max. 255) are available.
*++ With these the user can implement devices
*++ which can handle independent CAN networks,
*++ on targets without operating system
*++ or with operating systems without resource allocating mechanism.
*++ This is useful for building gateways and for a convenient segmentation
*++ of CAN networks.
*
*++ The highlights of this library are:
*++ \li supports all CANopen services
*++ \li all transmission modes of PDOs are implemented
*++ \li variable/dynamic PDO Mapping is possible
*++ \li bitwise PDO Mapping is possible
*++ \li supports PDO Dummy Mapping
*++ \li unlimited number of PDOs and SDOs
*++ \li easy interface to the user application
*++ \li universal Object Dictionary implementation
*++ created by a database tool
*++ \li Program download is possible
*++ \li Support of multiple CAN-networks possible
*++ \li scalable program code size
*++ supported by an interactive configuration tool.
*++ \li On-Line Reference Manual as UNIX-man pages
*++ or HTML files
*++ \li complete set of tools for generating
*++ the Object Dictionary, EDS and device documentation
*++ and for testing and integration
*
*
*-- \section e Einleitung
*-- Die CANopen-Library
*-- von
*-- \e port
*-- ist eine flexible CANopen-Bibliothek.
*-- Sie entspricht dem Standard
*-- "CANopen Application Layer and Communication Profile"
*-- CiA301
*-- und CiA302 des CiA e. V. beziehungsweise dem Standard EN50325-4.
*
*-- Sie ist verfügbar in den Varianten für
*-- Slave als auch für Master und Slave.
*-- Zusätzlich bietet
*-- \e port
*-- die o.g. Varianten auch als Multi-CAN-Linien-Version an.
*
*-- \note
*-- \b ACHTUNG: Im Referenzhandbuch sind alle Module und alle Funktionen aufgeführt,
*--    unabhängig davon, ob diese in iherer Auslieferung enthalten sind oder nicht.
*--    Ausserdem enthalten viele Funktionen den \b canLine Parameter der Multi Line Version.
*--    Besitzen Sie nur die Single Line Version der CANopen Library, entfällt dieser Parameter
*--    in allen Relevanten Funktionen.
*
*
*-- \section a Anwendung
*-- Der Kode wurde vollständig in ANSI-C erstellt.
*-- Alle Hardware-spezifischen Teile
*-- sind in separaten Treiber-Modulen kodiert,
*-- so dass eine einfache Portierung
*-- auf andere Hardwareplattformen problemlos möglich ist.
*-- Dafür steht ein definiertes Funktionsinterface zur Verfügung.
*
*-- Für die Nutzung der Kommunikationsdienste
*-- steht ein leistungsfähiges User-Interface zur Verfügung,
*-- das mit Funktionsaufrufen und Indikationfunktionen
*-- das Anwendungssystem
*-- von der Library entkoppelt.
*
*-- \section b Beschreibung
*-- Die CANopen-Library
*-- von \e port
*-- unterstützt folgende CANopen-Eigenschaften:
*-- \li Minimum Boot Up
*-- \li Service Data Object (SDO)
*-- \li Process Data Object (PDO)
*-- \li Multiplexed PDOs (MPDO)
*-- \li Emergency Object (EMCY)
*-- \li Synchronisation Object (SYNC)
*-- \li Time Stamp (TIME)
*-- \li Nodeguarding oder Heartbeat
*-- \li Programm Download
*-- \li LED Support
*-- \li SDO Manager
*-- \li Flying Master
*-- \li Redundancy Support
*-- \li Safety Relevant Communication
*-- \li Layer Setting Services (LSS)
*
*-- In der CANopen-Library
*-- ist das Standard-Netzwerk-Boot-Up
*-- für CANopen-Geräte
*-- (Minimum Boot Up) implementiert.
*-- Es garantiert ein automatisches
*-- Durchlaufen der Geräteinitialisierung.
*-- Im daran anschliessenden Knotenzustand
*-- PRE_OPERATIONAL können vom Anwender die CAN Object Identifier (COB-ID)
*-- den jeweiligen Nachrichtenobjekten mittels eines SDO zugewiesen werden.
*-- In diesem Zustand können weiterhin die PDO-Parameter verändert und den
*-- PDOs andere Variablen zugewiesen werden (variables PDO-Mapping).
*-- Die implementierten PDOs
*-- unterstützen den asynchronen, synchronen, zyklischen sowie
*-- azyklischen Sendemodus (Transmission Type).
*-- Ihre Anzahl sowie die Anzahl der
*-- verwendeten SDOs unterliegt nur den
*-- Speicherrestriktionen der Zielhardware.
*
*-- Das Objektverzeichnis (OV) ist so ausgelegt, dass es Referenzen auf
*-- die Variablen der Anwenderapplikation enthält.
*-- Damit ist es möglich,
*-- dass Variablen bereits existierender Software
*-- ohne Veränderung des Applikationskodes
*-- in das OV aufgenommen werden können.
*
*-- Die Schnittstelle zur Anwenderapplikation wird über Funktionen realisiert.
*-- In diesen kann der Anwender Reaktionen festlegen, die bei Änderung
*-- von Objektverzeichniseinträgen abgearbeitet werden.
*
*-- Eine weitere Besonderheit dieser Bibliothek ist ihre hohe Skalierbarkeit.
*-- Auf der einen Seite wird dies durch die Modularisierung der Bibliothek
*-- in einzelne Dienstgruppen z.B sdo.c, pdo.c, ... erreicht und zum anderen
*-- durch die Nutzung von Compilerdirektiven in den jeweiligen Modulen.
*-- Auf diese Art und Weise ist die Kodegrösse proportional zu den
*-- genutzten CANopen-Diensten.
*
*-- Zur Entwicklung, zum Test und zur Inbetriebnahme von CANopen-Geräten
*-- bietet
*-- \e port
*-- eine vollständige Toolkette an.
*-- Ein Werkzeug ist das CANopen-Design-Tool, das für jedes Gerät
*-- ein Objektverzeichnis, ein Electronic Data Sheet (EDS)
*-- und eine Dokumentation des Geräteinterfaces aus einer Datenbank generiert.
*-- Mit diesem Tool wird die Entwicklung entscheidend beschleunigt
*-- und die Konsistenz von
*-- Implementierung und Dokumentation gewährleistet.
*
*-- Weiterhin sind Varianten der Library zur
*-- Unterstützung mehrerer CAN Linien (max. 255) verfügbar.
*-- Damit ist es möglich,
*-- auf Geräten ohne Betriebssystem oder Betriebssystem
*-- mit unzureichenden Ressourcenschutzmechanismen mehrere
*-- von einander unabhängige CAN-Netzwerke zu bedienen.
*-- Es können damit Gateways geschaffen und Netze bequem segmentiert werden.
*
*-- Die Besonderheiten dieser Bibliothek sind:
*-- \li alle CANopen-Dienste werden unterstützt
*-- \li alle Sendemodi von PDOs sind implementiert
*-- \li variables/dynamisches PDO-Mapping ist möglich
*-- \li bitweises PDO-Mapping ist möglich
*-- \li unterstützt PDO-Dummy-Mapping
*-- \li unbegrenzte Anzahl von PDOs und SDOs
*-- \li einfache Schnittstelle zur Anwenderapplikation
*-- \li universale Objektverzeichnisimplementierung
*--     mit datenbankgestützter Generierung
*-- \li Programmdownload ist möglich
*-- \li Unterstützung mehrerer CAN-Netzwerke möglich
*-- \li hohe Skalierbarkeit der Kodegrösse
*--     unterstützt durch ein interaktives Konfigurationstool
*-- \li Online-Reference-Manual als UNIX-man-pages oder im HTML-Format
*-- \li komplette Toolkette zur Erzeugung von Objektverzeichnis,
*--     EDS und Dokumentation sowie zur
*--     Inbetriebnahme, zum Test und Integration verfügbar.
*
*
*++ \section s System enviroment
*++ The CANopen Library runs on targets with and without operating systems.
*++ It supports all available CAN controllers
*++ and many microcontrollers/processors.
*++ For detailed information see the data sheet of the CANopen Library Driver Packages.
*-- \section s Systemumgebung
*-- Die CANopen-Library
*-- ist lauffähig auf Plattformen mit und ohne Betriebssystem.
*-- Es werden alle gängigen CAN-Controller
*-- und viele Mikrocontroller/-prozessoren
*-- unterstützt.
*-- Für detallierte Informationen siehe Datenblatt CANopen-Driver-Packages.
*-
*++ Furthermore a CANopen Starter Kit for evaluation of the library
*++ is available.
*-- Weiterhin ist ein CANopen-Starter-Kit zur Evaluierung dieser Library
*-- erhältlich.
*-
*++ For further information
*++ please use the \b CANopen \b User \b Manual .
*-- Für weitere Informationen benutzen Sie bitte das
*-- \b CANopen \b Benutzerhandbuch.
*
*++ \note
*++ This documentation was created using the wonderful tool
*++ \b Doxygen http://www.doxygen.org/index.html .
*-- \note
*-- Die Dokumentation wurde unter Verwendung von
*-- \b Doxygen http://www.doxygen.org/index.html
*-- erstellt.
*
*
*/


/* header of standard C - libraries */

#include <stdio.h>
#include <string.h>
#include <ctype.h>

/* header of project specific types */

#include <cal_conf.h>

#include <co_lme.h>
#include <co_usr.h>
#include <co_setcp.h>
#include <co_flag.h>
#include <co_def.h>
#include "pdo.h"
#include "sdo.h"
#include "drv.h"
#include "nmt.h"
#include "nmt_s.h"
#include "emerg.h"
#include "access.h"
#include "nmterr.h"

#ifdef CONFIG_HEARTBEAT_CONSUMER
# include "heartbt.h"
#endif /* CONFIG_HEARTBEAT_CONSUMER */

#if defined(CONFIG_SYNC_PRODUCER) || defined(CONFIG_SYNC_CONSUMER)
# include "sync.h"
#endif /* defined(CONFIG_SYNC_PRODUCER) || defined(CONFIG_SYNC_CONSUMER) */

#if defined(CONFIG_TIME_PRODUCER) || defined(CONFIG_TIME_CONSUMER)
# include "time_lib.h"
#endif /* defined(CONFIG_TIME_PRODUCER) || defined(CONFIG_TIME_CONSUMER) */

#ifdef CONFIG_NON_VOLATILE_MEM
# include <co_stor.h>
#endif /* CONFIG_NON_VOLATILE_MEM */

#ifdef CONFIG_MASTER
# include "nmt_m.h"
#endif /* CONFIG_MASTER */

#ifdef CONFIG_EMCY_CONSUMER
# include "emerg.h"
#endif /* CONFIG_EMCY_CONSUMER */

#ifdef CONFIG_FLYING_MASTER
# include "flyma.h"
#endif /* CONFIG_FLYING_MASTER */

#if defined(CONFIG_SRDO_CONSUMER) || defined(CONFIG_SRDO_PRODUCER)
# include "srdo.h"
#endif /* CONFIG_SRDO_CONSUMER/PRODUCER */

#ifdef CONFIG_REDUNDANCY_SUPPORT
# include "reduncy.h"
#endif /* CONFIG_REDUNDANCY_SUPPORT */

#ifdef CONFIG_DYN_MEM_ALLOC
# include "dynmem.h"
#endif /* CONFIG_DYN_MEM_ALLOC */

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
#ifdef CONFIG_EVA_VERSION
extern TIMER_EVENT_T	evaTimer;
#endif /* CONFIG_EVA_VERSION */

/* global variables
---------------------------------------------------------------------------*/
#ifdef CONFIG_NO_GLOBAL_VARS
#else /* CONFIG_NO_GLOBAL_VARS */

CO_LIB_UNINIT_VAR UNSIGNED8	coNodeId CO_LINE_PARA_ARRAY_DEF;	/* CANopen Node Id */
/**
*++ this variable contains global flags
*++ indicating special events.
*++ Each call to internal function flagIndentification()
*++ first checks these flags.
*-- Diese Variable enthält globale Flags für spezielle Ereignisse.
*-- Bei jedem Aufruf der Funktion flagIdentification()
*-- werden diese Flags ausgewertet.
*/
CO_LIB_UNINIT_VAR UNSIGNED8	coLibFlags CO_REDCY_PARA_ARRAY_DEF;	/* CANopen flags */

#endif /* CONFIG_NO_GLOBAL_VARS */

/* local defined variables
---------------------------------------------------------------------------*/
#ifdef CONFIG_RCS_IDENT
CO_LIB_INIT_VAR static char _rcsid[] = "$Id: lme.c,v 2.57 2016/11/02 14:59:09 rli Exp $";
#endif /* CONFIG_RCS_IDENT */

/****************************************************************************/
/**
*
*++ \brief initCANopen - initialisation of the CANopen Library
*-- \brief initCANopen - Initialisierung der CANopen Library
*
*-- Diese Funktion ist für die Initialisierung der Daten- und Funktionaufrufe
*-- der Library notwendig.
*++ The function does all necessary initialization
*++ of the CANopen library functions and data.
*-- Weiterhin werden alle Werte im Kommunikationsteil des OV
*-- auf ihre Standardwerte (Predefined Connection Set) gesetzt.
*++ Additionally all values at the communication part at the object dictionary
*++ are set to it default values according the predefined connection set.
*-- Wenn der Knoten nichtflüchtigen Speicher besitzt,
*-- wird anschliessend die Indikation-Funktion
*++ If the node has non-volatile memory
*++ the indication function
* loadParameterInd()
*-- aufgerufen,
*++ is called,
*-- so dass die mit der Funktion
*++ to reload the values saved by
* saveParameterInd()
*-- gespeicherten Daten restauriert werden können.
*-- Der Anwender kann anschliessend auch andere Power-On Werte
*-- im OV hinterlegen,
*-- die dann bei der Initialisierung der Dienste genutzt werden.
*++ The user can load other power-on values after this function.
*-- Zur Ermittlung der Node-Id wird die Funktion
*++ To get the node id the user function
* getNodeId()
*-- aus dem File
*++ from the file
* usr_301.c
*-- aufgerufen, welche vom Anwender entsprechend bereitzustellen ist.
*++ is called.
* \par
*-- Diese Funktion ist nach der Hardware Initialisierung des CAN Controllers
*-- und vor der Initialisierung des Timers aufzurufen.
*++ It should be called after calling hardware initialization
*++ of the CAN controller and before initialization of the CANopen timer.
* \par
*-- Eine typische Aufrufreihenfolge für CANopen Applikationen ist:
*++ A typical calling sequence for CANopen applications might be:
* \par
* \code
* initCAN();
* initCANopen();
* Start_CAN();
* initTimer();
* \endcode
*
* \retval CO_OK
*-- Erfolg
*++ Success
* \retval CO_E_NO_ACCESS
*++ no access to Object Dictionary (Node - ID)
*-- kein Zugriff auf das Objektverzeichnis möglich (Node - ID)
* \retval O_E_BAD_NODEID
*-- ungültige Node id
*++ invalid node id
*
*/

RET_T initCANopen(
	CO_GLOBVARS_PARA_DECL
    )
{
RET_T retVal = CO_OK;
#ifdef CONFIG_MULT_LINES
UNSIGNED8	canLine;  /* number of CAN line 0..CO_MAX_CAN_LINES-1 */
#endif /* CONFIG_MULT_LINES */

#ifdef CONFIG_NO_GLOBAL_VARS
    GL_VAR(coTimerPulse) = CONFIG_TIMER_INC;
#else /* CONFIG_NO_GLOBAL_VARS */

# ifdef CONFIG_MULT_LINES
    GL_VAR(pObjDirMan) = co_objDirMan;/* pointer to od for multi line */
    GL_VAR(pMaxObjDicElements) = co_maxObjDicElements;
# else /* CONFIG_MULT_LINES */
    GL_VAR(pObjDir) = co_objDir;	/* pointer to od for single line  */
    GL_VAR(pMaxObjDicElements) = co_maxObjDicElements;
# endif /* CONFIG_MULT_LINES */
#endif /* CONFIG_NO_GLOBAL_VARS */

#ifdef CONFIG_EVA_VERSION
    /* don't allow more object dictionary entries for eval version */
    if (*pMaxObjDicElements > 50u)  {
        GL_ARRAY(coNodeId) = 0u;
        return(CO_E_NO_ACCESS);
    }
#endif /* CONFIG_EVA_VERSION */


#ifdef CONFIG_MULT_LINES
    for (canLine = 0u; canLine < CO_ACT_CAN_LINES; canLine++) {
#endif /* CONFIG_MULT_LINES */

	/* init the global library flags */
#ifdef CONFIG_REDUNDANCY_SUPPORT
        GL_ARRAY(coLibFlags) [0] = 0u;
        GL_ARRAY(coLibFlags) [1] = 0u;

        initRedcyVars(CO_LINE_PARA);
#else /* CONFIG_REDUNDANCY_SUPPORT */
        GL_ARRAY(coLibFlags) = 0u;
#endif /* CONFIG_REDUNDANCY_SUPPORT */

	   /* init timer list */
        GL_ARRAY(co_timerList) = NULL;
        GL_ARRAY(co_inhibitList) = NULL;

	   /* init timer ticks */
        GL_ARRAY(coTimerTicks) = 0u;

#ifdef CONFIG_HEARTBEAT_CONSUMER
        initHeartBeatVars(CO_LINE_PARA);
#endif /* CONFIG_HEARTBEAT_CONSUMER */

#if defined(CONFIG_HEARTBEAT_CONSUMER) \
	|| (defined(CONFIG_MASTER) && defined(CONFIG_NODE_GUARDING))
	initNmtErrVars(CO_LINE_PARA);
#endif /* defined(CONFIG_HEARTBEAT_CONSUMER) || (defined(CONFIG_MASTER) && defined(CONFIG_NODE_GUARDING)) */

#if defined(CONFIG_PDO_CONSUMER) || defined(CONFIG_PDO_PRODUCER)
	initPdoVars(CO_LINE_PARA);
#endif /* defined(CONFIG_PDO_CONSUMER) || defined(CONFIG_PDO_PRODUCER) */

#if defined(CONFIG_SDO_SERVER) || defined(CONFIG_SDO_CLIENT)
	initSdoVars(CO_LINE_PARA);
#endif /* defined(CONFIG_SDO_SERVER) || defined(CONFIG_SDO_CLIENT) */

#if defined(CONFIG_EMCY_PRODUCER) || defined(CONFIG_EMCY_CONSUMER)
	initEmcyVars(CO_LINE_PARA);
#endif /* defined(CONFIG_EMCY_PRODUCER) || defined(CONFIG_EMCY_CONSUMER) */

#ifdef CONFIG_CO_LED
	initLedVars(CO_LINE_PARA);
#endif /* CONFIG_CO_LED */

#if defined(CONFIG_SYNC_PRODUCER) || defined(CONFIG_SYNC_CONSUMER)
	initSyncVars(CO_LINE_PARA);
#endif /* defined(CONFIG_SYNC_PRODUCER) || defined(CONFIG_SYNC_CONSUMER) */

#if defined(CONFIG_TIME_PRODUCER) || defined(CONFIG_TIME_CONSUMER)
	initTimeVars(CO_LINE_PARA);
#endif /* defined(CONFIG_TIME_PRODUCER) || defined(CONFIG_TIME_CONSUMER) */

# ifdef CONFIG_MASTER
#  ifdef CONFIG_NODE_GUARDING
	initGuardVars(CO_LINE_PARA);
#  endif /* CONFIG_NODE_GUARDING */

	initNmtMasterVars(CO_LINE_PARA);
# endif /* CONFIG_MASTER */

#ifdef CONFIG_FLYING_MASTER
	initFlyMa(CO_LINE_PARA);
#endif /* CONFIG_FLYING_MASTER */


#if defined(CONFIG_SRDO_PRODUCER) || defined(CONFIG_SRDO_CONSUMER)
	initSrdoVars(CO_LINE_PARA);
#endif /* defined(CONFIG_SRDO_PRODUCER) && defined(CONFIG_SRDO_CONSUMER) */

	/* calls user function to get the node id from DIP switch or EEPROM */
	GL_ARRAY(coNodeId) = getNodeId(CO_LINE_PARA);
	if (GL_ARRAY(coNodeId) == 0u)  {
	    retVal = CO_E_BAD_NODEID;
	} else {

	    /* set comm-objects to their default cob-id values */
            (void) resetObjDir(MEM_SEG_ALL_PARAMETERS CO_COMMA_LINE_PARA);

#ifdef CONFIG_NON_VOLATILE_MEM
            /* load saved values from flash */
            loadParameterInd(MEM_SEG_ALL_PARAMETERS, CO_RESTORE_MODE_BOOTUP
		CO_COMMA_LINE_PARA);
#endif /* CONFIG_NON_VOLATILE_MEM */

#ifdef CONFIG_NO_ERROR_BEHAVIOR
#else /* CONFIG_NO_ERROR_BEHAVIOR */
	    /* load object 1029:1 if available */
	    setupCommErrorBehavior(CO_LINE_PARA);
#endif /* CONFIG_NO_ERROR_BEHAVIOR */

#ifdef CONFIG_MULT_LINES
        } /* end of for loop canLine */
#endif /* CONFIG_MULT_LINES */

#ifdef CONFIG_EVA_VERSION
# ifndef CONFIG_EVA_TIME
#  define CONFIG_EVA_TIME  60 /* minutes */
# endif
    /* reset application after 1 h */
    addTimerEvent(&evaTimer, 10UL * 1000UL * 60UL * CONFIG_EVA_TIME, 0);
#endif /* CONFIG_EVA_VERSION */
	}
    return (retVal);
}


/****************************************************************************/
/**
*
*-- \brief leaveCANopen - Beendigung von CANopen
*++ \brief leaveCANopen - leave and finish CANopen
*
*-- Mit dieser Funktion sollten
*-- nach Beendigung des Anwenderprogramms
*-- die CANopen-Initialisierungen wieder
*-- zurückgenommen werden.
*++ The application should use this function to
*++ reset all CANopen library inizializations,
*++ possibly after ending the application programm
* \par
*-- Die CAN Controller Hardware oder der Timer
*-- werden von dieser Funktion nicht berührt.
*-- Beide müssen aber vor Aufruf dieser Funktion deaktiviert werden.
*++ The function does not reset the hardware of the CAN controller
*++ nor the timer.
*++ But both have to be deactivated before calling
*++ leaveCANopen()
* \par
* \code
  ReleaseTimer();
  leaveCANopen();
* \endcode
*/

void leaveCANopen(
	CO_GLOBVARS_PARA_DECL
    )
{
    /* to avoid compiler warnings */
#if defined(CONFIG_NO_GLOBAL_VARS)
    /* CO_LINE_PARA = CO_LINE_PARA; */
    CO_INTERNAL_NOT_USED(CO_LINE_PARA);
#endif

#ifdef CONFIG_DYN_MEM_ALLOC
    /* release memory */
    freeDynMem();
#endif /* CONFIG_DYN_MEM_ALLOC */

    return;
}


/*______________________________________________________________________EOF_*/
