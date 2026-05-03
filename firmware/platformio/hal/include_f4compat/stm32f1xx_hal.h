#ifndef STM32F1XX_HAL_COMPAT_H
#define STM32F1XX_HAL_COMPAT_H

#include "stm32f4xx_hal.h"

/* Keep legacy F1 remap calls buildable on F4; they are no-ops here. */
#ifndef __HAL_RCC_AFIO_CLK_ENABLE
#define __HAL_RCC_AFIO_CLK_ENABLE() ((void)0)
#endif

#ifndef __HAL_AFIO_REMAP_SWJ_NOJTAG
#define __HAL_AFIO_REMAP_SWJ_NOJTAG() ((void)0)
#endif

#ifndef __HAL_AFIO_REMAP_SWJ_DISABLE
#define __HAL_AFIO_REMAP_SWJ_DISABLE() ((void)0)
#endif

#endif
