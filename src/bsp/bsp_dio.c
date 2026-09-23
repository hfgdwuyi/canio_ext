/*!
 * Copyright © Siemens Healthcare GmbH 2022, All Rights Reserved
 *
 * Project: Building Block Low End MCU
 *
 * @file
 * @brief    Digital input/output pin control functions
 */
/*----------------------------------------------------------------------------*/
// Standart includes
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <limits.h>
#include <stdlib.h>
// HAL includes
#include <hal.h>
// Application includes
#include "bsp_dio.h"
#include "timing.h"

#ifndef BOOTLOADER
#include "terminal.h"
#endif

/* Number of DINs */
#define DIN_MAX (sizeof(pinCfgDin) / sizeof(pinCfgDin[0]))
/* Number of DOUTs */
#define DOUT_MAX (sizeof(pinCfgDout) / sizeof(pinCfgDout[0]))
/* Number of DIN bytes */
#define DIN_BYTES ((DIN_MAX + CHAR_BIT - 1) / CHAR_BIT)
/* Number of DOUT bytes */
#define DOUT_BYTES ((DOUT_MAX + CHAR_BIT - 1) / CHAR_BIT)
#define DOUT_UI_LED_OUTPUT_PIN 20U
#define DOUT_UI_POWER_OUTPUT_PIN 21U
#define DIN_SUBIDX4_UI_POWER_DETECT_BIT 25U


// Static functions declaration
static void bspDoutInit(void);
static void bspDinInit(void);
static void bspDinUpdate(void);
static void bspDoutWritePin(const pinCfg *cfg, bool state);
static bool bspDinReadPin(const pinCfg *cfg);

/*----------------------------------------------------------------------------*/
/*!
 @name           IO configuration
@{
*/
/*----------------------------------------------------------------------------*/
/*! Table containing output pin group assignment */
static const pinCfg uiLedOutputRight = { GPIOJ, { GPIO_PIN_14, GPIO_MODE_OUTPUT_PP, GPIO_PULLUP, GPIO_SPEED_FREQ_MEDIUM, 0 } };
static const pinCfg uiPowerOutputRight = { GPIOJ, { GPIO_PIN_15, GPIO_MODE_OUTPUT_PP, GPIO_PULLUP, GPIO_SPEED_FREQ_MEDIUM, 0 } };
static const pinCfg uiPowerDetectRight = { GPIOF, { GPIO_PIN_15, GPIO_MODE_INPUT, GPIO_PULLUP, GPIO_SPEED_FREQ_MEDIUM, 0 } };

static const pinCfg pinCfgDout[] = {
    // Byte 0 CANOpen subindex 1
    { GPIOA, { GPIO_PIN_0, GPIO_MODE_OUTPUT_PP, GPIO_PULLUP, GPIO_SPEED_FREQ_MEDIUM, 0 } },     // 
    { GPIOA, { GPIO_PIN_0, GPIO_MODE_OUTPUT_PP, GPIO_PULLUP, GPIO_SPEED_FREQ_MEDIUM, 0 } },     // 
    { GPIOA, { GPIO_PIN_0, GPIO_MODE_OUTPUT_PP, GPIO_PULLUP, GPIO_SPEED_FREQ_MEDIUM, 0 } },     // 
    { GPIOA, { GPIO_PIN_0, GPIO_MODE_OUTPUT_PP, GPIO_PULLUP, GPIO_SPEED_FREQ_MEDIUM, 0 } },     // 
    { GPIOA, { GPIO_PIN_0, GPIO_MODE_OUTPUT_PP, GPIO_PULLUP, GPIO_SPEED_FREQ_MEDIUM, 0 } },     // 
    { GPIOA, { GPIO_PIN_0, GPIO_MODE_OUTPUT_PP, GPIO_PULLUP, GPIO_SPEED_FREQ_MEDIUM, 0 } },     // 
    { GPIOA, { GPIO_PIN_0, GPIO_MODE_OUTPUT_PP, GPIO_PULLUP, GPIO_SPEED_FREQ_MEDIUM, 0 } },    // 
    { GPIOA, { GPIO_PIN_0, GPIO_MODE_OUTPUT_PP, GPIO_PULLUP, GPIO_SPEED_FREQ_MEDIUM, 0 } },    // 
    // Byte 1 CANOpen subindex 2
    { GPIOA, { GPIO_PIN_0, GPIO_MODE_OUTPUT_PP, GPIO_PULLUP, GPIO_SPEED_FREQ_MEDIUM, 0 } },    // 
    { GPIOA, { GPIO_PIN_0, GPIO_MODE_OUTPUT_PP, GPIO_PULLUP, GPIO_SPEED_FREQ_MEDIUM, 0 } },    // 
    { GPIOA, { GPIO_PIN_0, GPIO_MODE_OUTPUT_PP, GPIO_PULLUP, GPIO_SPEED_FREQ_MEDIUM, 0 } },    // 
    { GPIOA, { GPIO_PIN_0, GPIO_MODE_OUTPUT_PP, GPIO_PULLUP, GPIO_SPEED_FREQ_MEDIUM, 0 } },    // 
    { GPIOA, { GPIO_PIN_0, GPIO_MODE_OUTPUT_PP, GPIO_PULLUP, GPIO_SPEED_FREQ_MEDIUM, 0 } },    //
    { GPIOA, { GPIO_PIN_0, GPIO_MODE_OUTPUT_PP, GPIO_PULLUP, GPIO_SPEED_FREQ_MEDIUM, 0 } },    // 
    { GPIOA, { GPIO_PIN_0, GPIO_MODE_OUTPUT_PP, GPIO_PULLUP, GPIO_SPEED_FREQ_MEDIUM, 0 } },    //
    { GPIOA, { GPIO_PIN_0, GPIO_MODE_OUTPUT_PP, GPIO_PULLUP, GPIO_SPEED_FREQ_MEDIUM, 0 } },    // 
    // Byte 2 CANOpen subindex 3
    { GPIOD, { GPIO_PIN_1, GPIO_MODE_OUTPUT_PP, GPIO_PULLUP, GPIO_SPEED_FREQ_MEDIUM, 0 } },     // BRK_PWR_EN1
    { GPIOG, { GPIO_PIN_2, GPIO_MODE_OUTPUT_PP, GPIO_PULLUP, GPIO_SPEED_FREQ_MEDIUM, 0 } },     // BRK_PWR_EN2
    { GPIOD, { GPIO_PIN_14, GPIO_MODE_OUTPUT_PP, GPIO_PULLUP, GPIO_SPEED_FREQ_MEDIUM, 0 } },    // BRK_PWR_EN3
    { GPIOF, { GPIO_PIN_0, GPIO_MODE_OUTPUT_PP, GPIO_PULLUP, GPIO_SPEED_FREQ_MEDIUM, 0 } },     // BRK_PWR_EN4
    { GPIOG, { GPIO_PIN_0, GPIO_MODE_OUTPUT_PP, GPIO_PULLUP, GPIO_SPEED_FREQ_MEDIUM, 0 } },     // TLC5917_OE_N_L
    { GPIOG, { GPIO_PIN_7, GPIO_MODE_OUTPUT_PP, GPIO_PULLUP, GPIO_SPEED_FREQ_MEDIUM, 0 } },    // UI-PWR-EN_L
    { GPIOG, { GPIO_PIN_3, GPIO_MODE_OUTPUT_PP, GPIO_PULLUP, GPIO_SPEED_FREQ_MEDIUM, 0 } },     // CAN-PWR-EN

};
/*! Table containing input pin group assignment */
static const pinCfg pinCfgDin[] = {
    // Byte 0 CANOpen subindex 1
    { GPIOG, { GPIO_PIN_4, GPIO_MODE_INPUT, GPIO_PULLUP, GPIO_SPEED_FREQ_MEDIUM, 0 } },     // CAP-CAN-PWR-FLT
    { GPIOB, { GPIO_PIN_0, GPIO_MODE_INPUT, GPIO_PULLUP, GPIO_SPEED_FREQ_MEDIUM, 0 } },     // 
    { GPIOB, { GPIO_PIN_0, GPIO_MODE_INPUT, GPIO_PULLUP, GPIO_SPEED_FREQ_MEDIUM, 0 } },     // 
    { GPIOB, { GPIO_PIN_0, GPIO_MODE_INPUT, GPIO_PULLUP, GPIO_SPEED_FREQ_MEDIUM, 0 } },     // 
    { GPIOB, { GPIO_PIN_0, GPIO_MODE_INPUT, GPIO_PULLUP, GPIO_SPEED_FREQ_MEDIUM, 0 } },     // 
    { GPIOB, { GPIO_PIN_0, GPIO_MODE_INPUT, GPIO_PULLUP, GPIO_SPEED_FREQ_MEDIUM, 0 } },     // 
    { GPIOB, { GPIO_PIN_0, GPIO_MODE_INPUT, GPIO_PULLUP, GPIO_SPEED_FREQ_MEDIUM, 0 } },     // 
    { GPIOB, { GPIO_PIN_0, GPIO_MODE_INPUT, GPIO_PULLUP, GPIO_SPEED_FREQ_MEDIUM, 0 } },     // 
    // Byte 1 CANOpen subindex 2
    { GPIOI, { GPIO_PIN_6, GPIO_MODE_INPUT, GPIO_PULLUP, GPIO_SPEED_FREQ_MEDIUM, 0 } },     // UI-IN(1)
    { GPIOI, { GPIO_PIN_7, GPIO_MODE_INPUT, GPIO_PULLUP, GPIO_SPEED_FREQ_MEDIUM, 0 } },     // UI-IN(2)
    { GPIOI, { GPIO_PIN_8, GPIO_MODE_INPUT, GPIO_PULLUP, GPIO_SPEED_FREQ_MEDIUM, 0 } },     // UI-IN(3)
    { GPIOI, { GPIO_PIN_9, GPIO_MODE_INPUT, GPIO_PULLUP, GPIO_SPEED_FREQ_MEDIUM, 0 } },     // UI-IN(4)
    { GPIOI, { GPIO_PIN_10, GPIO_MODE_INPUT, GPIO_PULLUP, GPIO_SPEED_FREQ_MEDIUM, 0 } },    // UI-IN(5)
    { GPIOI, { GPIO_PIN_11, GPIO_MODE_INPUT, GPIO_PULLUP, GPIO_SPEED_FREQ_MEDIUM, 0 } },    // UI-IN(6)
    { GPIOI, { GPIO_PIN_12, GPIO_MODE_INPUT, GPIO_PULLUP, GPIO_SPEED_FREQ_MEDIUM, 0 } },    // UI-IN(7)
    { GPIOI, { GPIO_PIN_13, GPIO_MODE_INPUT, GPIO_PULLUP, GPIO_SPEED_FREQ_MEDIUM, 0 } },    // 
    // Byte 2 CANOpen subindex 3
    { GPIOE, { GPIO_PIN_15, GPIO_MODE_INPUT, GPIO_PULLUP, GPIO_SPEED_FREQ_MEDIUM, 0 } },    // UI-IN(1)
    { GPIOE, { GPIO_PIN_14, GPIO_MODE_INPUT, GPIO_PULLUP, GPIO_SPEED_FREQ_MEDIUM, 0 } },    // UI-IN(2)
    { GPIOE, { GPIO_PIN_13, GPIO_MODE_INPUT, GPIO_PULLUP, GPIO_SPEED_FREQ_MEDIUM, 0 } },    // UI-IN(3)
    { GPIOE, { GPIO_PIN_12, GPIO_MODE_INPUT, GPIO_PULLUP, GPIO_SPEED_FREQ_MEDIUM, 0 } },    // UI-IN(4)
    { GPIOE, { GPIO_PIN_11, GPIO_MODE_INPUT, GPIO_PULLUP, GPIO_SPEED_FREQ_MEDIUM, 0 } },    // UI-IN(5)
    { GPIOE, { GPIO_PIN_10, GPIO_MODE_INPUT, GPIO_PULLUP, GPIO_SPEED_FREQ_MEDIUM, 0 } },    // UI-IN(6)
    { GPIOE, { GPIO_PIN_9, GPIO_MODE_INPUT, GPIO_PULLUP, GPIO_SPEED_FREQ_MEDIUM, 0 } },     // UI-IN(7)
    { GPIOE, { GPIO_PIN_8, GPIO_MODE_INPUT, GPIO_PULLUP, GPIO_SPEED_FREQ_MEDIUM, 0 } },     // 
    // Byte 3 CANOpen subindex 4
    { GPIOB, { GPIO_PIN_12, GPIO_MODE_INPUT, GPIO_PULLUP, GPIO_SPEED_FREQ_MEDIUM, 0 } },    // TEMP_ALERT
    { GPIOI, { GPIO_PIN_15, GPIO_MODE_INPUT, GPIO_PULLUP, GPIO_SPEED_FREQ_MEDIUM, 0 } },    // UI-P24V-DET_L
    { GPIOB, { GPIO_PIN_6, GPIO_MODE_INPUT, GPIO_PULLUP, GPIO_SPEED_FREQ_MEDIUM, 0 } },     // 
    { GPIOB, { GPIO_PIN_6, GPIO_MODE_INPUT, GPIO_PULLUP, GPIO_SPEED_FREQ_MEDIUM, 0 } },     // 
    { GPIOB, { GPIO_PIN_6, GPIO_MODE_INPUT, GPIO_PULLUP, GPIO_SPEED_FREQ_MEDIUM, 0 } },     // 
    { GPIOB, { GPIO_PIN_6, GPIO_MODE_INPUT, GPIO_PULLUP, GPIO_SPEED_FREQ_MEDIUM, 0 } },     // 
    { GPIOB, { GPIO_PIN_6, GPIO_MODE_INPUT, GPIO_PULLUP, GPIO_SPEED_FREQ_MEDIUM, 0 } },     // 
    { GPIOB, { GPIO_PIN_6, GPIO_MODE_INPUT, GPIO_PULLUP, GPIO_SPEED_FREQ_MEDIUM, 0 } },     // 
};


/*! Number of Digital input pins */
const uint8_t dinMax = sizeof(pinCfgDin) / sizeof(pinCfgDin[0]);
/*! Current state of digital inputs */
static uint8_t dinState[DIN_BYTES];
/*! Digital inputs debouncing settings */
static bspDinSettings dinSettings[DIN_MAX];

/*! Number of Digital output pins */
const uint8_t doutMax = sizeof(pinCfgDout) / sizeof(pinCfgDout[0]);

/*----------------------------------------------------------------------------*/
/*!
 * @brief          Init digital input/output pins
 *
 */
/*----------------------------------------------------------------------------*/
void bspDioInit(void)
{
    // Init input pins
    bspDinInit();
    // Init otput pins
    bspDoutInit();
}

/* ---------------------------------------------------------------------------*/
/*!
 * @brief        Digital input pins initialization
 *
 */
/*----------------------------------------------------------------------------*/
static void bspDinInit(void)
{
    static timingTimer dinTimer;
    for (uint8_t i = 0; i < dinMax; i++)
    {
        // Configure GPIO pin
        GPIO_InitTypeDef pin = pinCfgDin[i].pin;
        (void)HAL_GPIO_Init(pinCfgDin[i].port, &pin);
    }

    GPIO_InitTypeDef uiPowerDetectRightPin = uiPowerDetectRight.pin;
    (void)HAL_GPIO_Init(uiPowerDetectRight.port, &uiPowerDetectRightPin);

    // Start periodic state read
    (void)timingAddTimer(&dinTimer, TIMING_TIMER_CYCLIC, 1, bspDinUpdate);
}

/* ---------------------------------------------------------------------------*/
/*!
 * @brief        Set up DIN debouncing settings
 *
 * @param        pin  pin number
 * @param        settings debouncing settings to set
 *
 */
/*----------------------------------------------------------------------------*/
void bspDinSetDebouncing(uint8_t pin, bspDinSettings settings)
{
    if (pin < DIN_MAX)
    {
        dinSettings[pin] = settings;
    }
}

/* ---------------------------------------------------------------------------*/
/*!
 * @brief        Handler routine for periodic pin state read
 *
 */
/*----------------------------------------------------------------------------*/
static void bspDinUpdate(void)
{
    static uint32_t debounceTime[DIN_MAX] = { 0 };
    for (uint8_t dinIndex = 0; dinIndex < dinMax; dinIndex++)
    {
        uint8_t bit  = dinIndex % CHAR_BIT;
        uint8_t byte = dinIndex / CHAR_BIT;
        // read din pins
        bool curState = bspDinReadPin(&pinCfgDin[dinIndex]);

        if (dinIndex == DIN_SUBIDX4_UI_POWER_DETECT_BIT)
        {
            curState = curState || bspDinReadPin(&uiPowerDetectRight);
        }

        bool oldValue = (dinState[byte] & (1 << bit)) == 0 ? 0 : 1;

        uint8_t debTime = dinSettings[dinIndex].deb_en ? dinSettings[dinIndex].deb_time : 0;

        // Debouncing
        if (oldValue != curState)
        {
            debounceTime[dinIndex]++;
            if (debounceTime[dinIndex] >= debTime)
            {
                if (curState != 0)
                {
                    dinState[byte] |= (1U << bit);
                }
                else
                {
                    dinState[byte] &= ~(1U << bit);
                }
                debounceTime[dinIndex] = 0;
            }
        }
        else
        {
            debounceTime[dinIndex] = 0;
        }
    }
}

/* ---------------------------------------------------------------------------*/
/*!
 * @brief        Read a specific input GPIO described by pinCfg
 *
 * @param[in]    cfg - pin configuration to read
 * @return       pin state
 *
 */
/*----------------------------------------------------------------------------*/
static bool bspDinReadPin(const pinCfg *cfg)
{
    return HAL_GPIO_ReadPin(cfg->port, (uint16_t)cfg->pin.Pin) ? 1 : 0;
}

/* ---------------------------------------------------------------------------*/
/*!
 * @brief        Read a byte with DIN states
 *
 * @param        byte array index to read
 * @return       DIN values
 */
/*----------------------------------------------------------------------------*/
uint8_t bspDinRead(uint8_t byte)
{
    if (byte >= DIN_BYTES)
    {
        return 0;
    }
    return dinState[byte];
}

/* ---------------------------------------------------------------------------*/
/*!
 * @brief        Digital output pins initialization
 *
 * @param[in]    pinStateMask - pointer to initial state value
 *
 */
/*----------------------------------------------------------------------------*/
static void bspDoutInit(void)
{
    for (uint8_t i = 0; i < doutMax; i++)
    {
        // Configure GPIO pin
        GPIO_InitTypeDef pin = pinCfgDout[i].pin;
        (void)HAL_GPIO_Init(pinCfgDout[i].port, &pin);
    }

    GPIO_InitTypeDef uiLedPinRight = uiLedOutputRight.pin;
    GPIO_InitTypeDef uiPowerPinRight = uiPowerOutputRight.pin;
    (void)HAL_GPIO_Init(uiLedOutputRight.port, &uiLedPinRight);
    (void)HAL_GPIO_Init(uiPowerOutputRight.port, &uiPowerPinRight);
}

/* ---------------------------------------------------------------------------*/
/*!
 * @brief        Write a specific GPIO described by pinCfg
 *
 * @param[in]    cfg - pin configuration to write
 * @param[in]    state - 0 or 1, set or reset state
 *
 */
/*----------------------------------------------------------------------------*/
static void bspDoutWritePin(const pinCfg *cfg, bool state)
{
    if (state)
    {
        HAL_GPIO_WritePin(cfg->port, (uint16_t)cfg->pin.Pin, GPIO_PIN_SET);
    }
    else
    {
        HAL_GPIO_WritePin(cfg->port, (uint16_t)cfg->pin.Pin, GPIO_PIN_RESET);
    }
}

/* ---------------------------------------------------------------------------*/
/*!
 * @brief        Set specific output pin state
 *
 * @param[in]    pinNumber - pin Number from pinCfgDout table
 * @param[in]    state - 0 or 1, set or reset state
 *
 */
/*----------------------------------------------------------------------------*/
void bspDoutSet(uint8_t pinNumber, bool state)
{
    if (pinNumber < doutMax)
    {
        bspDoutWritePin(&pinCfgDout[pinNumber], state);

        if (pinNumber == DOUT_UI_LED_OUTPUT_PIN)
        {
            bspDoutWritePin(&uiLedOutputRight, state);
        }
        else if (pinNumber == DOUT_UI_POWER_OUTPUT_PIN)
        {
            bspDoutWritePin(&uiPowerOutputRight, state);
        }
    }
}

/* ---------------------------------------------------------------------------*/
/*!
 * @brief        Read output pins state
 *
 */
/*----------------------------------------------------------------------------*/
bool bspDoutRead(uint8_t pinNumber)
{
    if (pinNumber < doutMax && (bool)HAL_GPIO_ReadPin(pinCfgDout[pinNumber].port, (uint16_t)pinCfgDout[pinNumber].pin.Pin))
    {
        return true;
    }
    return false;
}

#ifndef BOOTLOADER
static terminalRet terminalDoutTest(uint8_t argc, char **argv);
static terminalRet terminalDinTest(uint8_t argc, char **argv);

/* ---------------------------------------------------------------------------*/
/*!
 * @brief        Adds din,dout commands to the list of terminal commands
 *
 */
/*----------------------------------------------------------------------------*/
__attribute__((constructor)) void pinWork(void)
{
    static terminalItem terminalDoutItem = { .name     = "dout",
                                             .desc     = "Set/Reset state of output pins",
                                             .help     = "This command changes state of output pin to high or low\n dout"
                                                         "[arg1] [arg2] [arg3]\n"
                                                         "arg1 - read(writes pin states of byte choosen), set(specifies an output state)\n"
                                                         "arg2 - byte number to change state\n"
                                                         "arg3 - 8 bit state value of 8 output pins(ignored in case of reading)\n",
                                             .callback = terminalDoutTest,
                                             .next     = NULL };
    terminalAddItem(&terminalDoutItem);
    static terminalItem terminalDinItem = { .name     = "din",
                                            .desc     = "Reads specific 8 digital inputs",
                                            .help     = "This command writes on screen the state of digital input pins\n din"
                                                        "[arg1]\n arg1 - byte number to read\n",
                                            .callback = terminalDinTest,
                                            .next     = NULL };
    terminalAddItem(&terminalDinItem);
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief   Executes DOUT read command from terminal
 *
 * @param[in] doutValue number of byte to read
 * @return    Status of operation
 */
/*----------------------------------------------------------------------------*/
static terminalRet readTerminalDout(uint32_t doutValue)
{
    if (doutValue < DOUT_BYTES)
    {
        uint8_t val = 0;
        for (uint8_t i = 0; i < CHAR_BIT; i++)
        {
            if (bspDoutRead((uint8_t)doutValue * CHAR_BIT + i))
            {
                val |= (1U << i);
            }
        }
        printf("Digital output byte %ld is: 0x%X\n", doutValue, val);
    }
    else
    {
        return SHELL_EARG;
    }
    return SHELL_OK;
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief   Executes DOUT write command from terminal
 *
 * @param[in] argv command arguments
 * @return  Status of operation
 */
/*----------------------------------------------------------------------------*/
static terminalRet setTerminalDout(char **argv)
{
    uint32_t setByte;
    uint32_t setValue;
    if (!terminalStrToInt(argv[0], &setByte))
    {
        printf("Cannot convert %s to number\n", argv[0]);
        return SHELL_ECONV;
    }
    if (!terminalStrToInt(argv[1], &setValue))
    {
        printf("Cannot convert %s to number\n", argv[1]);
        return SHELL_ECONV;
    }
    if (setByte < DOUT_BYTES)
    {
        for (uint8_t i = 0; i < CHAR_BIT; i++)
        {
            bool initState = ((uint8_t)setValue & (1U << i)) == 0 ? false : true;
            bspDoutSet(i + ((uint8_t)setByte * CHAR_BIT), initState);
        }
    }
    else
    {
        return SHELL_EARG;
    }
    return SHELL_OK;
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
static terminalRet terminalDoutTest(uint8_t argc, char **argv)
{
    // if arguments number is not sufficient for this operation
    if (argc < 3)
    {
        return SHELL_EARGC;
    }
    terminalRet ret = 0;
    if (strcmp(argv[1], "read") == 0)
    {
        uint32_t readByteDout;
        if (!terminalStrToInt(argv[2], &readByteDout))
        {
            printf("Cannot convert %s to number\n", argv[2]);
            return SHELL_ECONV;
        }
        ret = readTerminalDout(readByteDout);
        if (ret != SHELL_OK)
        {
            return ret;
        }
    }
    else if (strcmp(argv[1], "set") == 0)
    {
        ret = setTerminalDout(&argv[2]);
        if (ret != SHELL_OK)
        {
            return ret;
        }
    }
    else
    {
        return SHELL_EARG;
    }
    return SHELL_OK;
}

/* ---------------------------------------------------------------------------*/
/*!
 * @brief         Perform periodic din command
 *
 * @param[in]     argc number of command arguments
 * @param[in]     argv command arguments
 * @return        Status of operation
 */
/*----------------------------------------------------------------------------*/
static terminalRet terminalDinTest(uint8_t argc, char **argv)
{
    // if arguments number is not sufficient for this operation
    if (argc < 3)
    {
        return SHELL_EARGC;
    }
    if (strcmp(argv[1], "read") == 0)
    {
        uint32_t readByteDin;
        if (!terminalStrToInt(argv[2], &readByteDin))
        {
            printf("Cannot convert %s to number\n", argv[2]);
            return SHELL_ECONV;
        }
        if (readByteDin < DIN_BYTES)
        {
            printf("Digital input byte %ld is : 0x%X\n", readByteDin, dinState[readByteDin]);
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
//--------------------------------- End Of File -------------------------------/
