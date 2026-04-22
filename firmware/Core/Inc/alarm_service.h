#ifndef ALARM_SERVICE_H
#define ALARM_SERVICE_H

#include <stdbool.h>

void alarm_service_init(void);
void alarm_service_update(float t1, float t2, float t3, float threshold_c, bool sensor_fault);
bool alarm_service_is_active(void);

#endif
