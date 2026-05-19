#include "tempcontroller_pid_autotune.h"

#define TEMPCONTROLLER_PI_VALUE 3.1415926f

void tempctrl_pid_autotune_init(tempctrl_pid_autotune_t *ctx)
{
    if (ctx == NULL) {
        return;
    }
    *ctx = (tempctrl_pid_autotune_t){0};
}

void tempctrl_pid_autotune_start(tempctrl_pid_autotune_t *ctx, const tempctrl_pid_autotune_cfg_t *cfg, uint32_t now_ms)
{
    if (ctx == NULL || cfg == NULL) {
        return;
    }

    *ctx = (tempctrl_pid_autotune_t){0};
    ctx->cfg = *cfg;
    ctx->running = true;
    ctx->relay_on = true;
    ctx->start_tick_ms = now_ms;
}

void tempctrl_pid_autotune_stop(tempctrl_pid_autotune_t *ctx)
{
    if (ctx == NULL) {
        return;
    }
    ctx->running = false;
}

static bool tempctrl_pid_autotune_finalize(const tempctrl_pid_autotune_t *ctx, tempctrl_pid_gains_t *out_gains)
{
    if (ctx == NULL || out_gains == NULL || ctx->period_count == 0U || ctx->amplitude_count == 0U) {
        return false;
    }

    float pu_s = ctx->period_sum_s / (float)ctx->period_count;
    float amp_c = ctx->amplitude_sum_c / (float)ctx->amplitude_count;
    if (pu_s <= 0.0f || amp_c <= 0.0f || ctx->cfg.relay_amplitude_c <= 0.0f) {
        return false;
    }

    float ku = (4.0f * ctx->cfg.relay_amplitude_c) / (TEMPCONTROLLER_PI_VALUE * amp_c);
    out_gains->kp = 0.6f * ku;
    out_gains->ki = (1.2f * ku) / pu_s;
    out_gains->kd = 0.075f * ku * pu_s;
    return true;
}

bool tempctrl_pid_autotune_step(
    tempctrl_pid_autotune_t *ctx,
    float current_temp_c,
    uint32_t now_ms,
    bool *relay_on,
    tempctrl_pid_gains_t *out_gains)
{
    if (ctx == NULL || relay_on == NULL) {
        return false;
    }

    if (!ctx->running) {
        *relay_on = false;
        return false;
    }

    if ((now_ms - ctx->start_tick_ms) > ctx->cfg.timeout_ms) {
        ctx->running = false;
        *relay_on = false;
        return false;
    }

    float upper = ctx->cfg.setpoint_c + ctx->cfg.hysteresis_c;
    float lower = ctx->cfg.setpoint_c - ctx->cfg.hysteresis_c;

    if (ctx->relay_on && current_temp_c >= upper) {
        ctx->relay_on = false;
        ctx->last_peak_high_temp = current_temp_c;
        if (ctx->has_last_peak_high) {
            float period_s = (float)(now_ms - ctx->last_peak_high_tick_ms) / 1000.0f;
            if (period_s > 0.0f) {
                ctx->period_sum_s += period_s;
                ctx->period_count++;
            }
        }
        ctx->last_peak_high_tick_ms = now_ms;
        ctx->has_last_peak_high = true;
        if (ctx->has_last_peak_low) {
            float amp = (ctx->last_peak_high_temp - ctx->last_peak_low_temp) * 0.5f;
            if (amp > 0.0f) {
                ctx->amplitude_sum_c += amp;
                ctx->amplitude_count++;
            }
        }
    } else if (!ctx->relay_on && current_temp_c <= lower) {
        ctx->relay_on = true;
        ctx->last_peak_low_temp = current_temp_c;
        ctx->has_last_peak_low = true;
    }

    *relay_on = ctx->relay_on;

    if (ctx->period_count >= ctx->cfg.cycles && ctx->amplitude_count >= ctx->cfg.cycles) {
        bool ok = tempctrl_pid_autotune_finalize(ctx, out_gains);
        ctx->running = false;
        return ok;
    }

    return false;
}
