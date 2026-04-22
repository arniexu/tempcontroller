#include "bsp_uart.h"

#include <string.h>

static char g_in[32][96];
static unsigned int g_in_head = 0U;
static unsigned int g_in_tail = 0U;
static char g_out[8192];
static unsigned int g_out_len = 0U;

void bsp_uart_test_reset(void)
{
    g_in_head = 0U;
    g_in_tail = 0U;
    g_out_len = 0U;
    g_out[0] = '\0';
}

void bsp_uart_test_feed(const char *line)
{
    unsigned int next = (g_in_head + 1U) % 32U;
    if (next == g_in_tail)
    {
        return;
    }
    strncpy(g_in[g_in_head], line, sizeof(g_in[g_in_head]) - 1U);
    g_in[g_in_head][sizeof(g_in[g_in_head]) - 1U] = '\0';
    g_in_head = next;
}

const char *bsp_uart_test_output(void)
{
    return g_out;
}

void bsp_uart_init(void)
{
}

bool bsp_uart_read_line(char *buf, unsigned int buf_size)
{
    unsigned int len;
    if ((buf == 0) || (buf_size == 0U) || (g_in_tail == g_in_head))
    {
        return false;
    }

    len = (unsigned int)strlen(g_in[g_in_tail]);
    if (len >= buf_size)
    {
        len = buf_size - 1U;
    }

    memcpy(buf, g_in[g_in_tail], len);
    buf[len] = '\0';
    g_in_tail = (g_in_tail + 1U) % 32U;
    return true;
}

void bsp_uart_write(const char *text)
{
    unsigned int left;
    unsigned int n;
    if (text == 0)
    {
        return;
    }

    left = (unsigned int)(sizeof(g_out) - 1U - g_out_len);
    if (left == 0U)
    {
        return;
    }

    n = (unsigned int)strlen(text);
    if (n > left)
    {
        n = left;
    }

    memcpy(&g_out[g_out_len], text, n);
    g_out_len += n;
    g_out[g_out_len] = '\0';
}
