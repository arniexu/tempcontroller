#include "bsp_key.h"

#if defined(USE_STDPERIPH_DRIVER)
#include "stm32f10x_exti.h"
#include "stm32f10x_gpio.h"
#include "misc.h"
#include "stm32f10x_rcc.h"
#endif

#if !defined(USE_STDPERIPH_DRIVER)
static bool g_mock_pressed[BSP_KEY_COUNT] = {false, false, false, false};
#endif

#if defined(USE_STDPERIPH_DRIVER)
static volatile bool g_key_pressed[BSP_KEY_COUNT] = {false, false, false, false};

static uint16_t key_to_pin(bsp_key_id_t key)
{
    switch (key)
    {
    case BSP_KEY_SET:
        return GPIO_Pin_0;
    case BSP_KEY_UP:
        return GPIO_Pin_1;
    case BSP_KEY_DOWN:
        return GPIO_Pin_2;
    case BSP_KEY_BACK:
        return GPIO_Pin_3;
    default:
        return 0U;
    }
}

static void key_refresh_state(bsp_key_id_t key)
{
    uint16_t pin = key_to_pin(key);

    if (pin == 0U)
    {
        return;
    }

    g_key_pressed[(unsigned int)key] = (GPIO_ReadInputDataBit(GPIOA, pin) == Bit_RESET);
}

static void key_exti_common(uint32_t line, bsp_key_id_t key)
{
    if (EXTI_GetITStatus(line) != RESET)
    {
        key_refresh_state(key);
        EXTI_ClearITPendingBit(line);
    }
}
#endif

void bsp_key_init(void)
{
#if defined(USE_STDPERIPH_DRIVER)
    EXTI_InitTypeDef exti;
    GPIO_InitTypeDef gpio;
    NVIC_InitTypeDef nvic;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_AFIO, ENABLE);

    gpio.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_2 | GPIO_Pin_3;
    gpio.GPIO_Mode = GPIO_Mode_IPU;
    gpio.GPIO_Speed = GPIO_Speed_2MHz;
    GPIO_Init(GPIOA, &gpio);

    GPIO_EXTILineConfig(GPIO_PortSourceGPIOA, GPIO_PinSource0);
    GPIO_EXTILineConfig(GPIO_PortSourceGPIOA, GPIO_PinSource1);
    GPIO_EXTILineConfig(GPIO_PortSourceGPIOA, GPIO_PinSource2);
    GPIO_EXTILineConfig(GPIO_PortSourceGPIOA, GPIO_PinSource3);

    EXTI_StructInit(&exti);
    exti.EXTI_Mode = EXTI_Mode_Interrupt;
    exti.EXTI_Trigger = EXTI_Trigger_Rising_Falling;
    exti.EXTI_LineCmd = ENABLE;

    exti.EXTI_Line = EXTI_Line0;
    EXTI_Init(&exti);
    exti.EXTI_Line = EXTI_Line1;
    EXTI_Init(&exti);
    exti.EXTI_Line = EXTI_Line2;
    EXTI_Init(&exti);
    exti.EXTI_Line = EXTI_Line3;
    EXTI_Init(&exti);

    nvic.NVIC_IRQChannelPreemptionPriority = 3U;
    nvic.NVIC_IRQChannelSubPriority = 0U;
    nvic.NVIC_IRQChannelCmd = ENABLE;

    nvic.NVIC_IRQChannel = EXTI0_IRQn;
    NVIC_Init(&nvic);
    nvic.NVIC_IRQChannel = EXTI1_IRQn;
    NVIC_Init(&nvic);
    nvic.NVIC_IRQChannel = EXTI2_IRQn;
    NVIC_Init(&nvic);
    nvic.NVIC_IRQChannel = EXTI3_IRQn;
    NVIC_Init(&nvic);

    key_refresh_state(BSP_KEY_SET);
    key_refresh_state(BSP_KEY_UP);
    key_refresh_state(BSP_KEY_DOWN);
    key_refresh_state(BSP_KEY_BACK);
#endif
}

bool bsp_key_get_state(bsp_key_id_t key)
{
    if ((unsigned int)key >= (unsigned int)BSP_KEY_COUNT)
    {
        return false;
    }

#if defined(USE_STDPERIPH_DRIVER)
    return g_key_pressed[(unsigned int)key];
#else
    return g_mock_pressed[(unsigned int)key];
#endif
}

void bsp_key_mock_set_state(bsp_key_id_t key, bool pressed)
{
#if defined(USE_STDPERIPH_DRIVER)
    (void)key;
    (void)pressed;
#else
    if ((unsigned int)key >= (unsigned int)BSP_KEY_COUNT)
    {
        return;
    }

    g_mock_pressed[(unsigned int)key] = pressed;
#endif
}

#if defined(USE_STDPERIPH_DRIVER)
void EXTI0_IRQHandler(void)
{
    key_exti_common(EXTI_Line0, BSP_KEY_SET);
}

void EXTI1_IRQHandler(void)
{
    key_exti_common(EXTI_Line1, BSP_KEY_UP);
}

void EXTI2_IRQHandler(void)
{
    key_exti_common(EXTI_Line2, BSP_KEY_DOWN);
}

void EXTI3_IRQHandler(void)
{
    key_exti_common(EXTI_Line3, BSP_KEY_BACK);
}
#endif
