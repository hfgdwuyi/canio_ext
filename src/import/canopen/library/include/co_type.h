/*
 * co_type - defines basic types for CANopen
 *
 * Copyright (c) 2001-2017 port GmbH Halle/Saale
 *------------------------------------------------------------------
 */

/**
* \file co_type.h
*++ Defines basic types for CANopen
*-- Basisdatentypen für CANopen
* \author port GmbH, Halle Saale
*
*++ This file contains atomic types for the CANopen library.
*-- Dieses File enthält die Basic Daten Typen für die CANopen Library
*/

#ifndef __CO_TYPE_H
# define __CO_TYPE_H

/* CANopen Basic Types */

# ifdef BOOLEAN
#  define BOOL_T	BOOLEAN
# else /* BOOLEAN */
#  define BOOL_T	unsigned char
# endif /* BOOLEAN */
# ifndef UNSIGNED32_T
#  define	UNSIGNED32_T	unsigned long int
# endif /* UNSIGNED32_T */
# ifndef UNSIGNED16_T
#  define	UNSIGNED16_T	unsigned short int
# endif /* UNSIGNED16_T */
# ifndef UNSIGNED8_T
#  define	UNSIGNED8_T	unsigned char
# endif /* UNSIGNED8_T */
# ifndef INTEGER32_T
#  define	INTEGER32_T	long int
# endif /* UNSIGNED32_T */
# ifndef INTEGER16_T
#  define	INTEGER16_T	short int
# endif /* UNSIGNED16_T */
# ifndef INTEGER8_T
#  define	INTEGER8_T	signed char
# endif /* UNSIGNED8_T */
# ifndef REAL32_T
#  define	REAL32_T	float
# endif /* UNSIGNED8_T */
# ifndef PTR_DATA_TYPE_T
#  define PTR_DATA_TYPE_T	UNSIGNED32
# endif /* PTR_DATA_TYPE_T */


typedef unsigned char 	VIS_STRING_T;	/* visual string */
typedef unsigned char 	OCT_STRING_T;	/* octet string */
typedef unsigned char 	BIT_STRING_T;	/* bit string */
typedef void *		DOMAIN_T;	/* domain */
typedef unsigned char	FLAG_T;		/* for flags */

/* numeric data types */
typedef UNSIGNED32_T	UNSIGNED32;
typedef UNSIGNED16_T	UNSIGNED16;
typedef UNSIGNED8_T	UNSIGNED8;
typedef INTEGER32_T	INTEGER32;
typedef INTEGER16_T	INTEGER16;
typedef INTEGER8_T	INTEGER8;
typedef REAL32_T	REAL32;
typedef PTR_DATA_TYPE_T	PTR_DATA_TYPE;



/* SPECIAL */
# ifdef CONFIG_EXTENDED_DATA_TYPES

/* for compiler doesn't provide U64 data type */
#  ifdef CONFIG_EMULATE_U64
/* if unsigned is not supported then signed is not supported */
typedef struct {
    char val[8];
} UNSIGNED64_T;

typedef struct {
    char val[8];
} INTEGER64_T;
#  else /* not CONFIG_EMULATE_U64 */

#    ifndef UNSIGNED64_T
#      define UNSIGNED64_T    unsigned long long int
#    endif /* UNSIGNED64_T */

#    ifndef INTEGER64_T
#      define INTEGER64_T    signed long long int
#    endif /* INTEGER64_T */
#  endif /* CONFIG_EMULATE_U64 */

#  ifndef UNSIGNED24_T
#   define	UNSIGNED24_T	UNSIGNED32_T
#  endif /*  UNSIGNED24_T */

typedef struct {
	char    val[5];
} UNSIGNED40_T;

typedef struct {
	char    val[6];
} UNSIGNED48_T;

typedef struct {
	char    val[7];
} UNSIGNED56_T;


#  ifdef CONFIG_BIG_ENDIAN
#define SET_U40(b1, b2, b3, b4, b5)   \
	{{ b1, b2, b3, b4, b5 }}
#define SET_U48(b1, b2, b3, b4, b5, b6)   \
	{{ b1, b2, b3, b4, b5, b6 }}
#define SET_U56(b1, b2, b3, b4, b5, b6, b7)   \
	{{ b1, b2, b3, b4, b5, b6, b7 }}
#   ifdef CONFIG_EMULATE_U64
#define SET_U64(b1, b2, b3, b4, b5, b6, b7, b8)   \
	{{ b1, b2, b3, b4, b5, b6, b7, b8 }}
#define SET_I64(b1, b2, b3, b4, b5, b6, b7, b8)   \
	{{ b1, b2, b3, b4, b5, b6, b7, b8 }}
#  else /* CONFIG_EMULATE_U64 */
#define SET_U64(b1, b2, b3, b4, b5, b6, b7, b8) \
	  ((UNSIGNED64)b1 << 56) | ((UNSIGNED64)b2 << 48)	\
	| ((UNSIGNED64)b3 << 40) | ((UNSIGNED64)b4 << 32)	\
	| ((UNSIGNED64)b5 << 24) | ((UNSIGNED64)b6 << 16)	\
	| ((UNSIGNED64)b7 << 8)  | ((UNSIGNED64)b8)
#define SET_I64(b1, b2, b3, b4, b5, b6, b7, b8) \
	  ((INTEGER64)b1 << 56) | ((INTEGER64)b2 << 48)	\
	| ((INTEGER64)b3 << 40) | ((INTEGER64)b4 << 32)	\
	| ((INTEGER64)b5 << 24) | ((INTEGER64)b6 << 16)	\
	| ((INTEGER64)b7 << 8)  | ((INTEGER64)b8)
#   endif /* CONFIG_EMULATE_U64 */
#  else /* CONFIG_BIG_ENDIAN */
#define SET_U40(b1, b2, b3, b4, b5)   \
	{{ b5, b4, b3, b2, b1 }}
#define SET_U48(b1, b2, b3, b4, b5, b6)   \
	{{ b6, b5, b4, b3, b2, b1 }}
#define SET_U56(b1, b2, b3, b4, b5, b6, b7)   \
	{{ b7, b6, b5, b4, b3, b2, b1 }}
#   ifdef CONFIG_EMULATE_U64
#define SET_U64(b1, b2, b3, b4, b5, b6, b7, b8)   \
	{{ b8, b7, b6, b5, b4, b3, b2, b1 }}
#define SET_I64(b1, b2, b3, b4, b5, b6, b7, b8)   \
	{{ b8, b7, b6, b5, b4, b3, b2, b1 }}
#  else /* CONFIG_EMULATE_U64 */
#define SET_U64(b1, b2, b3, b4, b5, b6, b7, b8) \
	  ((UNSIGNED64)b1 << 56) | ((UNSIGNED64)b2 << 48)	\
	| ((UNSIGNED64)b3 << 40) | ((UNSIGNED64)b4 << 32)	\
	| ((UNSIGNED64)b5 << 24) | ((UNSIGNED64)b6 << 16)	\
	| ((UNSIGNED64)b7 << 8)  | ((UNSIGNED64)b8)
#define SET_I64(b1, b2, b3, b4, b5, b6, b7, b8) \
	  ((INTEGER64)b1 << 56) | ((INTEGER64)b2 << 48)	\
	| ((INTEGER64)b3 << 40) | ((INTEGER64)b4 << 32)	\
	| ((INTEGER64)b5 << 24) | ((INTEGER64)b6 << 16)	\
	| ((INTEGER64)b7 << 8)  | ((INTEGER64)b8)
#   endif /* CONFIG_EMULATE_U64 */
#  endif /* CONFIG_BIG_ENDIAN */


typedef UNSIGNED64_T	UNSIGNED64;
typedef UNSIGNED56_T	UNSIGNED56;
typedef UNSIGNED48_T	UNSIGNED48;
typedef UNSIGNED40_T	UNSIGNED40;
typedef UNSIGNED24_T	UNSIGNED24;

typedef INTEGER64_T     INTEGER64;

# endif /* CONFIG_EXTENDED_DATA_TYPES */


# ifdef __C51__
   typedef UNSIGNED8 LOOPCNT_U8;	/* loop counter < 256 */
   typedef UNSIGNED16 LOOPCNT_U16;		/* loop counter < 0x8000 */
# else
   typedef int LOOPCNT_U8;		/* loop counter < 256 */
   typedef int LOOPCNT_U16;		/* loop counter < 0x8000 */
# endif

# if defined(CO_INLINE)
# else /* CO_INLINE */
#  define CO_INLINE
# endif /* CO_INLINE */


/* Compiler and architecture dependent memory types */
# ifdef CO_CONST
# else /* CO_CONST */
#  ifdef __C51__
#    define CO_CONST	code
#  else /* __C51__ */
#    define CO_CONST
#  endif /* __C51__ */
# endif /* CO_CONST */

# ifdef CO_DATA
# else /* CO_DATA */
#  ifdef __C51__
#    define CO_DATA	data
#  else /* __C51__ */
#    define CO_DATA
#  endif /* __C51__ */
# endif /* CO_DATA */

# ifdef CO_LIB_INIT_VAR
# else /* CO_LIB_INIT_VAR */
#  define CO_LIB_INIT_VAR
# endif /* CO_LIB_INIT_VAR */

# ifdef CO_LIB_UNINIT_VAR
# else /* CO_LIB_UNINIT_VAR */
#  define CO_LIB_UNINIT_VAR
# endif /* CO_LIB_UNINIT_VAR */

# ifdef CO_LIB_CONST_VAR
# else  /* CO_LIB_CONST_VAR */
#  define CO_LIB_CONST_VAR CO_CONST
# endif /* CO_LIB_CONST_VAR */

/**
* \var COB_KIND_T
*  kind of CAN object
* \note
*-- Werte sind teilweise bitkodiert
*++ values are partly bitcoded
* \attention
*-- In der Dokumentation angegeben Werte dienen nur der Referenz,
*-- sie können sich ändern
*++ values are subject of change, use the enum definition.
*/
typedef enum {
    CO_COB_DISABLED =       0x80,                     /**< COB disabled */
    CO_COB_RTR =            0x40,
    CO_COB_TX =             0x20,                     /**< tx cob */
    CO_COB_RX =             0x00,                     /**< rx cob */
    CO_COB_TX_RTR =         (CO_COB_TX | CO_COB_RTR), /**< rtr tx cob */
    CO_COB_RX_RTR =         (CO_COB_RX | CO_COB_RTR), /**< rtr rx cob */
    CO_COB_DIR_MASK =       0x20,                     /* direction mask */
                            /* common COB type mask */
    CO_COB_DIR_RTR_MASK =   (CO_COB_DIR_MASK | CO_COB_RTR),
    CO_COB_NMT_MASTER =     (CO_COB_TX + 1),
    CO_COB_NMT_SLAVE =      (CO_COB_RX + 2),
    CO_COB_GUARD_MASTER =   ((CO_COB_RX | CO_COB_RTR) + 3),
    CO_COB_GUARD_SLAVE =    ((CO_COB_TX | CO_COB_RTR) + 3),
    CO_COB_HB_PROD =        (CO_COB_TX + 4),
    CO_COB_HB_CONS =        (CO_COB_RX + 4),
    CO_COB_SYNC_PROD =      (CO_COB_TX + 5),
    CO_COB_SYNC_CONS =      (CO_COB_RX + 5),
    CO_COB_TIME_PROD =      (CO_COB_TX + 6),
    CO_COB_TIME_CONS =      (CO_COB_RX + 6),
    CO_COB_PDO_PROD =       (CO_COB_TX + 7),
    CO_COB_PDO_PROD_RTR =   ((CO_COB_TX | CO_COB_RTR) + 7),
    CO_COB_PDO_CONS =       (CO_COB_RX + 7),
    CO_COB_PDO_CONS_RTR =   ((CO_COB_RX | CO_COB_RTR) + 7),
    CO_COB_SDO_TX =         (CO_COB_TX + 8),
    CO_COB_SDO_RX =         (CO_COB_RX + 8),
    CO_COB_EMCY_PROD =      (CO_COB_TX + 9),
    CO_COB_EMCY_CONS =      (CO_COB_RX + 9),
    CO_COB_SRDO_PROD =      (CO_COB_TX + 10),
    CO_COB_SRDO_CONS =      (CO_COB_RX + 10),
    CO_COB_FLYMA_TX =       (CO_COB_TX + 11),
    CO_COB_FLYMA_RX =       (CO_COB_RX + 11),
    CO_COB_LSS_TX =         (CO_COB_TX + 12),
    CO_COB_LSS_RX =         (CO_COB_RX + 12),
    CO_COB_SDOMGR_TX =      (CO_COB_TX + 13),
    CO_COB_SDOMGR_RX =      (CO_COB_RX + 13),
    CO_COB_SDOREQ_TX =      (CO_COB_TX + 14),
    CO_COB_REDCY_RX =       (CO_COB_RX + 15),
    CO_COB_REDCY_TX =       (CO_COB_TX + 15),
    CO_COB_DEBUG_RX =       (CO_COB_RX + 16),
    CO_COB_DEBUG_TX =       (CO_COB_TX + 16),
    CO_COB_USER_RX =        (CO_COB_RX + 17),
    CO_COB_USER_TX =        (CO_COB_TX + 17),
    CO_COB_GFC_RX =         (CO_COB_RX + 18),
    CO_COB_GFC_TX =         (CO_COB_TX + 18),

    /* defined to supress warnings when casting bitcoded combinations to this type */
    CO_COB_SDO_TX_DISABLED       = (CO_COB_SDO_TX | CO_COB_DISABLED),
    CO_COB_SDO_RX_DISABLED       = (CO_COB_SDO_RX | CO_COB_DISABLED),

    CO_COB_PDO_PROD_DISABLED     = (CO_COB_PDO_PROD | CO_COB_DISABLED),
    CO_COB_PDO_CONS_DISABLED     = (CO_COB_PDO_CONS | CO_COB_DISABLED),
    CO_COB_PDO_PROD_RTR_DISABLED = (CO_COB_PDO_PROD_RTR | CO_COB_DISABLED),
    CO_COB_PDO_CONS_RTR_DISABLED = (CO_COB_PDO_CONS | CO_COB_DISABLED),

    CO_COB_HB_CONS_DISABLED      = (CO_COB_HB_CONS | CO_COB_DISABLED),
    CO_COB_HB_PROD_DISABLED      = (CO_COB_HB_PROD | CO_COB_DISABLED),
    CO_COB_GUARD_SLAVE_DISABLED  = (CO_COB_GUARD_SLAVE | CO_COB_DISABLED),
    CO_COB_GUARD_MASTER_DISABLED = (CO_COB_GUARD_MASTER | CO_COB_DISABLED),

    CO_COB_LSS_TX_DISABLED       = (CO_COB_LSS_TX | CO_COB_DISABLED),
    CO_COB_LSS_RX_DISABLED       = (CO_COB_LSS_RX | CO_COB_DISABLED),

    CO_COB_EMCY_PROD_DISABLED    = (CO_COB_EMCY_PROD | CO_COB_DISABLED),
    CO_COB_EMCY_CONS_DISABLED    = (CO_COB_EMCY_CONS | CO_COB_DISABLED),

    CO_COB_SYNC_PROD_DISABLED    = (CO_COB_SYNC_PROD | CO_COB_DISABLED),
    CO_COB_SYNC_CONS_DISABLED    = (CO_COB_SYNC_CONS | CO_COB_DISABLED),

    CO_COB_TIME_PROD_DISABLED    = (CO_COB_TIME_PROD | CO_COB_DISABLED),
    CO_COB_TIME_CONS_DISABLED    = (CO_COB_TIME_CONS | CO_COB_DISABLED),

    CO_COB_NMT_MASTER_DISABLED   = (CO_COB_NMT_MASTER | CO_COB_DISABLED),
    CO_COB_NMT_SLAVE_DISABLED    = (CO_COB_NMT_SLAVE | CO_COB_DISABLED),

    CO_COB_SRDO_PROD_DISABLED    = (CO_COB_SRDO_PROD | CO_COB_DISABLED),
    CO_COB_SRDO_CONS_DISABLED    = (CO_COB_SRDO_CONS | CO_COB_DISABLED)

} COB_KIND_T;


typedef enum { CO_BOOLEAN, CO_INTEGER, CO_UNSIGNED, CO_NIL,
	       CO_DUMMY_SPACE, CO_INVALID, CO_STRING} BASIC_DATA_T;
typedef enum { UNKNOWN, INITIALISING, STOPPED=4, OPERATIONAL=5,
		  PRE_OPERATIONAL=127, RESET_APPLICATION, RESET_COMM
		} NODE_STATE_T;

/* In 302-2 objects definition 1f82 differ a bit from normal node state */
typedef enum { REQ_UNKNOWN, REQ_INITIALISING, REQ_STOPPED=4, REQ_OPERATIONAL=5,
		  REQ_PRE_OPERATIONAL=127, REQ_RESET_APPLICATION=6, REQ_RESET_COMM=7
		} NODE_STATE_REQ_T;

typedef enum { CLIENT, SERVER } USER_T;	    /* for variable and domains */
typedef enum { CONSUMER, PRODUCER } CO_USER_T;

/**
*++ return values for service requests
*-- Rückgabewerte von Dienstfunktionen
* \attention
*-- Achtung: in der Dokumentation angegeben Werte dienen nur der Referenz,
*-- sie können sich ändern
*++ values are subject of change, use the enum definition.
*/

/* to be able to generate automatically the errors strings for the c-function
   commonRet2strin()
   take care, that always the first 'word' is the enum,
   the second the comment intruduction *!< for Doxygen
   and the third to end the text description
   And leave the RET_T comment untouched

   The enum name should consist only of Upper case letters, numbers and underscores
*/
typedef enum   /* RET_T */
{
  CO_OK,                  /*!<  0 request successful */
  CO_E_MEM,               /*!<  1 not enough memory */
  CO_E_NOT_EXIST,         /*!<  2 object doesn't exist */
  CO_E_ALREADY_EXIST,     /*!<  3 object already exist */
  CO_E_STATE,             /*!<  4 operation not allowed in this state */
  CO_E_TYPE,              /*!<  5 no matching type */
  CO_E_INHIBITED,         /*!<  6 inhibit time active */
  CO_E_NO_INITIATE,       /*!<  7 no initiate service executed */
  CO_E_BUSY,              /*!<  8 service is already running */
  CO_E_DATA_LENGTH,       /*!<  9 datatype doesn't fit in telegram */
  CO_E_ERROR_LENGTH,      /*!< 10 errortype doesn't fit in telegram */
  CO_E_NO_NETWORK,        /*!< 11 no network object */
  CO_E_RANGE,             /*!< 12 invalid range */
  CO_E_NAME_LENGTH,       /*!< 13 name length not correct */
  CO_E_NAME_SYNTAX,       /*!< 14 name syntax not correct */
  CO_E_NO_DATABASE,       /*!< 15 no COB database available */
  CO_E_DISABLED,          /*!< 16 object disabled */
  CO_E_SYNTAX,            /*!< 17 syntax error in data- or errortype description (internal) */
  CO_E_SYNTAX_D_TYPE,     /*!< 18 syntax error in datatype description */
  CO_E_SYNTAX_E_TYPE,     /*!< 19 syntax error in errortype description */
  CO_E_MAP,               /*!< 20 mapping error */
  CO_E_NO_ACCESS,         /*!< 21 no access to object dictionary */
  CO_E_NONEXIST_OBJECT,   /*!< 22 object doesn't exist */
  CO_E_NONEXIST_SUBINDEX, /*!< 23 subindex doesn't exist */
  CO_E_NO_READ_PERM,      /*!< 24 no read permission */
  CO_E_NO_WRITE_PERM,     /*!< 25 no write permission */
  CO_E_VALUE_TO_HIGH,     /*!< 26 value greater upper limit */
  CO_E_VALUE_TO_LOW,      /*!< 27 value smaller lower limit */
  CO_E_WRONG_SIZE,        /*!< 28 object has wrong size */
  CO_E_TRANS_TYPE, 	  /*!< 29 wrong trans type */
  CO_E_HARDWARE_FAULT,	  /*!< 30 hardware fault */
  CO_E_PARA_INCOMP,	  /*!< 31 parameter incompatible */
  CO_E_SDO_OTHER,	  /*!< 32 unknown sdo error */
  CO_E_SDO_CMD_SPEC_INVALID, /*!< 33 sdo command specifier invalid */
  CO_E_SDO_INVALID_BLKSIZE, /*!< 34 invalid sdo block size */
  CO_E_SDO_INVALID_BLK_SEQ, /*!< 35 invalid block sequence number */
  CO_E_SDO_INVALID_BLKCRC, /*!< 36 invalid sdo block crc sum */
  CO_E_SRD_NO_RESSOURCE,  /*!< 37 no resources available for sdo connection */
  CO_E_BAD_ERROR_CTRL,	  /*!< 38 bad requested error control mechanism */
  CO_E_SDO_TIMEOUT,	  /*!< 39 sdo timed out */
  CO_E_SDO_INVALID_TOGGLEBIT, /*!< 40 sdo invalid togglebit */
  CO_E_INVALID_TRANSMODE,  /*!< 41 sdo invalid transmode */
  CO_E_DEVICE_STATE,       /*!< 42 bad device state */
  CO_E_BAD_CRC,		   /*!< 43 bad CRC */
  CO_E_BAD_SERVICE,	   /*!< 44 service not allowed */
  CO_E_CAN_TRANS_BUF,	   /*!< 45 CAN transmit buffer full */
  CO_E_CAN_TRANS_ERROR,	   /*!< 46 CAN transmit error */
  CO_E_CAN_TRANS_TOUT,	   /*!< 47 CAN transmit timeoutimeout */
  CO_E_CAN_TRANS_TYPE,	   /*!< 48 CAN transmit - bad COB type */
  CO_E_UNKNOWN_NODE,	   /*!< 49 unknown node */
  CO_E_NO_MASTER,	   /*!< 50 no NMT Startup master */
  CO_E_BAD_NODEID,	   /*!< 51 bad node id */
  CO_E_BAD_TIMEVAL,	   /*!< 52 bad timer value */
  CO_E_NO_DATA_AVAILABLE,  /*!< 53 no data available (SDO abort code) */
  CO_E_INTERNAL_INCOMP,	   /*!< 54 internal incompatible */
  CO_E_SIZE_TOO_HIGH,      /*!< 55 data type does not match, size too high */
  CO_E_SIZE_TOO_LOW,       /*!< 56 data type does not match, size too low */
  CO_E_LIMIT_ORDER,        /*!< 57 maximum value is less than minimum value */
  CO_E_LOCAL_CONTROL,      /*!< 58 data cannot be transfered or stored because of local control */
  CO_E_DICTIONARY,         /*!< 59 object dictionary creation failed or no dictionary present */
  CO_MSG1,		   /*!< message one */
  CO_MSG2,		   /*!< message two */
  CO_SDO_IND_BUSY=99	   /*!< 99 SDO indication busy */
} RET_T;

/**
*++ CAN/CANopen error conditions
*-- CAN/CANopen Fehler-Bedingungen
*/
typedef enum {
   CO_BUS_OFF,           /*!< can-controller error */
   CO_ERROR_PASSIVE,     /*!< can-controller error */
   CO_OVERRUN,           /*!< can-controller error */
   CO_RX_BUFFER_OVERFLOW,/*!< receive buffer overflow */
   CO_TX_BUFFER_OVERFLOW,/*!< transmit buffer overflow */
   CO_DRIVER_ERROR,      /*!< couldn't connect to driver */
   CO_LOST_GUARDING_MSG, /*!< slave misses guarding message */
   CO_NODE_STATE,        /*!< changed state recognized by guarding */
   CO_LOST_CONNECTION,   /*!< life time elapsed for node */
   CO_INVALID_COB,       /*!< protocol error (domain protocol) */
   CO_ERROR_ACTIVE,      /*!< CAN controller is active again */
   CO_GUARDING_STARTED,  /*!< first guarding message received */
   CO_LOST_HEARTBEAT,	 /*!< lost heartbeat message */
   CO_BOOT_UP,		 /*!< Bootup Message received */
   CO_HB_STARTED	 /*!< first Heartbeat received */
} ERROR_SPEC_T;

typedef enum { CO_DISABLED, CO_ENABLED } STATE_T;


# ifndef COB_IDENT_T
#  ifdef CONFIG_STANDARD_IDENTIFIER
    /** Message Identifier Datatype */
    typedef UNSIGNED16 COB_IDENT_T;
#  else /* CONFIG_STANDARD_IDENTIFIER */
    /** Message Identifier Datatype */
    typedef UNSIGNED32 COB_IDENT_T;
#  endif /* CONFIG_STANDARD_IDENTIFIER */
# endif /* COB_IDENT_T */


#define CAN_29_BIT_ID_FLAG  0x20000000UL
#define CAN_11_BIT_ID_MASK  0x7ffUL
#define CAN_29_BIT_ID_MASK  0x1fffffffUL
#define CAN_BIT_ID_MASK	    0x3fffffffUL


#define CO_FALSE	0
#define CO_TRUE		1

/* CANopen protocol errorCodes */
#define CO_PROT_ERR_NMT_CMD     0x0001u
#define CO_PROT_ERR_NMT_LEN     0x0002u
#define CO_PROT_ERR_HBC_LEN     0x0003u

#endif		/* __CO_TYPE_H */
