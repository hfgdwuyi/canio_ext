/*!
 * Copyright © Siemens Healthcare GmbH 2022, All Rights Reserved
 *
 * Project: Building Block Low End MCU
 *
 *  @file
 *  @brief          header file for tmp75.c
 */
/*----------------------------------------------------------------------------*/
#ifndef TMP75_H
#define TMP75_H

#define TMP75_RESOLUTION_9_BIT  0
#define TMP75_RESOLUTION_10_BIT 1
#define TMP75_RESOLUTION_11_BIT 2
#define TMP75_RESOLUTION_12_BIT 3

typedef enum
{
    TMP75_RET_OK = 0,         //<! Operation successful
    TMP75_RET_WRONG_REG,      //<! Wrong register used
    TMP75_RET_I2C_ERROR,      //<! I2C bus error
    TMP75_RET_WRONG_VALUE,    //<! Wrong function parameter
    TMP75_RET_READ_ONLY       //<! Read only register is used to writing
} tmp75Ret;

typedef struct tmp75Config
{
    uint8_t shutdown : 1;
    uint8_t thermostat : 1;
    uint8_t polarity : 1;
    uint8_t fault_queue : 2;
    uint8_t resolution : 2;
    uint8_t one_shot : 1;
} tmp75Config;

typedef struct tmp75Sensor
{
    uint8_t i2c;
    uint8_t address;
} tmp75Sensor;

extern tmp75Sensor sensorTMP;

tmp75Ret tmp75ReadConfig(const tmp75Sensor *sensor, tmp75Config *config);
tmp75Ret tmp75WriteConfig(const tmp75Sensor *sensor, tmp75Config config);
tmp75Ret tmp75ReadLimit(const tmp75Sensor *sensor, int16_t *limit, bool high);
tmp75Ret tmp75WriteLimit(const tmp75Sensor *sensor, int16_t limit, bool high);
tmp75Ret tmp75ReadTemperature(const tmp75Sensor *sensor, int16_t *temperature);


#endif    // TMP75
