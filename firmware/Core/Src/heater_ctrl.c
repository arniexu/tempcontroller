#include "heater_ctrl.h"

#include "app_config.h"
#include "hw_platform_port.h"

static uint32_t g_window_ms = 10000U;
static uint32_t g_elapsed_ms = 0U;
static float g_output_percent = 0.0f;
static bool g_heater_on = false;
static uint32_t g_last_toggle_ms = 0U;
static uint32_t g_global_ms = 0U;

void heater_ctrl_init(uint32_t window_ms)
{
    hw_relay_init();

    g_window_ms = (window_ms == 0U) ? 10000U : window_ms;
    g_elapsed_ms = 0U;
    g_output_percent = 0.0f;
    g_heater_on = false;
    g_last_toggle_ms = 0U;
    g_global_ms = 0U;
}

void heater_ctrl_set_output_percent(float percent)
{
    if (percent < 0.0f)
    {
        percent = 0.0f;
    }
    if (percent > 100.0f)
    {
        percent = 100.0f;
    }
    g_output_percent = percent;
}

void heater_ctrl_update_1ms(void)
{
    uint32_t on_ms;
    bool desired_on;
    uint32_t min_guard;

    g_global_ms++;
    g_elapsed_ms++;
    if (g_elapsed_ms >= g_window_ms)
    {
        g_elapsed_ms = 0U;
    }

    on_ms = (uint32_t)((g_output_percent / 100.0f) * (float)g_window_ms);
    desired_on = (g_elapsed_ms < on_ms);

    if (desired_on == g_heater_on)
    {
        return;
    }

    min_guard = g_heater_on ? APP_HEATER_MIN_ON_MS : APP_HEATER_MIN_OFF_MS;
    if ((g_global_ms - g_last_toggle_ms) < min_guard)
    {
        return;
    }

    g_heater_on = desired_on;
    g_last_toggle_ms = g_global_ms;
    hw_relay_set(g_heater_on);
}

bool heater_ctrl_get_state(void)
{
    return g_heater_on;
}

void heater_ctrl_force_off(void)
{
    g_output_percent = 0.0f;
    g_heater_on = false;
    g_last_toggle_ms = g_global_ms;
    hw_relay_set(false);
}
