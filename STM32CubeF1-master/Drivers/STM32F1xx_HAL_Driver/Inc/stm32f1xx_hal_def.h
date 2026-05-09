#ifndef __STM32F1xx_HAL_DEF_H
#define __STM32F1xx_HAL_DEF_H

#include "stm32f1xx.h"

#define __weak __attribute__((weak))

typedef enum
{
  HAL_OK       = 0x00U,
  HAL_ERROR    = 0x01U,
  HAL_BUSY     = 0x02U,
  HAL_TIMEOUT  = 0x03U
} HAL_StatusTypeDef;

typedef enum
{
  HAL_UNLOCKED = 0x00U,
  HAL_LOCKED   = 0x01U
} HAL_LockTypeDef;

#endif /* __STM32F1xx_HAL_DEF_H */
