#include "ui_service.h"

#include <string.h>

#include "bsp_key.h"
#include "bsp_oled.h"
#include "test_framework.h"

int test_ui_service_run(void)
{
    app_params_t p;
    temp_snapshot_t t;
    unsigned int i;

    p.set_temp_c = 45.0f;
    p.alarm_threshold_c = 60.0f;
    p.kp = 8.0f;
    p.ki = 0.3f;
    p.kd = 15.0f;

    t.t1 = 30.0f;
    t.t2 = 30.0f;
    t.t3 = 30.0f;
    t.t_ctrl = 30.0f;
    t.valid_mask = 0x07U;
    t.sensor_degraded = false;
    t.sensor_fault = false;

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

    TEST_ASSERT_TRUE(1);
    return 0;
}
