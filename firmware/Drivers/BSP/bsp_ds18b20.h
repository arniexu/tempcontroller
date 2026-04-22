#ifndef BSP_DS18B20_H
#define BSP_DS18B20_H

#include <stdbool.h>
#include <stdint.h>

#define BSP_DS18B20_SENSOR_COUNT    (3U)

void bsp_ds18b20_init(void);
bool bsp_ds18b20_read_c(uint8_t index, float *temp_c);
void bsp_ds18b20_mock_set_temp(uint8_t index, float temp_c);
void bsp_ds18b20_mock_set_valid(uint8_t index, bool valid);

#endif
