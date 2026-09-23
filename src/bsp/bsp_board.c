/*!
 * Copyright © Siemens Healthcare GmbH 2023, All Rights Reserved
 *
 * Project: Building Block Low End MCU
 *
 * @file
 * @brief Pins and interfaces initialization
 */
/*----------------------------------------------------------------------------*/
// Standeard includes
#include <stdio.h>
// HAL includes
#include <hal.h>
// CANOpen includes
#include <objects.h>
// Project includes
#include "sys_config.h"
#include "error.h"

#include "timing.h"
#include "at24xx.h"
#include "bsp_led.h"
#ifndef BOOTLOADER
#include "message_queue.h"
#endif
#include "bsp_board.h"

#ifndef SPI6_CS1_PORT
#define SPI6_CS1_PORT SPI6_CS_PORT
#endif

#ifndef SPI6_CS1_PIN
#define SPI6_CS1_PIN SPI6_CS_PIN
#endif


#define BOARD_SERIAL_BUFFER_SIZE 512

/*! Pins' initialization table */
static const pinCfg boardPins[] = {
    // UART pins
    { GPIOB, { GPIO_PIN_10, GPIO_MODE_AF_PP, GPIO_PULLUP, GPIO_SPEED_FREQ_LOW, GPIO_AF7_USART3 } },       // USART3 Tx
    { GPIOB, { GPIO_PIN_11, GPIO_MODE_AF_PP, GPIO_PULLUP, GPIO_SPEED_FREQ_LOW, GPIO_AF7_USART3 } },       // USART3 Rx
                                                                                                          // CAN pins
    { GPIOH, { GPIO_PIN_13, GPIO_MODE_AF_PP, GPIO_PULLUP, GPIO_SPEED_FREQ_MEDIUM, GPIO_AF9_FDCAN1 } },    // FDCAN1_TX
    { GPIOH, { GPIO_PIN_14, GPIO_MODE_AF_PP, GPIO_PULLUP, GPIO_SPEED_FREQ_MEDIUM, GPIO_AF9_FDCAN1 } },    // FDCAN1_RX

    { GPIOB, { GPIO_PIN_13, GPIO_MODE_AF_PP, GPIO_PULLUP, GPIO_SPEED_FREQ_MEDIUM, GPIO_AF9_FDCAN2 } },    // FDCAN2_TX
    { GPIOB, { GPIO_PIN_5, GPIO_MODE_AF_PP, GPIO_PULLUP, GPIO_SPEED_FREQ_MEDIUM, GPIO_AF9_FDCAN2 } },     // FDCAN2_RX
                                                                                                          // I2C1 pins
    { GPIOB, { GPIO_PIN_8, GPIO_MODE_AF_OD, GPIO_PULLUP, GPIO_SPEED_FREQ_LOW, GPIO_AF4_I2C1 } },          // I2C1_SCL
    { GPIOB, { GPIO_PIN_9, GPIO_MODE_AF_OD, GPIO_PULLUP, GPIO_SPEED_FREQ_LOW, GPIO_AF4_I2C1 } },          // I2C1_SDA

    // SPI pins
    { GPIOH, { GPIO_PIN_6, GPIO_MODE_AF_PP, GPIO_NOPULL, GPIO_SPEED_FREQ_HIGH, GPIO_AF5_SPI5 } },     // SPI5_SCK
    { GPIOH, { GPIO_PIN_7, GPIO_MODE_AF_PP, GPIO_NOPULL, GPIO_SPEED_FREQ_HIGH, GPIO_AF5_SPI5 } },     // SPI5_MISO
    { GPIOF, { GPIO_PIN_11, GPIO_MODE_AF_PP, GPIO_NOPULL, GPIO_SPEED_FREQ_HIGH, GPIO_AF5_SPI5 } },    // SPI5_MOSI
    { SPI5_CS_PORT, { SPI5_CS_PIN, GPIO_MODE_OUTPUT_PP, GPIO_NOPULL, GPIO_SPEED_FREQ_LOW, 0 } },      // SPI5_CS

    { GPIOA, { GPIO_PIN_5, GPIO_MODE_AF_PP, GPIO_PULLDOWN, GPIO_SPEED_FREQ_HIGH, GPIO_AF8_SPI6 } },     // SPI6_SCK
    { GPIOA, { GPIO_PIN_6, GPIO_MODE_AF_PP, GPIO_PULLDOWN, GPIO_SPEED_FREQ_HIGH, GPIO_AF8_SPI6 } },     // SPI6_MISO
    { GPIOG, { GPIO_PIN_14, GPIO_MODE_AF_PP, GPIO_PULLDOWN, GPIO_SPEED_FREQ_HIGH, GPIO_AF5_SPI6 } },    // SPI6_MOSI
    { SPI6_CS1_PORT, { SPI6_CS1_PIN, GPIO_MODE_OUTPUT_PP, GPIO_NOPULL, GPIO_SPEED_FREQ_LOW, 0 } },      // SPI6_CS1

    { GPIOG, { GPIO_PIN_9, GPIO_MODE_IT_RISING, GPIO_PULLDOWN, GPIO_SPEED_FREQ_HIGH, 0} },    // Accelerometer INT1
    { GPIOG, { GPIO_PIN_10, GPIO_MODE_IT_RISING, GPIO_PULLDOWN, GPIO_SPEED_FREQ_HIGH, 0} },    // Accelerometer INT2

#ifdef QSPI_ENABLE
    // QSPI pins
    { GPIOF, { GPIO_PIN_10, GPIO_MODE_AF_PP, GPIO_NOPULL, GPIO_SPEED_FREQ_VERY_HIGH, GPIO_AF9_QUADSPI } },    // QSPI1_CLK
    { GPIOG, { GPIO_PIN_6, GPIO_MODE_AF_PP, GPIO_NOPULL, GPIO_SPEED_FREQ_VERY_HIGH, GPIO_AF10_QUADSPI } },    // QSPI1_nCS
    { GPIOD, { GPIO_PIN_11, GPIO_MODE_AF_PP, GPIO_NOPULL, GPIO_SPEED_FREQ_VERY_HIGH, GPIO_AF9_QUADSPI } },    // QSPI1_IO0
    { GPIOF, { GPIO_PIN_9, GPIO_MODE_AF_PP, GPIO_NOPULL, GPIO_SPEED_FREQ_VERY_HIGH, GPIO_AF10_QUADSPI } },    // QSPI1_IO1
    { GPIOF, { GPIO_PIN_7, GPIO_MODE_AF_PP, GPIO_NOPULL, GPIO_SPEED_FREQ_VERY_HIGH, GPIO_AF9_QUADSPI } },     // QSPI1_IO2
    { GPIOF, { GPIO_PIN_6, GPIO_MODE_AF_PP, GPIO_NOPULL, GPIO_SPEED_FREQ_VERY_HIGH, GPIO_AF9_QUADSPI } },     // QSPI1_IO3
#endif
    // Comp pins
    { GPIOB, { GPIO_PIN_1, GPIO_MODE_ANALOG, GPIO_NOPULL, GPIO_SPEED_FREQ_HIGH, 0 } },    // COMP1_INL
#ifdef USB_FS_ENABLED
    // USB pins
    { GPIOA, { GPIO_PIN_9, GPIO_MODE_INPUT, GPIO_NOPULL, GPIO_SPEED_FREQ_LOW, 0 } },                           // USB_VBUS
    { GPIOA, { GPIO_PIN_11, GPIO_MODE_AF_PP, GPIO_NOPULL, GPIO_SPEED_FREQ_VERY_HIGH, GPIO_AF10_OTG2_FS } },    // USB_DM
    { GPIOA, { GPIO_PIN_12, GPIO_MODE_AF_PP, GPIO_NOPULL, GPIO_SPEED_FREQ_VERY_HIGH, GPIO_AF10_OTG2_FS } },    // USB_DP
#endif
};

/* UART default init values*/
UART_HandleTypeDef serialUart = {
    .Instance          = SERIAL_UART,
    .Init.BaudRate     = 115200,
    .Init.Mode         = UART_MODE_TX_RX,
    .Init.Parity       = UART_PARITY_NONE,
    .Init.WordLength   = UART_WORDLENGTH_8B,
    .Init.StopBits     = UART_STOPBITS_1,
    .Init.HwFlowCtl    = UART_HWCONTROL_NONE,
    .Init.OverSampling = UART_OVERSAMPLING_16,
};
// Peripheral handler declaration
QSPI_HandleTypeDef hqspi  = { .Instance = QUADSPI };
I2C_HandleTypeDef  hi2c1  = { .Instance = I2C1 };
SPI_HandleTypeDef  hspi5  = { .Instance = SPI5 };
SPI_HandleTypeDef  hspi6  = { .Instance = SPI6 };
COMP_HandleTypeDef hcomp1 = { .Instance = COMP1 };
IWDG_HandleTypeDef hiwdg1 = { .Instance = IWDG1 };

// Static functions declaration
static void boardSerialInit(void);
static void boardCANInit(void);
static void boardI2CInit(void);
static void boardSpiInit(void);
static void boardCompInit(void);
#ifdef QSPI_ENABLE
static void boardQSPIInit(void);
#endif
#ifdef USB_FS_ENABLED
PCD_HandleTypeDef hpcd;
static void       boardUSBInit(void);
#endif

#ifndef BOOTLOADER
static uint8_t      boardSerialRxBuffer[BOARD_SERIAL_BUFFER_SIZE];
static messageQueue boardMqSerialRx = {
    .elSize = sizeof(uint8_t),
    .elNum  = BOARD_SERIAL_BUFFER_SIZE,
    .data   = boardSerialRxBuffer,
};
static uint8_t boardSerialChar;
#endif


static struct
{
    SPI_HandleTypeDef *handler;
    GPIO_TypeDef      *csPort;
    uint32_t           csPin;
    SPI_InitTypeDef    init;
} spiSettings[] = {
    { NULL }, { NULL }, { NULL }, { NULL }, { NULL }, { &hspi5, SPI5_CS_PORT, SPI5_CS_PIN, SPI5_INIT }, { &hspi6, SPI6_CS1_PORT, SPI6_CS1_PIN, SPI6_INIT },
};

/*----------------------------------------------------------------------------*/
/*!
 * @brief   Pins initialization
 *
 */
/*----------------------------------------------------------------------------*/
void boardInit(void)
{
    // Enable GPIO Clock (to be able to program the configuration registers)
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOD_CLK_ENABLE();
    __HAL_RCC_GPIOE_CLK_ENABLE();
    __HAL_RCC_GPIOF_CLK_ENABLE();
    __HAL_RCC_GPIOG_CLK_ENABLE();
    __HAL_RCC_GPIOH_CLK_ENABLE();
    __HAL_RCC_GPIOI_CLK_ENABLE();
    __HAL_RCC_GPIOJ_CLK_ENABLE();
    __HAL_RCC_GPIOK_CLK_ENABLE();

    HAL_NVIC_EnableIRQ(EXTI9_5_IRQn);
    HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);

    for (uint8_t i = 0; i < sizeof(boardPins) / sizeof(boardPins[0]); i++)
    {
        GPIO_InitTypeDef pin = boardPins[i].pin;
        HAL_GPIO_Init(boardPins[i].port, &pin);
    }
    // Init serial
    boardSerialInit();
    // Init LEDs
    ledInit();
    // Init CAN
    boardCANInit();
#ifdef QSPI_ENABLE
    // Init QSPI
    boardQSPIInit();
#endif
    // Init I2C
    boardI2CInit();

    // Init SPI
    boardSpiInit();

    // Init Comp
    boardCompInit();
    // Init USB
#ifdef USB_FS_ENABLED
    boardUSBInit();
#endif
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief   UART for serial interface initialization
 *
 */
/*----------------------------------------------------------------------------*/
static void boardSerialInit(void)
{
    // Init UART
    __HAL_RCC_USART3_CLK_ENABLE();
    if (HAL_UART_Init(&serialUart) != HAL_OK)
    {
        set_error(ERR_MAN_HW_INIT, ERR_INIT_UART);
    }

    HAL_NVIC_SetPriority(SERIAL_IRQ_NUM, 3, 0);
#ifndef BOOTLOADER
    HAL_NVIC_EnableIRQ(SERIAL_IRQ_NUM);

    (void)HAL_UART_Receive_IT(&serialUart, &boardSerialChar, 1);
#endif
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief     Output a character to serial
 *
 * @param[in] c - symbol to output
 * @return    EOF if output failed, output character otherwise
 */
/*----------------------------------------------------------------------------*/
int32_t boardSerialSend(char c)
{
    if (HAL_UART_Transmit(&serialUart, (uint8_t *)&c, 1, 2) != HAL_OK)
    {
        return EOF;
    }

    return c;
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief  Receive character from file stream
 *
 * @return Received symbol, or EOF if nothing received
 */
/*----------------------------------------------------------------------------*/
#ifndef BOOTLOADER
int32_t boardSerialReceive(void)
{
    int32_t c = EOF;
    uint8_t tmp;
    if (msgQueuePop(&boardMqSerialRx, &tmp) == MSGQ_OK)
    {
        c = (int32_t)tmp;
    }

    return c;
}
#else
int32_t boardSerialReceive(char *buf)
{
    buf[0] = EOF;
    if (__HAL_USART_GET_FLAG(&serialUart, USART_FLAG_RXFNE))
    {
        buf[0] = (uint8_t)(READ_BIT(serialUart.Instance->RDR, USART_RDR_RDR) & 0xFFU);
    }
    return buf[0] == EOF ? 0 : 1;
}
#endif

/*----------------------------------------------------------------------------*/
/*!
 * @brief  Init CAN subsystem
 *
 */
/*----------------------------------------------------------------------------*/
static void boardCANInit(void)
{
    RCC_PeriphCLKInitTypeDef RCC_PeriphClkInit;
    // Select PLL1Q as source of FDCANx clock
    RCC_PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_FDCAN;
    RCC_PeriphClkInit.FdcanClockSelection  = RCC_FDCANCLKSOURCE_PLL;
    HAL_RCCEx_PeriphCLKConfig(&RCC_PeriphClkInit);

    // Enable FDCANx clock
    __HAL_RCC_FDCAN_CLK_ENABLE();
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief Init I2C subsystem
 *
 */
/*----------------------------------------------------------------------------*/
static void boardI2CInit(void)
{
    // Enable I2C1 Clock
    __HAL_RCC_I2C1_CLK_ENABLE();
    // Force the I2C1 peripheral clock reset
    __HAL_RCC_I2C1_FORCE_RESET();
    // Release the I2C1 peripheral clock reset
    __HAL_RCC_I2C1_RELEASE_RESET();


    // I2C init struct
    hi2c1.Init = (I2C_InitTypeDef){
        .Timing           = 0x70B03839,    // The value calculated in CubeMX based on current timings
        .OwnAddress1      = 0,
        .AddressingMode   = I2C_ADDRESSINGMODE_7BIT,
        .DualAddressMode  = I2C_DUALADDRESS_DISABLE,
        .OwnAddress2      = 0,
        .OwnAddress2Masks = I2C_OA2_NOMASK,
        .GeneralCallMode  = I2C_GENERALCALL_DISABLE,
        .NoStretchMode    = I2C_NOSTRETCH_DISABLE,
    };


    if (HAL_I2C_Init(&hi2c1) != HAL_OK)
    {
        set_error(ERR_MAN_HW_INIT, ERR_INIT_I2C);
    }

    uint32_t analog_filter = I2C_ANALOGFILTER_ENABLE;
    if (HAL_I2CEx_ConfigAnalogFilter(&hi2c1, analog_filter) != HAL_OK)
    {
        set_error(ERR_MAN_HW_INIT, ERR_I2C_AF_CONFIG);
    }
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief      This function performs the transmit/receive operations through I2C interface
 *
 * @param[in] i2cNum number of I2C intarface used
 * @param[in] devAddr peripheral address to connect with
 * @param[in] wrBuf pointer to write data buffer
 * @param[in] wrSize size of write data buffer
 * @param[in] rdBuf pointer to read data buffer
 * @param[in] rdSize size of read data buffer
 * @return     true - if oparation succeded, false - otherwise
 */
/*----------------------------------------------------------------------------*/
bool boardI2CTransfer(uint8_t i2cNum, uint16_t devAddr, uint8_t *wrBuf, uint16_t wrSize, uint8_t *rdBuf, uint16_t rdSize)
{
    I2C_HandleTypeDef *hi2c = NULL;
    if (i2cNum == 1)
    {
        hi2c = &hi2c1;
    }
    else
    {
        return false;
    }
    // Write data
    HAL_StatusTypeDef ret;
    if ((wrBuf != NULL) && (wrSize != 0))
    {
        ret = HAL_I2C_Master_Transmit(hi2c, devAddr, wrBuf, wrSize, 2000);
        if (ret != HAL_OK)
        {
            return false;
        }
    }

    // Read data
    if ((rdBuf != NULL) && (rdSize != 0))
    {
        ret = HAL_I2C_Master_Receive(hi2c, devAddr, rdBuf, rdSize, 2000);
        if (ret != HAL_OK)
        {
            return false;
        }
    }

    return true;
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief       Clock initialization for SPI
 *
 * @param[in]   inst - pointer to SPI instance
 *
 */
/*----------------------------------------------------------------------------*/
static void boardSPIPowerEn(SPI_TypeDef const *inst)
{
    if (inst == SPI1)
    {
        __HAL_RCC_SPI1_CLK_ENABLE();
    }
    else if (inst == SPI2)
    {
        __HAL_RCC_SPI2_CLK_ENABLE();
    }
    else if (inst == SPI3)
    {
        __HAL_RCC_SPI3_CLK_ENABLE();
    }
    else if (inst == SPI4)
    {
        __HAL_RCC_SPI4_CLK_ENABLE();
    }
    else if (inst == SPI5)
    {
        __HAL_RCC_SPI5_CLK_ENABLE();
    }
    else if (inst == SPI6)
    {
        __HAL_RCC_SPI6_CLK_ENABLE();
    }
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief          Init SPIsubsystem
 *
 */
/*----------------------------------------------------------------------------*/
static void boardSpiInit(void)
{
    for (uint32_t i = 0; i < sizeof(spiSettings) / sizeof(spiSettings[0]); i++)
    {
        if (spiSettings[i].handler == NULL)
        {
            continue;
        }
        // Enable SPI clock
        boardSPIPowerEn(spiSettings[i].handler->Instance);
        // Initialize SPI
        spiSettings[i].handler->Init = spiSettings[i].init;
        if (HAL_SPI_Init(spiSettings[i].handler) != HAL_OK)
        {
            set_error(ERR_MAN_HW_INIT, ERR_INIT_SPI);
        }
    }
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief         SPI data transfer
 *
 * @param         xfer pointer to SPI data transfer structure
 * @return        true, if data were transmitted successfully, false otherwise
 */
/*----------------------------------------------------------------------------*/
bool boardSpiTransfer(boardSpiXfer *xfer)
{
    // Sanity chack
    if (xfer->spi >= sizeof(spiSettings) / sizeof(spiSettings[0]))
    {
        return false;
    }

    // Check if SPI available
    if (spiSettings[xfer->spi].handler == NULL)
    {
        return false;
    }

    if (xfer->skipCs == false)
    {
        HAL_GPIO_WritePin(spiSettings[xfer->spi].csPort, (uint16_t)spiSettings[xfer->spi].csPin, GPIO_PIN_RESET);
    }

    // Transfer data
    HAL_StatusTypeDef status = HAL_SPI_TransmitReceive(spiSettings[xfer->spi].handler, xfer->wrBuf, xfer->rdBuf, xfer->size, xfer->timeout);

    if (xfer->skipCs == false)
    {
        HAL_GPIO_WritePin(spiSettings[xfer->spi].csPort, (uint16_t)spiSettings[xfer->spi].csPin, GPIO_PIN_SET);
    }

    // Check status
    if (status != HAL_OK)
    {
        return false;
    }

    return true;
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief          Init Comparator subsystem
 *
 */
/*----------------------------------------------------------------------------*/
static void boardCompInit(void)
{
    // Enable Comp clock
    __HAL_RCC_COMP12_CLK_ENABLE();

    // Comp init struct
    hcomp1.Init = (COMP_InitTypeDef){
        .InvertingInput    = COMP_INPUT_MINUS_IO1,
        .NonInvertingInput = COMP_INPUT_PLUS_IO2,
        .OutputPol         = COMP_OUTPUTPOL_NONINVERTED,
        .Hysteresis        = COMP_HYSTERESIS_NONE,
        .BlankingSrce      = COMP_BLANKINGSRC_NONE,
        .Mode              = COMP_POWERMODE_HIGHSPEED,
        .WindowMode        = COMP_WINDOWMODE_DISABLE,
        .TriggerMode       = COMP_TRIGGERMODE_NONE,
    };
    (void)HAL_COMP_Init(&hcomp1);
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief          Init QSPIsubsystem
 *
 */
/*----------------------------------------------------------------------------*/
#ifdef QSPI_ENABLE
static void boardQSPIInit(void)
{
    // QSPI initialization values
    // ClockPrescaler set to 0, so QSPI clock = 64MHz / (1+1) = 32MHz
    QSPI_InitTypeDef init = {
        .ClockPrescaler     = 4,
        .FifoThreshold      = 1,
        .SampleShifting     = QSPI_SAMPLE_SHIFTING_NONE,
        .FlashSize          = POSITION_VAL(MT25TL01G_FLASH_SIZE) - 1,
        .ChipSelectHighTime = QSPI_CS_HIGH_TIME_4_CYCLE,
        .ClockMode          = QSPI_CLOCK_MODE_0,
        .FlashID            = QSPI_FLASH_ID_1,
        .DualFlash          = QSPI_DUALFLASH_DISABLE,
    };
    // Enable clock
    __HAL_RCC_QSPI_CLK_ENABLE();

    // Reset the QuadSPI memory interface
    __HAL_RCC_QSPI_FORCE_RESET();
    __HAL_RCC_QSPI_RELEASE_RESET();

    // QSPI initialization
    hqspi.Init = init;

    if (HAL_QSPI_Init(&hqspi) != HAL_OK)
    {
        set_error(ERR_MAN_HW_INIT, ERR_INIT_QSPI);
    }
}
#endif

#ifdef USB_FS_ENABLED
/*----------------------------------------------------------------------------*/
/*!
 * @brief          Init USB peripheral module
 *
 */
/*----------------------------------------------------------------------------*/
static void boardUSBInit(void)
{
    // Enable USB FS Clocks
    __HAL_RCC_USB2_OTG_FS_CLK_ENABLE();

    // Disable USB clock during CSleep mode
    __HAL_RCC_USB2_OTG_FS_ULPI_CLK_SLEEP_DISABLE();

    // Set USBFS Interrupt to the lowest priority
    HAL_NVIC_SetPriority(OTG_FS_IRQn, 1, 1);


    // Enable USBFS Interrupt
    HAL_NVIC_EnableIRQ(OTG_FS_IRQn);
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief          Deinit USB peripheral module
 *
 */
/*----------------------------------------------------------------------------*/
void boardUSBDeInit(void)
{
    // Enable USB FS Clocks
    __HAL_RCC_USB2_OTG_FS_CLK_DISABLE();
    // Disable USBFS Interrupt
    HAL_NVIC_DisableIRQ(OTG_FS_IRQn);
}
#endif

#ifndef BOOTLOADER
/*----------------------------------------------------------------------------*/
/*!
 * @brief  Rx Transfer completed callback.
 * @param  huart UART handle.
 * @retval None
 */
/*----------------------------------------------------------------------------*/
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *UartHandle)
{
    if (UartHandle->Instance == serialUart.Instance)
    {
        // Put received character to the message queue
        msgQueuePush(&boardMqSerialRx, &boardSerialChar);

        while (HAL_UART_GetState(UartHandle) != HAL_UART_STATE_READY)
        {
            // Wait for change UART state to READY
        }
        // Receive new character
        (void)HAL_UART_Receive_IT(&serialUart, &boardSerialChar, 1);
    }
}
#endif