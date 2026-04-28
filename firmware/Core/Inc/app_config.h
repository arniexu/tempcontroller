#ifndef APP_CONFIG_H
#define APP_CONFIG_H

#define APP_TEMP_ALARM_THRESHOLD_C      (60.0f)
#define APP_TEMP_DEFAULT_SETPOINT_C     (45.0f)
#define APP_PID_WINDOW_MS               (10000U)

#define APP_TEMP_SENSOR_COUNT           (3U)
#define APP_TEMP_FILTER_WINDOW          (5U)
#define APP_TEMP_VALID_MIN_C            (-55.0f)
#define APP_TEMP_VALID_MAX_C            (125.0f)
#define APP_TEMP_MAX_SINGLE_STEP_C      (5.0f)
#define APP_TEMP_MAX_CONSEC_FAIL        (3U)

#define APP_HEATER_MIN_ON_MS            (1500U)
#define APP_HEATER_MIN_OFF_MS           (1500U)

#define APP_USE_MOCK_TEMP_SOURCE        (1U)

/*
 * When project is built for STM32F103 target, define USE_STDPERIPH_DRIVER
 * in compiler symbols to enable StdPeriph-based BSP implementation.
 */
#define APP_UART_BAUDRATE               (115200U)

#define APP_DEBUG_LOG_ENABLE            (1U)

#define APP_PARAM_STORE_USE_EEPROM      (0U)
#define APP_PARAM_STORE_USE_SPIFLASH    (1U)
#define APP_PARAM_STORE_FLUSH_DELAY_S   (3U)
#define APP_EEPROM_I2C_ADDR_7BIT        (0x50U)
#define APP_EEPROM_I2C_SPEED_HZ         (100000U)
#define APP_EEPROM_TOTAL_SIZE_BYTES     (256U)
#define APP_EEPROM_PAGE_SIZE_BYTES      (8U)

#define APP_SPIFLASH_TOTAL_SIZE_BYTES   (8U * 1024U * 1024U)

#define APP_TASK_KEY_PERIOD_MS          (100U)
#define APP_TASK_UI_PERIOD_MS           (200U)
#define APP_TASK_CONTROL_PERIOD_MS      (1000U)

#endif
