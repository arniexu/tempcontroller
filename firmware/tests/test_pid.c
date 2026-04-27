#include "pid_ctrl.h"
#include "test_framework.h"

static int test_pid_basic_range(void)
{
    pid_ctx_t pid;
    float out1;
    float out2;

    pid_init(&pid, 8.0f, 0.3f, 15.0f, 1.0f, 0.0f, 100.0f);
    out1 = pid_step(&pid, 50.0f, 25.0f);
    TEST_ASSERT_TRUE(out1 > 0.0f);

    out2 = pid_step(&pid, 25.0f, 50.0f);
    TEST_ASSERT_TRUE(out2 >= 0.0f);
    TEST_ASSERT_TRUE(out2 <= 100.0f);

    return 0;
}

static int test_pid_anti_windup_recovery(void)
{
    pid_ctx_t sat;
    float out_sat = 0.0f;
    float out_recover;
    unsigned int i;

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

static int test_pid_functional_step_response(void)
{
    pid_ctx_t functional;
    float expected[4] = {25.0f, 30.0f, 35.0f, 15.0f};
    float measured[4] = {40.0f, 40.0f, 40.0f, 50.0f};
    unsigned int step;

    /* Functional example: run PID once per second and verify each output. */
    pid_init(&functional, 2.0f, 0.5f, 0.0f, 1.0f, 0.0f, 100.0f);
    for (step = 0U; step < 4U; ++step)
    {
        float out = pid_step(&functional, 50.0f, measured[step]);
        TEST_ASSERT_NEAR_FLOAT(out, expected[step], 0.001f);
    }

    return 0;
}

static int test_pid_closed_loop_thermal_model(void)
{
    pid_ctx_t closed_loop;
    float temp;
    float ambient;
    float setpoint;
    float prev_temp;
    float err_start;
    float err_end;
    float out;
    unsigned int step;

    /* Closed-loop functional check: PID output drives a simple thermal plant. */
    pid_init(&closed_loop, 6.0f, 0.25f, 0.0f, 1.0f, 0.0f, 100.0f);
    ambient = 25.0f;
    temp = ambient;
    setpoint = 55.0f;
    err_start = setpoint - temp;
    if (err_start < 0.0f)
    {
        err_start = -err_start;
    }

    for (step = 0U; step < 180U; ++step)
    {
        float heater_ratio;

        out = pid_step(&closed_loop, setpoint, temp);
        heater_ratio = out / 100.0f;
        temp += (0.18f * heater_ratio) - (0.03f * (temp - ambient));
    }

    err_end = setpoint - temp;
    if (err_end < 0.0f)
    {
        err_end = -err_end;
    }

    TEST_ASSERT_TRUE(temp > ambient + 5.0f);
    TEST_ASSERT_TRUE(temp < 80.0f);
    TEST_ASSERT_TRUE(err_end < err_start);

    prev_temp = temp;
    setpoint = 35.0f;
    for (step = 0U; step < 120U; ++step)
    {
        float heater_ratio;

        out = pid_step(&closed_loop, setpoint, temp);
        heater_ratio = out / 100.0f;
        temp += (0.18f * heater_ratio) - (0.03f * (temp - ambient));
    }

    TEST_ASSERT_TRUE(temp < prev_temp);

    return 0;
}

typedef struct
{
    const char *name;
    int (*fn)(void);
} pid_test_case_t;

int test_pid_run(void)
{
    unsigned int i;
    unsigned int fail_count = 0U;
    const pid_test_case_t cases[] = {
        {"basic_range", test_pid_basic_range},
        {"anti_windup_recovery", test_pid_anti_windup_recovery},
        {"functional_step_response", test_pid_functional_step_response},
        {"closed_loop_thermal_model", test_pid_closed_loop_thermal_model}
    };

    printf("[pid] running %u subcases\n", (unsigned int)(sizeof(cases) / sizeof(cases[0])));

    for (i = 0U; i < (sizeof(cases) / sizeof(cases[0])); ++i)
    {
        int rc = cases[i].fn();
        if (rc == 0)
        {
            printf("  [PASS] %s\n", cases[i].name);
        }
        else
        {
            printf("  [FAIL] %s (code=%d)\n", cases[i].name, rc);
            fail_count++;
        }
    }

    if (fail_count == 0U)
    {
        printf("[pid] summary: all subcases passed\n");
        return 0;
    }

    printf("[pid] summary: %u subcase(s) failed\n", fail_count);
    return 1;
}
