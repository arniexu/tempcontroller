#include "schedule_service.h"

#include "hw_platform_port.h"

static bool g_heating_allowed = true;
static schedule_config_t g_cfg;

static bool schedule_is_active(uint16_t now_min, const schedule_config_t *cfg)
{
    uint16_t start = (uint16_t)(cfg->start_min_of_day % 1440U);
    uint16_t end = (uint16_t)(cfg->end_min_of_day % 1440U);

    if (start == end)
    {
        return true;
    }

    if (start < end)
    {
        return (now_min >= start) && (now_min < end);
    }

    return (now_min >= start) || (now_min < end);
}

void schedule_service_init(void)
{
    hw_rtc_init();

    g_cfg.enabled = false;
    g_cfg.start_min_of_day = 0U;
    g_cfg.end_min_of_day = 0U;
    g_heating_allowed = true;
}

void schedule_service_update(void)
{
    uint16_t now_min;

    if (!g_cfg.enabled)
    {
        g_heating_allowed = true;
        return;
    }

    if (!hw_rtc_get_minutes_of_day(&now_min))
    {
        g_heating_allowed = false;
        return;
    }

    g_heating_allowed = schedule_is_active(now_min, &g_cfg);
}

bool schedule_service_heating_allowed(void)
{
    return g_heating_allowed;
}

void schedule_service_set_config(const schedule_config_t *cfg)
{
    if (cfg == 0)
    {
        return;
    }

    g_cfg = *cfg;
    g_cfg.start_min_of_day = (uint16_t)(g_cfg.start_min_of_day % 1440U);
    g_cfg.end_min_of_day = (uint16_t)(g_cfg.end_min_of_day % 1440U);
}

void schedule_service_get_config(schedule_config_t *cfg)
{
    if (cfg == 0)
    {
        return;
    }

    *cfg = g_cfg;
}
