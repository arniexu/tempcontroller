#ifndef STM32F1XX_HAL_DEF_H
#define STM32F1XX_HAL_DEF_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    HAL_OK       = 0x00U,
    HAL_ERROR    = 0x01U,
    HAL_BUSY     = 0x02U,
    HAL_TIMEOUT  = 0x03U
} HAL_StatusTypeDef;

typedef enum {
    HAL_UNLOCKED = 0x00U,
    HAL_LOCKED   = 0x01U
} HAL_LockTypeDef;

#define __IO volatile
#define __weak __attribute__((weak))

#ifdef __cplusplus
}
#endif

#endif
