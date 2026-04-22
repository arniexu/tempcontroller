#include "param_store.h"
#include "test_framework.h"

int test_param_store_run(void)
{
    app_params_t p;

    param_store_init();
    param_store_load(&p);
    TEST_ASSERT_NEAR_FLOAT(p.set_temp_c, 45.0f, 0.01f);

    p.set_temp_c = 48.5f;
    param_store_save(&p);

    TEST_ASSERT_NEAR_FLOAT(param_store_get()->set_temp_c, 48.5f, 0.01f);
    param_store_get_mutable()->set_temp_c = 46.0f;
    TEST_ASSERT_NEAR_FLOAT(param_store_get()->set_temp_c, 46.0f, 0.01f);

    return 0;
}
