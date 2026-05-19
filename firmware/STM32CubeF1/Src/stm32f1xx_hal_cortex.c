#include "stm32f1xx_hal_cortex.h"

#if defined(__arm__) || defined(__thumb__)
#define NVIC_ISER_BASE ((volatile uint32_t *)0xE000E100UL)
#define NVIC_IPR_BASE  ((volatile uint8_t  *)0xE000E400UL)
#endif

void HAL_NVIC_SetPriority(IRQn_Type IRQn, uint32_t PreemptPriority, uint32_t SubPriority)
{
    (void)SubPriority;
#if defined(__arm__) || defined(__thumb__)
    if ((int32_t)IRQn >= 0) {
        NVIC_IPR_BASE[(uint32_t)IRQn] = (uint8_t)((PreemptPriority & 0x0FU) << 4);
    }
#else
    (void)IRQn;
    (void)PreemptPriority;
#endif
}

void HAL_NVIC_EnableIRQ(IRQn_Type IRQn)
{
#if defined(__arm__) || defined(__thumb__)
    if ((int32_t)IRQn >= 0) {
        NVIC_ISER_BASE[((uint32_t)IRQn) >> 5U] = (1UL << (((uint32_t)IRQn) & 0x1FU));
    }
#else
    (void)IRQn;
#endif
}
