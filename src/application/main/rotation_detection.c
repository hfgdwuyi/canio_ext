#include <stdio.h>
#include <hal.h>
#include <math.h>
#include <stdbool.h>

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

#include "bsp_board.h"
#include "ism330dlc_reg.h"
#include "MadgwickAHRS.h"

#include "rotation_detection.h"

#define X_AXIS 0
#define Y_AXIS 1
#define Z_AXIS 2

stmdev_ctx_t dev_ctx;

//Offsets to final rotation values
float rollOffset = 0;
float pitchOffset = 0;

//Variables to save axis reassignment
uint8_t x = X_AXIS;
uint8_t y = Y_AXIS;
uint8_t z = Z_AXIS;

//Signs for raw accelerometer and gyro output, applies AFTER axis reassignment
int8_t signX = 1;
int8_t signY = 1;
int8_t signZ = 1;

float calculate_roll(void);
float calculate_pitch(void);

void rotation_detection_init(void)
{
    //Initialization of ISM330DLC driver according to documentation
    dev_ctx.write_reg = platform_write;
    dev_ctx.read_reg = platform_read;
    uint8_t spiNum = SYS_SPI_ISM330_NUMBER;
    dev_ctx.handle = &spiNum;

    //Optimal configuration for Madgwick algorithm
    ism330dlc_xl_data_rate_set(&dev_ctx, ISM330DLC_XL_ODR_1k66Hz);
    ism330dlc_xl_full_scale_set(&dev_ctx, ISM330DLC_16g);
    ism330dlc_gy_data_rate_set(&dev_ctx, ISM330DLC_GY_ODR_1k66Hz);
    ism330dlc_gy_full_scale_set(&dev_ctx, ISM330DLC_2000dps);
    ism330dlc_gy_band_pass_set(&dev_ctx, ISM330DLC_HP_DISABLE_LP1_LIGHT);
}

//Handles the calculation of rotations, should be called once every two milliseconds for Madgwick algorithm to work correctly
void rotation_detection_handler(void)
{
    //Calculate accelerations in g from raw accelerometer output 
    float xAccel = signX * ((float)p401AnalogInput16bit[11 + x] * 0.488 / 1000);
    float yAccel = signY * ((float)p401AnalogInput16bit[11 + y] * 0.488 / 1000);
    float zAccel = signZ * ((float)p401AnalogInput16bit[11 + z] * 0.488 / 1000);

    //Calculate rotation velocities in dps from raw gyroscope output 
    float xAngleVelocity = signX * ((float)p401AnalogInput16bit[8 + x] * 70 / 1000) * M_PI / 180;
    float yAngleVelocity = signY * ((float)p401AnalogInput16bit[8 + y] * 70 / 1000 + 3.36) * M_PI / 180;
    float zAngleVelocity = signZ * ((float)p401AnalogInput16bit[8 + z] * 70 / 1000) * M_PI / 180;

    //Apply Madgwich algorithm
    MadgwickAHRSupdateIMU(xAngleVelocity, yAngleVelocity, zAngleVelocity, xAccel, yAccel, zAccel);

    //Save quaternion output from Madgwick to CANOpen objects
    manIsm330DLCQuaternions[1] = q0;
    manIsm330DLCQuaternions[2] = q1;
    manIsm330DLCQuaternions[3] = q2;
    manIsm330DLCQuaternions[4] = q3;

    //Calcalate roll and pitch from quaternions and apply offsets
    float roll = calculate_roll() + rollOffset;
    float pitch = calculate_pitch() + pitchOffset;

    //Change roll and pitch signs according to user configuration
    roll *= manIsm330DLCOrientConf[2] ? -1 : 1;
    pitch *= manIsm330DLCOrientConf[3] ? -1 : 1;

    //Save roll and pitch to CANOpen objects
    manIsm330DLCRotations[1] = roll;
    manIsm330DLCRotations[2] = pitch;
}

//Calculate offset for roll and pitch
void calibrate_angles(void)
{
    rollOffset = 0 - calculate_roll();
    pitchOffset = 0 - calculate_pitch();
}

//Automatically detect wich axis is "UP" and assign other axis accordingly
void calibrate_orientation(void)
{
    //Calculate accelerations in g from raw accelerometer output
    float xAccel = (float)p401AnalogInput16bit[11] * 0.488 / 1000;
    float yAccel = (float)p401AnalogInput16bit[12] * 0.488 / 1000;
    float zAccel = (float)p401AnalogInput16bit[13] * 0.488 / 1000;

    if(fabsf(zAccel) > fabsf(xAccel) && fabsf(zAccel) > fabsf(yAccel)) {
        //Default configuration, Z is pointing UP/DOWN
        x = X_AXIS;
        y = Y_AXIS;
        z = Z_AXIS;
        if(zAccel >= 0) {
            //Signs when Z is pointing UP
            signX = 1;
            signY = 1;
            signZ = 1;
        }
        else {
            //Signs when Z is pointing DOWN
            signX = 1;
            signY = -1;
            signZ = -1;
        }
    }
    else if(fabsf(xAccel) > fabsf(yAccel)) {
        //Reassign axis so that X is pointing UP/DOWN
        x = Z_AXIS;
        y = Y_AXIS;
        z = X_AXIS;
        if(xAccel >= 0) {
            //Signs when X is pointing UP
            signX = -1;
            signY = 1;
            signZ = 1;
        }
        else {
            //Signs when X is pointing DOWN
            signX = 1;
            signY = 1;
            signZ = -1;
        }
    }
    else {
        //Reassign axis so that Y is pointing UP/DOWN
        x = X_AXIS;
        y = Z_AXIS;
        z = Y_AXIS;
        if(yAccel >= 0) {
            //Signs when Y is pointing UP
            signX = 1;
            signY = -1;
            signZ = 1;
        }
        else {
            //Signs when Y is pointing DOWN
            signX = 1;
            signY = 1;
            signZ = -1;
        }
    }
    //Reset Madgwick algorithm
    q0 = 1.0f, q1 = 0.0f, q2 = 0.0f, q3 = 0.0f;
}

//Calculate roll from quaternions
float calculate_roll(void)
{
    if(manIsm330DLCOrientConf[1]) {
        //Calculating roll as rotation around y axis
        return atan2(2 * (q0 * q2 - q1 * q3), 1 - 2 * (pow(q1, 2) + pow(q2, 2))) * 180 / M_PI;
    }
    else {
        //Calculating roll as rotation around x axis
        return atan2(2 * (q0 * q1 + q2 * q3), 1 - 2 * (pow(q1, 2) + pow(q2, 2))) * 180 / M_PI;
    }
}

// Calculate pitch from quaternions
float calculate_pitch(void)
{
    if(manIsm330DLCOrientConf[1]) {
        //Calculating pitch as rotation around x axis
        return asin(2 * (q0 * q1 + q2 * q3)) * 180 / M_PI;
    }
    else {
        //Calculating pitch as rotation around y axis
        return asin(2 * (q0 * q2 - q1 * q3)) * 180 / M_PI;
    }
}