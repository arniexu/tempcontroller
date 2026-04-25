#include "bsp_eeprom.h"

#include "app_config.h"

#include <string.h>

#if defined(USE_STDPERIPH_DRIVER)
#include "stm32f10x_dma.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_i2c.h"
#include "misc.h"
#include "stm32f10x_rcc.h"
#endif

#ifndef APP_EEPROM_TOTAL_SIZE_BYTES
#define APP_EEPROM_TOTAL_SIZE_BYTES    (256U)
#endif

#ifndef APP_EEPROM_PAGE_SIZE_BYTES
#define APP_EEPROM_PAGE_SIZE_BYTES     (8U)
#endif

#ifndef APP_EEPROM_I2C_ADDR_7BIT
#define APP_EEPROM_I2C_ADDR_7BIT       (0x50U)
#endif

#ifndef APP_EEPROM_I2C_SPEED_HZ
#define APP_EEPROM_I2C_SPEED_HZ        (100000U)
#endif

#if (APP_EEPROM_TOTAL_SIZE_BYTES == 0U)
#error "APP_EEPROM_TOTAL_SIZE_BYTES must be > 0"
#endif

#if (APP_EEPROM_PAGE_SIZE_BYTES == 0U)
#error "APP_EEPROM_PAGE_SIZE_BYTES must be > 0"
#endif

/* This driver currently uses single-byte EEPROM word addressing. */
#if (APP_EEPROM_TOTAL_SIZE_BYTES > 256U)
#error "APP_EEPROM_TOTAL_SIZE_BYTES must be <= 256 for this driver"
#endif

#if (APP_EEPROM_I2C_SPEED_HZ == 0U)
#error "APP_EEPROM_I2C_SPEED_HZ must be > 0"
#endif

#if defined(USE_STDPERIPH_DRIVER)

#define EEPROM_I2C                      I2C1
#define EEPROM_GPIO_PORT                GPIOB
#define EEPROM_SCL_PIN                  GPIO_Pin_6
#define EEPROM_SDA_PIN                  GPIO_Pin_7
#define EEPROM_I2C_DMA_CLK              RCC_AHBPeriph_DMA1
#define EEPROM_I2C_DMA_TX_CHANNEL       DMA1_Channel6
#define EEPROM_I2C_DMA_TX_IRQn          DMA1_Channel6_IRQn
#define EEPROM_I2C_DMA_TX_TC_FLAG       DMA1_FLAG_TC6
#define EEPROM_I2C_DMA_TX_TE_FLAG       DMA1_FLAG_TE6
#define EEPROM_I2C_DMA_TX_GL_FLAG       DMA1_FLAG_GL6
#define EEPROM_I2C_ADDR_8BIT            ((uint8_t)(APP_EEPROM_I2C_ADDR_7BIT << 1U))
#define EEPROM_TIMEOUT                  (20000U)
#define EEPROM_READY_RETRY              (200U)
#define EEPROM_ASYNC_MAX_LEN            (128U)

static volatile uint32_t g_i2c1_evt_count = 0U;
static volatile uint32_t g_i2c1_err_count = 0U;
static volatile int g_eeprom_inited = 0;
static volatile int g_async_busy = 0;
static volatile int g_async_status = 1;
static uint16_t g_async_addr = 0U;
static uint16_t g_async_len = 0U;
static uint16_t g_async_off = 0U;
static uint16_t g_async_dma_chunk = 0U;
static volatile int g_async_dma_active = 0;
static uint8_t g_async_buf[EEPROM_ASYNC_MAX_LEN];

static uint32_t irq_lock(void)
{
    uint32_t primask = __get_PRIMASK();
    __disable_irq();
    return primask;
}

static void irq_unlock(uint32_t primask)
{
    if (primask == 0U)
    {
        __enable_irq();
    }
}

static int i2c_wait_event(uint32_t event)
{
    uint32_t timeout = EEPROM_TIMEOUT;
    uint32_t err_start = g_i2c1_err_count;
    uint32_t evt_seen = g_i2c1_evt_count;

    while (I2C_CheckEvent(EEPROM_I2C, event) == ERROR)
    {
        if (g_i2c1_err_count != err_start)
        {
            return 0;
        }

        if (timeout == 0U)
        {
            return 0;
        }
        --timeout;

        if (g_i2c1_evt_count == evt_seen)
        {
            if (__get_PRIMASK() == 0U)
            {
                __WFI();
            }
            else
            {
                __NOP();
            }
        }
        evt_seen = g_i2c1_evt_count;
    }
    return 1;
}

static int i2c_wait_flag_clear(uint32_t flag)
{
    uint32_t timeout = EEPROM_TIMEOUT;
    uint32_t err_start = g_i2c1_err_count;
    uint32_t evt_seen = g_i2c1_evt_count;

    while (I2C_GetFlagStatus(EEPROM_I2C, flag) != RESET)
    {
        if (g_i2c1_err_count != err_start)
        {
            return 0;
        }

        if (timeout == 0U)
        {
            return 0;
        }
        --timeout;

        if (g_i2c1_evt_count == evt_seen)
        {
            if (__get_PRIMASK() == 0U)
            {
                __WFI();
            }
            else
            {
                __NOP();
            }
        }
        evt_seen = g_i2c1_evt_count;
    }
    return 1;
}

static int eeprom_wait_ready(void)
{
    uint32_t retry;

    for (retry = 0U; retry < EEPROM_READY_RETRY; ++retry)
    {
        if (!i2c_wait_flag_clear(I2C_FLAG_BUSY))
        {
            return 0;
        }

        I2C_GenerateSTART(EEPROM_I2C, ENABLE);
        if (!i2c_wait_event(I2C_EVENT_MASTER_MODE_SELECT))
        {
            I2C_GenerateSTOP(EEPROM_I2C, ENABLE);
            return 0;
        }

        I2C_Send7bitAddress(EEPROM_I2C, EEPROM_I2C_ADDR_8BIT, I2C_Direction_Transmitter);
        if (i2c_wait_event(I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED))
        {
            I2C_GenerateSTOP(EEPROM_I2C, ENABLE);
            return 1;
        }

        I2C_GenerateSTOP(EEPROM_I2C, ENABLE);
    }

    return 0;
}

static int eeprom_write_chunk(uint16_t cur_addr, const uint8_t *buf, uint16_t chunk)
{
    uint16_t i;

    if ((buf == 0) || (chunk == 0U))
    {
        return 0;
    }

    I2C_GenerateSTART(EEPROM_I2C, ENABLE);
    if (!i2c_wait_event(I2C_EVENT_MASTER_MODE_SELECT))
    {
        I2C_GenerateSTOP(EEPROM_I2C, ENABLE);
        return 0;
    }

    I2C_Send7bitAddress(EEPROM_I2C, EEPROM_I2C_ADDR_8BIT, I2C_Direction_Transmitter);
    if (!i2c_wait_event(I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED))
    {
        I2C_GenerateSTOP(EEPROM_I2C, ENABLE);
        return 0;
    }

    I2C_SendData(EEPROM_I2C, (uint8_t)cur_addr);
    if (!i2c_wait_event(I2C_EVENT_MASTER_BYTE_TRANSMITTED))
    {
        I2C_GenerateSTOP(EEPROM_I2C, ENABLE);
        return 0;
    }

    for (i = 0U; i < chunk; ++i)
    {
        I2C_SendData(EEPROM_I2C, buf[i]);
        if (!i2c_wait_event(I2C_EVENT_MASTER_BYTE_TRANSMITTED))
        {
            I2C_GenerateSTOP(EEPROM_I2C, ENABLE);
            return 0;
        }
    }

    I2C_GenerateSTOP(EEPROM_I2C, ENABLE);
    return 1;
}

static int eeprom_write_chunk_dma_start(uint16_t cur_addr, const uint8_t *buf, uint16_t chunk)
{
    DMA_InitTypeDef dma;

    if ((buf == 0) || (chunk == 0U))
    {
        return 0;
    }

    I2C_GenerateSTART(EEPROM_I2C, ENABLE);
    if (!i2c_wait_event(I2C_EVENT_MASTER_MODE_SELECT))
    {
        I2C_GenerateSTOP(EEPROM_I2C, ENABLE);
        return 0;
    }

    I2C_Send7bitAddress(EEPROM_I2C, EEPROM_I2C_ADDR_8BIT, I2C_Direction_Transmitter);
    if (!i2c_wait_event(I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED))
    {
        I2C_GenerateSTOP(EEPROM_I2C, ENABLE);
        return 0;
    }

    I2C_SendData(EEPROM_I2C, (uint8_t)cur_addr);
    if (!i2c_wait_event(I2C_EVENT_MASTER_BYTE_TRANSMITTED))
    {
        I2C_GenerateSTOP(EEPROM_I2C, ENABLE);
        return 0;
    }

    DMA_Cmd(EEPROM_I2C_DMA_TX_CHANNEL, DISABLE);
    DMA_ClearFlag(EEPROM_I2C_DMA_TX_GL_FLAG);

    dma.DMA_PeripheralBaseAddr = (uint32_t)&(EEPROM_I2C->DR);
    dma.DMA_MemoryBaseAddr = (uint32_t)buf;
    dma.DMA_DIR = DMA_DIR_PeripheralDST;
    dma.DMA_BufferSize = (uint32_t)chunk;
    dma.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
    dma.DMA_MemoryInc = DMA_MemoryInc_Enable;
    dma.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
    dma.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
    dma.DMA_Mode = DMA_Mode_Normal;
    dma.DMA_Priority = DMA_Priority_High;
    dma.DMA_M2M = DMA_M2M_Disable;
    DMA_Init(EEPROM_I2C_DMA_TX_CHANNEL, &dma);
    DMA_ITConfig(EEPROM_I2C_DMA_TX_CHANNEL, DMA_IT_TC | DMA_IT_TE, ENABLE);

    I2C_DMACmd(EEPROM_I2C, ENABLE);

    g_async_dma_chunk = chunk;
    g_async_dma_active = 1;
    DMA_Cmd(EEPROM_I2C_DMA_TX_CHANNEL, ENABLE);
    return 1;
}

void bsp_eeprom_init(void)
{
    GPIO_InitTypeDef gpio;
    I2C_InitTypeDef i2c;
    NVIC_InitTypeDef nvic;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB | RCC_APB2Periph_AFIO, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_I2C1, ENABLE);
    RCC_AHBPeriphClockCmd(EEPROM_I2C_DMA_CLK, ENABLE);

    gpio.GPIO_Pin = EEPROM_SCL_PIN | EEPROM_SDA_PIN;
    gpio.GPIO_Mode = GPIO_Mode_AF_OD;
    gpio.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(EEPROM_GPIO_PORT, &gpio);

    I2C_DeInit(EEPROM_I2C);
    i2c.I2C_Mode = I2C_Mode_I2C;
    i2c.I2C_DutyCycle = I2C_DutyCycle_2;
    i2c.I2C_OwnAddress1 = 0x00U;
    i2c.I2C_Ack = I2C_Ack_Enable;
    i2c.I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_7bit;
    i2c.I2C_ClockSpeed = APP_EEPROM_I2C_SPEED_HZ;
    I2C_Init(EEPROM_I2C, &i2c);
    I2C_Cmd(EEPROM_I2C, ENABLE);
    I2C_ITConfig(EEPROM_I2C, I2C_IT_EVT | I2C_IT_ERR, ENABLE);

    nvic.NVIC_IRQChannelPreemptionPriority = 3U;
    nvic.NVIC_IRQChannelSubPriority = 1U;
    nvic.NVIC_IRQChannelCmd = ENABLE;

    nvic.NVIC_IRQChannel = I2C1_EV_IRQn;
    NVIC_Init(&nvic);
    nvic.NVIC_IRQChannel = I2C1_ER_IRQn;
    NVIC_Init(&nvic);

    nvic.NVIC_IRQChannel = EEPROM_I2C_DMA_TX_IRQn;
    nvic.NVIC_IRQChannelSubPriority = 0U;
    NVIC_Init(&nvic);

    g_i2c1_evt_count = 0U;
    g_i2c1_err_count = 0U;
    g_async_busy = 0;
    g_async_status = 1;
    g_async_addr = 0U;
    g_async_len = 0U;
    g_async_off = 0U;
    g_async_dma_chunk = 0U;
    g_async_dma_active = 0;
    g_eeprom_inited = 1;
}

void DMA1_Channel6_IRQHandler(void)
{
    if (DMA_GetFlagStatus(EEPROM_I2C_DMA_TX_TE_FLAG) != RESET)
    {
        DMA_ClearFlag(EEPROM_I2C_DMA_TX_GL_FLAG);
        DMA_Cmd(EEPROM_I2C_DMA_TX_CHANNEL, DISABLE);
        I2C_DMACmd(EEPROM_I2C, DISABLE);
        I2C_GenerateSTOP(EEPROM_I2C, ENABLE);
        g_async_dma_active = 0;
        g_async_status = -1;
        g_async_busy = 0;
        return;
    }

    if (DMA_GetFlagStatus(EEPROM_I2C_DMA_TX_TC_FLAG) != RESET)
    {
        uint32_t timeout = EEPROM_TIMEOUT;

        DMA_ClearFlag(EEPROM_I2C_DMA_TX_GL_FLAG);
        DMA_Cmd(EEPROM_I2C_DMA_TX_CHANNEL, DISABLE);
        I2C_DMACmd(EEPROM_I2C, DISABLE);

        while ((I2C_GetFlagStatus(EEPROM_I2C, I2C_FLAG_BTF) == RESET) && (timeout > 0U))
        {
            timeout--;
        }

        I2C_GenerateSTOP(EEPROM_I2C, ENABLE);
        g_async_dma_active = 0;

        if (timeout == 0U)
        {
            g_async_status = -1;
            g_async_busy = 0;
            return;
        }

        g_async_off = (uint16_t)(g_async_off + g_async_dma_chunk);
        if (g_async_off >= g_async_len)
        {
            g_async_status = 1;
            g_async_busy = 0;
        }
    }
}

void I2C1_EV_IRQHandler(void)
{
    g_i2c1_evt_count++;
}

void I2C1_ER_IRQHandler(void)
{
    if (I2C_GetITStatus(EEPROM_I2C, I2C_IT_BERR) != RESET)
    {
        I2C_ClearITPendingBit(EEPROM_I2C, I2C_IT_BERR);
    }
    if (I2C_GetITStatus(EEPROM_I2C, I2C_IT_ARLO) != RESET)
    {
        I2C_ClearITPendingBit(EEPROM_I2C, I2C_IT_ARLO);
    }
    if (I2C_GetITStatus(EEPROM_I2C, I2C_IT_AF) != RESET)
    {
        I2C_ClearITPendingBit(EEPROM_I2C, I2C_IT_AF);
    }
    if (I2C_GetITStatus(EEPROM_I2C, I2C_IT_OVR) != RESET)
    {
        I2C_ClearITPendingBit(EEPROM_I2C, I2C_IT_OVR);
    }

    g_i2c1_err_count++;
}

void bsp_eeprom_mock_set_access_ok(int read_ok, int write_ok)
{
    (void)read_ok;
    (void)write_ok;
}

int bsp_eeprom_write(uint16_t addr, const uint8_t *buf, uint16_t len)
{
    uint16_t off = 0U;

    if (!g_eeprom_inited)
    {
        return 0;
    }

    if ((buf == 0) || (len == 0U))
    {
        return 0;
    }

    if (((uint32_t)addr + (uint32_t)len) > APP_EEPROM_TOTAL_SIZE_BYTES)
    {
        return 0;
    }

    while (off < len)
    {
        uint16_t cur_addr = (uint16_t)(addr + off);
        uint16_t page_remain = (uint16_t)(APP_EEPROM_PAGE_SIZE_BYTES - (cur_addr % APP_EEPROM_PAGE_SIZE_BYTES));
        uint16_t chunk = (uint16_t)(len - off);

        if (chunk > page_remain)
        {
            chunk = page_remain;
        }

        if (!eeprom_wait_ready())
        {
            return 0;
        }

        if (!eeprom_write_chunk(cur_addr, &buf[off], chunk))
        {
            return 0;
        }
        off = (uint16_t)(off + chunk);
    }

    return eeprom_wait_ready();
}

int bsp_eeprom_read(uint16_t addr, uint8_t *buf, uint16_t len)
{
    uint16_t i;

    if (!g_eeprom_inited)
    {
        return 0;
    }

    if ((buf == 0) || (len == 0U))
    {
        return 0;
    }

    if (((uint32_t)addr + (uint32_t)len) > APP_EEPROM_TOTAL_SIZE_BYTES)
    {
        return 0;
    }

    if (!eeprom_wait_ready())
    {
        return 0;
    }

    I2C_GenerateSTART(EEPROM_I2C, ENABLE);
    if (!i2c_wait_event(I2C_EVENT_MASTER_MODE_SELECT))
    {
        I2C_GenerateSTOP(EEPROM_I2C, ENABLE);
        return 0;
    }

    I2C_Send7bitAddress(EEPROM_I2C, EEPROM_I2C_ADDR_8BIT, I2C_Direction_Transmitter);
    if (!i2c_wait_event(I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED))
    {
        I2C_GenerateSTOP(EEPROM_I2C, ENABLE);
        return 0;
    }

    I2C_SendData(EEPROM_I2C, (uint8_t)addr);
    if (!i2c_wait_event(I2C_EVENT_MASTER_BYTE_TRANSMITTED))
    {
        I2C_GenerateSTOP(EEPROM_I2C, ENABLE);
        return 0;
    }

    I2C_GenerateSTART(EEPROM_I2C, ENABLE);
    if (!i2c_wait_event(I2C_EVENT_MASTER_MODE_SELECT))
    {
        I2C_GenerateSTOP(EEPROM_I2C, ENABLE);
        return 0;
    }

    I2C_Send7bitAddress(EEPROM_I2C, EEPROM_I2C_ADDR_8BIT, I2C_Direction_Receiver);
    if (!i2c_wait_event(I2C_EVENT_MASTER_RECEIVER_MODE_SELECTED))
    {
        I2C_GenerateSTOP(EEPROM_I2C, ENABLE);
        return 0;
    }

    for (i = 0U; i < len; ++i)
    {
        if (i == (uint16_t)(len - 1U))
        {
            I2C_AcknowledgeConfig(EEPROM_I2C, DISABLE);
            I2C_GenerateSTOP(EEPROM_I2C, ENABLE);
        }

        if (!i2c_wait_event(I2C_EVENT_MASTER_BYTE_RECEIVED))
        {
            I2C_AcknowledgeConfig(EEPROM_I2C, ENABLE);
            return 0;
        }
        buf[i] = I2C_ReceiveData(EEPROM_I2C);
    }

    I2C_AcknowledgeConfig(EEPROM_I2C, ENABLE);
    return 1;
}

int bsp_eeprom_write_async(uint16_t addr, const uint8_t *buf, uint16_t len)
{
    uint32_t irq_state;

    if (!g_eeprom_inited)
    {
        return 0;
    }

    if ((buf == 0) || (len == 0U) || (len > EEPROM_ASYNC_MAX_LEN))
    {
        return 0;
    }

    if (((uint32_t)addr + (uint32_t)len) > APP_EEPROM_TOTAL_SIZE_BYTES)
    {
        return 0;
    }

    irq_state = irq_lock();
    if (g_async_busy)
    {
        irq_unlock(irq_state);
        return 0;
    }

    memcpy(g_async_buf, buf, len);
    g_async_addr = addr;
    g_async_len = len;
    g_async_off = 0U;
    g_async_status = 0;
    g_async_busy = 1;
    irq_unlock(irq_state);
    return 1;
}

void bsp_eeprom_process(void)
{
    if (!g_eeprom_inited)
    {
        return;
    }

    if (!g_async_busy)
    {
        return;
    }

    if (g_async_dma_active)
    {
        return;
    }

    {
        uint16_t cur_addr = (uint16_t)(g_async_addr + g_async_off);
        uint16_t page_remain = (uint16_t)(APP_EEPROM_PAGE_SIZE_BYTES - (cur_addr % APP_EEPROM_PAGE_SIZE_BYTES));
        uint16_t chunk = (uint16_t)(g_async_len - g_async_off);

        if (chunk > page_remain)
        {
            chunk = page_remain;
        }

        if (!eeprom_wait_ready())
        {
            g_async_status = -1;
            g_async_busy = 0;
            return;
        }

        if (!eeprom_write_chunk_dma_start(cur_addr, &g_async_buf[g_async_off], chunk))
        {
            g_async_status = -1;
            g_async_busy = 0;
            return;
        }
    }
}

int bsp_eeprom_is_busy(void)
{
    uint32_t irq_state = irq_lock();
    int busy = g_async_busy ? 1 : 0;
    irq_unlock(irq_state);
    return busy;
}

int bsp_eeprom_get_last_status(void)
{
    uint32_t irq_state = irq_lock();
    int status = g_async_status;
    irq_unlock(irq_state);
    return status;
}

#else

static uint8_t g_mock_eeprom[APP_EEPROM_TOTAL_SIZE_BYTES];
static int g_mock_inited = 0;
static int g_mock_read_ok = 1;
static int g_mock_write_ok = 1;
static int g_mock_last_status = 1;

void bsp_eeprom_init(void)
{
    if (!g_mock_inited)
    {
        memset(g_mock_eeprom, 0xFF, sizeof(g_mock_eeprom));
        g_mock_inited = 1;
    }
}

void bsp_eeprom_mock_set_access_ok(int read_ok, int write_ok)
{
    g_mock_read_ok = (read_ok != 0) ? 1 : 0;
    g_mock_write_ok = (write_ok != 0) ? 1 : 0;
}

int bsp_eeprom_read(uint16_t addr, uint8_t *buf, uint16_t len)
{
    if ((buf == 0) || (len == 0U))
    {
        return 0;
    }

    if (((uint32_t)addr + (uint32_t)len) > APP_EEPROM_TOTAL_SIZE_BYTES)
    {
        return 0;
    }

    if (!g_mock_read_ok)
    {
        g_mock_last_status = -1;
        return 0;
    }

    memcpy(buf, &g_mock_eeprom[addr], len);
    g_mock_last_status = 1;
    return 1;
}

int bsp_eeprom_write(uint16_t addr, const uint8_t *buf, uint16_t len)
{
    if ((buf == 0) || (len == 0U))
    {
        return 0;
    }

    if (((uint32_t)addr + (uint32_t)len) > APP_EEPROM_TOTAL_SIZE_BYTES)
    {
        return 0;
    }

    if (!g_mock_write_ok)
    {
        g_mock_last_status = -1;
        return 0;
    }

    memcpy(&g_mock_eeprom[addr], buf, len);
    g_mock_last_status = 1;
    return 1;
}

int bsp_eeprom_write_async(uint16_t addr, const uint8_t *buf, uint16_t len)
{
    return bsp_eeprom_write(addr, buf, len);
}

void bsp_eeprom_process(void)
{
}

int bsp_eeprom_is_busy(void)
{
    return 0;
}

int bsp_eeprom_get_last_status(void)
{
    return g_mock_last_status;
}

#endif
