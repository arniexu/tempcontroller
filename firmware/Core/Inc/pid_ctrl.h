#ifndef PID_CTRL_H
#define PID_CTRL_H

typedef struct
{
    float kp;
    float ki;
    float kd;
    float dt_s;
    float integrator;
    float prev_error;
    float prev_measured;
    float d_lpf;
    float d_lpf_alpha;
    float out_min;
    float out_max;
} pid_ctx_t;

void pid_init(pid_ctx_t *ctx, float kp, float ki, float kd, float dt_s, float out_min, float out_max);
float pid_step(pid_ctx_t *ctx, float setpoint, float measured);

#endif
