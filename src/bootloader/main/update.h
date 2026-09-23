/*!
 * Copyright Siemens Healthcare GmbH 2024, All Rights Reserved
 *
 * Project: Building Block Low End MCU
 *
 * @file
 * @brief Header file for update.c
 */
/*----------------------------------------------------------------------------*/
#ifndef UPDATE_H
#define UPDATE_H

typedef enum
{
    MEMO_RAM = 0,
    MEMO_FLASH,
    MEMO_EEPROM
} memoryType;

typedef struct memoStore
{
    memoryType memoType;
    uint32_t   address;
    uint32_t   size;
} memoStore;

extern uint8_t file_buffer[];
extern bool    isUSBCommand;

bool checkAppValidity(bool memType, uint32_t address);
bool checkRamValidity(uint32_t address, uint32_t size);
bool memoryWrite(memoStore memData);
bool eraseMemory(char **argv);
void memoryMap(void);
void memoryRead(memoStore memData);
void memoryCalcCRC(memoStore memData);
bool copyApplication(uint32_t startAddress);
void startApplication(uint32_t addr);

#endif    // CAN_CFG_H