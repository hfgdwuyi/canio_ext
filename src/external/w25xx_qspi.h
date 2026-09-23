/*!
 * Copyright Siemens Healthcare GmbH 2022, All Rights Reserved
 *
 * Project: Building Block Low End MCU
 *
 * @file
 * @brief Header file for w25xx_qspi.c
 */
/*----------------------------------------------------------------------------*/
#ifndef W25QXX_H
#define W25QXX_H

#define W25_MEMORY_SECTOR_SIZE 4096
#define W25_WRITE_PAGE_SIZE    256 /*!< Write page size */

typedef enum
{
    W25_OK = 0,
    W25_NO_DEVICE,
    W25_COMMAND_ERROR,
    W25_RECEIVE_ERROR,
    W25_TRANSMIT_ERROR,
    W25_TIMEOUT,
    W25_MEMORY_MAP,
} w25Status;

typedef struct
{
    uint8_t  manId;
    uint16_t devId;
} w25Id;

w25Status w25InitQuad(void);
w25Status w25QReset(void);
w25Status w25QReadID(w25Id *id);
w25Status w25ReadStatusRegister(uint8_t *data);
w25Status w25WriteStatusRegister(uint8_t value);

w25Status w25Read(uint32_t address, uint8_t *rxBuf, uint32_t length);
w25Status w25Write(uint32_t address, uint8_t *txBuf, uint32_t length);
w25Status w25Erase(uint32_t address, uint32_t length);
w25Status w25EraseChip(void);
w25Status w25EnableMemoryMappedMode(void);

#endif    // W25QXX_H
