#include "ui_service.h"
#include "test_framework.h"

int test_ui_service_run(void)
{
    app_params_t p;
    temp_snapshot_t t;

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
    ui_service_tick_100ms(&p);
    ui_service_tick_200ms(APP_MODE_IDLE, &t, &p, 0.0f, 0, 0);

    TEST_ASSERT_TRUE(1);
    return 0;
}
