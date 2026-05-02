# LVGL 最小接入方案（面向当前 STM32F103 工程）

- 目标仓库：`firmware/`
- 目标 MCU：STM32F103
- 当前显示：240x320 并口 TFTLCD
- 当前 UI 实现：`firmware/Core/Src/ui_service.c`
- 当前显示 BSP：`firmware/Drivers/BSP/bsp_oled.c`

---

## 1. 结论

当前工程适合接入 LVGL，原因有两个：

1. 现有 LCD 驱动已经具备区域写入的底层能力。`bsp_oled.c` 内部已有：
   - `lcd_set_window()`
   - `lcd_prepare_write_ram()`
   - `lcd_write_data()`

2. 现有主循环已经具备稳定节拍：
   - 1 ms：`scheduler_tick_1ms()` / `heater_ctrl_update_1ms()`
   - 100 ms：`ui_service_tick_100ms()`
   - 200 ms：`ui_service_tick_200ms()`

因此，LVGL 的两个关键接点都已经存在：

- `lv_tick_inc()`：可挂到 1 ms 节拍
- `flush_cb()`：可接到 LCD 区域刷写路径

---

## 2. 接入目标

第一阶段只做“LVGL 跑起来并能显示一个页面”，不替换全部业务逻辑。

第一阶段范围：

1. 在工程中加入 LVGL 核心源码
2. 建立 `display flush` 适配层
3. 建立 `tick` 与 `handler` 调度接入
4. 在 LVGL 中先画一个最小页面（如首页 demo）
5. 暂时不接管全部按键状态机

不在第一阶段做的事：

1. 不立即删除 `ui_service.c`
2. 不立即把所有页面迁到 LVGL
3. 不立即引入复杂动画和主题
4. 不立即做全量中文字体

---

## 3. 推荐目录结构

建议新增以下目录：

```text
firmware/
  ThirdParty/
    lvgl/
      lvgl.h
      src/
      examples/              （可不加入工程）
  Core/
    Inc/
      ui_lvgl_port.h
      ui_lvgl_view.h
      lv_conf.h
    Src/
      ui_lvgl_port.c
      ui_lvgl_view.c
```

说明：

- `ThirdParty/lvgl/`：放官方 LVGL 源码
- `lv_conf.h`：项目专用裁剪配置
- `ui_lvgl_port.c`：LVGL 与 LCD/时基/输入的适配层
- `ui_lvgl_view.c`：LVGL 页面搭建代码

---

## 4. Keil 工程最小增量文件

需要加入 Keil 的源文件至少包括：

1. `ThirdParty/lvgl/src/core/*.c`
2. `ThirdParty/lvgl/src/draw/*.c`
3. `ThirdParty/lvgl/src/hal/*.c`
4. `ThirdParty/lvgl/src/misc/*.c`
5. `ThirdParty/lvgl/src/widgets/label/*.c`
6. `ThirdParty/lvgl/src/widgets/obj/*.c`
7. `ThirdParty/lvgl/src/widgets/button/*.c`（若需要按钮）
8. `Core/Src/ui_lvgl_port.c`
9. `Core/Src/ui_lvgl_view.c`

第一阶段建议只打开最少控件：

- `obj`
- `label`
- `button`（可选）
- `bar`（可选）

不要一开始就把全部 widgets、themes、examples 全部编进来。

---

## 5. `lv_conf.h` 推荐裁剪方向

STM32F103 资源有限，建议第一阶段配置思路如下：

1. 颜色格式：RGB565
2. 关闭复杂动画
3. 关闭不需要的主题
4. 关闭文件系统、图片解码、复杂字体引擎
5. 字体只保留一个英文小字库和一个中号数字字库
6. 日志等级先设为 warning 或关闭

建议重点项：

```c
#define LV_COLOR_DEPTH 16
#define LV_USE_LOG 0
#define LV_USE_ANIMATION 0
#define LV_USE_THEME_DEFAULT 0
#define LV_USE_IMG 0
#define LV_USE_GROUP 0
#define LV_USE_LABEL 1
#define LV_USE_BTN 1
#define LV_USE_BAR 0
```

如果后续要上中文，再单独补字库，不要第一阶段就上整套中文字模。

---

## 6. LCD 刷新适配方案

### 6.1 当前驱动现状

`bsp_oled.c` 内部已经有可用底层：

- `lcd_set_window(x0, y0, x1, y1)`
- `lcd_prepare_write_ram()`
- `lcd_write_data(color)`

但这些目前是 `static`，外部不能直接作为 LVGL flush 使用。

### 6.2 建议新增 BSP 接口

在 `bsp_oled.h` 中新增：

```c
void bsp_oled_write_area_rgb565(uint16_t x,
                                uint16_t y,
                                uint16_t w,
                                uint16_t h,
                                const uint16_t *pixels);
```

在 `bsp_oled.c` 中实现思路：

1. 做边界裁剪
2. 调用 `lcd_set_window()`
3. 调用 `lcd_prepare_write_ram()`
4. 顺序写入 `w * h` 个 RGB565 像素

这个接口是 LVGL 接入的关键，不建议让 LVGL 直接穿透到 `static` 内部函数。

### 6.3 LVGL flush 回调

`ui_lvgl_port.c` 中的 `flush_cb` 可写成：

```c
static void ui_lvgl_flush_cb(lv_display_t *disp,
                             const lv_area_t *area,
                             uint8_t *px_map)
{
    uint16_t w = (uint16_t)(area->x2 - area->x1 + 1);
    uint16_t h = (uint16_t)(area->y2 - area->y1 + 1);

    bsp_oled_write_area_rgb565((uint16_t)area->x1,
                               (uint16_t)area->y1,
                               w,
                               h,
                               (const uint16_t *)px_map);

    lv_display_flush_ready(disp);
}
```

说明：

- 当前 LCD 是同步写总线，第一阶段可直接同步完成后 `flush_ready`
- 不必一开始就做 DMA 或异步刷屏

---

## 7. Draw Buffer 建议

STM32F103 不适合整帧缓冲。

240x320x2 字节 = 153600 字节，明显太大。

建议第一阶段用部分缓冲：

```c
#define UI_LVGL_BUF_LINES 16
static uint16_t g_lvgl_buf[240 * UI_LVGL_BUF_LINES];
```

内存占用：

- `240 * 16 * 2 = 7680 bytes`

这对 F103 相对可接受。

如果仍然紧张，可以降到 8 行：

- `240 * 8 * 2 = 3840 bytes`

---

## 8. 时基接入方案

### 8.1 当前工程节拍

`app_main_loop()` 已经有 1 ms 演进和调度：

- `scheduler_tick_1ms()`
- `scheduler_poll(&flags)`
- `flags.task_key_100ms`
- `flags.task_ui_200ms`

### 8.2 建议接法

在 `app_main_loop()` 的 1 ms 路径调用：

```c
lv_tick_inc(1);
```

在 UI 相关节拍里调用：

```c
lv_timer_handler();
```

建议第一阶段放在 `flags.task_ui_200ms` 中先跑通；
如果后续交互变多，再调整到 10~20ms 调用一次。

### 8.3 推荐最终节奏

第一阶段：

- `lv_tick_inc(1)`：每 1 ms
- `lv_timer_handler()`：每 20 ms 或 50 ms

不建议先挂在 200 ms，页面刷新会显得过慢。

因此建议后续在 `scheduler` 增加一个更细的 UI 任务标志，例如：

- `task_ui_fast_20ms`

---

## 9. 按键接入方案

第一阶段不要让 LVGL 直接接管全部按键。

建议分两阶段：

### 阶段一

- 继续由 `ui_service.c` 控业务页面状态机
- LVGL 只负责渲染某个页面 demo
- 按键逻辑保持现状

### 阶段二

- 把 `SET/UP/DOWN/BACK` 适配为 LVGL input device
- 让 LVGL 接管焦点与控件交互

如果第一阶段就全量接管输入，会同时改状态机和渲染层，风险太高。

---

## 10. 页面迁移顺序

建议按这个顺序迁移：

1. 首页
2. 超温报警页
3. 设定温度页
4. 参数档位页
5. 导出页
6. 自检页
7. 探头故障页

理由：

- 首页和报警页最能验证 LVGL 的基础能力
- 设定页能验证数值更新和交互
- 异常页能验证状态切换

---

## 11. 第一阶段建议新增文件职责

### `ui_lvgl_port.h`

负责暴露：

```c
void ui_lvgl_port_init(void);
void ui_lvgl_port_tick_1ms(void);
void ui_lvgl_port_task(void);
```

### `ui_lvgl_port.c`

负责：

1. `lv_init()`
2. 初始化 draw buffer
3. 注册 display
4. 注册 flush_cb
5. 对接时基

### `ui_lvgl_view.h`

负责暴露：

```c
void ui_lvgl_view_create_home(void);
void ui_lvgl_view_update_home(float t_ctrl,
                              float set_temp,
                              int heater_on,
                              int alarm_on);
```

### `ui_lvgl_view.c`

负责：

1. 创建首页容器
2. 创建标题、温度值、状态标签
3. 提供更新接口

---

## 12. 与当前 `ui_service.c` 的关系

第一阶段不删除 `ui_service.c`。

建议策略：

1. 增加一个编译开关

```c
#define APP_USE_LVGL_UI 1
```

2. 在 `app_main.c` 中做切换：

- `APP_USE_LVGL_UI=0`：走现有 `ui_service`
- `APP_USE_LVGL_UI=1`：走 LVGL 页面

这样可以双轨验证，不会一次性推翻现有 UI。

---

## 13. 最小接入步骤

建议实际执行顺序：

1. 引入 LVGL 源码到 `ThirdParty/lvgl`
2. 新增 `lv_conf.h`
3. 给 `bsp_oled` 新增 `bsp_oled_write_area_rgb565()`
4. 新增 `ui_lvgl_port.c/.h`
5. 新增 `ui_lvgl_view.c/.h`
6. 在 `app_main_init()` 调用 `ui_lvgl_port_init()`
7. 在 1 ms 路径调用 `ui_lvgl_port_tick_1ms()`
8. 在 20~50 ms 节拍调用 `ui_lvgl_port_task()`
9. 先显示首页 demo
10. 再逐页迁移

---

## 14. 风险与注意事项

1. `bsp_oled.c` 当前文本绘制和图元绘制混在一个驱动里，接入 LVGL 后建议把“文本行缓存接口”和“像素绘图接口”职责拆清。
2. F103 RAM 紧张，第一阶段必须严格控制 draw buffer 和字库大小。
3. 若未来要上中文，建议优先做固定词条字模，不要一开始就全量 Unicode。
4. 并口 TFT 同步刷屏会占用 CPU，复杂动画不适合第一阶段。

---

## 15. 建议的下一步动作

如果继续实施，下一步最值当的是两件事：

1. 先给 `bsp_oled` 增加 `bsp_oled_write_area_rgb565()`
2. 再加 `ui_lvgl_port` 的最小脚手架

这样即使 LVGL 源码还没完全进工程，底层关键适配口也已经定住，后续不会返工。
