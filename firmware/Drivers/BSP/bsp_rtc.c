#include "bsp_rtc.h"

#if defined(USE_HAL_DRIVER)
#include "stm32f1xx_hal.h"

static bool g_rtc_ready = false;
static uint16_t g_mock_minutes_of_day = 0U;
static bool g_mock_valid = true;
static uint32_t g_base_tick_ms = 0U;
#endif

#if !defined(USE_HAL_DRIVER)
static uint16_t g_mock_minutes_of_day = 0U;
static bool g_mock_valid = true;
#endif

void bsp_rtc_init(void)
{
#if defined(USE_HAL_DRIVER)
    g_base_tick_ms = HAL_GetTick();
    g_rtc_ready = true;
    g_mock_valid = true;
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

#if defined(USE_HAL_DRIVER)
    if (!g_rtc_ready)
    {
        return false;
    }

    {
        uint32_t elapsed_ms = HAL_GetTick() - g_base_tick_ms;
        uint32_t minute_of_day = ((elapsed_ms / 60000U) + (uint32_t)g_mock_minutes_of_day) % 1440U;
        *minutes_of_day = (uint16_t)minute_of_day;
        if (!g_mock_valid)
        {
            return false;
        }
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
#if defined(USE_HAL_DRIVER)
    g_mock_minutes_of_day = (uint16_t)(minutes_of_day % 1440U);
    g_base_tick_ms = HAL_GetTick();
#else
    g_mock_minutes_of_day = (uint16_t)(minutes_of_day % 1440U);
#endif
}

void bsp_rtc_mock_set_valid(bool valid)
{
#if defined(USE_HAL_DRIVER)
    g_mock_valid = valid;
#else
    g_mock_valid = valid;
#endif
}

