/*!
 * Copyright Siemens Healthcare GmbH 2022, All Rights Reserved
 *
 * Project: Building Block Low End MCU
 *
 * @file
 * @brief Pins and interfaces initialization
 */
/*----------------------------------------------------------------------------*/
#ifndef BSP_BOARD_H
#define BSP_BOARD_H

#include <hal.h>
#include <stdbool.h>
#include "tlc591x.h"

typedef struct
{
    uint8_t  spi;
    void    *wrBuf;
    void    *rdBuf;
    uint16_t size;
    uint32_t timeout;
    bool     skipCs;
} boardSpiXfer;

#define SYS_I2C_TMP_ADDRESS 0x48    //!< Temperature sensor's address


extern tlc591xInstance tlcLedDriverL;
extern tlc591xInstance tlcLedDriverR;

void    boardInit(void);
int32_t boardSerialSend(char c);
#ifdef BOOTLOADER
int32_t boardSerialReceive(char *buf);
#else
int32_t boardSerialReceive(void);
#endif
bool boardI2CTransfer(uint8_t i2cNum, uint16_t devAddr, uint8_t *wrBuf, uint16_t wrSize, uint8_t *rdBuf, uint16_t rdSize);
bool boardSpiTransfer(boardSpiXfer *xfer);
void boardWtdgInit(void);

#endif    // BSP_BOARD_H
