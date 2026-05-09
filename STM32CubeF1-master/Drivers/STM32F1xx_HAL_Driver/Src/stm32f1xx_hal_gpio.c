#include "stm32f1xx_hal.h"

void HAL_GPIO_Init(GPIO_TypeDef *GPIOx, GPIO_InitTypeDef *GPIO_Init)
{
  uint32_t position;

  for (position = 0U; position < 16U; position++)
  {
    uint32_t ioposition = 1U << position;

    if ((GPIO_Init->Pin & ioposition) != 0U)
    {
      __IO uint32_t *configregister;
      uint32_t registeroffset;
      uint32_t config = 0x4U;

      if (GPIO_Init->Mode == GPIO_MODE_OUTPUT_PP)
      {
        config = (GPIO_Init->Speed & 0x3U);
      }

      configregister = (position < 8U) ? &GPIOx->CRL : &GPIOx->CRH;
      registeroffset = (position % 8U) * 4U;
      MODIFY_REG((*configregister), (0xFU << registeroffset), (config << registeroffset));
    }
  }
}

void HAL_GPIO_WritePin(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin, GPIO_PinState PinState)
{
  if (PinState == GPIO_PIN_SET)
  {
    GPIOx->BSRR = GPIO_Pin;
  }
  else
  {
    GPIOx->BRR = GPIO_Pin;
  }
}

void HAL_GPIO_TogglePin(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin)
{
  if ((GPIOx->ODR & GPIO_Pin) != 0U)
  {
    GPIOx->BRR = GPIO_Pin;
  }
  else
  {
    GPIOx->BSRR = GPIO_Pin;
  }
}
