#include "bsp_ds18b20.h"

#include "app_config.h"

#if defined(USE_STDPERIPH_DRIVER)
#include "stm32f10x.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_rcc.h"
#include "stm32f10x_tim.h"

#define DS18B20_CMD_SKIP_ROM      (0xCCU)
#define DS18B20_CMD_CONVERT_T     (0x44U)
#define DS18B20_CMD_READ_SCRATCH  (0xBEU)
#define DS18B20_CONVERT_TIMEOUT_MS (750U)
#define DS18B20_READ_RETRY         (2U)

static GPIO_TypeDef *const g_sensor_port[BSP_DS18B20_SENSOR_COUNT] = {GPIOB, GPIOB, GPIOB};
static const uint16_t g_sensor_pin[BSP_DS18B20_SENSOR_COUNT] = {GPIO_Pin_6, GPIO_Pin_7, GPIO_Pin_8};
static int g_delay_timer_ready = 0;

static void delay_us(uint32_t us)
{
    if (g_delay_timer_ready)
    {
        uint32_t start = TIM_GetCounter(TIM2);
        while ((uint32_t)(TIM_GetCounter(TIM2) - start) < us)
        {
        }
    }
    else
    {
        volatile uint32_t count = (SystemCoreClock / 8000000U) * us;
        while (count > 0U)
        {
            __NOP();
            count--;
        }
    }
}

static void delay_ms(uint32_t ms)
{
    while (ms > 0U)
    {
        delay_us(1000U);
        ms--;
    }
}

static void ow_drive_low(uint8_t index)
{
    GPIO_InitTypeDef gpio;

    gpio.GPIO_Pin = g_sensor_pin[index];
    gpio.GPIO_Mode = GPIO_Mode_Out_OD;
    gpio.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(g_sensor_port[index], &gpio);
    GPIO_ResetBits(g_sensor_port[index], g_sensor_pin[index]);
}

static void ow_release(uint8_t index)
{
    GPIO_InitTypeDef gpio;

    gpio.GPIO_Pin = g_sensor_pin[index];
    gpio.GPIO_Mode = GPIO_Mode_IPU;
    gpio.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(g_sensor_port[index], &gpio);
}

static uint8_t ow_read_pin(uint8_t index)
{
    return (uint8_t)(GPIO_ReadInputDataBit(g_sensor_port[index], g_sensor_pin[index]) ? 1U : 0U);
}

static bool ow_reset(uint8_t index)
{
    bool presence;

    ow_drive_low(index);
    delay_us(480U);
    ow_release(index);
    delay_us(70U);
    presence = (ow_read_pin(index) == 0U);
    delay_us(410U);

    return presence;
}

static void ow_write_bit(uint8_t index, uint8_t bit)
{
    ow_drive_low(index);
    if (bit != 0U)
    {
        delay_us(6U);
        ow_release(index);
        delay_us(64U);
    }
    else
    {
        delay_us(60U);
        ow_release(index);
        delay_us(10U);
    }
}

static uint8_t ow_read_bit(uint8_t index)
{
    uint8_t bit;

    ow_drive_low(index);
    delay_us(6U);
    ow_release(index);
    delay_us(9U);
    bit = ow_read_pin(index);
    delay_us(55U);

    return bit;
}

static void ow_write_byte(uint8_t index, uint8_t data)
{
    uint8_t i;

    for (i = 0U; i < 8U; ++i)
    {
        ow_write_bit(index, (uint8_t)(data & 0x01U));
        data >>= 1U;
    }
}

static uint8_t ow_read_byte(uint8_t index)
{
    uint8_t i;
    uint8_t data = 0U;

    for (i = 0U; i < 8U; ++i)
    {
        data >>= 1U;
        if (ow_read_bit(index) != 0U)
        {
            data |= 0x80U;
        }
    }

    return data;
}

static uint8_t ds18b20_crc8(const uint8_t *data, uint8_t len)
{
    uint8_t i;
    uint8_t j;
    uint8_t crc = 0U;

    for (i = 0U; i < len; ++i)
    {
        uint8_t inbyte = data[i];
        for (j = 0U; j < 8U; ++j)
        {
            uint8_t mix = (uint8_t)((crc ^ inbyte) & 0x01U);
            crc >>= 1U;
            if (mix != 0U)
            {
                crc ^= 0x8CU;
            }
            inbyte >>= 1U;
        }
    }

    return crc;
}

static bool ds18b20_wait_convert_done(uint8_t index)
{
    uint32_t wait_ms = 0U;

    while (wait_ms < DS18B20_CONVERT_TIMEOUT_MS)
    {
        if (ow_read_pin(index) != 0U)
        {
            return true;
        }
        delay_ms(1U);
        wait_ms++;
    }

    return false;
}

static bool ds18b20_read_once(uint8_t index, float *temp_c)
{
    uint8_t scratch[9];
    int16_t raw;
    uint8_t i;

    if (!ow_reset(index))
    {
        return false;
    }
    ow_write_byte(index, DS18B20_CMD_SKIP_ROM);
    ow_write_byte(index, DS18B20_CMD_CONVERT_T);

    if (!ds18b20_wait_convert_done(index))
    {
        return false;
    }

    if (!ow_reset(index))
    {
        return false;
    }
    ow_write_byte(index, DS18B20_CMD_SKIP_ROM);
    ow_write_byte(index, DS18B20_CMD_READ_SCRATCH);

    for (i = 0U; i < 9U; ++i)
    {
        scratch[i] = ow_read_byte(index);
    }

    if (ds18b20_crc8(scratch, 8U) != scratch[8])
    {
        return false;
    }

    raw = (int16_t)((((uint16_t)scratch[1]) << 8U) | scratch[0]);
    *temp_c = (float)raw / 16.0f;
    return true;
}
#endif

static float g_mock_temp[BSP_DS18B20_SENSOR_COUNT] = {25.0f, 25.2f, 24.9f};
static bool g_mock_valid[BSP_DS18B20_SENSOR_COUNT] = {true, true, true};

void bsp_ds18b20_init(void)
{
#if defined(USE_STDPERIPH_DRIVER)
    GPIO_InitTypeDef gpio;
    TIM_TimeBaseInitTypeDef tim;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);

    gpio.GPIO_Pin = GPIO_Pin_6 | GPIO_Pin_7 | GPIO_Pin_8;
    gpio.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    gpio.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOB, &gpio);

    TIM_TimeBaseStructInit(&tim);
    tim.TIM_Prescaler = (uint16_t)((SystemCoreClock / 1000000U) - 1U);
    tim.TIM_Period = 0xFFFFU;
    tim.TIM_ClockDivision = TIM_CKD_DIV1;
    tim.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseInit(TIM2, &tim);
    TIM_Cmd(TIM2, ENABLE);
    g_delay_timer_ready = 1;
#endif
}

bool bsp_ds18b20_read_c(uint8_t index, float *temp_c)
{
    if ((temp_c == 0) || (index >= BSP_DS18B20_SENSOR_COUNT))
    {
        return false;
    }

#if (APP_USE_MOCK_TEMP_SOURCE == 1U)
    if (!g_mock_valid[index])
    {
        return false;
    }

    *temp_c = g_mock_temp[index];
    return true;
#else
#if defined(USE_STDPERIPH_DRIVER)
    uint8_t retry;

    for (retry = 0U; retry < DS18B20_READ_RETRY; ++retry)
    {
        if (ds18b20_read_once(index, temp_c))
        {
            return true;
        }
    }

    return false;
#endif
    (void)index;
    return false;
#endif
}

void bsp_ds18b20_mock_set_temp(uint8_t index, float temp_c)
{
    if (index >= BSP_DS18B20_SENSOR_COUNT)
    {
        return;
    }

    g_mock_temp[index] = temp_c;
}

void bsp_ds18b20_mock_set_valid(uint8_t index, bool valid)
{
    if (index >= BSP_DS18B20_SENSOR_COUNT)
    {
        return;
    }

    g_mock_valid[index] = valid;
}
