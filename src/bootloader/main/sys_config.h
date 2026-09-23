/*!
 * Copyright Siemens Healthcare GmbH 2022, All Rights Reserved
 *
 * Project: Building Block Low End MCU
 *
 * @file
 * @brief System configuration defines
 */
/*----------------------------------------------------------------------------*/
#ifndef SYS_CONFIG_H
#define SYS_CONFIG_H

#define SPI5_CS_PORT   GPIOH
#define SPI5_CS_PIN    GPIO_PIN_5
#define SPI6_CS_PORT   GPIOG
#define SPI6_CS_PIN    GPIO_PIN_8


#define SERIAL_UART    USART3
#define SERIAL_IRQ_NUM USART3_IRQn

#define SPI5_INIT                                                                                                                    \
    {                                                                                                                                \
        .BaudRatePrescaler = SPI_BAUDRATEPRESCALER_64, .Direction = SPI_DIRECTION_2LINES, .CLKPhase = SPI_PHASE_1EDGE,               \
        .CLKPolarity = SPI_POLARITY_LOW, .DataSize = SPI_DATASIZE_16BIT, .FirstBit = SPI_FIRSTBIT_MSB, .TIMode = SPI_TIMODE_DISABLE, \
        .CRCCalculation = SPI_CRCCALCULATION_DISABLE, .CRCPolynomial = 7, .NSS = SPI_NSS_SOFT, .Mode = SPI_MODE_MASTER,              \
    }

#define SPI6_INIT                                                                                                                   \
    {                                                                                                                               \
        .BaudRatePrescaler = SPI_BAUDRATEPRESCALER_256, .Direction = SPI_DIRECTION_2LINES, .CLKPhase = SPI_PHASE_1EDGE,             \
        .CLKPolarity = SPI_POLARITY_LOW, .DataSize = SPI_DATASIZE_8BIT, .FirstBit = SPI_FIRSTBIT_MSB, .TIMode = SPI_TIMODE_DISABLE, \
        .CRCCalculation = SPI_CRCCALCULATION_DISABLE, .CRCPolynomial = 7, .NSS = SPI_NSS_SOFT, .Mode = SPI_MODE_MASTER,             \
    }

#define MT25TL01G_FLASH_SIZE 0x800000 /* 64MBytes */
#define USB_FS_ENABLED       1
#define QSPI_ENABLE

#endif
