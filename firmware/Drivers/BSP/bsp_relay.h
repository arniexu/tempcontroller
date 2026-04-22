#ifndef BSP_RELAY_H
#define BSP_RELAY_H

#include <stdbool.h>

void bsp_relay_init(void);
void bsp_relay_set(bool on);
bool bsp_relay_get(void);

#endif
