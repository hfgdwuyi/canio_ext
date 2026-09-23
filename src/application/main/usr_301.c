/*!
 * Copyright � Siemens Healthcare GmbH 2022, All Rights Reserved
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
#include <limits.h>
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

#include "bsp_board.h"
#include "error_defines.h"
#include "timing.h"
#include "bsp_dio.h"
#include "prog_ctrl.h"
#include "p401_dout.h"
#include "p401_ain.h"
#include "p401_aout.h"
#include "storage.h"
#include "error.h"
#include "tmp75.h"
#include "ism330dlc.h"
#include "rotation_detection.h"

#include "debug.h"

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
static volatile uint32_t g_syncLastRxTickMs = 0U;
static volatile BOOL_T   g_syncSeenInOperational = CO_FALSE;
static volatile BOOL_T   g_syncWatchdogEnabled = CO_FALSE;

void appSyncWatchdogOnSyncRx(void)
{
    g_syncLastRxTickMs       = HAL_GetTick();
    g_syncSeenInOperational  = CO_TRUE;
}

void appSyncWatchdogOnNmtStateChange(NODE_STATE_T newState)
{
    if (newState == OPERATIONAL)
    {
        g_syncWatchdogEnabled    = CO_TRUE;
        g_syncSeenInOperational  = CO_FALSE;
        g_syncLastRxTickMs       = HAL_GetTick();
    }
    else
    {
        g_syncWatchdogEnabled    = CO_FALSE;
        g_syncSeenInOperational  = CO_FALSE;
    }
}

void appSyncWatchdogProcess(void)
{
    if (g_syncWatchdogEnabled == CO_FALSE)
    {
        return;
    }

    if (getNodeState() != OPERATIONAL)
    {
        g_syncWatchdogEnabled = CO_FALSE;
        return;
    }

    if (p301_comm_cycle_period == 0U)
    {
        return;
    }

    // Require one SYNC in current OP session before enforcing timeout.
    if (g_syncSeenInOperational == CO_FALSE)
    {
        return;
    }

    // 0x1006 is in microseconds.
    uint32_t timeoutMs = p301_comm_cycle_period / 1000U;
    if (timeoutMs == 0U)
    {
        timeoutMs = 1U;
    }

    uint32_t now = HAL_GetTick();
    if ((now - g_syncLastRxTickMs) <= timeoutMs)
    {
        return;
    }

    (void)setNodePREOP();
    g_syncWatchdogEnabled = CO_FALSE;
}

static void changeDIFilterTime(uint8_t pinNumb)
{
    uint8_t        pin  = pinNumb - 1;
    bspDinSettings dset = { .deb_time = manDIFilterTime[pinNumb] };
    uint8_t        byte = pin / CHAR_BIT;
    uint8_t        bit  = pin % CHAR_BIT;
    dset.deb_en         = (p401DIFilterConstant8Bit[byte + 1] & (1U << bit)) != 0 ? true : false;
    bspDinSetDebouncing(pin, dset);
}

/* external variables
---------------------------------------------------------------------------*/
extern UNSIGNED8 nodeId;

/* global variables
---------------------------------------------------------------------------*/

/* local defined variables
---------------------------------------------------------------------------*/


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


#ifdef CONFIG_EMCY_CONSUMER
/*******************************************************************/
/**
 * \brief emcyInd - indicate the receipt of an EMCY message
 *
 * This function is called after an EMCY messages was received
 * from an other EMCY producer node in the network.
 * The user can define an application-specific error handling here.
 *
 * \return
 * nothing
 */
void emcyInd(UNSIGNED8 emcyNum,          /**< emergency number */
             EMERGENCY_T *pEmcy          /**< data of the received EMCY message */
                 CO_COMMA_LINE_PARA_DECL /**< number of CAN line 0..CO_MAX_CAN_LINES-1 */
)
{
    switch (pEmcy->errCode)
    {
        case 0x4000: break;
        default: break;
    }
}
#endif /* CONFIG_EMCY_CONSUMER */


#ifdef CONFIG_TIME_CONSUMER
/*******************************************************************/
/**
 * \brief timeInd - indicate the receipt of a Time Stamp object
 *
 * In this function the user can implement the application-specific
 * Time Stamp handling.
 * The \c TIME_OF_DAY_T structure, referenced by address, contains
 * the time in ms after midnight and the number of day since January 1, 1984.
 *
 * \return
 * nothing
 */
void timeInd(TIME_OF_DAY_T *address      /**< Time Stamp object */
                 CO_COMMA_LINE_PARA_DECL /**< number of CAN line 0..CO_MAX_CAN_LINES-1 */
)
{
}
#endif /* CONFIG_TIME_CONSUMER */


#ifdef CONFIG_PDO_CONSUMER
/*******************************************************************/
/**
 * \brief pdoInd - indicate the receipt of a PDO
 *
 * This function is called after a PDO was received.
 * All data from this PDO are saved at the object dictionary
 * before this function is called.
 * Synchronous PDOs are processed after the next SYNC was received.
 * It will be saved at the object dictionary
 * and after that this function is called.
 *
 * \return
 * nothing
 */
void pdoInd(UNSIGNED16 pdoNum           /**< number of PDO 1..512 */
                CO_COMMA_LINE_PARA_DECL /**< number of CAN line 0..CO_MAX_CAN_LINES-1 */
)
{
    switch (pdoNum)
    {
        case 1: break;
        case 2: break;
        case 3:
            {
                p401WriteDACValue(p401AnalogOutput16bit);
                break;
            }
        default: break;
    }
}
#endif /* CONFIG_PDO_CONSUMER */


#if defined(CONFIG_PDO_CONSUMER) && defined(CONFIG_PDO_BAD_LEN_INDICATION)
/*******************************************************************/
/**
 * \brief pdoLenInd - indicate the receipt of a PDO with invalid length
 *
 * This function is called after a PDO was received with invalid length.
 * The received PDO data do not match the PDO mapping.
 *
 * The parameter info means:
 * - PDO_LEN_TO_SHORT - to less data received, data will not be processed
 * - PDO_LEN_TO_LONG - to much data received, PDO data will be processed
 *   unused data are ignored
 *
 * The call of this indication function can be enabled for PDO consumer
 * by the CANopen Design Tool about:
 * Line / Object Dictionary / Communication Segment /
 * <PDO communication object> / tab Mask / General PDO Settings /
 * Enable Wrong PDO Lenght indication
 *
 * \return
 * CANopen return value
 */
RET_T pdoLenInd(UNSIGNED16 pdoNum,          /**< number of PDO */
                UNSIGNED8 info              /**< type of the error */
                    CO_COMMA_LINE_PARA_DECL /**< number of CAN line 0..CO_MAX_CAN_LINES-1 */
)
{
    return (CO_OK);
}
#endif /* CONFIG_PDO_CONSUMER && CONFIG_PDO_BAD_LEN_INDICATION */


#if defined(CONFIG_PDO_CONSUMER) && defined(CONFIG_PDO_EVENTTIMER)
/*******************************************************************/
/**
 * \brief pdoTimerInd - indicate the occurrence of a PDO timer event
 *
 * In this function the user can implement an application-specific
 * handling for an occurred PDO timer event.
 * For PDO remote requests, initiated by calling the function readPdoReq(),
 * the optional timer event time is used to watch the
 * occurrence of the requested PDO.
 * If the PDO does not arrive in this period,
 * this function will be called.
 *
 * \return
 * nothing
 */
void pdoTimerInd(UNSIGNED16 pdoNum           /**< number of PDO */
                     CO_COMMA_LINE_PARA_DECL /**< number of CAN line 0..CO_MAX_CAN_LINES-1 */
)
{
    // For now, no reaction on PDO timer event
    (void)pdoNum;
}
#endif /* defined(CONFIG_PDO_CONSUMER) && defined(CONFIG_PDO_EVENTTIMER) */


#if defined(CONFIG_PDO_PRODUCER) && defined(CONFIG_PDO_EVENTTIMER) && defined(CONFIG_PDO_EVENTTIMER_INDICATION)
/*******************************************************************/
/**
 * \brief pdoEventTimerInd - event timer PDO shall be transmitted
 *
 * This function is called if the time for a timer driven PDO has been elapsed
 * and the PDO should be transmitted.
 * The user has the possibility to actualize the data
 * or start other activities for this event.
 *
 * The call of this indication function can be enabled for PDO producers
 * with a PDO event timer by the CANopen Design Tool about:
 * Line / Object Dictionary / Communication Segment /
 * <PDO communication object> / tab Mask / General PDO Settings /
 * Enable PDO Event Timer Indication
 *
 * \return
 * nothing
 */
void pdoEventTimerInd(UNSIGNED16 pdoNum           /**< number of PDO */
                          CO_COMMA_LINE_PARA_DECL /**< number of CAN line 0..CO_MAX_CAN_LINES-1 */
)
{
}
#endif


#if defined(CONFIG_PDO_PRODUCER) && defined(CONFIG_PDO_RTR_IND)
/*******************************************************************/
/**
 * \brief rtrPdoInd - RTR PDO shall be transmitted
 *
 * This function is called if a RTR was received
 * and the PDO should be transmitted.
 * The user has the possibility to actualize the data
 * or start other activities for this event.
 *
 * The call of this indication function can be enabled for PDO producers
 * by the CANopen Design Tool about:
 * Line / Object Dictionary / Communication Segment /
 * <PDO communication object> / tab Mask / General PDO Settings /
 * Enable RTR-PDO Indication Function
 *
 * \return
 * nothing
 */
void rtrPdoInd(UNSIGNED16 pdoNum           /**< number of PDO */
                   CO_COMMA_LINE_PARA_DECL /**< number of CAN line 0..CO_MAX_CAN_LINES-1 */
)
{
    // TODO: Decide if we need RTR indication
    (void)pdoNum;
}
#endif /* defined(CONFIG_PDO_PRODUCER) && defined(CONFIG_PDO_RTR_IND) */


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
    switch (index)
    {
        case 0x5003:    // DI filter time
            changeDIFilterTime(subIndex);
            break;
        case 0x5004:    // AI filter time
            p401AIFilterCalculate();
            break;
        case 0x6003:    // DI filter constant
            for (uint8_t i = 0; i < CHAR_BIT; i++)
            {
                uint8_t        pin  = ((subIndex - 1) * CHAR_BIT) + i;
                bspDinSettings dset = { .deb_time = manDIFilterTime[pin + 1] };
                dset.deb_en         = (p401DIFilterConstant8Bit[subIndex] & (1U << i)) != 0 ? true : false;
                bspDinSetDebouncing(pin, dset);
            }
            break;
        case DOWNLOAD_PROGRAM_CONTROL:
            if (subIndex == 1 && p302_program_control[1] == PROG_COMMAND_BOOTLOADER)
            {
                // Set flag to start bootloader
                progCtrlSetSignature(PROG_CTRL_START_BOOTLOADER);
                // Need to reset
                needToReset = true;
            }
            // Set value back for reading
            p302_program_control[1] = PROG_COMMAND_APPLICATION;
            break;
        case 0x5101:
            {    // Temperature sensor configuration
                tmp75Config config;
                memcpy(&config, &manTMP75Config, sizeof(config));
                if (tmp75WriteConfig(&sensorTMP, config) != TMP75_RET_OK)
                {
                    set_error(ERR_MAN_HW_INIT, ERR_TPM75_STATUS_RW);
                    return CO_E_HARDWARE_FAULT;
                }
            }
            break;
        case 0x5102:
            {    // Temperature sensor limits
                if (tmp75WriteLimit(&sensorTMP, manTPM75Limits[subIndex], subIndex == 1 ? true : false) != TMP75_RET_OK)
                {
                    set_error(ERR_MAN_HW_INIT, ERR_TPM75_STATUS_RW);
                    return CO_E_HARDWARE_FAULT;
                }
            }
            break;
        case 0x5200:
            {    // Accelerometer write
                if (subIndex == 0x02)
                {
                    if (ism330dlcWriteRegister8bit(SYS_SPI_ISM330_NUMBER, (ism330dlcReg)manIsm330DLCData[1], manIsm330DLCData[2]) ==
                        RET_ISM330_OK)
                    {
                        clear_error(ERR_MAN_HW_INIT, ERR_ISM330_STATUS_RW);
                    }
                    else
                    {
                        set_error(ERR_MAN_HW_INIT, ERR_ISM330_STATUS_RW);
                        return CO_E_RANGE;
                    }
                }
            }
            break;
        case 0x5302:
            if(subIndex == 0x01)
            {
                if(manIsm330DLCCalibrate[1]) {
                    manIsm330DLCCalibrate[1] = 0;
                    calibrate_angles();
                }
            }
            else if(subIndex == 0x02)
            {
                if(manIsm330DLCCalibrate[2]) {
                    manIsm330DLCCalibrate[2] = 0;
                    calibrate_orientation();
                }
            }
            break;
        case 0x6411: p401WriteDACValue(p401AnalogOutput16bit); break;
        case 0x6200:
            {
                if (subIndex == 1 || subIndex == 2)
                {
                    p401WriteDOUT((uint8_t)(subIndex - 1U), p401DOWrite8Bit[subIndex]);
                }
            }
            break;
        case 0x2008:
            memcpy(p301_manu_device_name, manDeviceName, sizeof(p301_manu_device_name));
            break;
        case 0x2009:
            memcpy(p301_manu_hardware_version, manHWVersion, sizeof(p301_manu_hardware_version));
            break;
        case 0x200A:
            memcpy(p301_manu_software_version, manSWVersion, sizeof(p301_manu_software_version));
            break;
        case 0x2018:
            p301_identity = co_man_identity;
            break;
        default:
            // Nothing here
            break;
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
    switch (index)
    {
        case 0x5101:
            {    // Temperature sensor config
                tmp75Config config;
                if (tmp75ReadConfig(&sensorTMP, &config) != TMP75_RET_OK)
                {
                    set_error(ERR_MAN_HW_INIT, ERR_TPM75_STATUS_RW);
                    return CO_E_HARDWARE_FAULT;
                }
                else
                {
                    memcpy(&manTMP75Config, &config, sizeof(manTMP75Config));
                }
                break;
            }
        case 0x5102:
            {    // Temperature sensor limits
                if (tmp75ReadLimit(&sensorTMP, &manTPM75Limits[subIndex], subIndex == 1 ? true : false) != TMP75_RET_OK)
                {
                    set_error(ERR_MAN_HW_INIT, ERR_TPM75_STATUS_RW);
                    return CO_E_HARDWARE_FAULT;
                }
            }
            break;
        case 0x5200:
            {    // Accelerometer read
                if (subIndex == 0x02)
                {
                    uint16_t data;
                    if (ism330dlcReadRegister16bit(SYS_SPI_ISM330_NUMBER, (ism330dlcReg)manIsm330DLCData[1], &data) == RET_ISM330_OK)
                    {
                        clear_error(ERR_MAN_HW_INIT, ERR_ISM330_STATUS_RW);
                        manIsm330DLCData[2] = (uint8_t)data;
                    }
                    else
                    {
                        set_error(ERR_MAN_HW_INIT, ERR_ISM330_STATUS_RW);
                        return CO_E_RANGE;
                    }
                }
            }
            break;
        default: break;
    }
    return CO_OK;
}
#endif /* CONFIG_SDO_SERVER */


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
            if (updateData.block != CO_TRUE)
            {
                if (writeBufferToFlash() == CO_FALSE)
                {
                    return CO_E_HARDWARE_FAULT;
                }
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
            uint32_t status = mt25Write(downloadBuffer, updateData.address, FLASH_BUFFER_SIZE);
            if (status != HAL_OK)
            {
                return CO_E_HARDWARE_FAULT;
            }
        }
    }

    return (CO_OK);
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
    return (CO_OK);
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
    if ((index == DOWNLOAD_PROGRAM_DATA) && (subIndex == 1))
    {
        updateData.block = CO_TRUE;

        // Check if program falsh is necessary
        if (writeBufferToFlash() == CO_FALSE)
        {
            return CO_E_HARDWARE_FAULT;
        }
    }
    return (CO_OK);
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
        if (mt25Erase(0, size) != HAL_OK)
        {
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
        memset(manDeviceName, '\0', sizeof(manDeviceName));
    }
    if (index == 0x2009)
    {
        memset(manHWVersion, '\0', sizeof(manHWVersion));
    }
    if (index == 0x200A)
    {
        memset(manSWVersion, '\0', sizeof(manSWVersion));
    }

    return CO_OK;
}
#endif /* defined(CONFIG_SDO_SERVER) && defined(CONFIG_VALUE_CHECK_FUNCTION) */


#if defined(CONFIG_NODE_GUARDING)
/********************************************************************/
/**
 * \brief sGuardErrorInd - indicate the occurrence of a Node Guarding event
 *
 * This function defines the reaction on a Node Guarding event
 * on the local NMT slave.
 * The first missing Guarding message is accepted because the timer resolution.
 * An error event is occurred for the second missed Guarding message.
 *
 * Meaning of the parameter:
 *
 * \li CO_GUARDING_STARTED
 * \par
 * Node Guarding was (re)-started.
 *
 * \li  CO_LOST_GUARDING_MSG
 * \par
 * Guarding time is elapsed at least the second time.
 *
 * \li  CO_LOST_CONNECTION
 * \par
 * The lifetime (lifetime factor * guarding time) is elapsed.
 *
 * \retval 0
 * Node shall keep in the current state.
 * \retval 1
 * Node shall be forced to the state PRE_OPERATIONAL.
 */
UNSIGNED8 sGuardErrorInd(ERROR_SPEC_T kind           /**< kind of Node Guarding event */
                             CO_COMMA_LINE_PARA_DECL /**< number of CAN line 0..CO_MAX_CAN_LINES-1 */
)
{
    UNSIGNED8 u8Ret; /* return value */

    switch (kind)
    {
        case CO_GUARDING_STARTED: u8Ret = 0;
        case CO_LOST_GUARDING_MSG: u8Ret = 0;
        case CO_LOST_CONNECTION: u8Ret = 1;
        default: u8Ret = 0;
    }

    return (u8Ret);
}
#endif /* CONFIG_NODE_GUARDING */


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


#ifdef CONFIG_SYNC_PRE_CMD
/*******************************************************************/
/**
 * \brief syncPreCommand - actions after SYNC
 *
 * This function is called immediately after the SYNC was received,
 * before other actions, e.g. transmit and receice PDOs, are started.
 * The application can update data for PDOs or define own actions.
 *
 * The call of this indication function can be enabled by the
 * CANopen Design Tool about:
 * Line / Object Dictionary / Communication Segment / object 1005h /
 * tab Mask / General SYNC Settings /
 * Enable User Function immediately at SYNC Message
 *
 * \return
 * nothing
 */
void syncPreCommand(CO_LINE_PARA_DECL /**< number of CAN line 0..CO_MAX_CAN_LINES-1 */
)
{
    appSyncWatchdogOnSyncRx();
}
#endif /* CONFIG_SYNC_PRE_CMD */


#ifdef CONFIG_SYNC_CMD
/*******************************************************************/
/**
 * \brief syncCommand - actions after SYNC and updated services
 *
 * This function is called after the SYNC was received
 * and all activities of the Library, e.g. transmit PDOs, are done.
 * The application can execute own actions.
 *
 * The call of this indication function can be enabled by the
 * CANopen Design Tool about:
 * Line / Object Dictionary / Communication Segment / object 1005h /
 * tab Mask / General SYNC Settings /
 * Enable User Function after SYNC Message
 *
 * \return
 * nothing
 */
void syncCommand(CO_LINE_PARA_DECL /**< number of CAN line 0..CO_MAX_CAN_LINES-1 */
)
{
}
#endif /* CONFIG_SYNC_CMD */


#ifdef CONFIG_USER_TIMER_EVENT
/********************************************************************/
/**
 * userTimerEvent - user timer event occurred
 *
 * This function is called if a user-specific timer has been elapsed.
 * The parameter contains the pointer to the actual timer structure.
 *
 * The call of this indication function can be enabled by the
 * CANopen Design Tool about:
 * General Settings / Apply user-timer functionality
 *
 * \return
 * nothing
 */
void userTimerEvent(TIMER_EVENT_T *pTimer       /**< pointer at user timer */
                        CO_COMMA_LINE_PARA_DECL /**< number of CAN line 0..CO_MAX_CAN_LINES-1 */
)
{
    /* if (pTimer == &invalidTime)  { */
    /* } */
}
#endif /* CONFIG_USER_TIMER_EVENT */

/*______________________________________________________________________EOF_*/
