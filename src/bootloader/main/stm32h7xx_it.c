/*!
 * Copyright Siemens Healthcare GmbH 2022, All Rights Reserved
 *
 * Project: Building Block Low End MCU
 *
 * @file
 * @brief Interrupt handlers routines
 */
/*----------------------------------------------------------------------------*/
// HAL includes
#include <hal.h>
// CANOpen includes
#define DEF_HW_PART
#include <cal_conf.h>
#include <environ.h>
// Project Includes
#include "timing.h"

/*!
 @brief  This function handles SysTick Handler.
 @param  None
 @retval None
*/
void SysTick_Handler(void)
{
    timingTick();
    Timer_int();
    HAL_IncTick();
}

/**
 * @brief This function handles FDCAN1 interrupt 0.
 */
void FDCAN1_IT0_IRQHandler(void)
{
    /* USER CODE BEGIN FDCAN1_IT0_IRQn 0 */
#ifdef CONFIG_MULT_LINES
    if (hfdcan[0].Instance == FDCAN1)
    {
        HAL_FDCAN_IRQHandler(&hfdcan[0]);
    }
    else if (hfdcan[1].Instance == FDCAN1)
    {
        HAL_FDCAN_IRQHandler(&hfdcan[1]);
    }
#ifdef CONFIG_CAN_MULTICAN_NODE_LINE2
    else if (hfdcan[2].Instance == FDCAN1)
    {
        HAL_FDCAN_IRQHandler(&hfdcan[2]);
    }
#endif
#else
    /* USER CODE END FDCAN1_IT0_IRQn 0 */
    HAL_FDCAN_IRQHandler(&hfdcan);
    /* USER CODE BEGIN FDCAN1_IT0_IRQn 1 */
#endif
    /* USER CODE END FDCAN1_IT0_IRQn 1 */
}

#ifdef FDCAN2
/**
 * @brief This function handles FDCAN2 interrupt 0.
 */
void FDCAN2_IT0_IRQHandler(void)
{
    /* USER CODE BEGIN FDCAN2_IT0_IRQn 0 */
#ifdef CONFIG_MULT_LINES
    if (hfdcan[0].Instance == FDCAN2)
    {
        HAL_FDCAN_IRQHandler(&hfdcan[0]);
    }
    else if (hfdcan[1].Instance == FDCAN2)
    {
        HAL_FDCAN_IRQHandler(&hfdcan[1]);
    }
#ifdef CONFIG_CAN_MULTICAN_NODE_LINE2
    else if (hfdcan[2].Instance == FDCAN2)
    {
        HAL_FDCAN_IRQHandler(&hfdcan[2]);
    }
#endif
#else
    /* USER CODE END FDCAN2_IT0_IRQn 0 */
    HAL_FDCAN_IRQHandler(&hfdcan);
    /* USER CODE BEGIN FDCAN2_IT0_IRQn 1 */
#endif
    /* USER CODE END FDCAN2_IT0_IRQn 1 */
}
#endif

void FDCAN_CAL_IRQHandler(void)
{
#ifdef CONFIG_MULT_LINES
    if (hfdcan[0].Instance == FDCAN1)
    {
        HAL_FDCAN_IRQHandler(&hfdcan[0]);
    }
    else if (hfdcan[1].Instance == FDCAN1)
    {
        HAL_FDCAN_IRQHandler(&hfdcan[1]);
    }
#ifdef CONFIG_CAN_MULTICAN_NODE_LINE2
    else if (hfdcan[2].Instance == FDCAN1)
    {
        HAL_FDCAN_IRQHandler(&hfdcan[2]);
    }
#endif
#else
    HAL_FDCAN_IRQHandler(&hfdcan);
#endif
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief  This function handles USB-On-The-Go FS/HS global interrupt request.
 *
 */
/*----------------------------------------------------------------------------*/
#ifdef USE_USB_FS
void OTG_FS_IRQHandler(void)
#else
void OTG_HS_IRQHandler(void)
#endif
{
    HAL_PCD_IRQHandler(&hpcd);
}
