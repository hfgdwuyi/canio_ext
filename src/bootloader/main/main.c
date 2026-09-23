/*!
 * Copyright Siemens Healthcare GmbH 2022, All Rights Reserved
 *
 * Project: Building Block Low End MCU
 *
 * @file
 * @brief Main function
 */
/*----------------------------------------------------------------------------*/
// Standard includes
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
// HAL includes
#include <hal.h>
// USB Library includes
#include <usbd_core.h>
#include <usbd_desc.h>
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
#include <co_odidx.h>
#include <objects.h>
// Project includes
#include "mem_map.h"
#include "selftest.h"
#include "sysinit.h"
#include "bsp_board.h"
#include "timing.h"
#include "bsp_led.h"
#include "w25xx_qspi.h"
#include "prog_ctrl.h"
#include "terminal.h"
#include "can_cfg.h"
#include "version.h"
#include "storage.h"
#include "error.h"
#include "debug.h"
#include "bsp_wtdg.h"
#include "usbd_cdc.h"
#include "usbd_cdc_interface.h"
#include "update.h"

USBD_HandleTypeDef USBD_Device;

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
 * @brief          Enables VBus sensing for USB protocol
 *
 */
/*----------------------------------------------------------------------------*/
static inline void enableVBusSensing(void)
{
    hpcd.Instance->GOTGCTL &= ~USB_OTG_GOTGCTL_BVALOEN;
    hpcd.Instance->GOTGCTL &= ~USB_OTG_GOTGCTL_BVALOVAL;
    hpcd.Instance->GCCFG |= USB_OTG_GCCFG_VBDEN;
    hpcd.Instance->GINTMSK |= USB_OTG_GINTMSK_OTGINT;
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief          Check watchdog reset flag
 *
 * @return         true if IWDG caused the MCU reset, false otherwise
 *
 */
/*----------------------------------------------------------------------------*/
static bool checkRstStatus(void)
{
    uint32_t rstReg = READ_REG(RCC->RSR);
    SET_BIT(RCC->RSR, RCC_RSR_RMVF);
    printf("Reset reg: 0x%lX\n", rstReg);
    if ((rstReg >> RCC_RSR_IWDG1RSTF_Pos) & 1)
    {
        return true;
    }
    else
    {
        return false;
    }
}

/*----------------------------------------------------------------------------*/
/*!
 @brief          Start application according

*/
/*----------------------------------------------------------------------------*/
static void startByTypeApp(uint32_t sign)
{
    uint32_t appCode;
    uint32_t status;
    bool appStatus;
    uint32_t trueAddr = sign == PROG_CTRL_START_DEFAULT_IMAGE ? DEFAULT_APP_ADDRESS : 0;
    status            = w25Read(trueAddr + APP_START_CODE, (uint8_t *)&appCode, sizeof(appCode));
    if (status != W25_OK)
    {
        return;
    }
    if (sign == PROG_CTRL_START_DEFAULT_IMAGE)
    {
        appStatus = selftestFlashApp(DEFAULT_APP_ADDRESS);
        if (!appStatus)
        {
            printf("Default app CRC error\n");
            return;
        }
    }
    printf("Start app from 0x%X address in %s memory\n",
           sign == PROG_CTRL_START_DEFAULT_IMAGE ? APP_FLASH_ADDRESS + DEFAULT_APP_ADDRESS : APP_FLASH_ADDRESS + 0,
           appCode == PROG_APP_START_LOCATION ? "RAM" : "FLASH");
    if (appCode == PROG_APP_START_LOCATION)
    {
        if (copyApplication(trueAddr))
        {
            startApplication(RAM_AXI_ADDRESS);
        }
        else
        {
            printf("Copy app error\n");
            return;
        }
    }
    else
    {
        // Enable Memory Mapped mode
        if (w25EnableMemoryMappedMode() != W25_OK)
        {
            return;
        }
        startApplication(APP_FLASH_ADDRESS + trueAddr);
    }
    return;
}

/*----------------------------------------------------------------------------*/
/*!
 @brief          LED blinking (alive sign)

*/
/*----------------------------------------------------------------------------*/
static void blinkHandler(void)
{
    ledToggle(SYSTEM_OK_LED_NUM);
}

/*----------------------------------------------------------------------------*/
/*!
 @brief          Check application consistency and run it

*/
/*----------------------------------------------------------------------------*/
static void checkForStartApp(void)
{
    // Read the signature
    uint32_t sign = progCtrlGetSignature();
    // Set default signature
    progCtrlSetSignature(PROG_CTRL_START_DEFAULT);
    printf("Sign = 0x%lX\n", sign);
    if (checkRstStatus())
    {
        printf("Watchdog reset!\n");
        return;
    }
    // Check the rescue switch and bootloader signature
    if (selftestRescueSwitchRead())
    {
        // Bootloader shall be started, no need to check application
        return;
    }

    switch (sign)
    {
        case PROG_CTRL_START_BOOTLOADER: break;
        case PROG_CTRL_START_APPLICATION:
        case PROG_CTRL_START_DEFAULT_IMAGE: startByTypeApp(sign); break;
        case PROG_CTRL_START_DEFAULT:
            // Check if valid application is present
            if (selftestFlashApp(APPLICATION_STORED_ADDRESS))
            {
                // Set signature to start application
                progCtrlSetSignature(PROG_CTRL_START_APPLICATION);
            }
            else
            {
                printf("Invalid CRC\nStarting default image!\n");
                // Set signature to start default image
                progCtrlSetSignature(PROG_CTRL_START_DEFAULT_IMAGE);
            }
            // Soft Reset, application will be started next
            HAL_NVIC_SystemReset();
            break;
        default:
            // Nothing to do here
            break;
    }
}

/*----------------------------------------------------------------------------*/
/*!
 @brief          Reassignment of variable objects due to the fact that they
                 were reset in init_Library()
*/
/*----------------------------------------------------------------------------*/
static void updateODValues(void)
{
    RET_T      ret;
    void      *dest;
    void      *src;
    UNSIGNED32 dstSize;
    UNSIGNED32 srcSize;

    // 0x100A, SW version
    snprintf((char *)p301_manu_software_version,
             sizeof(p301_manu_software_version),
             "%d.%d.%d Bootloader  ",
             BL_SW_VERSION,
             BL_SW_SUBVERSION,
             BL_SW_REVISION);

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
}

/*----------------------------------------------------------------------------*/
/*!
@brief          Main function

*/
/*----------------------------------------------------------------------------*/
int main(void)
{
    // Enable the CPU Cache
    SystemCache_Enable();

    // Initialize HAL
    HAL_Init();

    // Initialize System Clock
    SystemClock_Config();
    // Initialize pins and interfaces
    boardInit();

    // Initialize QSPI memory
    uint32_t status = w25InitQuad();
    // Check if application need to be started
    checkForStartApp();
    // Initialize storage
    for (UNSIGNED8 i = 0; i < storageSegmentsCount; i++)
    {
        storageSegmentInit(i);
    }

    // Initialize external watchdog
    WTDG_Init();
    // Feed external watchdog
    WTDG_Feed();
    printf("Bootloader started\n");
    printf("Built %s %s\n", __DATE__, __TIME__);
    printf("QSPI memory init: %ld\n", status);


    HAL_PWREx_EnableUSBVoltageDetector();

    // Init Device Library
    USBD_Init(&USBD_Device, &VCP_Desc, 0);
    WTDG_Feed();
    // Add Supported Class
    USBD_RegisterClass(&USBD_Device, &USBD_CDC);

    /* Add CDC Interface Class */
    USBD_CDC_RegisterInterface(&USBD_Device, &USBD_CDC_fops);
    WTDG_Feed();

    enableVBusSensing();
    // Start Device Process
    USBD_Start(&USBD_Device);
    printf("USB started!\n");
    WTDG_Feed();


    // Read CAN settings
    bool bRet = canCfgGetConfig();

    WTDG_Feed();

    // Initialize CAN controller
    UNSIGNED8 ret = initCan(bitRate);
    printf("Init CAN: %d\n", ret);

    // Initialize CANOpen library
    RET_T commonRet = init_Library();
    printf("Init library: %d\n", commonRet);

    // Initialize CANOpen timer
    (void)initTimer();
    WTDG_Feed();
    Start_CAN();
    ENABLE_CPU_INTERRUPTS();
    WTDG_Feed();
    // Copy CAN config pins state to the OD's variable
    canCfgSetCode();
    // Set status and error registers in case of CAN settings error
    if (!bRet)
    {
        DEBUGOUT(LOG_ERROR, "CAN settings error. Default values were set\n");
        set_error(ERR_MAN_STORAGE, ERR_STORAGE_ASSET);
    }
    WTDG_Feed();
    // Update asset data
    updateODValues();
    WTDG_Feed();

    timingTimer blinkTimer;
    timingAddTimer(&blinkTimer, TIMING_TIMER_CYCLIC, 300, blinkHandler);

    while (!needToReset)
    {
        WTDG_Feed();
        timingExecute();
        terminalRun();
        FlushMbox();
        if (strlen(TxBuf) != 0)
        {
            CDC_Itf_Transmit(TxBuf, strlen(TxBuf));
            memset(TxBuf, 0, strlen(TxBuf));
        }
    }

    // Wait until all responses set
    for (uint16_t i = 0; i < 500; i++)
    {
        FlushMbox();
        timingDelay_ms(1);
        WTDG_Feed();
        if (strlen(TxBuf) != 0)
        {
            CDC_Itf_Transmit(TxBuf, strlen(TxBuf));
            memset(TxBuf, 0, strlen(TxBuf));
        }
    }

    // Perform Softreset
    NVIC_SystemReset();
}
