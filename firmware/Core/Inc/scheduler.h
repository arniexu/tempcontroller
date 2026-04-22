#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <stdbool.h>
#include <stdint.h>

typedef struct
{
    bool task_key_100ms;
    bool task_ui_200ms;
    bool task_control_1s;
} scheduler_flags_t;

void scheduler_init(void);
void scheduler_tick_1ms(void);
void scheduler_poll(scheduler_flags_t *flags);
uint32_t scheduler_now_ms(void);

#endif
