#include "app_main.h"

#include "alarm_service.h"
#include "app_config.h"
#include "app_state.h"
#include "bsp_buzzer.h"
#include "debug_log.h"
#include "heater_ctrl.h"
#include "log_service.h"
#include "param_store.h"
#include "pid_ctrl.h"
#include "protocol_export.h"
#include "schedule_service.h"
#include "scheduler.h"
#include "temp_manager.h"
#include "ui_service.h"

static app_mode_t g_mode = APP_MODE_IDLE;
static app_mode_t g_prev_mode = APP_MODE_IDLE;
static pid_ctx_t g_pid;
static float g_pid_out = 0.0f;
static bool g_prev_alarm = false;
static unsigned int g_log_tick_count = 0U;

static const char *mode_to_str(app_mode_t mode)
{
    switch (mode)
    {
    case APP_MODE_IDLE:
        return "IDLE";
    case APP_MODE_HEATING:
        return "HEATING";
    case APP_MODE_SCHEDULED:
        return "SCHEDULED";
    case APP_MODE_ALARM:
        return "ALARM";
    case APP_MODE_EXPORT:
        return "EXPORT";
    default:
        return "UNKNOWN";
    }
}

void app_main_init(void)
{
    scheduler_init();
    param_store_init();

    protocol_export_init();
    debug_log_init();
    debug_log_info("APP", "init start");

    temp_manager_init();
    alarm_service_init();
    bsp_buzzer_init();
    bsp_buzzer_set(false);
    schedule_service_init();
    {
        schedule_config_t cfg;
        const app_params_t *params = param_store_get();
        cfg.enabled = (params->schedule_enabled != 0U);
        cfg.start_min_of_day = (uint16_t)(params->schedule_start_min % 1440U);
        cfg.end_min_of_day = (uint16_t)(params->schedule_end_min % 1440U);
        schedule_service_set_config(&cfg);
    }
    log_service_init();
    ui_service_init();

    {
        const app_params_t *params = param_store_get();
        pid_init(&g_pid, params->kp, params->ki, params->kd, 1.0f, 0.0f, 100.0f);
    }
    heater_ctrl_init(APP_PID_WINDOW_MS);

    g_mode = APP_MODE_IDLE;
    g_prev_mode = g_mode;
    g_prev_alarm = false;
    g_log_tick_count = 0U;

    debug_log_info("APP", "init done set=%.2f alarm=%.2f", param_store_get()->set_temp_c, param_store_get()->alarm_threshold_c);
}

void app_main_loop(void)
{
    scheduler_flags_t flags;
    const temp_snapshot_t *temp;

    scheduler_tick_1ms();
    heater_ctrl_update_1ms();

    scheduler_poll(&flags);

    if (flags.task_key_100ms)
    {
        ui_service_tick_100ms(param_store_get_mutable());
    }

    if (flags.task_control_1s)
    {
        bool alarm_now;

        temp_manager_update();
        temp = temp_manager_get_snapshot();
        {
            const app_params_t *params = param_store_get();

            alarm_service_update(temp->t1, temp->t2, temp->t3, params->alarm_threshold_c, temp->sensor_fault);
            schedule_service_update();
            alarm_now = alarm_service_is_active();
            bsp_buzzer_set(alarm_now);

            if (alarm_now)
            {
                g_mode = APP_MODE_ALARM;
                g_pid_out = 0.0f;
                heater_ctrl_force_off();
            }
            else if (!schedule_service_heating_allowed())
            {
                g_mode = APP_MODE_SCHEDULED;
                g_pid_out = 0.0f;
                heater_ctrl_set_output_percent(0.0f);
            }
            else
            {
                g_mode = APP_MODE_HEATING;
                g_pid_out = pid_step(&g_pid, params->set_temp_c, temp->t_ctrl);
                heater_ctrl_set_output_percent(g_pid_out);
            }

            if (g_mode != g_prev_mode)
            {
                debug_log_info("APP", "mode %s -> %s", mode_to_str(g_prev_mode), mode_to_str(g_mode));
                g_prev_mode = g_mode;
            }

            if (alarm_now != g_prev_alarm)
            {
                if (alarm_now)
                {
                    debug_log_warn("APP", "alarm on t1=%.2f t2=%.2f t3=%.2f", temp->t1, temp->t2, temp->t3);
                }
                else
                {
                    debug_log_info("APP", "alarm cleared");
                }
                g_prev_alarm = alarm_now;
            }

            g_log_tick_count++;
            if (g_log_tick_count >= ((params->log_period_s == 0U) ? 1U : params->log_period_s))
            {
                g_log_tick_count = 0U;
                log_service_push(temp, params->set_temp_c, g_pid_out, heater_ctrl_get_state() ? 1 : 0, alarm_now ? 1 : 0);
            }
        }
    }

    if (flags.task_ui_200ms)
    {
        temp = temp_manager_get_snapshot();
        ui_service_tick_200ms(g_mode,
                              temp,
                              param_store_get(),
                              g_pid_out,
                              heater_ctrl_get_state() ? 1 : 0,
                              alarm_service_is_active() ? 1 : 0);
    }

    protocol_export_process();
}
