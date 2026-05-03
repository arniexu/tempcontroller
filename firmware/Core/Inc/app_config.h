#ifndef APP_CONFIG_H
#define APP_CONFIG_H

#include "../../ProjectConfig/project_config_select.h"
#include "../../ProjectConfig/bsp_config_select.h"

#if defined(USE_STDPERIPH_DRIVER) || defined(USE_HAL_DRIVER)
#if !defined(APP_BUILD_DEBUG) && !defined(APP_BUILD_RELEASE)
#error "Keil hardware targets must define exactly one of APP_BUILD_DEBUG or APP_BUILD_RELEASE."
#endif
#endif

#if !defined(APP_BUILD_DEBUG)
#define APP_BUILD_DEBUG                 (0U)
#endif

#if !defined(APP_BUILD_RELEASE)
#define APP_BUILD_RELEASE               (0U)
#endif

#if ((APP_BUILD_DEBUG + APP_BUILD_RELEASE) > 1U)
#error "APP_BUILD_DEBUG and APP_BUILD_RELEASE cannot both be enabled."
#endif

#if !defined(APP_HW_DRIVER_TEST_DISPLAY)
#define APP_HW_DRIVER_TEST_DISPLAY      (0U)
#endif

#if (APP_BUILD_RELEASE == 1U) && (APP_HW_DRIVER_TEST_DISPLAY == 1U)
#error "Display driver hardware tests must not be compiled into release builds."
#endif

/*
 * When project is built for STM32F103 target, define USE_STDPERIPH_DRIVER
 * or USE_HAL_DRIVER in compiler symbols to enable hardware BSP implementation.
 */

#if !defined(APP_TEMP_ALARM_THRESHOLD_C)
#error "Project config must define APP_TEMP_ALARM_THRESHOLD_C"
#endif
#if !defined(APP_TEMP_DEFAULT_SETPOINT_C)
#error "Project config must define APP_TEMP_DEFAULT_SETPOINT_C"
#endif
#if !defined(APP_PID_WINDOW_MS)
#error "Project config must define APP_PID_WINDOW_MS"
#endif
#if !defined(APP_TEMP_SENSOR_COUNT)
#error "Project config must define APP_TEMP_SENSOR_COUNT"
#endif
#if !defined(APP_HEATER_MIN_ON_MS)
#error "Project config must define APP_HEATER_MIN_ON_MS"
#endif
#if !defined(COMP_TEMP_SOURCE_MOCK_ENABLE)
#error "Project config must define COMP_TEMP_SOURCE_MOCK_ENABLE"
#endif
#if !defined(COMP_SENSOR_DS18B20_ENABLE)
#error "Project config must define COMP_SENSOR_DS18B20_ENABLE"
#endif
#if !defined(COMP_LOG_SERVICE_ENABLE)
#error "Project config must define COMP_LOG_SERVICE_ENABLE"
#endif
#if !defined(COMP_PARAM_STORE_EEPROM_ENABLE)
#error "Project config must define COMP_PARAM_STORE_EEPROM_ENABLE"
#endif
#if !defined(COMP_PARAM_STORE_SPIFLASH_ENABLE)
#error "Project config must define COMP_PARAM_STORE_SPIFLASH_ENABLE"
#endif
#if !defined(COMP_UI_LVGL_ENABLE)
#error "Project config must define COMP_UI_LVGL_ENABLE"
#endif
#if !defined(APP_USE_MOCK_TEMP_SOURCE)
#error "Project config must define APP_USE_MOCK_TEMP_SOURCE"
#endif
#if !defined(APP_PARAM_STORE_USE_EEPROM)
#error "Project config must define APP_PARAM_STORE_USE_EEPROM"
#endif
#if !defined(APP_PARAM_STORE_USE_SPIFLASH)
#error "Project config must define APP_PARAM_STORE_USE_SPIFLASH"
#endif
#if !defined(APP_TASK_KEY_PERIOD_MS)
#error "Project config must define APP_TASK_KEY_PERIOD_MS"
#endif
#if !defined(APP_USE_LVGL_UI)
#error "Project config must define APP_USE_LVGL_UI"
#endif
#if !defined(APP_LVGL_TASK_PERIOD_MS)
#error "Project config must define APP_LVGL_TASK_PERIOD_MS"
#endif
#if !defined(APP_LOG_CAPACITY)
#error "Project config must define APP_LOG_CAPACITY"
#endif

#if !defined(BSP_UART_BAUDRATE)
#error "BSP config must define BSP_UART_BAUDRATE"
#endif

#if (BSP_BUZZER_HW_CONFIG_ENABLE == 1U)
#if !defined(BSP_BUZZER_GPIO_PORT) || !defined(BSP_BUZZER_PIN)
#error "BSP config must define BSP_BUZZER_GPIO_PORT/BSP_BUZZER_PIN"
#endif
#endif

#if (BSP_RELAY_HW_CONFIG_ENABLE == 1U)
#if !defined(BSP_RELAY_GPIO_PORT) || !defined(BSP_RELAY_PIN)
#error "BSP config must define BSP_RELAY_GPIO_PORT/BSP_RELAY_PIN"
#endif
#endif

#if (BSP_KEY_HW_CONFIG_ENABLE == 1U)
#if !defined(BSP_KEY_GPIO_PORT)
#error "BSP config must define BSP_KEY_GPIO_PORT"
#endif
#if !defined(BSP_KEY_PIN_SET) || !defined(BSP_KEY_PIN_UP) || !defined(BSP_KEY_PIN_DOWN)
#error "BSP config must define BSP_KEY_PIN_SET/BSP_KEY_PIN_UP/BSP_KEY_PIN_DOWN"
#endif
#endif

#if (BSP_UART_HW_CONFIG_ENABLE == 1U)
#if !defined(BSP_UART_GPIO_PORT) || !defined(BSP_UART_PIN_TX) || !defined(BSP_UART_PIN_RX)
#error "BSP config must define BSP_UART_GPIO_PORT/BSP_UART_PIN_TX/BSP_UART_PIN_RX"
#endif
#endif

#if (BSP_EEPROM_HW_CONFIG_ENABLE == 1U)
#if !defined(BSP_EEPROM_I2C_ADDR_7BIT)
#error "BSP config must define BSP_EEPROM_I2C_ADDR_7BIT"
#endif
#if !defined(BSP_EEPROM_I2C_SPEED_HZ)
#error "BSP config must define BSP_EEPROM_I2C_SPEED_HZ"
#endif
#if !defined(BSP_EEPROM_TOTAL_SIZE_BYTES)
#error "BSP config must define BSP_EEPROM_TOTAL_SIZE_BYTES"
#endif
#if !defined(BSP_EEPROM_PAGE_SIZE_BYTES)
#error "BSP config must define BSP_EEPROM_PAGE_SIZE_BYTES"
#endif
#if !defined(BSP_EEPROM_GPIO_PORT) || !defined(BSP_EEPROM_PIN_SCL) || !defined(BSP_EEPROM_PIN_SDA)
#error "BSP config must define BSP_EEPROM_GPIO_PORT/BSP_EEPROM_PIN_SCL/BSP_EEPROM_PIN_SDA"
#endif
#endif

#if (BSP_SPIFLASH_HW_CONFIG_ENABLE == 1U)
#if !defined(BSP_SPIFLASH_TOTAL_SIZE_BYTES)
#error "BSP config must define BSP_SPIFLASH_TOTAL_SIZE_BYTES"
#endif
#if !defined(BSP_SPIFLASH_GPIO_PORT) || !defined(BSP_SPIFLASH_PIN_SCK) || !defined(BSP_SPIFLASH_PIN_MISO) || !defined(BSP_SPIFLASH_PIN_MOSI) || !defined(BSP_SPIFLASH_PIN_CS)
#error "BSP config must define SPI flash pin mapping macros"
#endif
#endif

#if (BSP_SENSOR_DS18B20_HW_CONFIG_ENABLE == 1U)
#if !defined(BSP_SENSOR_DS18B20_SENSOR_COUNT)
#error "BSP config must define BSP_SENSOR_DS18B20_SENSOR_COUNT"
#endif
#if !defined(BSP_SENSOR_DS18B20_GPIO_APB2_MASK)
#error "BSP config must define BSP_SENSOR_DS18B20_GPIO_APB2_MASK"
#endif
#if !defined(BSP_SENSOR_DS18B20_GPIO_PORT_0) || !defined(BSP_SENSOR_DS18B20_GPIO_PORT_1) || !defined(BSP_SENSOR_DS18B20_GPIO_PORT_2)
#error "BSP config must define BSP_SENSOR_DS18B20_GPIO_PORT_0/1/2"
#endif
#if !defined(BSP_SENSOR_DS18B20_PIN_0) || !defined(BSP_SENSOR_DS18B20_PIN_1) || !defined(BSP_SENSOR_DS18B20_PIN_2)
#error "BSP config must define BSP_SENSOR_DS18B20_PIN_0/1/2"
#endif
#if !defined(BSP_SENSOR_DS18B20_PIN_SOURCE_0) || !defined(BSP_SENSOR_DS18B20_PIN_SOURCE_1) || !defined(BSP_SENSOR_DS18B20_PIN_SOURCE_2)
#error "BSP config must define BSP_SENSOR_DS18B20_PIN_SOURCE_0/1/2"
#endif
#endif

#if !defined(APP_UART_BAUDRATE)
#define APP_UART_BAUDRATE             (BSP_UART_BAUDRATE)
#endif
#if !defined(APP_EEPROM_I2C_ADDR_7BIT)
#define APP_EEPROM_I2C_ADDR_7BIT      (BSP_EEPROM_I2C_ADDR_7BIT)
#endif
#if !defined(APP_EEPROM_I2C_SPEED_HZ)
#define APP_EEPROM_I2C_SPEED_HZ       (BSP_EEPROM_I2C_SPEED_HZ)
#endif
#if !defined(APP_EEPROM_TOTAL_SIZE_BYTES)
#define APP_EEPROM_TOTAL_SIZE_BYTES   (BSP_EEPROM_TOTAL_SIZE_BYTES)
#endif
#if !defined(APP_EEPROM_PAGE_SIZE_BYTES)
#define APP_EEPROM_PAGE_SIZE_BYTES    (BSP_EEPROM_PAGE_SIZE_BYTES)
#endif
#if !defined(APP_SPIFLASH_TOTAL_SIZE_BYTES)
#define APP_SPIFLASH_TOTAL_SIZE_BYTES (BSP_SPIFLASH_TOTAL_SIZE_BYTES)
#endif

#if (COMP_PARAM_STORE_EEPROM_ENABLE + COMP_PARAM_STORE_SPIFLASH_ENABLE) > 1U
#error "COMP_PARAM_STORE_EEPROM_ENABLE and COMP_PARAM_STORE_SPIFLASH_ENABLE cannot both be enabled."
#endif

#if ((COMP_TEMP_SOURCE_MOCK_ENABLE + COMP_SENSOR_DS18B20_ENABLE) != 1U)
#error "Exactly one temperature source component must be enabled."
#endif

#if (APP_PARAM_STORE_USE_EEPROM + APP_PARAM_STORE_USE_SPIFLASH) > 1U
#error "APP_PARAM_STORE_USE_EEPROM and APP_PARAM_STORE_USE_SPIFLASH cannot both be enabled."
#endif

#endif

