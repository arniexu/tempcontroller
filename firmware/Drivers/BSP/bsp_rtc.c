#include "bsp_rtc.h"

#if defined(USE_STDPERIPH_DRIVER)
#include "stm32f10x_bkp.h"
#include "stm32f10x_pwr.h"
#include "stm32f10x_rcc.h"
#include "stm32f10x_rtc.h"
#endif

static uint16_t g_mock_minutes_of_day = 0U;
static bool g_mock_valid = true;

void bsp_rtc_init(void)
{
#if defined(USE_STDPERIPH_DRIVER)
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_BKP | RCC_APB1Periph_PWR, ENABLE);
    PWR_BackupAccessCmd(ENABLE);
#endif
}

bool bsp_rtc_get_minutes_of_day(uint16_t *minutes_of_day)
{
    if (minutes_of_day == 0)
    {
        return false;
    }

#if defined(USE_STDPERIPH_DRIVER)
    {
        uint32_t sec = RTC_GetCounter();
        uint32_t sec_of_day = sec % 86400U;
        *minutes_of_day = (uint16_t)(sec_of_day / 60U);
        return true;
    }
#else
    if (!g_mock_valid)
    {
        return false;
    }

    *minutes_of_day = g_mock_minutes_of_day;
    return true;
#endif
}

void bsp_rtc_mock_set_minutes_of_day(uint16_t minutes_of_day)
{
    g_mock_minutes_of_day = (uint16_t)(minutes_of_day % 1440U);
}

void bsp_rtc_mock_set_valid(bool valid)
{
    g_mock_valid = valid;
}
