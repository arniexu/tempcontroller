#ifndef ADS1220_H
#define ADS1220_H

#include "stm32f1xx_hal.h"
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint8_t reg0;
    uint8_t reg1;
    uint8_t reg2;
    uint8_t reg3;
    uint32_t spi_timeout_ms;
    uint32_t drdy_timeout_ms;
    uint8_t io_retries;
    float temp_scale_c_per_lsb;
    float temp_offset_c;
} ads1220_config_t;

typedef struct {
    SPI_HandleTypeDef *hspi;
    GPIO_TypeDef *cs_port;
    uint16_t cs_pin;
    GPIO_TypeDef *drdy_port;
    uint16_t drdy_pin;
    GPIO_TypeDef *rst_port;
    uint16_t rst_pin;
    ads1220_config_t config;
    bool initialized;
} ads1220_t;

void ads1220_get_default_config(ads1220_config_t *config);
HAL_StatusTypeDef ads1220_init(ads1220_t *dev);
HAL_StatusTypeDef ads1220_reset(ads1220_t *dev);
HAL_StatusTypeDef ads1220_start(ads1220_t *dev);
HAL_StatusTypeDef ads1220_stop(ads1220_t *dev);
HAL_StatusTypeDef ads1220_configure(ads1220_t *dev, const ads1220_config_t *config);
HAL_StatusTypeDef ads1220_read_registers(ads1220_t *dev, uint8_t start_reg, uint8_t *data, uint8_t len);
HAL_StatusTypeDef ads1220_write_registers(ads1220_t *dev, uint8_t start_reg, const uint8_t *data, uint8_t len);
HAL_StatusTypeDef ads1220_read_raw24(ads1220_t *dev, int32_t *raw, uint32_t timeout_ms);
float ads1220_raw_to_celsius(const ads1220_t *dev, int32_t raw);

#ifdef __cplusplus
}
#endif

#endif
