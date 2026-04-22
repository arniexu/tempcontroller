#ifndef SCHEDULE_SERVICE_H
#define SCHEDULE_SERVICE_H

#include <stdbool.h>
#include <stdint.h>

typedef struct
{
	bool enabled;
	uint16_t start_min_of_day;
	uint16_t end_min_of_day;
} schedule_config_t;

void schedule_service_init(void);
void schedule_service_update(void);
bool schedule_service_heating_allowed(void);
void schedule_service_set_config(const schedule_config_t *cfg);
void schedule_service_get_config(schedule_config_t *cfg);

#endif
