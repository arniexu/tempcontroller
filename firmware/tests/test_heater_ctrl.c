#include "heater_ctrl.h"
#include "bsp_relay.h"
#include "test_framework.h"

int test_heater_ctrl_run(void)
{
    unsigned int i;

    heater_ctrl_init(1000U);
    heater_ctrl_set_output_percent(100.0f);

    for (i = 0U; i < 2000U; ++i)
    {
        heater_ctrl_update_1ms();
    }

    TEST_ASSERT_TRUE(heater_ctrl_get_state());
    TEST_ASSERT_TRUE(bsp_relay_get());

    heater_ctrl_force_off();
    TEST_ASSERT_TRUE(!heater_ctrl_get_state());
    TEST_ASSERT_TRUE(!bsp_relay_get());

    return 0;
}
