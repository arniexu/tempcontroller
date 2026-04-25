#include "bsp_uart.h"

#if defined(USE_STDPERIPH_DRIVER)
#include <string.h>

#include "app_config.h"
#include "stm32f10x_gpio.h"
#include "misc.h"
#include "stm32f10x_rcc.h"
#include "stm32f10x_usart.h"

#define BSP_UART_TX_TIMEOUT_LOOPS   (200000U)
#define BSP_UART_TX_BUF_SIZE         (256U)

static char g_line_buf[128];
static volatile unsigned int g_line_len = 0U;
static volatile bool g_line_ready = false;
static volatile unsigned int g_tx_head = 0U;
static volatile unsigned int g_tx_tail = 0U;
static volatile char g_tx_buf[BSP_UART_TX_BUF_SIZE];

static void uart_rx_char(char ch)
{
    if (g_line_ready)
    {
        return;
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

void USART1_IRQHandler(void)
{
    if (USART_GetITStatus(USART1, USART_IT_RXNE) != RESET)
    {
        uart_rx_char((char)(USART_ReceiveData(USART1) & 0xFFU));
    }

    if (USART_GetITStatus(USART1, USART_IT_TXE) != RESET)
    {
        if (g_tx_tail != g_tx_head)
        {
            USART_SendData(USART1, (uint16_t)(uint8_t)g_tx_buf[g_tx_tail]);
            g_tx_tail = (g_tx_tail + 1U) % BSP_UART_TX_BUF_SIZE;
        }
        else
        {
            USART_ITConfig(USART1, USART_IT_TXE, DISABLE);
        }
    }
}
#endif

void bsp_uart_init(void)
{
#if defined(USE_STDPERIPH_DRIVER)
    GPIO_InitTypeDef gpio;
    USART_InitTypeDef usart;
    NVIC_InitTypeDef nvic;

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
    USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);

    nvic.NVIC_IRQChannel = USART1_IRQn;
    nvic.NVIC_IRQChannelPreemptionPriority = 2U;
    nvic.NVIC_IRQChannelSubPriority = 0U;
    nvic.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&nvic);

    g_line_len = 0U;
    g_line_ready = false;
    g_line_buf[0] = '\0';
    g_tx_head = 0U;
    g_tx_tail = 0U;
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
        timeout = BSP_UART_TX_TIMEOUT_LOOPS;
        while (timeout > 0U)
        {
            unsigned int head;
            unsigned int next;

            __disable_irq();
            head = g_tx_head;
            next = (head + 1U) % BSP_UART_TX_BUF_SIZE;
            if (next != g_tx_tail)
            {
                g_tx_buf[head] = *text;
                g_tx_head = next;
                USART_ITConfig(USART1, USART_IT_TXE, ENABLE);
                __enable_irq();
                break;
            }
            __enable_irq();
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
