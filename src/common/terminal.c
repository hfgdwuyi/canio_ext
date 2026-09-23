/*!
 * Copyright Siemens Healthcare GmbH 2024, All Rights Reserved
 *
 * Project: Building Block Low End MCU
 *
 * @file
 * @brief Contains functions for serial terminal
 *
 */
/*----------------------------------------------------------------------------*/
// Standard includes
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

// HAL includes
#include <hal.h>

// Project includes
#include "terminal.h"
#include "mem_map.h"
#include "bsp_wtdg.h"
#include "timing.h"
#include "prog_ctrl.h"
#ifdef BOOTLOADER
#include "update.h"
#endif

// Compiler adaptation
#if __ARMCC_VERSION >= 6000000
#define STRTOK_R _strtok_r
#else
#define STRTOK_R strtok_r
#endif
/* Maximum number of received words from terminal*/
#define MAX_TOKENS_NUMBER 32
/* Maximum length of received string*/
#define MAX_LINE_LENGTH 128

#ifdef BOOTLOADER
bool               fileReceptionOn     = false;
uint32_t           receiveUartDataSize = 0;
static memoStore   memData;
static timingTimer resetWrite;
static uint32_t    receiveBuffIdx = 0;
#endif
// Static functions declaration
static void          terminalParseLine(char *argv);
static terminalRet   terminalShowHelp(uint8_t argc, char **argv);
static void          terminalExecuteOperation(uint8_t argc, char **argv);
static terminalItem *terminalFindItem(const char *name);
#ifdef BOOTLOADER
static terminalRet terminalEcho(uint8_t argc, char **argv);
static terminalRet terminalWriteIn(uint8_t argc, char **argv);
static terminalRet terminalReadIn(uint8_t argc, char **argv);
static terminalRet terminalEraseIn(uint8_t argc, char **argv);
static terminalRet terminalStartApplication(uint8_t argc, char **argv);
static terminalRet terminalMemMap(uint8_t argc, char **argv);
static terminalRet terminalCalcCRC(uint8_t argc, char **argv);


static terminalItem terminalCalculateCRC = { .name     = "crc",
                                             .desc     = "Calculates CRC of given memory space",
                                             .help     = "Calculates Cyclic Redundancy Check for the given address space in RAM, External EEPROM or Flash\n\n crc"
                                                         "[arg1][arg2][arg3]\n\narg1 - type of memory from where to start,\n"
                                                         "arg2 - starting memory address to start calculation from ,\n"
                                                         "arg3 - size of memory to use for calculation\n",
                                             .callback = terminalCalcCRC,
                                             .next     = NULL
};

static terminalItem terminalMemoMap = { .name     = "memmap",
                                        .desc     = "Returns controllers memory map",
                                        .help     = "Returns controllers memory map, existing internal and external memories with their types and sizes\n\n memmap\n",
                                        .callback = terminalMemMap,
                                        .next     = &terminalCalculateCRC
};

static terminalItem terminalStartApp = { .name     = "runapp",
                                         .desc     = "Starts execution of application",
                                         .help     = "Starts execution of application\n\n runapp "
                                                     "[arg1][arg2]\n\narg1 - type of memory from where to start,\n"
                                                     "arg2 - starting address, where application is stored,\n",
                                         .callback = terminalStartApplication,
                                         .next     = &terminalMemoMap };

static terminalItem terminalEraseFlash = { .name     = "erasemem",
                                           .desc     = "Erases external flash",
                                           .help     = "Erases External flash\n\n erasemem "
                                                       "[arg1][arg2]\n\narg1 - mass(erases all external flash), address(address to erase),\n"
                                                       "arg2 - size of flash to erase(ommited in case of mass erase),\n",
                                           .callback = terminalEraseIn,
                                           .next     = &terminalStartApp };


static terminalItem terminalRead = { .name     = "readmem",
                                     .desc     = "Reads data from RAM, Flash, EEPROM",
                                     .help     = "Reads data from internal RAM, External EEPROM or flash\n\n readmem "
                                                 "[arg1][arg2][arg3]\n\narg1 - memory type,\n"
                                                 "arg2 - address in memory to read from,\narg3 - size to read\n",
                                     .callback = terminalReadIn,
                                     .next     = &terminalEraseFlash };

static terminalItem terminalWrite = { .name     = "writemem",
                                      .desc     = "Writes data into RAM, Flash, EEPROM",
                                      .help     = "Writes data into internal RAM, External EEPROM or flash\n\n writemem "
                                                  "[arg1][arg2][arg3]\n\narg1 - memory type,\n"
                                                  "arg2 - address in memory to store to,\narg3 - size of file\n",
                                      .callback = terminalWriteIn,
                                      .next     = &terminalRead };

/*! Terminal item used to write received text in terminal */
static terminalItem terminalEchoItem = { .name     = "echo",
                                         .desc     = "Returns given message",
                                         .help     = "Returns arguments given to this command\n\n  echo "
                                                     "[arg1][arg2]...[arg n] \n\n\t arg1,arg2...arg n - arguments to return\n",
                                         .callback = terminalEcho,
                                         .next     = &terminalWrite
};
#else
static terminalRet   terminalBoot(uint8_t argc, char **argv);
/*! Terminal item used to switch to bootloader in terminal */
static terminalItem terminalBootItem = { .name     = "boot",
                                         .desc     = "Switches to bootloader",
                                         .help     = "Switches to bootloader after reseiving this command\n\n  boot \n",
                                         .callback = terminalBoot,
                                         .next     = NULL
};
#endif
/*! Terminal item used to show list of command available and help for each of them */
static terminalItem terminalHelpItem = { .name     = "help",
                                         .desc     = "Show this message",
                                         .help     = "Provides help information for available commands\n\n  help "
                                                     "[command] \n\n\t command - shows help for this command\n",
                                         .callback = terminalShowHelp,
#ifdef BOOTLOADER
                                         .next     = &terminalEchoItem
#else  
                                         .next     = &terminalBootItem
#endif
};

/*! Pointer to the head of the linked list of terminal items */
static terminalItem *terminalListHead = &terminalHelpItem;

/*! Busy flag */
static bool terminalBusyFlag;
#ifdef BOOTLOADER
void USART3_IRQHandler(void)
{
    if (__HAL_USART_GET_FLAG(&serialUart, USART_FLAG_RXFNE))
    {
        resetWrite.counter          = 5;
        file_buffer[receiveBuffIdx] = (uint8_t)(READ_BIT(serialUart.Instance->RDR, USART_RDR_RDR) & 0xFFU);
        receiveBuffIdx++;
    }
    if (receiveBuffIdx == receiveUartDataSize)
    {
        printf("\n\ni = %ld\n", receiveBuffIdx);
        receiveBuffIdx  = 0;
        fileReceptionOn = false;
        isUSBCommand    = false;
        NVIC_DisableIRQ(USART3_IRQn);
        memoryWrite(memData);
        timingRemoveTimer(&resetWrite);
    }
}

void resetWriteUART(void)
{
    receiveBuffIdx  = 0;
    fileReceptionOn = false;
    NVIC_DisableIRQ(USART3_IRQn);
    printf("Reception interrupted!! Timeout expired!\n");
}
#else
/*----------------------------------------------------------------------------*/
/*!
 * @brief         Switches application to bootloader
 *
 * @param[in]     argc number of command arguments
 * @param[in]     argv command arguments
 * @return        Status of operation
 */
/*----------------------------------------------------------------------------*/
static terminalRet terminalBoot(uint8_t argc, char **argv)
{
    (void)argc;
    (void)argv;
    progCtrlSetSignature(PROG_CTRL_START_BOOTLOADER);
    needToReset = true;
    return SHELL_OK;
}
#endif
/*----------------------------------------------------------------------------*/
/*!
 * @brief         Add item to the list
 * @param[in]     item item to add
 *
 */
/*----------------------------------------------------------------------------*/
void terminalAddItem(terminalItem *item)
{
    // begin from the list head
    terminalItem *head = terminalListHead;
    item->next         = NULL;
    // if head exists
    if (head != NULL)
    {
        // goto the last node
        while (head->next != NULL)
        {
            head = head->next;
        }
        // insert item at the end of the list
        head->next = item;
    }
    else
    {
        // make item a head of the list
        terminalListHead = item;
    }
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief         Looks for terminal item in the linked list by name given
 * @param[in]     name item name to search
 * @return        if found, pointer to the item, NULL otherwise
 */
/*----------------------------------------------------------------------------*/
static terminalItem *terminalFindItem(const char *name)
{
    terminalItem *currentItem = terminalListHead;
    // while next node exists
    while (currentItem != NULL)
    {
        // if name we searching is equal to current item name
        if (strcmp(name, currentItem->name) == 0)
        {
            // returning item that matches
            return currentItem;
        }
        // moving to the next node
        currentItem = currentItem->next;
    }
    return NULL;
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief         Prints help message for command that was passed as argument, if
 *                no argument was passed prints short description for all operations
 *
 * @param[in]     argc number of command arguments
 * @param[in]     argv command arguments
 * @return        Status of operation
 */
/*----------------------------------------------------------------------------*/
static terminalRet terminalShowHelp(uint8_t argc, char **argv)
{
    terminalItem *operation;
    // if argv contains command without arguments
    if (argc == 1)
    {
        // begin from the list head
        operation = terminalListHead;
        // printing short description for all operations
        while (operation != NULL)
        {
            printf("%s: %s \n", operation->name, operation->desc);
            operation = operation->next;
        }
    }
    else
    {
        // trying to find operation, given as argument
        operation = terminalFindItem(argv[1]);
        // if operation wasn't found
        if (operation == NULL)
        {
            // returning error message
            printf("Sorry, there is no such command\n");
            return SHELL_EARGC;
        }
        // printing help message for operation that was passed
        // as argument
        printf("%s \n", operation->help);
    }
    return SHELL_OK;
}
#ifdef BOOTLOADER
/*----------------------------------------------------------------------------*/
/*!
 * @brief         Prints arguments, given to an echo command
 *
 * @param[in]     argc number of command arguments
 * @param[in]     argv command arguments
 * @return        Status of operation
 */
/*----------------------------------------------------------------------------*/
static terminalRet terminalEcho(uint8_t argc, char **argv)
{
    // Skipping first argument, because it's an echo command
    for (uint8_t argIndex = 1; argIndex != argc; argIndex++)
    {
        // printing all arguments with line feed
        printf("%s\n", argv[argIndex]);
    }
    return SHELL_OK;
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief         Calculates the CRC of given memory region
 *
 * @param[in]     argc number of command arguments
 * @param[in]     argv command arguments
 * @return        Status of operation
 */
/*----------------------------------------------------------------------------*/
static terminalRet terminalCalcCRC(uint8_t argc, char **argv)
{
    if (argc < 4)
    {
        return SHELL_EARGC;
    }
    // Store the parameters for the data file
    if (strcmp("eeprom", argv[1]) == 0)
    {
        memData.memoType = MEMO_EEPROM;
    }
    else if (strcmp("flash", argv[1]) == 0)
    {
        memData.memoType = MEMO_FLASH;
    }
    else if (strcmp("ram", argv[1]) == 0)
    {
        memData.memoType = MEMO_RAM;
    }
    else
    {
        printf("There is no memory type like %s\n", argv[1]);
        return SHELL_EARG;
    }
    // Second argument is start address
    if (!terminalStrToInt(argv[2], &memData.address))
    {
        printf("Cannot convert %s to number\n", argv[2]);
        return SHELL_ECONV;
    }
    // Third argument is data size
    if (!terminalStrToInt(argv[3], &memData.size))
    {
        printf("Cannot convert %s to number\n", argv[3]);
        return SHELL_ECONV;
    }
    isUSBCommand = false;
    memoryCalcCRC(memData);
    return SHELL_OK;
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief         Prints memory mapping of the microcontroller
 *
 * @param[in]     argc number of command arguments
 * @param[in]     argv command arguments
 * @return        Status of operation
 */
/*----------------------------------------------------------------------------*/
static terminalRet terminalMemMap(uint8_t argc, char **argv)
{
    (void)argc;
    (void)argv;
    isUSBCommand = false;
    memoryMap();
    return SHELL_OK;
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief         Erases data from external flash
 *
 * @param[in]     argc number of command arguments
 * @param[in]     argv command arguments
 * @return        Status of operation
 */
/*----------------------------------------------------------------------------*/
static terminalRet terminalEraseIn(uint8_t argc, char **argv)
{
    (void)argc;
    isUSBCommand = false;
    if (!eraseMemory(argv))
    {
        return SHELL_ECBK;
    }
    return SHELL_OK;
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief         Reads data from one following memory storages(flash, eeprom, ram)
 *
 * @param[in]     argc number of command arguments
 * @param[in]     argv command arguments
 * @return        Status of operation
 */
/*----------------------------------------------------------------------------*/
static terminalRet terminalReadIn(uint8_t argc, char **argv)
{
    if (argc < 4)
    {
        return SHELL_EARGC;
    }
    // Store the parameters for the data file
    if (strcmp("eeprom", argv[1]) == 0)
    {
        memData.memoType = MEMO_EEPROM;
    }
    else if (strcmp("flash", argv[1]) == 0)
    {
        memData.memoType = MEMO_FLASH;
    }
    else if (strcmp("ram", argv[1]) == 0)
    {
        memData.memoType = MEMO_RAM;
    }
    else
    {
        printf("There is no memory type like %s\n", argv[1]);
        return SHELL_EARG;
    }
    // Second argument is start address
    if (!terminalStrToInt(argv[2], &memData.address))
    {
        printf("Cannot convert %s to number\n", argv[2]);
        return SHELL_ECONV;
    }
    // Third argument is data size
    if (!terminalStrToInt(argv[3], &memData.size))
    {
        printf("Cannot convert %s to number\n", argv[3]);
        return SHELL_ECONV;
    }
    isUSBCommand = false;
    memoryRead(memData);
    return SHELL_OK;
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief         Writes data into one following memory storages(flash, eeprom, ram)
 *
 * @param[in]     argc number of command arguments
 * @param[in]     argv command arguments
 * @return        Status of operation
 */
/*----------------------------------------------------------------------------*/
static terminalRet terminalWriteIn(uint8_t argc, char **argv)
{
    if (argc < 4)
    {
        return SHELL_EARGC;
    }
    // Store the parameters for the data file
    if (strcmp("eeprom", argv[1]) == 0)
    {
        memData.memoType = MEMO_EEPROM;
    }
    else if (strcmp("flash", argv[1]) == 0)
    {
        memData.memoType = MEMO_FLASH;
    }
    else if (strcmp("ram", argv[1]) == 0)
    {
        memData.memoType = MEMO_RAM;
    }
    else
    {
        printf("There is no memory type like %s\n", argv[1]);
        return SHELL_EARG;
    }
    // Second argument is start address
    if (!terminalStrToInt(argv[2], &memData.address))
    {
        printf("Cannot convert %s to number\n", argv[2]);
        return SHELL_ECONV;
    }
    // Third argument is data size
    if (!terminalStrToInt(argv[3], &memData.size))
    {
        printf("Cannot convert %s to number\n", argv[3]);
        return SHELL_ECONV;
    }
    if (memData.memoType == MEMO_RAM && !checkRamValidity(memData.address, memData.size))
    {
        printf("Can not be stored in this address range\n");
        return SHELL_EARG;
    }
    if (memData.size > RAM_AXI_SIZE)
    {
        printf("Sise is too big!!!Max possible received size is %d bytes\n", RAM_AXI_SIZE);
        return SHELL_EARG;
    }
    else
    {
        printf("File Receiving started!\nYOU HAVE 20S TO START UPLOADING!!\n");
    }
    receiveUartDataSize = memData.size;
    fileReceptionOn     = true;
    timingAddTimer(&resetWrite, TIMING_TIMER_ONE_SHOT, 20000, resetWriteUART);
    __HAL_USART_ENABLE_IT(&serialUart, USART_IT_RXNE);
    HAL_NVIC_EnableIRQ(USART3_IRQn);
    return SHELL_OK;
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief         Starts applicaton execution from one following memory storages(flash, ram)
 *
 * @param[in]     argc number of command arguments
 * @param[in]     argv command arguments
 * @return        Status of operation
 */
/*----------------------------------------------------------------------------*/
static terminalRet terminalStartApplication(uint8_t argc, char **argv)
{
    (void)argc;
    bool     memoType   = 0;
    uint32_t appAddress = 0;
    // Second argument is start address
    if (!terminalStrToInt(argv[2], &appAddress))
    {
        printf("Cannot convert %s to number\n", argv[2]);
        return SHELL_ECONV;
    }
    if (strcmp("ram", argv[1]) == 0)
    {
        memoType = false;
    }
    else if (strcmp("flash", argv[1]) == 0)
    {
        memoType = true;
    }
    else
    {
        printf("Can not start application from 0x%08lX\n", appAddress);
        return SHELL_EARG;
    }
    // Check the CRC of application stored and jump to it
    isUSBCommand = false;
    if (!checkAppValidity(memoType, appAddress))
    {
        return SHELL_ECBK;
    }
    return SHELL_OK;
}
#endif
/*----------------------------------------------------------------------------*/
/*!
 * @brief         Reads the line via UART bytewise
 *
 */
/*----------------------------------------------------------------------------*/
void terminalRun(void)
{
    static uint32_t charCounter = 0;
    static char     charBuffer[MAX_LINE_LENGTH];
#ifdef BOOTLOADER
    if (!fileReceptionOn)
    {
#endif
        // Check whether there is a char in the UART receive buffer
        int32_t receivedChar;
        if ((receivedChar = getchar()) == EOF)
        {
            return;
        }

        if (receivedChar == '\r')
        {
            // Command received
            // changing CR or LF symbol on EOL symbol to ensure correct work of string
            // functions
            charBuffer[charCounter] = '\0';
            // resetting counter
            charCounter = 0;
            // calling parsing function
            terminalParseLine(charBuffer);
        }
        else if (isprint(receivedChar))
        {
            // Add character to the local buffer
            charBuffer[charCounter++] = (char)receivedChar;
        }

        // Check for overlow
        if (charCounter >= MAX_LINE_LENGTH)
        {
            charCounter = MAX_LINE_LENGTH - 2;
        }
#ifdef BOOTLOADER
    }
#endif
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief         Parses the line
 * @param[in]     argv line to parse
 *
 */
/*----------------------------------------------------------------------------*/
static void terminalParseLine(char *argv)
{
    static char *argvTokens[MAX_TOKENS_NUMBER];
    static char *save;
    uint8_t      tokenIndex = 0;
    // operation must be a first token
    char *token = STRTOK_R(argv, " \t\n\r", &save);
    // cycle until there is no available tokens
    while (token != NULL)
    {
        // Copying arguments
        argvTokens[tokenIndex++] = token;
        token                    = STRTOK_R(NULL, " \t\n\r", &save);
    }
    terminalExecuteOperation(tokenIndex, argvTokens);
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief         Search an item in the linked list and then execute the callback
 *
 * @param[in]     argc number of command arguments
 * @param[in]     argv command arguments
 * @return        Status of operation
 */
/*----------------------------------------------------------------------------*/
static void terminalExecuteOperation(uint8_t argc, char **argv)
{
    // Pointer to current operation in progress item
    static const terminalItem *opInProgress = &terminalHelpItem;
    // searching operation
    const terminalItem *opToExecute = terminalFindItem(argv[0]);
    // if operation was not found
    if (opToExecute == NULL)
    {
        printf("Command not found: %s \n", argv[0]);
        return;
    }
    if (opToExecute->callback == NULL)
    {
        printf("Callback function does not exist! \n");
        return;
    }

    // Check whether previous operation is currently running
    if (terminalBusyFlag && (opInProgress != opToExecute))
    {
        printf("Other operation is currently in progress, stop this operation via appropriate command and continue\n");
        return;
    }

    terminalBusyFlag = false;
    // execute operation callback function
    terminalRet previousOperationStatus = opToExecute->callback(argc, argv);
    switch (previousOperationStatus)
    {
        case SHELL_EARG: printf("Wrong function arguments!\n"); break;

        case SHELL_ECONV: printf("Convertion error!\n"); break;

        case SHELL_EARGC: printf("Number of arguments is not sufficient!\n"); break;

        case SHELL_EHW: printf("Hardware error!\n"); break;

        case SHELL_ECBK: printf("Callback function error!\n"); break;

        case SHELL_OK: break;

        case SHELL_OIP:
            opInProgress     = opToExecute;
            terminalBusyFlag = true;
            printf("Operation started\n");
            break;

        default: printf("Received status is not recognized!\n"); break;
    }
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief  String to integer converter
 *        strtol wrapper
 * @param[in]  value     pointer to the value to convert
 * @param[out] res       pointer to the variable for the result
 * @return     true - successful conversion, false - otherwise
 */
/* ---------------------------------------------------------------------------- */
bool terminalStrToInt(const char *value, uint32_t *res)
{
    char *p_end = NULL;
    *res        = (uint32_t)strtol(value, &p_end, 0);
    if (*p_end != '\0')
    {
        return false;
    }
    return true;
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief  Finish current operation in progress
 *
 */
/* ---------------------------------------------------------------------------- */
void terminalFinishOperation(void)
{
    terminalBusyFlag = false;
}
