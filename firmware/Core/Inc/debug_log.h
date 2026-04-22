#ifndef DEBUG_LOG_H
#define DEBUG_LOG_H

void debug_log_init(void);
void debug_log_info(const char *tag, const char *fmt, ...);
void debug_log_warn(const char *tag, const char *fmt, ...);
void debug_log_error(const char *tag, const char *fmt, ...);

#endif
