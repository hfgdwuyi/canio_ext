/*!
 * Copyright © Siemens Healthcare GmbH 2022, All Rights Reserved
 *
 * Project: Building Block Low End MCU
 *
 *  @file           at24xx.h
 *  @brief          Header file for at24xx.c
 */
/*----------------------------------------------------------------------------*/
#ifndef AT24XX_H
#define AT24XX_H

/*! Enumeration class for EEPROM chip capacity */
typedef enum
{
    KiBit_1   = 128,       ///<    128 bytes EEPROM capacity
    KiBit_2   = 256,       ///<    256 bytes EEPROM capacity
    KiBit_4   = 512,       ///<    512 bytes EEPROM capacity
    KiBit_8   = 1024,      ///<   1024 bytes EEPROM capacity
    KiBit_16  = 2048,      ///<   2048 bytes EEPROM capacity
    KiBit_32  = 4096,      ///<   4096 bytes EEPROM capacity
    KiBit_64  = 8192,      ///<   8192 bytes EEPROM capacity
    KiBit_128 = 16384,     ///<  16384 bytes EEPROM capacity
    KiBit_256 = 32768,     ///<  32768 bytes EEPROM capacity
    KiBit_512 = 65536,     ///<  65536 bytes EEPROM capacity
    MiBit_1   = 131072,    ///< 131072 bytes EEPROM capacity
    MiBit_2   = 262144     ///< 262144 bytes EEPROM capacity
} at24_Capacity;

/*! Enumeration class for EEPROM chip page size */
typedef enum
{
    Byte_8   = 8,      ///<   8 byte page size
    Byte_16  = 16,     ///<  16 byte page size
    Byte_32  = 32,     ///<  32 byte page size
    Byte_64  = 64,     ///<  64 byte page size
    Byte_128 = 128,    ///< 128 byte page size
    Byte_256 = 256     ///< 256 byte page size
} at24_PageSize;

/*! Typedef for at24 EERPOM instance */
typedef struct at24_EEPROM
{
    uint8_t       i2c;
    uint8_t       address;
    at24_Capacity capacity;
    at24_PageSize page_size;

} at24_EEPROM;

extern const at24_EEPROM eeprom;

uint32_t at24_Write(const at24_EEPROM *eepromInstance, uint32_t addr, const uint8_t *tx_buf, uint32_t len);
uint32_t at24_Read(const at24_EEPROM *eepromInstance, uint32_t addr, uint8_t *rx_buf, uint32_t len);

#endif    // AT24_EEPROM_H
