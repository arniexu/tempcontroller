#ifndef TEST_DECLS_H
#define TEST_DECLS_H

int test_bsp_oled_driver_run(void);
int test_scheduler_run(void);
int test_pid_run(void);
int test_debug_log_run(void);
int test_alarm_service_run(void);
int test_param_store_run(void);
int test_log_service_run(void);
int test_heater_ctrl_run(void);
int test_temp_manager_run(void);
int test_schedule_service_run(void);
int test_ui_service_run(void);
int test_protocol_export_run(void);
int test_app_main_run(void);

#endif
