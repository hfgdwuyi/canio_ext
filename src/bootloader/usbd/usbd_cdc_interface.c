/*!
 * Copyright Siemens Healthcare GmbH 2024, All Rights Reserved
 *
 * Project: Building Block Low End MCU
 *
 * @file
 * @brief Building block USB CDC device core functions
 *
 */
/*----------------------------------------------------------------------------*/

// Standard includes
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
// HAL includes
#include <hal.h>
// Project includes
#include "usbd_cdc.h"
#include "version.h"
#include "mem_map.h"
#include "terminal.h"
#include "update.h"
#include "timing.h"
#include "usbd_cdc_interface.h"

USBD_CDC_LineCodingTypeDef LineCoding = {
    115200, /* baud rate */
    0x00,   /* stop bits-1 */
    0x00,   /* parity - none */
    0x08    /* nb. of bits 8 */
};
uint8_t UserRxBuffer[APP_DATA_SIZE]; /* Received Data over USB are stored in
                                      * this buffer */
uint8_t UserTxBuffer[APP_DATA_SIZE]; /* Received Data over UART (CDC
                                      * interface) are stored in this buffer */

// Compiler adaptation
#if __ARMCC_VERSION >= 6000000
#define STRTOK_R _strtok_r
#else
#define STRTOK_R strtok_r
#endif
/* Maximum number of received words from terminal*/
#define MAX_TOKEN_NUMBER 32
/* USB handler declaration */
extern USBD_HandleTypeDef USBD_Device;
memoStore                 memoryData;
/* Private function prototypes ----------------------------------------------- */
static int8_t      CDC_Itf_Init(void);
static int8_t      CDC_Itf_DeInit(void);
static int8_t      CDC_Itf_Control(uint8_t cmd, uint8_t *pbuf, uint16_t length);
static int8_t      CDC_Itf_Receive(uint8_t *pbuf, uint32_t *Len);
static bool        fileUSBReceptionOn = false;
static bool        transmitFinish;
static uint32_t    currentFileSize = 0;
uint8_t            TxBuffer[APP_DATA_SIZE];
static timingTimer resetWr;

USBD_CDC_ItfTypeDef USBD_CDC_fops = { CDC_Itf_Init, CDC_Itf_DeInit, CDC_Itf_Control, CDC_Itf_Receive };

void resetWriteUSB(void)
{
    currentFileSize    = 0;
    fileUSBReceptionOn = false;
    snprintf(TxBuf, sizeof(TxBuf), "Reception interrupted!! Timeout expired!\n");
}

/* ----------------------------------------------------------------------------*/
/*!
 * @brief         Initializes the CDC media low layer
 *
 * @return        Result of the operation: USBD_OK if all operations are OK else USBD_FAIL
 *
 */
/* ----------------------------------------------------------------------------*/
static int8_t CDC_Itf_Init(void)
{
    // Set Application Buffers
    USBD_CDC_SetTxBuffer(&USBD_Device, UserTxBuffer, 0);
    USBD_CDC_SetRxBuffer(&USBD_Device, UserRxBuffer);
    return USBD_OK;
}

/* ----------------------------------------------------------------------------*/
/*!
 * @brief         DeInitializes the CDC media low layer
 *
 * @return        Result of the operation: USBD_OK if all operations are OK else USBD_FAIL
 *
 */
/* ----------------------------------------------------------------------------*/
static int8_t CDC_Itf_DeInit(void)
{
    return USBD_OK;
}

/* ----------------------------------------------------------------------------*/
/*!
 * @brief         Manage the CDC class requests
 *
 * @param[in]     Cmd Command code
 * @param[in]     Buf Buffer containing command data (request parameters)
 * @param[in]     Len Number of data to be sent (in bytes)
 *
 * @return        Result of the operation: USBD_OK if all operations are OK else USBD_FAIL
 *
 */
/* ----------------------------------------------------------------------------*/
static int8_t CDC_Itf_Control(uint8_t cmd, uint8_t *pbuf, uint16_t length)
{
    (void)length;
    switch (cmd)
    {
        case CDC_SEND_ENCAPSULATED_COMMAND:
            /* Add your code here */
            break;

        case CDC_GET_ENCAPSULATED_RESPONSE:
            /* Add your code here */
            break;

        case CDC_SET_COMM_FEATURE:
            /* Add your code here */
            break;

        case CDC_GET_COMM_FEATURE:
            /* Add your code here */
            break;

        case CDC_CLEAR_COMM_FEATURE:
            /* Add your code here */
            break;

        case CDC_SET_LINE_CODING:
            LineCoding.bitrate    = (pbuf[0] | (((uint32_t)pbuf[1]) << 8) | (((uint32_t)pbuf[2]) << 16) | (((uint32_t)pbuf[3]) << 24));
            LineCoding.format     = pbuf[4];
            LineCoding.paritytype = pbuf[5];
            LineCoding.datatype   = pbuf[6];

            break;

        case CDC_GET_LINE_CODING:
            pbuf[0] = (uint8_t)(LineCoding.bitrate);
            pbuf[1] = (uint8_t)(LineCoding.bitrate >> 8);
            pbuf[2] = (uint8_t)(LineCoding.bitrate >> 16);
            pbuf[3] = (uint8_t)(LineCoding.bitrate >> 24);
            pbuf[4] = LineCoding.format;
            pbuf[5] = LineCoding.paritytype;
            pbuf[6] = LineCoding.datatype;
            break;

        case CDC_SET_CONTROL_LINE_STATE:
            /* Add your code here */
            break;

        case CDC_SEND_BREAK:
            /* Add your code here */
            break;

        default: break;
    }

    return USBD_OK;
}

/* ----------------------------------------------------------------------------*/
/*!
 * @brief         Transmit packet on IN endpoint.
 *
 * @param[in]     Buf Buffer of data to be transmitted
 * @param[in]     Len Number of data received (in bytes)
 *
 * @return        Result of the operation: USBD_OK if all operations are OK else USBD_FAIL
 *
 */
/* ----------------------------------------------------------------------------*/
int8_t CDC_Itf_Transmit(char *Buf, uint32_t Len)
{
    USBD_CDC_HandleTypeDef *hcdc = (USBD_CDC_HandleTypeDef *)USBD_Device.pClassData;
    // set values in buffer
    transmitFinish = false;
    memset(UserTxBuffer, 0x00, APP_DATA_SIZE);
    memcpy(UserTxBuffer, Buf, Len);
    hcdc->TxBuffer = UserTxBuffer;
    hcdc->TxLength = Len;
    if (USBD_CDC_TransmitPacket(&USBD_Device) != USBD_OK)
    {
        return USBD_FAIL;
    }
    return USBD_OK;
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief         Parses the line
 * @param[in]     argv line to parse
 *
 */
/*----------------------------------------------------------------------------*/
static void usbParseLine(char *argv)
{
    static char *argvTokens[MAX_TOKEN_NUMBER];
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
    if ((strcmp("writemem", argvTokens[0]) == 0) || (strcmp("readmem", argvTokens[0]) == 0) || (strcmp("crc", argvTokens[0]) == 0))
    {
        // Store the parameters for the data file
        if (strcmp("eeprom", argvTokens[1]) == 0)
        {
            memoryData.memoType = MEMO_EEPROM;
        }
        else if (strcmp("flash", argvTokens[1]) == 0)
        {
            memoryData.memoType = MEMO_FLASH;
        }
        else if (strcmp("ram", argvTokens[1]) == 0)
        {
            memoryData.memoType = MEMO_RAM;
        }
        else
        {
            snprintf(TxBuf, sizeof(TxBuf), "There is no memory type like %s\n", argvTokens[1]);
            return;
        }
        // Second argument is start address
        if (!terminalStrToInt(argvTokens[2], &memoryData.address))
        {
            snprintf(TxBuf, sizeof(TxBuf), "Cannot convert %s to number\n", argvTokens[2]);
            return;
        }
        // Third argument is data size
        if (!terminalStrToInt(argvTokens[3], &memoryData.size))
        {
            snprintf(TxBuf, sizeof(TxBuf), "Cannot convert %s to number\n", argvTokens[3]);
            return;
        }
    }
    if (strcmp("writemem", argvTokens[0]) == 0)
    {
        if (memoryData.memoType == MEMO_RAM && !checkRamValidity(memoryData.address, memoryData.size))
        {
            snprintf(TxBuf, sizeof(TxBuf), "Can not be stored in this address range\n");
            return;
        }
        if (memoryData.size > RAM_AXI_SIZE)
        {
            snprintf(TxBuf, sizeof(TxBuf), "Sise is too big!!!Max possible received size is %d bytes\n", RAM_AXI_SIZE);
            return;
        }
        else
        {
            fileUSBReceptionOn = true;
            timingAddTimer(&resetWr, TIMING_TIMER_ONE_SHOT, 20000, resetWriteUSB);
            snprintf(TxBuf, sizeof(TxBuf), "File Receiving started!\nYOU HAVE 20S TO START UPLOADING!!\n");
        }
    }
    else if (strcmp("readmem", argvTokens[0]) == 0)
    {
        isUSBCommand = true;
        memoryRead(memoryData);
    }
    else if (strcmp("erasemem", argvTokens[0]) == 0)
    {
        isUSBCommand = true;
        eraseMemory(argvTokens);
    }
    else if (strcmp("runapp", argvTokens[0]) == 0)
    {
        bool     memoType   = 0;
        uint32_t appAddress = 0;
        // Second argument is start address
        if (!terminalStrToInt(argvTokens[2], &appAddress))
        {
            snprintf(TxBuf, sizeof(TxBuf), "Cannot convert %s to number\n", argvTokens[2]);
            return;
        }
        if (strcmp("ram", argvTokens[1]) == 0)
        {
            memoType = false;
        }
        else if (strcmp("flash", argvTokens[1]) == 0)
        {
            memoType = true;
        }
        else
        {
            snprintf(TxBuf, sizeof(TxBuf), "Can not start application from 0x%08lX\n", appAddress);
            return;
        }
        // Check the CRC of application stored and jump to it
        isUSBCommand = true;
        checkAppValidity(memoType, appAddress);
    }
    else if (strcmp("memmap", argvTokens[0]) == 0)
    {
        isUSBCommand = true;
        memoryMap();
    }
    else if (strcmp("crc", argvTokens[0]) == 0)
    {
        isUSBCommand = true;
        memoryCalcCRC(memoryData);
    }
    else
    {
        snprintf(TxBuf, sizeof(TxBuf), "Unknown Command %s!\n", argvTokens[0]);
    }
}

/* ----------------------------------------------------------------------------*/
/*!
 * @brief         Data received over USB OUT endpoint are sent over CDC interface
 *                through this function.
 *
 * @param[in]     Buf Buffer of data to be transmitted
 * @param[in]     Len Number of data received (in bytes)
 *
 * @return        Result of the operation: USBD_OK if all operations are OK else USBD_FAIL
 *
 */
/* ----------------------------------------------------------------------------*/
static int8_t CDC_Itf_Receive(uint8_t *Buf, uint32_t *Len)
{
    /* cdcVcpData.state = Buf[0];
    static bool writeStatus; */

    if (!fileUSBReceptionOn)
    {
        usbParseLine((char *)Buf);
    }
    else
    {
        resetWr.counter = 5;
        memcpy(&file_buffer[currentFileSize], Buf, *Len);
        currentFileSize += *Len;
        if (currentFileSize >= memoryData.size)
        {
            currentFileSize    = 0;
            fileUSBReceptionOn = false;
            isUSBCommand       = true;
            memoryWrite(memoryData);
            timingRemoveTimer(&resetWr);
        }
    }
    USBD_CDC_ReceivePacket(&USBD_Device);
    return USBD_OK;
}