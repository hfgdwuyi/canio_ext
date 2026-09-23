/*!
 * Copyright Siemens Healthcare GmbH 2024, All Rights Reserved
 *
 * Project: Building Block Low End MCU
 *
 * @file
 * @brief Functions for selftest
 */
/*----------------------------------------------------------------------------*/
// Standard includes
#include <stdbool.h>
// HAL includes
#include <hal.h>
// Project includes
#include "mem_map.h"
#include "w25xx_qspi.h"
#include "crc_calc.h"
#include "selftest.h"
#include "bsp_wtdg.h"
#include "error.h"

#ifndef VDX8
/*! Error LED port */
#define ERROR_LED_PORT GPIOJ
/*! Error LED pin */
#define ERROR_LED_PIN GPIO_PIN_0
/*! Error LED port initialization */
#define ERROR_LED_PORT_INIT() __HAL_RCC_GPIOJ_CLK_ENABLE()


/*! Rescue switch port */
#define RESCUE_SWITCH_PORT GPIOB
/*! Rescue switch pin */
#define RESCUE_SWITCH_PIN GPIO_PIN_2
#else
/*! Rescue switch port */
#define RESCUE_SWITCH_PORT GPIOH
/*! Rescue switch pin */
#define RESCUE_SWITCH_PIN GPIO_PIN_15


/*! Error LED port */
#define ERROR_LED_PORT GPIOI
/*! Error LED pin */
#define ERROR_LED_PIN GPIO_PIN_2
/*! Error LED port initialization */
#define ERROR_LED_PORT_INIT() __HAL_RCC_GPIOI_CLK_ENABLE()
#endif
/*----------------------------------------------------------------------------*/
/*!
 @brief          Error signalization and switch to safe state.

 @return         Function will not return
*/
/*----------------------------------------------------------------------------*/
void selftestErrorState(void)
{
    // Init port
    ERROR_LED_PORT_INIT();
    // Init error LED
    HAL_GPIO_Init(ERROR_LED_PORT, &((GPIO_InitTypeDef){ ERROR_LED_PIN, GPIO_MODE_OUTPUT_PP, GPIO_PULLUP, GPIO_SPEED_FREQ_HIGH, 0 }));

    // Go to an endless loop with LED blinking
    for (;;)
    {
        for (uint32_t i = 0; i < 2000000; i++)
        {
            __NOP();
        }
        HAL_GPIO_TogglePin(ERROR_LED_PORT, ERROR_LED_PIN);
        WTDG_Feed();
    }
}

/*----------------------------------------------------------------------------*/
/*!
 @brief          Read rescue switch to check if bootloader must be started

 @return         true if bootloader need to be started, false otherwise
*/
/*----------------------------------------------------------------------------*/
bool selftestRescueSwitchRead(void)
{
    // Initialize rescue switch pin
    (void)HAL_GPIO_Init(RESCUE_SWITCH_PORT, &(GPIO_InitTypeDef){ RESCUE_SWITCH_PIN, GPIO_MODE_INPUT, GPIO_PULLUP, GPIO_SPEED_FREQ_LOW, 0 });
    // Read rescue switch state
    uint8_t state = (uint8_t)HAL_GPIO_ReadPin(RESCUE_SWITCH_PORT, RESCUE_SWITCH_PIN);
    if (state == 0)
    {
        return true;
    }

    return false;
}

/*----------------------------------------------------------------------------*/
/*!
 @brief      Check bootloader image consistency

    CRC32 of the bootloader's image is stored at the end of the image.
    After boot up the reference CRC is calculated over the
    bootloader's image. When the compared values doesn't fit, the
    infinite loop with error output is reached.

 @return         None
*/
/*----------------------------------------------------------------------------*/
__attribute__((constructor)) static void selftestFlashBoot(void)
{
    // Bootloader image size in flash
    const uint32_t *imageLength = (uint32_t *)(BOOT_FLASH_ADDRESS + IMAGE_SIZE_OFFSET);
    // CRC stored in flash
    const uint32_t *pStoredCRC = (uint32_t *)(BOOT_FLASH_ADDRESS + *imageLength);
    // Check size
    if (*imageLength > FLASH_FULL_SIZE)
    {
        selftestErrorState();
    }

    // Enable CRC module
    crcInit();

    // Calculate CRC32
    uint32_t crc = crcCalc32((void *)BOOT_FLASH_ADDRESS, *imageLength);
    WTDG_Feed();
    // Compare the values
    if (crc != *pStoredCRC)
    {
        selftestErrorState();
    }
}

/*----------------------------------------------------------------------------*/
/*!
 @brief      Check bootloader image consistency

    CRC32 of image is stored at the end of the image.

 @return         None
*/
/*----------------------------------------------------------------------------*/
bool selftestFlashApp(uint32_t address)
{
    uint32_t imageLength;
    uint32_t storedCRC;
    // Read image size from flash
    uint32_t status = w25Read(address + IMAGE_SIZE_OFFSET, (uint8_t *)&imageLength, sizeof(imageLength));
    if (status != W25_OK)
    {
        return false;
    }
    WTDG_Feed();
    // Check Image length
    if (imageLength > APP_MAX_SIZE || imageLength == 0)
    {
        set_error(ERR_MAN_APP_ERR, ERR_NO_APPLICATION);
        return false;
    }
    WTDG_Feed();
    // Read stored CRC from flash
    status = w25Read(address + imageLength, (uint8_t *)&storedCRC, sizeof(storedCRC));
    if (status != W25_OK)
    {
        return false;
    }
    WTDG_Feed();
    // Rest CRC calculation
    crcReset();
    uint8_t  buf[16];
    uint32_t index = 0;
    uint32_t crc   = 0;

    // Calculate CRC by portions of 16 bytes
    while (index < imageLength)
    {
        uint32_t amount = sizeof(buf) / sizeof(buf[0]);

        if (imageLength - index <= amount)
        {
            amount = imageLength - index;
        }
        status = w25Read(address + index, buf, amount);
        if (status != W25_OK)
        {
            return false;
        }
        crc = crcCalc32(buf, amount);
        index += amount;
        WTDG_Feed();
    }
    if (storedCRC == crc)
    {
        return true;
    }
    else
    {
        set_error(ERR_MAN_APP_ERR, ERR_APPLICATION_CRC);
        return false;
    }
}
