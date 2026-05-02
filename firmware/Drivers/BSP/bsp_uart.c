#include "bsp_uart.h"

#if defined(USE_HAL_DRIVER)
#include <string.h>

#include "app_config.h"
#include "stm32f1xx_hal.h"

#define BSP_UART_TX_TIMEOUT_LOOPS   (200000U)
#define BSP_UART_TX_BUF_SIZE         (256U)

static UART_HandleTypeDef g_huart1;
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
    if ((USART1->SR & USART_SR_RXNE) != 0U)
    {
        uart_rx_char((char)(USART1->DR & 0xFFU));
    }

    if (((USART1->SR & USART_SR_TXE) != 0U) && ((USART1->CR1 & USART_CR1_TXEIE) != 0U))
    {
        if (g_tx_tail != g_tx_head)
        {
            USART1->DR = (uint16_t)(uint8_t)g_tx_buf[g_tx_tail];
            g_tx_tail = (g_tx_tail + 1U) % BSP_UART_TX_BUF_SIZE;
        }
        else
        {
            CLEAR_BIT(USART1->CR1, USART_CR1_TXEIE);
        }
    }
}
#endif

void bsp_uart_init(void)
{
#if defined(USE_HAL_DRIVER)
    GPIO_InitTypeDef gpio = {0};

    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_USART1_CLK_ENABLE();

    gpio.Pin = GPIO_PIN_9;
    gpio.Mode = GPIO_MODE_AF_PP;
    gpio.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOA, &gpio);

    gpio.Pin = GPIO_PIN_10;
    gpio.Mode = GPIO_MODE_INPUT;
    gpio.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &gpio);

    g_huart1.Instance = USART1;
    g_huart1.Init.BaudRate = APP_UART_BAUDRATE;
    g_huart1.Init.WordLength = UART_WORDLENGTH_8B;
    g_huart1.Init.StopBits = UART_STOPBITS_1;
    g_huart1.Init.Parity = UART_PARITY_NONE;
    g_huart1.Init.Mode = UART_MODE_TX_RX;
    g_huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    g_huart1.Init.OverSampling = UART_OVERSAMPLING_16;
    (void)HAL_UART_Init(&g_huart1);

    HAL_NVIC_SetPriority(USART1_IRQn, 2U, 0U);
    HAL_NVIC_EnableIRQ(USART1_IRQn);

    SET_BIT(USART1->CR1, USART_CR1_RXNEIE);

    g_line_len = 0U;
    g_line_ready = false;
    g_line_buf[0] = '\0';
    g_tx_head = 0U;
    g_tx_tail = 0U;
#endif
}

bool bsp_uart_read_line(char *buf, unsigned int buf_size)
{
#if defined(USE_HAL_DRIVER)
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
#if defined(USE_HAL_DRIVER)
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
                SET_BIT(USART1->CR1, USART_CR1_TXEIE);
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

