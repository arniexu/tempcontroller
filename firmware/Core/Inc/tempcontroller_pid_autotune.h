#ifndef TEMPCONTROLLER_PID_AUTOTUNE_H
#define TEMPCONTROLLER_PID_AUTOTUNE_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif
typedef struct {
    float kp;
    float ki;
    float kd;
} tempctrl_pid_gains_t;

typedef struct {
    float setpoint_c;
    float relay_amplitude_c;
    float hysteresis_c;
    uint8_t cycles;
    uint32_t timeout_ms;
} tempctrl_pid_autotune_cfg_t;

typedef struct {
    bool running;
    bool relay_on;
    bool has_last_peak_high;
    bool has_last_peak_low;
    float last_peak_high_temp;
    float last_peak_low_temp;
    uint32_t start_tick_ms;
    uint32_t last_peak_high_tick_ms;
    uint32_t period_count;
    uint32_t amplitude_count;
    float period_sum_s;
    float amplitude_sum_c;
    tempctrl_pid_autotune_cfg_t cfg;
} tempctrl_pid_autotune_t;

void tempctrl_pid_autotune_init(tempctrl_pid_autotune_t *ctx);
void tempctrl_pid_autotune_start(tempctrl_pid_autotune_t *ctx, const tempctrl_pid_autotune_cfg_t *cfg, uint32_t now_ms);
void tempctrl_pid_autotune_stop(tempctrl_pid_autotune_t *ctx);
bool tempctrl_pid_autotune_step(
    tempctrl_pid_autotune_t *ctx,
    float current_temp_c,
    uint32_t now_ms,
    bool *relay_on,
    tempctrl_pid_gains_t *out_gains);

#ifdef __cplusplus
}
#endif

#endif
