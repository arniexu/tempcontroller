#include "bsp_rtc.h"

#if defined(USE_STDPERIPH_DRIVER)
#include "stm32f10x_bkp.h"
#include "stm32f10x_pwr.h"
#include "stm32f10x_rcc.h"
#include "stm32f10x_rtc.h"

#define RTC_BKP_MAGIC               (0xA5A5U)
#define RTC_READY_TIMEOUT           (1000000U)

static bool wait_rtc_flag(uint32_t flag)
{
    uint32_t timeout = RTC_READY_TIMEOUT;

    while ((RCC_GetFlagStatus(flag) == RESET) && (timeout > 0U))
    {
        timeout--;
    }

    return (timeout > 0U);
}

static void rtc_select_clock(void)
{
    if ((BKP_ReadBackupRegister(BKP_DR1) == RTC_BKP_MAGIC) && (RCC_GetFlagStatus(RCC_FLAG_LSERDY) != RESET))
    {
        RCC_RTCCLKConfig(RCC_RTCCLKSource_LSE);
        return;
    }

    RCC_LSEConfig(RCC_LSE_ON);
    if (wait_rtc_flag(RCC_FLAG_LSERDY))
    {
        RCC_RTCCLKConfig(RCC_RTCCLKSource_LSE);
        return;
    }

    RCC_LSICmd(ENABLE);
    if (wait_rtc_flag(RCC_FLAG_LSIRDY))
    {
        RCC_RTCCLKConfig(RCC_RTCCLKSource_LSI);
    }
}

static void rtc_init_counter_if_needed(void)
{
    if (BKP_ReadBackupRegister(BKP_DR1) == RTC_BKP_MAGIC)
    {
        return;
    }

    if (RCC_GetFlagStatus(RCC_FLAG_LSERDY) != RESET)
    {
        RTC_SetPrescaler(32767U);
    }
    else
    {
        RTC_SetPrescaler(39999U);
    }
    RTC_WaitForLastTask();

    RTC_SetCounter(0U);
    RTC_WaitForLastTask();

    BKP_WriteBackupRegister(BKP_DR1, RTC_BKP_MAGIC);
}
#endif

static uint16_t g_mock_minutes_of_day = 0U;
static bool g_mock_valid = true;

void bsp_rtc_init(void)
{
#if defined(USE_STDPERIPH_DRIVER)
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_BKP | RCC_APB1Periph_PWR, ENABLE);
    PWR_BackupAccessCmd(ENABLE);

    rtc_select_clock();
    RCC_RTCCLKCmd(ENABLE);

    RTC_WaitForSynchro();
    RTC_WaitForLastTask();

    rtc_init_counter_if_needed();
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
