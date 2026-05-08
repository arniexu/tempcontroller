#include "bsp_systick.h"
#include "stm32f1xx_hal.h"

void bsp_delay_ms(uint32_t ms)
{
    HAL_Delay(ms);
}

void bsp_delay_cycles(unsigned int n)
{
    volatile unsigned int i;
    for (i = 0U; i < n; ++i)
    {
        __NOP();
    }
}
