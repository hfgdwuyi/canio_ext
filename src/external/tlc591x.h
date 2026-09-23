/*!
 * Copyright Siemens Healthcare GmbH 2022, All Rights Reserved
 *
 * Project: Building Block Low End MCU
 *
 *  @file
 *  @brief          Header file for tlc591x.c
 */
/*----------------------------------------------------------------------------*/
#ifndef TLC591X_H
#define TLC591X_H

/*! Return codes */
typedef enum
{
    RET_TLC591X_OK = 0,
    RET_TLC591X_ESPI,
} tlc591xRet;

typedef struct
{
    uint8_t spi;
    pinCfg  le;
    pinCfg  oe;
} tlc591xInstance;

void       tlc591xInit(tlc591xInstance inst);
tlc591xRet tlc591xSetValue(tlc591xInstance inst, uint8_t value);
tlc591xRet tlc591xSetValues(tlc591xInstance inst, const uint8_t *values, uint16_t size);

#endif    // TLC591X_H