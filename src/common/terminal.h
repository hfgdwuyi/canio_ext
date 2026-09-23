/*!
 * Copyright Siemens Healthcare GmbH 2022, All Rights Reserved
 *
 * Project: Building Block Low End MCU
 *
 * @file
 * @brief Header file for terminal.c
 */
/*----------------------------------------------------------------------------*/
#ifndef TERMINAL_H
#define TERMINAL_H

#include <stdbool.h>

/*! Return values item's callbacks */
typedef enum
{
    SHELL_OK = 0, /*!< Operation successfull */
    SHELL_EARGC,  /*!< Wrong number of arguments */
    SHELL_EARG,   /*!< Wrong arguments */
    SHELL_ECONV,  /*!< Convertion error */
    SHELL_EHW,    /*!< Hardware error */
    SHELL_ECBK,   /*!< Callback error */
    SHELL_OIP     /*!< Operation in progress */
} terminalRet;

/*! Typedef for terminal callback function */
typedef terminalRet (*callbackType)(uint8_t argc, char **argv);

/*! Terminal item, organized in linked list */
typedef struct terminalItem
{
    const char          *name;        //!< Command's name
    const char          *desc;        //!< Command's description
    const char          *help;        //!< Command's help
    callbackType         callback;    //!< Callback to be called, argc - number of arguments, argv - pointers to the arguments
    struct terminalItem *next;        //!< Pointer to the next item in the linked list
} terminalItem;

void terminalAddItem(terminalItem *item);
void terminalRun(void);
bool terminalStrToInt(const char *value, uint32_t *res);
void terminalFinishOperation(void);
#endif
