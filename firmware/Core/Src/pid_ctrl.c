#include "pid_ctrl.h"

static float clamp(float x, float low, float high)
{
    if (x < low)
    {
        return low;
    }
    if (x > high)
    {
        return high;
    }
    return x;
}

void pid_init(pid_ctx_t *ctx, float kp, float ki, float kd, float dt_s, float out_min, float out_max)
{
    if (ctx == 0)
    {
        return;
    }

    ctx->kp = kp;
    ctx->ki = ki;
    ctx->kd = kd;
    ctx->dt_s = dt_s;
    ctx->integrator = 0.0f;
    ctx->prev_error = 0.0f;
    ctx->prev_measured = 0.0f;
    ctx->d_lpf = 0.0f;
    ctx->d_lpf_alpha = 0.2f;
    ctx->out_min = out_min;
    ctx->out_max = out_max;
}

float pid_step(pid_ctx_t *ctx, float setpoint, float measured)
{
    float error;
    float p_term;
    float d_raw;
    float d_term;
    float i_candidate;
    float unsat;
    float output;

    if (ctx == 0)
    {
        return 0.0f;
    }

    if (ctx->dt_s <= 0.0f)
    {
        error = setpoint - measured;
        return clamp(ctx->kp * error, ctx->out_min, ctx->out_max);
    }

    error = setpoint - measured;
    p_term = ctx->kp * error;

    d_raw = (measured - ctx->prev_measured) / ctx->dt_s;
    ctx->d_lpf += ctx->d_lpf_alpha * (d_raw - ctx->d_lpf);
    d_term = -ctx->kd * ctx->d_lpf;

    i_candidate = ctx->integrator + (ctx->ki * error * ctx->dt_s);
    unsat = p_term + i_candidate + d_term;
    output = clamp(unsat, ctx->out_min, ctx->out_max);

    if (!((output >= ctx->out_max && error > 0.0f) || (output <= ctx->out_min && error < 0.0f)))
    {
        ctx->integrator = i_candidate;
    }

    output = clamp(p_term + ctx->integrator + d_term, ctx->out_min, ctx->out_max);

    ctx->prev_error = error;
    ctx->prev_measured = measured;
    return output;
}
