#include "app_main.h"

#include "bsp_buzzer.h"
#include "bsp_ds18b20.h"
#include "test_framework.h"

int test_app_main_run(void)
{
    unsigned int i;

    app_main_init();
    TEST_ASSERT_TRUE(!bsp_buzzer_get());

    for (i = 0U; i < 2500U; ++i)
    {
        app_main_loop();
    }

    bsp_ds18b20_mock_set_valid(0U, true);
    bsp_ds18b20_mock_set_valid(1U, true);
    bsp_ds18b20_mock_set_valid(2U, true);
    bsp_ds18b20_mock_set_temp(0U, 70.0f);
    bsp_ds18b20_mock_set_temp(1U, 69.0f);
    bsp_ds18b20_mock_set_temp(2U, 68.0f);
    for (i = 0U; i < 1200U; ++i)
    {
        app_main_loop();
    }
    TEST_ASSERT_TRUE(bsp_buzzer_get());

    TEST_ASSERT_TRUE(1);
    return 0;
}
