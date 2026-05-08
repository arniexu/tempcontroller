#include "tune_service.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

typedef struct
{
    tune_scene_info_t info;
    float kp;
    float ki;
    float kd;
} scene_preset_t;

static const scene_preset_t g_scene_defaults[] = {
    {{"FAST", "Cold start", "Large pot"}, 10.0f, 0.60f, 20.0f},
    {{"DAILY", "Daily keep", "Everyday hold"}, 8.0f, 0.30f, 15.0f},
    {{"GENTLE", "Low overshoot", "Small cup"}, 5.0f, 0.15f, 10.0f}
};

static const tune_flow_state_t g_step_to_flow[] = {
    TUNE_FLOW_PREHEAT,
    TUNE_FLOW_APPROACH,
    TUNE_FLOW_STABLE,
    TUNE_FLOW_DISTURB,
    TUNE_FLOW_ACCEPT
};

static tune_runtime_t g_runtime;
static tune_pid_profile_t g_profile;
static uint8_t g_scene_id = 1U;
static float g_prev_t_ctrl = 0.0f;
static float g_prev_error = 0.0f;
static int g_prev_valid = 0;

static float clampf(float value, float min_value, float max_value)
{
    if (value < min_value)
    {
        return min_value;
    }
    if (value > max_value)
    {
        return max_value;
    }
    return value;
}

static int clampi(int value, int min_value, int max_value)
{
    if (value < min_value)
    {
        return min_value;
    }
    if (value > max_value)
    {
        return max_value;
    }
    return value;
}

static void update_action_and_advice(void)
{
    g_runtime.action = TUNE_ACTION_NONE;

    if (g_runtime.issue == TUNE_ISSUE_SENSOR_FAULT)
    {
        g_runtime.action = TUNE_ACTION_CHECK_SENSOR;
        (void)snprintf(g_runtime.advice_text, sizeof(g_runtime.advice_text), "Check sensor wiring");
        return;
    }

    if (g_runtime.issue == TUNE_ISSUE_OUTPUT_SATURATED)
    {
        g_runtime.action = TUNE_ACTION_CHECK_HEATER;
        (void)snprintf(g_runtime.advice_text, sizeof(g_runtime.advice_text), "Check heater output");
        return;
    }

    if ((g_runtime.flow == TUNE_FLOW_ACCEPT) &&
        (g_runtime.issue == TUNE_ISSUE_OK))
    {
        g_runtime.action = TUNE_ACTION_ENTER_ACCEPT;
        (void)snprintf(g_runtime.advice_text, sizeof(g_runtime.advice_text), "Ready to save scene");
        return;
    }

    switch (g_runtime.issue)
    {
    case TUNE_ISSUE_SLOW:
        g_runtime.action = TUNE_ACTION_KP_UP;
        (void)snprintf(g_runtime.advice_text, sizeof(g_runtime.advice_text), "Suggest KP +0.2");
        break;
    case TUNE_ISSUE_OVERSHOOT:
        g_runtime.action = TUNE_ACTION_KP_DOWN;
        (void)snprintf(g_runtime.advice_text, sizeof(g_runtime.advice_text), "Suggest KP -0.2");
        break;
    case TUNE_ISSUE_WAVE:
        g_runtime.action = TUNE_ACTION_KI_DOWN;
        (void)snprintf(g_runtime.advice_text, sizeof(g_runtime.advice_text), "Suggest KI -0.02");
        break;
    case TUNE_ISSUE_STEADY_ERROR:
        g_runtime.action = TUNE_ACTION_KI_UP;
        (void)snprintf(g_runtime.advice_text, sizeof(g_runtime.advice_text), "Suggest KI +0.02");
        break;
    case TUNE_ISSUE_OK:
    default:
        g_runtime.action = TUNE_ACTION_NONE;
        (void)snprintf(g_runtime.advice_text, sizeof(g_runtime.advice_text), "Keep observing");
        break;
    }
}

static uint8_t scene_clamped(uint8_t scene_id)
{
    uint8_t count = (uint8_t)(sizeof(g_scene_defaults) / sizeof(g_scene_defaults[0]));
    if (count == 0U)
    {
        return 0U;
    }
    return (uint8_t)(scene_id % count);
}

void tune_service_init(void)
{
    memset(&g_runtime, 0, sizeof(g_runtime));
    memset(&g_profile, 0, sizeof(g_profile));

    g_profile.kp_min = 0.5f;
    g_profile.kp_max = 20.0f;
    g_profile.ki_min = 0.0f;
    g_profile.ki_max = 10.0f;
    g_profile.kd_min = 0.0f;
    g_profile.kd_max = 30.0f;
    g_profile.tmin_c = 20.0f;
    g_profile.tmax_c = 95.0f;

    g_scene_id = 1U;
    g_runtime.scene_id = g_scene_id;
    g_runtime.step_index = 0U;
    g_runtime.flow = TUNE_FLOW_PREHEAT;
    g_runtime.issue = TUNE_ISSUE_OK;
    g_runtime.action = TUNE_ACTION_NONE;
    (void)snprintf(g_runtime.advice_text, sizeof(g_runtime.advice_text), "Keep observing");
    g_prev_t_ctrl = 0.0f;
    g_prev_error = 0.0f;
    g_prev_valid = 0;
}

void tune_service_set_scene(uint8_t scene_id)
{
    g_scene_id = scene_clamped(scene_id);
    g_runtime.scene_id = g_scene_id;
}

uint8_t tune_service_get_scene(void)
{
    return g_scene_id;
}

uint8_t tune_service_scene_count(void)
{
    return (uint8_t)(sizeof(g_scene_defaults) / sizeof(g_scene_defaults[0]));
}

const tune_scene_info_t *tune_service_get_scene_info(uint8_t scene_id)
{
    if (tune_service_scene_count() == 0U)
    {
        return 0;
    }

    return &g_scene_defaults[scene_clamped(scene_id)].info;
}

void tune_service_clamp_params(app_params_t *params)
{
    if (params == 0)
    {
        return;
    }

    params->kp = clampf(params->kp, g_profile.kp_min, g_profile.kp_max);
    params->ki = clampf(params->ki, g_profile.ki_min, g_profile.ki_max);
    params->kd = clampf(params->kd, g_profile.kd_min, g_profile.kd_max);
    params->set_temp_c = clampf(params->set_temp_c, g_profile.tmin_c, g_profile.tmax_c);
}

void tune_service_load_scene_defaults(app_params_t *params)
{
    const scene_preset_t *preset;

    if (params == 0)
    {
        return;
    }

    g_scene_id = scene_clamped(g_scene_id);
    preset = &g_scene_defaults[g_scene_id];

    params->kp = preset->kp;
    params->ki = preset->ki;
    params->kd = preset->kd;
    tune_service_clamp_params(params);
}

void tune_service_step_change(int dir)
{
    int step = (int)g_runtime.step_index;

    if (dir > 0)
    {
        step++;
    }
    else if (dir < 0)
    {
        step--;
    }

    step = clampi(step, 0, 4);
    g_runtime.step_index = (uint8_t)step;
    g_runtime.flow = g_step_to_flow[g_runtime.step_index];
    update_action_and_advice();
}

void tune_service_issue_change(int dir)
{
    int issue = (int)g_runtime.issue;

    if (dir > 0)
    {
        issue++;
    }
    else if (dir < 0)
    {
        issue--;
    }

    issue = clampi(issue, (int)TUNE_ISSUE_OK, (int)TUNE_ISSUE_COUNT - 1);
    g_runtime.issue = (tune_issue_t)issue;
    update_action_and_advice();
}

void tune_service_set_issue(tune_issue_t issue)
{
    if (issue >= TUNE_ISSUE_COUNT)
    {
        return;
    }
    g_runtime.issue = issue;
    update_action_and_advice();
}

void tune_service_update_observation(const temp_snapshot_t *temp,
                                     const app_params_t *params,
                                     float pid_out,
                                     int heater_on)
{
    float error;
    float delta_t;
    tune_issue_t inferred = TUNE_ISSUE_OK;

    if ((temp == 0) || (params == 0))
    {
        return;
    }

    error = params->set_temp_c - temp->t_ctrl;
    delta_t = 0.0f;
    if (g_prev_valid)
    {
        delta_t = temp->t_ctrl - g_prev_t_ctrl;
        g_runtime.rise_rate = delta_t / 0.2f;
    }
    else
    {
        g_runtime.rise_rate = 0.0f;
    }

    if (temp->t_ctrl > params->set_temp_c)
    {
        float overshoot = temp->t_ctrl - params->set_temp_c;
        if (overshoot > g_runtime.overshoot)
        {
            g_runtime.overshoot = overshoot;
        }
    }

    g_runtime.steady_error = (float)fabs((double)error);

    if (temp->sensor_fault)
    {
        inferred = TUNE_ISSUE_SENSOR_FAULT;
    }
    else if ((pid_out >= 95.0f) && (error > 2.0f))
    {
        inferred = TUNE_ISSUE_OUTPUT_SATURATED;
    }
    else if (g_runtime.overshoot > 1.5f)
    {
        inferred = TUNE_ISSUE_OVERSHOOT;
    }
    else if ((g_prev_valid) && (error > 1.0f) && (heater_on != 0) && (g_runtime.rise_rate < 0.02f))
    {
        inferred = TUNE_ISSUE_SLOW;
    }
    else if ((g_prev_valid) && ((error * g_prev_error) < 0.0f) && ((float)fabs((double)error) > 0.8f))
    {
        inferred = TUNE_ISSUE_WAVE;
    }
    else if ((float)fabs((double)error) > 0.6f)
    {
        inferred = TUNE_ISSUE_STEADY_ERROR;
    }

    g_runtime.issue = inferred;
    g_runtime.accept_ready = ((g_runtime.flow == TUNE_FLOW_ACCEPT) &&
                              (g_runtime.overshoot < 1.0f) &&
                              (g_runtime.steady_error < 0.5f)) ? 1U : 0U;
    if (g_runtime.accept_ready)
    {
        g_runtime.action = TUNE_ACTION_ENTER_ACCEPT;
        (void)snprintf(g_runtime.advice_text, sizeof(g_runtime.advice_text), "Pass, save advised");
    }
    else
    {
        update_action_and_advice();
    }

    g_prev_t_ctrl = temp->t_ctrl;
    g_prev_error = error;
    g_prev_valid = 1;
}

void tune_service_apply_suggestion(app_params_t *params)
{
    if (params == 0)
    {
        return;
    }

    switch (g_runtime.action)
    {
    case TUNE_ACTION_KP_UP:
        params->kp += 0.2f;
        break;
    case TUNE_ACTION_KP_DOWN:
        params->kp -= 0.2f;
        break;
    case TUNE_ACTION_KI_UP:
        params->ki += 0.02f;
        break;
    case TUNE_ACTION_KI_DOWN:
        params->ki -= 0.02f;
        break;
    case TUNE_ACTION_KD_UP:
        params->kd += 0.02f;
        break;
    default:
        break;
    }

    tune_service_clamp_params(params);
}

const tune_runtime_t *tune_service_get_runtime(void)
{
    return &g_runtime;
}

const tune_pid_profile_t *tune_service_get_profile(void)
{
    return &g_profile;
}

void tune_service_set_profile(const tune_pid_profile_t *profile)
{
    if (profile == 0)
    {
        return;
    }
    g_profile = *profile;
    if (g_profile.kp_max < g_profile.kp_min)
    {
        g_profile.kp_max = g_profile.kp_min;
    }
    if (g_profile.ki_max < g_profile.ki_min)
    {
        g_profile.ki_max = g_profile.ki_min;
    }
    if (g_profile.kd_max < g_profile.kd_min)
    {
        g_profile.kd_max = g_profile.kd_min;
    }
    if (g_profile.tmax_c < g_profile.tmin_c)
    {
        g_profile.tmax_c = g_profile.tmin_c;
    }
}

const char *tune_service_step_text(uint8_t step_index)
{
    switch (step_index)
    {
    case 0U:
        return "Preheat";
    case 1U:
        return "Approach";
    case 2U:
        return "Steady";
    case 3U:
        return "Recover";
    case 4U:
        return "Accept";
    default:
        return "Unknown";
    }
}

const char *tune_service_issue_text(tune_issue_t issue)
{
    switch (issue)
    {
    case TUNE_ISSUE_OK:
        return "OK";
    case TUNE_ISSUE_SLOW:
        return "Slow";
    case TUNE_ISSUE_OVERSHOOT:
        return "Overshoot";
    case TUNE_ISSUE_WAVE:
        return "Wave";
    case TUNE_ISSUE_STEADY_ERROR:
        return "SteadyErr";
    case TUNE_ISSUE_OUTPUT_SATURATED:
        return "Saturated";
    case TUNE_ISSUE_SENSOR_FAULT:
        return "SensorFault";
    default:
        return "Unknown";
    }
}

const char *tune_service_action_text(tune_action_t action)
{
    switch (action)
    {
    case TUNE_ACTION_KP_UP:
        return "KP +";
    case TUNE_ACTION_KP_DOWN:
        return "KP -";
    case TUNE_ACTION_KI_UP:
        return "KI +";
    case TUNE_ACTION_KI_DOWN:
        return "KI -";
    case TUNE_ACTION_KD_UP:
        return "KD +";
    case TUNE_ACTION_CHECK_HEATER:
        return "CheckHeat";
    case TUNE_ACTION_CHECK_SENSOR:
        return "CheckSens";
    case TUNE_ACTION_ENTER_ACCEPT:
        return "Accept";
    default:
        return "Hold";
    }
}
