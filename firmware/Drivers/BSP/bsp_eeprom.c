#include "bsp_eeprom.h"

#include "app_config.h"

#include <string.h>

#if defined(USE_STDPERIPH_DRIVER)
#include "stm32f10x_gpio.h"
#include "stm32f10x_i2c.h"
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

#if defined(USE_STDPERIPH_DRIVER)

#define EEPROM_I2C                      I2C1
#define EEPROM_GPIO_PORT                GPIOB
#define EEPROM_SCL_PIN                  GPIO_Pin_6
#define EEPROM_SDA_PIN                  GPIO_Pin_7
#define EEPROM_I2C_ADDR_8BIT            ((uint8_t)(APP_EEPROM_I2C_ADDR_7BIT << 1U))
#define EEPROM_TIMEOUT                  (20000U)
#define EEPROM_READY_RETRY              (200U)

static int i2c_wait_event(uint32_t event)
{
    uint32_t timeout = EEPROM_TIMEOUT;

    while (I2C_CheckEvent(EEPROM_I2C, event) == ERROR)
    {
        if (timeout == 0U)
        {
            return 0;
        }
        --timeout;
    }
    return 1;
}

static int i2c_wait_flag_clear(uint32_t flag)
{
    uint32_t timeout = EEPROM_TIMEOUT;

    while (I2C_GetFlagStatus(EEPROM_I2C, flag) != RESET)
    {
        if (timeout == 0U)
        {
            return 0;
        }
        --timeout;
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

void bsp_eeprom_init(void)
{
    GPIO_InitTypeDef gpio;
    I2C_InitTypeDef i2c;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB | RCC_APB2Periph_AFIO, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_I2C1, ENABLE);

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
}

void bsp_eeprom_mock_set_access_ok(int read_ok, int write_ok)
{
    (void)read_ok;
    (void)write_ok;
}

int bsp_eeprom_write(uint16_t addr, const uint8_t *buf, uint16_t len)
{
    uint16_t off = 0U;

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
        uint16_t i;

        if (chunk > page_remain)
        {
            chunk = page_remain;
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

        I2C_SendData(EEPROM_I2C, (uint8_t)cur_addr);
        if (!i2c_wait_event(I2C_EVENT_MASTER_BYTE_TRANSMITTED))
        {
            I2C_GenerateSTOP(EEPROM_I2C, ENABLE);
            return 0;
        }

        for (i = 0U; i < chunk; ++i)
        {
            I2C_SendData(EEPROM_I2C, buf[off + i]);
            if (!i2c_wait_event(I2C_EVENT_MASTER_BYTE_TRANSMITTED))
            {
                I2C_GenerateSTOP(EEPROM_I2C, ENABLE);
                return 0;
            }
        }

        I2C_GenerateSTOP(EEPROM_I2C, ENABLE);
        off = (uint16_t)(off + chunk);
    }

    return eeprom_wait_ready();
}

int bsp_eeprom_read(uint16_t addr, uint8_t *buf, uint16_t len)
{
    uint16_t i;

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

#else

static uint8_t g_mock_eeprom[APP_EEPROM_TOTAL_SIZE_BYTES];
static int g_mock_inited = 0;
static int g_mock_read_ok = 1;
static int g_mock_write_ok = 1;

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
        return 0;
    }

    memcpy(buf, &g_mock_eeprom[addr], len);
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
        return 0;
    }

    memcpy(&g_mock_eeprom[addr], buf, len);
    return 1;
}

#endif
