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
    ctx->out_min = out_min;
    ctx->out_max = out_max;
}

float pid_step(pid_ctx_t *ctx, float setpoint, float measured)
{
    float error;
    float derivative;
    float output;

    if (ctx == 0)
    {
        return 0.0f;
    }

    error = setpoint - measured;
    ctx->integrator += error * ctx->dt_s;
    derivative = (error - ctx->prev_error) / ctx->dt_s;

    output = (ctx->kp * error) + (ctx->ki * ctx->integrator) + (ctx->kd * derivative);
    output = clamp(output, ctx->out_min, ctx->out_max);

    if ((output <= ctx->out_min) || (output >= ctx->out_max))
    {
        ctx->integrator -= error * ctx->dt_s;
    }

    ctx->prev_error = error;
    return output;
}
