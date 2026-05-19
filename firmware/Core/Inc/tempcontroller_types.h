#ifndef TEMPCONTROLLER_TYPES_H
#define TEMPCONTROLLER_TYPES_H

#include <stdbool.h>
#include <stdint.h>

typedef enum {
    TEMPCTRL_STATE_IDLE = 0,
    TEMPCTRL_STATE_HEATING,
    TEMPCTRL_STATE_HOLD,
    TEMPCTRL_STATE_AUTOTUNE
} tempctrl_state_t;

typedef enum {
    TEMPCTRL_FOCUS_TARGET = 0,
    TEMPCTRL_FOCUS_TOLERANCE
} tempctrl_focus_t;

typedef struct {
    float current_temp_c;
    float target_temp_c;
    float tolerance_c;
    float pid_kp;
    float pid_ki;
    float pid_kd;
    bool heating_on;
    bool autotune_running;
    bool autotune_done;
    tempctrl_state_t state;
    tempctrl_focus_t focus;
} tempctrl_runtime_t;

typedef struct {
    int8_t step;
    bool key_pressed;
} tempctrl_input_event_t;

#endif
