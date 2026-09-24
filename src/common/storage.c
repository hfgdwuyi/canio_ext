/*!
 * Copyright Siemens Healthcare GmbH 2021, All Rights Reserved
 *
 * Project: Building Block Low End MCU
 *
 * @file
 * @brief Non-volatile storage implementation
 *
 */
/*----------------------------------------------------------------------------*/
// Standard C includes
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
// HAL includes
#include "hal.h"
// CANOpen includes
#include <cal_conf.h>
#include <co_acces.h>
// Project includes
#include "at24xx.h"
#include "crc_calc.h"
#include "storage.h"
#include "bsp_wtdg.h"

#include "debug.h"
/* The size of every segment saved in EEPROM*/
#define STORAGE_SEGMENT_SIZE 0x800
/* Starting address in EEPROM to save the CAN objects*/
#define STORAGE_START_ADDRESS 0


// Static functions declaration
static bool storageWrite(uint8_t segment, const uint8_t *pData);
static bool storageRead(uint8_t segment, uint8_t *pData);


/* Buffer to write the data before saving*/
static uint8_t storageBuffer[STORAGE_SEGMENT_SIZE];

/*----------------------------------------------------------------------------*/
/*!
 @brief           Structure for storage segment description
*/
/*----------------------------------------------------------------------------*/
typedef struct
{
    uint16_t version;    //!< Version number
    uint16_t begin;      //!< Address of the first CANOpen object
    uint16_t end;        //!< Address of the last CANOpen object
} storageDataSegment;

/*----------------------------------------------------------------------------*/
/*!
 @brief           Header is written before data in EEPROM
*/
/*----------------------------------------------------------------------------*/
typedef struct
{
    uint16_t index;       ///<! CANOpen index of the variable to be stored
    uint8_t  subindex;    ///<! CANOpen subindex of the variable to be stored
    uint16_t size;        ///<! Variable size
    uint8_t  crc;         ///<! CRC8 of the data
} __attribute__((packed)) storageRecordHeader;

/*!
@}
*/

static const storageDataSegment segmentData[] = {
    { 0x00000002, START_COM_PROF, END_COM_PROF },          // Communication segment
    { 0x00000002, START_DEVICE_PROF, END_DEVICE_PROF },    // Device profile segment
    { 0x00000002, 0x2008, 0x2018 },                        // Asset data
    { 0x00000002, 0x2100, 0x2102 },                        // CAN settings data
};

/*! Number of segments in the storage */
const uint16_t storageSegmentsCount = sizeof(segmentData) / sizeof(segmentData[0]);
/*! Number of saved elements in each segment */
static uint16_t storageObjectsToFind[4] = { 0 };

/*----------------------------------------------------------------------------*/
/*!
 *  @brief          Initialization of one specific storage block
 *  @param[in]      segment number of segment to initialize
 *
 */
/*----------------------------------------------------------------------------*/
void storageSegmentInit(uint8_t segment)
{
    for (UNSIGNED16 index = segmentData[segment].begin; index <= segmentData[segment].end; index++)
    {
        UNSIGNED8 subNum = getNumOfElem(index CO_COMMA_LINE_PARA_DECL);

        for (UNSIGNED8 subindex = 0; subindex < subNum; subindex++)
        {
            if (getObjStoreEnableReq(index, subindex) == CO_OK)
            {
                storageObjectsToFind[segment]++;
            }
        }
    }
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief          Write data sement to non-volatile memory
 *
 * @param[in]      segment Data segment to write
 * @return         true - write succeed, false - otherwise
 */
/*----------------------------------------------------------------------------*/
bool storageSaveSegment(uint8_t segment)
{
    // Check if the segment is present
    if (segment >= storageSegmentsCount)
    {
        return false;
    }

    // Check if there is something to write
    if (storageObjectsToFind[segment] == 0)
    {
        // Nothing to write, it's OK
        return true;
    }

    // Initialize write buffer
    memset(storageBuffer, 0xFF, STORAGE_SEGMENT_SIZE);
    UNSIGNED32 address = 0;
    // Write version
    memcpy(storageBuffer, &segmentData[segment].version, sizeof(segmentData[segment].version));
    address += sizeof(segmentData[segment].version);


    // Write objects that are marked as "Save in non-volatile memory"
    for (UNSIGNED16 index = segmentData[segment].begin; index <= segmentData[segment].end; index++)
    {
        // Get number of subindexes
        UNSIGNED8 subNum = getNumOfElem(index CO_COMMA_LINE_PARA_DECL);
        // Walk through all subindexes
        for (UNSIGNED8 subindex = 0; subindex < subNum; subindex++)
        {
            // Check if the entry must be saved
            if (getObjStoreEnableReq(index, subindex) == CO_OK)
            {
                void      *data;
                UNSIGNED32 size;

                // Get value and data size
                RET_T ret = getObjAddr(index, subindex, (UNSIGNED8 **)&data, &size CO_COMMA_LINE_PARA);
                if (ret != CO_OK)
                {
                    return false;
                }

                storageRecordHeader header = { .index = index, .subindex = subindex, .size = (uint16_t)size, .crc = 0 };
                // Calculate data's crc
                crcReset();
                header.crc = crcCalc8(data, size);

                // Write header
                memcpy(&storageBuffer[address], &header, sizeof(header));
                address += sizeof(header);

                // Write data
                memcpy(&storageBuffer[address], data, size);
                address += size;

                DEBUGOUT(LOG_DEBUG, "Object 0x%04X.%d, crc 0x%02X, size %d \n", header.index, header.subindex, header.crc, header.size);
            }
            WTDG_Feed();
        }
    }
    // Write empty header at the end of the segment
    storageRecordHeader header = { .index = 0, .subindex = 0, .size = 0, .crc = 0 };
    memcpy(&storageBuffer[address], &header, sizeof(header));

    return storageWrite(segment, storageBuffer);
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief          Read data sement from non-volatile memory
 *
 * @param[in]      segment Data segment to read
 * @return         true - read succeed, false - otherwise
 */
/*----------------------------------------------------------------------------*/
bool storageLoadSegment(uint8_t segment)
{
    // Check if the segment is present
    if (segment >= storageSegmentsCount)
    {
        return false;
    }

    // Check if there is something to read
    if (storageObjectsToFind[segment] == 0)
    {
        // Nothing to read, it's OK
        return true;
    }

    // Read data
    if (!storageRead(segment, storageBuffer))
    {
        return false;
    }

    UNSIGNED32 address = 0;
    // Read version
    uint16_t version;
    memcpy(&version, storageBuffer, sizeof(version));
    if (version != segmentData[segment].version)
    {
        // TODO: handle wrong version
        return false;
    }
    address += sizeof(version);

    UNSIGNED16          recordsFound = 0;
    storageRecordHeader header       = { .index = 0, .subindex = 0, .size = 0, .crc = 0 };

    do
    {
        // Parse header
        memcpy(&header, &storageBuffer[address], sizeof(header));
        address += sizeof(header);

        // Check if the entry must be saved
        if (getObjStoreEnableReq(header.index, header.subindex) == CO_OK)
        {
            // Check CRC
            crcReset();
            uint8_t crc = crcCalc8(&storageBuffer[address], header.size);
            if (crc != header.crc)
            {
                DEBUGOUT(LOG_ERROR, "Wrong crc: 0x%04X.%d\n", header.index, header.subindex);
                return false;
            }

            // Copy data to the destination
            // Get object's pointer to data and size
            UNSIGNED8 *data;
            UNSIGNED32 size;

            RET_T ret = getObjAddr(header.index, header.subindex, &data, &size CO_COMMA_LINE_PARA);
            if (ret != CO_OK || size != header.size)
            {
                DEBUGOUT(LOG_ERROR, "Cannot get object address: 0x%04X.%d\n", header.index, header.subindex);
                return false;
            }
            // Copy data to the object
            memcpy(data, &storageBuffer[address], size);

            // Increment found counter
            recordsFound++;
        }

        // Always skip the payload. Records of objects that are no longer marked
        // for non-volatile storage (e.g. after a firmware update removed the
        // storage attribute) must not break parsing of the following records.
        address += header.size;
        WTDG_Feed();
    } while ((header.size != 0) && (address < STORAGE_SEGMENT_SIZE));

    return recordsFound == storageObjectsToFind[segment];
}

/*----------------------------------------------------------------------------*/
/*!
 *@brief          Erase storage segment
 *
 * @param[in]      segment Data segment to erase
 * @return         true - erase succeed, false - otherwise
 */
/*----------------------------------------------------------------------------*/
bool storageEraseSegment(uint8_t segment)
{
    // Check if the segment is present
    if (segment >= storageSegmentsCount)
    {
        return false;
    }
    // Buffer for erase
    static uint8_t eraseBuffer[64] = { [0 ... 63] = 0xFF };
    uint32_t       amount          = sizeof(eraseBuffer) / sizeof(eraseBuffer[0]);
    // Set address to write to
    uint32_t address = STORAGE_START_ADDRESS + STORAGE_SEGMENT_SIZE * segment;
    // Erase data
    uint32_t bytesErased = 0;
    while (bytesErased < STORAGE_SEGMENT_SIZE)
    {
        if (bytesErased + amount > STORAGE_SEGMENT_SIZE)
        {
            amount = (bytesErased + amount) - STORAGE_SEGMENT_SIZE;
        }
        uint32_t bytesWritten = at24_Write(&eeprom, address + bytesErased, eraseBuffer, amount);
        if (bytesWritten != amount)
        {
            return false;
        }
        bytesErased += amount;
    }

    return true;
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief          Function that really writes data to a non-volatile memory
 *
 * @param[in]      segment Data segment to write
 * @param[in]      pData Pointer to data
 * @return         true - write succeed, false - otherwise
 */
/*----------------------------------------------------------------------------*/
static bool storageWrite(uint8_t segment, const uint8_t *pData)
{
    // Set address to write to
    uint32_t address = STORAGE_START_ADDRESS + STORAGE_SEGMENT_SIZE * segment;
    // Write data
    uint32_t bytesWritten = at24_Write(&eeprom, address, pData, STORAGE_SEGMENT_SIZE);

    return bytesWritten == STORAGE_SEGMENT_SIZE;
}

/*----------------------------------------------------------------------------*/
/*!
 @brief          Function that really reads data from a non-volatile memory

 @param[in]      segment Data segment to read
 @param[out]     pData Pointer to buffer
 @return         true - read succeed, false - otherwise
*/
/*----------------------------------------------------------------------------*/
static bool storageRead(uint8_t segment, uint8_t *pData)
{
    // Set address to read from
    uint32_t address = STORAGE_START_ADDRESS + STORAGE_SEGMENT_SIZE * segment;
    // Read data
    uint32_t bytesRead = at24_Read(&eeprom, address, pData, STORAGE_SEGMENT_SIZE);

    return bytesRead == STORAGE_SEGMENT_SIZE;
}
