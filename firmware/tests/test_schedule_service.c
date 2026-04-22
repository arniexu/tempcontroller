#include "schedule_service.h"
#include "bsp_rtc.h"
#include "test_framework.h"

int test_schedule_service_run(void)
{
    schedule_config_t cfg;

    schedule_service_init();
    schedule_service_update();
    TEST_ASSERT_TRUE(schedule_service_heating_allowed());

    cfg.enabled = true;
    cfg.start_min_of_day = 8U * 60U;
    cfg.end_min_of_day = 10U * 60U;
    schedule_service_set_config(&cfg);

    bsp_rtc_mock_set_valid(true);
    bsp_rtc_mock_set_minutes_of_day(9U * 60U);
    schedule_service_update();
    TEST_ASSERT_TRUE(schedule_service_heating_allowed());

    bsp_rtc_mock_set_minutes_of_day(11U * 60U);
    schedule_service_update();
    TEST_ASSERT_TRUE(!schedule_service_heating_allowed());

    cfg.start_min_of_day = 22U * 60U;
    cfg.end_min_of_day = 2U * 60U;
    schedule_service_set_config(&cfg);

    bsp_rtc_mock_set_minutes_of_day(23U * 60U);
    schedule_service_update();
    TEST_ASSERT_TRUE(schedule_service_heating_allowed());

    bsp_rtc_mock_set_minutes_of_day(3U * 60U);
    schedule_service_update();
    TEST_ASSERT_TRUE(!schedule_service_heating_allowed());

    bsp_rtc_mock_set_valid(false);
    schedule_service_update();
    TEST_ASSERT_TRUE(!schedule_service_heating_allowed());

    return 0;
}
