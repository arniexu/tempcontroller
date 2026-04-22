#ifndef LOG_SERVICE_H
#define LOG_SERVICE_H

#include <stdbool.h>

#include "temp_manager.h"

typedef struct
{
	unsigned int index;
	float t1;
	float t2;
	float t3;
	float t_avg;
	float set_temp;
	float pid_out;
	int heater_on;
	int alarm_on;
} log_record_t;

void log_service_init(void);
void log_service_push(const temp_snapshot_t *temp, float set_temp, float pid_out, int heater_on, int alarm_on);
unsigned int log_service_count(void);
bool log_service_get(unsigned int offset_oldest, log_record_t *out);
void log_service_clear(void);

#endif
