/*!
 * Copyright Siemens Healthcare GmbH 2022, All Rights Reserved
 *
 * Project: Building Block Low End MCU
 *
 * @file
 * @brief Analog output writing
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
#include "bsp_aout.h"
#ifndef BOOTLOADER
#include "terminal.h"
#endif

/*! Number of AOUT available */
#define AOUT_MAX (sizeof(dacPins) / sizeof(dacPins[0]))


// DAC handler declaration
DAC_HandleTypeDef hdac1 = { .Instance = DAC1 };

/*! DAC channel default init values */
static const DAC_ChannelConfTypeDef aoutDefConfig = {
    .DAC_SampleAndHold           = DAC_SAMPLEANDHOLD_DISABLE,
    .DAC_Trigger                 = DAC_TRIGGER_NONE,
    .DAC_OutputBuffer            = DAC_OUTPUTBUFFER_ENABLE,
    .DAC_ConnectOnChipPeripheral = DAC_CHIPCONNECT_DISABLE,
    .DAC_UserTrimming            = DAC_TRIMMING_FACTORY,
};

/*! DAC Pins' initialization table */
static const struct
{
    GPIO_TypeDef      *port;
    GPIO_InitTypeDef   pin;
    DAC_HandleTypeDef *hdac;
    uint32_t           channel;
} dacPins[] = {
    { GPIOA, { GPIO_PIN_4, GPIO_MODE_ANALOG, GPIO_NOPULL, GPIO_SPEED_FREQ_LOW, 0 }, &hdac1, DAC_CHANNEL_1 },    // DAC_OUT1
};

/*----------------------------------------------------------------------------*/
/*!
 * @brief          DAC subsystem initialization
 *
 */
/*----------------------------------------------------------------------------*/
void bspAoutInit(void)
{
    // Enable DAC
    __HAL_RCC_DAC12_CLK_ENABLE();
    // Initialize DAC pins
    for (uint8_t i = 0; i < sizeof(dacPins) / sizeof(dacPins[0]); i++)
    {
        GPIO_InitTypeDef pin = dacPins[i].pin;
        HAL_GPIO_Init(dacPins[i].port, &pin);

        // Initialize DAC module
        if (HAL_DAC_Init(&hdac1) != HAL_OK)
        {
            // DAC initialization error
            set_error(ERR_MAN_HW_INIT, ERR_INIT_DAC);
        }
        DAC_ChannelConfTypeDef config = aoutDefConfig;
        if (HAL_DAC_ConfigChannel(dacPins[i].hdac, &config, dacPins[i].channel) != HAL_OK)
        {
            set_error(ERR_MAN_HW_INIT, ERR_DAC_CHANNEL_CONFIG);
        }
    }
    HAL_DAC_Start(dacPins[0].hdac, dacPins[0].channel);
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief         Write value to microcontroller's DAC.
 *
 * @param[in]     channel       DAC`s channel to work with.
 * @param[in]     writeValue - value that should be written to microcontroller`s DAC.
 *
 */
/*----------------------------------------------------------------------------*/
void bspAoutWrite(uint8_t channel, int16_t val)
{
    float    DACvalue        = (float)(val / 1000.0);
    uint32_t writeValueToDAC = (uint32_t)((DACvalue * 4096) / 3.3);

    HAL_DAC_SetValue(dacPins[channel].hdac, dacPins[channel].channel, DAC_ALIGN_12B_R, writeValueToDAC);
    // reading DAC value and printf is only for testing
    float readDAC = (float)HAL_DAC_GetValue(dacPins[channel].hdac, dacPins[channel].channel);
    readDAC       = (float)(readDAC * 3.3 / 4096);
    printf("The value of DAC is: %.3f V\n", readDAC);
}
#ifndef BOOTLOADER
static terminalRet terminalAoutTest(uint8_t argc, char **argv);

__attribute__((constructor)) void aOutWork(void)
{
    static terminalItem terminalAoutItem = { .name     = "aout",
                                             .desc     = "Set analog value to output analog pin",
                                             .help     = "This command changes state of output pin to high or low\n dout"
                                                         "[arg1] [arg2] [arg3]\n"
                                                         "arg1 - set(specifies an output state)\n"
                                                         "arg2 - channel to set\n"
                                                         "arg3 - value to set\n",
                                             .callback = terminalAoutTest,
                                             .next     = NULL };
    terminalAddItem(&terminalAoutItem);
}

/* ---------------------------------------------------------------------------*/
/*!
 * @brief         Allows to change state of output pins
 *
 * @param[in]     argc number of command arguments
 * @param[in]     argv command arguments
 * @return        Status of operation
 */
/*----------------------------------------------------------------------------*/
static terminalRet terminalAoutTest(uint8_t argc, char **argv)
{
    // if arguments number is not sufficient for this operation
    if (argc < 4)
    {
        return SHELL_EARGC;
    }
    uint32_t setValue;
    if (!terminalStrToInt(argv[3], &setValue))
    {
        printf("Cannot convert %s to number\n", argv[3]);
        return SHELL_ECONV;
    }
    uint32_t channel;
    if (!terminalStrToInt(argv[2], &channel))
    {
        printf("Cannot convert %s to number\n", argv[2]);
        return SHELL_ECONV;
    }
    if (channel > AOUT_MAX)
    {
        return SHELL_EARG;
    }
    if (strcmp(argv[1], "set") == 0)
    {
        if (setValue < 65535)
        {
            bspAoutWrite(0, (int16_t)setValue);
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