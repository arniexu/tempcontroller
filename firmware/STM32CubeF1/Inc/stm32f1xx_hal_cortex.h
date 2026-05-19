#ifndef STM32F1XX_HAL_CORTEX_H
#define STM32F1XX_HAL_CORTEX_H

#include "stm32f1xx_hal_def.h"
#include "stm32f1xx.h"

#ifdef __cplusplus
extern "C" {
#endif

void HAL_NVIC_SetPriority(IRQn_Type IRQn, uint32_t PreemptPriority, uint32_t SubPriority);
void HAL_NVIC_EnableIRQ(IRQn_Type IRQn);

#ifdef __cplusplus
}
#endif

#endif
