#include "stm32f1xx_hal.h"

static __IO uint32_t uwTick;

HAL_StatusTypeDef HAL_Init(void)
{
  SysTick_Config(SystemCoreClock / 1000U);
  HAL_MspInit();
  return HAL_OK;
}

void HAL_IncTick(void)
{
  uwTick++;
}

uint32_t HAL_GetTick(void)
{
  return uwTick;
}

__weak void HAL_MspInit(void)
{
}

__weak void HAL_MspDeInit(void)
{
}
