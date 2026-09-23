/*!
 * Copyright Siemens Healthcare GmbH 2022, All Rights Reserved
 *
 * Project: Building Block Low End MCU
 *
 * @file
 * @brief PWM implementation
 */
/*----------------------------------------------------------------------------*/


// Standart includes
#include <stdbool.h>
#include <stdio.h>
// HAL includes
#include <hal.h>
// Project includes
#include "error_defines.h"
#include "error.h"
#include "bsp_pwm.h"
#ifndef BOOTLOADER
#include "terminal.h"
#endif

/* PWM prescaler value */
#define PWM_PRESCALER 3
/* Default PWM frequency used during startup before OD update */
#define PWM_FREQ_DEFAULT 15000UL
/* TIM1/TIM8 are 16-bit timers */
#define PWM_TIMER_MAX 0xFFFFUL
/* Maximum duty cycle value*/
#define DUTY_MAX 100
/* Number of PWMs*/
#define PWM_MAX (sizeof(pwmPins) / sizeof(pwmPins[0]))

static bool pwmCalcTimerParams(uint32_t frequency, uint32_t *prescaler, uint32_t *period)
{
    if ((frequency == 0U) || (prescaler == NULL) || (period == NULL))
    {
        return false;
    }

    uint64_t timerClk = HAL_RCC_GetHCLKFreq();
    uint64_t denom    = (uint64_t)frequency * (PWM_TIMER_MAX + 1ULL);
    uint64_t pscDiv   = (timerClk + denom - 1ULL) / denom;

    if (pscDiv == 0ULL)
    {
        pscDiv = 1ULL;
    }
    if (pscDiv > (PWM_TIMER_MAX + 1ULL))
    {
        return false;
    }

    uint64_t arr = timerClk / ((uint64_t)frequency * pscDiv);
    if (arr == 0ULL)
    {
        return false;
    }
    arr -= 1ULL;

    if (arr > PWM_TIMER_MAX)
    {
        arr = PWM_TIMER_MAX;
    }

    *prescaler = (uint32_t)(pscDiv - 1ULL);
    *period    = (uint32_t)arr;
    return true;
}


/*! PWM default configuration structure*/
static const TIM_Base_InitTypeDef pwmDefInit = {
    .Prescaler         = PWM_PRESCALER,                  /**/
    .CounterMode       = TIM_COUNTERMODE_UP,             /**/
    .ClockDivision     = TIM_CLOCKDIVISION_DIV1,         /**/
    .RepetitionCounter = 0,                              /**/
    .AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE, /**/
};

/*PWM Output Compare Configuration structure*/
static const TIM_OC_InitTypeDef pwmOutput = {
    .OCMode       = TIM_OCMODE_PWM1,        /**/
    .Pulse        = 0,                      /**/
    .OCPolarity   = TIM_OCPOLARITY_HIGH,    /**/
    .OCNPolarity  = TIM_OCNPOLARITY_HIGH,   /**/
    .OCFastMode   = TIM_OCFAST_DISABLE,     /**/
    .OCIdleState  = TIM_OCIDLESTATE_RESET,  /**/
    .OCNIdleState = TIM_OCNIDLESTATE_RESET, /**/
};

/* PWM handle declaration*/
TIM_HandleTypeDef htim1 = { .Instance = TIM1 };
TIM_HandleTypeDef htim8 = { .Instance = TIM8 };

/*! Pins' initialization table */
static const struct
{
    GPIO_TypeDef      *port;
    GPIO_InitTypeDef   pin;
    TIM_HandleTypeDef *htim;
    uint32_t           channel;
} pwmPins[] = {
        { GPIOK,
            { GPIO_PIN_1, GPIO_MODE_AF_PP, GPIO_NOPULL, GPIO_SPEED_FREQ_VERY_HIGH, GPIO_AF1_TIM1 },
            &htim1,
            TIM_CHANNEL_1 },    // PWM1: PK1 TIM1_CH1
        { GPIOJ,
            { GPIO_PIN_11, GPIO_MODE_AF_PP, GPIO_NOPULL, GPIO_SPEED_FREQ_VERY_HIGH, GPIO_AF1_TIM1 },
            &htim1,
            TIM_CHANNEL_2 },    // PWM2: PJ11 TIM1_CH2
        { GPIOJ,
            { GPIO_PIN_10, GPIO_MODE_AF_PP, GPIO_NOPULL, GPIO_SPEED_FREQ_VERY_HIGH, GPIO_AF3_TIM8 },
            &htim8,
            TIM_CHANNEL_2 },    // PWM3: PJ10 TIM8_CH2
        { GPIOK,
            { GPIO_PIN_0, GPIO_MODE_AF_PP, GPIO_NOPULL, GPIO_SPEED_FREQ_VERY_HIGH, GPIO_AF3_TIM8 },
            &htim8,
            TIM_CHANNEL_3 },    // PWM4: PK0 TIM8_CH3
};

/*----------------------------------------------------------------------------*/
/*!
 * @brief    Init PWM block and its pins
 *
 */
/*----------------------------------------------------------------------------*/
void bspPwmInit(void)
{
    // GPIO and timer clocks for PWM outputs
    __HAL_RCC_GPIOJ_CLK_ENABLE();
    __HAL_RCC_GPIOK_CLK_ENABLE();
    __HAL_RCC_TIM1_CLK_ENABLE();
    __HAL_RCC_TIM8_CLK_ENABLE();

    uint32_t initFreq = PWM_FREQ_DEFAULT;
    uint32_t prescaler;
    uint32_t period;

    if (!pwmCalcTimerParams(initFreq, &prescaler, &period))
    {
        set_error(ERR_MAN_HW_INIT, ERR_INIT_PWM);
        return;
    }

    // Initialize pwm pins
    for (uint8_t i = 0; i < PWM_MAX; i++)
    {
        GPIO_InitTypeDef pin = pwmPins[i].pin;
        HAL_GPIO_Init(pwmPins[i].port, &pin);

        pwmPins[i].htim->Init            = pwmDefInit;
        pwmPins[i].htim->Init.Prescaler  = prescaler;
        pwmPins[i].htim->Init.Period     = period;

        TIM_OC_InitTypeDef configOC = pwmOutput;
        if (HAL_TIM_PWM_Init(pwmPins[i].htim) != HAL_OK)
        {
            // Timer initialization error
            set_error(ERR_MAN_HW_INIT, ERR_INIT_PWM);
            return;
        }
        if (HAL_TIM_PWM_ConfigChannel(pwmPins[i].htim, &configOC, pwmPins[i].channel) != HAL_OK)
        {
            // Timer channel config error
            set_error(ERR_MAN_HW_INIT, ERR_PWM_CHANNEL_CONFIG);
            return;
        }

        __HAL_TIM_SET_PRESCALER(pwmPins[i].htim, pwmPins[i].htim->Init.Prescaler);
        __HAL_TIM_SET_AUTORELOAD(pwmPins[i].htim, pwmPins[i].htim->Init.Period);
        __HAL_TIM_SET_COMPARE(pwmPins[i].htim, pwmPins[i].channel, 0U);
    }
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief      Starts pwm work
 *
 * @param[in]  pwmNum - Number of PWM outputs to start
 *
 */
/*----------------------------------------------------------------------------*/
void bspPwmStart(uint8_t pwmNum)
{
    if (pwmNum < PWM_MAX && HAL_TIM_PWM_Start(pwmPins[pwmNum].htim, pwmPins[pwmNum].channel) != HAL_OK)
    {
        set_error(ERR_MAN_HW_INIT, ERR_PWM_STARTSTOP);
    }
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief      Stops pwm work
 *
 * @param[in]  pwmNum - Number of PWM outputs to stop PWM work
 *
 */
/*----------------------------------------------------------------------------*/
void bspPwmStop(uint8_t pwmNum)
{
    if (pwmNum < PWM_MAX && HAL_TIM_PWM_Stop(pwmPins[pwmNum].htim, pwmPins[pwmNum].channel) != HAL_OK)
    {
        set_error(ERR_MAN_HW_INIT, ERR_PWM_STARTSTOP);
    }
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief      Set pwm carrier frequency
 *
 * @param[in]  frequency - frequncy to change
 *             PWMfreq = Fclk / ((period + 1)*(prescaler + 1))
 * @param[in]  pwmNum - Number of PWM outputs
 *
 */
/*----------------------------------------------------------------------------*/
void bspPwmSetCarrierFreq(uint8_t pwmNum, uint32_t frequency)
{
    if (pwmNum < PWM_MAX && frequency > 0)
    {
        // Calculate current duty cycle
        uint32_t oldPeriod = __HAL_TIM_GET_AUTORELOAD(pwmPins[pwmNum].htim);
        if (oldPeriod == 0)
        {
            return;
        }
        uint32_t oldPulse  = __HAL_TIM_GET_COMPARE(pwmPins[pwmNum].htim, pwmPins[pwmNum].channel);
        uint8_t  duty      = (uint8_t)(oldPulse * DUTY_MAX / oldPeriod);

        uint32_t prescaler;
        uint32_t period;
        if (!pwmCalcTimerParams(frequency, &prescaler, &period))
        {
            set_error(ERR_MAN_HW_INIT, ERR_INIT_PWM);
            return;
        }

        // Set new frequency
        __HAL_TIM_SET_PRESCALER(pwmPins[pwmNum].htim, prescaler);
        __HAL_TIM_SET_AUTORELOAD(pwmPins[pwmNum].htim, period);
        pwmPins[pwmNum].htim->Instance->EGR = TIM_EGR_UG;
        // Fix duty cycle
        bspPwmSetDutyCycle(pwmNum, duty);
    }
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief      Sets duty cycle of pwmChannel choosen
 *
 * @param[in]  value - duty cycle to set to pwm channel
 * @param[in]  pwmNum - Number of PWM working output
 *             DutyCycle = (CCRx/period)
 *
 */
/*----------------------------------------------------------------------------*/
void bspPwmSetDutyCycle(uint8_t pwmNum, uint32_t value)
{
    uint32_t pwmVal = 0;
    if (pwmNum < PWM_MAX)
    {
        if (value > DUTY_MAX)
        {
            pwmVal = DUTY_MAX;
        }
        else
        {
            pwmVal = value;
        }
        // Get current period
        uint32_t period = __HAL_TIM_GET_AUTORELOAD(pwmPins[pwmNum].htim);
        __HAL_TIM_SET_COMPARE(pwmPins[pwmNum].htim, pwmPins[pwmNum].channel, ((period + 1) * pwmVal) / DUTY_MAX);
    }
}

#ifndef BOOTLOADER
static terminalRet terminalPwmTest(uint8_t argc, char **argv);

/* ---------------------------------------------------------------------------*/
/*!
 * @brief     Adds pwm commands to the list of terminal commands
 *
 */
/*----------------------------------------------------------------------------*/
__attribute__((constructor)) void pwmTestInit(void)
{
    static terminalItem terminalPwmItem = {
        .name     = "pwm",
        .desc     = "Configures PWM channel",
        .help     = "This command allows to change pwm channel configuration\n pwm"
                    "[arg1] [arg2] [arg3]\n"
                    "arg1 - start or stop pwm, duty - change duty cycle(0-100), freq - change carrier frequency of pwm choosen\n"
                    "arg2 - pwm number to work with\n"
                    "arg3 - value of duty cycle or carrier frequency to pwm work(ignored in case of start/stop)\n",
        .callback = terminalPwmTest,
        .next     = NULL
    };
    terminalAddItem(&terminalPwmItem);
}

/* ---------------------------------------------------------------------------*/
/*!
 * @brief         Perform periodic din commaand
 *
 * @param[in]     argc number of command arguments
 * @param[in]     argv command arguments
 * @return        Status of operation
 */
/*----------------------------------------------------------------------------*/
static terminalRet terminalPwmTest(uint8_t argc, char **argv)
{
    // if arguments number is not sufficient for this operation
    if (argc < 3)
    {
        return SHELL_EARGC;
    }
    uint32_t pwmNum;
    if (strcmp(argv[1], "start") == 0)
    {
        if (!terminalStrToInt(argv[2], &pwmNum))
        {
            printf("Cannot convert %s to number\n", argv[2]);
            return SHELL_ECONV;
        }
        bspPwmStart((uint8_t)pwmNum);
    }
    else if (strcmp(argv[1], "stop") == 0)
    {
        if (!terminalStrToInt(argv[2], &pwmNum))
        {
            printf("Cannot convert %s to number\n", argv[2]);
            return SHELL_ECONV;
        }
        bspPwmStop((uint8_t)pwmNum);
    }
    else if (strcmp(argv[1], "duty") == 0)
    {
        uint32_t dutyCycle;
        if (!terminalStrToInt(argv[2], &pwmNum))
        {
            printf("Cannot convert %s to number\n", argv[2]);
            return SHELL_ECONV;
        }
        if (!terminalStrToInt(argv[3], &dutyCycle))
        {
            printf("Cannot convert %s to number\n", argv[3]);
            return SHELL_ECONV;
        }
        bspPwmSetDutyCycle((uint8_t)pwmNum, dutyCycle);
    }
    else if (strcmp(argv[1], "freq") == 0)
    {
        uint32_t carrierFrequency;
        if (!terminalStrToInt(argv[2], &pwmNum))
        {
            printf("Cannot convert %s to number\n", argv[2]);
            return SHELL_ECONV;
        }
        if (!terminalStrToInt(argv[3], &carrierFrequency))
        {
            printf("Cannot convert %s to number\n", argv[3]);
            return SHELL_ECONV;
        }
        bspPwmSetCarrierFreq((uint8_t)pwmNum, carrierFrequency);
    }
    else
    {
        return SHELL_EARG;
    }
    return SHELL_OK;
}
#endif