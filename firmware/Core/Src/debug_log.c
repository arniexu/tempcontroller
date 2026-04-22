#include "debug_log.h"

#include <stdarg.h>
#include <stdio.h>

#include "app_config.h"
#include "bsp_uart.h"
#include "scheduler.h"

static void debug_log_v(const char *level, const char *tag, const char *fmt, va_list ap)
{
#if (APP_DEBUG_LOG_ENABLE == 1U)
    char msg[160];
    char line[220];
    uint32_t ms;

    (void)vsnprintf(msg, sizeof(msg), fmt, ap);
    ms = scheduler_now_ms();
    (void)snprintf(line, sizeof(line), "[%lu][%s][%s] %s\r\n", (unsigned long)ms, level, tag, msg);
    bsp_uart_write(line);
#else
    (void)level;
    (void)tag;
    (void)fmt;
    (void)ap;
#endif
}

void debug_log_init(void)
{
}

void debug_log_info(const char *tag, const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    debug_log_v("I", tag, fmt, ap);
    va_end(ap);
}

void debug_log_warn(const char *tag, const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    debug_log_v("W", tag, fmt, ap);
    va_end(ap);
}

void debug_log_error(const char *tag, const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    debug_log_v("E", tag, fmt, ap);
    va_end(ap);
}
