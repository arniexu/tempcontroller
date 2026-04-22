#ifndef PARAM_STORE_H
#define PARAM_STORE_H

typedef struct
{
    float set_temp_c;
    float alarm_threshold_c;
    float kp;
    float ki;
    float kd;
} app_params_t;

void param_store_init(void);
void param_store_load_defaults(app_params_t *params);
void param_store_load(app_params_t *params);
void param_store_save(const app_params_t *params);
const app_params_t *param_store_get(void);
app_params_t *param_store_get_mutable(void);

#endif
