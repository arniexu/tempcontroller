#include "param_store.h"

#include "app_config.h"

static app_params_t g_params;

void param_store_init(void)
{
    param_store_load_defaults(&g_params);
}

void param_store_load_defaults(app_params_t *params)
{
    if (params == 0)
    {
        return;
    }

    params->set_temp_c = APP_TEMP_DEFAULT_SETPOINT_C;
    params->alarm_threshold_c = APP_TEMP_ALARM_THRESHOLD_C;
    params->kp = 8.0f;
    params->ki = 0.3f;
    params->kd = 15.0f;
}

void param_store_load(app_params_t *params)
{
    if (params == 0)
    {
        return;
    }

    *params = g_params;
}

void param_store_save(const app_params_t *params)
{
    if (params == 0)
    {
        return;
    }

    g_params = *params;
}

const app_params_t *param_store_get(void)
{
    return &g_params;
}

app_params_t *param_store_get_mutable(void)
{
    return &g_params;
}
