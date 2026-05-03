#ifndef PROJECT_CONFIG_STM32F103_H
#define PROJECT_CONFIG_STM32F103_H

/* STM32F103 hardware profile. */
#define APP_TEMP_ALARM_THRESHOLD_C    (60.0f)
#define APP_TEMP_DEFAULT_SETPOINT_C   (45.0f)
#define APP_PID_WINDOW_MS             (10000U)

#define APP_TEMP_SENSOR_COUNT         (3U)
#define APP_TEMP_FILTER_WINDOW        (5U)
#define APP_TEMP_VALID_MIN_C          (-55.0f)
#define APP_TEMP_VALID_MAX_C          (125.0f)
#define APP_TEMP_MAX_SINGLE_STEP_C    (5.0f)
#define APP_TEMP_MAX_CONSEC_FAIL      (3U)

#define APP_HEATER_MIN_ON_MS          (1500U)
#define APP_HEATER_MIN_OFF_MS         (1500U)

/* Component switches (single source of truth in project config) */
#define COMP_TEMP_SOURCE_MOCK_ENABLE      (1U)
#define COMP_SENSOR_DS18B20_ENABLE        (0U)
#define COMP_LOG_SERVICE_ENABLE           (1U)
#define COMP_PARAM_STORE_EEPROM_ENABLE    (0U)
#define COMP_PARAM_STORE_SPIFLASH_ENABLE  (1U)
#define COMP_UI_LVGL_ENABLE               (1U)

/* Legacy APP_* compatibility mapping */
#define APP_USE_MOCK_TEMP_SOURCE      (COMP_TEMP_SOURCE_MOCK_ENABLE)
#define APP_DEBUG_LOG_ENABLE          (COMP_LOG_SERVICE_ENABLE)

#define APP_PARAM_STORE_USE_EEPROM    (COMP_PARAM_STORE_EEPROM_ENABLE)
#define APP_PARAM_STORE_USE_SPIFLASH  (COMP_PARAM_STORE_SPIFLASH_ENABLE)
#define APP_PARAM_STORE_FLUSH_DELAY_S (3U)

#define APP_TASK_KEY_PERIOD_MS        (100U)
#define APP_TASK_UI_PERIOD_MS         (200U)
#define APP_TASK_CONTROL_PERIOD_MS    (1000U)

#define APP_USE_LVGL_UI               (COMP_UI_LVGL_ENABLE)
#define APP_LVGL_TASK_PERIOD_MS       (20U)
#define APP_LVGL_BUF_LINES            (8U)
#define APP_LVGL_STARTUP_SELF_TEST    (1U)

#define APP_LOG_CAPACITY              (256U)

#endif
