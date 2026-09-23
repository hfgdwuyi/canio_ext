/*!
 * Copyright Siemens Healthcare GmbH 2022, All Rights Reserved
 *
 * Project: Building Block Low End MCU
 *
 * @file
 * @brief Functions for work with MT25 QSPI flash
 */
/*----------------------------------------------------------------------------*/
// HAL includes
#include <hal.h>

// Project includes
#include "w25xx_qspi.h"
#include "bsp_wtdg.h"

#ifndef BOOTLOADER
#include <stdio.h>
#include <string.h>
#include "timing.h"
#include "terminal.h"
#endif

#define W25_MANUFACTURER_ID        0x9D
#define W25_Q64_DEVICE_ID          0x6017

#define W25_STATUS_BIT_BUSY        (1U)
#define W25_STATUS_BIT_QUAD_ENABLE (1U << 6)

/*----------------------------------------------------------------------------*/
/*!
  @name          Size definitions for erase/write
*/
/*----------------------------------------------------------------------------*/

#define W25_SECTOR_SIZE    4096  /*!< Erase sector size */
#define W25_BLOCK_32K_SIZE 32768 /*!< Erase block 32k size */
#define W25_BLOCK_64K_SIZE 65536 /*!< Erase block 64k size */
/*!
 @}
*/

/*----------------------------------------------------------------------------*/
/*!
  @name          Timeout definitions for erase/write operations
*/
/*----------------------------------------------------------------------------*/
#define W25_TIMEOUT_SECTOR_ERASE_MS   500
#define W25_TIMEOUT_BLOCK32K_ERASE_MS 2000
#define W25_TIMEOUT_BLOCK64K_ERASE_MS 2500
#define W25_TIMEOUT_CHIP_ERASE_MS     105000
#define W25_TIMEOUT_PAGE_WRITE_MS     5
/*!
 @}
*/

/*----------------------------------------------------------------------------*/
/*!
  @name          W25 commands
*/
/*----------------------------------------------------------------------------*/
#define W25Q_COMMAND_WRITE_STATUS          0x01

#define W25Q_COMMAND_READ_DATA             0x03
#define W25Q_COMMAND_WRITE_ENABLE          0x06
#define W25Q_COMMAND_WRITE_ENABLE_VOLATILE 0x50

#define W25Q_COMMAND_READ_STATUS_1         0x05
#define W25Q_COMMAND_READ_STATUS_2         0x35

#define W25Q_COMMAND_ENABLE_RESET          0x66
#define W25Q_COMMAND_RESET_DEVICE          0x99

#define W25Q_COMMAND_FAST_READ_QUAD_IO     0xEB
#define W25Q_COMMAND_FAST_READ_QUAD_OUTPUT 0x6B

#define W25Q_COMMAND_PAGE_PROGRAM_QUAD     0x32

#define W25Q_COMMAND_SECTOR_ERASE          0x20
#define W25Q_COMMAND_BLOCK_ERASE_32K       0x52
#define W25Q_COMMAND_BLOCK_ERASE_64K       0xD8
#define W25Q_COMMAND_CHIP_ERASE            0x60

#define W25Q_COMMAND_READ_DEVICE_ID        0x90
#define W25Q_COMMAND_READ_JEDEC            0x9F
/*!
 @}
*/

// Command defaults
static const QSPI_CommandTypeDef defCommand = { .AddressSize        = QSPI_ADDRESS_24_BITS,
                                                .AlternateByteMode  = QSPI_ALTERNATE_BYTES_NONE,
                                                .AlternateBytes     = QSPI_ALTERNATE_BYTES_NONE,
                                                .AlternateBytesSize = QSPI_ALTERNATE_BYTES_NONE,
                                                .DdrMode            = QSPI_DDR_MODE_DISABLE,
                                                .DdrHoldHalfCycle   = QSPI_DDR_HHC_ANALOG_DELAY,
                                                .SIOOMode           = QSPI_SIOO_INST_EVERY_CMD };

extern QSPI_HandleTypeDef hqspi;

/*----------------------------------------------------------------------------*/
/*!
 @brief         Enable write operations

 @return        Operation status
*/
/*----------------------------------------------------------------------------*/
static uint32_t w25WriteEnable(void)
{
    HAL_StatusTypeDef status;

    // Initialize the reset enable command
    QSPI_CommandTypeDef command = defCommand;

    command.InstructionMode = QSPI_INSTRUCTION_1_LINE;
    command.Instruction     = W25Q_COMMAND_WRITE_ENABLE;
    command.AddressMode     = QSPI_ADDRESS_NONE;
    command.DummyCycles     = 0;
    command.DataMode        = QSPI_DATA_NONE;

    // Enable reset
    status = HAL_QSPI_Command(&hqspi, &command, HAL_QPSI_TIMEOUT_DEFAULT_VALUE);
    if (status != HAL_OK)
    {
        return status;
    }

    return HAL_OK;
}

static uint32_t w25Wait4MemReady(uint32_t timeout)
{
    // Initialize the reset enable command
    QSPI_CommandTypeDef command = defCommand;

    command.InstructionMode = QSPI_INSTRUCTION_1_LINE;
    command.AddressMode     = QSPI_ADDRESS_NONE;
    command.DataMode        = QSPI_DATA_1_LINE;
    command.DummyCycles     = 0;
    command.NbData          = 1;
    command.Instruction     = W25Q_COMMAND_READ_STATUS_1;


    QSPI_AutoPollingTypeDef config = {
        .Match           = 0,
        .MatchMode       = QSPI_MATCH_MODE_AND,
        .Interval        = 0x10,
        .AutomaticStop   = QSPI_AUTOMATIC_STOP_ENABLE,
        .Mask            = W25_STATUS_BIT_BUSY,
        .StatusBytesSize = 1,
    };

    HAL_StatusTypeDef status = HAL_QSPI_AutoPolling(&hqspi, &command, &config, timeout);
    if (status != HAL_OK)
    {
        return status;
    }

    return HAL_OK;
}

/*----------------------------------------------------------------------------*/
/*!
 @brief         Reset the QSPI memory.
 @return        HAL status
*/
/*----------------------------------------------------------------------------*/
w25Status w25QReset(void)
{
    HAL_StatusTypeDef status;

    // Initialize the reset enable command
    QSPI_CommandTypeDef command = defCommand;

    command.InstructionMode = QSPI_INSTRUCTION_1_LINE;
    command.Instruction     = W25Q_COMMAND_ENABLE_RESET;
    command.AddressMode     = QSPI_ADDRESS_NONE;
    command.DummyCycles     = 0;
    command.DataMode        = QSPI_DATA_NONE;

    // Enable reset
    status = HAL_QSPI_Command(&hqspi, &command, HAL_QPSI_TIMEOUT_DEFAULT_VALUE);
    if (status != HAL_OK)
    {
        return W25_COMMAND_ERROR;
    }

    // Send reset command
    command.Instruction = W25Q_COMMAND_RESET_DEVICE;
    status              = HAL_QSPI_Command(&hqspi, &command, HAL_QPSI_TIMEOUT_DEFAULT_VALUE);
    if (status != HAL_OK)
    {
        return W25_COMMAND_ERROR;
    }

    return W25_OK;
}

/*----------------------------------------------------------------------------*/
/*!
 @brief         Read status registers

@param[out]    pointer to the return value
 @return        HAL status
*/
/*----------------------------------------------------------------------------*/
w25Status w25ReadStatusRegister(uint8_t *data)
{
    HAL_StatusTypeDef status;

    // Initialize the reset enable command
    QSPI_CommandTypeDef command = defCommand;

    command.InstructionMode = QSPI_INSTRUCTION_1_LINE;
    command.Instruction     = W25Q_COMMAND_READ_STATUS_1;
    command.AddressMode     = QSPI_ADDRESS_NONE;
    command.DataMode        = QSPI_DATA_1_LINE;
    command.DummyCycles     = 0;
    command.NbData          = 1;

    // Read status register 1
    uint8_t reg1;

    // Send command
    status = HAL_QSPI_Command(&hqspi, &command, HAL_QPSI_TIMEOUT_DEFAULT_VALUE);
    if (status != HAL_OK)
    {
        return W25_COMMAND_ERROR;
    }

    // Receive data
    status = HAL_QSPI_Receive(&hqspi, &reg1, HAL_QPSI_TIMEOUT_DEFAULT_VALUE);
    if (status != HAL_OK)
    {
        return W25_RECEIVE_ERROR;
    }

    // Return value
    *data = reg1;

    return W25_OK;
}

/*----------------------------------------------------------------------------*/
/*!
 @brief         Write status registers

 @param[in]     value
 @return        HAL status
*/
/*----------------------------------------------------------------------------*/
w25Status w25WriteStatusRegister(uint8_t value)
{
    HAL_StatusTypeDef status;

    QSPI_CommandTypeDef command = defCommand;

    command.InstructionMode = QSPI_INSTRUCTION_1_LINE;
    command.DataMode        = QSPI_DATA_1_LINE;
    command.Instruction     = W25Q_COMMAND_WRITE_STATUS;
    command.AddressMode     = QSPI_ADDRESS_NONE;
    command.DummyCycles     = 0;
    command.NbData          = 1;
    command.DataMode        = QSPI_DATA_1_LINE;

    // Enable write
    status = (HAL_StatusTypeDef)w25WriteEnable();
    if (status != HAL_OK)
    {
        return W25_COMMAND_ERROR;
    }

    // Send command
    status = HAL_QSPI_Command(&hqspi, &command, HAL_QPSI_TIMEOUT_DEFAULT_VALUE);
    if (status != HAL_OK)
    {
        return W25_COMMAND_ERROR;
    }

    // transmit data
    status = HAL_QSPI_Transmit(&hqspi, &value, HAL_QPSI_TIMEOUT_DEFAULT_VALUE);
    if (status != HAL_OK)
    {
        return W25_TRANSMIT_ERROR;
    }
    return W25_OK;
}

/*----------------------------------------------------------------------------*/
/*!
 @brief         Read data via 4 data lines

 @param[in]     address address to read from
 @param[out]    rxBuf   read buffer
 @param[in]     length  data length

 @return        Read status
*/
/*----------------------------------------------------------------------------*/
w25Status w25Read(uint32_t address, uint8_t *rxBuf, uint32_t length)
{
    HAL_StatusTypeDef status;

    // Initialize the reset enable command
    QSPI_CommandTypeDef command = defCommand;

    command.InstructionMode = QSPI_INSTRUCTION_1_LINE;
    command.Instruction     = W25Q_COMMAND_FAST_READ_QUAD_IO;
    command.AddressMode     = QSPI_ADDRESS_4_LINES;
    command.Address         = address;
    command.DummyCycles     = 6;
    command.DataMode        = QSPI_DATA_4_LINES;
    command.NbData          = length;

    // Send command
    status = HAL_QSPI_Command(&hqspi, &command, HAL_QPSI_TIMEOUT_DEFAULT_VALUE);
    if (status != HAL_OK)
    {
        return W25_COMMAND_ERROR;
    }

    // Receive data
    status = HAL_QSPI_Receive(&hqspi, rxBuf, HAL_QPSI_TIMEOUT_DEFAULT_VALUE);
    if (status != HAL_OK)
    {
        return W25_RECEIVE_ERROR;
    }

    return W25_OK;
}

/*----------------------------------------------------------------------------*/
/*!
 @brief         Erase data

 @param[in]     address address to erase
 @param[in]     length  data length

    Erase is performed by 4kBytes blocks at minimum.

 @return        Erase status
*/
/*----------------------------------------------------------------------------*/
w25Status w25Erase(uint32_t address, uint32_t length)
{
    uint32_t eraseCount = 0;
    // Erase data
    while (eraseCount < length)
    {
        HAL_StatusTypeDef status;
        uint32_t          eraseBlockSize;
        uint32_t          eraseTimeout;
        uint8_t           eraseCommand;

        // Choose erase block size
        if ((length - eraseCount) >= W25_BLOCK_64K_SIZE)
        {
            eraseBlockSize = W25_BLOCK_64K_SIZE;
            eraseTimeout   = W25_TIMEOUT_BLOCK64K_ERASE_MS;
            eraseCommand   = W25Q_COMMAND_BLOCK_ERASE_64K;
        }
        else if ((length - eraseCount) >= W25_BLOCK_32K_SIZE)
        {
            eraseBlockSize = W25_BLOCK_32K_SIZE;
            eraseTimeout   = W25_TIMEOUT_BLOCK32K_ERASE_MS;
            eraseCommand   = W25Q_COMMAND_BLOCK_ERASE_32K;
        }
        else
        {
            eraseBlockSize = W25_SECTOR_SIZE;
            eraseTimeout   = W25_TIMEOUT_SECTOR_ERASE_MS;
            eraseCommand   = W25Q_COMMAND_SECTOR_ERASE;
        }

        // Enable write
        status = (HAL_StatusTypeDef)w25WriteEnable();
        if (status != HAL_OK)
        {
            return W25_COMMAND_ERROR;
        }

        // Initialize the command
        QSPI_CommandTypeDef command = defCommand;

        command.InstructionMode = QSPI_INSTRUCTION_1_LINE;
        command.Instruction     = eraseCommand;
        command.AddressMode     = QSPI_ADDRESS_1_LINE;
        command.Address         = address;
        command.DummyCycles     = 0;
        command.DataMode        = QSPI_DATA_NONE;

        // Send command
        status = HAL_QSPI_Command(&hqspi, &command, HAL_QPSI_TIMEOUT_DEFAULT_VALUE);
        if (status != HAL_OK)
        {
            return W25_COMMAND_ERROR;
        }
        // Wait until operation is finished
        if (w25Wait4MemReady(eraseTimeout) != HAL_OK)
        {
            return W25_TIMEOUT;
        }
        WTDG_Feed();

        // Increment bytes counter
        eraseCount += eraseBlockSize;
        // Increment address
        address += eraseBlockSize;
    }

    return W25_OK;
}

/*----------------------------------------------------------------------------*/
/*!
 @brief         Erase full chip

 @return        Erase status
*/
/*----------------------------------------------------------------------------*/
w25Status w25EraseChip(void)
{
    HAL_StatusTypeDef status;

    // Enable write
    status = (HAL_StatusTypeDef)w25WriteEnable();
    if (status != HAL_OK)
    {
        return W25_COMMAND_ERROR;
    }

    // Initialize the reset enable command
    QSPI_CommandTypeDef command = defCommand;

    command.InstructionMode = QSPI_INSTRUCTION_1_LINE;
    command.Instruction     = W25Q_COMMAND_CHIP_ERASE;
    command.AddressMode     = QSPI_ADDRESS_NONE;
    command.DummyCycles     = 0;
    command.DataMode        = QSPI_DATA_NONE;

    // Send command
    status = HAL_QSPI_Command(&hqspi, &command, HAL_QPSI_TIMEOUT_DEFAULT_VALUE);
    if (status != HAL_OK)
    {
        return W25_COMMAND_ERROR;
    }

    if (w25Wait4MemReady(W25_TIMEOUT_CHIP_ERASE_MS) != HAL_OK)
    {
        return W25_TIMEOUT;
    }

    return W25_OK;
}

/*----------------------------------------------------------------------------*/
/*!
 @brief         Read data via 4 data lines

 @param[in]     address address to write to
 @param[in]     txBuf   write buffer
 @param[in]     length  data length

 @return        HAL status
*/
/*----------------------------------------------------------------------------*/
w25Status w25Write(uint32_t address, uint8_t *txBuf, uint32_t length)
{
    uint32_t txCount = 0;
    // Write data
    while (txCount < length)
    {
        HAL_StatusTypeDef status;
        // Enable write
        status = (HAL_StatusTypeDef)w25WriteEnable();
        if (status != HAL_OK)
        {
            return W25_COMMAND_ERROR;
        }
        uint32_t page_num      = (address / W25_WRITE_PAGE_SIZE);
        uint32_t page_addr_min = page_num * W25_WRITE_PAGE_SIZE;
        uint32_t page_addr_max = page_addr_min + W25_WRITE_PAGE_SIZE - 1;

        uint32_t len_diff   = length - txCount;
        uint32_t addr_start = address;
        uint32_t addr_stop  = (address + len_diff - 1) < page_addr_max ? address + len_diff - 1 : page_addr_max;
        uint32_t data_len   = (addr_stop - addr_start) + 1;

        // Initialize the command
        QSPI_CommandTypeDef command = defCommand;

        command.InstructionMode = QSPI_INSTRUCTION_1_LINE;
        command.Instruction     = W25Q_COMMAND_PAGE_PROGRAM_QUAD;
        command.AddressMode     = QSPI_ADDRESS_1_LINE;
        command.Address         = addr_start;
        command.DummyCycles     = 0;
        command.DataMode        = QSPI_DATA_4_LINES;
        command.NbData          = data_len;

        // Send command
        status = HAL_QSPI_Command(&hqspi, &command, HAL_QPSI_TIMEOUT_DEFAULT_VALUE);
        if (status != HAL_OK)
        {
            return W25_COMMAND_ERROR;
        }

        // Receive data
        status = HAL_QSPI_Transmit(&hqspi, &txBuf[txCount], HAL_QPSI_TIMEOUT_DEFAULT_VALUE);
        if (status != HAL_OK)
        {
            return W25_TRANSMIT_ERROR;
        }

        // Wait until operation is finished
        if (w25Wait4MemReady(W25_TIMEOUT_PAGE_WRITE_MS) != HAL_OK)
        {
            return W25_TIMEOUT;
        }

        // Increment bytes counter
        txCount += data_len;
        // Increment address
        address += data_len;
    }

    return W25_OK;
}

/*----------------------------------------------------------------------------*/
/*!
 @brief         Enable memory mapped mode

 @return        Readstatus
*/
/*----------------------------------------------------------------------------*/
w25Status w25EnableMemoryMappedMode(void)
{
    HAL_StatusTypeDef status;
    // Initialize the command
    QSPI_CommandTypeDef command = defCommand;

    command.InstructionMode = QSPI_INSTRUCTION_1_LINE;
    command.Instruction     = W25Q_COMMAND_FAST_READ_QUAD_IO;
    command.AddressMode     = QSPI_ADDRESS_4_LINES;
    command.DummyCycles     = 6;
    command.DataMode        = QSPI_DATA_4_LINES;

    /* Configure the memory mapped mode */
    QSPI_MemoryMappedTypeDef config = { .TimeOutActivation = QSPI_TIMEOUT_COUNTER_DISABLE, .TimeOutPeriod = 0 };

    status = HAL_QSPI_MemoryMapped(&hqspi, &command, &config);
    if (status != HAL_OK)
    {
        return W25_MEMORY_MAP;
    }

    return W25_OK;
}

/*----------------------------------------------------------------------------*/
/*!
 @brief         Read JEDEC ID

 @return        Readstatus
*/
/*----------------------------------------------------------------------------*/
w25Status w25QReadID(w25Id *id)
{
    HAL_StatusTypeDef status;

    // Initialize the reset enable command
    QSPI_CommandTypeDef command = defCommand;

    command.InstructionMode = QSPI_INSTRUCTION_1_LINE;
    command.Instruction     = W25Q_COMMAND_READ_JEDEC;
    command.AddressMode     = QSPI_ADDRESS_NONE;
    command.Address         = 0;
    command.DummyCycles     = 0;
    command.DataMode        = QSPI_DATA_1_LINE;
    command.NbData          = 3;

    // Enable reset
    status = HAL_QSPI_Command(&hqspi, &command, HAL_QPSI_TIMEOUT_DEFAULT_VALUE);
    if (status != HAL_OK)
    {
        return W25_COMMAND_ERROR;
    }

    // Receive data
    uint8_t pData[3];
    status = HAL_QSPI_Receive(&hqspi, pData, HAL_QPSI_TIMEOUT_DEFAULT_VALUE);
    if (status != HAL_OK)
    {
        return W25_RECEIVE_ERROR;
    }

    // Return data
    id->manId = pData[0];
    id->devId = (uint16_t)((uint16_t)((uint16_t)pData[1] << 8) | pData[2]);

    return W25_OK;
}

/*----------------------------------------------------------------------------*/
/*!
 @brief         Initialize QSPI

 @return        Initialization status
*/
/*----------------------------------------------------------------------------*/
w25Status w25InitQuad(void)
{
    w25Status status;
    // Check if device is present
    w25Id id;
    status = w25QReadID(&id);
    if (status != W25_OK)
    {
        return status;
    }

    if ((id.manId != W25_MANUFACTURER_ID) || (id.devId != W25_Q64_DEVICE_ID))
    {
        return W25_NO_DEVICE;
    }

    // Reset device
    status = w25QReset();
    if (status != W25_OK)
    {
        return status;
    }

    // Read status
    uint8_t statusReg;
    status = w25ReadStatusRegister(&statusReg);
    if (status != W25_OK)
    {
        return status;
    }
    // Enable Quad SPI
    statusReg |= W25_STATUS_BIT_QUAD_ENABLE;
    status = w25WriteStatusRegister(statusReg);
    if (status != W25_OK)
    {
        return status;
    }


    return W25_OK;
}
#ifndef BOOTLOADER

static terminalRet terminalFlash(uint8_t argc, char **argv);

static struct
{
    uint32_t address;
    uint32_t size;
} testData;

static timingTimer flashTimer;

/*----------------------------------------------------------------------------*/
/*!
 @brief         Initialies flash terminal item

 @param         none

 @return        none
*/
/*----------------------------------------------------------------------------*/

__attribute__((constructor)) void flashItemInit(void)
{
    static terminalItem terminalFlashItem = {
        .name = "qspi",
        .desc = "Writes/Reads data to/from external flash (w25xx)",
        .help = "Writes/Reads data of given size into/from flash\n\n  eeprom "
                "[arg1] - read or write modifier(-r/-w), [arg4]...[arg n] - arguments to write(ignored in case of reading)\n"
                "arg2 arg3...arg n \n\n\t arg2 - starting address ,arg3 - size",

        .callback = terminalFlash,
        .next     = NULL
    };
    terminalAddItem(&terminalFlashItem);
}

/*----------------------------------------------------------------------------*/
/*!
 @brief         Timer handler of flash operations

 @param         none

 @return        none
*/
/*----------------------------------------------------------------------------*/
static void flashOperation(void)
{
    static uint8_t buff[16];
    uint32_t       amount = sizeof(buff) / sizeof(buff[0]);

    if (testData.size <= amount)
    {
        amount = testData.size;
    }

    // Read portion
    if (w25Read(testData.address, buff, amount) != W25_OK)
    {
        printf("Read error at address %ld\n", testData.address);
        terminalFinishOperation();
        timingRemoveTimer(&flashTimer);
        return;
    }
    printf("0x%08lX: ", testData.address);
    testData.size -= amount;
    testData.address += amount;

    for (uint32_t i = 0; i < amount; i++)
    {
        printf("0x%02X ", buff[i]);
    }
    printf("\n");

    if (testData.size == 0)
    {
        terminalFinishOperation();
        timingRemoveTimer(&flashTimer);
    }
}

/* ----------------------------------------------------------------------------
 */
/*!
 @brief         Reads/Writes flash data

 @param[in]     argc number of command arguments
 @param[in]     argv command arguments

 @return        Status of operation
*/
/* ----------------------------------------------------------------------------
 */
static terminalRet terminalFlash(uint8_t argc, char **argv)
{
    // If arguments number is not sufficient for this operation
    if (argc < 3)
    {
        return SHELL_EARGC;
    }
    // Second argument is start address
    if (!terminalStrToInt(argv[2], &testData.address))
    {
        printf("Cannot convert %s to number\n", argv[2]);
        return SHELL_ECONV;
    }

    uint8_t  buff[16];
    uint32_t amount = sizeof(buff) / sizeof(buff[0]);
    if (strcmp(argv[1], "read") == 0)
    {
        if (!terminalStrToInt(argv[3], &testData.size))
        {
            printf("Cannot convert %s to number\n", argv[3]);
            return SHELL_ECONV;
        }

        timingAddTimer(&flashTimer, TIMING_TIMER_CYCLIC, 10, flashOperation);
        return SHELL_OIP;
    }
    else if (strcmp(argv[1], "write") == 0)
    {
        testData.size         = argc - 3;
        uint32_t bytesWritten = 0;
        while (testData.size > 0)
        {
            if (testData.size <= amount)
            {
                amount = testData.size;
            }
            // Get data to write
            for (uint32_t i = 0; i < amount; i++)
            {
                uint32_t tmp;
                if (!terminalStrToInt(argv[(i + 3) + bytesWritten], &tmp))
                {
                    printf("Cannot convert %s to number\n", argv[(i + 3) + bytesWritten]);
                    return SHELL_ECONV;
                }
                buff[i] = (uint8_t)tmp;
            }

            // Write data
            if (w25Write(testData.address, buff, amount) != W25_OK)
            {
                return SHELL_EHW;
            }
            testData.size -= amount;
            testData.address += amount;
            bytesWritten += amount;
        }

        printf("Written successfully\n");
    }
    else if (strcmp(argv[1], "erase") == 0)
    {
        if (!terminalStrToInt(argv[3], &testData.size))
        {
            printf("Cannot convert %s to number\n", argv[3]);
            return SHELL_ECONV;
        }
        // Erase data
        if (w25Erase(testData.address, testData.size) != W25_OK)
        {
            return SHELL_EHW;
        }
        printf("Erased successfully\n");
    }
    else
    {
        return SHELL_EARG;
    }
    return SHELL_OK;
}
#endif