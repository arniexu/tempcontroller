#include "scheduler.h"
#include "app_config.h"

#if defined(USE_HAL_DRIVER)
#include "stm32f1xx.h"
#include "stm32f1xx_hal.h"
#endif

static volatile uint32_t g_ms = 0U;
static uint32_t g_last_key = 0U;
static uint32_t g_last_ui = 0U;
static uint32_t g_last_control = 0U;

#define SCHED_KEY_PERIOD_MS     ((APP_TASK_KEY_PERIOD_MS == 0U) ? 1U : APP_TASK_KEY_PERIOD_MS)
#define SCHED_UI_PERIOD_MS      ((APP_TASK_UI_PERIOD_MS == 0U) ? 1U : APP_TASK_UI_PERIOD_MS)
#define SCHED_CONTROL_PERIOD_MS ((APP_TASK_CONTROL_PERIOD_MS == 0U) ? 1U : APP_TASK_CONTROL_PERIOD_MS)

void scheduler_init(void)
{
    g_ms = 0U;
    g_last_key = 0U;
    g_last_ui = 0U;
    g_last_control = 0U;

#if defined(USE_HAL_DRIVER)
    (void)SysTick_Config(SystemCoreClock / 1000U);
#endif
}

//@TODO: why not use the systick timer  
void scheduler_tick_1ms(void)
{
    g_ms++;
}

#if defined(USE_HAL_DRIVER)
void SysTick_Handler(void)
{
    HAL_IncTick();
    scheduler_tick_1ms();
}
#endif

void scheduler_poll(scheduler_flags_t *flags)
{
    uint32_t now;

    if (flags == 0)
    {
        return;
    }

    flags->task_key_100ms = false;
    flags->task_ui_200ms = false;
    flags->task_control_1s = false;

    now = g_ms;

    if ((now - g_last_key) >= SCHED_KEY_PERIOD_MS)
    {
        g_last_key = now;
        flags->task_key_100ms = true;
    }

    if ((now - g_last_ui) >= SCHED_UI_PERIOD_MS)
    {
        g_last_ui = now;
        flags->task_ui_200ms = true;
    }

    if ((now - g_last_control) >= SCHED_CONTROL_PERIOD_MS)
    {
        g_last_control = now;
        flags->task_control_1s = true;
    }
}

uint32_t scheduler_now_ms(void)
{
    return g_ms;
}
