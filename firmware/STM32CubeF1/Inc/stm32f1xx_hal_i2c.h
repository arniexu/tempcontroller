#ifndef STM32F1XX_HAL_I2C_H
#define STM32F1XX_HAL_I2C_H

#include "stm32f1xx_hal_def.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    void *Instance;
    HAL_LockTypeDef Lock;
} I2C_HandleTypeDef;

HAL_StatusTypeDef HAL_I2C_Master_Transmit(
    I2C_HandleTypeDef *hi2c,
    uint16_t DevAddress,
    uint8_t *pData,
    uint16_t Size,
    uint32_t Timeout);

#ifdef __cplusplus
}
#endif

#endif
