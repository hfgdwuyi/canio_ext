/*!
* Copyright © Siemens Healthcare GmbH 2022, All Rights Reserved
*
* Project: Building Block Low End MCU
* 
* @file
* @brief System initialization functions 
*
*/
/*----------------------------------------------------------------------------*/
#include <stdbool.h>
// HAL includes
#include <hal.h>
// Project includes
#include "mem_map.h"
#include "sysinit.h"

/*! Vector Table base offset field. This value must be a multiple of 0x200. */
#define VECT_TAB_OFFSET  0x00000000UL      
                                      

/*! 
@brief System Core clock

    This variable is updated in three ways:
    1) by calling CMSIS function SystemCoreClockUpdate()
    2) by calling HAL API function HAL_RCC_GetHCLKFreq()
    3) each time HAL_RCC_ClockConfig() is called to configure the system clock frequency 
    
    @note: If you use this function to configure the system clock; then there
    is no need to call the 2 first functions listed above, since SystemCoreClock
    variable is updated automatically.
*/
uint32_t SystemCoreClock = 64000000;
uint32_t SystemD2Clock = 64000000;
const  uint8_t D1CorePrescTable[16] = {0, 0, 0, 0, 1, 2, 3, 4, 1, 2, 3, 4, 6, 7, 8, 9};


/*! Error LED port */
#define ERROR_LED_PORT          GPIOJ
/*! Error LED pin */
#define ERROR_LED_PIN           GPIO_PIN_0
/*! Error LED port initialization */
#define ERROR_LED_PORT_INIT()   __HAL_RCC_GPIOJ_CLK_ENABLE()
/*----------------------------------------------------------------------------*/
/*!
 @brief          Error signalization and switch to safe state.
*/
/*----------------------------------------------------------------------------*/
void selftestErrorState(void){

    // Init port
    ERROR_LED_PORT_INIT();
    // Init error LED
    HAL_GPIO_Init(ERROR_LED_PORT, &((GPIO_InitTypeDef){ERROR_LED_PIN, GPIO_MODE_OUTPUT_PP, GPIO_PULLUP, GPIO_SPEED_FREQ_HIGH, 0}));
    
    // Go to an endless loop with LED blinking
    for(;;){
        for(uint32_t i = 0; i < 2000000; i++){
            __NOP();
        }
        HAL_GPIO_TogglePin(ERROR_LED_PORT, ERROR_LED_PIN); 
    }

}
    
/*----------------------------------------------------------------------------*/
/*!
  @brief  Setup the microcontroller system.

          Initialize the FPU setting, vector table location and External memory 
          configuration.
*/
/*----------------------------------------------------------------------------*/
void SystemInit (void){
    
#if defined (DATA_IN_D2_SRAM)
    __IO uint32_t tmpreg;
#endif 
    
    // FPU settings ------------------------------------------------------------
#if (__FPU_PRESENT == 1) && (__FPU_USED == 1)
    SCB->CPACR |= ((3UL << (10 * 2))|(3UL << (11 * 2)));  // set CP10 and CP11 Full Access 
#endif
    // Reset the RCC clock configuration to the default reset state ------------
    // Set HSION bit 
    RCC->CR |= RCC_CR_HSION;

    // Reset CFGR register 
    RCC->CFGR = 0x00000000;

    // Reset HSEON, CSSON , CSION,RC48ON, CSIKERON PLL1ON, PLL2ON and PLL3ON bits 
    RCC->CR &= 0xEAF6ED7FU;

    // Reset D1CFGR register 
    RCC->D1CFGR = 0x00000000;

    // Reset D2CFGR register 
    RCC->D2CFGR = 0x00000000;

    // Reset D3CFGR register 
    RCC->D3CFGR = 0x00000000;

    // Reset PLLCKSELR register 
    RCC->PLLCKSELR = 0x00000000;

    // Reset PLLCFGR register 
    RCC->PLLCFGR = 0x00000000;
    // Reset PLL1DIVR register 
    RCC->PLL1DIVR = 0x00000000;
    // Reset PLL1FRACR register 
    RCC->PLL1FRACR = 0x00000000;

    // Reset PLL2DIVR register 
    RCC->PLL2DIVR = 0x00000000;

    // Reset PLL2FRACR register 

    RCC->PLL2FRACR = 0x00000000;
    // Reset PLL3DIVR register 
    RCC->PLL3DIVR = 0x00000000;

    // Reset PLL3FRACR register 
    RCC->PLL3FRACR = 0x00000000;

    // Reset HSEBYP bit 
    RCC->CR &= 0xFFFBFFFFU;

    // Disable all interrupts 
    RCC->CIER = 0x00000000;

    // Change  the switch matrix read issuing capability to 1 for the AXI SRAM target (Target 7) 
    if((DBGMCU->IDCODE & 0xFFFF0000U) < 0x20000000U)
    {
    // if stm32h7 revY
    // Change  the switch matrix read issuing capability to 1 for the AXI SRAM target (Target 7) 
    *((__IO uint32_t*)0x51008108) = 0x00000001U;
    }

#if defined (DATA_IN_D2_SRAM)
    // in case of initialized data in D2 SRAM , enable the D2 SRAM clock 
    RCC->AHB2ENR |= (RCC_AHB2ENR_D2SRAM1EN | RCC_AHB2ENR_D2SRAM2EN | RCC_AHB2ENR_D2SRAM3EN);
    tmpreg = RCC->AHB2ENR;
    (void) tmpreg;
#endif 
    
    // Disable the FMC bank1 (enabled after reset).
    // This, prevents CPU speculation access on this bank which blocks the use of FMC during
    // 24us. During this time the others FMC master (such as LTDC) cannot use it!
    FMC_Bank1_R->BTCR[0] = 0x000030D2;

#if defined (DATA_IN_ExtSDRAM)
    SystemInit_ExtMemCtl(); 
#endif 

    // Configure the Vector Table location add offset address 
    SCB->VTOR = APP_FLASH_ADDRESS | VECT_TAB_OFFSET; 

}

/*----------------------------------------------------------------------------*/
/*!
  * @brief  System Clock Configuration
  *         The system Clock is configured as follow :
  *            System Clock source            = PLL1 (HSE BYPASS)
  *            SYSCLK(Hz)                     = 400000000 (CPU Clock)
  *            HCLK(Hz)                       = 200000000 (AXI and AHBs Clock)
  *            AHB Prescaler                  = 2
  *            D1 APB3 Prescaler              = 2 (APB3 Clock  100MHz)
  *            D2 APB1 Prescaler              = 2 (APB1 Clock  100MHz)
  *            D2 APB2 Prescaler              = 2 (APB2 Clock  100MHz)
  *            D3 APB4 Prescaler              = 2 (APB4 Clock  100MHz)
  *            HSE Frequency(Hz)              = 80000000
  *            PLL_M                          = 5
  *            PLL_N                          = 160
  *            PLL_P                          = 2
  *            PLL_Q                          = 10
  *            PLL_R                          = 2
  *            VDD(V)                         = 3.3
  *            Flash Latency(WS)              = 4
  */
/*----------------------------------------------------------------------------*/
void SystemClock_Config(void){
    RCC_ClkInitTypeDef RCC_ClkInitStruct;
    RCC_OscInitTypeDef RCC_OscInitStruct;
    HAL_StatusTypeDef ret = HAL_OK;

    /*!< Supply configuration update enable */
    HAL_PWREx_ConfigSupply(PWR_LDO_SUPPLY);

    /* The voltage scaling allows optimizing the power consumption when the device is
    clocked below the maximum system frequency, to update the voltage scaling value
    regarding system frequency refer to product datasheet.  */
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

    while(!__HAL_PWR_GET_FLAG(PWR_FLAG_VOSRDY)) {
        // Wait until voltage is applied
    }

    /* Enable HSE Oscillator and activate PLL with HSE as source */
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState = RCC_HSE_ON;
    RCC_OscInitStruct.HSIState = RCC_HSI_OFF;
    RCC_OscInitStruct.CSIState = RCC_CSI_OFF;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;

    RCC_OscInitStruct.PLL.PLLM = 5;
    RCC_OscInitStruct.PLL.PLLN = 160;
    RCC_OscInitStruct.PLL.PLLFRACN = 0;
    RCC_OscInitStruct.PLL.PLLP = 2;
    RCC_OscInitStruct.PLL.PLLR = 2;
    RCC_OscInitStruct.PLL.PLLQ = 10;

    RCC_OscInitStruct.PLL.PLLVCOSEL = RCC_PLL1VCOWIDE;
    RCC_OscInitStruct.PLL.PLLRGE = RCC_PLL1VCIRANGE_2;
    ret = HAL_RCC_OscConfig(&RCC_OscInitStruct);
    if(ret != HAL_OK){
        selftestErrorState();
    }

    /* Select PLL as system clock source and configure  bus clocks dividers */
    RCC_ClkInitStruct.ClockType = (RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_D1PCLK1 | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2  | RCC_CLOCKTYPE_D3PCLK1);

    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.SYSCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_HCLK_DIV2;
    RCC_ClkInitStruct.APB3CLKDivider = RCC_APB3_DIV2;  
    RCC_ClkInitStruct.APB1CLKDivider = RCC_APB1_DIV2; 
    RCC_ClkInitStruct.APB2CLKDivider = RCC_APB2_DIV2; 
    RCC_ClkInitStruct.APB4CLKDivider = RCC_APB4_DIV2; 
    ret = HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4);
    if(ret != HAL_OK){
        selftestErrorState();
    }
    
    // update System Core Clock
    SystemCoreClockUpdate();
}


/*----------------------------------------------------------------------------*/
/*!
  * @brief  Update SystemCoreClock variable according to Clock Register Values.
  *         The SystemCoreClock variable contains the core clock , it can
  *         be used by the user application to setup the SysTick timer or configure
  *         other parameters.
  *           
  * @note   Each time the core clock changes, this function must be called
  *         to update SystemCoreClock variable value. Otherwise, any configuration
  *         based on this variable will be incorrect.         
  *     
  * @note   - The system frequency computed by this function is not the real 
  *           frequency in the chip. It is calculated based on the predefined 
  *           constant and the selected clock source:
  *             
  *           - If SYSCLK source is CSI, SystemCoreClock will contain the CSI_VALUE(*)                                 
  *           - If SYSCLK source is HSI, SystemCoreClock will contain the HSI_VALUE(**)
  *           - If SYSCLK source is HSE, SystemCoreClock will contain the HSE_VALUE(***) 
  *           - If SYSCLK source is PLL, SystemCoreClock will contain the CSI_VALUE(*),
  *             HSI_VALUE(**) or HSE_VALUE(***) multiplied/divided by the PLL factors.
  *
  *         (*) CSI_VALUE is a constant defined in stm32h7xx_hal.h file (default value
  *             4 MHz) but the real value may vary depending on the variations
  *             in voltage and temperature.        
  *         (**) HSI_VALUE is a constant defined in stm32h7xx_hal.h file (default value
  *             64 MHz) but the real value may vary depending on the variations
  *             in voltage and temperature.   
  *    
  *         (***)HSE_VALUE is a constant defined in stm32h7xx_hal.h file (default value
  *              25 MHz), user has to ensure that HSE_VALUE is same as the real
  *              frequency of the crystal used. Otherwise, this function may
  *              have wrong result.
  *                
  *         - The result of this function could be not correct when using fractional
  *           value for HSE crystal.
  */
/*----------------------------------------------------------------------------*/
void SystemCoreClockUpdate (void){
    uint32_t pllp, pllsource, pllm, pllfracen, hsivalue, tmp;
    float_t fracn1, pllvco;

  /* Get SYSCLK source -------------------------------------------------------*/

    switch (RCC->CFGR & RCC_CFGR_SWS){
    case RCC_CFGR_SWS_HSI:  /* HSI used as system clock source */
        SystemCoreClock = (uint32_t) (HSI_VALUE >> ((RCC->CR & RCC_CR_HSIDIV)>> 3));
        break;

    case RCC_CFGR_SWS_CSI:  /* CSI used as system clock  source */
        SystemCoreClock = CSI_VALUE;
        break;

    case RCC_CFGR_SWS_HSE:  /* HSE used as system clock  source */
        SystemCoreClock = HSE_VALUE;
        break;

    case RCC_CFGR_SWS_PLL1:  /* PLL1 used as system clock  source */

        /* PLL_VCO = (HSE_VALUE or HSI_VALUE or CSI_VALUE/ PLLM) * PLLN
        SYSCLK = PLL_VCO / PLLR
        */
        pllsource = (RCC->PLLCKSELR & RCC_PLLCKSELR_PLLSRC);
        pllm = ((RCC->PLLCKSELR & RCC_PLLCKSELR_DIVM1)>> 4)  ;
        pllfracen = ((RCC->PLLCFGR & RCC_PLLCFGR_PLL1FRACEN)>>RCC_PLLCFGR_PLL1FRACEN_Pos);
        fracn1 = (float_t)(uint32_t)(pllfracen* ((RCC->PLL1FRACR & RCC_PLL1FRACR_FRACN1)>> 3));

        if (pllm != 0U){
            switch (pllsource){
            case RCC_PLLCKSELR_PLLSRC_HSI:  /* HSI used as PLL clock source */
                hsivalue = (HSI_VALUE >> ((RCC->CR & RCC_CR_HSIDIV)>> 3)) ;
                pllvco = ( (float_t)hsivalue / (float_t)pllm) * ((float_t)(uint32_t)(RCC->PLL1DIVR & RCC_PLL1DIVR_N1) + (fracn1/(float_t)0x2000) +(float_t)1 );
                break;

            case RCC_PLLCKSELR_PLLSRC_CSI:  /* CSI used as PLL clock source */
                pllvco = ((float_t)CSI_VALUE / (float_t)pllm) * ((float_t)(uint32_t)(RCC->PLL1DIVR & RCC_PLL1DIVR_N1) + (fracn1/(float_t)0x2000) +(float_t)1 );
                break;

            case RCC_PLLCKSELR_PLLSRC_HSE:  /* HSE used as PLL clock source */
                pllvco = ((float_t)HSE_VALUE / (float_t)pllm) * ((float_t)(uint32_t)(RCC->PLL1DIVR & RCC_PLL1DIVR_N1) + (fracn1/(float_t)0x2000) +(float_t)1 );
                break;

            default:
                pllvco = ((float_t)CSI_VALUE / (float_t)pllm) * ((float_t)(uint32_t)(RCC->PLL1DIVR & RCC_PLL1DIVR_N1) + (fracn1/(float_t)0x2000) +(float_t)1 );
                break;
            }
            pllp = (((RCC->PLL1DIVR & RCC_PLL1DIVR_P1) >>9) + 1U ) ;
            SystemCoreClock =  (uint32_t)(float_t)(pllvco/(float_t)pllp);
        }else{
            SystemCoreClock = 0U;
        }
        break;

    default:
        SystemCoreClock = CSI_VALUE;
        break;
  }

    /* Compute SystemClock frequency --------------------------------------------------*/

    tmp = D1CorePrescTable[(RCC->D1CFGR & RCC_D1CFGR_D1CPRE)>> RCC_D1CFGR_D1CPRE_Pos];
    /* SystemCoreClock frequency : CM7 CPU frequency  */
    SystemCoreClock >>= tmp;

    /* SystemD2Clock frequency : AXI and AHBs Clock frequency  */
    SystemD2Clock = (SystemCoreClock >> ((D1CorePrescTable[(RCC->D1CFGR & RCC_D1CFGR_HPRE)>> RCC_D1CFGR_HPRE_Pos]) & 0x1FU));
}

/*----------------------------------------------------------------------------*/
/*!
 @brief  CPU L1-Cache enable
*/
/*----------------------------------------------------------------------------*/
void SystemCache_Config(void){
    //Enable I-Cache
    SCB_EnableICache();

    // Enable D-Cache
    SCB_EnableDCache();
}

