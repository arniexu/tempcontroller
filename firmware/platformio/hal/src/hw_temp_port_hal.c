#include "hw_temp_port.h"

#include "stm32f1xx_hal.h"

static GPIO_TypeDef *const g_temp_port[3] = {GPIOA, GPIOA, GPIOA};
static const uint16_t g_temp_pin[3] = {GPIO_PIN_1, GPIO_PIN_3, GPIO_PIN_4};

static void delay_us(uint32_t us)
{
    uint32_t n = (SystemCoreClock / 8000000U) * us;

    while (n > 0U)
    {
        __NOP();
        n--;
    }
}

static void ow_drive_low(uint8_t index)
{
    GPIO_InitTypeDef gpio = {0};

    gpio.Pin = g_temp_pin[index];
    gpio.Mode = GPIO_MODE_OUTPUT_OD;
    gpio.Pull = GPIO_NOPULL;
    gpio.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(g_temp_port[index], &gpio);
    HAL_GPIO_WritePin(g_temp_port[index], g_temp_pin[index], GPIO_PIN_RESET);
}

static void ow_release(uint8_t index)
{
    GPIO_InitTypeDef gpio = {0};

    gpio.Pin = g_temp_pin[index];
    gpio.Mode = GPIO_MODE_INPUT;
    gpio.Pull = GPIO_PULLUP;
    gpio.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(g_temp_port[index], &gpio);
}

static uint8_t ow_read_pin(uint8_t index)
{
    return (HAL_GPIO_ReadPin(g_temp_port[index], g_temp_pin[index]) == GPIO_PIN_SET) ? 1U : 0U;
}

static int ow_reset(uint8_t index)
{
    uint8_t presence;

    ow_drive_low(index);
    delay_us(480U);
    ow_release(index);
    delay_us(70U);
    presence = (uint8_t)(ow_read_pin(index) == 0U);
    delay_us(410U);
    return (presence != 0U) ? 1 : 0;
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

static uint8_t crc8(const uint8_t *data, uint8_t len)
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

static int ds18b20_read_once(uint8_t index, float *temp_c)
{
    uint8_t scratch[9];
    uint8_t i;
    int16_t raw;
    uint32_t start_ms;

    if (!ow_reset(index))
    {
        return 0;
    }

    ow_write_byte(index, 0xCCU);
    ow_write_byte(index, 0x44U);

    start_ms = HAL_GetTick();
    while ((HAL_GetTick() - start_ms) < 800U)
    {
        if (ow_read_bit(index) != 0U)
        {
            break;
        }
    }

    if ((HAL_GetTick() - start_ms) >= 800U)
    {
        return 0;
    }

    if (!ow_reset(index))
    {
        return 0;
    }

    ow_write_byte(index, 0xCCU);
    ow_write_byte(index, 0xBEU);
    for (i = 0U; i < 9U; ++i)
    {
        scratch[i] = ow_read_byte(index);
    }

    if (crc8(scratch, 8U) != scratch[8])
    {
        return 0;
    }

    raw = (int16_t)((((uint16_t)scratch[1]) << 8U) | scratch[0]);
    *temp_c = (float)raw / 16.0f;
    return 1;
}

void hw_temp_port_init(void)
{
    uint8_t i;

    __HAL_RCC_GPIOA_CLK_ENABLE();
    for (i = 0U; i < 3U; ++i)
    {
        ow_release(i);
    }
}

bool hw_temp_port_read_c(uint8_t index, float *temp_c)
{
    if ((temp_c == 0) || (index >= 3U))
    {
        return false;
    }

    return (ds18b20_read_once(index, temp_c) != 0);
}
