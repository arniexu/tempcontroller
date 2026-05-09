#ifndef __STM32F1xx_HAL_GPIO_H
#define __STM32F1xx_HAL_GPIO_H

#include "stm32f1xx_hal_def.h"

#define GPIO_PIN_0                 ((uint16_t)0x0001U)
#define GPIO_PIN_1                 ((uint16_t)0x0002U)
#define GPIO_PIN_2                 ((uint16_t)0x0004U)
#define GPIO_PIN_3                 ((uint16_t)0x0008U)
#define GPIO_PIN_4                 ((uint16_t)0x0010U)
#define GPIO_PIN_5                 ((uint16_t)0x0020U)
#define GPIO_PIN_6                 ((uint16_t)0x0040U)
#define GPIO_PIN_7                 ((uint16_t)0x0080U)
#define GPIO_PIN_8                 ((uint16_t)0x0100U)
#define GPIO_PIN_9                 ((uint16_t)0x0200U)
#define GPIO_PIN_10                ((uint16_t)0x0400U)
#define GPIO_PIN_11                ((uint16_t)0x0800U)
#define GPIO_PIN_12                ((uint16_t)0x1000U)
#define GPIO_PIN_13                ((uint16_t)0x2000U)
#define GPIO_PIN_14                ((uint16_t)0x4000U)
#define GPIO_PIN_15                ((uint16_t)0x8000U)
#define GPIO_PIN_All               ((uint16_t)0xFFFFU)

typedef enum
{
  GPIO_PIN_RESET = 0,
  GPIO_PIN_SET
} GPIO_PinState;

typedef struct
{
  uint32_t Pin;
  uint32_t Mode;
  uint32_t Pull;
  uint32_t Speed;
} GPIO_InitTypeDef;

#define GPIO_MODE_INPUT            0x00000000U
#define GPIO_MODE_OUTPUT_PP        0x00000001U

#define GPIO_NOPULL                0x00000000U

#define GPIO_SPEED_FREQ_LOW        0x00000002U
#define GPIO_SPEED_FREQ_MEDIUM     0x00000001U
#define GPIO_SPEED_FREQ_HIGH       0x00000003U

void HAL_GPIO_Init(GPIO_TypeDef *GPIOx, GPIO_InitTypeDef *GPIO_Init);
void HAL_GPIO_WritePin(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin, GPIO_PinState PinState);
void HAL_GPIO_TogglePin(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin);

#endif /* __STM32F1xx_HAL_GPIO_H */
