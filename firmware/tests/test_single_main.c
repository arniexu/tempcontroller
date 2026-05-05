#include "test_decls.h"

#ifndef HOST_SINGLE_TEST
#error "Define HOST_SINGLE_TEST to the test function name, for example -DHOST_SINGLE_TEST=test_tune_service_run"
#endif

int main(void)
{
    return HOST_SINGLE_TEST();
}