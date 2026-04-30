#include "bsp_rtc.h"

#if defined(USE_STDPERIPH_DRIVER)
#include "stm32f10x_bkp.h"
#include "stm32f10x_pwr.h"
#include "stm32f10x_rcc.h"
#include "stm32f10x_rtc.h"

#define RTC_BKP_MAGIC               (0xA5A5U)
#define RTC_READY_TIMEOUT           (1000000U)

static bool g_rtc_ready = false;

static bool wait_rtc_flag(uint32_t flag)
{
    uint32_t timeout = RTC_READY_TIMEOUT;

    while ((RCC_GetFlagStatus(flag) == RESET) && (timeout > 0U))
    {
        timeout--;
    }

    return (timeout > 0U);
}

static bool rtc_wait_for_synchro_timeout(void)
{
    uint32_t timeout = RTC_READY_TIMEOUT;

    RTC->CRL &= (uint16_t)~RTC_CRL_RSF;
    while (((RTC->CRL & RTC_CRL_RSF) == 0U) && (timeout > 0U))
    {
        timeout--;
    }

    return (timeout > 0U);
}

static bool rtc_wait_for_last_task_timeout(void)
{
    uint32_t timeout = RTC_READY_TIMEOUT;

    while (((RTC->CRL & RTC_CRL_RTOFF) == 0U) && (timeout > 0U))
    {
        timeout--;
    }

    return (timeout > 0U);
}

static bool rtc_select_clock(void)
{
    if ((BKP_ReadBackupRegister(BKP_DR1) == RTC_BKP_MAGIC) && (RCC_GetFlagStatus(RCC_FLAG_LSERDY) != RESET))
    {
        RCC_RTCCLKConfig(RCC_RTCCLKSource_LSE);
        return true;
    }

    RCC_LSEConfig(RCC_LSE_ON);
    if (wait_rtc_flag(RCC_FLAG_LSERDY))
    {
        RCC_RTCCLKConfig(RCC_RTCCLKSource_LSE);
        return true;
    }

    RCC_LSICmd(ENABLE);
    if (wait_rtc_flag(RCC_FLAG_LSIRDY))
    {
        RCC_RTCCLKConfig(RCC_RTCCLKSource_LSI);
        return true;
    }

    return false;
}

static bool rtc_init_counter_if_needed(void)
{
    if (BKP_ReadBackupRegister(BKP_DR1) == RTC_BKP_MAGIC)
    {
        return true;
    }

    if (RCC_GetFlagStatus(RCC_FLAG_LSERDY) != RESET)
    {
        RTC_SetPrescaler(32767U);
    }
    else
    {
        RTC_SetPrescaler(39999U);
    }
    if (!rtc_wait_for_last_task_timeout())
    {
        return false;
    }

    RTC_SetCounter(0U);
    if (!rtc_wait_for_last_task_timeout())
    {
        return false;
    }

    BKP_WriteBackupRegister(BKP_DR1, RTC_BKP_MAGIC);
    return true;
}
#endif

#if !defined(USE_STDPERIPH_DRIVER)
static uint16_t g_mock_minutes_of_day = 0U;
static bool g_mock_valid = true;
#endif

void bsp_rtc_init(void)
{
#if defined(USE_STDPERIPH_DRIVER)
    g_rtc_ready = false;

    RCC_APB1PeriphClockCmd(RCC_APB1Periph_BKP | RCC_APB1Periph_PWR, ENABLE);
    PWR_BackupAccessCmd(ENABLE);

    if (!rtc_select_clock())
    {
        return;
    }

    RCC_RTCCLKCmd(ENABLE);

    if (!rtc_wait_for_synchro_timeout())
    {
        return;
    }

    if (!rtc_wait_for_last_task_timeout())
    {
        return;
    }

    if (!rtc_init_counter_if_needed())
    {
        return;
    }

    g_rtc_ready = true;
#else
    g_mock_valid = true;
#endif
}

bool bsp_rtc_get_minutes_of_day(uint16_t *minutes_of_day)
{
    if (minutes_of_day == 0)
    {
        return false;
    }

#if defined(USE_STDPERIPH_DRIVER)
    if (!g_rtc_ready)
    {
        return false;
    }

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
#if defined(USE_STDPERIPH_DRIVER)
    (void)minutes_of_day;
#else
    g_mock_minutes_of_day = (uint16_t)(minutes_of_day % 1440U);
#endif
}

void bsp_rtc_mock_set_valid(bool valid)
{
#if defined(USE_STDPERIPH_DRIVER)
    (void)valid;
#else
    g_mock_valid = valid;
#endif
}
