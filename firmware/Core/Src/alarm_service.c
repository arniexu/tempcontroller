#include "alarm_service.h"

#include <math.h>

#define ALARM_TEMP_MIN_C            (-55.0f)
#define ALARM_TEMP_MAX_C            (125.0f)
#define ALARM_RELEASE_HYSTERESIS_C  (0.5f)

static bool g_alarm_active = false;
static bool g_overtemp_latched = false;

static bool is_temp_invalid(float t)
{
    return isnan(t) || (t < ALARM_TEMP_MIN_C) || (t > ALARM_TEMP_MAX_C);
}

static bool any_temp_over_threshold(float t1, float t2, float t3, float threshold_c)
{
    return (t1 > threshold_c) || (t2 > threshold_c) || (t3 > threshold_c);
}

static bool all_temp_below_release(float t1, float t2, float t3, float threshold_c)
{
    float release_c = threshold_c - ALARM_RELEASE_HYSTERESIS_C;
    return (t1 < release_c) && (t2 < release_c) && (t3 < release_c);
}

void alarm_service_init(void)
{
    g_alarm_active = false;
    g_overtemp_latched = false;
}

void alarm_service_update(float t1, float t2, float t3, float threshold_c, bool sensor_fault)
{
    if (sensor_fault || isnan(threshold_c) || is_temp_invalid(t1) || is_temp_invalid(t2) || is_temp_invalid(t3))
    {
        g_alarm_active = true;
        return;
    }

    if (!g_overtemp_latched)
    {
        g_overtemp_latched = any_temp_over_threshold(t1, t2, t3, threshold_c);
    }
    else if (all_temp_below_release(t1, t2, t3, threshold_c))
    {
        g_overtemp_latched = false;
    }

    g_alarm_active = g_overtemp_latched;
}

bool alarm_service_is_active(void)
{
    return g_alarm_active;
}
