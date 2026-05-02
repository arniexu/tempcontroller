#include "bsp_spiflash.h"

#include "app_config.h"

#include <string.h>

#if defined(USE_STDPERIPH_DRIVER)
#include "stm32f10x_gpio.h"
#include "stm32f10x_rcc.h"
#include "stm32f10x_spi.h"

#define SPIFLASH_SPI              SPI1
#define SPIFLASH_SPI_CLK          RCC_APB2Periph_SPI1
#define SPIFLASH_GPIO_PORT        GPIOA
#define SPIFLASH_GPIO_CLK         RCC_APB2Periph_GPIOA
#define SPIFLASH_PIN_SCK          GPIO_Pin_5
#define SPIFLASH_PIN_MISO         GPIO_Pin_6
#define SPIFLASH_PIN_MOSI         GPIO_Pin_7
#define SPIFLASH_PIN_CS           GPIO_Pin_2

#define SPIFLASH_CMD_WREN         (0x06U)
#define SPIFLASH_CMD_RDSR         (0x05U)
#define SPIFLASH_CMD_READ         (0x03U)
#define SPIFLASH_CMD_PP           (0x02U)
#define SPIFLASH_CMD_SE           (0x20U)

#define SPIFLASH_STATUS_WIP       (0x01U)
//@TODO: TOO BIG RETRIES
#define SPIFLASH_TIMEOUT          (1000000UL)
#define SPIFLASH_PAGE_SIZE        (256UL)
#define SPIFLASH_SECTOR_SIZE      (4096UL)

static int g_spiflash_inited = 0;
static int g_spiflash_last_status = 1;

static void spiflash_cs_low(void)
{
    GPIO_ResetBits(SPIFLASH_GPIO_PORT, SPIFLASH_PIN_CS);
}

static void spiflash_cs_high(void)
{
    GPIO_SetBits(SPIFLASH_GPIO_PORT, SPIFLASH_PIN_CS);
}

static uint8_t spiflash_xfer(uint8_t data)
{
    uint32_t timeout = SPIFLASH_TIMEOUT;

    while ((SPI_I2S_GetFlagStatus(SPIFLASH_SPI, SPI_I2S_FLAG_TXE) == RESET) && (timeout > 0UL))
    {
        timeout--;
    }
    if (timeout == 0UL)
    {
        return 0xFFU;
    }

    SPI_I2S_SendData(SPIFLASH_SPI, data);

    timeout = SPIFLASH_TIMEOUT;
    while ((SPI_I2S_GetFlagStatus(SPIFLASH_SPI, SPI_I2S_FLAG_RXNE) == RESET) && (timeout > 0UL))
    {
        timeout--;
    }
    if (timeout == 0UL)
    {
        return 0xFFU;
    }

    return (uint8_t)SPI_I2S_ReceiveData(SPIFLASH_SPI);
}

static int spiflash_wait_ready(void)
{
    uint32_t timeout = SPIFLASH_TIMEOUT;

    while (timeout > 0UL)
    {
        uint8_t status;

        spiflash_cs_low();
        (void)spiflash_xfer(SPIFLASH_CMD_RDSR);
        status = spiflash_xfer(0xFFU);
        spiflash_cs_high();

        if ((status & SPIFLASH_STATUS_WIP) == 0U)
        {
            return 1;
        }
        timeout--;
    }

    return 0;
}

static void spiflash_write_enable(void)
{
    spiflash_cs_low();
    (void)spiflash_xfer(SPIFLASH_CMD_WREN);
    spiflash_cs_high();
}

static void spiflash_send_addr(uint32_t addr)
{
    (void)spiflash_xfer((uint8_t)(addr >> 16U));
    (void)spiflash_xfer((uint8_t)(addr >> 8U));
    (void)spiflash_xfer((uint8_t)addr);
}

static int spiflash_erase_sector(uint32_t addr)
{
    if (!spiflash_wait_ready())
    {
        return 0;
    }

    spiflash_write_enable();
    spiflash_cs_low();
    (void)spiflash_xfer(SPIFLASH_CMD_SE);
    spiflash_send_addr(addr);
    spiflash_cs_high();

    return spiflash_wait_ready();
}

static int spiflash_page_program(uint32_t addr, const uint8_t *buf, uint32_t len)
{
    uint32_t i;

    if (!spiflash_wait_ready())
    {
        return 0;
    }

    spiflash_write_enable();
    spiflash_cs_low();
    (void)spiflash_xfer(SPIFLASH_CMD_PP);
    spiflash_send_addr(addr);
    for (i = 0UL; i < len; ++i)
    {
        (void)spiflash_xfer(buf[i]);
    }
    spiflash_cs_high();

    return spiflash_wait_ready();
}

void bsp_spiflash_init(void)
{
    GPIO_InitTypeDef gpio;
    SPI_InitTypeDef spi;

    RCC_APB2PeriphClockCmd(SPIFLASH_GPIO_CLK | SPIFLASH_SPI_CLK, ENABLE);

    gpio.GPIO_Pin = SPIFLASH_PIN_SCK | SPIFLASH_PIN_MOSI;
    gpio.GPIO_Speed = GPIO_Speed_50MHz;
    gpio.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_Init(SPIFLASH_GPIO_PORT, &gpio);

    gpio.GPIO_Pin = SPIFLASH_PIN_MISO;
    gpio.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(SPIFLASH_GPIO_PORT, &gpio);

    gpio.GPIO_Pin = SPIFLASH_PIN_CS;
    gpio.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_Init(SPIFLASH_GPIO_PORT, &gpio);
    spiflash_cs_high();

    SPI_I2S_DeInit(SPIFLASH_SPI);
    SPI_StructInit(&spi);
    spi.SPI_Direction = SPI_Direction_2Lines_FullDuplex;
    spi.SPI_Mode = SPI_Mode_Master;
    spi.SPI_DataSize = SPI_DataSize_8b;
    spi.SPI_CPOL = SPI_CPOL_Low;
    spi.SPI_CPHA = SPI_CPHA_1Edge;
    spi.SPI_NSS = SPI_NSS_Soft;
    spi.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_4;
    spi.SPI_FirstBit = SPI_FirstBit_MSB;
    spi.SPI_CRCPolynomial = 7U;
    SPI_Init(SPIFLASH_SPI, &spi);
    SPI_Cmd(SPIFLASH_SPI, ENABLE);

    g_spiflash_inited = 1;
    g_spiflash_last_status = 1;
}

// HAL
int bsp_spiflash_read(uint32_t addr, uint8_t *buf, uint32_t len)
{
    uint32_t i;

    if ((!g_spiflash_inited) || (buf == 0) || (len == 0UL) || ((addr + len) > APP_SPIFLASH_TOTAL_SIZE_BYTES))
    {
        g_spiflash_last_status = -1;
        return 0;
    }

    if (!spiflash_wait_ready())
    {
        g_spiflash_last_status = -1;
        return 0;
    }

    spiflash_cs_low();
    (void)spiflash_xfer(SPIFLASH_CMD_READ);
    spiflash_send_addr(addr);
    for (i = 0UL; i < len; ++i)
    {
        buf[i] = spiflash_xfer(0xFFU);
    }
    spiflash_cs_high();

    g_spiflash_last_status = 1;
    return 1;
}

int bsp_spiflash_write(uint32_t addr, const uint8_t *buf, uint32_t len)
{
    uint32_t off = 0UL;
    uint32_t last_sector = 0xFFFFFFFFUL;

    if ((!g_spiflash_inited) || (buf == 0) || (len == 0UL) || ((addr + len) > APP_SPIFLASH_TOTAL_SIZE_BYTES))
    {
        g_spiflash_last_status = -1;
        return 0;
    }

    while (off < len)
    {
        uint32_t cur_addr = addr + off;
        uint32_t sector = cur_addr / SPIFLASH_SECTOR_SIZE;
        uint32_t page_off = cur_addr % SPIFLASH_PAGE_SIZE;
        uint32_t chunk = len - off;

        if (sector != last_sector)
        {
            if (!spiflash_erase_sector(sector * SPIFLASH_SECTOR_SIZE))
            {
                g_spiflash_last_status = -1;
                return 0;
            }
            last_sector = sector;
        }

        if (chunk > (SPIFLASH_PAGE_SIZE - page_off))
        {
            chunk = SPIFLASH_PAGE_SIZE - page_off;
        }

        if (!spiflash_page_program(cur_addr, &buf[off], chunk))
        {
            g_spiflash_last_status = -1;
            return 0;
        }
        off += chunk;
    }

    g_spiflash_last_status = 1;
    return 1;
}

void bsp_spiflash_process(void)
{
}

int bsp_spiflash_is_busy(void)
{
    return 0;
}

int bsp_spiflash_get_last_status(void)
{
    return g_spiflash_last_status;
}

void bsp_spiflash_mock_set_access_ok(int read_ok, int write_ok)
{
    (void)read_ok;
    (void)write_ok;
}

#else

static uint8_t g_mock_spiflash[APP_SPIFLASH_TOTAL_SIZE_BYTES];
static int g_mock_inited = 0;
static int g_mock_read_ok = 1;
static int g_mock_write_ok = 1;
static int g_mock_last_status = 1;

void bsp_spiflash_init(void)
{
    if (!g_mock_inited)
    {
        memset(g_mock_spiflash, 0xFF, sizeof(g_mock_spiflash));
        g_mock_inited = 1;
    }
}

int bsp_spiflash_read(uint32_t addr, uint8_t *buf, uint32_t len)
{
    if ((buf == 0) || (len == 0UL) || ((addr + len) > APP_SPIFLASH_TOTAL_SIZE_BYTES))
    {
        return 0;
    }
    if (!g_mock_read_ok)
    {
        g_mock_last_status = -1;
        return 0;
    }

    memcpy(buf, &g_mock_spiflash[addr], len);
    g_mock_last_status = 1;
    return 1;
}

int bsp_spiflash_write(uint32_t addr, const uint8_t *buf, uint32_t len)
{
    if ((buf == 0) || (len == 0UL) || ((addr + len) > APP_SPIFLASH_TOTAL_SIZE_BYTES))
    {
        return 0;
    }
    if (!g_mock_write_ok)
    {
        g_mock_last_status = -1;
        return 0;
    }

    memcpy(&g_mock_spiflash[addr], buf, len);
    g_mock_last_status = 1;
    return 1;
}

void bsp_spiflash_process(void)
{
}

int bsp_spiflash_is_busy(void)
{
    return 0;
}

int bsp_spiflash_get_last_status(void)
{
    return g_mock_last_status;
}

void bsp_spiflash_mock_set_access_ok(int read_ok, int write_ok)
{
    g_mock_read_ok = (read_ok != 0) ? 1 : 0;
    g_mock_write_ok = (write_ok != 0) ? 1 : 0;
}

#endif
