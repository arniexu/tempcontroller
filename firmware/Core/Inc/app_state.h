#ifndef APP_STATE_H
#define APP_STATE_H

typedef enum
{
    APP_MODE_IDLE = 0,
    APP_MODE_HEATING,
    APP_MODE_SCHEDULED,
    APP_MODE_ALARM,
    APP_MODE_EXPORT
} app_mode_t;

#endif
