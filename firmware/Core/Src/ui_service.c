#include "ui_service.h"

void ui_service_init(void)
{
}

void ui_service_tick_100ms(app_params_t *params)
{
    (void)params;
}

void ui_service_tick_200ms(app_mode_t mode, const temp_snapshot_t *temp, const app_params_t *params, float pid_out, int heater_on, int alarm_on)
{
    (void)mode;
    (void)temp;
    (void)params;
    (void)pid_out;
    (void)heater_on;
    (void)alarm_on;
}
