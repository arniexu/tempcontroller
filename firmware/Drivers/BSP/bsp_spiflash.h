#ifndef BSP_SPIFLASH_H
#define BSP_SPIFLASH_H

#include <stdint.h>

void bsp_spiflash_init(void);
int bsp_spiflash_read(uint32_t addr, uint8_t *buf, uint32_t len);
int bsp_spiflash_write(uint32_t addr, const uint8_t *buf, uint32_t len);
void bsp_spiflash_process(void);
int bsp_spiflash_is_busy(void);
int bsp_spiflash_get_last_status(void);
void bsp_spiflash_mock_set_access_ok(int read_ok, int write_ok);

#endif
