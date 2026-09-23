/*!
 * Copyright � Siemens Healthcare GmbH 2022, All Rights Reserved
 *
 * Project: Building Block Low End MCU
 *
 * @file
 * @brief Interrupt handlers routines
 *
 */
/*----------------------------------------------------------------------------*/

// HAL includes
#include <hal.h>
// CANOpen includes
#define DEF_HW_PART
#include <cal_conf.h>
#include <environ.h>
#include <stdio.h>
// Project Includes
#include "timing.h"
#include "bsp_ain.h"
#include "ism330dlc.h"
#include "ism330dlc_reg.h"

/*----------------------------------------------------------------------------*/
/*!
 * @brief  This function handles SysTick Handler
 *
 */
/*----------------------------------------------------------------------------*/
void SysTick_Handler(void)
{
    timingTick();
    Timer_int();
    HAL_IncTick();
}

/*----------------------------------------------------------------------------*/
/*!
 *   @brief This function handles FDCAN1 interrupt 0
 *
 */
/*----------------------------------------------------------------------------*/
void FDCAN1_IT0_IRQHandler(void)
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

#ifdef FDCAN2
/*----------------------------------------------------------------------------*/
/*!
 *  @brief This function handles FDCAN2 interrupt 0
 *
 */
/*----------------------------------------------------------------------------*/
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
/*----------------------------------------------------------------------------*/
/*!
 *  @brief This function handles FDCAN interrupt 0
 *
 */
/*----------------------------------------------------------------------------*/
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

/**
 * @brief This function handles DMA1 stream0 global interrupt.
 */
void DMA1_Stream0_IRQHandler(void)
{
    HAL_DMA_IRQHandler(&hdma1);
}

void DMA1_Stream1_IRQHandler(void)
{
    HAL_DMA_IRQHandler(&hdma3);
}

/**
 * @brief  This function handles UART interrupt request.
 * @param  None
 * @retval None
 * @Note   This function is redefined in "main.h" and related to DMA stream
 *         used for USART data transmission
 */
void USART3_IRQHandler(void)
{
    __disable_irq();
    extern UART_HandleTypeDef serialUart;
    HAL_UART_IRQHandler(&serialUart);
    __enable_irq();
}

void EXTI9_5_IRQHandler(void)
{
    if(__HAL_GPIO_EXTI_GET_IT(GPIO_PIN_9) != RESET) {
        //uint8_t tapSrc;
        //ism330dlcReadRegister8bit(SYS_SPI_ISM330_NUMBER, REG_ISM330_TAP_SRC, &tapSrc);
        //bool sign = tapSrc & 0x08;
        HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_9); 
    }
}

void EXTI15_10_IRQHandler(void)
{
    if(__HAL_GPIO_EXTI_GET_IT(GPIO_PIN_10) != RESET) {
        HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_10); 
        uint8_t wakeUpSrc;
        ism330dlcReadRegister8bit(SYS_SPI_ISM330_NUMBER, REG_ISM330_WAKE_UP_SRC, &wakeUpSrc);
        printf("%X\n", wakeUpSrc);
    }
}