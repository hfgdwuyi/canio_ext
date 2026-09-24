/*!
 * Copyright � Siemens Healthcare GmbH 2022, All Rights Reserved
 *
 * Project: Building Block Low End MCU
 *
 * @file
 * @brief Main function
 */
/*----------------------------------------------------------------------------*/
// Standard includes
#include <stdio.h>
#include <math.h>
#include <limits.h>
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
#include <co_odidx.h>
#include <co_init.h>
#include <objects.h>
#include <co_setcp.h>

// Project includes
#include "version.h"
#include "sysinit.h"
#include "bsp_board.h"
#include "bsp_led.h"
#include "bsp_dio.h"
#include "bsp_ain.h"
#include "bsp_aout.h"
#include "timing.h"
#include "crc_calc.h"
#include "prog_ctrl.h"
#include "terminal.h"
#include "p401_ain.h"
#include "p401_aout.h"
#include "p401_din.h"
#include "p401_dout.h"
#include "can_cfg.h"
#include "man_pwm.h"
#include "bsp_pwm.h"
#include "storage.h"
#include "error.h"
#include "tmp75.h"
//#include "ism330dlc.h"
#include "ism330dlc_reg.h"
#include "tlc591x.h"
#include "bsp_wtdg.h"
#include "sys_config.h"
#include "rotation_detection.h"
#include "MadgwickAHRS.h"

void appSyncWatchdogProcess(void);

/*----------------------------------------------------------------------------*/
/*!
 * @brief  Terminal command: print the version of the running firmware.
 *
 */
/*----------------------------------------------------------------------------*/
static terminalRet terminalVersion(uint8_t argc, char **argv)
{
    (void)argc;
    (void)argv;
    printf("FW %s, built %s %s\n", APP_SW_VERSION_STR, __DATE__, __TIME__);
    return SHELL_OK;
}

static terminalItem terminalVersionItem = { .name     = "version",
                                            .desc     = "Print firmware version",
                                            .help     = "version\n",
                                            .callback = terminalVersion,
                                            .next     = NULL };

__attribute__((constructor)) void versionCmdInit(void)
{
    terminalAddItem(&terminalVersionItem);
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief Initializes the Global MSP.
 *
 */
/*----------------------------------------------------------------------------*/
void HAL_MspInit(void)
{
    __HAL_RCC_SYSCFG_CLK_ENABLE();
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief  LED blinking (alive sign)
 *
 */
/*----------------------------------------------------------------------------*/
static void blinkHandler(void)
{
    ledToggle(SYSTEM_OK_LED_NUM);
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief  DINs and AINs reading
 *
 */
/*----------------------------------------------------------------------------*/
static void ioHandler(void)
{
    // Read DI state
    p401ReadDI();

    // Check if conditions for PDO sending met
    if (p401CheckForPdo())
    {
        (void)writePdoReq(1);
    }

    // Read AI state
    p401ReadAI();

    // Check if conditions for PDO sending met
    uint32_t ret = p401AIInterruptsCalculate();
    for (UNSIGNED8 i = 0; i < sizeof(p301_n2_tpdo_map.map) / sizeof(p301_n2_tpdo_map.map[0]); i++)
    {
        UNSIGNED32 mapEntry = p301_n2_tpdo_map.map[i];
        if (mapEntry == 0U)
        {
            continue;
        }

        UNSIGNED8 subindex = (mapEntry & 0x0000FF00UL) >> 8;
        if ((subindex == 0U) || (subindex > 32U))
        {
            continue;
        }

        if ((ret & (1UL << (subindex - 1U))) != 0)
        {
            (void)writePdoReq(2);
            // PDO is already issued, no need to check further
            break;
        }
    }
    uint32_t mask= ERR_CANOPEN_BUS_OFF | ERR_CANOPEN_CHGSTATE_STOPPED;
    // Check if error occurs
    if ((manErrorDesc[ERR_DESC_SUBINDEX_CANOPEN] & mask) != 0)
    {
        p401SetDefaultDACValues();
    }
}

/*----------------------------------------------------------------------------*/
/*!
 * \fn           static void updateODValues(void)
 * \brief        Reassignment of variable objects due to the fact that they
 *               were reset in init_Library().
 */
/*----------------------------------------------------------------------------*/
static void updateODValues(void)
{
    RET_T      ret;
    void      *dest;
    void      *src;
    UNSIGNED32 dstSize;
    UNSIGNED32 srcSize;

    // The SW version is owned by the running firmware image: overwrite the
    // stored value here (after loadParameterInd) so EEPROM cannot override it.
    snprintf((char *)manSWVersion, sizeof(manSWVersion), "%s", APP_SW_VERSION_STR);

    // Copy Manufacturer Software Version to object 0x100A
    ret = getObjAddr(0x200A, 0, (UNSIGNED8 **)&src, &srcSize CO_COMMA_LINE_PARA);
    ret |= getObjAddr(0x100A, 0, (UNSIGNED8 **)&dest, &dstSize CO_COMMA_LINE_PARA);
    if (ret == CO_OK)
    {
        memcpy(dest, src, dstSize);
    }

    // Copy part number, revision number and serial number to object 0x1018
    for (uint8_t i = 2; i <= 4; i++)
    {
        ret = getObjAddr(0x2018, i, (UNSIGNED8 **)&src, &srcSize CO_COMMA_LINE_PARA);
        ret |= getObjAddr(IDENTITY_INDEX, i, (UNSIGNED8 **)&dest, &dstSize CO_COMMA_LINE_PARA);
        if (ret == CO_OK)
        {
            memcpy(dest, src, dstSize);
        }
    }

    // Copy Manufacturer Device Name to object 0x1008
    ret = getObjAddr(0x2008, 0, (UNSIGNED8 **)&src, &srcSize CO_COMMA_LINE_PARA);
    ret |= getObjAddr(MANUFACTURER_DEVICE_NAME_INDEX, 0, (UNSIGNED8 **)&dest, &dstSize CO_COMMA_LINE_PARA);
    if (ret == CO_OK)
    {
        memcpy(dest, src, dstSize);
    }

    // Copy Manufacturer Hardware Version to object 0x1009
    ret = getObjAddr(0x2009, 0, (UNSIGNED8 **)&src, &srcSize CO_COMMA_LINE_PARA);
    ret |= getObjAddr(MANUFACTURER_HARDWARE_VERSION_INDEX, 0, (UNSIGNED8 **)&dest, &dstSize CO_COMMA_LINE_PARA);
    if (ret == CO_OK)
    {
        memcpy(dest, src, dstSize);
    }
    // Redefine TPDO5 - TPDO7 to have correct cobIds
    for (uint8_t i = 4; i < 8; i++)
    {
        UNSIGNED32 *destin;
        UNSIGNED32  size;
        // Get current cobId
        (void)getObjAddr(TPDO_PARA_BASE_INDEX + i, 1, (UNSIGNED8 **)&destin, &size);
        UNSIGNED32 cobId = *destin;
        // disable PDO
        (void)setCobId(TPDO_PARA_BASE_INDEX + i, 1, PDO_NO_VALID_BIT);
        // Set new cobId and enable PDO
        (void)setCobId(TPDO_PARA_BASE_INDEX + i, 1, cobId + nodeId);
    }

    // Turn off heartbeat despite the value stored in EEPROM
    p301_prod_hb_time = 0;
    (void)setCommPar(HEARTBEAT_PROD_INDEX, 0);
}

/*----------------------------------------------------------------------------*/
/*!
 * \fn             int main(void)
 * \brief          Main function.
 *
 *
 */
/*----------------------------------------------------------------------------*/
int main(void)
{
    // Enable the CPU Cache
    SystemCache_Config();

    // Initialize HAL
    HAL_Init();


    // Initialize System Clock
    SystemClock_Config();
    // Initialize pins and interfaces
    boardInit();
    // Initialize external watchdog
    WTDG_Init();
    // Feed external watchdog
    WTDG_Feed();
    // Initialize storage
    for (UNSIGNED8 i = 0; i < storageSegmentsCount; i++)
    {
        storageSegmentInit(i);
    }
    WTDG_Feed();
    // Enable CRC module
    crcInit();
    WTDG_Feed();

    printf("Application started\n");
    printf("Built %s %s\n", __DATE__, __TIME__);
    printf("FW %s\n", APP_SW_VERSION_STR);
    // Copy CAN config pins state to the OD's variable
    canCfgSetCode();
    ENABLE_CPU_INTERRUPTS();
    // Read CAN settings
    bool bRet = canCfgGetConfig();
    WTDG_Feed();
    // Initialize CAN controller
    UNSIGNED8 ret = initCan(bitRate);
    printf("Init CAN: %d\n", ret);
    WTDG_Feed();
    // Initialize CANOpen library
    RET_T commonRet = init_Library();
    printf("Init library: %d\n", commonRet);
    WTDG_Feed();
    // Initialize CANOpen timer
    (void)initTimer();
    WTDG_Feed();
    // Set up debouncing for digital inputs
    for (uint8_t i = 0; i < dinMax; i++)
    {
        bspDinSettings dset = { .deb_time = manDIFilterTime[i + 1] };
        uint8_t        byte = i / CHAR_BIT;
        uint8_t        bit  = i % CHAR_BIT;
        dset.deb_en         = (p401DIFilterConstant8Bit[byte + 1] & (1U << bit)) != 0 ? true : false;
        bspDinSetDebouncing(i, dset);
    }
    // Initialize digital inputs and outputs
    bspDioInit();
    WTDG_Feed();

    // Set initial state for digital outputs
    for (uint8_t i = 0; i < doutMax; i++)
    {
        uint8_t bit         = i % CHAR_BIT;
        uint8_t byte        = i / CHAR_BIT;
        bool    pinDefState = ((p401DOWrite8Bit[byte + 1] ^ p401DOPolarity8Bit[byte + 1]) & (1U << bit)) == 0 ? false : true;
        bspDoutSet(i, pinDefState);
    }

    // Initialize LED drivers
    tlc591xInit(tlcLedDriverL);
    tlc591xInit(tlcLedDriverR);
    WTDG_Feed();
    // Initialize AINs
    bspAinInit();
    WTDG_Feed();
    p401AIFilterCalculate();
    // Initialize PWMs
    bspPwmInit();
    WTDG_Feed();

    // Initialize DAC
    bspAoutInit();
    WTDG_Feed();


    Start_CAN();
    ENABLE_CPU_INTERRUPTS();
    WTDG_Feed();

    // Set status and error registers in case of CAN settings error
    if (!bRet)
    {
        printf("CAN settings error. Default values were set\n");
        set_error(ERR_MAN_STORAGE, ERR_STORAGE_ASSET);
    }

    timingTimer blinkTimer;
    timingAddTimer(&blinkTimer, TIMING_TIMER_CYCLIC, 500, blinkHandler);

    // Update asset data
    updateODValues();

    // Timer for periodical input read
    timingTimer ioTimer;
    timingAddTimer(&ioTimer, TIMING_TIMER_CYCLIC, 1, ioHandler);

    // Timer for reading/writing dout objects
    timingTimer doutTimer;
    timingAddTimer(&doutTimer, TIMING_TIMER_CYCLIC, 1, doutHandler);

    // Timer for PWM handling
    timingTimer pwmTimer;
    timingAddTimer(&pwmTimer, TIMING_TIMER_CYCLIC, 1, manPwmHandler);

    timingTimer rotationTimer;
    timingAddTimer(&rotationTimer, TIMING_TIMER_CYCLIC, 2, rotation_detection_handler);

    rotation_detection_init();

    WTDG_Feed();
    while (!needToReset)
    {
        timingExecute();
        appSyncWatchdogProcess();
        terminalRun();
        FlushMbox();
        WTDG_Feed();
    }

    // Wait until all responses set
    for (uint8_t i = 0; i < 10; i++)
    {
        FlushMbox();
        timingDelay_ms(1);
        WTDG_Feed();
    }

    // Perform Softreset
    NVIC_SystemReset();
}
