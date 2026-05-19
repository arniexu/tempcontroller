#ifndef OLED96X96_I2C_H
#define OLED96X96_I2C_H

#include "stm32f1xx_hal.h"
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint32_t i2c_timeout_ms;
    uint8_t io_retries;
} oled96x96_config_t;

typedef struct {
    I2C_HandleTypeDef *hi2c;
    uint16_t addr_7bit;
    uint8_t framebuffer[96U * 96U / 8U];
    oled96x96_config_t config;
    bool initialized;
} oled96x96_t;

void oled96x96_get_default_config(oled96x96_config_t *config);
HAL_StatusTypeDef oled96x96_init(oled96x96_t *dev);
HAL_StatusTypeDef oled96x96_fill(oled96x96_t *dev, uint8_t value);
HAL_StatusTypeDef oled96x96_flush(oled96x96_t *dev);
HAL_StatusTypeDef oled96x96_show_status(oled96x96_t *dev, bool heating, float current_temp, float target_temp, float tolerance);

#ifdef __cplusplus
}
#endif

#endif
