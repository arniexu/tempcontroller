#include "tempcontroller_app.h"

#include "board_pins.h"
#include "tempcontroller_config.h"
#include "tempcontroller_types.h"

#include "ads1220.h"
#include "ec11.h"
#include "oled96x96_i2c.h"

#include "FreeRTOS.h"
#include "event_groups.h"
#include "queue.h"
#include "task.h"

extern I2C_HandleTypeDef OLED_I2C_HANDLE;
extern SPI_HandleTypeDef ADS1220_SPI_HANDLE;

static ads1220_t s_ads1220;
static oled96x96_t s_oled;
static ec11_t s_ec11;
static tempctrl_runtime_t s_runtime;

static QueueHandle_t s_input_queue;
static EventGroupHandle_t s_event_group;

#define EVT_NEW_SAMPLE   (1U << 0)

static void tempcontroller_runtime_init(void)
{
    s_runtime.current_temp_c = 0.0f;
    s_runtime.target_temp_c = TEMPCONTROLLER_DEFAULT_TARGET_C;
    s_runtime.tolerance_c = TEMPCONTROLLER_DEFAULT_TOLERANCE_C;
    s_runtime.heating_on = false;
    s_runtime.state = TEMPCTRL_STATE_IDLE;
    s_runtime.focus = TEMPCTRL_FOCUS_TARGET;
}

static void task_sample(void *arg)
{
    (void)arg;

    for (;;) {
        int32_t raw = 0;
        if (ads1220_read_raw24(&s_ads1220, &raw, TEMPCONTROLLER_SAMPLE_TIMEOUT_MS) == HAL_OK) {
            s_runtime.current_temp_c = ads1220_raw_to_celsius(raw);
            xEventGroupSetBits(s_event_group, EVT_NEW_SAMPLE);
        }
        vTaskDelay(pdMS_TO_TICKS(TEMPCONTROLLER_CONTROL_PERIOD_MS));
    }
}

static void task_input(void *arg)
{
    (void)arg;

    for (;;) {
        tempctrl_input_event_t evt;
        evt.step = ec11_take_step(&s_ec11);
        evt.key_pressed = ec11_take_key_press(&s_ec11);
        if (evt.step != 0 || evt.key_pressed) {
            (void)xQueueSend(s_input_queue, &evt, 0U);
        }
        vTaskDelay(pdMS_TO_TICKS(TEMPCONTROLLER_INPUT_PERIOD_MS));
    }
}

static void task_control(void *arg)
{
    (void)arg;

    tempctrl_input_event_t evt;

    for (;;) {
        EventBits_t bits = xEventGroupWaitBits(s_event_group, EVT_NEW_SAMPLE, pdTRUE, pdFALSE, pdMS_TO_TICKS(TEMPCONTROLLER_CONTROL_PERIOD_MS));
        if ((bits & EVT_NEW_SAMPLE) != 0U) {
            if (s_runtime.current_temp_c < (s_runtime.target_temp_c - s_runtime.tolerance_c)) {
                s_runtime.heating_on = true;
                s_runtime.state = TEMPCTRL_STATE_HEATING;
            } else {
                s_runtime.heating_on = false;
                s_runtime.state = TEMPCTRL_STATE_HOLD;
            }
        }

        while (xQueueReceive(s_input_queue, &evt, 0U) == pdPASS) {
            if (evt.key_pressed) {
                s_runtime.focus = (s_runtime.focus == TEMPCTRL_FOCUS_TARGET) ? TEMPCTRL_FOCUS_TOLERANCE : TEMPCTRL_FOCUS_TARGET;
            }
            if (evt.step != 0) {
                if (s_runtime.focus == TEMPCTRL_FOCUS_TARGET) {
                    s_runtime.target_temp_c += evt.step * 0.1f;
                } else {
                    s_runtime.tolerance_c += evt.step * 0.1f;
                    if (s_runtime.tolerance_c < 0.1f) {
                        s_runtime.tolerance_c = 0.1f;
                    }
                }
            }
        }
    }
}

static void task_display(void *arg)
{
    (void)arg;

    for (;;) {
        (void)oled96x96_show_status(
            &s_oled,
            s_runtime.heating_on,
            s_runtime.current_temp_c,
            s_runtime.target_temp_c,
            s_runtime.tolerance_c);

        vTaskDelay(pdMS_TO_TICKS(TEMPCONTROLLER_DISPLAY_PERIOD_MS));
    }
}

void tempcontroller_app_init(void)
{
    tempcontroller_runtime_init();

    s_ads1220.hspi = &ADS1220_SPI_HANDLE;
    s_ads1220.cs_port = ADS1220_CS_PORT;
    s_ads1220.cs_pin = ADS1220_CS_PIN;
    s_ads1220.drdy_port = ADS1220_DRDY_PORT;
    s_ads1220.drdy_pin = ADS1220_DRDY_PIN;
    s_ads1220.rst_port = ADS1220_RST_PORT;
    s_ads1220.rst_pin = ADS1220_RST_PIN;

    s_oled.hi2c = &OLED_I2C_HANDLE;
    s_oled.addr_7bit = OLED_I2C_ADDR_7BIT;

    s_ec11.port_a = EC11_A_PORT;
    s_ec11.pin_a = EC11_A_PIN;
    s_ec11.port_b = EC11_B_PORT;
    s_ec11.pin_b = EC11_B_PIN;
    s_ec11.port_key = EC11_KEY_PORT;
    s_ec11.pin_key = EC11_KEY_PIN;

    ec11_init(&s_ec11);

    (void)ads1220_init(&s_ads1220);
    (void)oled96x96_init(&s_oled);

    s_input_queue = xQueueCreate(TEMPCONTROLLER_QUEUE_LEN, sizeof(tempctrl_input_event_t));
    s_event_group = xEventGroupCreate();
}

void tempcontroller_app_start(void)
{
    xTaskCreate(task_sample, "sample", 256U, NULL, osPriorityNormal, NULL);
    xTaskCreate(task_control, "control", 256U, NULL, osPriorityAboveNormal, NULL);
    xTaskCreate(task_display, "display", 256U, NULL, osPriorityBelowNormal, NULL);
    xTaskCreate(task_input, "input", 192U, NULL, osPriorityNormal, NULL);
}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    ec11_on_exti(&s_ec11, GPIO_Pin);
}
