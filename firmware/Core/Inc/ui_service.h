#ifndef UI_SERVICE_H
#define UI_SERVICE_H

#include "app_state.h"
#include "param_store.h"
#include "temp_manager.h"

void ui_service_init(void);
void ui_service_tick_100ms(app_params_t *params);
void ui_service_tick_200ms(app_mode_t mode, const temp_snapshot_t *temp, const app_params_t *params, float pid_out, int heater_on, int alarm_on);

#endif
