/*!
 * Copyright � Siemens Healthcare GmbH 2022, All Rights Reserved
 *
 * Project: Building Block Low End MCU
 *
 *  @file
 *  @brief          Functions for TMP75 sensor read, write and control operations
 */
/*----------------------------------------------------------------------------*/
// Standard includes
#include <string.h>
// HAL includes
#include <hal.h>
// Project includes
#include "bsp_board.h"
#include "tmp75.h"
#ifdef TMP75_TEST
#include <stdio.h>
#include "terminal.h"
#endif
/* Register number defines*/
#define TMP75_REG_TEMPERATURE 0
#define TMP75_REG_CONFIG      1
#define TMP75_REG_T_LOW       2
#define TMP75_REG_T_HIGH      3


tmp75Sensor sensorTMP = {
    .i2c     = 1,
    .address = 0x48,
};

static int16_t tmp75TempToCelsius(uint16_t data, uint8_t resolution);
static bool    tmp75I2CTransfer(const tmp75Sensor *sensor, uint8_t *txBuf, uint32_t txSize, uint8_t *rxBuf, uint32_t rxSize);

/*----------------------------------------------------------------------------*/
/*!
 @brief      Reads configuration from TMP75 sensor

 @param[out]  config Pointer to value to be filled

 @return     operation result
*/
/*----------------------------------------------------------------------------*/
tmp75Ret tmp75ReadConfig(const tmp75Sensor *sensor, tmp75Config *config)
{
    uint8_t reg = TMP75_REG_CONFIG;

    if (!tmp75I2CTransfer(sensor, &reg, 1, (uint8_t *)config, 1))
    {
        return TMP75_RET_I2C_ERROR;
    }

    return TMP75_RET_OK;
}

/*----------------------------------------------------------------------------*/
/*!
 @brief      Writes configuration to TMP75 sensor

 @param[in]  config configuration to write

 @return     operation result
*/
/*----------------------------------------------------------------------------*/
tmp75Ret tmp75WriteConfig(const tmp75Sensor *sensor, tmp75Config config)
{
    uint8_t data[2] = { TMP75_REG_CONFIG, *(uint8_t *)&config };

    if (!tmp75I2CTransfer(sensor, data, 2, NULL, 0))
    {
        return TMP75_RET_I2C_ERROR;
    }

    return TMP75_RET_OK;
}

/*----------------------------------------------------------------------------*/
/*!
 @brief      Reads temperature limit from TMP75 sensor

 @param[out]  limit Pointer to value to be filled
 @param[in]   high read high limit if true, low limit if false

 @return     operation result
*/
/*----------------------------------------------------------------------------*/
tmp75Ret tmp75ReadLimit(const tmp75Sensor *sensor, int16_t *limit, bool high)
{
    // Read config - necessary for temperature calculation
    tmp75Config config;
    tmp75Ret    ret = tmp75ReadConfig(sensor, &config);
    if (ret != TMP75_RET_OK)
    {
        return ret;
    }

    // Read value
    uint8_t  reg = high ? TMP75_REG_T_HIGH : TMP75_REG_T_LOW;
    uint16_t data;

    if (!tmp75I2CTransfer(sensor, &reg, 1, (uint8_t *)&data, 2))
    {
        return TMP75_RET_I2C_ERROR;
    }
    // Flip bytes
    data = (uint16_t)((data >> 8) | (uint16_t)(data << 8));

    // Calculate temperature in Celsius
    *limit = tmp75TempToCelsius(data, config.resolution);

    return TMP75_RET_OK;
}

/*----------------------------------------------------------------------------*/
/*!
 @brief      Reads temperature limit from TMP75 sensor

 @param[out]  limit Value to be set
 @param[in]   high read high limit if true, low limit if false

 @return     operation result
*/
/*----------------------------------------------------------------------------*/
tmp75Ret tmp75WriteLimit(const tmp75Sensor *sensor, int16_t limit, bool high)
{
    // No negative values here
    if (limit < 0)
    {
        return TMP75_RET_WRONG_VALUE;
    }

    // Set register
    uint8_t reg = high ? TMP75_REG_T_HIGH : TMP75_REG_T_LOW;

    // Calculate register value
    uint32_t value = (uint16_t)limit;

    // Limits are always 12 bits, adjust to right
    value <<= 8;
    // Fill the write buffer
    uint8_t buf[] = { reg, (uint8_t)(value >> 8), (uint8_t)value };

    if (!tmp75I2CTransfer(sensor, buf, sizeof(buf), NULL, 0))
    {
        return TMP75_RET_I2C_ERROR;
    }

    return TMP75_RET_OK;
}

/*----------------------------------------------------------------------------*/
/*!
 @brief      Reads temperature from TMP75 sensor

 @param[out]  temperature Pointer to value to be filled

 @return     operation result
*/
/*----------------------------------------------------------------------------*/
tmp75Ret tmp75ReadTemperature(const tmp75Sensor *sensor, int16_t *temperature)
{
    // Read config - necessary for temperature calculation
    tmp75Config config;
    tmp75Ret    ret = tmp75ReadConfig(sensor, &config);
    if (ret != TMP75_RET_OK)
    {
        return ret;
    }

    // Read value
    uint8_t  reg = TMP75_REG_TEMPERATURE;
    uint16_t data;

    if (!tmp75I2CTransfer(sensor, &reg, 1, (uint8_t *)&data, 2))
    {
        return TMP75_RET_I2C_ERROR;
    }
    // Flip bytes
    data = (uint16_t)((data >> 8) | (((uint32_t)data) << 8));

    // Calculate temperature in Celsius
    *temperature = tmp75TempToCelsius(data, config.resolution);

    return TMP75_RET_OK;
}

/*----------------------------------------------------------------------------*/
/*!
 @brief      Recalculates temperature value from TMP75 to Celsius degrees

 @param[in]  data Temperature value to convert
 @param[in]  resolution Resolution, one of TMP75_RESOLUTION_*

 @return     Temperature in Celcius degrees
*/
/*----------------------------------------------------------------------------*/
static int16_t tmp75TempToCelsius(uint16_t data, uint8_t resolution)
{
    int16_t  temp;
    uint8_t  activeBits;
    uint8_t  divValue;
    uint16_t maxValue;

    switch (resolution)
    {
        case TMP75_RESOLUTION_10_BIT:
            activeBits = 10;
            divValue   = 4;
            maxValue   = 0x3FF;
            break;
        case TMP75_RESOLUTION_11_BIT:
            activeBits = 11;
            divValue   = 8;
            maxValue   = 0x7FF;
            break;
        case TMP75_RESOLUTION_12_BIT:
            activeBits = 12;
            divValue   = 16;
            maxValue   = 0xFFF;
            break;
        default:
            activeBits = 9;
            divValue   = 2;
            maxValue   = 0x1FF;
            break;
    }

    uint16_t dataVal = data >> (uint8_t)(((uint8_t)16) - activeBits);
    if ((dataVal & (1U << (uint16_t)(activeBits - 1))) != 0)
    {
        temp = (int16_t)(maxValue - dataVal) + 1;
        temp *= -1;
    }
    else
    {
        temp = (int16_t)dataVal;
    }

    temp /= divValue;
    return temp;
}

static bool tmp75I2CTransfer(const tmp75Sensor *sensor, uint8_t *txBuf, uint32_t txSize, uint8_t *rxBuf, uint32_t rxSize)
{
    return boardI2CTransfer(sensor->i2c, ((uint16_t)sensor->address << 1U), txBuf, (uint16_t)txSize, rxBuf, (uint16_t)rxSize);
}


#ifdef TMP75_TEST


static terminalRet terminalTmp75(uint8_t argc, char **argv);

/* ---------------------------------------------------------------------------- */
/*!
 @brief         Initialize terminal item for temperature sensor testing

 @param[in]     argc number of command arguments
 @param[in]     argv command arguments

 @return        Status of operation
*/
/* ---------------------------------------------------------------------------- */
__attribute__((constructor)) static void terminalTmp75Init(void)
{
    static terminalItem terminalTmp75Item = { .name     = "tmp75",
                                              .desc     = "Read temperature from TMP75 sensor",
                                              .help     = "Read temperature from TMP75 sensor \n\n  tmp75 ",
                                              .callback = terminalTmp75,
                                              .next     = NULL };
    terminalAddItem(&terminalTmp75Item);
}

/*----------------------------------------------------------------------------*/
/*!
 @brief         Reads TMP75 temperature register

 @return        None
*/
/*----------------------------------------------------------------------------*/
static terminalRet terminalTmp75(uint8_t argc, char **argv)
{
    if (argc < 3)
    {
        return SHELL_EARGC;
    }
    int16_t  value = 0;
    tmp75Ret ret;
    if (strcmp(argv[1], "read") == 0)
    {
        if (strcmp(argv[2], "temp") == 0)
        {
            ret = tmp75ReadTemperature(&sensorTMP, &value);
            if (ret == TMP75_RET_OK)
            {
                printf("Temperature: %d C \n", value);
            }
        }
        else if (strcmp(argv[2], "config") == 0)
        {
            ret = tmp75ReadConfig(&sensorTMP, (tmp75Config *)&value);
            if (ret == TMP75_RET_OK)
            {
                printf("Config: 0x%02X\n", value);
            }
        }
        else if (strcmp(argv[2], "tlow") == 0)
        {
            ret = tmp75ReadLimit(&sensorTMP, &value, false);
            if (ret == TMP75_RET_OK)
            {
                printf("Low limit: %d C \n", value);
            }
        }
        else if (strcmp(argv[2], "thigh") == 0)
        {
            ret = tmp75ReadLimit(&sensorTMP, &value, true);
            if (ret == TMP75_RET_OK)
            {
                printf("High limit: %d C \n", value);
            }
        }
        else
        {
            printf("Unknown argument: %s\n", argv[2]);
            return SHELL_EARG;
        }
        if (ret != TMP75_RET_OK)
        {
            printf("Read error\n");
            return SHELL_EHW;
        }
    }
    else if (strcmp(argv[1], "write") == 0)
    {
        uint32_t data;
        // Third argument is value
        if (!terminalStrToInt(argv[3], &data))
        {
            printf("Cannot convert %s to number\n", argv[3]);
            return SHELL_ECONV;
        }
        if (strcmp(argv[2], "config") == 0)
        {
            ret = tmp75WriteConfig(&sensorTMP, *((tmp75Config *)&data));
        }
        else if (strcmp(argv[2], "tlow") == 0)
        {
            ret = tmp75WriteLimit(&sensorTMP, (int16_t)data, false);
        }
        else if (strcmp(argv[2], "thigh") == 0)
        {
            ret = tmp75WriteLimit(&sensorTMP, (int16_t)data, true);
        }
        else
        {
            printf("Unknown argument: %s\n", argv[2]);
            return SHELL_EARG;
        }
        if (ret != TMP75_RET_OK)
        {
            printf("Read error\n");
            return SHELL_EHW;
        }
        else
        {
            printf("Value written\n");
        }
    }

    return SHELL_OK;
}

#endif
