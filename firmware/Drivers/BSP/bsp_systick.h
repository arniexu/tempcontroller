#ifndef BSP_SYSTICK_H
#define BSP_SYSTICK_H

#include <stdint.h>

void bsp_delay_ms(uint32_t ms);
void bsp_delay_cycles(unsigned int n);

#endif // BSP_SYSTICK_H
