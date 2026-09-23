/*!
 * Copyright Siemens Healthcare GmbH 2022, All Rights Reserved
 *
 * Project: Building Block Low End MCU
 *
 * @file
 * @brief Callback functions to the CANopen services
 *
 * This module contains callback functions to the CANopen services
 * specified in the CiA standard CiA-301.
 * Additionally there are callback functions for the error handling
 * and CANopen Library timers.
 * These callback functions are called by the CANopen Library
 * when the CANopen service is active and the implemented application-specific
 * actions are executed.
 *
 */

/* headers of standard C - libraries */
#include <stdio.h>
#include <stdbool.h>
// HAL includes
#include <hal.h>

/* headers of the CANopen Library  */
#define DEF_HW_PART
#include <cal_conf.h>
#include <co_stru.h>
#include <co_acces.h>
#include <co_sdo.h>
#include <co_pdo.h>
#include <co_time.h>
#include <co_emcy.h>
#include <co_flag.h>
#include <co_usr.h>
#include <co_nmt.h>
#include <co_timer.h>
#include <co_stor.h>
#include <co_drv.h>
#include <co_odidx.h>

#if defined(CONFIG_MASTER) || defined(CONFIG_SLAVE_PLUS)
#include <co_nmt_m.h>
#endif /* defined(CONFIG_MASTER) || defined(CONFIG_SLAVE_PLUS) */

#include <objects.h>

// Project includes
#include "mem_map.h"
#include "selftest.h"
#include "timing.h"
#include "w25xx_qspi.h"
#include "prog_ctrl.h"
#include "storage.h"
#include "can_cfg.h"
#include "error_defines.h"
#include "error.h"
#include "debug.h"
#include "bsp_wtdg.h"

/* constant definitions
---------------------------------------------------------------------------*/
#define FLASH_BUFFER_SIZE W25_WRITE_PAGE_SIZE
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
extern UNSIGNED8 lNodeId;

/* global variables
---------------------------------------------------------------------------*/

/* local defined variables
---------------------------------------------------------------------------*/
/*! Structure for update data*/
static struct
{
    UNSIGNED32 address;
    UNSIGNED32 crc;
    UNSIGNED32 size;
    UNSIGNED32 received;
    UNSIGNED32 counter;
    BOOL_T     block;
} updateData;

/*! Mediate buffer for flash update */
static UNSIGNED8 downloadBuffer[2 * FLASH_BUFFER_SIZE];
/*! Buffer for update*/
static uint8_t domainCommBuffer[CONFIG_DOMAIN_INDICATION_SIZE + 7];

/*******************************************************************/
/**
 * \brief getNodeId - get the node ID of the device
 *
 * This function has to be filled by the user.
 * It has to return the node ID of the device from e.g. a DIP switch
 * or nonvolatile memory to the CANopen layer.
 *
 * \return node-ID
 * node-ID in the range of 1..127, 255
 */
UNSIGNED8 getNodeId(CO_LINE_PARA_DECL /**< number of CAN line 0..CO_MAX_CAN_LINES-1 */
)
{
    return nodeId;
}

/*----------------------------------------------------------------------------*/
/*!
@brief          Initialize CANOpen user settings

@return         None
*/
/*----------------------------------------------------------------------------*/
void init_UserSettings(void)
{
    (void)setDomainAddr(DOWNLOAD_PROGRAM_DATA, 1, domainCommBuffer CO_COMMA_LINE_PARA);
    (void)setDomainSize(DOWNLOAD_PROGRAM_DATA, 1, APP_MAX_SIZE CO_COMMA_LINE_PARA);
}


#ifdef CONFIG_SDO_SERVER
/********************************************************************/
/**
 * \brief sdoWrInd - indicate the receipt of a SDO write request
 *
 * This function is called if an SDO write request reaches the CANopen
 * SDO server. Parameters of the function are the index and sub-index
 * of the entry in the local object dictionary where the data
 * should be written to.
 *
 * If numerical data with size up to 4 byte should be written,
 * the Library stores the previous value in a temporary buffer.
 * The new value is put into the local object dictionary.
 *
 * If the application does not accept this new value and this function
 * return with an error, the old value is restored from the temporary buffer
 * and written back to the object dictionary and the SDO write request
 * will be answered with a "\b Abort \b Domain \b Transfer" by the Library.
 * The SDO Abort Code can be specified by the \b return -value.
 *
 * \return
 * The return value, which has to be specified by the application,
 * selects the possible protocol answer of the SDO write request.
 * \retval CO_OK
 * success
 * \retval RET_T
 * One of the valid, SDO related, values can be returned.
 * This value is transferred  to  \em abortSdoTransf_Req() .
 * Possible are:
 * \li \c CO_E_NONEXIST_OBJECT
 * \li \c CO_E_NONEXIST_SUBINDEX
 * \li \c CO_E_NO_READ_PERM
 * \li \c CO_E_NO_WRITE_PERM
 * \li \c CO_E_MAP
 * \li \c CO_E_DATA_LENGTH
 * \li \c CO_E_TRANS_TYPE
 * \li \c CO_E_VALUE_TO_HIGH
 * \li \c CO_E_VALUE_TO_LOW
 * \li \c CO_E_WRONG_SIZE
 * \li \c CO_E_PARA_INCOMP
 * \li \c CO_E_HARDWARE_FAULT
 * \li \c CO_E_SRD_NO_RESSOURCE
 * \li \c CO_E_SDO_CMD_SPEC_INVALID
 * \li \c CO_E_MEM
 * \li \c CO_E_SDO_INVALID_BLKSIZE
 * \li \c CO_E_SDO_INVALID_BLKCRC
 * \li \c CO_E_SDO_TIMEOUT
 * \li \c CO_E_INVALID_TRANSMODE
 * \li \c CO_E_SDO_OTHER
 * \li \c CO_E_DEVICE_STATE
 *
 * All other return values are defaulting to E_SDO_OTHER.
 */
RET_T sdoWrInd(UNSIGNED16 index,   /**< index of object */
               UNSIGNED8  subIndex /**< sub-index of object */
#ifdef CONFIG_SPLIT_INDICATION
               ,
               UNSIGNED8 sdoNum /**< number of the SDO service */
#endif
                   CO_COMMA_LINE_PARA_DECL /**< number of CAN line 0..CO_MAX_CAN_LINES-1 */
)
{
    if (index == DOWNLOAD_PROGRAM_CONTROL && subIndex == 1)
    {
        switch (p302_program_control[1])
        {
            case PROG_COMMAND_BOOTLOADER:
                // Nothing to do
                break;
            case PROG_COMMAND_APPLICATION:
                // Check application
                if (selftestFlashApp(APPLICATION_STORED_ADDRESS))
                {
                    // Set signature to start application
                    progCtrlSetSignature(PROG_CTRL_START_APPLICATION);
                    // Application will be run after the reset
                    needToReset = true;
                }
                else
                {
                    return CO_E_BAD_CRC;
                }
                break;
            default: return CO_E_VALUE_TO_HIGH;
        }
    }


    // Update Device Name
    if (index == 0x2008)
    {
        memcpy(p301_manu_device_name, manDeviceName, sizeof(p301_manu_device_name));
    }
    // Update HW version
    if (index == 0x2009)
    {
        memcpy(p301_manu_hardware_version, manHWVersion, sizeof(p301_manu_hardware_version));
    }
    // Update Identity Object
    if (index == 0x2018)
    {
        p301_identity = manIdentityObject;
    }


    return CO_OK;
}
#endif /* CONFIG_SDO_SERVER */


#ifdef CONFIG_SDO_SERVER
/********************************************************************/
/**
 * \brief sdoRdInd - indicate the receipt of a SDO read request
 *
 * This function is called after the SDO server has received a SDO read
 * request and before the Library transmits the requried object value
 * from the object dictionary. The user has the possibility to update
 * the object value before the object value is sent.
 *
 * If this functions returns an error, a SDO Abort Transfer is initiated.
 *
 * \retval CO_OK
 * success
 * \retval RET_T
 * One of the valid, SDO related, values can be returned.
 * This value is transferred  to  \em abortSdoTransf_Req() .
 * Possible are:
 * \li \c CO_E_NONEXIST_OBJECT
 * \li \c CO_E_NONEXIST_SUBINDEX
 * \li \c CO_E_NO_READ_PERM
 * \li \c CO_E_NO_WRITE_PERM
 * \li \c CO_E_MAP
 * \li \c CO_E_DATA_LENGTH
 * \li \c CO_E_TRANS_TYPE
 * \li \c CO_E_VALUE_TO_HIGH
 * \li \c CO_E_VALUE_TO_LOW
 * \li \c CO_E_WRONG_SIZE
 * \li \c CO_E_PARA_INCOMP
 * \li \c CO_E_HARDWARE_FAULT
 * \li \c CO_E_SRD_NO_RESSOURCE
 * \li \c CO_E_SDO_CMD_SPEC_INVALID
 * \li \c CO_E_MEM
 * \li \c CO_E_SDO_INVALID_BLKSIZE
 * \li \c CO_E_SDO_INVALID_BLKCRC
 * \li \c CO_E_SDO_TIMEOUT
 * \li \c CO_E_INVALID_TRANSMODE
 * \li \c CO_E_SDO_OTHER
 * \li \c CO_E_DEVICE_STATE
 *
 * All other return values are defaulting to E_SDO_OTHER.
 */
RET_T sdoRdInd(UNSIGNED16 index,   /**< index of object */
               UNSIGNED8  subIndex /**< sub-index of object */
#ifdef CONFIG_SPLIT_INDICATION
               ,
               UNSIGNED8 sdoNum /**< number of the SDO service */
#endif
                   CO_COMMA_LINE_PARA_DECL /**< number of CAN line 0..CO_MAX_CAN_LINES-1 */
)
{
    (void)index;
    (void)subIndex;
    DEBUGOUT(LOG_DEBUG, "index: 0x%X, subIndex: %d\n", index, subIndex);
    return CO_OK;
}
#endif /* CONFIG_SDO_SERVER */

/*----------------------------------------------------------------------------*/
/*!
@brief          Copies data from the RAM buffer to the flash
*/
/*----------------------------------------------------------------------------*/
static BOOL_T writeBufferToFlash(void)
{
    if (updateData.counter > FLASH_BUFFER_SIZE)
    {
        uint32_t status = w25Write(updateData.address, downloadBuffer, FLASH_BUFFER_SIZE);
        if (status != HAL_OK)
        {
            set_error(ERR_MAN_APP_ERR, ERR_FLASH_PROGRAM);
            return CO_FALSE;
        }
        // Increment address
        updateData.address += FLASH_BUFFER_SIZE;

        // Move the rest of data to the begginning of the buffer
        updateData.counter -= FLASH_BUFFER_SIZE;
        memmove(downloadBuffer, &downloadBuffer[FLASH_BUFFER_SIZE], updateData.counter);
    }
    return CO_TRUE;
}

#if defined(CONFIG_SDO_SERVER) && defined(CONFIG_DOMAIN_INDICATION_SIZE)
/********************************************************************/
/**
 * \brief sdoDomainInd - domain size border reached
 *
 * This function is called for SDO Domain transfers
 * after the receipt of CONFIG_DOMAIN_INDICATION_SIZE bytes.
 * The application gets the possibilty to save the received data
 * for instance in the flash memory.
 * After leaving this function is buffer will be overwritten
 * with new received data.
 *
 * The buffer size CONFIG_DOMAIN_INDICATION_SIZE will not be
 * divisible by the data length of the SDO messages, i.e. divisible by 7.
 * This function is called when CONFIG_DOMAIN_INDICATION_SIZE and more data
 * are received.
 * The application is responsible to save the oversized bytes temporarily.
 * Therefore the function gets as parameter:
 * - actSize: number of bytes at the buffer to process by the application,
 *   including the number of overSize bytes from the last cycle
 * - overSize: number of oversized bytes received with last SDO
 * The CANopen Library controls the byte counting.
 *
 * At the end of SDO transfer the indication function sdoWrInd() is called.
 *
 * \return
 * CANopen return value
 */

RET_T sdoDomainInd(UNSIGNED16 index,           /**< index if current SDO access */
                   UNSIGNED8  subIndex,        /**< sub-index of current SDO access */
                   UNSIGNED8 *pData,           /**< pointer to domain buffer */
                   UNSIGNED32 actSize,         /**< number of bytes to flash */
                   UNSIGNED8 overSize          /**< number of bytes to store temporarily */
                       CO_COMMA_LINE_PARA_DECL /**< number of CAN line 0..CO_MAX_CAN_LINES-1 */
)
{
    if ((index == DOWNLOAD_PROGRAM_DATA) && (subIndex == 1))
    {
        if (actSize + overSize != 0)
        {
            // Copy data to buffer
            memcpy(&downloadBuffer[updateData.counter], pData, actSize + overSize);
            updateData.counter += actSize + overSize;
            if (updateData.block != CO_TRUE && writeBufferToFlash() == CO_FALSE)
            {
                return CO_E_HARDWARE_FAULT;
            }
            // Calculate received bytes count
            updateData.received += actSize + overSize;
        }
        else
        {
            // Check received size
            if (updateData.received != updateData.size)
            {
                return CO_E_HARDWARE_FAULT;
            }

            // Finish download
            if (writeBufferToFlash() == CO_FALSE)
            {
                return CO_E_HARDWARE_FAULT;
            }

            // Write rest of the data to flash
            // Fill up the buffer with 0xFF
            memset(&downloadBuffer[updateData.counter], 0xFF, FLASH_BUFFER_SIZE - updateData.counter);
            // Write data to flash
            uint32_t status = w25Write(updateData.address, downloadBuffer, FLASH_BUFFER_SIZE);
            if (status != HAL_OK)
            {
                set_error(ERR_MAN_APP_ERR, ERR_FLASH_PROGRAM);
                return CO_E_HARDWARE_FAULT;
            }
        }
    }

    return CO_OK;
}
#endif /* defined(CONFIG_SDO_SERVER) && defined(CONFIG_DOMAIN_INDICATION_SIZE) */


#if defined(CONFIG_SDO_SERVER) && defined(CONFIG_DOMAIN_INDICATION_SIZE)
/********************************************************************/
/**
 * \brief coUserSdoDomainUploadInd - update buffer for domain upload
 *
 * This function is called for SDO domain upload transfers
 * after the sending of CONFIG_DOMAIN_INDICATION_SIZE bytes.
 * The data has to be provided by the application.
 *
 * \return
 * CANopen return value
 */
RET_T coUserSdoDomainUploadInd(UNSIGNED16 index,           /**< index if current SDO access */
                               UNSIGNED8  subIndex,        /**< sub-index of current SDO access */
                               UNSIGNED8 *pData,           /**< pointer to the domain buffer */
                               UNSIGNED32 *pSize           /**< number of loaded bytes to the domain buffer */
                                   CO_COMMA_LINE_PARA_DECL /**< number of CAN line 0..CO_MAX_CAN_LINES-1 */
)
{
    (void)index;
    (void)subIndex;
    (void)pData;
    (void)pSize;
    return CO_OK;
}
#endif /* defined(CONFIG_SDO_SERVER) && defined(CONFIG_DOMAIN_INDICATION_SIZE) */

#ifdef CONFIG_SDO_BLOCK_INDICATION
/*******************************************************************/
/**
 * \brief sdoBlockInd - indicate the receive of one block
 *
 * This function is called for block transfers,
 * after each block is saved at receive buffer
 * and before the answer is sent to the sdo client.
 * The application can process the data
 * and has the possibility to abort the transfer
 * with a special abort code.
 *
 *++ The parameter
 * size
 *++ provides the actual received data count.
 *
 * \retval CO_OK
 *	ok - continue transfer
 * \retval RET_T
 *	abort sdo transfer
 *
 */
RET_T sdoBlockInd(UNSIGNED16 index,    /**< index */
                  UNSIGNED8  subIndex, /**< subIndex */
                  UNSIGNED32 size      /**< actual received size */
                      CO_COMMA_LINE_PARA_DECL)
{
    (void)size;
    if ((index == DOWNLOAD_PROGRAM_DATA) && (subIndex == 1))
    {
        updateData.block = CO_TRUE;

        // Check if program falsh is necessary
        if (writeBufferToFlash() == CO_FALSE)
        {
            return CO_E_HARDWARE_FAULT;
        }
    }
    return CO_OK;
}
#endif /* CONFIG_SDO_BLOCK_INDICATION */


#if defined(CONFIG_SDO_SERVER) && defined(CONFIG_VALUE_CHECK_FUNCTION)
/********************************************************************/
/**
 * \brief testSdoValue - check the value of a SDO before writing to OD
 *
 * This function makes it possible to check the received value
 * before the new value is written into the object dictinary.
 * If the return value is not CO_OK the write access will be refused.
 * The new value is not written into the object dictinary.
 *
 * This function is also called for non-numerical objects which usually contain
 * more than 8 byte and so both pData and size may not point to valid data.
 * The library is then not able to provide backed up data and the new value is
 * always written. In this case this function will simply provide another
 * point at which to attach some logic, for example some initialization
 * for an SDO domain transfer. Care must be taken that this function returns
 * CO_OK for non-numerical data.
 *
 * \return
 * CANopen return value
 */
RET_T testSdoValue(UNSIGNED16 index,           /**< index of object */
                   UNSIGNED8  subIndex,        /**< sub-index of object */
                   void      *pData,           /**< pointer to new data, little-endian format */
                   UNSIGNED32 size             /**< data size */
                       CO_COMMA_LINE_PARA_DECL /**< number of CAN line 0..CO_MAX_CAN_LINES-1 */
)
{
    (void)pData;
    if ((index == DOWNLOAD_PROGRAM_DATA) && (subIndex == 1))
    {
        // Erase memory
        if (w25Erase(0, size) != W25_OK)
        {
            set_error(ERR_MAN_APP_ERR, ERR_FLASH_ERASE);
            return CO_E_HARDWARE_FAULT;
        }

        // Set update data
        updateData.address  = 0;
        updateData.size     = size;
        updateData.received = 0;
        updateData.counter  = 0;
        updateData.block    = CO_FALSE;
        printf("Size: %ld\n", updateData.size);
    }

    if (index == 0x2008)
    {
        // "Erase" previous data so as new string will be written
        memset(manDeviceName, '\0', sizeof(manDeviceName));
    }
    if (index == 0x2009)
    {
        // "Erase" previous data so as new string will be written
        memset(manHWVersion, '\0', sizeof(manHWVersion));
    }

    return CO_OK;
}
#endif /* defined(CONFIG_SDO_SERVER) && defined(CONFIG_VALUE_CHECK_FUNCTION) */


#ifdef CONFIG_NON_VOLATILE_MEM
/********************************************************************/
/**
 * \brief saveParameterInd - store data into nonvolatile memory
 *
 * This function indicates a "store parameters to nonvolatile memory"
 * command via SDO. This command is a SDO write access to object 0x1010
 * with the signature "save".
 * In this function the application has to integrate the target-specific
 * save functions.
 * If this function returns an error an \b SDO \b Abort \b Domain \b Transfer
 * is initiated with the error code "hardware fault".
 * The parameter segment corresponds to the sub-index of object 0x1010.
 *
 * \retval CO_TRUE
 * success
 * \retval CO_FALSE
 * error
 */
BOOL_T saveParameterInd(UNSIGNED8 segment           /**< sub-index which specifies the memory segment */
                            CO_COMMA_LINE_PARA_DECL /**< number of CAN line 0..CO_MAX_CAN_LINES-1 */
)
{
    BOOL_T bRet = CO_TRUE;

    uint16_t segToWrite = 0;
    switch (segment)
    {
        // All parameters
        case MEM_SEG_ALL_PARAMETERS:
            for (uint16_t i = 0; i < storageSegmentsCount; i++)
            {
                segToWrite |= (1U << i);
            }
            break;
        // Communication parameter
        case MEM_SEG_COM_PARAMETERS: segToWrite = (1U << STORAGE_SEG_COMM); break;
        // Application parameter
        case MEM_SEG_APPL_PARAMETERS: segToWrite = (1U << STORAGE_SEG_APPL); break;
        // Segment 4 - 127 manufacturer specific
        // Asset Data
        case MEM_SEG_ASSET_DATA: segToWrite = (1U << STORAGE_SEG_ASSET); break;
        default: return CO_FALSE;
    }

    for (uint8_t i = 0; i < storageSegmentsCount; i++)
    {
        if (((segToWrite & (1U << i)) != 0) && (!storageSaveSegment(i)))
        {
            // Set error registers
            set_error(ERR_MAN_STORAGE, (1U << i));
            bRet = CO_FALSE;
        }
    }

    return bRet;
}
#endif /* CONFIG_NON_VOLATILE_MEM */


#ifdef CONFIG_NON_VOLATILE_MEM
/********************************************************************/
/**
 * \brief clearParameterInd - restoring of default parameter
 *
 * This function indicates a "restore default parameter".
 * It is called at a write access to the object 0x1011 with the signature
 * "load".
 *
 * The application has to ensure that the next call of the function
 * loadParameterInd() after Reset Communication or Reset Application
 * makes all default values available. This can be done by erasing
 * the nonvolatile memory at this function.
 *
 * If this function returns an error an
 * \b SDO \b Abort \b Domain \b Transfer
 * is initiated with the error code "hardware fault".
 *
 * \retval CO_TRUE
 * success
 * \retval CO_FALSE
 * error
 */
BOOL_T clearParameterInd(UNSIGNED8 segment           /**< sub-index which specifies the memory segment */
                             CO_COMMA_LINE_PARA_DECL /**< number of CAN line 0..CO_MAX_CAN_LINES-1 */
)
{
    BOOL_T bRet = CO_TRUE;

    uint16_t segToClear = 0;
    switch (segment)
    {
        // All parameters
        case MEM_SEG_ALL_PARAMETERS:
            for (uint16_t i = 0; i < storageSegmentsCount; i++)
            {
                segToClear |= (1U << i);
            }
            break;
        // Communication parameter
        case MEM_SEG_COM_PARAMETERS: segToClear = (1U << STORAGE_SEG_COMM); break;
        // Application parameter
        case MEM_SEG_APPL_PARAMETERS: segToClear = (1U << STORAGE_SEG_APPL); break;
        // Segment 4 - 127 manufacturer specific
        // Asset Data
        case MEM_SEG_ASSET_DATA: segToClear = (1U << STORAGE_SEG_ASSET); break;
        default: return CO_FALSE;
    }

    for (uint8_t i = 0; i < storageSegmentsCount; i++)
    {
        if (((segToClear & (1U << i)) != 0) && (!storageEraseSegment(i)))
        {
            // Set error registers
            set_error(ERR_MAN_STORAGE, (1U << i));
            bRet = CO_FALSE;
        }
    }

    return bRet;
}
#endif /* CONFIG_NON_VOLATILE_MEM */


#ifdef CONFIG_NON_VOLATILE_MEM
/********************************************************************/
/**
 * \brief loadParameterInd - load parameters from nonvolatile memory
 *
 * This function is called to load parameters from the nonvolatile memory
 * to the object dictionary. It is called from the Library
 * at Boot-up, Reset Communication and Reset Application.
 *
 * The parameter \em segment describes,
 * which part of the object dictionary shall be updated.
 *
 * The parameter mode specifies, which restore mode should be used:
 *
 * \li CO_RESTORE_MODE_BOOTUP
 * \par
 * During boot-up the object dictionary is overwritten
 * by data from nonvolatile memory.
 *
 * \li CO_RESTORE_MODE_RESETCOMM
 * \par
 * During the next Reset Communication after a write access to object 0x1011
 * the object dictionary is overwritten by data from nonvolatile memory.
 *
 * \li CO_RESTORE_MODE_SDO
 * \par
 * The signature 'load' was written to object 0x1011.
 *
 * \retval CO_TRUE
 * success
 * \retval CO_FALSE
 * error
 */
BOOL_T loadParameterInd(UNSIGNED8 segment,          /**< sub-index which specifies the memory segment */
                        UNSIGNED8 mode              /**< restore mode */
                            CO_COMMA_LINE_PARA_DECL /**< number of CAN line 0..CO_MAX_CAN_LINES-1 */
)
{
    (void)mode;
    BOOL_T   bRet      = CO_TRUE;
    uint16_t segToRead = 0;
    switch (segment)
    {
        // All parameters
        case MEM_SEG_ALL_PARAMETERS:
            for (uint16_t i = 0; i < storageSegmentsCount; i++)
            {
                segToRead |= (1U << i);
            }
            break;
        // Communication parameter
        case MEM_SEG_COM_PARAMETERS: segToRead = (1U << STORAGE_SEG_COMM); break;
        // Application parameter
        case MEM_SEG_APPL_PARAMETERS: segToRead = (1U << STORAGE_SEG_APPL); break;
        // Segment 4 - 127 manufacturer specific
        // Asset Data
        case MEM_SEG_ASSET_DATA: segToRead = (1U << STORAGE_SEG_ASSET); break;
        default: return CO_FALSE;
    }

    for (uint8_t i = 0; i < storageSegmentsCount; i++)
    {
        if (((segToRead & (1U << i)) != 0) && (!storageLoadSegment(i)))
        {
            // Set error registers
            set_error(ERR_MAN_STORAGE, (1U << i));
            bRet = CO_FALSE;
        }
    }


    return bRet;
}
#endif /* CONFIG_NON_VOLATILE_MEM */


#ifdef CONFIG_CAN_ERROR_HANDLING
/********************************************************************/
/**
 * \brief canErrorInd - indicate the occurrence of errors on the CAN driver
 *
 * This function indicates the following errors:
 *
 * - \c CANFLAG_ACTIVE -
 *   CAN Error Active
 *
 * - \c CANFLAG_BUSOFF -
 * CAN-controller error CAN Busoff
 *
 * - \c CANFLAG_PASSIVE -
 * CAN-controller error
 *
 * - \c CANFLAG_OVERFLOW -
 * CAN-controller overrun error
 *
 * - \c CANFLAG_TXBUFFER_OVERFLOW -
 * transmit buffer overflow
 *
 * - \c CANFLAG_RXBUFFER_OVERFLOW -
 * receive buffer overflow
 *
 *
 * All occurred status changes since the last canErrorInd() call are indicated.
 * The current state can be read with getCanDriverState().
 *
 * The handling of CAN driver error can be enabled
 * by the CANopen Design Tool about:
 * General Settings / Enable CAN communication error handling
 *
 * \retval
 * CO_TRUE
 * CAN controller has to stay in the current state
 * \retval
 * CO_FALSE
 * CAN controller has to go to BUS-ON again
 */
BOOL_T canErrorInd(UNSIGNED8 errorFlags         /**< CAN error flags */
                       CO_COMMA_REDCY_PARA_DECL /**< number of CAN line 0..CO_MAX_CAN_LINES-1 */
)
{
    BOOL_T ret = CO_TRUE;    // return value


    // CAN error passive
    if ((errorFlags & CANFLAG_PASSIVE) != 0)
    {
        // Set error registers
        set_error(ERR_MAN_CANOPEN, ERR_CANOPEN_PASSIVE);
#ifdef CONFIG_EMCY_PRODUCER
        (void)writeEmcyReq(ERRCODE_CAN_PASSIVE, NULL CO_COMMA_LINE_PARA);
#endif    // CONFIG_EMCY_PRODUCER
    }

    // CAN bus-off
    if ((errorFlags & CANFLAG_BUSOFF) != 0)
    {
        // Set error registers
        set_error(ERR_MAN_CANOPEN, ERR_CANOPEN_BUS_OFF);
        ret = CO_FALSE;    // auto Bus-On
    }

    // CAN controller overflow
    if ((errorFlags & CANFLAG_OVERFLOW) != 0)
    {
        // Set error registers
        set_error(ERR_MAN_CANOPEN, ERR_CANOPEN_OVERFLOW);
    }

    // Library RX buffer overflow
    if ((errorFlags & CANFLAG_RXBUFFER_OVERFLOW) != 0)
    {
        // Set error registers
        set_error(ERR_MAN_CANOPEN, ERR_CANOPEN_RX_OVERFLOW);
    }

    // Library TX buffer overflow
    if ((errorFlags & CANFLAG_TXBUFFER_OVERFLOW) != 0)
    {
        // Set error registers
        set_error(ERR_MAN_CANOPEN, ERR_CANOPEN_TX_OVERFLOW);
    }

    return ret;
}
#endif /* CONFIG_CAN_ERROR_HANDLING */


#ifdef CONFIG_USER_CAN_MSG
/****************************************************************************/
/**
 * \brief usrCanMsgReceived - callback for user defined rx messages
 *
 * This function supports the handling of \em user-specific cob-IDs
 * which where defined by the user.
 *
 * The Library calls this function when a CAN message
 * with a user-defined cob-ID was received.
 *
 * The compiler define CONFIG_USER_CAN_MSG is set by the CANopen Design Tool
 * automatically when the number of user-specific cob-IDs was specified about:
 * Line / Additional Settings / Number of user-specific cob-IDs
 *
 * \return
 * nothing
 */
void usrCanMsgReceived(CAN_MSG_T *canMsg           /**< pointer at CAN message */
                           CO_COMMA_LINE_PARA_DECL /**< number of CAN line 0..CO_MAX_CAN_LINES-1 */
)
{
}
#endif /* CONFIG_USER_CAN_MSG */


#ifdef CO_CONFIG_USER_MESSAGE_TEST
/*******************************************************************/
/**
 * \brief coUserMessageTestInd - indicate any received message
 *
 * Any CAN message received by the library will be indicated in this
 * function. The application can cause special behaviour or decide that
 * the message should be ignored by the library.
 *
 * Note: SYNC messages are given preference by the stack and handled
 * differently. They will not be reported by this function.
 *
 *\retval CO_OK
 * library shall handle the message
 *
 *\retval other RET_T
 * library shall ignore the message
 *
 */
RET_T coUserMessageTestInd(CAN_MSG_T *canMsg CO_COMMA_REDCY_PARA_DECL)
{
    UNSIGNED8 i = 0x0;

    PRINTF("0x%x : ", canMsg->cobId);
    for (i = 1; i < canMsg->length; i++)
    {
        PRINTF("%x ", canMsg->pData[i - 1]);
    }
    PRINTF("\n");

    return CO_OK;
}
#endif /* CO_CONFIG_USER_MESSAGE_TEST */


/*______________________________________________________________________EOF_*/
