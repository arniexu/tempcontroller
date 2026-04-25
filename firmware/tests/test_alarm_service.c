#include "alarm_service.h"
#include "test_framework.h"

#include <math.h>

int test_alarm_service_run(void)
{
    alarm_service_init();
    alarm_service_update(40.0f, 41.0f, 42.0f, 60.0f, false);
    TEST_ASSERT_TRUE(!alarm_service_is_active());

    alarm_service_update(61.0f, 41.0f, 42.0f, 60.0f, false);
    TEST_ASSERT_TRUE(alarm_service_is_active());

    alarm_service_update(40.0f, 41.0f, 42.0f, 60.0f, true);
    TEST_ASSERT_TRUE(alarm_service_is_active());

    alarm_service_init();
    alarm_service_update(60.2f, 41.0f, 42.0f, 60.0f, false);
    TEST_ASSERT_TRUE(alarm_service_is_active());

    alarm_service_update(59.7f, 59.7f, 59.7f, 60.0f, false);
    TEST_ASSERT_TRUE(alarm_service_is_active());

    alarm_service_update(59.4f, 59.4f, 59.4f, 60.0f, false);
    TEST_ASSERT_TRUE(!alarm_service_is_active());

    alarm_service_init();
    alarm_service_update(40.0f, NAN, 42.0f, 60.0f, false);
    TEST_ASSERT_TRUE(alarm_service_is_active());

    return 0;
}
