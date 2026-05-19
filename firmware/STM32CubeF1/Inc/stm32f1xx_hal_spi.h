#ifndef STM32F1XX_HAL_SPI_H
#define STM32F1XX_HAL_SPI_H

#include "stm32f1xx_hal_def.h"
#include "stm32f1xx.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    SPI_TypeDef *Instance;
    HAL_LockTypeDef Lock;
} SPI_HandleTypeDef;

HAL_StatusTypeDef HAL_SPI_Transmit(
    SPI_HandleTypeDef *hspi,
    uint8_t *pData,
    uint16_t Size,
    uint32_t Timeout);

HAL_StatusTypeDef HAL_SPI_Receive(
    SPI_HandleTypeDef *hspi,
    uint8_t *pData,
    uint16_t Size,
    uint32_t Timeout);

#ifdef __cplusplus
}
#endif

#endif
