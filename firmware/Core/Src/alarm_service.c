#include "alarm_service.h"

static bool g_alarm_active = false;

void alarm_service_init(void)
{
    g_alarm_active = false;
}

void alarm_service_update(float t1, float t2, float t3, float threshold_c, bool sensor_fault)
{
    g_alarm_active = sensor_fault || (t1 > threshold_c) || (t2 > threshold_c) || (t3 > threshold_c);
}

bool alarm_service_is_active(void)
{
    return g_alarm_active;
}
