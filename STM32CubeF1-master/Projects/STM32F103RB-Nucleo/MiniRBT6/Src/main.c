/**
  ******************************************************************************
  * @file    Templates/Src/main.c 
  * @author  MCD Application Team
  * @brief   Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2016 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "lcd8080.h"

/** @addtogroup STM32F1xx_HAL_Examples
  * @{
  */

/** @addtogroup Templates
  * @{
  */

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void DelayMs(uint32_t delay_ms);
static void RunStage1_BacklightPath(void);
static void RunStage2_Profile1Write(void);
static void RunStage3_Profile23Write(void);
static void SetStageMarker(uint8_t stage);

/* Private functions ---------------------------------------------------------*/

/**
  * @brief  Main program
  * @param  None
  * @retval None
  */
int main(void)
{

  /* STM32F103xB HAL library initialization:
       - Configure the Flash prefetch
       - Systick timer is configured by default as source of time base, but user 
         can eventually implement his proper time base source (a general purpose 
         timer for example or other time source), keeping in mind that Time base 
         duration should be kept 1ms since PPP_TIMEOUT_VALUEs are defined and 
         handled in milliseconds basis.
       - Set NVIC Group Priority to 4
       - Low Level Initialization
     */
  HAL_Init();

  /* Configure the system clock to 64 MHz */
  SystemClock_Config();

  /* Initialize GPIO for two LEDs */
  MX_GPIO_Init();

  HAL_GPIO_WritePin(LED0_GPIO_Port, LED0_Pin, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin, GPIO_PIN_RESET);


  /* Infinite loop */
  while (1)
  {
    RunStage1_BacklightPath();
    RunStage2_Profile1Write();
    RunStage3_Profile23Write();
  }
}

static void RunStage1_BacklightPath(void)
{
  uint32_t i;

  SetStageMarker(1U);
  LCD_InitWithProfile(1U);

  /* Stage 1: only test BL path. Screen content is ignored in this stage. */
  for (i = 0U; i < 3U; i++)
  {
    LCD_SetBacklightRaw(0U);
    DelayMs(1500U);
    LCD_SetBacklightRaw(1U);
    DelayMs(1500U);
  }
}

static void RunStage2_Profile1Write(void)
{
  static const uint16_t colors[] = {
    LCD_COLOR_RED,
    LCD_COLOR_GREEN,
    LCD_COLOR_BLUE,
    LCD_COLOR_BLACK
  };
  uint32_t i;

  SetStageMarker(2U);
  LCD_InitWithProfile(1U);
  LCD_SetBacklightRaw(1U);

  /* Stage 2: 9341 path write test. */
  for (i = 0U; i < (sizeof(colors) / sizeof(colors[0])); i++)
  {
    LCD_FillScreen(colors[i]);
    DelayMs(2000U);
  }
}

static void RunStage3_Profile23Write(void)
{
  static const uint16_t colors[] = {
    LCD_COLOR_BLUE,
    LCD_COLOR_WHITE,
    LCD_COLOR_BLACK
  };
  uint32_t i;

  SetStageMarker(3U);

  /* Stage 3A: 9325/5408 path write test. */
  LCD_InitWithProfile(2U);
  LCD_SetBacklightRaw(1U);
  for (i = 0U; i < (sizeof(colors) / sizeof(colors[0])); i++)
  {
    LCD_FillScreen(colors[i]);
    DelayMs(1800U);
  }

  /* Stage 3B: bare profile write test. */
  LCD_InitWithProfile(3U);
  LCD_SetBacklightRaw(1U);
  for (i = 0U; i < (sizeof(colors) / sizeof(colors[0])); i++)
  {
    LCD_FillScreen(colors[i]);
    DelayMs(1800U);
  }
}

static void SetStageMarker(uint8_t stage)
{
  /* Clear separator so each stage starts with a visible gap. */
  HAL_GPIO_WritePin(LED0_GPIO_Port, LED0_Pin, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin, GPIO_PIN_RESET);
  DelayMs(1000U);

  /* Stage marker pulse count = stage index. */
  while (stage-- > 0U)
  {
    HAL_GPIO_WritePin(LED0_GPIO_Port, LED0_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin, GPIO_PIN_SET);
    DelayMs(350U);
    HAL_GPIO_WritePin(LED0_GPIO_Port, LED0_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin, GPIO_PIN_RESET);
    DelayMs(350U);
  }

  switch (stage)
  {
    case 0U:
      HAL_GPIO_WritePin(LED0_GPIO_Port, LED0_Pin, GPIO_PIN_SET);
      HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin, GPIO_PIN_RESET);
      break;
    case 1U:
      HAL_GPIO_WritePin(LED0_GPIO_Port, LED0_Pin, GPIO_PIN_RESET);
      HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin, GPIO_PIN_SET);
      break;
    default:
      HAL_GPIO_WritePin(LED0_GPIO_Port, LED0_Pin, GPIO_PIN_SET);
      HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin, GPIO_PIN_SET);
      break;
  }
}

static void DelayMs(uint32_t delay_ms)
{
  uint32_t start_tick = HAL_GetTick();

  while ((HAL_GetTick() - start_tick) < delay_ms)
  {
  }
}

static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();

  GPIO_InitStruct.Pin = LED0_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(LED0_GPIO_Port, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = LED1_Pin;
  HAL_GPIO_Init(LED1_GPIO_Port, &GPIO_InitStruct);
}

/**
  * @brief  System Clock Configuration
  *         The system Clock is configured as follow : 
  *            System Clock source            = PLL (HSI)
  *            SYSCLK(Hz)                     = 64000000
  *            HCLK(Hz)                       = 64000000
  *            AHB Prescaler                  = 1
  *            APB1 Prescaler                 = 2
  *            APB2 Prescaler                 = 1
  *            PLLMUL                         = 16
  *            Flash Latency(WS)              = 2
  * @param  None
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC->CR |= RCC_CR_HSION;
  while ((RCC->CR & RCC_CR_HSIRDY) == 0U)
  {
  }

  FLASH->ACR = FLASH_ACR_PRFTBE | FLASH_ACR_LATENCY_2;
  RCC->CFGR &= ~(RCC_CFGR_HPRE | RCC_CFGR_PPRE1 | RCC_CFGR_PPRE2 | RCC_CFGR_SW | RCC_CFGR_PLLSRC | RCC_CFGR_PLLMULL);
  RCC->CFGR |= RCC_CFGR_PPRE1_DIV2 | RCC_CFGR_PLLSRC_HSI_Div2 | RCC_CFGR_PLLMULL16;

  RCC->CR |= RCC_CR_PLLON;
  while ((RCC->CR & RCC_CR_PLLRDY) == 0U)
  {
  }

  RCC->CFGR &= ~RCC_CFGR_SW;
  RCC->CFGR |= RCC_CFGR_SW_PLL;
  while ((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_PLL)
  {
  }

  SystemCoreClock = 64000000U;
}

#ifdef  USE_FULL_ASSERT

/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t* file, uint32_t line)
{ 
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */

  /* Infinite loop */
  while (1)
  {
  }
}
#endif

/**
  * @}
  */

/**
  * @}
  */
