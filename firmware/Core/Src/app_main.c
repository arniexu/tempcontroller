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
    HW_SMOKE_STAGE_BUZZER,
    HW_SMOKE_STAGE_RELAY,
    HW_SMOKE_STAGE_UART,
    HW_SMOKE_STAGE_RTC,
    HW_SMOKE_STAGE_TEMP,
    HW_SMOKE_STAGE_EEPROM,
    HW_SMOKE_STAGE_SPIFLASH,
    HW_SMOKE_STAGE_DONE
} hw_smoke_stage_t;

static hw_smoke_stage_t g_hw_smoke_stage = HW_SMOKE_STAGE_DISPLAY;
static uint32_t g_hw_smoke_stage_started_ms = 0U;
static uint8_t g_hw_smoke_stage_entered = 0U;
static uint8_t g_hw_smoke_key_seen_mask = 0U;
static uint8_t g_hw_smoke_pass_count = 0U;
static uint8_t g_hw_smoke_fail_count = 0U;

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
#define HW_SMOKE_DISPLAY_MS          (8000U)
#define HW_SMOKE_KEY_TIMEOUT_MS      (30000U)
#define HW_SMOKE_OBSERVE_MS          (1200U)
#define HW_SMOKE_STAGE_MSG_MS        (1500U)
#define HW_SMOKE_KEY_REQUIRED_MASK   (0x05U)

static void run_display_driver_hw_test_loop(void);

static void hw_smoke_show_lines(const char *l0,
                                const char *l1,
                                const char *l2,
                                const char *l3)
{
    hw_oled_draw_text(0U, l0);
    hw_oled_draw_text(1U, l1);
    hw_oled_draw_text(2U, l2);
    hw_oled_draw_text(3U, l3);
    hw_oled_refresh();
}

static void hw_smoke_next_stage(hw_smoke_stage_t stage, uint32_t now_ms)
{
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
    char line2[24];
    char line3[24];

    switch (g_hw_smoke_stage)
    {
    case HW_SMOKE_STAGE_DISPLAY:
        if (g_hw_smoke_stage_entered == 0U)
        {
            g_hw_smoke_stage_entered = 1U;
            g_hw_test_phase = HW_DISPLAY_TEST_SOLID;
            g_hw_test_phase_started_ms = now_ms;
            g_hw_test_last_step_ms = 0U;
            g_hw_test_solid_idx = 0U;
            hw_smoke_show_lines("HW SMOKE: DISPLAY",
                                "RUN PATTERN 8S",
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
        hw_oled_refresh();
        if ((now_ms - g_hw_smoke_stage_started_ms) >= HW_SMOKE_DISPLAY_MS)
        {
            hw_smoke_record_result(1);
            hw_oled_clear();
            hw_smoke_next_stage(HW_SMOKE_STAGE_KEYS, now_ms);
        }
        break;

    case HW_SMOKE_STAGE_KEYS:
        if (g_hw_smoke_stage_entered == 0U)
        {
            g_hw_smoke_stage_entered = 1U;
            g_hw_smoke_key_seen_mask = 0U;
            hw_smoke_show_lines("HW SMOKE: KEYS",
                                "PRESS SET/UP/DN",
                                "MASK 000",
                                "REQ SET+DN");
        }

        if (hw_key_get_state(HW_KEY_SET))
        {
            g_hw_smoke_key_seen_mask |= 0x01U;
        }
        if (hw_key_get_state(HW_KEY_UP))
        {
            g_hw_smoke_key_seen_mask |= 0x02U;
        }
        if (hw_key_get_state(HW_KEY_DOWN))
        {
            g_hw_smoke_key_seen_mask |= 0x04U;
        }

        (void)snprintf(line2,
                       sizeof(line2),
                       "MASK %u%u%u",
                       (g_hw_smoke_key_seen_mask & 0x01U) ? 1U : 0U,
                       (g_hw_smoke_key_seen_mask & 0x02U) ? 1U : 0U,
                       (g_hw_smoke_key_seen_mask & 0x04U) ? 1U : 0U);
        (void)snprintf(line3,
                   sizeof(line3),
                   "RAW S%u U%u D%u",
                   hw_key_get_state(HW_KEY_SET) ? 1U : 0U,
                   hw_key_get_state(HW_KEY_UP) ? 1U : 0U,
                   hw_key_get_state(HW_KEY_DOWN) ? 1U : 0U);
        hw_oled_draw_text(2U, line2);
        hw_oled_draw_text(3U, line3);
        hw_oled_refresh();

        if ((g_hw_smoke_key_seen_mask & HW_SMOKE_KEY_REQUIRED_MASK) == HW_SMOKE_KEY_REQUIRED_MASK)
        {
            hw_smoke_record_result(1);
            hw_smoke_next_stage(HW_SMOKE_STAGE_BUZZER, now_ms);
        }
        else if ((now_ms - g_hw_smoke_stage_started_ms) >= HW_SMOKE_KEY_TIMEOUT_MS)
        {
            hw_smoke_record_result(0);
            hw_smoke_next_stage(HW_SMOKE_STAGE_BUZZER, now_ms);
        }
        break;

    case HW_SMOKE_STAGE_BUZZER:
        if (g_hw_smoke_stage_entered == 0U)
        {
            g_hw_smoke_stage_entered = 1U;
            hw_buzzer_set(true);
            hw_smoke_show_lines("HW SMOKE: BUZZER",
                                "BUZZER ON 1.2S",
                                "MANUAL CHECK",
                                "LISTEN SOUND");
        }

        if ((now_ms - g_hw_smoke_stage_started_ms) >= HW_SMOKE_OBSERVE_MS)
        {
            hw_buzzer_set(false);
            hw_smoke_record_result(1);
            hw_smoke_next_stage(HW_SMOKE_STAGE_RELAY, now_ms);
        }
        break;

    case HW_SMOKE_STAGE_RELAY:
        if (g_hw_smoke_stage_entered == 0U)
        {
            g_hw_smoke_stage_entered = 1U;
            hw_relay_set(true);
            hw_smoke_show_lines("HW SMOKE: RELAY",
                                "RELAY ON 1.2S",
                                "MANUAL CHECK",
                                "LISTEN CLICK");
        }

        if ((now_ms - g_hw_smoke_stage_started_ms) >= HW_SMOKE_OBSERVE_MS)
        {
            hw_relay_set(false);
            hw_smoke_record_result(1);
            hw_smoke_next_stage(HW_SMOKE_STAGE_UART, now_ms);
        }
        break;

    case HW_SMOKE_STAGE_UART:
        if (g_hw_smoke_stage_entered == 0U)
        {
            g_hw_smoke_stage_entered = 1U;
            hw_uart_write("[HW_SMOKE] UART TX OK\r\n");
            hw_uart_write("[HW_SMOKE] WAIT HOST ECHO OPTIONAL\r\n");
            hw_smoke_record_result(1);
            hw_smoke_show_lines("HW SMOKE: UART",
                                "SEND TEXT FRAME",
                                "CHECK SERIAL LOG",
                                "TX OK;RX OPT");
        }

        if ((now_ms - g_hw_smoke_stage_started_ms) >= HW_SMOKE_STAGE_MSG_MS)
        {
            hw_smoke_next_stage(HW_SMOKE_STAGE_RTC, now_ms);
        }
        break;

    case HW_SMOKE_STAGE_RTC:
        if (g_hw_smoke_stage_entered == 0U)
        {
            uint16_t minutes = 0U;
            int ok;

            g_hw_smoke_stage_entered = 1U;
            ok = hw_rtc_get_minutes_of_day(&minutes) ? 1 : 0;
            hw_smoke_record_result(ok);
            (void)snprintf(line2, sizeof(line2), "RTC %s", ok ? "PASS" : "FAIL");
            (void)snprintf(line3,
                           sizeof(line3),
                           "MIN %u HH:%02u",
                           (unsigned int)(minutes % 1440U),
                           (unsigned int)((minutes / 60U) % 24U));
            hw_smoke_show_lines("HW SMOKE: RTC",
                                "READ MIN OF DAY",
                                line2,
                                line3);
        }

        if ((now_ms - g_hw_smoke_stage_started_ms) >= HW_SMOKE_STAGE_MSG_MS)
        {
            hw_smoke_next_stage(HW_SMOKE_STAGE_TEMP, now_ms);
        }
        break;

    case HW_SMOKE_STAGE_TEMP:
        if (g_hw_smoke_stage_entered == 0U)
        {
            float t0 = 0.0f;
            float t1 = 0.0f;
            float t2 = 0.0f;
            uint8_t ok_count = 0U;
            int ok0;
            int ok1;
            int ok2;

            g_hw_smoke_stage_entered = 1U;
            ok0 = hw_temp_port_read_c(0U, &t0) ? 1 : 0;
            ok1 = hw_temp_port_read_c(1U, &t1) ? 1 : 0;
            ok2 = hw_temp_port_read_c(2U, &t2) ? 1 : 0;
            if (ok0)
            {
                ok_count++;
            }
            if (ok1)
            {
                ok_count++;
            }
            if (ok2)
            {
                ok_count++;
            }

            hw_smoke_record_result(ok_count > 0U ? 1 : 0);
            (void)snprintf(line2,
                           sizeof(line2),
                           "DS18B20 OK %u/3",
                           (unsigned int)ok_count);
            (void)snprintf(line3,
                           sizeof(line3),
                           "%s%.1f %s%.1f %s%.1f",
                           ok0 ? "A" : "-", t0,
                           ok1 ? "B" : "-", t1,
                           ok2 ? "C" : "-", t2);
            hw_smoke_show_lines("HW SMOKE: TEMP",
                                "READ 3 SENSORS",
                                line2,
                                line3);
        }

        if ((now_ms - g_hw_smoke_stage_started_ms) >= HW_SMOKE_STAGE_MSG_MS)
        {
            hw_smoke_next_stage(HW_SMOKE_STAGE_EEPROM, now_ms);
        }
        break;

    case HW_SMOKE_STAGE_EEPROM:
        if (g_hw_smoke_stage_entered == 0U)
        {
            static const uint8_t tx[8] = {0x5AU, 0xA5U, 0x11U, 0x22U, 0x33U, 0x44U, 0x55U, 0x66U};
            uint8_t rx[8];
            int ok = 0;
            int wr_ok;
            int rd_ok;

            g_hw_smoke_stage_entered = 1U;
            wr_ok = hw_eeprom_write(0x20U, tx, 8U) ? 1 : 0;
            rd_ok = hw_eeprom_read(0x20U, rx, 8U) ? 1 : 0;
            if (wr_ok && rd_ok &&
                (memcmp(tx, rx, 8U) == 0))
            {
                ok = 1;
            }

            hw_smoke_record_result(ok);
            (void)snprintf(line2,
                           sizeof(line2),
                           "WR%d RD%d CMP%d",
                           wr_ok,
                           rd_ok,
                           ok);
            (void)snprintf(line3,
                           sizeof(line3),
                           "R0=%02X R1=%02X",
                           (unsigned int)rx[0],
                           (unsigned int)rx[1]);
            hw_smoke_show_lines("HW SMOKE: EEPROM",
                                "WRITE/READ 8B",
                                line2,
                                line3);
        }

        if ((now_ms - g_hw_smoke_stage_started_ms) >= HW_SMOKE_STAGE_MSG_MS)
        {
            hw_smoke_next_stage(HW_SMOKE_STAGE_SPIFLASH, now_ms);
        }
        break;

    case HW_SMOKE_STAGE_SPIFLASH:
        if (g_hw_smoke_stage_entered == 0U)
        {
            static const uint8_t tx[8] = {0xC3U, 0x3CU, 0x10U, 0x20U, 0x30U, 0x40U, 0x50U, 0x60U};
            uint8_t rx[8];
            int ok = 0;
            int wr_ok;
            int rd_ok;

            g_hw_smoke_stage_entered = 1U;
            wr_ok = hw_spiflash_write(0x1000UL, tx, 8UL) ? 1 : 0;
            rd_ok = hw_spiflash_read(0x1000UL, rx, 8UL) ? 1 : 0;
            if (wr_ok && rd_ok &&
                (memcmp(tx, rx, 8U) == 0))
            {
                ok = 1;
            }

            hw_smoke_record_result(ok);
            (void)snprintf(line2,
                           sizeof(line2),
                           "WR%d RD%d CMP%d",
                           wr_ok,
                           rd_ok,
                           ok);
            (void)snprintf(line3,
                           sizeof(line3),
                           "R0=%02X R1=%02X",
                           (unsigned int)rx[0],
                           (unsigned int)rx[1]);
            hw_smoke_show_lines("HW SMOKE: FLASH",
                                "WRITE/READ 8B",
                                line2,
                                line3);
        }

        if ((now_ms - g_hw_smoke_stage_started_ms) >= HW_SMOKE_STAGE_MSG_MS)
        {
            hw_smoke_next_stage(HW_SMOKE_STAGE_DONE, now_ms);
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
    scheduler_init();
    debug_log_init();
    hw_oled_init();
    hw_oled_clear();
    hw_key_init();
    hw_buzzer_init();
    hw_buzzer_set(false);
    hw_relay_init();
    hw_relay_set(false);
    hw_uart_init();
    hw_rtc_init();
    hw_eeprom_init();
    hw_spiflash_init();
    hw_temp_port_init();
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
    ui_service_init();

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

#if !defined(USE_STDPERIPH_DRIVER)
    scheduler_tick_1ms();
#endif

    now_ms = scheduler_now_ms();
    while (g_last_1ms_tick != now_ms)
    {
        g_last_1ms_tick++;
        heater_ctrl_update_1ms();
    }

    scheduler_poll(&flags);

    if (flags.task_key_100ms)
    {
        ui_service_tick_100ms(param_store_get_mutable());
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
        ui_service_tick_200ms(g_mode,
                              temp,
                              param_store_get(),
                              g_pid_out,
                              heater_ctrl_get_state() ? 1 : 0,
                              alarm_service_is_active() ? 1 : 0);
    }

    protocol_export_process();
    (void)hw_oled_process();
    hw_eeprom_process();
    sync_runtime_params_if_changed();
}
