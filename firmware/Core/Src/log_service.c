#include "log_service.h"

#define LOG_CAPACITY    (512U)

static log_record_t g_logs[LOG_CAPACITY];
static unsigned int g_head = 0U;
static unsigned int g_count = 0U;
static unsigned int g_seq = 0U;

void log_service_init(void)
{
    g_head = 0U;
    g_count = 0U;
    g_seq = 0U;
}

void log_service_push(const temp_snapshot_t *temp, float set_temp, float pid_out, int heater_on, int alarm_on)
{
    log_record_t *slot;

    if (temp == 0)
    {
        return;
    }

    slot = &g_logs[g_head];
    slot->index = g_seq++;
    slot->t1 = temp->t1;
    slot->t2 = temp->t2;
    slot->t3 = temp->t3;
    slot->t_avg = temp->t_ctrl;
    slot->set_temp = set_temp;
    slot->pid_out = pid_out;
    slot->heater_on = heater_on;
    slot->alarm_on = alarm_on;

    g_head = (g_head + 1U) % LOG_CAPACITY;
    if (g_count < LOG_CAPACITY)
    {
        g_count++;
    }
}

unsigned int log_service_count(void)
{
    return g_count;
}

bool log_service_get(unsigned int offset_oldest, log_record_t *out)
{
    unsigned int oldest;
    unsigned int pos;

    if ((out == 0) || (offset_oldest >= g_count))
    {
        return false;
    }

    oldest = (g_head + LOG_CAPACITY - g_count) % LOG_CAPACITY;
    pos = (oldest + offset_oldest) % LOG_CAPACITY;
    *out = g_logs[pos];
    return true;
}

void log_service_clear(void)
{
    g_head = 0U;
    g_count = 0U;
}
