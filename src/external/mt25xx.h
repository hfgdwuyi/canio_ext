/*!
 * Copyright © Siemens Healthcare GmbH 2022, All Rights Reserved
 *
 * Project: Building Block Low End MCU
 *
 * @file
 * @brief Header file for mt25xx.c
 */
/*----------------------------------------------------------------------------*/
#ifndef MT25XX_H
#define MT25XX_H


#define MT25XX_ERASE_PAGE_SIZE 8192
#define MT25XX_WRITE_PAGE_SIZE 256


// Status Register
#define MT25XX_SR_WIP  ((uint8_t)0x01) /*!< Write in progress */
#define MT25XX_SR_WREN ((uint8_t)0x02) /*!< Write enable latch */

uint32_t mt25xxInit(void);
uint32_t mt25Read(uint8_t *pData, uint32_t readAddr, uint32_t size);
uint32_t mt25Erase(uint32_t addr, uint32_t size);
uint32_t mt25Write(uint8_t *pData, uint32_t addr, uint32_t size);
uint32_t mt25EnableMemoryMappedMode(void);
uint32_t mt25DisableMemoryMappedMode(void);
uint32_t mt25ReadVolatileCfgReg(uint8_t *reg);

#endif
