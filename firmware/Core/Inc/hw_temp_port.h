#ifndef HW_TEMP_PORT_H
#define HW_TEMP_PORT_H

#include <stdbool.h>
#include <stdint.h>

/*
 * Core business layer should call the hw_temp_port API only.
 *
 * Default path: use BSP DS18B20 implementation.
 * Custom path: define APP_HW_TEMP_PORT_CUSTOM and provide
 * hw_temp_port_init()/hw_temp_port_read_c() elsewhere.
 */
#if !defined(APP_HW_TEMP_PORT_CUSTOM)
#include "bsp_ds18b20.h"

static inline void hw_temp_port_init(void)
{
    bsp_ds18b20_init();
}

static inline bool hw_temp_port_read_c(uint8_t index, float *temp_c)
{
    return bsp_ds18b20_read_c(index, temp_c);
}
#else
void hw_temp_port_init(void);
bool hw_temp_port_read_c(uint8_t index, float *temp_c);
#endif

#endif
