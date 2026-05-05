#include "ui_key_input.h"

typedef struct
{
    int prev_pressed;
    unsigned int hold_ticks;
    int long_fired;
    int short_fired;
    unsigned int repeat_ticks;
} ui_key_track_t;

typedef struct
{
    ui_key_event_t queue[8];
    unsigned int q_head;
    unsigned int q_tail;
    ui_key_track_t key_track[HW_KEY_COUNT];
    ui_key_input_read_fn_t read_fn;
    void *read_ctx;
} ui_key_input_ctx_t;

static ui_key_input_ctx_t g_input;

static int ui_key_input_default_read(hw_key_id_t key, void *ctx)
{
    (void)ctx;
    return hw_key_get_state(key) ? 1 : 0;
}

static int queue_push(ui_key_event_t key)
{
    unsigned int next = (g_input.q_head + 1U) % (unsigned int)(sizeof(g_input.queue) / sizeof(g_input.queue[0]));

    if (next == g_input.q_tail)
    {
        g_input.q_tail = (g_input.q_tail + 1U) % (unsigned int)(sizeof(g_input.queue) / sizeof(g_input.queue[0]));
    }

    g_input.queue[g_input.q_head] = key;
    g_input.q_head = next;
    return 1;
}

void ui_key_input_init(void)
{
    unsigned int i;

    g_input.q_head = 0U;
    g_input.q_tail = 0U;
    g_input.read_fn = ui_key_input_default_read;
    g_input.read_ctx = 0;

    for (i = 0U; i < (unsigned int)HW_KEY_COUNT; ++i)
    {
        g_input.key_track[i].prev_pressed = 0;
        g_input.key_track[i].hold_ticks = 0U;
        g_input.key_track[i].long_fired = 0;
        g_input.key_track[i].short_fired = 0;
        g_input.key_track[i].repeat_ticks = 0U;
    }
}

void ui_key_input_set_read_fn(ui_key_input_read_fn_t fn, void *ctx)
{
    g_input.read_fn = (fn != 0) ? fn : ui_key_input_default_read;
    g_input.read_ctx = ctx;
}

int ui_key_input_pop_event(ui_key_event_t *key)
{
    if ((key == 0) || (g_input.q_tail == g_input.q_head))
    {
        return 0;
    }

    *key = g_input.queue[g_input.q_tail];
    g_input.q_tail = (g_input.q_tail + 1U) % (unsigned int)(sizeof(g_input.queue) / sizeof(g_input.queue[0]));
    return 1;
}

void ui_key_input_inject_event(ui_key_event_t key)
{
    (void)queue_push(key);
}

void ui_key_input_poll_100ms(void)
{
    unsigned int i;

    for (i = 0U; i < (unsigned int)HW_KEY_COUNT; ++i)
    {
        int pressed = g_input.read_fn((hw_key_id_t)i, g_input.read_ctx);

        if (pressed)
        {
            if (!g_input.key_track[i].prev_pressed)
            {
                g_input.key_track[i].prev_pressed = 1;
                g_input.key_track[i].hold_ticks = 1U;
                g_input.key_track[i].long_fired = 0;
                g_input.key_track[i].short_fired = 0;
                g_input.key_track[i].repeat_ticks = 0U;
            }
            else
            {
                g_input.key_track[i].hold_ticks++;
            }

            if ((!g_input.key_track[i].short_fired) && (g_input.key_track[i].hold_ticks >= 3U))
            {
                if (i == (unsigned int)HW_KEY_SET)
                {
                    (void)queue_push(UI_KEY_SET);
                    g_input.key_track[i].short_fired = 1;
                }
                else if (i == (unsigned int)HW_KEY_UP)
                {
                    (void)queue_push(UI_KEY_UP);
                    g_input.key_track[i].short_fired = 1;
                }
                else if (i == (unsigned int)HW_KEY_DOWN)
                {
                    (void)queue_push(UI_KEY_DOWN);
                    g_input.key_track[i].short_fired = 1;
                }
            }

            if (i == (unsigned int)HW_KEY_SET)
            {
                if ((!g_input.key_track[i].long_fired) && (g_input.key_track[i].hold_ticks >= 10U))
                {
                    (void)queue_push(UI_KEY_SET_LONG);
                    g_input.key_track[i].long_fired = 1;
                }
            }
            else if (i == (unsigned int)HW_KEY_DOWN)
            {
                if ((!g_input.key_track[i].long_fired) && (g_input.key_track[i].hold_ticks >= 10U))
                {
                    (void)queue_push(UI_KEY_BACK);
                    g_input.key_track[i].long_fired = 1;
                }

                if ((!g_input.key_track[i].long_fired) && (g_input.key_track[i].hold_ticks >= 5U))
                {
                    g_input.key_track[i].repeat_ticks++;
                    if (g_input.key_track[i].repeat_ticks >= 2U)
                    {
                        (void)queue_push(UI_KEY_DOWN_REPEAT);
                        g_input.key_track[i].repeat_ticks = 0U;
                    }
                }
            }
            else if (i == (unsigned int)HW_KEY_UP)
            {
                if (g_input.key_track[i].hold_ticks >= 5U)
                {
                    g_input.key_track[i].repeat_ticks++;
                    if (g_input.key_track[i].repeat_ticks >= 2U)
                    {
                        (void)queue_push(UI_KEY_UP_REPEAT);
                        g_input.key_track[i].repeat_ticks = 0U;
                    }
                }
            }
        }
        else
        {
            if (g_input.key_track[i].prev_pressed)
            {
                if (i == (unsigned int)HW_KEY_SET)
                {
                    if ((!g_input.key_track[i].long_fired) && (!g_input.key_track[i].short_fired))
                    {
                        (void)queue_push(UI_KEY_SET);
                    }
                }
                else if (i == (unsigned int)HW_KEY_UP)
                {
                    if (!g_input.key_track[i].short_fired)
                    {
                        (void)queue_push(UI_KEY_UP);
                    }
                }
                else if (i == (unsigned int)HW_KEY_DOWN)
                {
                    if ((!g_input.key_track[i].long_fired) && (!g_input.key_track[i].short_fired))
                    {
                        (void)queue_push(UI_KEY_DOWN);
                    }
                }
                else if (i == (unsigned int)HW_KEY_BACK)
                {
                    (void)queue_push(UI_KEY_BACK);
                }

                g_input.key_track[i].prev_pressed = 0;
                g_input.key_track[i].hold_ticks = 0U;
                g_input.key_track[i].long_fired = 0;
                g_input.key_track[i].short_fired = 0;
                g_input.key_track[i].repeat_ticks = 0U;
            }
        }
    }
}
