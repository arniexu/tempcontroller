#ifndef BSP_BUZZER_H
#define BSP_BUZZER_H

#include <stdbool.h>

void bsp_buzzer_init(void);
void bsp_buzzer_set(bool on);
bool bsp_buzzer_get(void);

#endif
