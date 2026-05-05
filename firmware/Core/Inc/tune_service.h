#ifndef TUNE_SERVICE_H
#define TUNE_SERVICE_H

#include <stdint.h>

#include "param_store.h"
#include "temp_manager.h"

typedef enum
{
    TUNE_FLOW_IDLE = 0,
    TUNE_FLOW_READY,
    TUNE_FLOW_PREHEAT,
    TUNE_FLOW_APPROACH,
    TUNE_FLOW_STABLE,
    TUNE_FLOW_DISTURB,
    TUNE_FLOW_ACCEPT,
    TUNE_FLOW_SAVE,
    TUNE_FLOW_ABORT
} tune_flow_state_t;

typedef enum
{
    TUNE_ISSUE_OK = 0,
    TUNE_ISSUE_SLOW,
    TUNE_ISSUE_OVERSHOOT,
    TUNE_ISSUE_WAVE,
    TUNE_ISSUE_STEADY_ERROR,
    TUNE_ISSUE_OUTPUT_SATURATED,
    TUNE_ISSUE_SENSOR_FAULT,
    TUNE_ISSUE_COUNT
} tune_issue_t;

typedef enum
{
    TUNE_ACTION_NONE = 0,
    TUNE_ACTION_KP_UP,
    TUNE_ACTION_KP_DOWN,
    TUNE_ACTION_KI_UP,
    TUNE_ACTION_KI_DOWN,
    TUNE_ACTION_KD_UP,
    TUNE_ACTION_CHECK_HEATER,
    TUNE_ACTION_CHECK_SENSOR,
    TUNE_ACTION_ENTER_ACCEPT
} tune_action_t;

typedef struct
{
    float kp;
    float ki;
    float kd;
    float kp_min;
    float kp_max;
    float ki_min;
    float ki_max;
    float kd_min;
    float kd_max;
    float tmin_c;
    float tmax_c;
} tune_pid_profile_t;

typedef struct
{
    const char *code;
    const char *name;
    const char *usage;
} tune_scene_info_t;

typedef struct
{
    tune_flow_state_t flow;
    tune_issue_t issue;
    tune_action_t action;
    uint8_t scene_id;
    uint8_t step_index;
    uint8_t accept_ready;
    float overshoot;
    float steady_error;
    float rise_rate;
    float recovery_time_s;
    char advice_text[24];
} tune_runtime_t;

void tune_service_init(void);
void tune_service_set_scene(uint8_t scene_id);
uint8_t tune_service_get_scene(void);
uint8_t tune_service_scene_count(void);
const tune_scene_info_t *tune_service_get_scene_info(uint8_t scene_id);
void tune_service_load_scene_defaults(app_params_t *params);
void tune_service_clamp_params(app_params_t *params);

void tune_service_step_change(int dir);
void tune_service_issue_change(int dir);
void tune_service_set_issue(tune_issue_t issue);

void tune_service_update_observation(const temp_snapshot_t *temp,
                                     const app_params_t *params,
                                     float pid_out,
                                     int heater_on);
void tune_service_apply_suggestion(app_params_t *params);

const tune_runtime_t *tune_service_get_runtime(void);
const tune_pid_profile_t *tune_service_get_profile(void);
void tune_service_set_profile(const tune_pid_profile_t *profile);

const char *tune_service_step_text(uint8_t step_index);
const char *tune_service_issue_text(tune_issue_t issue);
const char *tune_service_action_text(tune_action_t action);

#endif