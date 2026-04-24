#include "pid_ctrl.h"
#include "test_framework.h"

int test_pid_run(void)
{
    pid_ctx_t pid;
    pid_ctx_t sat;
    float out1;
    float out2;
    float out_sat;
    float out_recover;
    unsigned int i;

    pid_init(&pid, 8.0f, 0.3f, 15.0f, 1.0f, 0.0f, 100.0f);
    out1 = pid_step(&pid, 50.0f, 25.0f);
    TEST_ASSERT_TRUE(out1 > 0.0f);

    out2 = pid_step(&pid, 25.0f, 50.0f);
    TEST_ASSERT_TRUE(out2 >= 0.0f);
    TEST_ASSERT_TRUE(out2 <= 100.0f);

    pid_init(&sat, 5.0f, 1.0f, 0.0f, 1.0f, 0.0f, 100.0f);
    for (i = 0U; i < 20U; ++i)
    {
        out_sat = pid_step(&sat, 100.0f, 0.0f);
    }
    TEST_ASSERT_TRUE(out_sat <= 100.0f);

    out_recover = pid_step(&sat, 0.0f, 100.0f);
    TEST_ASSERT_TRUE(out_recover >= 0.0f);
    TEST_ASSERT_TRUE(out_recover < 20.0f);

    return 0;
}
