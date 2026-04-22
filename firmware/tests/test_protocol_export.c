#include "protocol_export.h"

#include <string.h>

#include "param_store.h"
#include "test_framework.h"

void bsp_uart_test_reset(void);
void bsp_uart_test_feed(const char *line);
const char *bsp_uart_test_output(void);

int test_protocol_export_run(void)
{
    param_store_init();
    protocol_export_init();

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
    bsp_uart_test_feed("NO_SUCH_CMD");
    protocol_export_process();
    TEST_ASSERT_TRUE(strstr(bsp_uart_test_output(), "ERR,UNKNOWN_CMD") != 0);

    return 0;
}
