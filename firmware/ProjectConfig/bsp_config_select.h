#ifndef BSP_CONFIG_SELECT_H
#define BSP_CONFIG_SELECT_H

#include "project_config_select.h"

#if defined(USE_HAL_DRIVER)
#define BSP_BUZZER_HW_CONFIG_ENABLE    (1U)
#define BSP_RELAY_HW_CONFIG_ENABLE     (1U)
#define BSP_KEY_HW_CONFIG_ENABLE       (1U)
#define BSP_UART_HW_CONFIG_ENABLE      (1U)
#define BSP_EEPROM_HW_CONFIG_ENABLE    (1U)
#define BSP_SPIFLASH_HW_CONFIG_ENABLE  (1U)
#define BSP_SENSOR_DS18B20_HW_CONFIG_ENABLE (COMP_SENSOR_DS18B20_ENABLE)
#else
#define BSP_BUZZER_HW_CONFIG_ENABLE    (0U)
#define BSP_RELAY_HW_CONFIG_ENABLE     (0U)
#define BSP_KEY_HW_CONFIG_ENABLE       (0U)
#define BSP_UART_HW_CONFIG_ENABLE      (0U)
#define BSP_EEPROM_HW_CONFIG_ENABLE    (0U)
#define BSP_SPIFLASH_HW_CONFIG_ENABLE  (0U)
#define BSP_SENSOR_DS18B20_HW_CONFIG_ENABLE (0U)
#endif

#if defined(APP_PROJECT_EDGEGATEWAY)
#include "bsp_configs/bsp_edgegateway.h"
#elif defined(APP_PROJECT_STM32F103)
#include "bsp_configs/bsp_stm32f103.h"
#else
#include "bsp_configs/bsp_default.h"
#endif

#endif
