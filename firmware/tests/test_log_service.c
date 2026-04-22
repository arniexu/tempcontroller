#include "log_service.h"
#include "test_framework.h"

int test_log_service_run(void)
{
    temp_snapshot_t t;
    log_record_t r;

    t.t1 = 25.0f;
    t.t2 = 26.0f;
    t.t3 = 27.0f;
    t.t_ctrl = 26.0f;
    t.valid_mask = 0x07U;
    t.sensor_degraded = false;
    t.sensor_fault = false;

    log_service_init();
    log_service_push(&t, 45.0f, 33.0f, 1, 0);

    TEST_ASSERT_EQ_INT(log_service_count(), 1U);
    TEST_ASSERT_TRUE(log_service_get(0U, &r));
    TEST_ASSERT_NEAR_FLOAT(r.t_avg, 26.0f, 0.01f);

    log_service_clear();
    TEST_ASSERT_EQ_INT(log_service_count(), 0U);

    return 0;
}
