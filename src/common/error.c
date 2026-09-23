/*!
 * Copyright Siemens Healthcare GmbH 2022, All Rights Reserved
 *
 * Project: Building Block Low End MCU
 *
 * @file
 * @brief Error handling
 */
/*----------------------------------------------------------------------------*/
// CANopen includes
#include <environ.h>
#define DEF_HW_PART
#include <cal_conf.h>
#include <co_pdo.h>
#include <co_drv.h>
#include <co_sdo.h>
#include <co_stor.h>
#include <co_acces.h>
#include <co_emcy.h>
#include <objects.h>
// Standart includes
#include <stdint.h>
#include <stdbool.h>
// Project includes
#include "error_defines.h"
#include "error.h"
#include "bsp_led.h"

/*! @struct Structure of emcy error codes */
struct emcyList
{
    uint8_t  errManuStatusBit;
    uint8_t  errDescRegByte;
    uint32_t emcyIdBit;
    uint16_t emcyCode;
};

static struct emcyList emcyListTable[] = {
    { ERR_MAN_HW_INIT, ERR_DESC_SUBINDEX_HW_INIT, ERR_INIT_UART, ERRCODE_UART_INIT },
    { ERR_MAN_HW_INIT, ERR_DESC_SUBINDEX_HW_INIT, ERR_INIT_I2C, ERRCODE_I2C_INIT },
    { ERR_MAN_HW_INIT, ERR_DESC_SUBINDEX_HW_INIT, ERR_INIT_SPI, ERRCODE_SPI_INIT },
    { ERR_MAN_HW_INIT, ERR_DESC_SUBINDEX_HW_INIT, ERR_INIT_ADC, ERRCODE_ADC_INIT },
    { ERR_MAN_HW_INIT, ERR_DESC_SUBINDEX_HW_INIT, ERR_INIT_DAC, ERRCODE_DAC_INIT },
    { ERR_MAN_HW_INIT, ERR_DESC_SUBINDEX_HW_INIT, ERR_INIT_PWM, ERRCODE_PWM_INIT },
    { ERR_MAN_HW_INIT, ERR_DESC_SUBINDEX_HW_INIT, ERR_INIT_QSPI, ERRCODE_QSPI_INIT },
    { ERR_MAN_HW_INIT, ERR_DESC_SUBINDEX_HW_INIT, ERR_I2C_AF_CONFIG, ERRCODE_I2C_AF_CONFIG },
    { ERR_MAN_HW_INIT, ERR_DESC_SUBINDEX_HW_INIT, ERR_ADC_CALIBRSTART, ERRCODE_ADC_CALIBRSTART },
    { ERR_MAN_HW_INIT, ERR_DESC_SUBINDEX_HW_INIT, ERR_PWM_CHANNEL_CONFIG, ERRCODE_PWM_CHANNEL_CONFIG },
    { ERR_MAN_HW_INIT, ERR_DESC_SUBINDEX_HW_INIT, ERR_PWM_STARTSTOP, ERRCODE_PWM_STARTSTOP },
    { ERR_MAN_HW_INIT, ERR_DESC_SUBINDEX_HW_INIT, ERR_DAC_CHANNEL_CONFIG, ERRCODE_DAC_CHANNEL_CONFIG },
    { ERR_MAN_HW_INIT, ERR_DESC_SUBINDEX_HW_INIT, ERR_TPM75_STATUS_RW, ERRCODE_TMP75 },
    { ERR_MAN_HW_INIT, ERR_DESC_SUBINDEX_HW_INIT, ERR_ISM330_STATUS_RW, ERRCODE_ISM330 },
    { ERR_MAN_HW_INIT, ERR_DESC_SUBINDEX_HW_INIT, ERR_INIT_WTDG, ERRCODE_WTDG_INIT },
    { ERR_MAN_CANOPEN, ERR_DESC_SUBINDEX_CANOPEN, ERR_CANOPEN_PASSIVE, ERRCODE_CAN_PASSIVE },
    { ERR_MAN_CANOPEN, ERR_DESC_SUBINDEX_CANOPEN, ERR_CANOPEN_BUS_OFF, ERRCODE_CAN_RECOVER_BOFF },
    { ERR_MAN_CANOPEN, ERR_DESC_SUBINDEX_CANOPEN, ERR_CANOPEN_OVERFLOW, ERRCODE_CAN_OVERRUN },
    { ERR_MAN_CANOPEN, ERR_DESC_SUBINDEX_CANOPEN, ERR_CANOPEN_TX_OVERFLOW, ERRCODE_CAN_OVERRUN_TX },
    { ERR_MAN_CANOPEN, ERR_DESC_SUBINDEX_CANOPEN, ERR_CANOPEN_RX_OVERFLOW, ERRCODE_CAN_OVERRUN_RX },
    { ERR_MAN_CANOPEN, ERR_DESC_SUBINDEX_CANOPEN, ERR_CANOPEN_CHGSTATE_STOPPED, ERRCODE_CAN_CHGSTATE_STOPPED },
    { ERR_MAN_STORAGE, ERR_DESC_SUBINDEX_STORAGE, ERR_STORAGE_SECTOR1, ERRCODE_STORAGE_SECTOR },
    { ERR_MAN_STORAGE, ERR_DESC_SUBINDEX_STORAGE, ERR_STORAGE_SECTOR2, ERRCODE_STORAGE_SECTOR },
    { ERR_MAN_STORAGE, ERR_DESC_SUBINDEX_STORAGE, ERR_STORAGE_SECTOR3, ERRCODE_STORAGE_SECTOR },
    { ERR_MAN_STORAGE, ERR_DESC_SUBINDEX_STORAGE, ERR_STORAGE_SECTOR4, ERRCODE_STORAGE_SECTOR },
    { ERR_MAN_STORAGE, ERR_DESC_SUBINDEX_STORAGE, ERR_STORAGE_SECTOR5, ERRCODE_STORAGE_SECTOR },
    { ERR_MAN_STORAGE, ERR_DESC_SUBINDEX_STORAGE, ERR_STORAGE_ASSET, ERRCODE_STORAGE_ASSET },
    { ERR_MAN_APP_ERR, ERR_DESC_SUBINDEX_APP_ERR, ERR_APPLICATION_CRC, ERRCODE_APP_CRC },
    { ERR_MAN_APP_ERR, ERR_DESC_SUBINDEX_APP_ERR, ERR_NO_APPLICATION, ERRCODE_NO_APP },
    { ERR_MAN_APP_ERR, ERR_DESC_SUBINDEX_APP_ERR, ERR_FLASH_ERASE, ERRCODE_FLASH_ERASE },
    { ERR_MAN_APP_ERR, ERR_DESC_SUBINDEX_APP_ERR, ERR_FLASH_PROGRAM, ERRCODE_FLASH_PROGRAM },

};

/*! Number of emcy codes */
static const uint8_t EMCY_LIST_SIZE = (sizeof(emcyListTable) / sizeof(emcyListTable[0]));

/*----------------------------------------------------------------------------*/
/*!
 @brief   Find the needed error emcy code

 @param   ERR_HANDLE the type of error written in p301_manu_status_register
 @param   ERR_TYPE   the type of specific error type in manErrorDesc
 @param   numTable   pointer to save error table number
 @return  true if error exists, false otherwise
*/
/*----------------------------------------------------------------------------*/
static bool findElement(uint32_t const ERR_HANDLE, uint32_t const ERR_TYPE, uint8_t *numTable)
{
    // while next node exists
    for (uint8_t i = 0; i < EMCY_LIST_SIZE; i++)
    {
        // if error type we are searching is equal to current item error type
        if (emcyListTable[i].errManuStatusBit == ERR_HANDLE && emcyListTable[i].emcyIdBit == ERR_TYPE)
        {
            // returning item that matches
            *numTable = i;
            return true;
        }
    }
    return false;
}

/*----------------------------------------------------------------------------*/
/*!
 @brief   This function sets the error type in manErrorDesc object and
          manufacturer status register

 @param   ERR_HANDLE the type of error written in p301_manu_status_register
 @param   ERR_TYPE   the type of specific error type in manErrorDesc
*/
/*----------------------------------------------------------------------------*/
void set_error(uint32_t const ERR_HANDLE, uint32_t const ERR_TYPE)
{
    // Search for element needed
    uint8_t emcyTableNum = 0;
    if (findElement(ERR_HANDLE, ERR_TYPE, &emcyTableNum))
    {
        p301_manu_status_register |= ERR_HANDLE;
        manErrorDesc[emcyListTable[emcyTableNum].errDescRegByte] |= ERR_TYPE;

        // Send EMCY
        (void)writeEmcyReq(emcyListTable[emcyTableNum].emcyCode, NULL CO_COMMA_LINE_PARA);
    }
}

/*----------------------------------------------------------------------------*/
/*!
 @brief   This function clears the error type in manErrorDesc object and
          manufacturer status register

 @param   ERR_HANDLE the type of error written in p301_manu_status_register
 @param   ERR_TYPE   the type of specific error type in manErrorDesc
*/
/*----------------------------------------------------------------------------*/
void clear_error(uint32_t const ERR_HANDLE, uint32_t const ERR_TYPE)
{
    // Search for element needed
    uint8_t emcyTableNum = 0;
    if (findElement(ERR_HANDLE, ERR_TYPE, &emcyTableNum))
    {
        p301_manu_status_register &= ~ERR_HANDLE;
        manErrorDesc[emcyListTable[emcyTableNum].errDescRegByte] &= ~ERR_TYPE;
    }
}
