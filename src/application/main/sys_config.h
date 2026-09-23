/*!
 * Copyright � Siemens Healthcare GmbH 2023, All Rights Reserved
 *
 * Project: Building Block Low End MCU
 *
 * @file
 * @brief System configuration defines
 */
/*----------------------------------------------------------------------------*/
#ifndef SYS_CONFIG_H
#define SYS_CONFIG_H

/*! TMP75 sensor read period in ms */
#define SYS_TMP75_PERIOD_MS 300

/*! ISM330 sensor read period in ms */
#define SYS_ISM300_PERIOD_MS 1

#define SPI5_CS_PORT         GPIOH
#define SPI5_CS_PIN          GPIO_PIN_5
#define SPI6_CS1_PORT        GPIOG
#define SPI6_CS1_PIN         GPIO_PIN_8
#define SPI6_CS2_PORT        GPIOA
#define SPI6_CS2_PIN         GPIO_PIN_7
#define SPI6_CS_PORT         SPI6_CS1_PORT
#define SPI6_CS_PIN          SPI6_CS1_PIN


#define SERIAL_UART          USART3
#define SERIAL_IRQ_NUM       USART3_IRQn

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

#endif
