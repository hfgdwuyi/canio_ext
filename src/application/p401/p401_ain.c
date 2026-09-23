/*!
 * Copyright � Siemens Healthcare GmbH 2022, All Rights Reserved
 *
 * Project: Building Block Low End MCU
 *
 * @file
 * @brief Analog inputs implementation in regard for p401 profile
 *
 */
// Standard includes
#include <stdbool.h>

// HAL includes
#include <hal.h>

// CANOpen includes
#define DEF_HW_PART
#include <cal_conf.h>

#include <co_acces.h>
#include <co_sdo.h>
#include <co_pdo.h>
#include <co_drv.h>
#include <co_lme.h>
#include <co_nmt.h>
#include <co_init.h>
#include <objects.h>

// Project includes
#include "sys_config.h"
#include "bsp_ain.h"
#include "bsp_aout.h"
#include "ain_lut.h"
#include "error.h"
#include "tmp75.h"
#include "ism330dlc.h"
#include "p401_ain.h"

/*! Mask for F&B compatible ADC values */
#define FB_ADC_MASK      0x7FE0U

#define P401_AIN_NUMBER  ((sizeof(p401AnalogInput16bit) / sizeof(p401AnalogInput16bit[0])) - 1)
#define P401_AOUT_NUMBER (sizeof(p401AnalogOutput16bit) / sizeof(p401AnalogOutput16bit[0]) - 1)

#define ISM330_READ_REGS 6
#define P401_TMP75_TPDO2_CHANNEL 7U

/*----------------------------------------------------------------------------*/
/*!
@name           Analog input control variables
@{
*/
/*----------------------------------------------------------------------------*/
/*! Filter factor A */
static REAL32 fl_A[P401_AIN_NUMBER];
/*! Filter factor B */
static REAL32 fl_B[P401_AIN_NUMBER];
/*!
@}
*/

INTEGER32 ainValue[P401_AIN_NUMBER];

typedef void (*p401CalcFunc)(INTEGER16 *dest, UNSIGNED8 param);

static void p401RawValue(INTEGER16 *dest, UNSIGNED8 index);
static void p401Tmp75(INTEGER16 *dest, UNSIGNED8 index);
static void p401Tmp75(INTEGER16 *dest, UNSIGNED8 param);
static void p401Temp(INTEGER16 *dest, UNSIGNED8 index);
static void p401Ism330(INTEGER16 *dest, UNSIGNED8 reg);
static void p401PumpCurrent(INTEGER16 *dest, UNSIGNED8 index);

static const struct
{
    p401CalcFunc func;     // Function to calculate value
    uint8_t      param;    // Parameter to pass to the function
} p401Calc[] = {
    /*Sub 1 */ { p401Tmp75, 0 },                        // Temperature from tmp75 sensor
    /*Sub 2 */ { p401RawValue, 1 },                     // Force sensor X
    /*Sub 3 */ { p401RawValue, 2 },                     // Force sensor Y
    /*Sub 4 */ { p401RawValue, 3 },                     // Force sensor Z
    /*Sub 5 */ { p401RawValue, 4 },                     // FD Temperature from LUT
    /*Sub 6 */ { p401RawValue, 5 },                     // MB Temperature from LUT
    /*Sub 7 */ { p401Tmp75, 0 },                        // Temperature from tmp75 sensor
    /*Sub 8 */ { p401Ism330, REG_ISM330_OUTX_L_G },     // ISM330 Gyroscope X
    /*Sub 9 */ { p401Ism330, REG_ISM330_OUTY_L_G },     // ISM330 Gyroscope Y
    /*Sub 10*/ { p401Ism330, REG_ISM330_OUTZ_L_G },     // ISM330 Gyroscope Z
    /*Sub 11*/ { p401Ism330, REG_ISM330_OUTX_L_XL },    // ISM330 Accelerometer X
    /*Sub 12*/ { p401Ism330, REG_ISM330_OUTY_L_XL },    // ISM330 Accelerometer X
    /*Sub 13*/ { p401Ism330, REG_ISM330_OUTZ_L_XL },    // ISM330 Accelerometer X
    /*Sub 14*/ { p401PumpCurrent, 0 },                  // Pump current in mA
    /*Sub 15*/ { p401Temp, 4 },                         // FD Temperature in 0.1 Celsius
    /*Sub 16*/ { p401Temp, 5 },                         // MB Temperature in 0.1 Celsius
    /*Sub 17*/ { p401RawValue, 6 },                     // For test only, 2.2V
    /*Sub 18*/ { p401RawValue, 7 },                     // For test only, 2.5V
    /*Sub 19*/ { NULL, 0 },
    /*Sub 20*/ { NULL, 0 },
};

/*----------------------------------------------------------------------------*/
/*!
 @brief         Raw ADC value

 @param[out]    dest Pointer to destination variable

 @return        Value in ADC units
*/
/*----------------------------------------------------------------------------*/
static void p401RawValue(INTEGER16 *dest, UNSIGNED8 index)
{
    if (dest == NULL)
    {
        return;
    }
    *dest = (INTEGER16)ainValue[index];
}

/*----------------------------------------------------------------------------*/
/*!
 @brief         Calculate value in 0.1 Celsius degrees

 @param[out]    dest Pointer to destination variable
 @param[in]     index Index in ADC table

 @return        Temperature value in 0.1 Celsius degrees
*/
/*----------------------------------------------------------------------------*/
static void p401Temp(INTEGER16 *dest, UNSIGNED8 index)
{
    if (dest == NULL)
    {
        return;
    }

    uint32_t adcTemp = ainValue[index];
    // Scale to 12bit
    adcTemp >>= 4;

    // Open load => Temperature 0C
    if (adcTemp <= 10)
    {
        *dest = 0;
        return;
    }
    // Short circuit => Temperature 100C
    if (adcTemp >= 4080)
    {
        *dest = 100;
        return;
    }

    double adcTempDouble = (double)adcTemp;

    double tempDouble = adcTempDouble * adcTempDouble * adcTempDouble;
    tempDouble *= 0.000000001436;
    tempDouble -= 0.000008359597 * adcTempDouble * adcTempDouble;
    tempDouble += 0.034387306913 * adcTempDouble;
    tempDouble += 0.318184890380;

    *dest = (INTEGER16)(tempDouble * 10);
}

/*----------------------------------------------------------------------------*/
/*!
 @brief         Read values from tmp75 sensor

 @param[out]    dest Pointer to destination variable

 @return        Temperature value in Celsius degrees
*/
/*----------------------------------------------------------------------------*/
static void p401Tmp75(INTEGER16 *dest, UNSIGNED8 param)
{
    if (dest == NULL)
    {
        return;
    }

    (void)param;
    static uint32_t counter = 0;

    // The sensor need time to convert the value
    if (++counter >= SYS_TMP75_PERIOD_MS)
    {
        int16_t tmp = 0;
        counter     = 0;
        // Read temperature
        if (tmp75ReadTemperature(&sensorTMP, &tmp) == TMP75_RET_OK)
        {
            clear_error(ERR_DESC_SUBINDEX_HW_INIT, ERR_TPM75_STATUS_RW);
            *dest = tmp;
        }
        else
        {
            set_error(ERR_MAN_HW_INIT, ERR_TPM75_STATUS_RW);
        }
    }
}

/*----------------------------------------------------------------------------*/
/*!
 @brief         Read data from ISM330 accelerometer

 @param[out]    dest Pointer to destination variable
 @param[in]     reg Register to read

 @return        ISM330 register value
*/
/*----------------------------------------------------------------------------*/
static void p401Ism330(INTEGER16 *dest, UNSIGNED8 reg)
{
    if (dest == NULL)
    {
        return;
    }

    static uint32_t timeCounter = 0;
    static uint32_t readCounter = 0;

    if (++timeCounter < SYS_ISM300_PERIOD_MS)
    {
        // Data not ready yet, return
        return;
    }

    uint16_t data = 0;
    // Read accelerometer data
    if (ism330dlcReadRegister16bit(SYS_SPI_ISM330_NUMBER, reg, &data) == RET_ISM330_OK)
    {
        clear_error(ERR_MAN_HW_INIT, ERR_ISM330_STATUS_RW);
        *dest = data;
    }
    else
    {
        set_error(ERR_MAN_HW_INIT, ERR_ISM330_STATUS_RW);
    }

    // Check if all registers were read
    if (++readCounter >= ISM330_READ_REGS)
    {
        timeCounter = 0;
        readCounter = 0;
    }
}

/*----------------------------------------------------------------------------*/
/*!
 @brief         Pump current in mA

 @param[out]    dest Pointer to destination variable
 @param[in]     index Index in ADC table

 @return        Current in 1 mA
*/
/*----------------------------------------------------------------------------*/
static void p401PumpCurrent(INTEGER16 *dest, UNSIGNED8 index)
{
    if (dest == NULL)
    {
        return;
    }

    // Get raw ADC value
    uint32_t adcVal = ainValue[index];
    // Scale to 12bit
    adcVal >>= 4;
    *dest = pumpCurrentLUT[adcVal];
}

/*----------------------------------------------------------------------------*/
/*!
 @brief         Read values from AI subsystem and put them to CANOpen objects

 @return        None
*/
/*----------------------------------------------------------------------------*/
void p401ReadAI(void)
{
    static REAL32 prevFilteredADCValue[P401_AIN_NUMBER];

    // Filter real ADCs
    for (uint8_t i = 0; i < AIN_NUMBER; i++)
    {
        /*Analog filtration according to formula
         Y(uc_Channel_Idx) = T/(T + dT)*Y(uc_Channel_Idx - 1) + dT/(T + dT)*X(uc_Channel_Idx)
         where X(uc_Channel_Idx) = current ADC value,
         Y(uc_Channel_Idx) = current filtered value,
         Y(uc_Channel_Idx-1) = previous filtered value*/
        uint32_t rawADCValue          = bspAinGetRawValue(i);
        REAL32   currFilteredADCValue = (fl_A[i] * prevFilteredADCValue[i]) + (fl_B[i] * (REAL32)rawADCValue);
        // Save value for the next calculation step
        prevFilteredADCValue[i] = currFilteredADCValue;
        // Apply offset and prescaling
        ainValue[i] = ((INTEGER32)currFilteredADCValue + p401AIOffset[i + 1]) * p401AIPrescale[i + 1];
    }

    // Fill Canopen objects
    for (uint8_t i = 0; i < P401_AIN_NUMBER; i++)
    {
        if (p401Calc[i].func != NULL)
        {
            // Set object value
            p401Calc[i].func(&p401AnalogInput16bit[i + 1], p401Calc[i].param);
        }
    }
}

/*----------------------------------------------------------------------------*/
/*!
 @brief         Calculate parameters for AI filter

                Is called each time when external SW wants to change AIN
                filter time constants


 @return         None
*/
/*----------------------------------------------------------------------------*/
void p401AIFilterCalculate(void)
{
    for (uint8_t i = 0; i < P401_AIN_NUMBER; i++)
    {
        // T/(T+dT)
        fl_A[i] = manAIFilterTime[i + 1] / (manAIFilterTime[i + 1] + 1.0f);
        // dT/(T+dT)
        fl_B[i] = 1.0f / (manAIFilterTime[i + 1] + 1.0f);
    }
}

/*----------------------------------------------------------------------------*/
/*!
 @brief          Calculates the deltas for AIN interrupts

 @return         true, if delta interuupt should be triggered, false otherwise
*/
/*----------------------------------------------------------------------------*/
static bool p401DeltaCalculate(uint8_t channel, uint32_t currValue, uint32_t prevValue)
{
    // Interrupt trigger for deltas
    bool deltaTrigger = false;

    // Difference between current and previous value
    uint32_t delta;
    // Negative flag
    bool neg = false;
    if (currValue >= prevValue)
    {
        delta = currValue - prevValue;
    }
    else
    {
        delta = prevValue - currValue;
        neg   = true;
    }

    // Delta
    if (delta >= p401AIIRQDelta[channel])
    {
        deltaTrigger = true;
    }

    // Delta negative
    if (neg && (delta >= p401AIIRQDeltaNeg[channel]))
    {
        deltaTrigger = true;
    }

    // Delta positive
    if (!neg && (delta >= p401AIIRQDeltaPos[channel]))
    {
        deltaTrigger = true;
    }

    return deltaTrigger;
}

/*----------------------------------------------------------------------------*/
/*!
 @brief         Checks if PDO conditions are met

                This function does processing of AINs according to conditions
                of logical triggers or "interrupts" described in CiA DS401. IRQs
                are just flags for PDO sending and not real physical IRQs

 @return        0 if no interrupt conditions are met, and not 0 otherwise.
*/
/*----------------------------------------------------------------------------*/
uint32_t p401AIInterruptsCalculate(void)
{
    // Local array containing previous analog input values
    static int16_t prevValue[P401_AIN_NUMBER] = { 0 };
    static bool    tmp75Tpdo2AlarmActive      = false;
    // Return value variable
    uint32_t ret = 0;

    // Check if AI interrupts enabled
    if (p401AIIRQEnable == CO_FALSE)
    {
        return 0;
    }

    for (uint8_t i = 1; i < P401_AIN_NUMBER; i++)
    {
        // Interrupt trigger for limits
        bool limitTrigger = false;
        // Upper limit trigger
        bool upperLimit = false;
        // Lower limit trigger
        bool lowerLimit = false;

        // Check upper limit
        if (p401AnalogInput16bit[i] >= p401AIIRQUpperLimit[i])
        {
            upperLimit = true;
        }

        // Check lower limit
        if (p401AnalogInput16bit[i] < p401AIIRQLowerLimit[i])
        {
            lowerLimit = true;
        }

        // XOR is the requirement of DS401 (XOR operator switched for != for bool variables)
        limitTrigger = (upperLimit != lowerLimit);

        if (i == P401_TMP75_TPDO2_CHANNEL)
        {
            bool trigger = false;

            if (!tmp75Tpdo2AlarmActive && (p401AnalogInput16bit[i] >= p401AIIRQUpperLimit[i]))
            {
                tmp75Tpdo2AlarmActive = true;
                trigger               = true;
            }
            else if (tmp75Tpdo2AlarmActive && (p401AnalogInput16bit[i] < p401AIIRQLowerLimit[i]))
            {
                tmp75Tpdo2AlarmActive = false;
                trigger               = true;
            }

            if (trigger)
            {
                ret |= 1UL << (i - 1U);
                prevValue[i - 1] = p401AnalogInput16bit[i];
            }

            continue;
        }

        // Interrupt trigger for deltas
        bool deltaTrigger = p401DeltaCalculate(i, (uint32_t)p401AnalogInput16bit[i], (uint32_t)prevValue[i - 1]);

        // According to DS401 AND operation must be performed
        if (limitTrigger && deltaTrigger)
        {
            ret |= 1UL << (i - 1U);
            // To remember the last communicated AIN for Delta calculations
            prevValue[i - 1] = p401AnalogInput16bit[i];
        }
    }

    return ret;
}
