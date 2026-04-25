#include "bsp_ds18b20.h"

#include "app_config.h"

#if defined(USE_STDPERIPH_DRIVER)
#include "stm32f10x.h"
#include "stm32f10x_exti.h"
#include "stm32f10x_gpio.h"
#include "misc.h"
#include "stm32f10x_rcc.h"
#include "stm32f10x_tim.h"

#define DS18B20_CMD_SKIP_ROM      (0xCCU)
#define DS18B20_CMD_CONVERT_T     (0x44U)
#define DS18B20_CMD_READ_SCRATCH  (0xBEU)
#define DS18B20_CONVERT_TIMEOUT_MS (750U)
#define DS18B20_READ_RETRY         (2U)

static GPIO_TypeDef *const g_sensor_port[BSP_DS18B20_SENSOR_COUNT] = {GPIOA, GPIOA, GPIOA};
static const uint16_t g_sensor_pin[BSP_DS18B20_SENSOR_COUNT] = {GPIO_Pin_4, GPIO_Pin_5, GPIO_Pin_6};
static int g_delay_timer_ready = 0;
static volatile uint8_t g_presence_fall_seen[BSP_DS18B20_SENSOR_COUNT] = {0U, 0U, 0U};

typedef enum
{
    OW_IRQ_OP_NONE = 0,
    OW_IRQ_OP_RESET,
    OW_IRQ_OP_WRITE_BIT,
    OW_IRQ_OP_READ_BIT
} ow_irq_op_t;

typedef struct
{
    volatile uint8_t active;
    volatile uint8_t done;
    volatile uint8_t success;
    volatile uint8_t stage;
    volatile uint8_t index;
    volatile uint8_t bit;
    volatile uint8_t read_bit;
    volatile ow_irq_op_t op;
} ow_irq_ctx_t;

static volatile ow_irq_ctx_t g_ow_irq = {0U, 0U, 0U, 0U, 0U, 0U, 0U, OW_IRQ_OP_NONE};

static uint8_t ow_read_pin(uint8_t index);

static void ds18b20_diag_inc(uint32_t *counter)
{
    if (*counter < 0xFFFFFFFFUL)
    {
        (*counter)++;
    }
}

static uint32_t ds18b20_exti_line_for_index(uint8_t index)
{
    switch (index)
    {
    case 0U:
        return EXTI_Line4;
    case 1U:
        return EXTI_Line5;
    case 2U:
        return EXTI_Line6;
    default:
        return 0U;
    }
}

static void ds18b20_ic_clear(uint8_t index)
{
    uint32_t line = ds18b20_exti_line_for_index(index);

    if (line == 0U)
    {
        return;
    }

    g_presence_fall_seen[index] = 0U;
    EXTI_ClearITPendingBit(line);
}

static void ds18b20_ic_enable(uint8_t index, int enable)
{
    EXTI_InitTypeDef exti;
    uint32_t line = ds18b20_exti_line_for_index(index);

    if (line == 0U)
    {
        return;
    }

    EXTI_StructInit(&exti);
    exti.EXTI_Line = line;
    exti.EXTI_Mode = EXTI_Mode_Interrupt;
    exti.EXTI_Trigger = EXTI_Trigger_Falling;
    exti.EXTI_LineCmd = (enable != 0) ? ENABLE : DISABLE;
    EXTI_Init(&exti);
}

static void ds18b20_cc4_schedule(uint16_t delta_us)
{
    uint16_t now = (uint16_t)TIM_GetCounter(TIM4);
    uint16_t next = (uint16_t)(now + delta_us);

    TIM_SetCompare4(TIM4, next);
    TIM_ClearITPendingBit(TIM4, TIM_IT_CC4);
    TIM_ITConfig(TIM4, TIM_IT_CC4, ENABLE);
}

static void ds18b20_cc4_stop(void)
{
    TIM_ITConfig(TIM4, TIM_IT_CC4, DISABLE);
    TIM_ClearITPendingBit(TIM4, TIM_IT_CC4);
}

static bool ds18b20_wait_irq_done(uint32_t timeout_us)
{
    uint32_t start = TIM_GetCounter(TIM2);

    while ((uint32_t)(TIM_GetCounter(TIM2) - start) < timeout_us)
    {
        if (g_ow_irq.done != 0U)
        {
            return (g_ow_irq.success != 0U);
        }

        if (__get_PRIMASK() == 0U)
        {
            __WFI();
        }
        else
        {
            __NOP();
        }
    }

    g_ow_irq.active = 0U;
    g_ow_irq.done = 1U;
    g_ow_irq.success = 0U;
    g_ow_irq.op = OW_IRQ_OP_NONE;
    ds18b20_diag_inc(&g_diag.irq_timeout_count);
    ds18b20_cc4_stop();
    return false;
}

static bool ds18b20_wait_line_high(uint8_t index, uint32_t timeout_us)
{
    uint32_t start = TIM_GetCounter(TIM2);

    while ((uint32_t)(TIM_GetCounter(TIM2) - start) < timeout_us)
    {
        if (ow_read_pin(index) != 0U)
        {
            return true;
        }

        if (__get_PRIMASK() == 0U)
        {
            __WFI();
        }
        else
        {
            __NOP();
        }
    }

    if (ow_read_pin(index) == 0U)
    {
        ds18b20_diag_inc(&g_diag.bus_stuck_low_count[index]);
    }

    return false;
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
    bool ok;

    if (g_ow_irq.active != 0U)
    {
        return false;
    }

    ds18b20_ic_clear(index);
    ds18b20_ic_enable(index, 1);

    ow_drive_low(index);

    g_ow_irq.index = index;
    g_ow_irq.op = OW_IRQ_OP_RESET;
    g_ow_irq.stage = 0U;
    g_ow_irq.success = 0U;
    g_ow_irq.done = 0U;
    g_ow_irq.active = 1U;

    ds18b20_cc4_schedule(480U);
    ok = ds18b20_wait_irq_done(1200U);
    if (!ok)
    {
        ds18b20_diag_inc(&g_diag.reset_timeout_count[index]);
    }
    return ok;
}

static void ow_write_bit(uint8_t index, uint8_t bit)
{
    if (g_ow_irq.active != 0U)
    {
        return;
    }

    ow_drive_low(index);
    g_ow_irq.index = index;
    g_ow_irq.bit = (bit != 0U) ? 1U : 0U;
    g_ow_irq.op = OW_IRQ_OP_WRITE_BIT;
    g_ow_irq.stage = 0U;
    g_ow_irq.success = 0U;
    g_ow_irq.done = 0U;
    g_ow_irq.active = 1U;

    ds18b20_cc4_schedule((g_ow_irq.bit != 0U) ? 6U : 60U);
    (void)ds18b20_wait_irq_done(200U);
}

static uint8_t ow_read_bit(uint8_t index)
{
    if (g_ow_irq.active != 0U)
    {
        return 1U;
    }

    ow_drive_low(index);
    g_ow_irq.index = index;
    g_ow_irq.read_bit = 1U;
    g_ow_irq.op = OW_IRQ_OP_READ_BIT;
    g_ow_irq.stage = 0U;
    g_ow_irq.success = 0U;
    g_ow_irq.done = 0U;
    g_ow_irq.active = 1U;

    ds18b20_cc4_schedule(6U);
    if (!ds18b20_wait_irq_done(200U))
    {
        return 1U;
    }

    return g_ow_irq.read_bit;
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
    bool done = ds18b20_wait_line_high(index, (uint32_t)DS18B20_CONVERT_TIMEOUT_MS * 1000U);

    if (!done)
    {
        ds18b20_diag_inc(&g_diag.convert_timeout_count[index]);
    }

    return done;
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
        ds18b20_diag_inc(&g_diag.crc_error_count[index]);
        return false;
    }

    raw = (int16_t)((((uint16_t)scratch[1]) << 8U) | scratch[0]);
    *temp_c = (float)raw / 16.0f;
    return true;
}
#endif

static bsp_ds18b20_presence_mode_t g_presence_mode = BSP_DS18B20_PRESENCE_IRQ_ONLY;
static bsp_ds18b20_diag_t g_diag = {0};

static float g_mock_temp[BSP_DS18B20_SENSOR_COUNT] = {25.0f, 25.2f, 24.9f};
static bool g_mock_valid[BSP_DS18B20_SENSOR_COUNT] = {true, true, true};

void bsp_ds18b20_init(void)
{
#if defined(USE_STDPERIPH_DRIVER)
    GPIO_InitTypeDef gpio;
    TIM_TimeBaseInitTypeDef tim;
    EXTI_InitTypeDef exti;
    NVIC_InitTypeDef nvic;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_AFIO, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2 | RCC_APB1Periph_TIM4, ENABLE);

    gpio.GPIO_Pin = GPIO_Pin_4 | GPIO_Pin_5 | GPIO_Pin_6;
    gpio.GPIO_Mode = GPIO_Mode_IPU;
    gpio.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &gpio);

    GPIO_EXTILineConfig(GPIO_PortSourceGPIOA, GPIO_PinSource4);
    GPIO_EXTILineConfig(GPIO_PortSourceGPIOA, GPIO_PinSource5);
    GPIO_EXTILineConfig(GPIO_PortSourceGPIOA, GPIO_PinSource6);

    EXTI_StructInit(&exti);
    exti.EXTI_Mode = EXTI_Mode_Interrupt;
    exti.EXTI_Trigger = EXTI_Trigger_Falling;
    exti.EXTI_LineCmd = DISABLE;
    exti.EXTI_Line = EXTI_Line4;
    EXTI_Init(&exti);
    exti.EXTI_Line = EXTI_Line5;
    EXTI_Init(&exti);
    exti.EXTI_Line = EXTI_Line6;
    EXTI_Init(&exti);

    TIM_TimeBaseStructInit(&tim);
    tim.TIM_Prescaler = (uint16_t)((SystemCoreClock / 1000000U) - 1U);
    tim.TIM_Period = 0xFFFFU;
    tim.TIM_ClockDivision = TIM_CKD_DIV1;
    tim.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseInit(TIM2, &tim);
    TIM_Cmd(TIM2, ENABLE);

    TIM_TimeBaseStructInit(&tim);
    tim.TIM_Prescaler = (uint16_t)((SystemCoreClock / 1000000U) - 1U);
    tim.TIM_Period = 0xFFFFU;
    tim.TIM_ClockDivision = TIM_CKD_DIV1;
    tim.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseInit(TIM4, &tim);

    TIM_ITConfig(TIM4, TIM_IT_CC4, DISABLE);

    nvic.NVIC_IRQChannel = TIM4_IRQn;
    nvic.NVIC_IRQChannelPreemptionPriority = 2U;
    nvic.NVIC_IRQChannelSubPriority = 1U;
    nvic.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&nvic);

    nvic.NVIC_IRQChannel = EXTI4_IRQn;
    nvic.NVIC_IRQChannelPreemptionPriority = 2U;
    nvic.NVIC_IRQChannelSubPriority = 2U;
    nvic.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&nvic);

    nvic.NVIC_IRQChannel = EXTI9_5_IRQn;
    nvic.NVIC_IRQChannelPreemptionPriority = 2U;
    nvic.NVIC_IRQChannelSubPriority = 3U;
    nvic.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&nvic);

    TIM_Cmd(TIM4, ENABLE);
    g_delay_timer_ready = 1;
#endif

    g_presence_mode = BSP_DS18B20_PRESENCE_IRQ_ONLY;
    bsp_ds18b20_reset_diag();
}

#if defined(USE_STDPERIPH_DRIVER)
void EXTI4_IRQHandler(void)
{
    if (EXTI_GetITStatus(EXTI_Line4) != RESET)
    {
        g_presence_fall_seen[0] = 1U;
        EXTI_ClearITPendingBit(EXTI_Line4);
    }
}

void EXTI9_5_IRQHandler(void)
{
    if (EXTI_GetITStatus(EXTI_Line5) != RESET)
    {
        g_presence_fall_seen[1] = 1U;
        EXTI_ClearITPendingBit(EXTI_Line5);
    }

    if (EXTI_GetITStatus(EXTI_Line6) != RESET)
    {
        g_presence_fall_seen[2] = 1U;
        EXTI_ClearITPendingBit(EXTI_Line6);
    }
}

void TIM4_IRQHandler(void)
{
    if (TIM_GetITStatus(TIM4, TIM_IT_CC4) != RESET)
    {
        TIM_ClearITPendingBit(TIM4, TIM_IT_CC4);

        if (g_ow_irq.active == 0U)
        {
            ds18b20_cc4_stop();
            return;
        }

        switch (g_ow_irq.op)
        {
        case OW_IRQ_OP_RESET:
            if (g_ow_irq.stage == 0U)
            {
                ow_release(g_ow_irq.index);
                g_ow_irq.stage = 1U;
                ds18b20_cc4_schedule(70U);
            }
            else if (g_ow_irq.stage == 1U)
            {
                g_ow_irq.success = (g_presence_fall_seen[g_ow_irq.index] != 0U) ? 1U : 0U;
                ds18b20_ic_enable(g_ow_irq.index, 0);
                g_ow_irq.stage = 2U;
                ds18b20_cc4_schedule(410U);
            }
            else
            {
                g_ow_irq.active = 0U;
                g_ow_irq.done = 1U;
                g_ow_irq.op = OW_IRQ_OP_NONE;
                ds18b20_cc4_stop();
            }
            break;

        case OW_IRQ_OP_WRITE_BIT:
            if (g_ow_irq.stage == 0U)
            {
                ow_release(g_ow_irq.index);
                g_ow_irq.stage = 1U;
                ds18b20_cc4_schedule((g_ow_irq.bit != 0U) ? 64U : 10U);
            }
            else
            {
                g_ow_irq.success = 1U;
                g_ow_irq.active = 0U;
                g_ow_irq.done = 1U;
                g_ow_irq.op = OW_IRQ_OP_NONE;
                ds18b20_cc4_stop();
            }
            break;

        case OW_IRQ_OP_READ_BIT:
            if (g_ow_irq.stage == 0U)
            {
                ow_release(g_ow_irq.index);
                g_ow_irq.stage = 1U;
                ds18b20_cc4_schedule(9U);
            }
            else if (g_ow_irq.stage == 1U)
            {
                g_ow_irq.read_bit = ow_read_pin(g_ow_irq.index);
                g_ow_irq.stage = 2U;
                ds18b20_cc4_schedule(55U);
            }
            else
            {
                g_ow_irq.success = 1U;
                g_ow_irq.active = 0U;
                g_ow_irq.done = 1U;
                g_ow_irq.op = OW_IRQ_OP_NONE;
                ds18b20_cc4_stop();
            }
            break;

        default:
            g_ow_irq.success = 0U;
            g_ow_irq.active = 0U;
            g_ow_irq.done = 1U;
            g_ow_irq.op = OW_IRQ_OP_NONE;
            ds18b20_cc4_stop();
            break;
        }
    }
}
#endif

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

    ds18b20_diag_inc(&g_diag.read_fail_count[index]);
    return false;
#endif
    (void)index;
    return false;
#endif
}

void bsp_ds18b20_set_presence_mode(bsp_ds18b20_presence_mode_t mode)
{
    if (mode == BSP_DS18B20_PRESENCE_IRQ_DMA)
    {
        /* PA4/PA5/PA6 remap uses EXTI presence detection only. */
        g_presence_mode = BSP_DS18B20_PRESENCE_IRQ_ONLY;
        g_diag.presence_mode = BSP_DS18B20_PRESENCE_IRQ_ONLY;
        return;
    }

    if (mode != BSP_DS18B20_PRESENCE_IRQ_ONLY)
    {
        return;
    }

    g_presence_mode = mode;
    g_diag.presence_mode = mode;
}

bsp_ds18b20_presence_mode_t bsp_ds18b20_get_presence_mode(void)
{
    return g_presence_mode;
}

void bsp_ds18b20_get_diag(bsp_ds18b20_diag_t *diag)
{
    if (diag == 0)
    {
        return;
    }

    *diag = g_diag;
    diag->presence_mode = g_presence_mode;
}

void bsp_ds18b20_reset_diag(void)
{
    uint8_t i;

    for (i = 0U; i < BSP_DS18B20_SENSOR_COUNT; ++i)
    {
        g_diag.reset_timeout_count[i] = 0U;
        g_diag.convert_timeout_count[i] = 0U;
        g_diag.crc_error_count[i] = 0U;
        g_diag.read_fail_count[i] = 0U;
        g_diag.bus_stuck_low_count[i] = 0U;
    }

    g_diag.irq_timeout_count = 0U;
    g_diag.dma_error_count = 0U;
    g_diag.presence_mode = g_presence_mode;
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
