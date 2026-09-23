/*!
 * Copyright © Siemens Healthcare GmbH 2022, All Rights Reserved
 *
 * Project: Building Block Low End MCU
 *
 * @file
 * @brief    Header file for bsp_dio.c
 */
/*----------------------------------------------------------------------------*/
#ifndef BSP_DIO_H
#define BSP_DIO_H

typedef struct
{
    bool    deb_en;      //<! Debouncing enabled
    uint8_t deb_time;    //<! Debouncing time
} bspDinSettings;

extern const uint8_t dinMax;
extern const uint8_t doutMax;

void    bspDioInit(void);
void    bspDinSetDebouncing(uint8_t pin, bspDinSettings settings);
uint8_t bspDinRead(uint8_t byte);
void    bspDoutSet(uint8_t pinNumber, bool state);
bool    bspDoutRead(uint8_t pinNumber);

#endif

//--------------------------------- End Of File -------------------------------/
