#ifndef __STM32F1xx_HAL_H
#define __STM32F1xx_HAL_H

#include "stm32f1xx_hal_conf.h"
#include "stm32f1xx_hal_def.h"
#include "stm32_hal_legacy.h"

HAL_StatusTypeDef HAL_Init(void);
void HAL_IncTick(void);
uint32_t HAL_GetTick(void);
void HAL_MspInit(void);
void HAL_MspDeInit(void);

#endif /* __STM32F1xx_HAL_H */
