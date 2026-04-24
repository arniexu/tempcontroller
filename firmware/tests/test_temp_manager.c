#include "bsp_ds18b20.h"
#include "temp_manager.h"
#include "test_framework.h"

int test_temp_manager_run(void)
{
    const temp_snapshot_t *t;
    unsigned int i;

    temp_manager_init();

    bsp_ds18b20_mock_set_valid(0U, true);
    bsp_ds18b20_mock_set_valid(1U, true);
    bsp_ds18b20_mock_set_valid(2U, true);
    bsp_ds18b20_mock_set_temp(0U, 30.0f);
    bsp_ds18b20_mock_set_temp(1U, 31.0f);
    bsp_ds18b20_mock_set_temp(2U, 32.0f);

    temp_manager_update();
    t = temp_manager_get_snapshot();
    TEST_ASSERT_TRUE(!t->sensor_fault);
    TEST_ASSERT_EQ_INT(t->valid_mask, 0x07U);

    bsp_ds18b20_mock_set_temp(0U, 30.0f);
    bsp_ds18b20_mock_set_temp(1U, 31.0f);
    bsp_ds18b20_mock_set_temp(2U, 80.0f);
    temp_manager_update();
    t = temp_manager_get_snapshot();
    TEST_ASSERT_NEAR_FLOAT(t->t_ctrl, 31.0f, 0.2f);

    bsp_ds18b20_mock_set_valid(2U, false);
    for (i = 0U; i < 3U; ++i)
    {
        temp_manager_update();
    }
    t = temp_manager_get_snapshot();
    TEST_ASSERT_TRUE(t->sensor_degraded);
    TEST_ASSERT_TRUE(!t->sensor_fault);

    bsp_ds18b20_mock_set_valid(1U, false);
    for (i = 0U; i < 3U; ++i)
    {
        temp_manager_update();
    }
    t = temp_manager_get_snapshot();
    TEST_ASSERT_TRUE(t->sensor_fault);

    return 0;
}
