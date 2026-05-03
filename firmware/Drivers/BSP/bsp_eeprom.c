#include "bsp_eeprom.h"

#include "../../ProjectConfig/bsp_config_select.h"

#include <string.h>

#if defined(USE_HAL_DRIVER)
#include "stm32f1xx_hal.h"
#endif

#ifndef BSP_EEPROM_TOTAL_SIZE_BYTES
#define BSP_EEPROM_TOTAL_SIZE_BYTES    (256U)
#endif

#ifndef BSP_EEPROM_PAGE_SIZE_BYTES
#define BSP_EEPROM_PAGE_SIZE_BYTES     (8U)
#endif

#ifndef BSP_EEPROM_I2C_ADDR_7BIT
#define BSP_EEPROM_I2C_ADDR_7BIT       (0x50U)
#endif

#ifndef BSP_EEPROM_I2C_SPEED_HZ
#define BSP_EEPROM_I2C_SPEED_HZ        (100000U)
#endif

#if (BSP_EEPROM_TOTAL_SIZE_BYTES == 0U)
#error "BSP_EEPROM_TOTAL_SIZE_BYTES must be > 0"
#endif

#if (BSP_EEPROM_PAGE_SIZE_BYTES == 0U)
#error "BSP_EEPROM_PAGE_SIZE_BYTES must be > 0"
#endif

#if (BSP_EEPROM_TOTAL_SIZE_BYTES > 256U)
#error "BSP_EEPROM_TOTAL_SIZE_BYTES must be <= 256 for this driver"
#endif

#if (BSP_EEPROM_I2C_SPEED_HZ == 0U)
#error "BSP_EEPROM_I2C_SPEED_HZ must be > 0"
#endif

#if defined(USE_HAL_DRIVER)
static I2C_HandleTypeDef g_hi2c2;
static volatile int g_async_busy = 0;
static volatile int g_async_status = 1;
static volatile int g_inited = 0;

static int eeprom_wait_ready(void)
{
    uint32_t retry;

    for (retry = 0U; retry < 200U; ++retry)
    {
        if (HAL_I2C_IsDeviceReady(&g_hi2c2, (uint16_t)(BSP_EEPROM_I2C_ADDR_7BIT << 1U), 1U, 10U) == HAL_OK)
        {
            return 1;
        }
    }

    return 0;
}

static int eeprom_write_chunk(uint16_t addr, const uint8_t *buf, uint16_t len)
{
    if (HAL_I2C_Mem_Write(&g_hi2c2,
                          (uint16_t)(BSP_EEPROM_I2C_ADDR_7BIT << 1U),
                          addr,
                          I2C_MEMADD_SIZE_8BIT,
                          (uint8_t *)buf,
                          len,
                          200U) != HAL_OK)
    {
        return 0;
    }

    return eeprom_wait_ready();
}
#endif

#if !defined(USE_HAL_DRIVER)
static uint8_t g_mock_mem[BSP_EEPROM_TOTAL_SIZE_BYTES];
static int g_mock_read_ok = 1;
static int g_mock_write_ok = 1;
static int g_mock_busy = 0;
static int g_mock_status = 1;
#endif

void bsp_eeprom_init(void)
{
#if defined(USE_HAL_DRIVER)
    GPIO_InitTypeDef gpio = {0};

    BSP_EEPROM_GPIO_CLK_ENABLE();
    BSP_EEPROM_PERIPH_CLK_ENABLE();

    gpio.Pin = BSP_EEPROM_PIN_SCL | BSP_EEPROM_PIN_SDA;
    gpio.Mode = GPIO_MODE_AF_OD;
    gpio.Pull = GPIO_NOPULL;
    gpio.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(BSP_EEPROM_GPIO_PORT, &gpio);

    g_hi2c2.Instance = I2C2;
    g_hi2c2.Init.ClockSpeed = BSP_EEPROM_I2C_SPEED_HZ;
    g_hi2c2.Init.DutyCycle = I2C_DUTYCYCLE_2;
    g_hi2c2.Init.OwnAddress1 = 0U;
    g_hi2c2.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
    g_hi2c2.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
    g_hi2c2.Init.OwnAddress2 = 0U;
    g_hi2c2.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
    g_hi2c2.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;

    g_inited = (HAL_I2C_Init(&g_hi2c2) == HAL_OK) ? 1 : 0;
    g_async_busy = 0;
    g_async_status = 1;
#else
    memset(g_mock_mem, 0xFF, sizeof(g_mock_mem));
    g_mock_read_ok = 1;
    g_mock_write_ok = 1;
    g_mock_busy = 0;
    g_mock_status = 1;
#endif
}

void bsp_eeprom_mock_set_access_ok(int read_ok, int write_ok)
{
#if defined(USE_HAL_DRIVER)
    (void)read_ok;
    (void)write_ok;
#else
    g_mock_read_ok = read_ok;
    g_mock_write_ok = write_ok;
#endif
}

int bsp_eeprom_read(uint16_t addr, uint8_t *buf, uint16_t len)
{
    if ((buf == 0) || (len == 0U) || ((uint32_t)addr + (uint32_t)len > (uint32_t)BSP_EEPROM_TOTAL_SIZE_BYTES))
    {
        return 0;
    }

#if defined(USE_HAL_DRIVER)
    if (!g_inited)
    {
        g_async_status = -1;
        return 0;
    }

    if (HAL_I2C_Mem_Read(&g_hi2c2,
                         (uint16_t)(BSP_EEPROM_I2C_ADDR_7BIT << 1U),
                         addr,
                         I2C_MEMADD_SIZE_8BIT,
                         buf,
                         len,
                         200U) != HAL_OK)
    {
        g_async_status = -1;
        return 0;
    }

    g_async_status = 1;
    return 1;
#else
    if (!g_mock_read_ok)
    {
        g_mock_status = -1;
        return 0;
    }

    memcpy(buf, &g_mock_mem[addr], len);
    g_mock_status = 1;
    return 1;
#endif
}

int bsp_eeprom_write(uint16_t addr, const uint8_t *buf, uint16_t len)
{
    if ((buf == 0) || (len == 0U) || ((uint32_t)addr + (uint32_t)len > (uint32_t)BSP_EEPROM_TOTAL_SIZE_BYTES))
    {
        return 0;
    }

#if defined(USE_HAL_DRIVER)
    uint16_t off = 0U;

    if (!g_inited)
    {
        g_async_status = -1;
        return 0;
    }

    while (off < len)
    {
        uint16_t cur = (uint16_t)(addr + off);
        uint16_t page_off = (uint16_t)(cur % BSP_EEPROM_PAGE_SIZE_BYTES);
        uint16_t chunk = (uint16_t)(len - off);

        if (chunk > (uint16_t)(BSP_EEPROM_PAGE_SIZE_BYTES - page_off))
        {
            chunk = (uint16_t)(BSP_EEPROM_PAGE_SIZE_BYTES - page_off);
        }

        if (!eeprom_write_chunk(cur, &buf[off], chunk))
        {
            g_async_status = -1;
            return 0;
        }

        off = (uint16_t)(off + chunk);
    }

    g_async_status = 1;
    return 1;
#else
    if (!g_mock_write_ok)
    {
        g_mock_status = -1;
        return 0;
    }

    memcpy(&g_mock_mem[addr], buf, len);
    g_mock_status = 1;
    return 1;
#endif
}

int bsp_eeprom_write_async(uint16_t addr, const uint8_t *buf, uint16_t len)
{
#if defined(USE_HAL_DRIVER)
    int ok;

    if (g_async_busy)
    {
        return 0;
    }

    g_async_busy = 1;
    ok = bsp_eeprom_write(addr, buf, len);
    g_async_busy = 0;
    g_async_status = ok ? 1 : -1;
    return ok;
#else
    return bsp_eeprom_write(addr, buf, len);
#endif
}

void bsp_eeprom_process(void)
{
}

int bsp_eeprom_is_busy(void)
{
#if defined(USE_HAL_DRIVER)
    return g_async_busy;
#else
    return g_mock_busy;
#endif
}

int bsp_eeprom_get_last_status(void)
{
#if defined(USE_HAL_DRIVER)
    return g_async_status;
#else
    return g_mock_status;
#endif
}
