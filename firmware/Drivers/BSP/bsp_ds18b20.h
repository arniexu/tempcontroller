#ifndef BSP_DS18B20_H
#define BSP_DS18B20_H

#include <stdbool.h>
#include <stdint.h>

#define BSP_DS18B20_SENSOR_COUNT    (3U)

typedef enum
{
	BSP_DS18B20_PRESENCE_IRQ_ONLY = 0,
	BSP_DS18B20_PRESENCE_IRQ_DMA = 1
} bsp_ds18b20_presence_mode_t;

typedef struct
{
	uint32_t reset_timeout_count[BSP_DS18B20_SENSOR_COUNT];
	uint32_t convert_timeout_count[BSP_DS18B20_SENSOR_COUNT];
	uint32_t crc_error_count[BSP_DS18B20_SENSOR_COUNT];
	uint32_t read_fail_count[BSP_DS18B20_SENSOR_COUNT];
	uint32_t bus_stuck_low_count[BSP_DS18B20_SENSOR_COUNT];
	uint32_t irq_timeout_count;
	uint32_t dma_error_count;
	bsp_ds18b20_presence_mode_t presence_mode;
} bsp_ds18b20_diag_t;

void bsp_ds18b20_init(void);
bool bsp_ds18b20_read_c(uint8_t index, float *temp_c);
void bsp_ds18b20_set_presence_mode(bsp_ds18b20_presence_mode_t mode);
bsp_ds18b20_presence_mode_t bsp_ds18b20_get_presence_mode(void);
void bsp_ds18b20_get_diag(bsp_ds18b20_diag_t *diag);
void bsp_ds18b20_reset_diag(void);
void bsp_ds18b20_mock_set_temp(uint8_t index, float temp_c);
void bsp_ds18b20_mock_set_valid(uint8_t index, bool valid);

#endif
