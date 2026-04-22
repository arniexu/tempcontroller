#include "scheduler.h"
#include "test_framework.h"

int test_scheduler_run(void)
{
    scheduler_flags_t flags;
    unsigned int i;

    scheduler_init();

    for (i = 0U; i < 100U; ++i)
    {
        scheduler_tick_1ms();
    }
    scheduler_poll(&flags);
    TEST_ASSERT_TRUE(flags.task_key_100ms);
    TEST_ASSERT_TRUE(!flags.task_ui_200ms);
    TEST_ASSERT_TRUE(!flags.task_control_1s);

    for (i = 0U; i < 100U; ++i)
    {
        scheduler_tick_1ms();
    }
    scheduler_poll(&flags);
    TEST_ASSERT_TRUE(flags.task_ui_200ms);

    for (i = 0U; i < 800U; ++i)
    {
        scheduler_tick_1ms();
    }
    scheduler_poll(&flags);
    TEST_ASSERT_TRUE(flags.task_control_1s);

    return 0;
}
