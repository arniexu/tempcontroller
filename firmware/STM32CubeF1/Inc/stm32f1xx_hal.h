#ifndef STM32F1XX_HAL_H
#define STM32F1XX_HAL_H

#include "stm32f1xx_hal_def.h"
#include "stm32f1xx.h"
#include "stm32f1xx_hal_gpio.h"
#include "stm32f1xx_hal_spi.h"
#include "stm32f1xx_hal_i2c.h"
#include "stm32f1xx_hal_cortex.h"

#ifdef __cplusplus
extern "C" {
#endif

HAL_StatusTypeDef HAL_Init(void);
uint32_t HAL_GetTick(void);
void HAL_Delay(uint32_t Delay);

#ifdef __cplusplus
}
#endif

#endif
