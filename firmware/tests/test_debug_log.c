#include "debug_log.h"

#include <string.h>

#include "scheduler.h"
#include "test_framework.h"

void bsp_uart_test_reset(void);
const char *bsp_uart_test_output(void);

int test_debug_log_run(void)
{
    scheduler_init();
    bsp_uart_test_reset();

    debug_log_init();
    debug_log_info("T", "temp=%.1f", 48.5f);

    TEST_ASSERT_TRUE(strstr(bsp_uart_test_output(), "[I][T] temp=48.5") != 0);
    return 0;
}
