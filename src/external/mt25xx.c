/*!
 * Copyright © Siemens Healthcare GmbH 2022, All Rights Reserved
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
#include "mt25xx.h"

// Reset Operations
#define MT25XX_RESET_ENABLE_CMD 0x66
#define MT25XX_RESET_MEMORY_CMD 0x99

// Read Operations
#define MT25XX_FAST_READ_4_BYTE_DTR_CMD     0x0E
#define MT25XX_QUAD_INOUT_FAST_READ_DTR_CMD 0xED
#define MT25XX_QUAD_INOUT_FAST_READ_CMD     0xEB

// Write Operation
#define MT25XX_WRITE_ENABLE_CMD 0x06

/* Register Operations */
#define MT25XX_READ_VOL_CFG_REG_CMD  0x85
#define MT25XX_WRITE_VOL_CFG_REG_CMD 0x81
#define MT25XX_READ_STATUS_REG_CMD   0x05

// Program Operation
#define MT25XX_QUAD_IN_FAST_PROG_4_BYTE_ADDR_CMD 0x32

// Register Operations
#define MT25XX_READ_VOL_CFG_REG_CMD  0x85
#define MT25XX_WRITE_VOL_CFG_REG_CMD 0x81
#define MT25XX_READ_STATUS_REG_CMD   0x05

// Erase Operations
#define MT25XX_SUBSECTOR_ERASE_CMD_4K             0x20
#define MT25XX_SUBSECTOR_ERASE_4_BYTE_ADDR_CMD_4K 0x21
#define MT25XX_SUBSECTOR_ERASE_CMD_32K            0x52
#define MT25XX_SECTOR_ERASE_CMD                   0xD8
#define MT25XX_SECTOR_ERASE_4_BYTE_ADDR_CMD       0xDC
#define MT25XX_DIE_ERASE_CMD                      0xC7
#define MT25XX_PROG_ERASE_RESUME_CMD              0x7A
#define MT25XX_PROG_ERASE_SUSPEND_CMD             0x75

// 4-byte Address Mode Operation
#define MT25XX_ENTER_4_BYTE_ADDR_MODE_CMD 0xB7

// Quad Operation
#define MT25XX_ENTER_QUAD_CMD 0x35

/*! Dummy cycles for STR read mode */
#define MT25XX_DUMMY_CYCLES_READ_QUAD 8U
/* Dummy cycles for DTR read mode */
#define MT25XX_DUMMY_CYCLES_READ_QUAD_DTR 8U

/*----------------------------------------------------------------------------*/
/*!
 * @brief             This function read the SR of the memory and wait the EOP.
 *                    Polling WIP(Write In Progress) bit become to 0
 *
 * @param[in]         timeout
 * @return            HAL status
 */
/*----------------------------------------------------------------------------*/
uint32_t mt25AutoPollingMemReady(uint32_t timeout)
{
    // Configure automatic polling mode to wait for memory ready
    QSPI_CommandTypeDef command = { .InstructionMode   = QSPI_INSTRUCTION_4_LINES,
                                    .Instruction       = MT25XX_READ_STATUS_REG_CMD,
                                    .AddressMode       = QSPI_ADDRESS_NONE,
                                    .AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE,
                                    .DataMode          = QSPI_DATA_4_LINES,
                                    .DummyCycles       = 2,
                                    .DdrMode           = QSPI_DDR_MODE_DISABLE,
                                    .DdrHoldHalfCycle  = QSPI_DDR_HHC_ANALOG_DELAY,
                                    .SIOOMode          = QSPI_SIOO_INST_EVERY_CMD };

    QSPI_AutoPollingTypeDef config = {
        .Match           = 0,
        .MatchMode       = QSPI_MATCH_MODE_AND,
        .Interval        = 0x10,
        .AutomaticStop   = QSPI_AUTOMATIC_STOP_ENABLE,
        .Mask            = MT25XX_SR_WIP,
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
 * @brief             This function reset the QSPI memory.
 * @return            HAL status
 */
/*----------------------------------------------------------------------------*/
static uint32_t mt25ResetMemory(void)
{
    HAL_StatusTypeDef status;

    // Initialize the reset enable command
    QSPI_CommandTypeDef command = { .InstructionMode   = QSPI_INSTRUCTION_1_LINE,
                                    .Instruction       = MT25XX_RESET_ENABLE_CMD,
                                    .AddressMode       = QSPI_ADDRESS_NONE,
                                    .AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE,
                                    .DataMode          = QSPI_DATA_NONE,
                                    .DummyCycles       = 0,
                                    .DdrMode           = QSPI_DDR_MODE_DISABLE,
                                    .DdrHoldHalfCycle  = QSPI_DDR_HHC_ANALOG_DELAY,
                                    .SIOOMode          = QSPI_SIOO_INST_EVERY_CMD };

    // Send the command
    status = HAL_QSPI_Command(&hqspi, &command, HAL_QPSI_TIMEOUT_DEFAULT_VALUE);
    if (status != HAL_OK)
    {
        return status;
    }

    // Send the reset memory command
    command.Instruction = MT25XX_RESET_MEMORY_CMD;
    status              = HAL_QSPI_Command(&hqspi, &command, HAL_QPSI_TIMEOUT_DEFAULT_VALUE);
    if (status != HAL_OK)
    {
        return status;
    }

    // Enter QSPI memory in QPI mode
    command.Instruction = MT25XX_ENTER_QUAD_CMD;
    status              = HAL_QSPI_Command(&hqspi, &command, HAL_QPSI_TIMEOUT_DEFAULT_VALUE);
    if (status != HAL_OK)
    {
        return status;
    }

    // Configure automatic polling mode to wait the memory is ready
    status = (HAL_StatusTypeDef)mt25AutoPollingMemReady(HAL_QPSI_TIMEOUT_DEFAULT_VALUE);
    if (status != HAL_OK)
    {
        return status;
    }

    return HAL_OK;
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief             This function send a Write Enable and wait it is effective.
 * @return            HAL status
 */
/*----------------------------------------------------------------------------*/
static uint32_t mt25WriteEnable(void)
{
    HAL_StatusTypeDef status;
    // Enable write operations
    QSPI_CommandTypeDef command = {

        .InstructionMode   = QSPI_INSTRUCTION_4_LINES,
        .Instruction       = MT25XX_WRITE_ENABLE_CMD,
        .AddressMode       = QSPI_ADDRESS_NONE,
        .AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE,
        .DataMode          = QSPI_DATA_NONE,
        .DummyCycles       = 0,
        .DdrMode           = QSPI_DDR_MODE_DISABLE,
        .DdrHoldHalfCycle  = QSPI_DDR_HHC_ANALOG_DELAY,
        .SIOOMode          = QSPI_SIOO_INST_EVERY_CMD
    };

    // Send the command
    status = HAL_QSPI_Command(&hqspi, &command, HAL_QPSI_TIMEOUT_DEFAULT_VALUE);
    if (status != HAL_OK)
    {
        return status;
    }

    // Wait while the device is busy
    status = (HAL_StatusTypeDef)mt25AutoPollingMemReady(HAL_QPSI_TIMEOUT_DEFAULT_VALUE);
    if (status != HAL_OK)
    {
        return status;
    }

    return HAL_OK;
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief             This function set the QSPI memory in 4-byte address mode.
 * @return            HAL status
 */
/*----------------------------------------------------------------------------*/
static uint32_t mt25EnterFourBytesAddress(void)
{
    HAL_StatusTypeDef status;

    QSPI_CommandTypeDef command = {
        .InstructionMode   = QSPI_INSTRUCTION_4_LINES,
        .Instruction       = MT25XX_ENTER_4_BYTE_ADDR_MODE_CMD,
        .AddressMode       = QSPI_ADDRESS_NONE,
        .AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE,
        .DataMode          = QSPI_DATA_NONE,
        .DummyCycles       = 0,
        .DdrMode           = QSPI_DDR_MODE_DISABLE,
        .DdrHoldHalfCycle  = QSPI_DDR_HHC_ANALOG_DELAY,
        .SIOOMode          = QSPI_SIOO_INST_EVERY_CMD,
    };

    // Enable write operations
    status = (HAL_StatusTypeDef)mt25WriteEnable();
    if (status != HAL_OK)
    {
        return status;
    }

    // Send the command
    status = HAL_QSPI_Command(&hqspi, &command, HAL_QPSI_TIMEOUT_DEFAULT_VALUE);
    if (status != HAL_OK)
    {
        return status;
    }

    // Wait while the device is busy
    status = (HAL_StatusTypeDef)mt25AutoPollingMemReady(HAL_QPSI_TIMEOUT_DEFAULT_VALUE);
    if (status != HAL_OK)
    {
        return status;
    }

    return HAL_OK;
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief             This function reads the QSPI memory volatile configuration
 *                    register.
 * @return            HAL status
 */
/*----------------------------------------------------------------------------*/
uint32_t mt25ReadVolatileCfgReg(uint8_t *reg)
{
    HAL_StatusTypeDef status;

    // Initialize the read volatile configuration register command
    QSPI_CommandTypeDef command = {
        .InstructionMode   = QSPI_INSTRUCTION_4_LINES,
        .Instruction       = 0xAF,
        .AddressMode       = QSPI_ADDRESS_NONE,
        .AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE,
        .DataMode          = QSPI_DATA_4_LINES,
        .DummyCycles       = 0,
        .NbData            = 40,
        .DdrMode           = QSPI_DDR_MODE_DISABLE,
        .DdrHoldHalfCycle  = QSPI_DDR_HHC_ANALOG_DELAY,
        .SIOOMode          = QSPI_SIOO_INST_EVERY_CMD,
    };

    // Send the command
    status = HAL_QSPI_Command(&hqspi, &command, HAL_QPSI_TIMEOUT_DEFAULT_VALUE);
    if (status != HAL_OK)
    {
        return status;
    }

    // Reception of the data
    status = HAL_QSPI_Receive(&hqspi, reg, HAL_QPSI_TIMEOUT_DEFAULT_VALUE);
    if (status != HAL_OK)
    {
        return status;
    }

    return HAL_OK;
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief             This function configure the dummy cycles on memory side.
 * @return            HAL status
 */
/*----------------------------------------------------------------------------*/
static uint32_t mt25DummyCyclesCfg(void)
{
    HAL_StatusTypeDef status;
    uint16_t          reg = 0;

    // Initialize the read volatile configuration register command
    QSPI_CommandTypeDef command = {
        .InstructionMode   = QSPI_INSTRUCTION_4_LINES,
        .Instruction       = MT25XX_READ_VOL_CFG_REG_CMD,
        .AddressMode       = QSPI_ADDRESS_NONE,
        .AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE,
        .DataMode          = QSPI_DATA_4_LINES,
        .DummyCycles       = 0,
        .NbData            = 2,
        .DdrMode           = QSPI_DDR_MODE_DISABLE,
        .DdrHoldHalfCycle  = QSPI_DDR_HHC_ANALOG_DELAY,
        .SIOOMode          = QSPI_SIOO_INST_EVERY_CMD,
    };

    // Send the command
    status = HAL_QSPI_Command(&hqspi, &command, HAL_QPSI_TIMEOUT_DEFAULT_VALUE);
    if (status != HAL_OK)
    {
        return status;
    }

    // Reception of the data
    status = HAL_QSPI_Receive(&hqspi, (uint8_t *)(&reg), HAL_QPSI_TIMEOUT_DEFAULT_VALUE);
    if (status != HAL_OK)
    {
        return status;
    }

    // Enable write operations
    status = (HAL_StatusTypeDef)mt25WriteEnable();
    if (status != HAL_OK)
    {
        return status;
    }

    // Update volatile configuration register (with new dummy cycles)
    command.Instruction = MT25XX_WRITE_VOL_CFG_REG_CMD;
    MODIFY_REG(reg, 0xF0F0, ((MT25XX_DUMMY_CYCLES_READ_QUAD << 4) | (MT25XX_DUMMY_CYCLES_READ_QUAD << 12)));

    // Send the command
    status = HAL_QSPI_Command(&hqspi, &command, HAL_QPSI_TIMEOUT_DEFAULT_VALUE);
    if (status != HAL_OK)
    {
        return status;
    }

    // Transmission of the data
    status = HAL_QSPI_Transmit(&hqspi, (uint8_t *)(&reg), HAL_QPSI_TIMEOUT_DEFAULT_VALUE);
    if (status != HAL_OK)
    {
        return status;
    }

    return HAL_OK;
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief             Initializes and configure the QSPI interface.
 * @return            HAL status
 */
/*----------------------------------------------------------------------------*/
uint32_t mt25xxInit(void)
{
    HAL_StatusTypeDef status;
    // Memory reset
    status = (HAL_StatusTypeDef)mt25ResetMemory();
    if (status != HAL_OK)
    {
        return status;
    }

    // Set the QSPI memory in 4-bytes address mode
    //    status = (HAL_StatusTypeDef) mt25EnterFourBytesAddress();
    //    if (status != HAL_OK){
    //        return status;
    //    }

    // Configuration of the dummy cycles on QSPI memory side
    //    status = (HAL_StatusTypeDef) mt25DummyCyclesCfg();
    //    if (status != HAL_OK){
    //        return status;
    //    }

    return HAL_OK;
}

/*----------------------------------------------------------------------------*/
/*!
 *   @brief             This function reads an amount of data from QSPI memory in DTR(4-4-4).
 *   @param[in]         pData: pointer for buffer to read into
 *   @param[in]         readAddr: address to start reading from
 *   @param[in]         size: size of received data
 *   @return            HAL status
 */
/*----------------------------------------------------------------------------*/
uint32_t mt25Read(uint8_t *pData, uint32_t readAddr, uint32_t size)
{
    HAL_StatusTypeDef status;

    // Initialize the read command
    QSPI_CommandTypeDef command = { .InstructionMode   = QSPI_INSTRUCTION_4_LINES,
                                    .Instruction       = MT25XX_QUAD_INOUT_FAST_READ_DTR_CMD,
                                    .AddressMode       = QSPI_ADDRESS_4_LINES,
                                    .DataMode          = QSPI_DATA_4_LINES,
                                    .DummyCycles       = MT25XX_DUMMY_CYCLES_READ_QUAD_DTR,
                                    .AddressSize       = QSPI_ADDRESS_24_BITS,
                                    .Address           = readAddr,
                                    .AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE,
                                    .NbData            = size,
                                    .DdrMode           = QSPI_DDR_MODE_ENABLE,
                                    .DdrHoldHalfCycle  = QSPI_DDR_HHC_HALF_CLK_DELAY,
                                    .SIOOMode          = QSPI_SIOO_INST_EVERY_CMD };

    // Send the command
    status = HAL_QSPI_Command(&hqspi, &command, HAL_QPSI_TIMEOUT_DEFAULT_VALUE);
    if (status != HAL_OK)
    {
        return status;
    }

    // Receive data
    status = HAL_QSPI_Receive(&hqspi, pData, HAL_QPSI_TIMEOUT_DEFAULT_VALUE);
    if (status != HAL_OK)
    {
        return status;
    }

    return HAL_OK;
}

/*----------------------------------------------------------------------------*/
/*!
 *   @brief             This function erased an amount of data from QSPI memory in DTR(4-4-4).
 *   @param[in]         addr: address to start erasing from
 *   @param[in]         size: size of erased block memory
 *   @return            HAL status
 */
/*----------------------------------------------------------------------------*/
uint32_t mt25Erase(uint32_t addr, uint32_t size)
{
    HAL_StatusTypeDef status;

    // Initialize the erase command
    QSPI_CommandTypeDef command = {
        .InstructionMode   = QSPI_INSTRUCTION_4_LINES,
        .Instruction       = MT25XX_SUBSECTOR_ERASE_CMD_4K,
        .AddressMode       = QSPI_ADDRESS_4_LINES,
        .AddressSize       = QSPI_ADDRESS_24_BITS,
        .AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE,
        .DataMode          = QSPI_DATA_NONE,
        .DummyCycles       = 0,
        .DdrMode           = QSPI_DDR_MODE_DISABLE,
        .DdrHoldHalfCycle  = QSPI_DDR_HHC_ANALOG_DELAY,
        .SIOOMode          = QSPI_SIOO_INST_EVERY_CMD,
    };

    uint32_t currAddr = addr;
    while (currAddr < (addr + size))
    {
        command.Address = currAddr;
        // Enable write operations
        status = (HAL_StatusTypeDef)mt25WriteEnable();
        if (status != HAL_OK)
        {
            return status;
        }
        // Send the command
        status = HAL_QSPI_Command(&hqspi, &command, HAL_QPSI_TIMEOUT_DEFAULT_VALUE);
        if (status != HAL_OK)
        {
            return status;
        }
        // Wait while the device is busy
        status = (HAL_StatusTypeDef)mt25AutoPollingMemReady(HAL_QPSI_TIMEOUT_DEFAULT_VALUE);
        if (status != HAL_OK)
        {
            return status;
        }
        // Increment current address by 4k
        currAddr += MT25XX_ERASE_PAGE_SIZE;
    }

    return HAL_OK;
}

/*----------------------------------------------------------------------------*/
/*!
 *   @brief             This function writes an amount of data to QSPI
 *                      memory in DTR(4-4-4).
 *   @param[in]         pData: pointer for buffer with write data
 *   @param[in]         addr: address to start writing to
 *   @param[in]         size: size of received data
 *   @return            HAL status
 */
/*----------------------------------------------------------------------------*/
uint32_t mt25Write(uint8_t *pData, uint32_t addr, uint32_t size)
{
    HAL_StatusTypeDef status;

    QSPI_CommandTypeDef command = { .InstructionMode   = QSPI_INSTRUCTION_4_LINES,
                                    .Instruction       = MT25XX_QUAD_IN_FAST_PROG_4_BYTE_ADDR_CMD,
                                    .AddressMode       = QSPI_ADDRESS_4_LINES,
                                    .DataMode          = QSPI_DATA_4_LINES,
                                    .AddressSize       = QSPI_ADDRESS_24_BITS,
                                    .Address           = addr,
                                    .NbData            = size,
                                    .AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE,
                                    .DummyCycles       = 0,
                                    .DdrMode           = QSPI_DDR_MODE_DISABLE,
                                    .DdrHoldHalfCycle  = QSPI_DDR_HHC_ANALOG_DELAY,
                                    .SIOOMode          = QSPI_SIOO_INST_EVERY_CMD };

    // Enable write operations
    status = (HAL_StatusTypeDef)mt25WriteEnable();
    if (status != HAL_OK)
    {
        return status;
    }
    // Send the command
    status = HAL_QSPI_Command(&hqspi, &command, HAL_QPSI_TIMEOUT_DEFAULT_VALUE);
    if (status != HAL_OK)
    {
        return status;
    }

    // Transfer data
    status = HAL_QSPI_Transmit(&hqspi, pData, HAL_QPSI_TIMEOUT_DEFAULT_VALUE);
    if (status != HAL_OK)
    {
        return status;
    }

    // Wait while the device is busy
    status = (HAL_StatusTypeDef)mt25AutoPollingMemReady(HAL_QPSI_TIMEOUT_DEFAULT_VALUE);
    if (status != HAL_OK)
    {
        return status;
    }

    return HAL_OK;
}

/*----------------------------------------------------------------------------*/
/*!
 *   @brief             This function enables memory mapped mode.
 *
 *   @return            HAL status
 */
/*----------------------------------------------------------------------------*/
uint32_t mt25EnableMemoryMappedMode(void)
{
    HAL_StatusTypeDef status;
    // Configure the command for the read instruction
    QSPI_CommandTypeDef command = {

        .InstructionMode   = QSPI_INSTRUCTION_4_LINES,
        .Instruction       = MT25XX_QUAD_INOUT_FAST_READ_DTR_CMD, /* DTR QUAD INPUT/OUTPUT FAST READ and 4-BYTE DTR FAST READ commands */
        .AddressMode       = QSPI_ADDRESS_4_LINES,
        .AddressSize       = QSPI_ADDRESS_24_BITS,
        .AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE,
        .DataMode          = QSPI_DATA_4_LINES,
        .DummyCycles       = MT25XX_DUMMY_CYCLES_READ_QUAD_DTR,
        .DdrMode           = QSPI_DDR_MODE_ENABLE,
        .DdrHoldHalfCycle  = QSPI_DDR_HHC_HALF_CLK_DELAY,
        .SIOOMode          = QSPI_SIOO_INST_EVERY_CMD
    };

    /* Configure the memory mapped mode */
    QSPI_MemoryMappedTypeDef config = { .TimeOutActivation = QSPI_TIMEOUT_COUNTER_DISABLE, .TimeOutPeriod = 0 };

    status = HAL_QSPI_MemoryMapped(&hqspi, &command, &config);
    if (status != HAL_OK)
    {
        return status;
    }

    return HAL_OK;
}

/*----------------------------------------------------------------------------*/
/*!
 *   @brief             This function disables memory mapped mode.
 *   @return            HAL status
 */
/*----------------------------------------------------------------------------*/
uint32_t mt25DisableMemoryMappedMode(void)
{
    // Dummy read to be sure Memory Mapped mode is activated
    uint32_t tmp = *(uint32_t *)(QSPI_BASE);
    // Abort current operation
    uint32_t status = HAL_QSPI_Abort(&hqspi);
    if (status != HAL_OK)
    {
        return status;
    }
    return HAL_OK;
}
