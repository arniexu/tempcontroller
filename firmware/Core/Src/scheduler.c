#include "scheduler.h"

static volatile uint32_t g_ms = 0U;
static uint32_t g_last_key = 0U;
static uint32_t g_last_ui = 0U;
static uint32_t g_last_control = 0U;

void scheduler_init(void)
{
    g_ms = 0U;
    g_last_key = 0U;
    g_last_ui = 0U;
    g_last_control = 0U;
}

void scheduler_tick_1ms(void)
{
    g_ms++;
}

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

    if ((now - g_last_key) >= 100U)
    {
        g_last_key = now;
        flags->task_key_100ms = true;
    }

    if ((now - g_last_ui) >= 200U)
    {
        g_last_ui = now;
        flags->task_ui_200ms = true;
    }

    if ((now - g_last_control) >= 1000U)
    {
        g_last_control = now;
        flags->task_control_1s = true;
    }
}

uint32_t scheduler_now_ms(void)
{
    return g_ms;
}
