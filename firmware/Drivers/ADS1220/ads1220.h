#ifndef ADS1220_H
#define ADS1220_H

#include "stm32f1xx_hal.h"
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    SPI_HandleTypeDef *hspi;
    GPIO_TypeDef *cs_port;
    uint16_t cs_pin;
    GPIO_TypeDef *drdy_port;
    uint16_t drdy_pin;
    GPIO_TypeDef *rst_port;
    uint16_t rst_pin;
} ads1220_t;

HAL_StatusTypeDef ads1220_init(ads1220_t *dev);
HAL_StatusTypeDef ads1220_read_raw24(ads1220_t *dev, int32_t *raw, uint32_t timeout_ms);
float ads1220_raw_to_celsius(int32_t raw);

#ifdef __cplusplus
}
#endif

#endif
