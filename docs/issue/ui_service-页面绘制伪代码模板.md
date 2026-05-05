# ui_service 页面绘制伪代码模板

- 目标文件：firmware/Core/Src/ui_service.c
- 目标：将 `lcd-ui-preview.html` 的页面视觉稿映射到当前 `render_*` 绘制函数。
- 说明：以下为伪代码模板，优先保证结构与调用顺序正确，再逐页替换文案和坐标。

---

## 1. 推荐常量与组件函数

```c
/* 统一布局常量（240x320） */
#define UI_W 240U
#define UI_H 320U

#define UI_HEADER_X 24U
#define UI_HEADER_Y 24U
#define UI_SUB_Y    56U
#define UI_RULE_Y   82U

#define UI_CARD_X   20U
#define UI_CARD_Y   106U
#define UI_CARD_W   200U

#define UI_KEYBAR_X 16U
#define UI_KEYBAR_Y 284U
#define UI_KEYBAR_W 208U

static void draw_header(const char *title, const char *subtitle)
{
    hw_oled_fill_rect(0U, 0U, UI_W, UI_H, UI_LCD_BG);
    hw_oled_draw_text_xy(UI_HEADER_X, UI_HEADER_Y, title, 3U, UI_LCD_TEXT);
    hw_oled_draw_text_xy(26U, UI_SUB_Y, subtitle, 1U, UI_LCD_PANEL_ALT);
    hw_oled_draw_line(24U, UI_RULE_Y, 216U, UI_RULE_Y, UI_LCD_ACCENT);
}

static void draw_hint_symbol(unsigned int x, unsigned int y, const char *symbol)
{
    /* 角落符号提示：每页不超过2个，不使用按键文字 */
    hw_oled_draw_text_xy(x, y, symbol, 1U, UI_LCD_PANEL_ALT);
}

static void draw_hint_pair(const char *symbol_pair)
{
    draw_hint_symbol(188U, 304U, symbol_pair);
}

static void draw_home_sensor_strip(float t_top, float t_mid, float t_bot)
{
    /* 三个小卡片：上层/中层/下层 */
    draw_sensor_chip(20U, 196U, 61U, "上层", t_top);
    draw_sensor_chip(89U, 196U, 61U, "中层", t_mid);
    draw_sensor_chip(158U, 196U, 61U, "下层", t_bot);
}

static void draw_kv_box(const kv_item_t *items, unsigned int count)
{
    /* x=20 y=102 w=200，高度按 count 自适配 */
}
```

---

## 2. 页面函数模板

## 2.1 启动页

```c
static void render_startup_splash(void)
{
    hw_oled_fill_rect(0U, 0U, UI_W, UI_H, UI_LCD_BG);
    render_centered_text(88U,  "水温", 5U, UI_LCD_TEXT);
    render_centered_text(142U, "控制", 5U, UI_LCD_WARM);
    hw_oled_draw_line(46U, 202U, 194U, 202U, UI_LCD_ACCENT);
    render_centered_text(232U, "固定容积三点测温", 1U, UI_LCD_PANEL_ALT);
}
```

## 2.2 首页

```c
static void render_home_dashboard(void)
{
    char fuse_text[16];
    char set_text[16];
    char margin_text[24];

    float t_safe = max3f(g_ui.last_temp.t1, g_ui.last_temp.t2, g_ui.last_temp.t3);
    float margin = g_ui.last_params.alarm_threshold_c - t_safe;

    draw_header("首页", g_ui.last_heater_on ? "加热中" : "待机");

    snprintf(fuse_text, sizeof(fuse_text), "%.1f度", g_ui.last_temp.t_ctrl);
    hw_oled_draw_text_xy(28U, 30U, "融合温度", 2U, UI_LCD_TEXT);
    hw_oled_draw_text_xy(28U, 68U, fuse_text, 6U, UI_LCD_WARM);

    hw_oled_draw_line(24U, 186U, 216U, 186U, UI_LCD_PANEL_ALT);

    snprintf(set_text, sizeof(set_text), "%.1f度", g_ui.last_params.set_temp_c);
    draw_row(28U, 196U, "目标", set_text, UI_LCD_GOOD);

    snprintf(margin_text, sizeof(margin_text), "距离报警阈值 %.1f度", margin);
    hw_oled_draw_text_xy(28U, 256U, "加热开启 / 稳定运行", 1U, UI_LCD_TEXT);
    hw_oled_draw_text_xy(28U, 276U, margin_text, 1U, UI_LCD_PANEL_ALT);

    draw_hint_pair("◀ ▶");
}
```

## 2.3 设定温度页

```c
static void render_set_temp_page(void)
{
    char set_text[16];
    char status1[32];

    draw_header("设定温度", "步进零点五度");

    snprintf(set_text, sizeof(set_text), "%.1f度", g_ui.last_params.set_temp_c);
    draw_value_card(UI_CARD_X, UI_CARD_Y, UI_CARD_W, 110U, "目标值", set_text, 5U, UI_LCD_GOOD);

    snprintf(status1, sizeof(status1), "融合%.1f度 / 安全%.1f度", g_ui.last_temp.t_ctrl,
             max3f(g_ui.last_temp.t1, g_ui.last_temp.t2, g_ui.last_temp.t3));
    render_status_line(242U, status1, UI_LCD_TEXT);
    render_status_line(264U, "设定保存 / 长按取消", UI_LCD_PANEL_ALT);

    draw_hint_pair("OK ⤒");
}
```

## 2.4 参数档位页

```c
static void render_pid_preset_page(void)
{
    draw_header("参数档位", "当前第二档 共三档");
    draw_value_card(UI_CARD_X, UI_CARD_Y, UI_CARD_W, 96U, "当前档位", "平衡档", 3U, UI_LCD_ACCENT);

    render_status_line(234U, "快热 / 平衡 / 保温", UI_LCD_TEXT);
    render_status_line(258U, "设定切换 / 长按退出", UI_LCD_PANEL_ALT);

    draw_hint_pair("OK ◀");
}
```

## 2.5 运行详情页

```c
static void render_runtime_page(void)
{
    kv_item_t items[3];

    draw_header("运行详情", "控制细节");

    items[0] = KV("上中下", "59.0/58.4/57.2", UI_LCD_TEXT);
    items[1] = KV("控制输出", "42%", UI_LCD_TEXT);
    items[2] = KV("安全最高", "59.0度", UI_LCD_TEXT);
    draw_kv_box(items, 3U);

    render_status_line(246U, "目标60.0度 / 融合58.2度", UI_LCD_TEXT);
    render_status_line(268U, "三点温度正常", UI_LCD_PANEL_ALT);

    draw_hint_pair("◀ ▶");
}
```

## 2.6 超温报警页

```c
static void render_alarm_trip_page(void)
{
    draw_header_alert("超温报警", "温度超过阈值");
    draw_value_card_alert(UI_CARD_X, UI_CARD_Y, UI_CARD_W, 96U, "安全最高", "61.4度");

    render_status_line(240U, "已强制停止加热", UI_LCD_ALERT);
    render_status_line(264U, "恢复阈值58.0度 持续60秒", UI_LCD_PANEL_ALT);

    draw_hint_symbol(202U, 304U, "OK");
}
```

## 2.7 探头异常页

```c
static void render_probe_fault_page(void)
{
    kv_item_t items[3];

    draw_header("探头故障", "降级运行模式");

    items[0] = KV("故障项", "下层探头离线", UI_LCD_ALERT);
    items[1] = KV("运行模式", "降级运行", UI_LCD_WARM);
    items[2] = KV("输出上限", "55%", UI_LCD_TEXT);
    draw_kv_box(items, 3U);

    render_status_line(246U, "加热已受限 / 请检查连线", UI_LCD_TEXT);
    render_status_line(268U, "每5秒自动重试", UI_LCD_PANEL_ALT);

    draw_hint_symbol(202U, 304U, "OK");
}
```

## 2.8 自检结果页

```c
static void render_post_result_page(void)
{
    kv_item_t items[3];

    draw_header("上电自检", "启动检查结果");

    items[0] = KV("参数区", "校验通过", UI_LCD_GOOD);
    items[1] = KV("温度探头", "2路可用", UI_LCD_TEXT);
    items[2] = KV("运行结论", "降级运行", UI_LCD_WARM);
    draw_kv_box(items, 3U);

    render_status_line(246U, "限制 预约功能已禁用", UI_LCD_TEXT);
    render_status_line(268U, "检查结束后自动进入首页", UI_LCD_PANEL_ALT);

    draw_hint_symbol(202U, 304U, "OK");
}
```

## 2.9 导出页

```c
static void render_export_page_v2(void)
{
    char log_cnt[16];

    draw_header("日志导出", "串口输出");

    snprintf(log_cnt, sizeof(log_cnt), "%u", log_service_count());
    draw_value_card(UI_CARD_X, UI_CARD_Y, UI_CARD_W, 88U, "日志条数", log_cnt, 5U, UI_LCD_GOOD);

    render_status_line(246U, "记录周期5秒", UI_LCD_TEXT);
    render_status_line(268U, "串口命令 日志导出", UI_LCD_PANEL_ALT);

    draw_hint_pair("OK ⤓");
}
```

---

## 3. 页面路由模板

```c
static void render_page(void)
{
    if (g_ui.splash_ticks > 0U)
    {
        render_startup_splash();
        return;
    }

    switch (g_ui.page)
    {
    case UI_PAGE_HOME:
        render_home_dashboard();
        break;
    case UI_PAGE_SET_TEMP:
        render_set_temp_page();
        break;
    case UI_PAGE_PID:
        render_pid_preset_page();
        break;
    case UI_PAGE_ALARM:
        if (g_ui.last_alarm_on)
        {
            render_alarm_trip_page();
        }
        else
        {
            render_runtime_page();
        }
        break;
    case UI_PAGE_SCHEDULE:
        render_post_result_page();
        break;
    case UI_PAGE_EXPORT:
        render_export_page_v2();
        break;
    case UI_PAGE_INFO:
        render_probe_fault_page();
        break;
    default:
        render_home_dashboard();
        break;
    }
}
```

```c
/* 导航行为模板（浏览态） */
static void process_nav_key(ui_key_event_t key)
{
    if (key == UI_KEY_UP || key == UI_KEY_UP_REPEAT)
    {
        page_prev();
    }
    else if (key == UI_KEY_DOWN || key == UI_KEY_DOWN_REPEAT)
    {
        page_next();
    }
}
```

---

## 4. 改造实施顺序（建议）

1. 先抽取通用组件函数（header/card/keybar/kv_box）。
2. 替换首页与设定页（风险低，收益高）。
3. 替换报警页与探头异常页（安全相关优先）。
4. 最后替换运行详情、自检结果、导出页。
5. 每步替换后都执行 `ui_service` 相关测试并实机确认显示布局。

---

## 5. 无按键阶段接入模板（mock 输入）

本阶段先不处理按键事件，只做显示状态机和页面绘制验证。

```c
typedef struct
{
    uint8_t splash_timeout;
    uint8_t post_ok;
    uint8_t post_degraded;
    uint8_t alarm_on;
    uint8_t sensor_fault;
    uint8_t view_tick;

    float t_top;
    float t_mid;
    float t_bot;
    float t_fused;
    float set_temp;
    float alarm_threshold;
    uint16_t log_count;
} ui_mock_input_t;

static ui_mock_input_t g_mock;
```

```c
static void ui_mock_step_pages(void)
{
    if (g_mock.view_tick == 0U)
    {
        return;
    }

    g_mock.view_tick = 0U;
    switch (g_ui.page)
    {
    case UI_PAGE_HOME:
        g_ui.page = UI_PAGE_SET_TEMP;
        break;
    case UI_PAGE_SET_TEMP:
        g_ui.page = UI_PAGE_PID;
        break;
    case UI_PAGE_PID:
        g_ui.page = UI_PAGE_ALARM;  /* 非告警时显示运行详情页 */
        break;
    case UI_PAGE_ALARM:
        g_ui.page = UI_PAGE_EXPORT;
        break;
    case UI_PAGE_EXPORT:
    default:
        g_ui.page = UI_PAGE_HOME;
        break;
    }
}

static void ui_apply_mock_input(void)
{
    if (g_mock.splash_timeout != 0U)
    {
        g_ui.splash_ticks = 0U;
        g_ui.page = UI_PAGE_SCHEDULE; /* 自检结果页 */
        g_mock.splash_timeout = 0U;
    }

    if (g_ui.page == UI_PAGE_SCHEDULE)
    {
        if (g_mock.post_degraded != 0U)
        {
            g_ui.page = UI_PAGE_INFO; /* 探头异常页 */
            g_mock.post_degraded = 0U;
        }
        else if (g_mock.post_ok != 0U)
        {
            g_ui.page = UI_PAGE_HOME;
            g_mock.post_ok = 0U;
        }
    }

    if (g_mock.alarm_on != 0U)
    {
        g_ui.last_alarm_on = 1;
        g_ui.page = UI_PAGE_ALARM; /* 告警态显示超温报警页 */
        return;
    }

    g_ui.last_alarm_on = 0;
    if (g_mock.sensor_fault != 0U)
    {
        g_ui.last_temp.sensor_fault = true;
        g_ui.page = UI_PAGE_INFO;
        return;
    }

    g_ui.last_temp.sensor_fault = false;
    ui_mock_step_pages();
}
```

```c
void ui_service_tick_200ms(app_params_t *params, const temp_snapshot_t *temp, float pid_out, int heater_on)
{
    /* 1) 先同步业务量测值 */
    /* 2) 再应用 mock 状态机输入 */
    /* 3) 最后按当前页 render */
    (void)params;
    (void)temp;
    (void)pid_out;
    (void)heater_on;

    ui_apply_mock_input();
    render_page();
}
```
