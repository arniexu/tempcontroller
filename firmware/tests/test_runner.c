#include <stdio.h>

#include "test_decls.h"

typedef struct
{
    const char *name;
    int (*fn)(void);
} test_case_t;

int main(void)
{
    unsigned int i;
    unsigned int fail_count = 0U;

    const test_case_t cases[] = {
        {"bsp_oled_driver", test_bsp_oled_driver_run},
        {"bsp_lcd_8080_driver", test_bsp_lcd_8080_driver_run},
        {"scheduler", test_scheduler_run},
        {"pid", test_pid_run},
        {"debug_log", test_debug_log_run},
        {"alarm_service", test_alarm_service_run},
        {"param_store", test_param_store_run},
        {"log_service", test_log_service_run},
        {"heater_ctrl", test_heater_ctrl_run},
        {"temp_manager", test_temp_manager_run},
        {"schedule_service", test_schedule_service_run},
        {"tune_service", test_tune_service_run},
        {"ui_service", test_ui_service_run},
        {"ui_legacy_tabs_mock", test_ui_legacy_tabs_mock_run},
        {"protocol_export", test_protocol_export_run},
        {"app_main", test_app_main_run}
    };

    for (i = 0U; i < (sizeof(cases) / sizeof(cases[0])); ++i)
    {
        int rc = cases[i].fn();
        if (rc == 0)
        {
            printf("[PASS] %s\n", cases[i].name);
        }
        else
        {
            printf("[FAIL] %s (code=%d)\n", cases[i].name, rc);
            fail_count++;
        }
    }

    if (fail_count == 0U)
    {
        printf("All module self-tests passed.\n");
        return 0;
    }

    printf("%u test(s) failed.\n", fail_count);
    return 1;
}
