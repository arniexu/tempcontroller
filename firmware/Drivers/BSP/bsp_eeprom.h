#ifndef BSP_EEPROM_H
#define BSP_EEPROM_H

#include <stdint.h>

void bsp_eeprom_init(void);
int bsp_eeprom_read(uint16_t addr, uint8_t *buf, uint16_t len);
int bsp_eeprom_write(uint16_t addr, const uint8_t *buf, uint16_t len);
int bsp_eeprom_write_async(uint16_t addr, const uint8_t *buf, uint16_t len);
void bsp_eeprom_process(void);
int bsp_eeprom_is_busy(void);
int bsp_eeprom_get_last_status(void);
void bsp_eeprom_mock_set_access_ok(int read_ok, int write_ok);

#endif
