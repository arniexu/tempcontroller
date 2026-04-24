#include "param_store.h"
#include "test_framework.h"

int test_param_store_run(void)
{
    app_params_t p;
    app_params_t q;

    param_store_init();
    param_store_load(&p);
    TEST_ASSERT_NEAR_FLOAT(p.set_temp_c, 45.0f, 0.01f);

    p.set_temp_c = 48.5f;
    p.schedule_enabled = 1U;
    p.schedule_start_min = 420U;
    p.schedule_end_min = 480U;
    p.log_period_s = 5U;
    param_store_save(&p);

    TEST_ASSERT_NEAR_FLOAT(param_store_get()->set_temp_c, 48.5f, 0.01f);
    TEST_ASSERT_EQ_INT(param_store_get()->schedule_enabled, 1U);
    TEST_ASSERT_EQ_INT(param_store_get()->schedule_start_min, 420U);
    TEST_ASSERT_EQ_INT(param_store_get()->schedule_end_min, 480U);
    TEST_ASSERT_EQ_INT(param_store_get()->log_period_s, 5U);

    param_store_init();
    param_store_load(&q);
    TEST_ASSERT_NEAR_FLOAT(q.set_temp_c, 48.5f, 0.01f);
    TEST_ASSERT_EQ_INT(q.schedule_enabled, 1U);
    TEST_ASSERT_EQ_INT(q.schedule_start_min, 420U);
    TEST_ASSERT_EQ_INT(q.schedule_end_min, 480U);
    TEST_ASSERT_EQ_INT(q.log_period_s, 5U);

    param_store_get_mutable()->set_temp_c = 46.0f;
    param_store_get_mutable()->schedule_enabled = 0U;
    TEST_ASSERT_NEAR_FLOAT(param_store_get()->set_temp_c, 46.0f, 0.01f);
    param_store_save(param_store_get());

    return 0;
}
