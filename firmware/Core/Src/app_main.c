#include "app_main.h"

#include "alarm_service.h"
#include "app_config.h"
#include "app_state.h"
#include "debug_log.h"
#include "heater_ctrl.h"
#include "hw_platform_port.h"
#include "log_service.h"
#include "param_store.h"
#include "pid_ctrl.h"
#include "protocol_export.h"
#include "schedule_service.h"
#include "scheduler.h"
#include "temp_manager.h"
#include "ui_lvgl_port.h"
#include "ui_lvgl_view.h"
#include "ui_service.h"
#include "hw_temp_port.h"

#include <stdio.h>
#include <string.h>

static app_mode_t g_mode = APP_MODE_IDLE;
static app_mode_t g_prev_mode = APP_MODE_IDLE;
static pid_ctx_t g_pid;
static float g_pid_out = 0.0f;
static bool g_prev_alarm = false;
static unsigned int g_log_tick_count = 0U;
static app_params_t g_runtime_params;
static uint32_t g_last_1ms_tick = 0U;
#if (APP_USE_LVGL_UI == 1U)
static uint32_t g_last_lvgl_task_ms = 0U;
#endif
static bool g_hw_driver_test_active = false;

typedef enum
{
    HW_DISPLAY_TEST_SOLID = 0,
    HW_DISPLAY_TEST_MOVING_BLOCK,
    HW_DISPLAY_TEST_API_PATTERN
} hw_display_test_phase_t;

static hw_display_test_phase_t g_hw_test_phase = HW_DISPLAY_TEST_SOLID;
static uint32_t g_hw_test_phase_started_ms = 0U;
static uint32_t g_hw_test_last_step_ms = 0U;
static uint8_t g_hw_test_solid_idx = 0U;
static uint16_t g_hw_test_block_x = 12U;
static uint16_t g_hw_test_block_y = 56U;
static uint16_t g_hw_test_prev_block_x = 12U;
static uint16_t g_hw_test_prev_block_y = 56U;
static int16_t g_hw_test_block_dx = 4;
static int16_t g_hw_test_block_dy = 3;
static bool g_hw_test_block_inited = false;

typedef enum
{
    HW_SMOKE_STAGE_DISPLAY = 0,
    HW_SMOKE_STAGE_KEYS,
    HW_SMOKE_STAGE_ACTUATORS,
    HW_SMOKE_STAGE_TEMP,
    HW_SMOKE_STAGE_SPIFLASH,
    HW_SMOKE_STAGE_DONE
} hw_smoke_stage_t;

static hw_smoke_stage_t g_hw_smoke_stage = HW_SMOKE_STAGE_DISPLAY;
static uint32_t g_hw_smoke_stage_started_ms = 0U;
static uint8_t g_hw_smoke_stage_entered = 0U;
static uint8_t g_hw_smoke_key_seen_mask = 0U;
static uint8_t g_hw_smoke_key_required_mask = 0x07U;
static uint8_t g_hw_smoke_key_baseline_mask = 0U;
static uint8_t g_hw_smoke_key_stuck_mask = 0U;
static uint8_t g_hw_smoke_pass_count = 0U;
static uint8_t g_hw_smoke_fail_count = 0U;
static uint8_t g_hw_smoke_last_key_raw_mask = 0U;
static uint32_t g_hw_smoke_rand_state = 0x13572468UL;
static uint32_t g_hw_smoke_last_temp_log_ms = 0U;
static uint8_t g_hw_smoke_spi_choice = 0U;
static uint32_t g_hw_smoke_spi_addr = 0x1000UL;
static uint8_t g_hw_smoke_spi_tx[3][8];
static uint8_t g_hw_smoke_spi_rx[8];
static uint8_t g_hw_smoke_spi_done = 0U;
static uint8_t g_hw_smoke_buzzer_on = 0U;
static uint8_t g_hw_smoke_relay_on = 0U;
static uint8_t g_hw_smoke_buzzer_tested = 0U;
static uint8_t g_hw_smoke_relay_tested = 0U;
static uint8_t g_hw_smoke_temp_port_inited = 0U;
static const char *g_hw_smoke_last_key_name = "NONE";

#define HW_TEST_SOLID_STEP_MS        (800U)
#define HW_TEST_SOLID_TOTAL_MS       (6400U)
#define HW_TEST_BLOCK_STEP_MS        (33U)
#define HW_TEST_BLOCK_TOTAL_MS       (12000U)
#define HW_TEST_API_TOTAL_MS         (4000U)
#define HW_TEST_BLOCK_W              (44U)
#define HW_TEST_BLOCK_H              (44U)
#define HW_TEST_DIRTY_MARGIN         (2U)
#define HW_TEST_BLOCK_MIN_X          (4U)
#define HW_TEST_BLOCK_MIN_Y          (44U)
#define HW_TEST_BLOCK_MAX_X          (BSP_LCD_WIDTH - HW_TEST_BLOCK_W - 4U)
#define HW_TEST_BLOCK_MAX_Y          (BSP_LCD_HEIGHT - HW_TEST_BLOCK_H - 4U)
#define HW_SMOKE_DISPLAY_MS          (5000U)
#define HW_SMOKE_SPI_OPTION_COUNT    (3U)
#define HW_SMOKE_SPI_DATA_LEN        (8U)
#define HW_SMOKE_SPI_STEP_ADDR       (0x100UL)
#define HW_SMOKE_TEMP_LOG_MS         (500U)
#define HW_SMOKE_KEY_REQUIRED_MASK   (0x07U)
#define HW_SMOKE_KEY_STUCK_MS        (800U)
#define HW_SMOKE_KEY_DEGRADED_MS     (2500U)

static void run_display_driver_hw_test_loop(void);

static const char *hw_smoke_stage_to_str(hw_smoke_stage_t stage)
{
    switch (stage)
    {
    case HW_SMOKE_STAGE_DISPLAY: return "DISPLAY";
    case HW_SMOKE_STAGE_KEYS: return "KEYS";
    case HW_SMOKE_STAGE_ACTUATORS: return "ACT";
    case HW_SMOKE_STAGE_TEMP: return "TEMP";
    case HW_SMOKE_STAGE_SPIFLASH: return "SPIFLASH";
    case HW_SMOKE_STAGE_DONE: return "DONE";
    default: return "UNKNOWN";
    }
}

static uint8_t hw_smoke_read_key_raw_mask(void)
{
    uint8_t raw_mask = 0U;

    if (hw_key_get_state(HW_KEY_SET))
    {
        raw_mask |= 0x01U;
    }
    if (hw_key_get_state(HW_KEY_UP))
    {
        raw_mask |= 0x02U;
    }
    if (hw_key_get_state(HW_KEY_DOWN))
    {
        raw_mask |= 0x04U;
    }

    return raw_mask;
}

static uint8_t hw_smoke_take_key_press_mask(void)
{
    uint8_t raw_mask = hw_smoke_read_key_raw_mask();
    uint8_t pressed_mask = (uint8_t)(raw_mask & (uint8_t)(~g_hw_smoke_last_key_raw_mask));

    if (raw_mask != g_hw_smoke_last_key_raw_mask)
    {
        debug_log_info("APP", "hw_smoke keys raw=%u%u%u",
                       (raw_mask & 0x01U) ? 1U : 0U,
                       (raw_mask & 0x02U) ? 1U : 0U,
                       (raw_mask & 0x04U) ? 1U : 0U);
        g_hw_smoke_last_key_raw_mask = raw_mask;
    }

    return pressed_mask;
}

static const char *hw_smoke_key_name_from_mask(uint8_t key_mask)
{
    switch (key_mask)
    {
    case 0x01U: return "SET";
    case 0x02U: return "UP";
    case 0x04U: return "DOWN";
    case 0x00U: return "NONE";
    default: return "MULTI";
    }
}

static uint32_t hw_smoke_rand_next(uint32_t salt)
{
    g_hw_smoke_rand_state = (g_hw_smoke_rand_state * 1664525UL) + 1013904223UL + salt;
    return g_hw_smoke_rand_state;
}

static void hw_smoke_fill_random_bytes(uint8_t *buf, uint8_t len, uint32_t salt)
{
    uint8_t i;
    uint32_t value = hw_smoke_rand_next(salt);

    for (i = 0U; i < len; ++i)
    {
        if ((i & 0x03U) == 0U)
        {
            value = hw_smoke_rand_next((uint32_t)i + salt);
        }
        buf[i] = (uint8_t)(value >> ((i & 0x03U) * 8U));
    }
}

static void hw_smoke_format_preview4(char *buf, size_t buf_size, const uint8_t *data)
{
    (void)snprintf(buf,
                   buf_size,
                   "%02X %02X %02X %02X",
                   (unsigned int)data[0],
                   (unsigned int)data[1],
                   (unsigned int)data[2],
                   (unsigned int)data[3]);
}

static void hw_smoke_prepare_spi_vectors(uint32_t now_ms)
{
    uint8_t option;

    g_hw_smoke_spi_choice = 0U;
    g_hw_smoke_spi_done = 0U;
    g_hw_smoke_spi_addr = 0x1000UL + ((hw_smoke_rand_next(now_ms) % 8UL) * HW_SMOKE_SPI_STEP_ADDR);
    memset(g_hw_smoke_spi_rx, 0, sizeof(g_hw_smoke_spi_rx));

    for (option = 0U; option < HW_SMOKE_SPI_OPTION_COUNT; ++option)
    {
        hw_smoke_fill_random_bytes(g_hw_smoke_spi_tx[option], HW_SMOKE_SPI_DATA_LEN, now_ms + option);
        debug_log_info("APP",
                       "hw_smoke SPI opt%u addr=0x%04lX tx=%02X %02X %02X %02X %02X %02X %02X %02X",
                       (unsigned int)(option + 1U),
                       (unsigned long)g_hw_smoke_spi_addr,
                       (unsigned int)g_hw_smoke_spi_tx[option][0],
                       (unsigned int)g_hw_smoke_spi_tx[option][1],
                       (unsigned int)g_hw_smoke_spi_tx[option][2],
                       (unsigned int)g_hw_smoke_spi_tx[option][3],
                       (unsigned int)g_hw_smoke_spi_tx[option][4],
                       (unsigned int)g_hw_smoke_spi_tx[option][5],
                       (unsigned int)g_hw_smoke_spi_tx[option][6],
                       (unsigned int)g_hw_smoke_spi_tx[option][7]);
    }
}

static void hw_smoke_show_lines(const char *l0,
                                const char *l1,
                                const char *l2,
                                const char *l3)
{
    uint8_t guard = 0U;

    hw_oled_draw_text(0U, l0);
    hw_oled_draw_text(1U, l1);
    hw_oled_draw_text(2U, l2);
    hw_oled_draw_text(3U, l3);
    hw_oled_refresh();
    while ((hw_oled_process() != 0) && (guard < 8U))
    {
        guard++;
    }
}

static void hw_smoke_boot_stage(const char *line1, const char *line2)
{
    uint8_t guard = 0U;

    hw_oled_draw_text(0U, "HW TEST BOOT");
    hw_oled_draw_text(1U, line1);
    hw_oled_draw_text(2U, line2);
    hw_oled_draw_text(3U, "WAIT...");
    hw_oled_refresh();
    while ((hw_oled_process() != 0) && (guard < 8U))
    {
        guard++;
    }
}

static void hw_smoke_refresh_blocking(void)
{
    uint8_t guard = 0U;

    hw_oled_refresh();
    while ((hw_oled_process() != 0) && (guard < 8U))
    {
        guard++;
    }
}

static void hw_smoke_next_stage(hw_smoke_stage_t stage, uint32_t now_ms)
{
    debug_log_info("APP", "hw_smoke stage %s -> %s at %lums",
                   hw_smoke_stage_to_str(g_hw_smoke_stage),
                   hw_smoke_stage_to_str(stage),
                   (unsigned long)now_ms);
    g_hw_smoke_stage = stage;
    g_hw_smoke_stage_started_ms = now_ms;
    g_hw_smoke_stage_entered = 0U;
}

static void hw_smoke_record_result(int pass)
{
    if (pass != 0)
    {
        if (g_hw_smoke_pass_count < 255U)
        {
            g_hw_smoke_pass_count++;
        }
    }
    else
    {
        if (g_hw_smoke_fail_count < 255U)
        {
            g_hw_smoke_fail_count++;
        }
    }
}

static void run_all_hw_driver_smoke_test_loop(void)
{
    uint32_t now_ms = scheduler_now_ms();
    uint8_t pressed_mask = hw_smoke_take_key_press_mask();
    uint8_t raw_mask = g_hw_smoke_last_key_raw_mask;
    uint8_t degraded_keys_ok = 0U;
    char line2[24];
    char line3[24];

    switch (g_hw_smoke_stage)
    {
    case HW_SMOKE_STAGE_DISPLAY:
        if (g_hw_smoke_stage_entered == 0U)
        {
            g_hw_smoke_stage_entered = 1U;
            debug_log_info("APP", "hw_smoke enter DISPLAY");
            g_hw_test_phase = HW_DISPLAY_TEST_SOLID;
            g_hw_test_phase_started_ms = now_ms;
            g_hw_test_last_step_ms = 0U;
            g_hw_test_solid_idx = 0U;
            hw_smoke_show_lines("HW SMOKE: DISPLAY",
                                "RUN PATTERN 5S",
                                "CHECK COLOR/MOVE",
                                "NO GLITCH");
        }

        run_display_driver_hw_test_loop();
        (void)snprintf(line2,
                       sizeof(line2),
                       "T %lus/%lus",
                       (unsigned long)((now_ms - g_hw_smoke_stage_started_ms) / 1000U),
                       (unsigned long)(HW_SMOKE_DISPLAY_MS / 1000U));
        hw_oled_draw_text(2U, line2);
        hw_smoke_refresh_blocking();
        if ((now_ms - g_hw_smoke_stage_started_ms) >= HW_SMOKE_DISPLAY_MS)
        {
            hw_smoke_record_result(1);
            debug_log_info("APP", "hw_smoke DISPLAY pass");
            hw_oled_clear();
            hw_smoke_next_stage(HW_SMOKE_STAGE_KEYS, now_ms);
        }
        break;

    case HW_SMOKE_STAGE_KEYS:
        if (g_hw_smoke_stage_entered == 0U)
        {
            g_hw_smoke_stage_entered = 1U;
            g_hw_smoke_key_seen_mask = 0U;
            g_hw_smoke_key_required_mask = HW_SMOKE_KEY_REQUIRED_MASK;
            g_hw_smoke_key_stuck_mask = 0U;
            g_hw_smoke_key_baseline_mask = hw_smoke_read_key_raw_mask();
            g_hw_smoke_last_key_raw_mask = g_hw_smoke_key_baseline_mask;
            raw_mask = g_hw_smoke_last_key_raw_mask;
            pressed_mask = 0U;
            g_hw_smoke_last_key_name = "NONE";
            debug_log_info("APP", "hw_smoke enter KEYS require_mask=0x%02X",
                           (unsigned int)HW_SMOKE_KEY_REQUIRED_MASK);
            if (g_hw_smoke_key_baseline_mask != 0U)
            {
                debug_log_info("APP", "hw_smoke keys baseline=0x%02X",
                               (unsigned int)g_hw_smoke_key_baseline_mask);
            }
        }

        if (((now_ms - g_hw_smoke_stage_started_ms) >= HW_SMOKE_KEY_STUCK_MS) &&
            (g_hw_smoke_key_baseline_mask != 0U) &&
            (g_hw_smoke_key_stuck_mask == 0U))
        {
            g_hw_smoke_key_stuck_mask = (uint8_t)(raw_mask & g_hw_smoke_key_baseline_mask);
            if (g_hw_smoke_key_stuck_mask != 0U)
            {
                g_hw_smoke_key_required_mask = (uint8_t)(HW_SMOKE_KEY_REQUIRED_MASK & (uint8_t)(~g_hw_smoke_key_stuck_mask));
                debug_log_info("APP", "hw_smoke keys stuck=0x%02X req=0x%02X",
                               (unsigned int)g_hw_smoke_key_stuck_mask,
                               (unsigned int)g_hw_smoke_key_required_mask);
            }
        }

        if (pressed_mask != 0U)
        {
            g_hw_smoke_key_seen_mask |= pressed_mask;
            g_hw_smoke_last_key_name = hw_smoke_key_name_from_mask(pressed_mask);
            debug_log_info("APP", "hw_smoke key press=%s seen=0x%02X",
                           g_hw_smoke_last_key_name,
                           (unsigned int)g_hw_smoke_key_seen_mask);
        }

        degraded_keys_ok = (uint8_t)((g_hw_smoke_key_stuck_mask == 0x02U) &&
                         ((g_hw_smoke_key_seen_mask & 0x01U) == 0U) &&
                         ((now_ms - g_hw_smoke_stage_started_ms) >= HW_SMOKE_KEY_DEGRADED_MS));

        hw_oled_draw_text(0U, "HW SMOKE: KEYS");
        if (strcmp(g_hw_smoke_last_key_name, "NONE") == 0)
        {
            hw_oled_draw_text(1U, "LAST NONE");
        }
        else
        {
            char key_line[24];

            (void)snprintf(key_line, sizeof(key_line), "LAST %s", g_hw_smoke_last_key_name);
            hw_oled_draw_text(1U, key_line);
        }
        (void)snprintf(line2,
                       sizeof(line2),
                       "RAW %u%u%u S%u%u%u",
                       (raw_mask & 0x01U) ? 1U : 0U,
                       (raw_mask & 0x02U) ? 1U : 0U,
                       (raw_mask & 0x04U) ? 1U : 0U,
                       (g_hw_smoke_key_seen_mask & 0x01U) ? 1U : 0U,
                       (g_hw_smoke_key_seen_mask & 0x02U) ? 1U : 0U,
                       (g_hw_smoke_key_seen_mask & 0x04U) ? 1U : 0U);
        (void)snprintf(line3,
                       sizeof(line3),
                       "%s",
                       ((g_hw_smoke_key_seen_mask & g_hw_smoke_key_required_mask) == g_hw_smoke_key_required_mask) ? "SET NEXT" :
                       (degraded_keys_ok != 0U) ? "UP BAD DN NEXT" :
                       ((g_hw_smoke_key_stuck_mask & 0x02U) != 0U) ? "UP STUCK? PA0" :
                       "PRESS ALL 3");
        hw_oled_draw_text(2U, line2);
        hw_oled_draw_text(3U, line3);
        hw_smoke_refresh_blocking();

        if (((g_hw_smoke_key_seen_mask & g_hw_smoke_key_required_mask) == g_hw_smoke_key_required_mask) &&
            ((pressed_mask & 0x01U) != 0U))
        {
            hw_smoke_record_result(1);
            debug_log_info("APP", "hw_smoke KEYS pass mask=0x%02X",
                           (unsigned int)g_hw_smoke_key_seen_mask);
            g_hw_smoke_last_temp_log_ms = 0U;
            hw_smoke_next_stage(HW_SMOKE_STAGE_TEMP, now_ms);
        }
        else if ((degraded_keys_ok != 0U) && ((pressed_mask & 0x04U) != 0U))
        {
            hw_smoke_record_result(0);
            debug_log_info("APP", "hw_smoke KEYS degraded pass mask=0x%02X stuck=0x%02X",
                           (unsigned int)g_hw_smoke_key_seen_mask,
                           (unsigned int)g_hw_smoke_key_stuck_mask);
            g_hw_smoke_last_temp_log_ms = 0U;
            hw_smoke_next_stage(HW_SMOKE_STAGE_TEMP, now_ms);
        }
        break;

    case HW_SMOKE_STAGE_TEMP:
    {
        float t0 = 0.0f;
        float t1 = 0.0f;
        float t2 = 0.0f;
        int ok0;
        int ok1;
        int ok2;

        if (g_hw_smoke_stage_entered == 0U)
        {
            g_hw_smoke_stage_entered = 1U;
            debug_log_info("APP", "hw_smoke enter TEMP");
            if (g_hw_smoke_temp_port_inited == 0U)
            {
                hw_temp_port_init();
                g_hw_smoke_temp_port_inited = 1U;
                debug_log_info("APP", "hw_smoke TEMP bus init done");
            }
            ok0 = hw_temp_port_read_c(0U, &t0) ? 1 : 0;
            ok1 = hw_temp_port_read_c(1U, &t1) ? 1 : 0;
            ok2 = hw_temp_port_read_c(2U, &t2) ? 1 : 0;
            hw_smoke_record_result((ok0 || ok1 || ok2) ? 1 : 0);
        }

        ok0 = hw_temp_port_read_c(0U, &t0) ? 1 : 0;
        ok1 = hw_temp_port_read_c(1U, &t1) ? 1 : 0;
        ok2 = hw_temp_port_read_c(2U, &t2) ? 1 : 0;
        hw_oled_draw_text(0U, "HW SMOKE: TEMP");
        hw_oled_draw_text(1U, "LIVE CHANGE TEMP");
        (void)snprintf(line2,
                       sizeof(line2),
                       "%s%.1f %s%.1f",
                       ok0 ? "A" : "-", t0,
                       ok1 ? "B" : "-", t1);
        (void)snprintf(line3,
                       sizeof(line3),
                       "%s%.1f SET>NXT",
                       ok2 ? "C" : "-", t2);
        hw_oled_draw_text(2U, line2);
        hw_oled_draw_text(3U, line3);
        hw_smoke_refresh_blocking();

        if ((now_ms - g_hw_smoke_last_temp_log_ms) >= HW_SMOKE_TEMP_LOG_MS)
        {
            g_hw_smoke_last_temp_log_ms = now_ms;
            debug_log_info("APP", "hw_smoke TEMP live t0=%.2f t1=%.2f t2=%.2f",
                           t0,
                           t1,
                           t2);
        }

        if ((pressed_mask & 0x01U) != 0U)
        {
            debug_log_info("APP", "hw_smoke TEMP proceed by SET");
            hw_smoke_next_stage(HW_SMOKE_STAGE_ACTUATORS, now_ms);
        }
        break;
    }

    case HW_SMOKE_STAGE_ACTUATORS:
        if (g_hw_smoke_stage_entered == 0U)
        {
            g_hw_smoke_stage_entered = 1U;
            g_hw_smoke_buzzer_on = 0U;
            g_hw_smoke_relay_on = 0U;
            g_hw_smoke_buzzer_tested = 0U;
            g_hw_smoke_relay_tested = 0U;
            hw_buzzer_set(false);
            hw_relay_set(false);
            debug_log_info("APP", "hw_smoke enter ACTUATORS");
        }

        if ((pressed_mask & 0x01U) != 0U)
        {
            g_hw_smoke_buzzer_on = (uint8_t)(g_hw_smoke_buzzer_on == 0U ? 1U : 0U);
            g_hw_smoke_buzzer_tested = 1U;
            hw_buzzer_set(g_hw_smoke_buzzer_on != 0U);
            debug_log_info("APP", "hw_smoke ACT buzzer=%u", (unsigned int)g_hw_smoke_buzzer_on);
        }
        if (((g_hw_smoke_key_stuck_mask & 0x02U) == 0U) && ((pressed_mask & 0x02U) != 0U))
        {
            g_hw_smoke_relay_on = (uint8_t)(g_hw_smoke_relay_on == 0U ? 1U : 0U);
            g_hw_smoke_relay_tested = 1U;
            hw_relay_set(g_hw_smoke_relay_on != 0U);
            debug_log_info("APP", "hw_smoke ACT relay=%u led1=%u",
                           (unsigned int)g_hw_smoke_relay_on,
                           (unsigned int)g_hw_smoke_relay_on);
        }
        if (((g_hw_smoke_key_stuck_mask & 0x02U) != 0U) && ((pressed_mask & 0x04U) != 0U) &&
            ((g_hw_smoke_buzzer_tested == 0U) || (g_hw_smoke_relay_tested == 0U)))
        {
            g_hw_smoke_relay_on = (uint8_t)(g_hw_smoke_relay_on == 0U ? 1U : 0U);
            g_hw_smoke_relay_tested = 1U;
            hw_relay_set(g_hw_smoke_relay_on != 0U);
            debug_log_info("APP", "hw_smoke ACT relay=%u led1=%u degraded=1",
                           (unsigned int)g_hw_smoke_relay_on,
                           (unsigned int)g_hw_smoke_relay_on);
        }

        hw_oled_draw_text(0U, "HW SMOKE: ACT");
        hw_oled_draw_text(1U,
                          ((g_hw_smoke_key_stuck_mask & 0x02U) != 0U) ? "SET BUZ DN RLY" : "SET BUZ UP RLY");
        (void)snprintf(line2,
                       sizeof(line2),
                       "BZ%u RY%u LED1%u",
                       (unsigned int)g_hw_smoke_buzzer_on,
                       (unsigned int)g_hw_smoke_relay_on,
                       (unsigned int)g_hw_smoke_relay_on);
        hw_oled_draw_text(2U, line2);
        hw_oled_draw_text(3U,
                          (((g_hw_smoke_key_stuck_mask & 0x02U) != 0U) &&
                           ((g_hw_smoke_buzzer_tested == 0U) || (g_hw_smoke_relay_tested == 0U))) ?
                              "TEST BUZ+RLY" :
                              "DN NEXT");
        hw_smoke_refresh_blocking();

        if (((pressed_mask & 0x04U) != 0U) &&
            (((g_hw_smoke_key_stuck_mask & 0x02U) == 0U) ||
             ((g_hw_smoke_buzzer_tested != 0U) && (g_hw_smoke_relay_tested != 0U))))
        {
            hw_buzzer_set(false);
            hw_relay_set(false);
            g_hw_smoke_buzzer_on = 0U;
            g_hw_smoke_relay_on = 0U;
            hw_smoke_record_result(1);
            hw_smoke_prepare_spi_vectors(now_ms);
            hw_smoke_next_stage(HW_SMOKE_STAGE_SPIFLASH, now_ms);
        }
        break;

    case HW_SMOKE_STAGE_SPIFLASH:
        if (g_hw_smoke_stage_entered == 0U)
        {
            g_hw_smoke_stage_entered = 1U;
            debug_log_info("APP", "hw_smoke enter SPIFLASH addr=0x%04lX",
                           (unsigned long)g_hw_smoke_spi_addr);
        }

        if ((pressed_mask & 0x02U) != 0U)
        {
            if ((g_hw_smoke_key_stuck_mask & 0x02U) == 0U)
            {
                g_hw_smoke_spi_choice = (uint8_t)((g_hw_smoke_spi_choice + 1U) % HW_SMOKE_SPI_OPTION_COUNT);
                g_hw_smoke_spi_done = 0U;
                debug_log_info("APP", "hw_smoke SPI select=%u",
                               (unsigned int)(g_hw_smoke_spi_choice + 1U));
            }
        }
        if ((pressed_mask & 0x04U) != 0U)
        {
            if ((g_hw_smoke_key_stuck_mask & 0x02U) == 0U)
            {
                hw_smoke_prepare_spi_vectors(now_ms);
                debug_log_info("APP", "hw_smoke SPI reroll");
            }
            else
            {
                g_hw_smoke_spi_choice = (uint8_t)((g_hw_smoke_spi_choice + 1U) % HW_SMOKE_SPI_OPTION_COUNT);
                g_hw_smoke_spi_done = 0U;
                debug_log_info("APP", "hw_smoke SPI select=%u degraded=1",
                               (unsigned int)(g_hw_smoke_spi_choice + 1U));
            }
        }
        if ((pressed_mask & 0x01U) != 0U)
        {
            int wr_ok = hw_spiflash_write(g_hw_smoke_spi_addr,
                                          g_hw_smoke_spi_tx[g_hw_smoke_spi_choice],
                                          HW_SMOKE_SPI_DATA_LEN) ? 1 : 0;
            int rd_ok = hw_spiflash_read(g_hw_smoke_spi_addr,
                                         g_hw_smoke_spi_rx,
                                         HW_SMOKE_SPI_DATA_LEN) ? 1 : 0;
            int cmp_ok = (wr_ok && rd_ok &&
                          (memcmp(g_hw_smoke_spi_tx[g_hw_smoke_spi_choice],
                                  g_hw_smoke_spi_rx,
                                  HW_SMOKE_SPI_DATA_LEN) == 0)) ? 1 : 0;

            g_hw_smoke_spi_done = 1U;
            hw_smoke_record_result(cmp_ok);
            debug_log_info("APP",
                           "hw_smoke SPIFLASH sel=%u addr=0x%04lX wr=%d rd=%d cmp=%d rx=%02X %02X %02X %02X %02X %02X %02X %02X",
                           (unsigned int)(g_hw_smoke_spi_choice + 1U),
                           (unsigned long)g_hw_smoke_spi_addr,
                           wr_ok,
                           rd_ok,
                           cmp_ok,
                           (unsigned int)g_hw_smoke_spi_rx[0],
                           (unsigned int)g_hw_smoke_spi_rx[1],
                           (unsigned int)g_hw_smoke_spi_rx[2],
                           (unsigned int)g_hw_smoke_spi_rx[3],
                           (unsigned int)g_hw_smoke_spi_rx[4],
                           (unsigned int)g_hw_smoke_spi_rx[5],
                           (unsigned int)g_hw_smoke_spi_rx[6],
                           (unsigned int)g_hw_smoke_spi_rx[7]);
            if (cmp_ok != 0)
            {
                hw_smoke_next_stage(HW_SMOKE_STAGE_DONE, now_ms);
            }
        }

        {
            char preview[24];

            hw_smoke_format_preview4(preview, sizeof(preview), g_hw_smoke_spi_tx[g_hw_smoke_spi_choice]);
            (void)snprintf(line2,
                           sizeof(line2),
                           "S%u @%04lX",
                           (unsigned int)(g_hw_smoke_spi_choice + 1U),
                           (unsigned long)g_hw_smoke_spi_addr);
            if (g_hw_smoke_spi_done != 0U)
            {
                char rx_preview[24];

                hw_smoke_format_preview4(rx_preview, sizeof(rx_preview), g_hw_smoke_spi_rx);
                (void)snprintf(line3, sizeof(line3), "RX %s", rx_preview);
            }
            else
            {
                (void)snprintf(line3,
                               sizeof(line3),
                               "%s",
                               ((g_hw_smoke_key_stuck_mask & 0x02U) != 0U) ? "SET WR DN SEL" : "SET WR DN NEW");
            }

            hw_oled_draw_text(0U, "HW SMOKE: SPI");
            hw_oled_draw_text(1U, line2);
            hw_oled_draw_text(2U, preview);
            hw_oled_draw_text(3U, line3);
            hw_smoke_refresh_blocking();
        }
        break;

    case HW_SMOKE_STAGE_DONE:
        (void)snprintf(line2,
                       sizeof(line2),
                       "PASS %u FAIL %u",
                       (unsigned int)g_hw_smoke_pass_count,
                       (unsigned int)g_hw_smoke_fail_count);
        (void)snprintf(line3,
                       sizeof(line3),
                       "KEY MASK %u",
                       (unsigned int)g_hw_smoke_key_seen_mask);
        hw_smoke_show_lines("HW SMOKE: DONE",
                            "CHECK RESULT",
                            line2,
                            line3);
        if (g_hw_smoke_stage_entered == 0U)
        {
            g_hw_smoke_stage_entered = 1U;
            debug_log_info("APP", "hw_smoke DONE pass=%u fail=%u key_mask=0x%02X",
                           (unsigned int)g_hw_smoke_pass_count,
                           (unsigned int)g_hw_smoke_fail_count,
                           (unsigned int)g_hw_smoke_key_seen_mask);
        }
        break;

    default:
        break;
    }
}

static void apply_schedule_params(const app_params_t *params)
{
    schedule_config_t cfg;

    if (params == 0)
    {
        return;
    }

    cfg.enabled = (params->schedule_enabled != 0U);
    cfg.start_min_of_day = (uint16_t)(params->schedule_start_min % 1440U);
    cfg.end_min_of_day = (uint16_t)(params->schedule_end_min % 1440U);
    schedule_service_set_config(&cfg);
}

static void apply_pid_params(const app_params_t *params)
{
    if (params == 0)
    {
        return;
    }

    pid_init(&g_pid, params->kp, params->ki, params->kd, 1.0f, 0.0f, 100.0f);
}

static void sync_runtime_params_if_changed(void)
{
    const app_params_t *params = param_store_get();
    int schedule_changed;
    int pid_changed;

    if (memcmp(&g_runtime_params, params, sizeof(g_runtime_params)) == 0)
    {
        return;
    }

    schedule_changed = (g_runtime_params.schedule_enabled != params->schedule_enabled) ||
                       (g_runtime_params.schedule_start_min != params->schedule_start_min) ||
                       (g_runtime_params.schedule_end_min != params->schedule_end_min);
    pid_changed = (g_runtime_params.kp != params->kp) ||
                  (g_runtime_params.ki != params->ki) ||
                  (g_runtime_params.kd != params->kd);

    param_store_save(params);
    if (schedule_changed)
    {
        apply_schedule_params(params);
    }
    if (pid_changed)
    {
        apply_pid_params(params);
    }
    g_runtime_params = *params;
}

static const char *mode_to_str(app_mode_t mode)
{
    switch (mode)
    {
    case APP_MODE_IDLE:
        return "IDLE";
    case APP_MODE_HEATING:
        return "HEATING";
    case APP_MODE_SCHEDULED:
        return "SCHEDULED";
    case APP_MODE_ALARM:
        return "ALARM";
    case APP_MODE_EXPORT:
        return "EXPORT";
    default:
        return "UNKNOWN";
    }
}

static void run_display_driver_api_test(void)
{
    const uint16_t x0 = 4U;
    const uint16_t y0 = 4U;
    const uint16_t x1 = (uint16_t)(BSP_LCD_WIDTH - 5U);
    const uint16_t y1 = (uint16_t)(BSP_LCD_HEIGHT - 5U);

    hw_oled_init();
    hw_oled_clear();
    hw_oled_fill_rect(0U, 0U, BSP_LCD_WIDTH, BSP_LCD_HEIGHT, BSP_LCD_COLOR_BLACK);
    hw_oled_draw_rect(x0, y0, (uint16_t)(BSP_LCD_WIDTH - 8U), (uint16_t)(BSP_LCD_HEIGHT - 8U), BSP_LCD_COLOR_WHITE);
    hw_oled_draw_line(x0, y0, x1, y0, 0xF800U);
    hw_oled_draw_line(x1, y0, x1, y1, 0x07E0U);
    hw_oled_draw_line(x0, y1, x1, y1, 0x001FU);
    hw_oled_draw_line(x0, y0, x0, y1, 0xFFE0U);
    hw_oled_fill_round_rect(12U, 16U, 96U, 48U, 10U, 0x0596U);
    hw_oled_draw_line(0U, 0U, (uint16_t)(BSP_LCD_WIDTH - 1U), (uint16_t)(BSP_LCD_HEIGHT - 1U), BSP_LCD_COLOR_WHITE);
    hw_oled_draw_line((uint16_t)(BSP_LCD_WIDTH - 1U), 0U, 0U, (uint16_t)(BSP_LCD_HEIGHT - 1U), 0xFD20U);
    hw_oled_draw_circle(180U, 72U, 28U, 0x3666U);
    hw_oled_draw_text_xy(20U, 96U, "DISPLAY", 3U, BSP_LCD_COLOR_WHITE);
    hw_oled_draw_text_xy(20U, 128U, "DRIVER TEST", 3U, 0xFD20U);
    hw_oled_draw_text_xy(20U, 176U, "API BORDER LINE", 2U, 0xC638U);
    hw_oled_draw_text_xy(20U, 202U, "TEXT REFRESH", 2U, 0xC638U);
    hw_oled_draw_text(0U, "DRV API TEST");
    hw_oled_draw_text(1U, "BORDER 4 COLORS");
    hw_oled_draw_text(2U, "TEXT+REFRESH");
    hw_oled_draw_text(3U, "EXPECT PATTERN");
    hw_oled_refresh();
}

static void run_display_driver_solid_color_step(uint8_t index)
{
    static const uint16_t k_colors[8] = {
        0x0000U,
        0xFFFFU,
        0xF800U,
        0x07E0U,
        0x001FU,
        0xFFE0U,
        0x07FFU,
        0xF81FU
    };
    static const char *k_names[8] = {
        "BLACK",
        "WHITE",
        "RED",
        "GREEN",
        "BLUE",
        "YELLOW",
        "CYAN",
        "MAGENTA"
    };
    uint8_t idx = (uint8_t)(index % 8U);

    hw_oled_fill_rect(0U, 0U, BSP_LCD_WIDTH, BSP_LCD_HEIGHT, k_colors[idx]);
    hw_oled_draw_rect(2U, 2U, (uint16_t)(BSP_LCD_WIDTH - 4U), (uint16_t)(BSP_LCD_HEIGHT - 4U), BSP_LCD_COLOR_WHITE);

    hw_oled_draw_text(0U, "SOLID SCREEN");
    hw_oled_draw_text(1U, k_names[idx]);
    hw_oled_draw_text(2U, "COLOR FILL TEST");
    hw_oled_draw_text(3U, "CHECK FULL AREA");
    hw_oled_refresh();
}

static void run_display_driver_moving_block_step(void)
{
    uint16_t x;
    uint16_t y;
    uint16_t block_color;
    uint16_t dirty_x;
    uint16_t dirty_y;
    uint16_t dirty_w;
    uint16_t dirty_h;
    uint16_t prev_x0;
    uint16_t prev_y0;
    uint16_t prev_x1;
    uint16_t prev_y1;
    uint16_t curr_x0;
    uint16_t curr_y0;
    uint16_t curr_x1;
    uint16_t curr_y1;
    uint16_t union_x0;
    uint16_t union_y0;
    uint16_t union_x1;
    uint16_t union_y1;

    x = g_hw_test_block_x;
    y = g_hw_test_block_y;

    if (g_hw_test_block_dx > 0)
    {
        x = (uint16_t)(x + (uint16_t)g_hw_test_block_dx);
    }
    else
    {
        x = (uint16_t)(x - (uint16_t)(-g_hw_test_block_dx));
    }

    if (g_hw_test_block_dy > 0)
    {
        y = (uint16_t)(y + (uint16_t)g_hw_test_block_dy);
    }
    else
    {
        y = (uint16_t)(y - (uint16_t)(-g_hw_test_block_dy));
    }

    if (x >= HW_TEST_BLOCK_MAX_X)
    {
        x = HW_TEST_BLOCK_MAX_X;
        g_hw_test_block_dx = -g_hw_test_block_dx;
    }
    else if (x <= HW_TEST_BLOCK_MIN_X)
    {
        x = HW_TEST_BLOCK_MIN_X;
        g_hw_test_block_dx = -g_hw_test_block_dx;
    }

    if (y >= HW_TEST_BLOCK_MAX_Y)
    {
        y = HW_TEST_BLOCK_MAX_Y;
        g_hw_test_block_dy = -g_hw_test_block_dy;
    }
    else if (y <= HW_TEST_BLOCK_MIN_Y)
    {
        y = HW_TEST_BLOCK_MIN_Y;
        g_hw_test_block_dy = -g_hw_test_block_dy;
    }

    g_hw_test_block_x = x;
    g_hw_test_block_y = y;

    block_color = (g_hw_test_block_dx > 0) ? 0xFFE0U : 0x07FFU;

    if (g_hw_test_block_inited)
    {
        /* Dirty-rectangle union avoids missed pixels and reduces per-frame write area. */
        prev_x0 = g_hw_test_prev_block_x;
        prev_y0 = g_hw_test_prev_block_y;
        prev_x1 = (uint16_t)(g_hw_test_prev_block_x + HW_TEST_BLOCK_W - 1U);
        prev_y1 = (uint16_t)(g_hw_test_prev_block_y + HW_TEST_BLOCK_H - 1U);

        curr_x0 = g_hw_test_block_x;
        curr_y0 = g_hw_test_block_y;
        curr_x1 = (uint16_t)(g_hw_test_block_x + HW_TEST_BLOCK_W - 1U);
        curr_y1 = (uint16_t)(g_hw_test_block_y + HW_TEST_BLOCK_H - 1U);

        union_x0 = (prev_x0 < curr_x0) ? prev_x0 : curr_x0;
        union_y0 = (prev_y0 < curr_y0) ? prev_y0 : curr_y0;
        union_x1 = (prev_x1 > curr_x1) ? prev_x1 : curr_x1;
        union_y1 = (prev_y1 > curr_y1) ? prev_y1 : curr_y1;

        if (union_x0 > HW_TEST_DIRTY_MARGIN)
        {
            union_x0 = (uint16_t)(union_x0 - HW_TEST_DIRTY_MARGIN);
        }
        else
        {
            union_x0 = 0U;
        }

        if (union_y0 > HW_TEST_DIRTY_MARGIN)
        {
            union_y0 = (uint16_t)(union_y0 - HW_TEST_DIRTY_MARGIN);
        }
        else
        {
            union_y0 = 0U;
        }

        if (union_x1 < (uint16_t)(BSP_LCD_WIDTH - 1U - HW_TEST_DIRTY_MARGIN))
        {
            union_x1 = (uint16_t)(union_x1 + HW_TEST_DIRTY_MARGIN);
        }
        else
        {
            union_x1 = (uint16_t)(BSP_LCD_WIDTH - 1U);
        }

        if (union_y1 < (uint16_t)(BSP_LCD_HEIGHT - 1U - HW_TEST_DIRTY_MARGIN))
        {
            union_y1 = (uint16_t)(union_y1 + HW_TEST_DIRTY_MARGIN);
        }
        else
        {
            union_y1 = (uint16_t)(BSP_LCD_HEIGHT - 1U);
        }

        dirty_x = union_x0;
        dirty_y = union_y0;
        dirty_w = (uint16_t)(union_x1 - union_x0 + 1U);
        dirty_h = (uint16_t)(union_y1 - union_y0 + 1U);
        hw_oled_fill_rect(dirty_x, dirty_y, dirty_w, dirty_h, BSP_LCD_COLOR_BLACK);
    }

    hw_oled_fill_rect(g_hw_test_block_x, g_hw_test_block_y, HW_TEST_BLOCK_W, HW_TEST_BLOCK_H, block_color);
    hw_oled_draw_rect(g_hw_test_block_x, g_hw_test_block_y, HW_TEST_BLOCK_W, HW_TEST_BLOCK_H, BSP_LCD_COLOR_WHITE);

    g_hw_test_prev_block_x = g_hw_test_block_x;
    g_hw_test_prev_block_y = g_hw_test_block_y;
    g_hw_test_block_inited = true;
}

static void enter_display_driver_moving_block_scene(void)
{
    g_hw_test_block_x = 12U;
    g_hw_test_block_y = 56U;
    g_hw_test_prev_block_x = g_hw_test_block_x;
    g_hw_test_prev_block_y = g_hw_test_block_y;
    g_hw_test_block_dx = 4;
    g_hw_test_block_dy = 3;
    g_hw_test_block_inited = false;

    hw_oled_fill_rect(0U, 0U, BSP_LCD_WIDTH, BSP_LCD_HEIGHT, BSP_LCD_COLOR_BLACK);
    hw_oled_draw_rect(2U, 2U, (uint16_t)(BSP_LCD_WIDTH - 4U), (uint16_t)(BSP_LCD_HEIGHT - 4U), BSP_LCD_COLOR_WHITE);
    hw_oled_draw_text_xy(8U, 12U, "MOVING BLOCK 30FPS", 1U, 0xC638U);
    hw_oled_draw_text_xy(8U, 26U, "DIRTY RECT TEST", 1U, 0xC638U);
}

static void run_display_driver_hw_test_loop(void)
{
    uint32_t now_ms;

    now_ms = scheduler_now_ms();

    if (g_hw_test_phase == HW_DISPLAY_TEST_SOLID)
    {
        if ((now_ms - g_hw_test_last_step_ms) >= HW_TEST_SOLID_STEP_MS)
        {
            g_hw_test_last_step_ms = now_ms;
            run_display_driver_solid_color_step(g_hw_test_solid_idx);
            g_hw_test_solid_idx = (uint8_t)((g_hw_test_solid_idx + 1U) % 8U);
        }

        if ((now_ms - g_hw_test_phase_started_ms) >= HW_TEST_SOLID_TOTAL_MS)
        {
            g_hw_test_phase = HW_DISPLAY_TEST_MOVING_BLOCK;
            g_hw_test_phase_started_ms = now_ms;
            g_hw_test_last_step_ms = 0U;
            enter_display_driver_moving_block_scene();
            hw_oled_draw_text(0U, "MOVING BLOCK");
            hw_oled_draw_text(1U, "PATH + BOUNCE");
            hw_oled_draw_text(2U, "CHECK STABILITY");
            hw_oled_draw_text(3U, "NO TEARING");
            hw_oled_refresh();
        }
        return;
    }

    if (g_hw_test_phase == HW_DISPLAY_TEST_MOVING_BLOCK)
    {
        if ((now_ms - g_hw_test_last_step_ms) >= HW_TEST_BLOCK_STEP_MS)
        {
            g_hw_test_last_step_ms = now_ms;
            run_display_driver_moving_block_step();
        }

        if ((now_ms - g_hw_test_phase_started_ms) >= HW_TEST_BLOCK_TOTAL_MS)
        {
            g_hw_test_phase = HW_DISPLAY_TEST_API_PATTERN;
            g_hw_test_phase_started_ms = now_ms;
            run_display_driver_api_test();
        }
        return;
    }

    if ((now_ms - g_hw_test_phase_started_ms) >= HW_TEST_API_TOTAL_MS)
    {
        g_hw_test_phase = HW_DISPLAY_TEST_SOLID;
        g_hw_test_phase_started_ms = now_ms;
        g_hw_test_last_step_ms = 0U;
        g_hw_test_solid_idx = 0U;
        g_hw_test_block_inited = false;
    }
}

void app_main_init(void)
{
#if (APP_HW_DRIVER_TEST_DISPLAY == 1U)
    uint16_t rtc_minutes = 0U;
    bool rtc_ready = false;

    scheduler_init();
    debug_log_init();
    hw_oled_init();
    hw_oled_clear();
    hw_smoke_boot_stage("OLED OK", "KEY INIT");
    hw_key_init();
    hw_smoke_boot_stage("KEY OK", "BUZZER INIT");
    hw_buzzer_init();
    hw_buzzer_set(false);
    hw_smoke_boot_stage("BUZZER OK", "RELAY INIT");
    hw_relay_init();
    hw_relay_set(false);
    hw_smoke_boot_stage("RELAY OK", "UART INIT");
    hw_uart_init();
    hw_smoke_boot_stage("UART OK", "RTC INIT");
    hw_rtc_init();
    rtc_ready = hw_rtc_get_minutes_of_day(&rtc_minutes);
    (void)rtc_minutes;
    hw_smoke_boot_stage(rtc_ready ? "RTC OK" : "RTC FAIL", "EEPROM INIT");
    hw_eeprom_init();
    hw_smoke_boot_stage("EEPROM OK", "FLASH INIT");
    hw_spiflash_init();
    hw_smoke_boot_stage("FLASH OK", "SMOKE TEST");
    g_hw_smoke_temp_port_inited = 0U;
    debug_log_info("APP", "debug hw smoke test mode");
    g_hw_test_phase = HW_DISPLAY_TEST_SOLID;
    g_hw_test_phase_started_ms = scheduler_now_ms();
    g_hw_test_last_step_ms = 0U;
    g_hw_test_solid_idx = 0U;
    g_hw_smoke_stage = HW_SMOKE_STAGE_DISPLAY;
    g_hw_smoke_stage_started_ms = scheduler_now_ms();
    g_hw_smoke_stage_entered = 0U;
    g_hw_smoke_key_seen_mask = 0U;
    g_hw_smoke_pass_count = 0U;
    g_hw_smoke_fail_count = 0U;
    g_hw_smoke_last_key_raw_mask = 0U;
    g_hw_smoke_last_key_name = "NONE";
    g_hw_smoke_last_temp_log_ms = 0U;
    g_hw_smoke_spi_choice = 0U;
    g_hw_smoke_spi_addr = 0x1000UL;
    g_hw_smoke_spi_done = 0U;
    g_hw_smoke_buzzer_on = 0U;
    g_hw_smoke_relay_on = 0U;
    g_hw_smoke_temp_port_inited = 0U;
    g_hw_smoke_rand_state ^= scheduler_now_ms();
    g_hw_driver_test_active = true;
    return;
#endif

    scheduler_init();
    param_store_init();

    protocol_export_init();
    debug_log_init();
    debug_log_info("APP", "init start");

    temp_manager_init();
    alarm_service_init();
    hw_buzzer_init();
    hw_buzzer_set(false);
    schedule_service_init();
    log_service_init();
#if (APP_USE_LVGL_UI == 1U)
    ui_lvgl_port_init();
#else
    ui_service_init();
#endif

    {
        const app_params_t *params = param_store_get();
        apply_schedule_params(params);
        apply_pid_params(params);
        g_runtime_params = *params;
    }
    heater_ctrl_init(APP_PID_WINDOW_MS);

    g_mode = APP_MODE_IDLE;
    g_prev_mode = g_mode;
    g_prev_alarm = false;
    g_log_tick_count = 0U;
    g_last_1ms_tick = scheduler_now_ms();
#if (APP_USE_LVGL_UI == 1U)
    g_last_lvgl_task_ms = g_last_1ms_tick;
#endif

    debug_log_info("APP", "init done set=%.2f alarm=%.2f", param_store_get()->set_temp_c, param_store_get()->alarm_threshold_c);
}

void app_main_loop(void)
{
    scheduler_flags_t flags;
    const temp_snapshot_t *temp;
    uint32_t now_ms;

    if (g_hw_driver_test_active)
    {
        (void)hw_oled_process();
        run_all_hw_driver_smoke_test_loop();
        hw_eeprom_process();
        hw_spiflash_process();
        return;
    }

#if !defined(USE_STDPERIPH_DRIVER) && !defined(USE_HAL_DRIVER)
    scheduler_tick_1ms();
#endif

    now_ms = scheduler_now_ms();
    while (g_last_1ms_tick != now_ms)
    {
        g_last_1ms_tick++;
        heater_ctrl_update_1ms();
#if (APP_USE_LVGL_UI == 1U)
        ui_lvgl_port_tick_1ms();
#endif
    }

#if (APP_USE_LVGL_UI == 1U)
    if ((now_ms - g_last_lvgl_task_ms) >= APP_LVGL_TASK_PERIOD_MS)
    {
        g_last_lvgl_task_ms = now_ms;
        ui_lvgl_port_task();
    }
#endif

    scheduler_poll(&flags);

    if (flags.task_key_100ms)
    {
#if (APP_USE_LVGL_UI == 0U)
        ui_service_tick_100ms(param_store_get_mutable());
#endif
    }

    if (flags.task_control_1s)
    {
        bool alarm_now;

        param_store_tick_1s();

        temp_manager_update();
        temp = temp_manager_get_snapshot();
        {
            const app_params_t *params = param_store_get();

            alarm_service_update(temp->t1, temp->t2, temp->t3, params->alarm_threshold_c, temp->sensor_fault);
            schedule_service_update();
            alarm_now = alarm_service_is_active();
            hw_buzzer_set(alarm_now);

            if (alarm_now)
            {
                g_mode = APP_MODE_ALARM;
                g_pid_out = 0.0f;
                heater_ctrl_force_off();
            }
            else if (!schedule_service_heating_allowed())
            {
                g_mode = APP_MODE_SCHEDULED;
                g_pid_out = 0.0f;
                heater_ctrl_set_output_percent(0.0f);
            }
            else
            {
                g_mode = APP_MODE_HEATING;
                g_pid_out = pid_step(&g_pid, params->set_temp_c, temp->t_ctrl);
                heater_ctrl_set_output_percent(g_pid_out);
            }

            if (g_mode != g_prev_mode)
            {
                debug_log_info("APP", "mode %s -> %s", mode_to_str(g_prev_mode), mode_to_str(g_mode));
                g_prev_mode = g_mode;
            }

            if (alarm_now != g_prev_alarm)
            {
                if (alarm_now)
                {
                    debug_log_warn("APP", "alarm on t1=%.2f t2=%.2f t3=%.2f", temp->t1, temp->t2, temp->t3);
                }
                else
                {
                    debug_log_info("APP", "alarm cleared");
                }
                g_prev_alarm = alarm_now;
            }

            g_log_tick_count++;
            if (g_log_tick_count >= ((params->log_period_s == 0U) ? 1U : params->log_period_s))
            {
                g_log_tick_count = 0U;
                log_service_push(temp, params->set_temp_c, g_pid_out, heater_ctrl_get_state() ? 1 : 0, alarm_now ? 1 : 0);
            }
        }
    }

    if (flags.task_ui_200ms)
    {
        temp = temp_manager_get_snapshot();
#if (APP_USE_LVGL_UI == 1U)
        ui_lvgl_view_update_home(temp->t_ctrl,
                                 param_store_get()->set_temp_c,
                                 heater_ctrl_get_state() ? 1 : 0,
                                 alarm_service_is_active() ? 1 : 0);
#else
        ui_service_tick_200ms(g_mode,
                              temp,
                              param_store_get(),
                              g_pid_out,
                              heater_ctrl_get_state() ? 1 : 0,
                              alarm_service_is_active() ? 1 : 0);
#endif
    }

    protocol_export_process();
    (void)hw_oled_process();
    hw_eeprom_process();
    sync_runtime_params_if_changed();
}

