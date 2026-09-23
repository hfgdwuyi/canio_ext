/*!
 * Copyright © Siemens Healthcare GmbH 2022, All Rights Reserved
 *
 * Project: Building Block Low End MCU
 *
 * @file
 * @brief Analog inputs reading
 *
 */
// Standard includes
#include <stdbool.h>
#include <stdio.h>
// HAL includes
#include <hal.h>
// CANOpen includes
#include <objects.h>
// Project includes
#include "error_defines.h"
#include "error.h"
#include "bsp_ain.h"
#ifndef BOOTLOADER
#include "terminal.h"
#endif

#define ADC1_PINS_NUM (sizeof(adc1Pins) / sizeof(adc1Pins[0]))
#define ADC3_PINS_NUM (sizeof(adc3Pins) / sizeof(adc3Pins[0]))
/*! Number of AI available */
#define AIN_MAX (ADC1_PINS_NUM + ADC3_PINS_NUM)

/*! ADC cinstabce default init values */
static const ADC_InitTypeDef ainDefInit = {
    .ClockPrescaler        = ADC_CLOCK_ASYNC_DIV128,    // Asynchronous clock mode, input ADC clock divided by 4
    .Resolution            = ADC_RESOLUTION_16B,        // 16-bit resolution for converted data
    .ScanConvMode          = ADC_SCAN_ENABLE,
    .EOCSelection          = ADC_EOC_SEQ_CONV,    // EOC flag picked-up to indicate conversion end
    .LowPowerAutoWait      = DISABLE,             // Auto-delayed conversion feature disabled
    .ContinuousConvMode    = DISABLE,             // Continuous mode to have maximum conversion speed (no delay between conversions)
    .DiscontinuousConvMode = DISABLE,
    .NbrOfDiscConversion   = 1,
    .ExternalTrigConv      = ADC_SOFTWARE_START,    // Software start to trig the 1st conversion manually, without external event
    .ExternalTrigConvEdge =
        ADC_EXTERNALTRIGCONVEDGE_NONE,    // Parameter discarded because trig of conversion by software start (no external event)
    .ConversionDataManagement = ADC_CONVERSIONDATA_DMA_CIRCULAR,
    .Overrun          = ADC_OVR_DATA_OVERWRITTEN,    // DR register is overwritten with the last conversion result in case of overrun
    .OversamplingMode = DISABLE,                     // No oversampling
};

static const DMA_InitTypeDef dmaDefInit = {
    .Direction           = DMA_PERIPH_TO_MEMORY,
    .PeriphInc           = DMA_PINC_DISABLE,
    .MemInc              = DMA_MINC_ENABLE,
    .PeriphDataAlignment = DMA_PDATAALIGN_HALFWORD,
    .MemDataAlignment    = DMA_MDATAALIGN_HALFWORD,
    .Mode                = DMA_NORMAL,
    .Priority            = DMA_PRIORITY_MEDIUM,
};

// ADC handler declaration
ADC_HandleTypeDef hadc1 = { .Instance = ADC1 };
ADC_HandleTypeDef hadc3 = { .Instance = ADC3 };

DMA_HandleTypeDef hdma1 = { .Instance = DMA1_Stream0 };
DMA_HandleTypeDef hdma3 = { .Instance = DMA1_Stream1 };

typedef struct
{
    GPIO_TypeDef *port;
    uint32_t      pin;
    uint32_t      channel;
    uint32_t      rank;
    uint32_t      index;
    char         *name;
} adcPin;

static const adcPin adc1Pins[] = {
    { GPIOC, GPIO_PIN_0, ADC_CHANNEL_10, ADC_REGULAR_RANK_1, 0, "PUMP-ADC" },       // PUMP-ADC
    { GPIOA, GPIO_PIN_15, ADC_CHANNEL_1, ADC_REGULAR_RANK_2, 4, "TEMP-FD-ADC" },    // ADC1_INP1 (TEMP-FD-ADC)
    { GPIOA, GPIO_PIN_3, ADC_CHANNEL_15, ADC_REGULAR_RANK_3, 5, "TEMP-MB-ADC" },    // TEMP-MB-ADC
    { GPIOC, GPIO_PIN_2, ADC_CHANNEL_12, ADC_REGULAR_RANK_4, 6, "ADC1_INP12" },     // ADC1_INP12
};

static const adcPin adc3Pins[] = {
    { GPIOF, GPIO_PIN_8, ADC_CHANNEL_7, ADC_REGULAR_RANK_1, 1, "FORCE-ADC(1)" },     // FORCE-ADC(1)
    { GPIOH, GPIO_PIN_2, ADC_CHANNEL_13, ADC_REGULAR_RANK_2, 2, "FORCE-ADC(2)" },    // FORCE-ADC(2)
    { GPIOH, GPIO_PIN_4, ADC_CHANNEL_15, ADC_REGULAR_RANK_3, 3, "FORCE-ADC(3)" },    // FORCE-ADC(3)
    { GPIOC, GPIO_PIN_3, ADC_CHANNEL_1, ADC_REGULAR_RANK_4, 7, "ADC3_INP1" },        // ADC3_INP1
};

static uint16_t dmaBuf1[ADC1_PINS_NUM] __attribute__((aligned(32)));
static uint16_t dmaBuf3[ADC3_PINS_NUM] __attribute__((aligned(32)));

static const struct
{
    ADC_HandleTypeDef *hadc;
    DMA_HandleTypeDef *hdma;
    IRQn_Type          dma_int;
    uint32_t           request;
    const adcPin      *pins;
    uint32_t           pin_num;
    uint16_t          *buf;
} adcTable[] = {
    { &hadc1, &hdma1, DMA1_Stream0_IRQn, DMA_REQUEST_ADC1, adc1Pins, ADC1_PINS_NUM, dmaBuf1 },
    { &hadc3, &hdma3, DMA1_Stream1_IRQn, DMA_REQUEST_ADC3, adc3Pins, ADC3_PINS_NUM, dmaBuf3 },
};

/*! Measured ADC values */
static uint32_t ainData[AIN_MAX];

uint8_t AIN_NUMBER = AIN_MAX;

/*----------------------------------------------------------------------------*/
/*!
 * @brief          ADC subsystem initialization
 *
 * @return         None
 */
/*----------------------------------------------------------------------------*/
void bspAinInit(void)
{
    // Enable ADC
    __HAL_RCC_ADC12_CLK_ENABLE();
    __HAL_RCC_ADC3_CLK_ENABLE();
    // ADC Periph interface clock configuration
    __HAL_RCC_ADC_CONFIG(RCC_ADCCLKSOURCE_CLKP);
    // DMA controller clock enable
    __HAL_RCC_DMA1_CLK_ENABLE();


    for (uint8_t i = 0; i < sizeof(adcTable) / sizeof(adcTable[0]); i++)
    {
        // Initialize ADC module
        adcTable[i].hadc->Init                 = ainDefInit;
        adcTable[i].hadc->Init.NbrOfConversion = adcTable[i].pin_num;
        if (HAL_ADC_Init(adcTable[i].hadc) != HAL_OK)
        {
            set_error(ERR_MAN_HW_INIT, ERR_INIT_ADC);
        }

        // Run the ADC calibration in single-ended mode
        if (HAL_ADCEx_Calibration_Start(adcTable[i].hadc, ADC_CALIB_OFFSET, ADC_SINGLE_ENDED) != HAL_OK)
        {
            // Calibration Error
            set_error(ERR_MAN_HW_INIT, ERR_ADC_CALIBRSTART);
        }

        // Initialize ADC pins and channels
        for (uint8_t j = 0; j < adcTable[i].pin_num; j++)
        {
            GPIO_InitTypeDef pin = { adcTable[i].pins[j].pin, GPIO_MODE_ANALOG, GPIO_NOPULL, GPIO_SPEED_FREQ_LOW, 0 };
            HAL_GPIO_Init(adcTable[i].pins[j].port, &pin);

            ADC_ChannelConfTypeDef config = {
                .Channel      = adcTable[i].pins[j].channel,
                .SamplingTime = ADC_SAMPLETIME_32CYCLES_5,
                .SingleDiff   = ADC_SINGLE_ENDED,
                .Rank         = adcTable[i].pins[j].rank,
                .OffsetNumber = ADC_OFFSET_NONE,
                .Offset       = 0,
            };
            if (HAL_ADC_ConfigChannel(adcTable[i].hadc, &config) != HAL_OK)
            {
                set_error(ERR_MAN_HW_INIT, ERR_INIT_ADC);
            }
        }

        // Initialize DMA

        adcTable[i].hdma->Init         = dmaDefInit;
        adcTable[i].hdma->Init.Request = adcTable[i].request;
        // Deinitialize  & Initialize the DMA for new transfer
        HAL_DMA_DeInit(adcTable[i].hdma);
        HAL_DMA_Init(adcTable[i].hdma);

        // Associate the DMA handle
        adcTable[i].hadc->DMA_Handle = adcTable[i].hdma;
        adcTable[i].hdma->Parent     = adcTable[i].hadc;

        // DMA interrupt init
        // DMA1_Stream0_IRQn interrupt configuration
        HAL_NVIC_SetPriority(adcTable[i].dma_int, 5, 1);
        HAL_NVIC_EnableIRQ(adcTable[i].dma_int);

        HAL_ADC_Start_DMA(adcTable[i].hadc, (uint32_t *)adcTable[i].buf, adcTable[i].pin_num);
    }
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief         Read raw ADC value
 *
 * @param[in]     channel AI number
 * @return        Raw ADC value for the channel
 */
/*----------------------------------------------------------------------------*/
uint32_t bspAinGetRawValue(uint8_t channel)
{
    if (channel >= AIN_MAX)
    {
        return 0;
    }
    return ainData[channel];
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief       Conversion complete callback in non-blocking mode.
 * @param       hadc ADC handle
 * @retval      None
 */
/*----------------------------------------------------------------------------*/
void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc)
{
    uint32_t index = ~0UL;
    for (uint8_t i = 0; i < sizeof(adcTable) / sizeof(adcTable[0]); i++)
    {
        if (adcTable[i].hadc == hadc)
        {
            index = i;
        }
    }
    if (index < sizeof(adcTable) / sizeof(adcTable[0]))
    {
        // Invalidate cash
        SCB_InvalidateDCache_by_Addr((uint32_t *)adcTable[index].buf, adcTable[index].pin_num);
        // Copy data from buffer
        for (uint32_t j = 0; j < adcTable[index].pin_num; j++)
        {
            ainData[adcTable[index].pins[j].index] = adcTable[index].buf[j];
        }

        // Start new conversion
        HAL_ADC_Start_DMA(adcTable[index].hadc, (uint32_t *)adcTable[index].buf, adcTable[index].pin_num);
    }
}
#ifndef BOOTLOADER
static terminalRet terminalAinTest(uint8_t argc, char **argv);

__attribute__((constructor)) void aInWork(void)
{
    static terminalItem terminalAinItem = { .name     = "ain",
                                            .desc     = "Read value from ADC channel",
                                            .help     = "This command changes state of output pin to high or low\n dout"
                                                        "[arg1] [arg2] [arg3]\n"
                                                        "arg1 - read(writes pin states of byte choosen), set(specifies an output state)\n"
                                                        "arg2 - byte number to change state\n"
                                                        "arg3 - 8 bit state value of 8 output pins(ignored in case of reading)\n",
                                            .callback = terminalAinTest,
                                            .next     = NULL };
    terminalAddItem(&terminalAinItem);
}

/* ---------------------------------------------------------------------------*/
/*!
 * @brief         Writes Analog inputs values in terminal
 *
 * @param[in]     argc number of command arguments
 * @param[in]     argv command arguments
 * @return        Status of operation
 */
/*----------------------------------------------------------------------------*/
static terminalRet terminalAinTest(uint8_t argc, char **argv)
{
    if (argc < 3)
    {
        return SHELL_EARGC;
    }
    if (strcmp(argv[1], "read") == 0)
    {
        uint32_t channel;
        if (!terminalStrToInt(argv[2], &channel))
        {
            printf("Cannot convert %s to number\n", argv[2]);
            return SHELL_ECONV;
        }
        if (channel < AIN_MAX)
        {
            // Find the correct index
            for (uint8_t i = 0; i < sizeof(adcTable) / sizeof(adcTable[0]); i++)
            {
                for (uint32_t j = 0; j < adcTable[i].pin_num; j++)
                {
                    if (adcTable[i].pins[j].index == channel)
                    {
                        printf("%s input = %ld\n", adcTable[i].pins[j].name, ainData[channel]);
                    }
                }
            }
        }
        else
        {
            return SHELL_EARG;
        }
    }
    else
    {
        return SHELL_EARG;
    }
    return SHELL_OK;
}
#endif