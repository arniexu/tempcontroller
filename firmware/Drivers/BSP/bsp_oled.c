#include "bsp_oled.h"

#include <string.h>

#if defined(USE_STDPERIPH_DRIVER)
#include "stm32f10x_gpio.h"
#include "stm32f10x_i2c.h"
#include "misc.h"
#include "stm32f10x_rcc.h"

#define OLED_I2C                    I2C1
#define OLED_I2C_CLK                RCC_APB1Periph_I2C1
#define OLED_GPIO_PORT              GPIOB
#define OLED_GPIO_CLK               RCC_APB2Periph_GPIOB
#define OLED_PIN_SCL                GPIO_Pin_8
#define OLED_PIN_SDA                GPIO_Pin_9
#define OLED_ADDR                   (0x3CU)
#define OLED_TIMEOUT                (10000U)

static volatile uint32_t g_i2c2_evt_count = 0U;
static volatile uint32_t g_i2c2_err_count = 0U;

static int oled_wait_event(uint32_t event)
{
    uint32_t timeout = OLED_TIMEOUT;
    uint32_t err_start = g_i2c2_err_count;

    while ((I2C_CheckEvent(OLED_I2C, event) == ERROR) && (timeout > 0U))
    {
        if (g_i2c2_err_count != err_start)
        {
            return 0;
        }

        timeout--;
        if (g_i2c2_evt_count > 0U)
        {
            g_i2c2_evt_count = 0U;
        }
        else
        {
            __WFI();
        }
    }
    return timeout > 0U;
}

static int oled_i2c_write(uint8_t control, const uint8_t *data, uint8_t len)
{
    uint8_t i;

    I2C_GenerateSTART(OLED_I2C, ENABLE);
    if (!oled_wait_event(I2C_EVENT_MASTER_MODE_SELECT))
    {
        return 0;
    }

    I2C_Send7bitAddress(OLED_I2C, (uint8_t)(OLED_ADDR << 1U), I2C_Direction_Transmitter);
    if (!oled_wait_event(I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED))
    {
        return 0;
    }

    I2C_SendData(OLED_I2C, control);
    if (!oled_wait_event(I2C_EVENT_MASTER_BYTE_TRANSMITTED))
    {
        return 0;
    }

    for (i = 0U; i < len; ++i)
    {
        I2C_SendData(OLED_I2C, data[i]);
        if (!oled_wait_event(I2C_EVENT_MASTER_BYTE_TRANSMITTED))
        {
            return 0;
        }
    }

    I2C_GenerateSTOP(OLED_I2C, ENABLE);
    return 1;
}

static void oled_cmd(uint8_t cmd)
{
    (void)oled_i2c_write(0x00U, &cmd, 1U);
}

static void oled_data(const uint8_t *buf, uint8_t len)
{
    (void)oled_i2c_write(0x40U, buf, len);
}

static void glyph_5x7(char c, uint8_t out[5])
{
    uint8_t i;

    for (i = 0U; i < 5U; ++i)
    {
        out[i] = 0x00U;
    }

    if ((c >= '0') && (c <= '9'))
    {
        static const uint8_t digit[10][5] = {
            {0x3EU, 0x51U, 0x49U, 0x45U, 0x3EU},
            {0x00U, 0x42U, 0x7FU, 0x40U, 0x00U},
            {0x62U, 0x51U, 0x49U, 0x49U, 0x46U},
            {0x22U, 0x49U, 0x49U, 0x49U, 0x36U},
            {0x18U, 0x14U, 0x12U, 0x7FU, 0x10U},
            {0x2FU, 0x49U, 0x49U, 0x49U, 0x31U},
            {0x3EU, 0x49U, 0x49U, 0x49U, 0x32U},
            {0x01U, 0x71U, 0x09U, 0x05U, 0x03U},
            {0x36U, 0x49U, 0x49U, 0x49U, 0x36U},
            {0x26U, 0x49U, 0x49U, 0x49U, 0x3EU}
        };
        memcpy(out, digit[(unsigned int)(c - '0')], 5U);
        return;
    }

    switch (c)
    {
    case 'A': out[0]=0x7EU; out[1]=0x11U; out[2]=0x11U; out[3]=0x11U; out[4]=0x7EU; break;
    case 'B': out[0]=0x7FU; out[1]=0x49U; out[2]=0x49U; out[3]=0x49U; out[4]=0x36U; break;
    case 'C': out[0]=0x3EU; out[1]=0x41U; out[2]=0x41U; out[3]=0x41U; out[4]=0x22U; break;
    case 'D': out[0]=0x7FU; out[1]=0x41U; out[2]=0x41U; out[3]=0x22U; out[4]=0x1CU; break;
    case 'E': out[0]=0x7FU; out[1]=0x49U; out[2]=0x49U; out[3]=0x49U; out[4]=0x41U; break;
    case 'F': out[0]=0x7FU; out[1]=0x09U; out[2]=0x09U; out[3]=0x09U; out[4]=0x01U; break;
    case 'G': out[0]=0x3EU; out[1]=0x41U; out[2]=0x49U; out[3]=0x49U; out[4]=0x3AU; break;
    case 'H': out[0]=0x7FU; out[1]=0x08U; out[2]=0x08U; out[3]=0x08U; out[4]=0x7FU; break;
    case 'I': out[0]=0x00U; out[1]=0x41U; out[2]=0x7FU; out[3]=0x41U; out[4]=0x00U; break;
    case 'K': out[0]=0x7FU; out[1]=0x08U; out[2]=0x14U; out[3]=0x22U; out[4]=0x41U; break;
    case 'L': out[0]=0x7FU; out[1]=0x40U; out[2]=0x40U; out[3]=0x40U; out[4]=0x40U; break;
    case 'M': out[0]=0x7FU; out[1]=0x02U; out[2]=0x0CU; out[3]=0x02U; out[4]=0x7FU; break;
    case 'N': out[0]=0x7FU; out[1]=0x04U; out[2]=0x08U; out[3]=0x10U; out[4]=0x7FU; break;
    case 'O': out[0]=0x3EU; out[1]=0x41U; out[2]=0x41U; out[3]=0x41U; out[4]=0x3EU; break;
    case 'P': out[0]=0x7FU; out[1]=0x09U; out[2]=0x09U; out[3]=0x09U; out[4]=0x06U; break;
    case 'R': out[0]=0x7FU; out[1]=0x09U; out[2]=0x19U; out[3]=0x29U; out[4]=0x46U; break;
    case 'S': out[0]=0x46U; out[1]=0x49U; out[2]=0x49U; out[3]=0x49U; out[4]=0x31U; break;
    case 'T': out[0]=0x01U; out[1]=0x01U; out[2]=0x7FU; out[3]=0x01U; out[4]=0x01U; break;
    case 'U': out[0]=0x3FU; out[1]=0x40U; out[2]=0x40U; out[3]=0x40U; out[4]=0x3FU; break;
    case 'V': out[0]=0x1FU; out[1]=0x20U; out[2]=0x40U; out[3]=0x20U; out[4]=0x1FU; break;
    case 'W': out[0]=0x3FU; out[1]=0x40U; out[2]=0x38U; out[3]=0x40U; out[4]=0x3FU; break;
    case 'Y': out[0]=0x07U; out[1]=0x08U; out[2]=0x70U; out[3]=0x08U; out[4]=0x07U; break;
    case ' ': break;
    case '.': out[2]=0x60U; break;
    case ':': out[1]=0x36U; out[3]=0x36U; break;
    case '-': out[1]=0x08U; out[2]=0x08U; out[3]=0x08U; break;
    case '/': out[0]=0x20U; out[1]=0x10U; out[2]=0x08U; out[3]=0x04U; out[4]=0x02U; break;
    case '*': out[0]=0x14U; out[1]=0x08U; out[2]=0x3EU; out[3]=0x08U; out[4]=0x14U; break;
    default:
        out[0]=0x7FU; out[1]=0x41U; out[2]=0x5DU; out[3]=0x41U; out[4]=0x7FU;
        break;
    }
}
#endif

static char g_lines[BSP_OLED_LINE_COUNT][BSP_OLED_LINE_CHARS + 1U];
static uint8_t g_frame[BSP_OLED_LINE_COUNT][128U];
static uint8_t g_refresh_pending = 0U;
static uint8_t g_refresh_page = 0U;

static void oled_build_frame(void)
{
#if defined(USE_STDPERIPH_DRIVER)
    uint8_t page;
    uint8_t col;
    uint8_t c;
    uint8_t glyph[5];

    for (page = 0U; page < BSP_OLED_LINE_COUNT; ++page)
    {
        for (col = 0U; col < 128U; ++col)
        {
            g_frame[page][col] = 0x00U;
        }

        for (col = 0U; col < BSP_OLED_LINE_CHARS; ++col)
        {
            uint8_t x = (uint8_t)(col * 6U);
            c = (uint8_t)g_lines[page][col];
            if (c == '\0')
            {
                c = ' ';
            }
            if ((c >= 'a') && (c <= 'z'))
            {
                c = (uint8_t)(c - ('a' - 'A'));
            }

            glyph_5x7((char)c, glyph);
            g_frame[page][x + 0U] = glyph[0];
            g_frame[page][x + 1U] = glyph[1];
            g_frame[page][x + 2U] = glyph[2];
            g_frame[page][x + 3U] = glyph[3];
            g_frame[page][x + 4U] = glyph[4];
            g_frame[page][x + 5U] = 0x00U;
        }
    }
#else
    unsigned int page;
    unsigned int col;

    for (page = 0U; page < BSP_OLED_LINE_COUNT; ++page)
    {
        for (col = 0U; col < 128U; ++col)
        {
            g_frame[page][col] = 0x00U;
        }
    }
#endif
}

void bsp_oled_init(void)
{
    unsigned int i;

#if defined(USE_STDPERIPH_DRIVER)
    GPIO_InitTypeDef gpio;
    I2C_InitTypeDef i2c;
    NVIC_InitTypeDef nvic;

    RCC_APB2PeriphClockCmd(OLED_GPIO_CLK | RCC_APB2Periph_AFIO, ENABLE);
    RCC_APB1PeriphClockCmd(OLED_I2C_CLK, ENABLE);
    GPIO_PinRemapConfig(GPIO_Remap_I2C1, ENABLE);

    gpio.GPIO_Pin = OLED_PIN_SCL | OLED_PIN_SDA;
    gpio.GPIO_Mode = GPIO_Mode_AF_OD;
    gpio.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(OLED_GPIO_PORT, &gpio);

    I2C_DeInit(OLED_I2C);
    I2C_StructInit(&i2c);
    i2c.I2C_Mode = I2C_Mode_I2C;
    i2c.I2C_DutyCycle = I2C_DutyCycle_2;
    i2c.I2C_OwnAddress1 = 0x00U;
    i2c.I2C_Ack = I2C_Ack_Enable;
    i2c.I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_7bit;
    i2c.I2C_ClockSpeed = 400000U;
    I2C_Init(OLED_I2C, &i2c);
    I2C_Cmd(OLED_I2C, ENABLE);
    I2C_ITConfig(OLED_I2C, I2C_IT_EVT | I2C_IT_ERR, ENABLE);

    nvic.NVIC_IRQChannelPreemptionPriority = 3U;
    nvic.NVIC_IRQChannelSubPriority = 2U;
    nvic.NVIC_IRQChannelCmd = ENABLE;

    nvic.NVIC_IRQChannel = I2C1_EV_IRQn;
    NVIC_Init(&nvic);
    nvic.NVIC_IRQChannel = I2C1_ER_IRQn;
    NVIC_Init(&nvic);

    oled_cmd(0xAEU);
    oled_cmd(0x20U);
    oled_cmd(0x00U);
    oled_cmd(0xB0U);
    oled_cmd(0xC8U);
    oled_cmd(0x00U);
    oled_cmd(0x10U);
    oled_cmd(0x40U);
    oled_cmd(0x81U);
    oled_cmd(0x7FU);
    oled_cmd(0xA1U);
    oled_cmd(0xA6U);
    oled_cmd(0xA8U);
    oled_cmd(0x3FU);
    oled_cmd(0xA4U);
    oled_cmd(0xD3U);
    oled_cmd(0x00U);
    oled_cmd(0xD5U);
    oled_cmd(0x80U);
    oled_cmd(0xD9U);
    oled_cmd(0xF1U);
    oled_cmd(0xDAU);
    oled_cmd(0x12U);
    oled_cmd(0xDBU);
    oled_cmd(0x40U);
    oled_cmd(0x8DU);
    oled_cmd(0x14U);
    oled_cmd(0xAFU);
#endif

    for (i = 0U; i < BSP_OLED_LINE_COUNT; ++i)
    {
        g_lines[i][0] = '\0';
    }
}

#if defined(USE_STDPERIPH_DRIVER)
void I2C1_EV_IRQHandler(void)
{
    g_i2c2_evt_count++;
}

void I2C1_ER_IRQHandler(void)
{
    if (I2C_GetITStatus(OLED_I2C, I2C_IT_BERR) != RESET)
    {
        I2C_ClearITPendingBit(OLED_I2C, I2C_IT_BERR);
    }
    if (I2C_GetITStatus(OLED_I2C, I2C_IT_ARLO) != RESET)
    {
        I2C_ClearITPendingBit(OLED_I2C, I2C_IT_ARLO);
    }
    if (I2C_GetITStatus(OLED_I2C, I2C_IT_AF) != RESET)
    {
        I2C_ClearITPendingBit(OLED_I2C, I2C_IT_AF);
    }
    if (I2C_GetITStatus(OLED_I2C, I2C_IT_OVR) != RESET)
    {
        I2C_ClearITPendingBit(OLED_I2C, I2C_IT_OVR);
    }

    g_i2c2_err_count++;
}
#endif

void bsp_oled_clear(void)
{
    unsigned int i;

    for (i = 0U; i < BSP_OLED_LINE_COUNT; ++i)
    {
        g_lines[i][0] = '\0';
    }
}

void bsp_oled_draw_text(uint8_t line, const char *text)
{
    if ((line >= BSP_OLED_LINE_COUNT) || (text == 0))
    {
        return;
    }

    strncpy(g_lines[line], text, BSP_OLED_LINE_CHARS);
    g_lines[line][BSP_OLED_LINE_CHARS] = '\0';
}

void bsp_oled_refresh(void)
{
    oled_build_frame();
    g_refresh_page = 0U;
    g_refresh_pending = 1U;
}

int bsp_oled_process(void)
{
#if defined(USE_STDPERIPH_DRIVER)
    if (!g_refresh_pending)
    {
        return 0;
    }

    oled_cmd((uint8_t)(0xB0U + g_refresh_page));
    oled_cmd(0x00U);
    oled_cmd(0x10U);
    oled_data(g_frame[g_refresh_page], 128U);

    g_refresh_page++;
    if (g_refresh_page >= BSP_OLED_LINE_COUNT)
    {
        g_refresh_pending = 0U;
    }

    return (g_refresh_pending != 0U) ? 1 : 0;
#else
    return 0;
#endif
}

int bsp_oled_is_busy(void)
{
    return (g_refresh_pending != 0U) ? 1 : 0;
}

const char *bsp_oled_mock_get_line(uint8_t line)
{
    if (line >= BSP_OLED_LINE_COUNT)
    {
        return "";
    }

    return g_lines[line];
}
