/*!
 * Copyright Siemens Healthcare GmbH 2024, All Rights Reserved
 *
 * Project: Building Block Low End MCU
 *
 * @file
 * @brief Functions for updating application via UART or USB
 */
/*----------------------------------------------------------------------------*/
// Standard includes
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
// HAL includes
#include <hal.h>

// Project includes
#include "update.h"
#include "mem_map.h"
#include "w25xx_qspi.h"
#include "at24xx.h"
#include "selftest.h"
#include "crc_calc.h"
#include "sysinit.h"
#include "bsp_wtdg.h"
#include "terminal.h"
#include "usbd_cdc.h"
#include "usbd_cdc_interface.h"
#include "timing.h"
#include "prog_ctrl.h"


__attribute__((section(".ram_axi"))) uint8_t file_buffer[RAM_AXI_SIZE];
char                                         TxBuf[400] = { 0 };
bool                                         isUSBCommand;

static timingTimer echoTimer;
static void        readOperation(void);
memoStore          readData;

/*----------------------------------------------------------------------------*/
/*!
 * @brief   Performs the jump to start application address in RAM
 *
 */
/*----------------------------------------------------------------------------*/
void startApplication(uint32_t addr)
{
    // Run the application
    // Disable CPU L1 cache before jumping to the QSPI code execution
    SystemCache_Disable();
    // Disable Systick interrupt
    SysTick->CTRL = 0;
    // Start application
    void (*jumpToApplication)(void);
    jumpToApplication = (void (*)(void))(*(volatile uint32_t *)(addr + 4));
    __set_MSP(*(__IO uint32_t *)addr);
    jumpToApplication();
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief   Copies application from external flash to RAM memory before start
 *
 * @param[in]   startAddress - address from where to start in external flash
 *
 * @return  true - if success, false - otherwise
 */
/*----------------------------------------------------------------------------*/
bool copyApplication(uint32_t startAddress)
{
    // Get application size
    uint32_t imageLength;
    // Read image size from flash
    uint32_t status = w25Read(startAddress + IMAGE_SIZE_OFFSET, (uint8_t *)&imageLength, sizeof(imageLength));
    if (status != HAL_OK)
    {
        return false;
    }
    // Check Image length
    if (imageLength == 0 || imageLength > APP_MAX_SIZE)
    {
        return false;
    }
    uint32_t index = 0;
    uint8_t  buf[W25_WRITE_PAGE_SIZE];
    // Copy data to the destination
    while (index < imageLength)
    {
        WTDG_Feed();
        uint32_t amount = sizeof(buf) / sizeof(buf[0]);

        if (imageLength - index <= amount)
        {
            amount = imageLength - index;
        }
        if (w25Read(startAddress + index, (uint8_t *)RAM_AXI_ADDRESS + index, amount))
        {
            if (isUSBCommand)
            {
                snprintf(TxBuf, sizeof(TxBuf), "Read error %ld\n", index);
            }
            else
            {
                printf("Read error %ld\n", index);
            }
            return false;
        }
        index += amount;
    }
    return true;
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief          Checks if application is valid and if yes, switches to it
 *
 * @param[in]      memType  memory type(flash or ram)
 * @param[in]      address  starting address where the application is stored
 *
 * @return         true - write succeed, false - otherwise
 */
/*----------------------------------------------------------------------------*/
bool checkAppValidity(bool memType, uint32_t address)
{
    if (!memType)
    {
        uint32_t ramAddr = RAM_AXI_ADDRESS + address;
        // Find image size in RAM
        const uint32_t *imageLength = (uint32_t *)(ramAddr + IMAGE_SIZE_OFFSET);
        if (isUSBCommand)
        {
            snprintf(TxBuf, sizeof(TxBuf), "Size = %ld\n", *imageLength);
        }
        else
        {
            printf("Size = %ld\n", *imageLength);
        }
        // Check size in RAM
        if (*imageLength > RAM_AXI_SIZE || imageLength == 0)
        {
            return false;
        }
        // CRC stored
        const uint32_t *pStoredCRC = (uint32_t *)(ramAddr + *imageLength);
        // Rest CRC calculation
        crcReset();
        // Calculate CRC32
        uint32_t crc = crcCalc32((void *)ramAddr, *imageLength);
        // Compare the values
        if (crc != *pStoredCRC)
        {
            if (isUSBCommand)
            {
                snprintf(TxBuf, sizeof(TxBuf), "CRC is incorrect!\nStored = 0x%lX\nCalculaded = 0x%lX\n", *pStoredCRC, crc);
            }
            else
            {
                printf("CRC is incorrect!\nStored = 0x%lX\nCalculaded = 0x%lX\n", *pStoredCRC, crc);
            }
            return false;
        }
        else
        {
            if (isUSBCommand)
            {
                snprintf(TxBuf, sizeof(TxBuf), "App stored in RAM address 0x%lX is valid!\n", ramAddr);
                CDC_Itf_Transmit(TxBuf, strlen(TxBuf));
            }
            else
            {
                printf("App stored in RAM address 0x%lX is valid!\n", ramAddr);
            }
            startApplication(ramAddr);
        }
    }
    else
    {
        uint32_t flashAddr = APP_FLASH_ADDRESS + address;
        // Check app validity
        bool appStatus = selftestFlashApp(address);
        if (appStatus)
        {
            if (isUSBCommand)
            {
                snprintf(TxBuf, sizeof(TxBuf), "App stored in FLASH address 0x%lX is valid!\n", flashAddr);
            }
            else
            {
                printf("App stored in FLASH address 0x%lX is valid!\n", flashAddr);
            }
            // Set signature to start application
            if (address == 0)
            {
                progCtrlSetSignature(PROG_CTRL_START_APPLICATION);
            }
            else
            {
                progCtrlSetSignature(PROG_CTRL_START_DEFAULT_IMAGE);
            }
            // Application will be run after the reset
            needToReset = true;
        }
        else
        {
            if (isUSBCommand)
            {
                snprintf(TxBuf, sizeof(TxBuf), "No valid application in FLASH!\n");
            }
            else
            {
                printf("No valid application in FLASH!\n");
            }
            return false;
        }
    }
    return true;
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief          Outputs the 4 last bytes of received data
 *
 * @param[in]      data pointer to buffer with the stored data
 *
 */
/*----------------------------------------------------------------------------*/
static void process_received_data(uint8_t *data)
{
    if (!isUSBCommand)
    {
        printf("CRC stored = ");
        for (uint16_t i = 0; i < 4; ++i)
        {
            uint8_t ch = data[i];
            // Handle other data as needed
            printf(" 0x%02X", ch);
        }
        printf("\n");
    }
    else
    {
        snprintf(TxBuf, sizeof(TxBuf), "CRC stored 0x%02X 0x%02X 0x%02X 0x%02X\n", data[0], data[1], data[2], data[3]);
    }
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief          Writes memory map to one of the outputs( usb or uart interface)
 *
 */
/*----------------------------------------------------------------------------*/
void memoryMap(void)
{
    // Check if device is present
    w25Id id;
    (void)w25QReadID(&id);
    uint32_t flashSize = 0;
    uint8_t  devID     = id.devId & 0xFF;
    if (devID == 0x12)
    {
        flashSize = 64;
    }
    else if (devID == 0x13)
    {
        flashSize = 128;
    }
    else if (devID == 0x14)
    {
        flashSize = 256;
    }
    else if (devID == 0x15)
    {
        flashSize = 512;
    }
    else if (devID == 0x16)
    {
        flashSize = 1024;
    }
    else if (devID == 0x17)
    {
        flashSize = 2048;
    }
    if (isUSBCommand == false)
    {
        printf("EEPROM: 0x%08X 0x%07X bytes\n", 0, eeprom.capacity);
        printf("FLASH:  0x%08X 0x%07lX bytes\n", APP_FLASH_ADDRESS, 4096 * flashSize);
        printf("RAM:    0x%08X 0x%07X bytes\n", RAM_DTCM_ADDRESS, RAM_DTCM_SIZE);
        printf("RAM:    0x%08X 0x%07X bytes\n", RAM_AXI_ADDRESS, RAM_AXI_SIZE);
        printf("RAM:    0x%08X 0x%07X bytes\n", RAM_SRAM123_ADDRESS, RAM_SRAM123_SIZE);
        printf("RAM:    0x%08X 0x%07X bytes\n", RAM_SRAM4_ADDRESS, RAM_SRAM4_SIZE);
    }
    else
    {
        snprintf(
            TxBuf,
            sizeof(TxBuf),
            "EEPROM: 0x%08X 0x%07X bytes\nFLASH:  0x%08X 0x%07lX bytes\nRAM:    0x%08X 0x%07X bytes\nRAM:    0x%08X 0x%07X bytes\nRAM:    "
            "0x%08X 0x%07X bytes\nRAM:    0x%08X 0x%07X bytes\n",
            0,
            eeprom.capacity,
            APP_FLASH_ADDRESS,
            4096 * flashSize,
            RAM_DTCM_ADDRESS,
            RAM_DTCM_SIZE,
            RAM_AXI_ADDRESS,
            RAM_AXI_SIZE,
            RAM_SRAM123_ADDRESS,
            RAM_SRAM123_SIZE,
            RAM_SRAM4_ADDRESS,
            RAM_SRAM4_SIZE);
    }
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief         Stores data in external eeprom
 *
 * @param[in]     memData - structure, where memory parameters are stored
 *
 */
/*----------------------------------------------------------------------------*/
static bool writeEeprom(memoStore memData)
{
    uint16_t writtenSize = 0;
    WTDG_Feed();
    while (writtenSize < memData.size)
    {
        // Write data by one page at one time
        if (at24_Write(&eeprom, memData.address + writtenSize, &file_buffer[writtenSize], eeprom.page_size) != eeprom.page_size)
        {
            if (isUSBCommand)
            {
                snprintf(TxBuf, sizeof(TxBuf), "Write in external EEPROM error\nWritten %d bytes\n", writtenSize);
            }
            else
            {
                printf("Write in external EEPROM error\nWritten %d bytes\n", writtenSize);
            }
            return false;
        }
        writtenSize += eeprom.page_size;
    }
    if (isUSBCommand)
    {
        snprintf(TxBuf, sizeof(TxBuf), "%ld bytes stored in external EEPROM\n", memData.size);
    }
    else
    {
        printf("%ld bytes stored in external EEPROM\n", memData.size);
    }
    return true;
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief         Stores data in external flash memory
 *
 * @param[in]     memData - structure, where memory parameters are stored
 *
 */
/*----------------------------------------------------------------------------*/

static bool writeFlash(memoStore memData)
{
    if (w25Erase(memData.address, memData.size) != W25_OK)
    {
        return false;
    }
    uint32_t index = 0;
    while (index < memData.size)
    {
        WTDG_Feed();
        if (memData.size - index <= W25_WRITE_PAGE_SIZE)
        {
            // Write last page if it is smaller than 256 bytes
            if (w25Write(memData.address + index, &file_buffer[index], memData.size - index) != W25_OK)
            {
                return false;
            }
        }
        else
        {
            // Write page in external Flash
            if (w25Write(memData.address + index, &file_buffer[index], W25_WRITE_PAGE_SIZE) != W25_OK)
            {
                return false;
            }
        }
        index += W25_WRITE_PAGE_SIZE;
    }
    if (isUSBCommand)
    {
        snprintf(TxBuf, sizeof(TxBuf), "%ld bytes stored in external flash\n", memData.size);
    }
    else
    {
        printf("%ld bytes stored in external flash\n", memData.size);
    }
    return true;
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief         Stores data in one following memory storages(flash, eeprom, ram)
 *
 * @param[in]     memData - structure, where memory parameters are stored
 *
 */
/*----------------------------------------------------------------------------*/
bool memoryWrite(memoStore memoData)
{
    process_received_data(&file_buffer[memoData.size - 4]);
    if (memoData.memoType == MEMO_FLASH)
    {
        writeFlash(memoData);
    }
    else if (memoData.memoType == MEMO_EEPROM)
    {
        writeEeprom(memoData);
    }
    else if (memoData.memoType == MEMO_RAM)
    {
        if (memoData.address + memoData.size >= RAM_AXI_ADDRESS + RAM_AXI_SIZE)
        {
            memcpy(&memoData.address, &file_buffer[0], memoData.size);
        }
        if (isUSBCommand)
        {
            snprintf(TxBuf, sizeof(TxBuf), "%ld bytes stored in RAM at address 0x%lX\n", memoData.size, memoData.address);
        }
        else
        {
            printf("%ld bytes stored in RAM at address 0x%lX\n", memoData.size, memoData.address);
        }
    }

    return true;
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief         Erases data from external flash
 *
 * @param[in]     argv command arguments
 *
 */
/*----------------------------------------------------------------------------*/
bool eraseMemory(char **argv)
{
    // char TxBuf[200];
    if (strcmp("mass", argv[1]) == 0)
    {
        if (w25Erase(0, 8388607) != W25_OK)
        {
            return false;
        }
        if (!isUSBCommand)
        {
            printf("Mass erase finished!\n");
        }
        else
        {
            snprintf(TxBuf, sizeof(TxBuf), "Mass erase finished!\n");
        }
    }
    else
    {
        uint32_t memAddress;
        uint32_t memSize;
        // Second argument is start address
        if (!terminalStrToInt(argv[1], &memAddress))
        {
            if (isUSBCommand)
            {
                snprintf(TxBuf, sizeof(TxBuf), "Cannot convert %s to number\n", argv[1]);
            }
            else
            {
                printf("Cannot convert %s to number\n", argv[1]);
            }
            return false;
        }
        // Third argument is data size
        if (!terminalStrToInt(argv[2], &memSize))
        {
            if (!isUSBCommand)
            {
                printf("Cannot convert %s to number\n", argv[2]);
            }
            else
            {
                snprintf(TxBuf, sizeof(TxBuf), "Cannot convert %s to number\n", argv[2]);
            }
            return false;
        }
        if (w25Erase(memAddress, memSize) != W25_OK)
        {
            return false;
        }
        if (isUSBCommand)
        {
            snprintf(TxBuf, sizeof(TxBuf), "Erased from 0x%lX, to 0x%lX!\n", memAddress, memAddress + memSize);
        }
        else
        {
            printf("Erased from  0x%lX  to 0x%lX!\n", memAddress, memAddress + memSize);
        }
    }
    return true;
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief         Reads data from one following memory storages(flash, eeprom, ram)
 *
 * @param[in]     memData - structure, where memory parameters are stored
 *
 */
/*----------------------------------------------------------------------------*/
void memoryRead(memoStore memData)
{
    readData = memData;
    timingAddTimer(&echoTimer, TIMING_TIMER_CYCLIC, 5, readOperation);
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief         Timer handler of read operations
 *
 */
/*----------------------------------------------------------------------------*/
static void readOperation(void)
{
    WTDG_Feed();
    static uint8_t buff[256];
    uint32_t       amount = sizeof(buff) / sizeof(buff[0]);
    bool           ret    = true;
    // Parameter for the address according the type of memory from where the read was performed
    uint32_t trueAddr = 0;

    if (readData.size <= amount)
    {
        amount = readData.size;
    }
    // Read portion of memory needed
    if (readData.memoType == MEMO_FLASH)
    {
        trueAddr = APP_FLASH_ADDRESS + readData.address;
        if (w25Read(readData.address, buff, amount) != W25_OK)
        {
            ret = false;
        }
    }
    else if (readData.memoType == MEMO_EEPROM)
    {
        trueAddr = readData.address;
        if (at24_Read(&eeprom, readData.address, buff, amount) != amount)
        {
            ret = false;
        }
    }
    else if (readData.memoType == MEMO_RAM)
    {
        trueAddr = RAM_AXI_ADDRESS + readData.address;
        memcpy(buff, &file_buffer[readData.address], amount);
        ret = true;
    }

    if (!ret)
    {
        if (isUSBCommand)
        {
            snprintf(TxBuf, sizeof(TxBuf), "Read error at address %ld\n", trueAddr);
        }
        else
        {
            printf("Read error at address %ld\n", trueAddr);
        }
        terminalFinishOperation();
        timingRemoveTimer(&echoTimer);
        return;
    }
    readData.size -= amount;
    readData.address += amount;
    // Print received Data
    if (!isUSBCommand)
    {
        HAL_UART_Transmit(&serialUart, buff, (uint16_t)amount, 2 * amount);
    }
    else
    {
        memcpy(&TxBuf, buff, amount);
    }

    if (readData.size == 0)
    {
        terminalFinishOperation();
        timingRemoveTimer(&echoTimer);
    }
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief         Check if RAM memory needed for storing is compatible to store the data
 *
 * @param[in]     address   address to start storing data
 * @param[in]     size      size of stored data
 *
 */
/*----------------------------------------------------------------------------*/
bool checkRamValidity(uint32_t address, uint32_t size)
{
    uint32_t ramAxiMaxAddr     = RAM_AXI_ADDRESS + RAM_AXI_SIZE;
    uint32_t ramSram123MaxAddr = RAM_SRAM123_ADDRESS + RAM_SRAM123_SIZE;
    uint32_t ramSram4MaxAddr   = RAM_SRAM4_ADDRESS + RAM_SRAM4_SIZE;
    uint32_t ramDtcmMaxAddr    = RAM_DTCM_ADDRESS + RAM_DTCM_SIZE;
    if (address + size > ramAxiMaxAddr)
    {
        return false;
    }
    else if (address + size > ramSram123MaxAddr)
    {
        return false;
    }
    else if (address + size > ramSram4MaxAddr)
    {
        return false;
    }
    else if (address + size > ramDtcmMaxAddr)
    {
        if (isUSBCommand)
        {
            snprintf(TxBuf, sizeof(TxBuf), "DTCM memory is used for Stack and Heap!!!!\n");
        }
        else
        {
            printf("DTCM memory is used for Stack and Heap!!!!\n");
        }
        return false;
    }
    return true;
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief         Reads data from one following memory storages(flash, eeprom, ram)
 *
 * @param[in]     memData - structure, where memory parameters are stored
 *
 */
/*----------------------------------------------------------------------------*/
void memoryCalcCRC(memoStore memData)
{
    static uint8_t buff[16];
    uint32_t       amount = sizeof(buff) / sizeof(buff[0]);
    uint32_t       index  = 0;
    uint32_t       crc    = 0;
    bool           ret    = true;
    // Parameter for the address according the type of memory from where the read was performed
    uint32_t trueAddr = 0;
    // Rest CRC calculation
    crcReset();
    // Calculate CRC by portions of 16 bytes
    while (index < memData.size)
    {
        WTDG_Feed();
        if (memData.memoType == MEMO_EEPROM)
        {
            trueAddr = memData.address;
            if (at24_Read(&eeprom, memData.address + index, buff, amount) != amount)
            {
                ret = false;
            }
        }
        else if (memData.memoType == MEMO_RAM)
        {
            trueAddr = RAM_AXI_ADDRESS + memData.address;
            memcpy(buff, &file_buffer[memData.address + index], amount);
        }
        else if (memData.memoType == MEMO_FLASH)
        {
            trueAddr = APP_FLASH_ADDRESS + memData.address;
            if (w25Read(memData.address + index, buff, amount) != W25_OK)
            {
                ret = false;
            }
        }
        if (ret == false)
        {
            if (isUSBCommand)
            {
                snprintf(TxBuf, sizeof(TxBuf), "Error on memory reading at address 0x%08lX\n", trueAddr + index);
            }
            else
            {
                printf("Error on memory reading at address 0x%08lX\n", trueAddr + index);
            }
            return;
        }
        crc = crcCalc32(buff, amount);
        index += amount;
    }
    if (isUSBCommand)
    {
        snprintf(
            TxBuf, sizeof(TxBuf), "Calculated CRC of memory region (0x%08lX - 0x%08lX): 0x%08lX\n", trueAddr, trueAddr + memData.size, crc);
    }
    else
    {
        printf("Calculated CRC of memory region (0x%08lX - 0x%08lX): 0x%08lX\n", trueAddr, trueAddr + memData.size, crc);
    }
}