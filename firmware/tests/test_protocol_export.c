#include "protocol_export.h"

#include <string.h>

#include "app_config.h"
#include "param_store.h"
#include "schedule_service.h"
#include "test_framework.h"

void bsp_uart_test_reset(void);
void bsp_uart_test_feed(const char *line);
const char *bsp_uart_test_output(void);

int test_protocol_export_run(void)
{
    schedule_config_t cfg;

    param_store_init();
    protocol_export_init();
    schedule_service_init();

    bsp_uart_test_reset();
    bsp_uart_test_feed("READ_PARAM");
    protocol_export_process();
    TEST_ASSERT_TRUE(strstr(bsp_uart_test_output(), "OK,PARAM") != 0);

    bsp_uart_test_reset();
    bsp_uart_test_feed("SET_TEMP=50.0");
    protocol_export_process();
    TEST_ASSERT_TRUE(strstr(bsp_uart_test_output(), "OK,SET_TEMP_APPLIED") != 0);
    TEST_ASSERT_NEAR_FLOAT(param_store_get()->set_temp_c, 50.0f, 0.01f);

    bsp_uart_test_reset();
    bsp_uart_test_feed("SET_PID=9.5,0.8,12.0");
    protocol_export_process();
    TEST_ASSERT_TRUE(strstr(bsp_uart_test_output(), "OK,PID_APPLIED") != 0);
    TEST_ASSERT_NEAR_FLOAT(param_store_get()->kp, 9.5f, 0.01f);
    TEST_ASSERT_NEAR_FLOAT(param_store_get()->ki, 0.8f, 0.01f);
    TEST_ASSERT_NEAR_FLOAT(param_store_get()->kd, 12.0f, 0.01f);

    bsp_uart_test_reset();
    bsp_uart_test_feed("CONF:PID 8.0,0.3,15.0");
    protocol_export_process();
    TEST_ASSERT_TRUE(strstr(bsp_uart_test_output(), "OK,PID_APPLIED") != 0);
    TEST_ASSERT_NEAR_FLOAT(param_store_get()->kp, 8.0f, 0.01f);
    TEST_ASSERT_NEAR_FLOAT(param_store_get()->ki, 0.3f, 0.01f);
    TEST_ASSERT_NEAR_FLOAT(param_store_get()->kd, 15.0f, 0.01f);

    bsp_uart_test_reset();
    bsp_uart_test_feed("SET_PID=100.0,0.3,15.0");
    protocol_export_process();
    TEST_ASSERT_TRUE(strstr(bsp_uart_test_output(), "ERR,PID_OUT_OF_RANGE") != 0);

    bsp_uart_test_reset();
    bsp_uart_test_feed("SET_ALARM=65.0");
    protocol_export_process();
    TEST_ASSERT_TRUE(strstr(bsp_uart_test_output(), "OK,ALARM_APPLIED") != 0);
    TEST_ASSERT_NEAR_FLOAT(param_store_get()->alarm_threshold_c, 65.0f, 0.01f);

    bsp_uart_test_reset();
    bsp_uart_test_feed("CONF:ALARM 95.0");
    protocol_export_process();
    TEST_ASSERT_TRUE(strstr(bsp_uart_test_output(), "ERR,ALARM_OUT_OF_RANGE") != 0);

    bsp_uart_test_reset();
    bsp_uart_test_feed("SET_SCHEDULE=1,480,600");
    protocol_export_process();
    TEST_ASSERT_TRUE(strstr(bsp_uart_test_output(), "OK,SCHEDULE_APPLIED") != 0);
    TEST_ASSERT_EQ_INT(param_store_get()->schedule_enabled, 1U);
    TEST_ASSERT_EQ_INT(param_store_get()->schedule_start_min, 480U);
    TEST_ASSERT_EQ_INT(param_store_get()->schedule_end_min, 600U);
    schedule_service_get_config(&cfg);
    TEST_ASSERT_TRUE(cfg.enabled);
    TEST_ASSERT_EQ_INT(cfg.start_min_of_day, 480U);
    TEST_ASSERT_EQ_INT(cfg.end_min_of_day, 600U);

    bsp_uart_test_reset();
    bsp_uart_test_feed("CONF:SCH 1,1500,20");
    protocol_export_process();
    TEST_ASSERT_TRUE(strstr(bsp_uart_test_output(), "ERR,SCHEDULE_OUT_OF_RANGE") != 0);

    bsp_uart_test_reset();
    bsp_uart_test_feed("SET_LOG_PERIOD=5");
    protocol_export_process();
    TEST_ASSERT_TRUE(strstr(bsp_uart_test_output(), "OK,LOG_PERIOD_APPLIED") != 0);
    TEST_ASSERT_EQ_INT(param_store_get()->log_period_s, 5U);

    {
        unsigned int i;
        for (i = 0U; i < APP_PARAM_STORE_FLUSH_DELAY_S; ++i)
        {
            param_store_tick_1s();
        }
    }
    param_store_init();
    TEST_ASSERT_EQ_INT(param_store_get()->log_period_s, 5U);

    bsp_uart_test_reset();
    bsp_uart_test_feed("CONF:LOGPERIOD 0");
    protocol_export_process();
    TEST_ASSERT_TRUE(strstr(bsp_uart_test_output(), "ERR,LOG_PERIOD_OUT_OF_RANGE") != 0);

    bsp_uart_test_reset();
    bsp_uart_test_feed("NO_SUCH_CMD");
    protocol_export_process();
    TEST_ASSERT_TRUE(strstr(bsp_uart_test_output(), "ERR,UNKNOWN_CMD") != 0);

    return 0;
}
