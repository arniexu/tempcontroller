#include "ui_service.h"

#include <string.h>

#include "app_config.h"
#include "bsp_key.h"
#include "bsp_oled.h"
#include "bsp_rtc.h"
#include "log_service.h"
#include "test_framework.h"

int test_ui_service_run(void)
{
    app_params_t p;
    temp_snapshot_t t;
    temp_snapshot_t log_temp;
    unsigned int i;

    p.set_temp_c = 45.0f;
    p.alarm_threshold_c = 60.0f;
    p.kp = 8.0f;
    p.ki = 0.3f;
    p.kd = 15.0f;
    p.schedule_enabled = 1U;
    p.schedule_start_min = 8U * 60U;
    p.schedule_end_min = 10U * 60U;
    p.log_period_s = 5U;

    t.t1 = 30.0f;
    t.t2 = 30.0f;
    t.t3 = 30.0f;
    t.t_ctrl = 30.0f;
    t.valid_mask = 0x07U;
    t.sensor_degraded = false;
    t.sensor_fault = false;

    log_temp = t;

    log_service_init();
    log_service_push(&log_temp, p.set_temp_c, 10.0f, 1, 0);

    ui_service_init();

    TEST_ASSERT_EQ_INT((int)ui_service_get_page(), (int)UI_PAGE_HOME);
    TEST_ASSERT_EQ_INT(ui_service_is_editing(), 0);

    bsp_key_mock_set_state(BSP_KEY_UP, true);
    ui_service_tick_100ms(&p);
    bsp_key_mock_set_state(BSP_KEY_UP, false);
    ui_service_tick_100ms(&p);
    TEST_ASSERT_EQ_INT((int)ui_service_get_page(), (int)UI_PAGE_SET_TEMP);

    ui_service_inject_key_event(UI_KEY_SET);
    ui_service_tick_100ms(&p);
    TEST_ASSERT_EQ_INT(ui_service_is_editing(), 1);

    for (i = 0U; i < 100U; ++i)
    {
        ui_service_inject_key_event(UI_KEY_UP_REPEAT);
        ui_service_tick_100ms(&p);
    }
    TEST_ASSERT_NEAR_FLOAT(p.set_temp_c, 60.0f, 0.01f);

    ui_service_inject_key_event(UI_KEY_SET_LONG);
    ui_service_tick_100ms(&p);
    TEST_ASSERT_EQ_INT(ui_service_is_editing(), 0);

    ui_service_inject_key_event(UI_KEY_UP);
    ui_service_tick_100ms(&p);
    TEST_ASSERT_EQ_INT((int)ui_service_get_page(), (int)UI_PAGE_PID);

    ui_service_inject_key_event(UI_KEY_SET);
    ui_service_tick_100ms(&p);
    TEST_ASSERT_EQ_INT(ui_service_is_editing(), 1);
    TEST_ASSERT_EQ_INT((int)ui_service_get_pid_field(), 0);

    ui_service_inject_key_event(UI_KEY_SET);
    ui_service_tick_100ms(&p);
    TEST_ASSERT_EQ_INT((int)ui_service_get_pid_field(), 1);

    ui_service_inject_key_event(UI_KEY_UP);
    ui_service_tick_100ms(&p);
    TEST_ASSERT_NEAR_FLOAT(p.ki, 0.35f, 0.001f);

    ui_service_tick_100ms(&p);
    ui_service_tick_200ms(APP_MODE_IDLE, &t, &p, 0.0f, 0, 0);
    TEST_ASSERT_TRUE(strstr(bsp_oled_mock_get_line(0U), "IDLE") != 0);
    TEST_ASSERT_TRUE(strstr(bsp_oled_mock_get_line(1U), "TC") != 0);

    ui_service_inject_key_event(UI_KEY_SET_LONG);
    ui_service_tick_100ms(&p);

    ui_service_inject_key_event(UI_KEY_SET);
    ui_service_tick_100ms(&p);
    TEST_ASSERT_EQ_INT(ui_service_is_editing(), 1);

    bsp_key_mock_set_state(BSP_KEY_DOWN, true);
    for (i = 0U; i < 10U; ++i)
    {
        ui_service_tick_100ms(&p);
    }
    bsp_key_mock_set_state(BSP_KEY_DOWN, false);
    ui_service_tick_100ms(&p);
    TEST_ASSERT_EQ_INT(ui_service_is_editing(), 0);

    ui_service_inject_key_event(UI_KEY_UP);
    ui_service_tick_100ms(&p);
    TEST_ASSERT_EQ_INT((int)ui_service_get_page(), (int)UI_PAGE_ALARM);

    bsp_key_mock_set_state(BSP_KEY_DOWN, true);
    for (i = 0U; i < 10U; ++i)
    {
        ui_service_tick_100ms(&p);
    }
    bsp_key_mock_set_state(BSP_KEY_DOWN, false);
    ui_service_tick_100ms(&p);
    TEST_ASSERT_EQ_INT((int)ui_service_get_page(), (int)UI_PAGE_HOME);

    ui_service_inject_key_event(UI_KEY_UP);
    ui_service_tick_100ms(&p);
    ui_service_inject_key_event(UI_KEY_UP);
    ui_service_tick_100ms(&p);
    ui_service_inject_key_event(UI_KEY_UP);
    ui_service_tick_100ms(&p);
    ui_service_inject_key_event(UI_KEY_UP);
    ui_service_tick_100ms(&p);

    bsp_rtc_mock_set_valid(true);
    bsp_rtc_mock_set_minutes_of_day(7U * 60U + 30U);
    ui_service_tick_200ms(APP_MODE_SCHEDULED, &t, &p, 0.0f, 0, 0);
    TEST_ASSERT_EQ_INT((int)ui_service_get_page(), (int)UI_PAGE_SCHEDULE);
    TEST_ASSERT_TRUE(strstr(bsp_oled_mock_get_line(2U), "07:30") != 0);
    TEST_ASSERT_TRUE(strstr(bsp_oled_mock_get_line(2U), "08:00") != 0);
    TEST_ASSERT_TRUE(strstr(bsp_oled_mock_get_line(3U), "ON 08:00-10:00") != 0);

    ui_service_inject_key_event(UI_KEY_UP);
    ui_service_tick_100ms(&p);
    ui_service_tick_200ms(APP_MODE_EXPORT, &t, &p, 0.0f, 0, 0);
    TEST_ASSERT_EQ_INT((int)ui_service_get_page(), (int)UI_PAGE_EXPORT);
    TEST_ASSERT_TRUE(strstr(bsp_oled_mock_get_line(2U), "LOG 1") != 0);
    TEST_ASSERT_TRUE(strstr(bsp_oled_mock_get_line(2U), "PER 5s") != 0);
    TEST_ASSERT_TRUE(strstr(bsp_oled_mock_get_line(3U), "LOG_EXPORT") != 0);

    p.set_temp_c = 58.0f;
    p.alarm_threshold_c = 75.0f;
    p.kp = 10.0f;
    p.ki = 1.0f;
    p.kd = 20.0f;
    p.schedule_enabled = 1U;
    p.schedule_start_min = 300U;
    p.schedule_end_min = 360U;
    p.log_period_s = 9U;

    ui_service_inject_key_event(UI_KEY_UP);
    ui_service_tick_100ms(&p);
    TEST_ASSERT_EQ_INT((int)ui_service_get_page(), (int)UI_PAGE_INFO);

    ui_service_inject_key_event(UI_KEY_SET_LONG);
    ui_service_tick_100ms(&p);
    ui_service_tick_200ms(APP_MODE_IDLE, &t, &p, 0.0f, 0, 0);
    TEST_ASSERT_TRUE(strstr(bsp_oled_mock_get_line(3U), "HOLD AGAIN RESET") != 0);

    ui_service_inject_key_event(UI_KEY_SET_LONG);
    ui_service_tick_100ms(&p);
    TEST_ASSERT_NEAR_FLOAT(p.set_temp_c, APP_TEMP_DEFAULT_SETPOINT_C, 0.01f);
    TEST_ASSERT_NEAR_FLOAT(p.alarm_threshold_c, APP_TEMP_ALARM_THRESHOLD_C, 0.01f);
    TEST_ASSERT_NEAR_FLOAT(p.kp, 8.0f, 0.01f);
    TEST_ASSERT_NEAR_FLOAT(p.ki, 0.3f, 0.01f);
    TEST_ASSERT_NEAR_FLOAT(p.kd, 15.0f, 0.01f);
    TEST_ASSERT_EQ_INT(p.schedule_enabled, 0U);
    TEST_ASSERT_EQ_INT(p.schedule_start_min, 8U * 60U);
    TEST_ASSERT_EQ_INT(p.schedule_end_min, 10U * 60U);
    TEST_ASSERT_EQ_INT(p.log_period_s, 1U);

    ui_service_tick_200ms(APP_MODE_IDLE, &t, &p, 0.0f, 0, 0);
    TEST_ASSERT_TRUE(strstr(bsp_oled_mock_get_line(3U), "DEFAULTS APPLIED") != 0);

    TEST_ASSERT_TRUE(1);
    return 0;
}
