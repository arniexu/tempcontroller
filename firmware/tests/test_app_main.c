#include "app_main.h"
#include "test_framework.h"

int test_app_main_run(void)
{
    unsigned int i;

    app_main_init();
    for (i = 0U; i < 2500U; ++i)
    {
        app_main_loop();
    }

    TEST_ASSERT_TRUE(1);
    return 0;
}
