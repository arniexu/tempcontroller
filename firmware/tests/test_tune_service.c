#include "tune_service.h"

#include <string.h>

#include "test_framework.h"

static void init_params(app_params_t *params)
{
    memset(params, 0, sizeof(*params));
    params->set_temp_c = 60.0f;
    params->alarm_threshold_c = 75.0f;
    params->log_period_s = 5U;
}

static void init_snapshot(temp_snapshot_t *temp, float t_ctrl)
{
    memset(temp, 0, sizeof(*temp));
    temp->t1 = t_ctrl;
    temp->t2 = t_ctrl;
    temp->t3 = t_ctrl;
    temp->t_ctrl = t_ctrl;
    temp->valid_mask = 0x07U;
}

static void feed_sample(app_params_t *params, temp_snapshot_t *temp, float t_ctrl, float pid_out, int heater_on)
{
    temp->t1 = t_ctrl;
    temp->t2 = t_ctrl;
    temp->t3 = t_ctrl;
    temp->t_ctrl = t_ctrl;
    tune_service_update_observation(temp, params, pid_out, heater_on);
}

static void move_to_accept(void)
{
    while (tune_service_get_runtime()->step_index < 4U)
    {
        tune_service_step_change(+1);
    }
}

int test_tune_service_run(void)
{
    app_params_t params;
    temp_snapshot_t temp;
    tune_pid_profile_t profile;
    const tune_runtime_t *rt;
    const tune_scene_info_t *scene;
    float current;
    unsigned int i;

    TEST_ASSERT_EQ_INT((int)tune_service_scene_count(), 3);
    scene = tune_service_get_scene_info(0U);
    TEST_ASSERT_TRUE(scene != 0);
    TEST_ASSERT_TRUE(strcmp(scene->code, "FAST") == 0);
    TEST_ASSERT_TRUE(strcmp(scene->usage, "Large pot") == 0);

    init_params(&params);
    init_snapshot(&temp, 25.0f);
    tune_service_init();
    tune_service_set_scene(0U);
    tune_service_load_scene_defaults(&params);

    current = 25.0f;
    for (i = 0U; i < 20U; ++i)
    {
        current += 0.003f;
        feed_sample(&params, &temp, current, 72.0f, 1);
    }
    rt = tune_service_get_runtime();
    TEST_ASSERT_EQ_INT((int)rt->issue, (int)TUNE_ISSUE_SLOW);
    TEST_ASSERT_EQ_INT((int)rt->action, (int)TUNE_ACTION_KP_UP);
    tune_service_apply_suggestion(&params);
    TEST_ASSERT_NEAR_FLOAT(params.kp, 10.2f, 0.01f);

    for (i = 0U; i < 120U; ++i)
    {
        if (current < 59.7f)
        {
            current += 0.30f;
        }
        else
        {
            current = 60.1f;
        }
        feed_sample(&params, &temp, current, 68.0f, 1);
    }
    move_to_accept();
    for (i = 0U; i < 10U; ++i)
    {
        feed_sample(&params, &temp, 60.1f, 24.0f, 1);
    }
    rt = tune_service_get_runtime();
    TEST_ASSERT_EQ_INT(rt->accept_ready, 1);
    TEST_ASSERT_EQ_INT((int)rt->action, (int)TUNE_ACTION_ENTER_ACCEPT);

    init_params(&params);
    init_snapshot(&temp, 42.0f);
    tune_service_init();
    tune_service_set_scene(1U);
    tune_service_load_scene_defaults(&params);
    scene = tune_service_get_scene_info(1U);
    TEST_ASSERT_TRUE(strcmp(scene->code, "DAILY") == 0);

    current = 42.0f;
    for (i = 0U; i < 90U; ++i)
    {
        if (current < 59.8f)
        {
            current += 0.20f;
        }
        else
        {
            current = 60.0f;
        }
        feed_sample(&params, &temp, current, 55.0f, 1);
    }
    move_to_accept();
    for (i = 0U; i < 10U; ++i)
    {
        feed_sample(&params, &temp, 60.0f, 20.0f, 0);
    }
    rt = tune_service_get_runtime();
    TEST_ASSERT_EQ_INT(rt->accept_ready, 1);
    TEST_ASSERT_EQ_INT((int)rt->issue, (int)TUNE_ISSUE_OK);

    init_params(&params);
    init_snapshot(&temp, 50.0f);
    tune_service_init();
    tune_service_set_scene(2U);
    tune_service_load_scene_defaults(&params);
    scene = tune_service_get_scene_info(2U);
    TEST_ASSERT_TRUE(strcmp(scene->code, "GENTLE") == 0);

    current = 50.0f;
    for (i = 0U; i < 24U; ++i)
    {
        current += 0.50f;
        feed_sample(&params, &temp, current, 48.0f, 1);
    }
    feed_sample(&params, &temp, 61.8f, 30.0f, 0);
    rt = tune_service_get_runtime();
    TEST_ASSERT_EQ_INT((int)rt->issue, (int)TUNE_ISSUE_OVERSHOOT);
    TEST_ASSERT_EQ_INT((int)rt->action, (int)TUNE_ACTION_KP_DOWN);
    tune_service_apply_suggestion(&params);
    TEST_ASSERT_NEAR_FLOAT(params.kp, 4.8f, 0.01f);

    init_snapshot(&temp, 50.0f);
    tune_service_init();
    tune_service_set_scene(2U);
    current = 50.0f;
    for (i = 0U; i < 80U; ++i)
    {
        if (current < 59.9f)
        {
            current += 0.14f;
        }
        else
        {
            current = 60.0f;
        }
        feed_sample(&params, &temp, current, 36.0f, 1);
    }
    move_to_accept();
    for (i = 0U; i < 10U; ++i)
    {
        feed_sample(&params, &temp, 60.0f, 18.0f, 0);
    }
    rt = tune_service_get_runtime();
    TEST_ASSERT_EQ_INT(rt->accept_ready, 1);
    TEST_ASSERT_EQ_INT((int)rt->action, (int)TUNE_ACTION_ENTER_ACCEPT);

    init_params(&params);
    init_snapshot(&temp, 60.0f);
    tune_service_init();
    params.ki = 0.30f;
    feed_sample(&params, &temp, 59.0f, 40.0f, 1);
    feed_sample(&params, &temp, 61.2f, 40.0f, 1);
    rt = tune_service_get_runtime();
    TEST_ASSERT_EQ_INT((int)rt->issue, (int)TUNE_ISSUE_WAVE);
    TEST_ASSERT_EQ_INT((int)rt->action, (int)TUNE_ACTION_KI_DOWN);
    tune_service_apply_suggestion(&params);
    TEST_ASSERT_NEAR_FLOAT(params.ki, 0.28f, 0.01f);

    init_params(&params);
    init_snapshot(&temp, 58.8f);
    tune_service_init();
    params.ki = 0.30f;
    feed_sample(&params, &temp, 58.8f, 32.0f, 1);
    feed_sample(&params, &temp, 59.0f, 28.0f, 1);
    rt = tune_service_get_runtime();
    TEST_ASSERT_EQ_INT((int)rt->issue, (int)TUNE_ISSUE_STEADY_ERROR);
    TEST_ASSERT_EQ_INT((int)rt->action, (int)TUNE_ACTION_KI_UP);
    tune_service_apply_suggestion(&params);
    TEST_ASSERT_NEAR_FLOAT(params.ki, 0.32f, 0.01f);

    init_params(&params);
    init_snapshot(&temp, 54.0f);
    tune_service_init();
    feed_sample(&params, &temp, 54.0f, 97.0f, 1);
    rt = tune_service_get_runtime();
    TEST_ASSERT_EQ_INT((int)rt->issue, (int)TUNE_ISSUE_OUTPUT_SATURATED);
    TEST_ASSERT_EQ_INT((int)rt->action, (int)TUNE_ACTION_CHECK_HEATER);
    TEST_ASSERT_TRUE(strcmp(rt->advice_text, "Check heater output") == 0);

    init_params(&params);
    init_snapshot(&temp, 54.0f);
    tune_service_init();
    temp.sensor_fault = true;
    feed_sample(&params, &temp, 54.0f, 10.0f, 0);
    rt = tune_service_get_runtime();
    TEST_ASSERT_EQ_INT((int)rt->issue, (int)TUNE_ISSUE_SENSOR_FAULT);
    TEST_ASSERT_EQ_INT((int)rt->action, (int)TUNE_ACTION_CHECK_SENSOR);
    TEST_ASSERT_TRUE(strcmp(rt->advice_text, "Check sensor wiring") == 0);

    tune_service_init();
    tune_service_step_change(-1);
    TEST_ASSERT_EQ_INT((int)tune_service_get_runtime()->step_index, 0);
    for (i = 0U; i < 8U; ++i)
    {
        tune_service_step_change(+1);
    }
    TEST_ASSERT_EQ_INT((int)tune_service_get_runtime()->step_index, 4);

    tune_service_init();
    tune_service_issue_change(-1);
    TEST_ASSERT_EQ_INT((int)tune_service_get_runtime()->issue, (int)TUNE_ISSUE_OK);
    for (i = 0U; i < 16U; ++i)
    {
        tune_service_issue_change(+1);
    }
    TEST_ASSERT_EQ_INT((int)tune_service_get_runtime()->issue, (int)TUNE_ISSUE_SENSOR_FAULT);

    tune_service_init();
    params.kp = -2.0f;
    params.ki = -1.0f;
    params.kd = 99.0f;
    params.set_temp_c = 200.0f;
    tune_service_clamp_params(&params);
    TEST_ASSERT_NEAR_FLOAT(params.kp, 0.5f, 0.01f);
    TEST_ASSERT_NEAR_FLOAT(params.ki, 0.0f, 0.01f);
    TEST_ASSERT_NEAR_FLOAT(params.kd, 30.0f, 0.01f);
    TEST_ASSERT_NEAR_FLOAT(params.set_temp_c, 95.0f, 0.01f);

    profile = *tune_service_get_profile();
    profile.kp_min = 3.0f;
    profile.kp_max = 1.0f;
    profile.ki_min = 2.0f;
    profile.ki_max = 1.0f;
    profile.kd_min = 4.0f;
    profile.kd_max = 1.0f;
    profile.tmin_c = 70.0f;
    profile.tmax_c = 30.0f;
    tune_service_set_profile(&profile);
    profile = *tune_service_get_profile();
    TEST_ASSERT_NEAR_FLOAT(profile.kp_max, 3.0f, 0.01f);
    TEST_ASSERT_NEAR_FLOAT(profile.ki_max, 2.0f, 0.01f);
    TEST_ASSERT_NEAR_FLOAT(profile.kd_max, 4.0f, 0.01f);
    TEST_ASSERT_NEAR_FLOAT(profile.tmax_c, 70.0f, 0.01f);

    /* Guided tuning flow end-to-end: preheat -> approach -> stable -> disturb -> accept */
    init_params(&params);
    init_snapshot(&temp, 45.0f);
    tune_service_init();
    tune_service_set_scene(1U);
    tune_service_load_scene_defaults(&params);
    TEST_ASSERT_NEAR_FLOAT(params.kp, 8.0f, 0.01f);
    TEST_ASSERT_NEAR_FLOAT(params.ki, 0.30f, 0.01f);
    TEST_ASSERT_EQ_INT((int)tune_service_get_runtime()->step_index, 0);

    tune_service_set_issue(TUNE_ISSUE_SLOW);
    TEST_ASSERT_EQ_INT((int)tune_service_get_runtime()->action, (int)TUNE_ACTION_KP_UP);
    tune_service_apply_suggestion(&params);
    TEST_ASSERT_NEAR_FLOAT(params.kp, 8.2f, 0.01f);

    tune_service_step_change(+1);
    TEST_ASSERT_EQ_INT((int)tune_service_get_runtime()->flow, (int)TUNE_FLOW_APPROACH);
    tune_service_set_issue(TUNE_ISSUE_OVERSHOOT);
    TEST_ASSERT_EQ_INT((int)tune_service_get_runtime()->action, (int)TUNE_ACTION_KP_DOWN);
    tune_service_apply_suggestion(&params);
    TEST_ASSERT_NEAR_FLOAT(params.kp, 8.0f, 0.01f);

    tune_service_step_change(+1);
    TEST_ASSERT_EQ_INT((int)tune_service_get_runtime()->flow, (int)TUNE_FLOW_STABLE);
    tune_service_set_issue(TUNE_ISSUE_STEADY_ERROR);
    TEST_ASSERT_EQ_INT((int)tune_service_get_runtime()->action, (int)TUNE_ACTION_KI_UP);
    tune_service_apply_suggestion(&params);
    TEST_ASSERT_NEAR_FLOAT(params.ki, 0.32f, 0.01f);

    tune_service_step_change(+1);
    TEST_ASSERT_EQ_INT((int)tune_service_get_runtime()->flow, (int)TUNE_FLOW_DISTURB);
    tune_service_set_issue(TUNE_ISSUE_WAVE);
    TEST_ASSERT_EQ_INT((int)tune_service_get_runtime()->action, (int)TUNE_ACTION_KI_DOWN);
    tune_service_apply_suggestion(&params);
    TEST_ASSERT_NEAR_FLOAT(params.ki, 0.30f, 0.01f);

    tune_service_step_change(+1);
    TEST_ASSERT_EQ_INT((int)tune_service_get_runtime()->flow, (int)TUNE_FLOW_ACCEPT);
    for (i = 0U; i < 10U; ++i)
    {
        feed_sample(&params, &temp, 60.0f, 18.0f, 0);
    }
    rt = tune_service_get_runtime();
    TEST_ASSERT_EQ_INT(rt->accept_ready, 1);
    TEST_ASSERT_EQ_INT((int)rt->action, (int)TUNE_ACTION_ENTER_ACCEPT);

    return 0;
}