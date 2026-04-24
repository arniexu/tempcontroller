#include "protocol_export.h"

#include <stdio.h>
#include <string.h>

#include "bsp_uart.h"
#include "debug_log.h"
#include "log_service.h"
#include "param_store.h"
#include "schedule_service.h"
#include "temp_manager.h"

static void send_ok(const char *payload)
{
	char line[160];
	(void)snprintf(line, sizeof(line), "OK,%s\r\n", payload);
	bsp_uart_write(line);
}

static void send_err(const char *payload)
{
	char line[160];
	(void)snprintf(line, sizeof(line), "ERR,%s\r\n", payload);
	bsp_uart_write(line);
}

static void handle_read_temp(void)
{
	char line[160];
	const temp_snapshot_t *t = temp_manager_get_snapshot();
	(void)snprintf(line,
				   sizeof(line),
				   "TEMP,%.2f,%.2f,%.2f,%.2f,mask=%u,degraded=%d,fault=%d",
				   t->t1,
				   t->t2,
				   t->t3,
				   t->t_ctrl,
				   (unsigned int)t->valid_mask,
				   t->sensor_degraded ? 1 : 0,
				   t->sensor_fault ? 1 : 0);
	send_ok(line);
}

static void handle_read_param(void)
{
	char line[160];
	const app_params_t *p = param_store_get();
	(void)snprintf(line,
				   sizeof(line),
				   "PARAM,set=%.2f,alarm=%.2f,kp=%.3f,ki=%.3f,kd=%.3f,sch_en=%u,sch_start=%u,sch_end=%u,log_s=%u",
				   p->set_temp_c,
				   p->alarm_threshold_c,
				   p->kp,
				   p->ki,
				   p->kd,
				   p->schedule_enabled,
				   p->schedule_start_min,
				   p->schedule_end_min,
				   p->log_period_s);
	send_ok(line);
}

static void handle_set_temp(const char *cmd)
{
	float set_temp;
	app_params_t *p;

	if (sscanf(cmd, "SET_TEMP=%f", &set_temp) != 1)
	{
		debug_log_warn("PROTO", "bad set temp cmd: %s", cmd);
		send_err("BAD_SET_TEMP");
		return;
	}

	if ((set_temp < 20.0f) || (set_temp > 60.0f))
	{
		debug_log_warn("PROTO", "set temp out of range: %.2f", set_temp);
		send_err("SET_TEMP_OUT_OF_RANGE");
		return;
	}

	p = param_store_get_mutable();
	p->set_temp_c = set_temp;
	param_store_save(p);
	debug_log_info("PROTO", "set temp applied: %.2f", set_temp);
	send_ok("SET_TEMP_APPLIED");
}

static void handle_set_pid_values(float kp, float ki, float kd)
{
	app_params_t *p;

	if ((kp < 0.0f) || (kp > 30.0f) || (ki < 0.0f) || (ki > 5.0f) || (kd < 0.0f) || (kd > 50.0f))
	{
		debug_log_warn("PROTO", "pid out of range kp=%.3f ki=%.3f kd=%.3f", kp, ki, kd);
		send_err("PID_OUT_OF_RANGE");
		return;
	}

	p = param_store_get_mutable();
	p->kp = kp;
	p->ki = ki;
	p->kd = kd;
	param_store_save(p);

	debug_log_info("PROTO", "pid applied kp=%.3f ki=%.3f kd=%.3f", kp, ki, kd);
	send_ok("PID_APPLIED");
}

static void handle_set_alarm_values(float alarm_c)
{
	app_params_t *p;

	if ((alarm_c < 40.0f) || (alarm_c > 90.0f))
	{
		debug_log_warn("PROTO", "alarm out of range: %.2f", alarm_c);
		send_err("ALARM_OUT_OF_RANGE");
		return;
	}

	p = param_store_get_mutable();
	p->alarm_threshold_c = alarm_c;
	param_store_save(p);

	debug_log_info("PROTO", "alarm applied: %.2f", alarm_c);
	send_ok("ALARM_APPLIED");
}

static void handle_set_schedule_values(unsigned int enabled, unsigned int start_min, unsigned int end_min)
{
	app_params_t *p;
	schedule_config_t cfg;

	if (enabled > 1U)
	{
		send_err("SCHEDULE_BAD_ENABLE");
		return;
	}
	if ((start_min >= 1440U) || (end_min >= 1440U))
	{
		send_err("SCHEDULE_OUT_OF_RANGE");
		return;
	}

	p = param_store_get_mutable();
	p->schedule_enabled = enabled;
	p->schedule_start_min = start_min;
	p->schedule_end_min = end_min;
	param_store_save(p);

	cfg.enabled = (enabled != 0U);
	cfg.start_min_of_day = (uint16_t)start_min;
	cfg.end_min_of_day = (uint16_t)end_min;
	schedule_service_set_config(&cfg);

	debug_log_info("PROTO", "schedule applied en=%u start=%u end=%u", enabled, start_min, end_min);
	send_ok("SCHEDULE_APPLIED");
}

static void handle_set_log_period_values(unsigned int period_s)
{
	app_params_t *p;

	if ((period_s < 1U) || (period_s > 60U))
	{
		send_err("LOG_PERIOD_OUT_OF_RANGE");
		return;
	}

	p = param_store_get_mutable();
	p->log_period_s = period_s;
	param_store_save(p);

	debug_log_info("PROTO", "log period applied: %us", period_s);
	send_ok("LOG_PERIOD_APPLIED");
}

static void handle_set_pid(const char *cmd)
{
	float kp;
	float ki;
	float kd;

	if (sscanf(cmd, "SET_PID=%f,%f,%f", &kp, &ki, &kd) != 3)
	{
		debug_log_warn("PROTO", "bad set pid cmd: %s", cmd);
		send_err("BAD_SET_PID");
		return;
	}

	handle_set_pid_values(kp, ki, kd);
}

static void handle_conf_pid(const char *cmd)
{
	float kp;
	float ki;
	float kd;

	if (sscanf(cmd, "CONF:PID %f,%f,%f", &kp, &ki, &kd) != 3)
	{
		debug_log_warn("PROTO", "bad conf pid cmd: %s", cmd);
		send_err("BAD_CONF_PID");
		return;
	}

	handle_set_pid_values(kp, ki, kd);
}

static void handle_set_alarm(const char *cmd)
{
	float alarm_c;

	if (sscanf(cmd, "SET_ALARM=%f", &alarm_c) != 1)
	{
		send_err("BAD_SET_ALARM");
		return;
	}

	handle_set_alarm_values(alarm_c);
}

static void handle_conf_alarm(const char *cmd)
{
	float alarm_c;

	if (sscanf(cmd, "CONF:ALARM %f", &alarm_c) != 1)
	{
		send_err("BAD_CONF_ALARM");
		return;
	}

	handle_set_alarm_values(alarm_c);
}

static void handle_set_schedule(const char *cmd)
{
	unsigned int enabled;
	unsigned int start_min;
	unsigned int end_min;

	if (sscanf(cmd, "SET_SCHEDULE=%u,%u,%u", &enabled, &start_min, &end_min) != 3)
	{
		send_err("BAD_SET_SCHEDULE");
		return;
	}

	handle_set_schedule_values(enabled, start_min, end_min);
}

static void handle_conf_schedule(const char *cmd)
{
	unsigned int enabled;
	unsigned int start_min;
	unsigned int end_min;

	if (sscanf(cmd, "CONF:SCH %u,%u,%u", &enabled, &start_min, &end_min) != 3)
	{
		send_err("BAD_CONF_SCH");
		return;
	}

	handle_set_schedule_values(enabled, start_min, end_min);
}

static void handle_set_log_period(const char *cmd)
{
	unsigned int period_s;

	if (sscanf(cmd, "SET_LOG_PERIOD=%u", &period_s) != 1)
	{
		send_err("BAD_SET_LOG_PERIOD");
		return;
	}

	handle_set_log_period_values(period_s);
}

static void handle_conf_log_period(const char *cmd)
{
	unsigned int period_s;

	if (sscanf(cmd, "CONF:LOGPERIOD %u", &period_s) != 1)
	{
		send_err("BAD_CONF_LOGPERIOD");
		return;
	}

	handle_set_log_period_values(period_s);
}

static void handle_log_export(void)
{
	char line[200];
	unsigned int i;
	unsigned int cnt = log_service_count();
	log_record_t rec;

	bsp_uart_write("OK,LOG_BEGIN\r\n");
	bsp_uart_write("index,t1,t2,t3,t_avg,set_temp,pid_out,heater,alarm\r\n");

	for (i = 0U; i < cnt; ++i)
	{
		if (!log_service_get(i, &rec))
		{
			continue;
		}

		(void)snprintf(line,
					   sizeof(line),
					   "%u,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%d,%d\r\n",
					   rec.index,
					   rec.t1,
					   rec.t2,
					   rec.t3,
					   rec.t_avg,
					   rec.set_temp,
					   rec.pid_out,
					   rec.heater_on,
					   rec.alarm_on);
		bsp_uart_write(line);
	}

	bsp_uart_write("OK,LOG_END\r\n");
}

void protocol_export_init(void)
{
	bsp_uart_init();
}

void protocol_export_process(void)
{
	char cmd[80];

	if (!bsp_uart_read_line(cmd, sizeof(cmd)))
	{
		return;
	}

	debug_log_info("PROTO", "rx cmd: %s", cmd);

	if (strcmp(cmd, "READ_TEMP") == 0)
	{
		handle_read_temp();
	}
	else if (strcmp(cmd, "READ_PARAM") == 0)
	{
		handle_read_param();
	}
	else if (strncmp(cmd, "SET_TEMP=", 9) == 0)
	{
		handle_set_temp(cmd);
	}
	else if (strncmp(cmd, "SET_PID=", 8) == 0)
	{
		handle_set_pid(cmd);
	}
	else if (strncmp(cmd, "CONF:PID ", 9) == 0)
	{
		handle_conf_pid(cmd);
	}
	else if (strncmp(cmd, "SET_ALARM=", 10) == 0)
	{
		handle_set_alarm(cmd);
	}
	else if (strncmp(cmd, "CONF:ALARM ", 11) == 0)
	{
		handle_conf_alarm(cmd);
	}
	else if (strncmp(cmd, "SET_SCHEDULE=", 13) == 0)
	{
		handle_set_schedule(cmd);
	}
	else if (strncmp(cmd, "CONF:SCH ", 9) == 0)
	{
		handle_conf_schedule(cmd);
	}
	else if (strncmp(cmd, "SET_LOG_PERIOD=", 15) == 0)
	{
		handle_set_log_period(cmd);
	}
	else if (strncmp(cmd, "CONF:LOGPERIOD ", 15) == 0)
	{
		handle_conf_log_period(cmd);
	}
	else if (strcmp(cmd, "LOG_CLEAR") == 0)
	{
		log_service_clear();
		debug_log_info("PROTO", "log cleared");
		send_ok("LOG_CLEARED");
	}
	else if (strcmp(cmd, "LOG_EXPORT") == 0)
	{
		debug_log_info("PROTO", "log export count=%u", log_service_count());
		handle_log_export();
	}
	else
	{
		debug_log_warn("PROTO", "unknown cmd: %s", cmd);
		send_err("UNKNOWN_CMD");
	}
}
