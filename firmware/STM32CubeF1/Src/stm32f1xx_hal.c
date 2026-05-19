#include "stm32f1xx_hal.h"

#ifndef STM32F1XX_HAL_SKIP_DEFAULT_TICK
static __IO uint32_t uwTick = 0U;
#endif

HAL_StatusTypeDef HAL_Init(void)
{
#ifndef STM32F1XX_HAL_SKIP_DEFAULT_TICK
    uwTick = 0U;
#endif
    return HAL_OK;
}

__weak uint32_t HAL_GetTick(void)
{
#ifndef STM32F1XX_HAL_SKIP_DEFAULT_TICK
    return uwTick;
#else
    return 0U;
#endif
}

__weak void HAL_Delay(uint32_t Delay)
{
#if defined(__arm__) || defined(__thumb__)
    uint32_t tickstart = HAL_GetTick();
    while ((HAL_GetTick() - tickstart) < Delay) {
        __asm volatile("nop");
    }
#else
    (void)Delay;
#ifndef STM32F1XX_HAL_SKIP_DEFAULT_TICK
    uwTick += Delay;
#endif
#endif
}

#ifndef STM32F1XX_HAL_SKIP_DEFAULT_TICK
__weak void HAL_IncTick(void)
{
    uwTick++;
}
#endif
