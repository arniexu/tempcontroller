#include "bsp_ml307r.h"

#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "bsp_uart.h"

#define ML307R_LINE_BUF_SIZE       (128U)

static bool send_crlf(void)
{
    bsp_uart_write("\r\n");
    return true;
}

void bsp_ml307r_init(void)
{
    bsp_uart_init();
}

bool bsp_ml307r_send_raw(const char *text)
{
    if (text == NULL)
    {
        return false;
    }

    bsp_uart_write(text);
    return true;
}

bool bsp_ml307r_send_cmd(const char *cmd)
{
    if (!bsp_ml307r_send_raw(cmd))
    {
        return false;
    }

    return send_crlf();
}

bool bsp_ml307r_read_line(char *buf, unsigned int buf_size)
{
    return bsp_uart_read_line(buf, buf_size);
}

bool bsp_ml307r_wait_for(const char *token, unsigned int max_polls)
{
    char line[ML307R_LINE_BUF_SIZE];
    unsigned int polls = 0U;

    if ((token == NULL) || (token[0] == '\0'))
    {
        return false;
    }

    while (polls < max_polls)
    {
        if (bsp_ml307r_read_line(line, sizeof(line)))
        {
            if (strstr(line, token) != NULL)
            {
                return true;
            }
        }
        polls++;
    }

    return false;
}

bool bsp_ml307r_check_alive(void)
{
    if (!bsp_ml307r_send_cmd("AT"))
    {
        return false;
    }

    return bsp_ml307r_wait_for("OK", 50000U);
}

bool bsp_ml307r_disable_echo(void)
{
    if (!bsp_ml307r_send_cmd("ATE0"))
    {
        return false;
    }

    return bsp_ml307r_wait_for("OK", 50000U);
}

bool bsp_ml307r_query_csq(int *rssi, int *ber)
{
    char line[ML307R_LINE_BUF_SIZE];
    unsigned int polls = 0U;

    if ((rssi == NULL) || (ber == NULL))
    {
        return false;
    }

    if (!bsp_ml307r_send_cmd("AT+CSQ"))
    {
        return false;
    }

    while (polls < 50000U)
    {
        if (bsp_ml307r_read_line(line, sizeof(line)))
        {
            if (sscanf(line, "+CSQ: %d,%d", rssi, ber) == 2)
            {
                return true;
            }
            if (strstr(line, "ERROR") != NULL)
            {
                return false;
            }
        }
        polls++;
    }

    return false;
}
