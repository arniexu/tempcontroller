#ifndef __STM32F1xx_HAL_RCC_H
#define __STM32F1xx_HAL_RCC_H

#include "stm32f1xx_hal_def.h"

#define __HAL_RCC_GPIOA_CLK_ENABLE() do { SET_BIT(RCC->APB2ENR, RCC_APB2ENR_IOPAEN); (void)READ_REG(RCC->APB2ENR); } while (0)
#define __HAL_RCC_GPIOD_CLK_ENABLE() do { SET_BIT(RCC->APB2ENR, RCC_APB2ENR_IOPDEN); (void)READ_REG(RCC->APB2ENR); } while (0)
#define __HAL_RCC_AFIO_CLK_ENABLE()  do { SET_BIT(RCC->APB2ENR, RCC_APB2ENR_AFIOEN); (void)READ_REG(RCC->APB2ENR); } while (0)

#endif /* __STM32F1xx_HAL_RCC_H */
