/*
 * init_stm32_h7 - Initialize the hardware for the STM32 application
 *
 * Copyright (c) 2020 port GmbH Halle/Saale
 *------------------------------------------------------------------
 */


/**
*  \file init_stm32_h7.c
*++ Initialize the Hardware for the STM32H7xx application
*-- Initialisierungsfunktionen für die Hardware des STM32H7xx Treibers
*  \author port GmbH Halle (Saale)
*
*++ This module contains the functions requiered to initialize the
*++ hardware components, i.e. processor with chip selects and timings
*++ or peripheral componets
*-- Dieses Modul enthält Funktionen zur Initialisierung der
*-- Hardwarekomponenten, z.B. Prozessor mit Chip Select und Timings
*-- oder Peripherie
*
*/


/* header of standard C - libraries
---------------------------------------------------------------------------*/
#include <string.h>
#include <stdio.h>
#include <stdarg.h>


#include <environ.h>


/* includes  CANopen library
---------------------------------------------------------------------------*/
#define DEF_HW_PART
#include <cal_conf.h>

#include <co_def.h>
#include <co_drv.h>
#include <co_drvif.h>


/* CAN driver prototypes
---------------------------------------------------------------------------*/
#ifdef CONFIG_NO_GLOBAL_VARS
#  include <glob_drv.h>
#else /* CONFIG_NO_GLOBAL_VARS */
#  include <can_stm32_fdcan.h>
#endif /* CONFIG_NO_GLOBAL_VARS */

#include <examples.h>


/* local defined data types
---------------------------------------------------------------------------*/
/* UART handle declaration */
#ifndef CONFIG_NO_PRINTF 
  UART_HandleTypeDef UartHandle;
#endif

/* global variables
---------------------------------------------------------------------------*/


/* constant definitions
---------------------------------------------------------------------------*/


/* list of external used functions, if not in headers
---------------------------------------------------------------------------*/


/* list of global defined functions
---------------------------------------------------------------------------*/


/* list of local defined functions
---------------------------------------------------------------------------*/
void usage(void);

static void GPIO_Configuration(void);
static void CLOCK_Configuration(void);
#ifndef CONFIG_NO_PRINTF 
  static void UART_Configuration(void);
#endif


/* local defined variables
---------------------------------------------------------------------------*/


/***************************************************************************/
/**
*++ \brief iniDevice - does all at boot up necessary initializations
*-- \brief iniDevice - führt alle notwendigen Bootup Initialisierungen durch.
*
*++ iniDevice() initialize the, especially the CANopen part.
*-- iniDevice() initialisiert die Hardware,
*-- insbesondere des CANopen Teils.
*
* \retval 0
*++ if all was OK
*-- OK
* \retval not 0
*++ if something could not initialized
*-- Fehler, etwas konnte nicht initialisiert werden
*/
UNSIGNED8 iniDevice(void)
{
RCC_PeriphCLKInitTypeDef RCC_PeriphClkInit;
    
    INIT_CPU_INTERRUPTS();
    
    /* Enable the CPU Cache */
    /* Enable I-Cache */
    SCB_EnableICache();
    /* Enable D-Cache */
    SCB_EnableDCache();
	
	/* STM32H7xx HAL library initialization */
    HAL_Init();
	/* Configure the system clock */
	CLOCK_Configuration();
    SystemCoreClockUpdate();

     /* Select PLL1Q as source of FDCANx clock */
    RCC_PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_FDCAN;
    RCC_PeriphClkInit.FdcanClockSelection = RCC_FDCANCLKSOURCE_PLL;
    HAL_RCCEx_PeriphCLKConfig(&RCC_PeriphClkInit);
    
# if defined(CONFIG_MULT_LINES) || defined(CONFIG_REDUNDANCY_SUPPORT)
	/* Enable both CAN clocks */
    __HAL_RCC_FDCAN_CLK_ENABLE();
#else	/* CONFIG_MULT_LINES || CONFIG_REDUNDANCY_SUPPORT */
	/* Enable CAN clock */
	__HAL_RCC_FDCAN_CLK_ENABLE();
#endif

	GPIO_Configuration();

#ifndef CONFIG_NO_PRINTF   
    /* initialize uart part, if used printf output */
    UART_Configuration();
#endif

    return 0;
} /* UNSIGNED8 iniDevice() */


/***************************************************************************/
/**
*++ \brief initCan - initializes the CAN controller
*-- \brief initCan - initialisiert den CAN Kontroller
*
*++ This is a wrapper.
*++ It really calls the init function from the can driver source.
*-- Diese Funktion ruft die init Funktion vom CAN Treiber Source auf.
*
*++ The driver is defined in
*-- Der Treiber ist definiert in
* drivers/shar_src/\<can_driver\>.c
*
* \retval 0
*++ if all was OK
*-- OK
* \retval not 0
*++ if something could not initialized
*-- Fehler, etwas konnte nicht initialisiert werden
*/
UNSIGNED8 initCan(
		UNSIGNED16 baud				/* bit rate for CAN - 125 = 125kbit/s */
		CO_COMMA_REDCY_PARA_DECL	/* canLine (Multiline) */
		)
{
UNSIGNED8 ret;
UNSIGNED8 module;
    
# ifdef CONFIG_MULT_LINES
    
    if (canLine == 0) 
    {/* set module and node number line 0 */
        module = CONFIG_CAN_CONTROLLER_NUMBER_LINE0;
    }
    else if (canLine == 1) 
    {/* set module and node number line 1 */
        module = CONFIG_CAN_CONTROLLER_NUMBER_LINE1;
    }
#ifdef CONFIG_CAN_MULTICAN_NODE_LINE2
    else if (canLine == 2) 
    {/* set module and node number line 2 */
        module = CONFIG_CAN_CONTROLLER_NUMBER_LINE2;
    }
#endif
    else
    { /* line not possible (max. 3 lines implemented) */
        return 0xFF;
    }
        
#else

    module = CONFIG_CAN_CONTROLLER_NUMBER;
    
#endif  
    INIT_CAN_INTERRUPTS(CO_REDCY_PARA);

    ret = Init_CAN(module, baud CO_COMMA_REDCY_PARA);

    return ret;
} /* UNSIGNED8 initCan() */


/***************************************************************************/
/**
*++ \brief endLoop - Check 'end' of Application
*-- \brief endLoop - prüft das Ende der Applikation
*
*++ function for the CANopen examples
*-- Funktion für die CANopen Beispiele
*
*++ This function give the possibility to test
*++ also leaving the application-endless-loop. a
*++ If an Operation System is used, in this function
*++ the exit command must work on.
*-- Diese Funktion ermöglicht es in den Beispielen
*-- auch die Möglichkeit zu testen,
*-- die Applikations-Endloss-Schleife zu verlassen.
*-- Bei Benutzung mit Betriebssystemen muß hier das
*-- Quit-Kommando bearbeitet werden.
*
* !!! only needed for our examples !!!
*
* \retval TRUE
*++ end criteria reached
*-- Ende Kriterium eingetreten
*
* \retval FALSE
*++ end criteria Not reached
*-- Ende Kriterium nicht eingetreten.
*/
BOOL_T endLoop(void)
{
    return CO_FALSE;
} /* BOOL_T endLoop() */


/***************************************************************************/
/**
*++ setOptions - set options from cmd line
*-- setOptions - setzt Optionen der cmd Zeile
*
*++ function for the CANopen examples
*-- Funktion für die CANopen Beispiele
*
*++ setOptions() is used for running the examples with operation systems.
*++ In this function the command line options will evaluated.
*-- setOptions() wird für Beispiele benötigt, welche mit einem Betriebssystem
*-- laufen.
*-- Diese Funktion bearbeitet die übergebenen Kommandozeilen-Parameter.
*
* !!! only needed for our examples !!!
*
* \retval TRUE
*++ success
*-- OK
*
* \retval FALSE
*++ failed
*-- Fehler
*/
BOOL_T setOptions(CO_EXAMPLE_ARGS_DECL)
{
    return(CO_TRUE);
} /* BOOL_T setOptions() */


/***************************************************************************/
/**
*++ \brief usage - give usage information to stderr
*-- \brief usage - gibt Anwendungsinformationen nach stderr
*
*++ function for the CANopen examples
*-- Funktion für die CANopen Beispiele
*
* !!! only needed for our examples !!!
*
* \returns
*++ nothing
*-- nichts
*
* INTERNAL
*/
void usage(void)
{
} /* void usage() */

/**
  * @brief  CLOCK_Configuration
  *   The system Clock is configured as follow : 
  *     System Clock source            = PLL (HSE)
  *     SYSCLK(Hz)                     = 400000000 (CPU Clock)
  *     HCLK(Hz)                       = 200000000 (AXI and AHBs Clock)
  *     AHB Prescaler                  = 2
  *     D1 APB3 Prescaler              = 2 (APB3 Clock  100MHz)
  *     D2 APB1 Prescaler              = 2 (APB1 Clock  100MHz)
  *     D2 APB2 Prescaler              = 2 (APB2 Clock  100MHz)
  *     D3 APB4 Prescaler              = 2 (APB4 Clock  100MHz)
  *     HSE Frequency(Hz)              = 8000000
  *     PLL_M                          = 4
  *     PLL_N                          = 400
  *     PLL_P                          = 2
  *     PLL_Q                          = 10
  *     PLL_R                          = 2
  *     VDD(V)                         = 3.3
  *     Flash Latency(WS)              = 4
  * @param  None
  * @retval None
  */
static void CLOCK_Configuration(void)
{
RCC_OscInitTypeDef RCC_OscInitStruct = {0};
RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    /*!< Supply configuration update enable */
    HAL_PWREx_ConfigSupply(PWR_LDO_SUPPLY);
    /** Initializes the CPU, AHB and APB busses clocks 
    */
    /* The voltage scaling allows optimizing the power consumption when the device is
    clocked below the maximum system frequency, to update the voltage scaling value
    regarding system frequency refer to product datasheet.  */
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

    while(!__HAL_PWR_GET_FLAG(PWR_FLAG_VOSRDY)) {}

    /* Enable HSE Oscillator and activate PLL with HSE as source */
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState = RCC_HSE_ON;
    RCC_OscInitStruct.HSIState = RCC_HSI_OFF;
    RCC_OscInitStruct.CSIState = RCC_CSI_OFF;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;

    RCC_OscInitStruct.PLL.PLLM = 4;
    RCC_OscInitStruct.PLL.PLLN = 400;
    RCC_OscInitStruct.PLL.PLLFRACN = 0;
    RCC_OscInitStruct.PLL.PLLP = 2;
    RCC_OscInitStruct.PLL.PLLR = 2;
    RCC_OscInitStruct.PLL.PLLQ = 10;  /* fdcan_ker_ck = 80 MHz (FDCAN core clock) */

    RCC_OscInitStruct.PLL.PLLVCOSEL = RCC_PLL1VCOWIDE;
    RCC_OscInitStruct.PLL.PLLRGE = RCC_PLL1VCIRANGE_1;
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
    {
      while(1)
      {
      }
    }
    /** Initializes the CPU, AHB and APB busses clocks 
    */
    /* Select PLL as system clock source and configure  bus clocks dividers */
    RCC_ClkInitStruct.ClockType = (RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_D1PCLK1 | RCC_CLOCKTYPE_PCLK1 | \
                                 RCC_CLOCKTYPE_PCLK2  | RCC_CLOCKTYPE_D3PCLK1);

    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.SYSCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_HCLK_DIV2;
    RCC_ClkInitStruct.APB3CLKDivider = RCC_APB3_DIV2;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_APB1_DIV2;  /* fdcan_pclk = 100 MHz */
    RCC_ClkInitStruct.APB2CLKDivider = RCC_APB2_DIV2;
    RCC_ClkInitStruct.APB4CLKDivider = RCC_APB4_DIV2; 

    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK)
    {
      while(1)
      {
      }
    }
}

/*******************************************************************************
* Function Name  : GPIO_Configuration
* Description    : Configures the different GPIO ports.
*                : Adapt this part on your HW requiements! 
*                : (Default: we use possible pin settings on the Nucleo-G474RE board)
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void GPIO_Configuration(void)
{
	/* Configure CAN Pins */
	/* ------------- */
	GPIO_InitTypeDef GPIO_InitStructure;
	
#  if defined(CONFIG_MULT_LINES) || defined(CONFIG_REDUNDANCY_SUPPORT)
	
	 /* Enable both GPIO clock */
	__HAL_RCC_GPIOD_CLK_ENABLE();
	__HAL_RCC_GPIOB_CLK_ENABLE();

	/* CAN1 TX GPIO pin configuration */
    GPIO_InitStructure.Pin = GPIO_PIN_1;
    GPIO_InitStructure.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStructure.Speed = GPIO_SPEED_HIGH;
    GPIO_InitStructure.Pull = GPIO_PULLUP;
    GPIO_InitStructure.Alternate =  GPIO_AF9_FDCAN1;

    HAL_GPIO_Init(GPIOD, &GPIO_InitStructure);

    /* CAN1 RX GPIO pin configuration */
    GPIO_InitStructure.Pin = GPIO_PIN_0;
    GPIO_InitStructure.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStructure.Speed = GPIO_SPEED_HIGH;
    GPIO_InitStructure.Pull = GPIO_PULLUP;
    GPIO_InitStructure.Alternate =  GPIO_AF9_FDCAN1;

    HAL_GPIO_Init(GPIOD, &GPIO_InitStructure);

	/* CAN2 TX GPIO pin configuration */
    GPIO_InitStructure.Pin = GPIO_PIN_6;
    GPIO_InitStructure.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStructure.Speed = GPIO_SPEED_HIGH;
    GPIO_InitStructure.Pull = GPIO_PULLUP;
    GPIO_InitStructure.Alternate =  GPIO_AF9_FDCAN2;

    HAL_GPIO_Init(GPIOB, &GPIO_InitStructure);

    /* CAN2 RX GPIO pin configuration */
    GPIO_InitStructure.Pin = GPIO_PIN_5;
    GPIO_InitStructure.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStructure.Speed = GPIO_SPEED_HIGH;
    GPIO_InitStructure.Pull = GPIO_PULLUP;
    GPIO_InitStructure.Alternate =  GPIO_AF9_FDCAN2;
	
    HAL_GPIO_Init(GPIOB, &GPIO_InitStructure);

#else /* CONFIG_MULT_LINES || CONFIG_REDUNDANCY_SUPPORT */

	/* Enable GPIO clock */
#  if(CONFIG_CAN_CONTROLLER_NUMBER == 1)
	 __HAL_RCC_GPIOD_CLK_ENABLE();
#  elif(CONFIG_CAN_CONTROLLER_NUMBER == 2)
	 __HAL_RCC_GPIOB_CLK_ENABLE();
#  endif /* CONFIG_CAN_CONTROLLER_NUMBER */

#  if(CONFIG_CAN_CONTROLLER_NUMBER == 1)
	
	 /* CAN1 TX GPIO pin configuration */
   GPIO_InitStructure.Pin = GPIO_PIN_1;
   GPIO_InitStructure.Mode = GPIO_MODE_AF_PP;
   GPIO_InitStructure.Speed = GPIO_SPEED_HIGH;
   GPIO_InitStructure.Pull = GPIO_PULLUP;
   GPIO_InitStructure.Alternate =  GPIO_AF9_FDCAN1;

   HAL_GPIO_Init(GPIOD, &GPIO_InitStructure);

   /* CAN1 RX GPIO pin configuration */
   GPIO_InitStructure.Pin = GPIO_PIN_0;
   GPIO_InitStructure.Mode = GPIO_MODE_AF_PP;
   GPIO_InitStructure.Speed = GPIO_SPEED_HIGH;
   GPIO_InitStructure.Pull = GPIO_PULLUP;
   GPIO_InitStructure.Alternate =  GPIO_AF9_FDCAN1;

   HAL_GPIO_Init(GPIOD, &GPIO_InitStructure);
	
#  elif(CONFIG_CAN_CONTROLLER_NUMBER == 2)
	 
	  /* CAN2 TX GPIO pin configuration */
   GPIO_InitStructure.Pin = GPIO_PIN_6;
   GPIO_InitStructure.Mode = GPIO_MODE_AF_PP;
   GPIO_InitStructure.Speed = GPIO_SPEED_HIGH;
   GPIO_InitStructure.Pull = GPIO_PULLUP;
   GPIO_InitStructure.Alternate =  GPIO_AF9_FDCAN2;

   HAL_GPIO_Init(GPIOB, &GPIO_InitStructure);

   /* CAN2 RX GPIO pin configuration */
   GPIO_InitStructure.Pin = GPIO_PIN_5;
   GPIO_InitStructure.Mode = GPIO_MODE_AF_PP;
   GPIO_InitStructure.Speed = GPIO_SPEED_HIGH;
   GPIO_InitStructure.Pull = GPIO_PULLUP;
   GPIO_InitStructure.Alternate =  GPIO_AF9_FDCAN2;

   HAL_GPIO_Init(GPIOB, &GPIO_InitStructure);
#  endif
#endif /* CONFIG_MULT_LINES */
} /* void GPIO_Configuration() */

#ifndef CONFIG_NO_PRINTF   
/*******************************************************************************
* Function Name  : UART_Configuration
* Description    : Configures the UART Interface.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void UART_Configuration()
{
	GPIO_InitTypeDef  GPIO_InitStruct;
	
    /*##-1- Enable peripherals and GPIO Clocks #################################*/
	/* Enable USARTx clock */
  __HAL_RCC_USART3_CLK_ENABLE();

   /* Enable GPIO TX/RX clock */
  __HAL_RCC_GPIOD_CLK_ENABLE();
  
  /*##-2- Configure peripheral GPIO ##########################################*/ 
  // PD8     ------> USART3_TX
  // PD9     ------> USART3_RX    
  /* UART TX GPIO pin configuration  */
  GPIO_InitStruct.Pin       = GPIO_PIN_8;
  GPIO_InitStruct.Mode      = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull      = GPIO_PULLUP;
  GPIO_InitStruct.Speed     = GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF7_USART3;
  
  HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);
    
  /* UART RX GPIO pin configuration  */
  GPIO_InitStruct.Pin = GPIO_PIN_9;
  GPIO_InitStruct.Alternate = GPIO_AF7_USART3;
    
  HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);  
	
	 /*Configure the UART peripheral */
  /* Put the USART peripheral in the Asynchronous mode (UART Mode) */
  /* UART configured as follows:
      - Word Length = 8 Bits
      - Stop Bit = One Stop bit
      - Parity = None
      - BaudRate = 115200 baud
      - Hardware flow control disabled (RTS and CTS signals) */
  UartHandle.Instance        = USART3;

  HAL_UART_DeInit(&UartHandle); 

  UartHandle.Init.BaudRate   = 115200;
  UartHandle.Init.WordLength = UART_WORDLENGTH_8B;
  UartHandle.Init.StopBits   = UART_STOPBITS_1;
  UartHandle.Init.Parity     = UART_PARITY_NONE;
  UartHandle.Init.HwFlowCtl  = UART_HWCONTROL_NONE;
  UartHandle.Init.Mode       = UART_MODE_TX_RX;
  UartHandle.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT; 
  
  HAL_UART_Init(&UartHandle);	
	
} /* void UART_Configuration() */

/***************************************************************************/
/**
* \brief uart_printf - translater function from PRINTF to uart output
*
*
*/
void uart_printf(char *fmt, ...)
{
    char str[255];
    int length = 0;

    va_list args;
    va_start(args, fmt);
    length = vsprintf(str, fmt, args);
    if(length > 0)
    {
        HAL_UART_Transmit(&UartHandle, (uint8_t*)str, length, 500);
    }
    va_end(args);
}
#endif

#ifdef DEBUG
/*******************************************************************************
* Function Name  : assert_failed
* Description    : Reports the name of the source file and the source line number
*                  where the assert_param error has occurred.
* Input          : - file: pointer to the source file name
*                  - line: assert_param error line source number
* Output         : None
* Return         : None
*******************************************************************************/
void assert_failed(UNSIGNED8* file, UNSIGNED32 line)
{
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */

  /* Infinite loop */
  while (1)
  {
  }
}
#endif /* DEBUG */


/*______________________________________________________________________EOF_*/
