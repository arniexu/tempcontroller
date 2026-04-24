#include "bsp_uart.h"

#if defined(USE_STDPERIPH_DRIVER)
#include <string.h>

#include "app_config.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_rcc.h"
#include "stm32f10x_usart.h"

#define BSP_UART_TX_TIMEOUT_LOOPS   (200000U)

static char g_line_buf[128];
static unsigned int g_line_len = 0U;
static bool g_line_ready = false;

static void uart_poll_rx(void)
{
    while (USART_GetFlagStatus(USART1, USART_FLAG_RXNE) == SET)
    {
        char ch = (char)(USART_ReceiveData(USART1) & 0xFFU);

        if (g_line_ready)
        {
            continue;
        }

        if ((ch == '\r') || (ch == '\n'))
        {
            if (g_line_len > 0U)
            {
                g_line_buf[g_line_len] = '\0';
                g_line_ready = true;
            }
        }
        else if (g_line_len < (sizeof(g_line_buf) - 1U))
        {
            g_line_buf[g_line_len++] = ch;
        }
    }
}
#endif

void bsp_uart_init(void)
{
#if defined(USE_STDPERIPH_DRIVER)
    GPIO_InitTypeDef gpio;
    USART_InitTypeDef usart;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_USART1, ENABLE);

    gpio.GPIO_Pin = GPIO_Pin_9;
    gpio.GPIO_Speed = GPIO_Speed_50MHz;
    gpio.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_Init(GPIOA, &gpio);

    gpio.GPIO_Pin = GPIO_Pin_10;
    gpio.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(GPIOA, &gpio);

    USART_StructInit(&usart);
    usart.USART_BaudRate = APP_UART_BAUDRATE;
    usart.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
    USART_Init(USART1, &usart);
    USART_Cmd(USART1, ENABLE);

    g_line_len = 0U;
    g_line_ready = false;
    g_line_buf[0] = '\0';
#endif
}

bool bsp_uart_read_line(char *buf, unsigned int buf_size)
{
#if defined(USE_STDPERIPH_DRIVER)
    unsigned int n;

    if ((buf == 0) || (buf_size == 0U))
    {
        return false;
    }

    uart_poll_rx();
    if (!g_line_ready)
    {
        return false;
    }

    n = g_line_len;
    if (n >= buf_size)
    {
        n = buf_size - 1U;
    }

    memcpy(buf, g_line_buf, n);
    buf[n] = '\0';
    g_line_len = 0U;
    g_line_ready = false;
    g_line_buf[0] = '\0';
    return true;
#else
    (void)buf;
    (void)buf_size;
    return false;
#endif
}

void bsp_uart_write(const char *text)
{
#if defined(USE_STDPERIPH_DRIVER)
    uint32_t timeout;

    if (text == 0)
    {
        return;
    }

    while (*text != '\0')
    {
        USART_SendData(USART1, (uint16_t)(unsigned char)(*text));

        timeout = BSP_UART_TX_TIMEOUT_LOOPS;
        while ((USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET) && (timeout > 0U))
        {
            timeout--;
        }

        if (timeout == 0U)
        {
            break;
        }

        text++;
    }
#else
    (void)text;
#endif
}
