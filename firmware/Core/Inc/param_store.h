#ifndef PARAM_STORE_H
#define PARAM_STORE_H

typedef struct
{
    float set_temp_c;
    float alarm_threshold_c;
    float kp;
    float ki;
    float kd;
    unsigned int schedule_enabled;
    unsigned int schedule_start_min;
    unsigned int schedule_end_min;
    unsigned int log_period_s;
} app_params_t;

void param_store_init(void);
void param_store_load_defaults(app_params_t *params);
void param_store_load(app_params_t *params);
void param_store_save(const app_params_t *params);
void param_store_tick_1s(void);
void param_store_flush_now(void);
const app_params_t *param_store_get(void);
app_params_t *param_store_get_mutable(void);

#endif
