# tempcontroller
a mcu project used to test ai workflow

## 测试环境

第一阶段测试环境采用**封闭泡沫箱**，具体设计如下：

- 使用密封泡沫箱作为温控腔体，提供良好的隔热效果
- 在箱体上开若干小孔，分别用于穿入温度传感器和热源（加热元件）
- 所有小孔穿线后做封闭处理，确保箱体内部保持密闭状态
- 封闭结构有助于建立稳定的热平衡，便于验证温控算法效果

## OLED UI（LVGL）

- `oled_ui_lvgl_draw.c`：96x96 OLED 温控界面 LVGL 绘制示例
- 包含状态标签、目标温度/允许误差、中心大号当前温度与右下误差快照

### LVGL 体量评估（STM32F103 + 96x96 OLED）

- 对 F103（RAM/Flash 紧张）来说，**完整 LVGL 通常偏重**，尤其在开启多字体、动画、复杂控件时。
- 对 `96x96` 小尺寸 OLED，仅显示温度/状态/菜单时，优先建议轻量绘制方案。

可选更小图形库：

- **u8g2**：面向单色 OLED/液晶，生态成熟，字体和绘图齐全，资源占用通常低于 LVGL。
- **u8x8（u8g2 子集）**：文本 UI 极轻量，适合参数菜单和状态页。
- **SSD1306/SH110x 专用驱动 + 自绘字库**：最省资源，代价是 UI 组件需自行维护。
- **Adafruit GFX（移植版）**：接口简单，功能比 u8x8 强，体量通常仍小于 LVGL。

## STM32F103RBT6 最小集成骨架（HAL + FreeRTOS + 设备驱动）

已新增目录：`firmware/`

- `Core/Inc/board_pins.h`：管脚定义（可按实际硬件改）
- `Core/Inc/tempcontroller_config.h`：控制周期与默认参数
- `Core/Src/tempcontroller_app.c`：FreeRTOS 任务与状态机骨架
- `Drivers/ADS1220`：SPI 读数驱动骨架
- `Drivers/OLED96X96`：I2C OLED 驱动骨架
- `Drivers/EC11`：EC11 编码器输入层（A/B 旋转 + KEY）

当前默认引脚分配：

- OLED(I2C1): `PB6(SCL)` `PB7(SDA)` 地址 `0x3C`
- ADS1220(SPI1): `PA5(SCK)` `PA6(MISO)` `PA7(MOSI)` `PA4(CS)` `PB0(DRDY)` `PB1(RST)`
- EC11: `PA0(A)` `PA1(B)` `PA2(KEY)`

接入步骤：

1. 在 CubeMX 工程中启用 `I2C1 / SPI1 / GPIO EXTI / FreeRTOS`。
2. 将 `firmware` 下文件加入工程。
3. 在 `main()` 中调用：
   - `tempcontroller_app_init();`
   - `tempcontroller_app_start();`
4. 启动调度器 `osKernelStart()`。

说明：

- 该骨架用于快速落地“可运行最小系统”，OLED/ADS1220初始化序列可按具体屏/传感器接线微调。
- STM32F103RBT6 可用但资源不宽裕，建议持续跟踪各任务栈水位与总RAM占用。

## PID 自整定（已移植：中继法 Relay Auto-Tune）

- 新增：
  - `firmware/Core/Inc/tempcontroller_pid_autotune.h`
  - `firmware/Core/Src/tempcontroller_pid_autotune.c`
- 在 `task_control` 控制流程中已接入一轮自整定：
  - 根据目标温度上下滞回做中继切换（加热开/关）
  - 采集振荡周期与幅值
  - 计算并回写 `Kp/Ki/Kd`（Ziegler-Nichols 经典 PID 参数）
- 默认参数位于 `firmware/Core/Inc/tempcontroller_config.h`：
  - `TEMPCONTROLLER_ENABLE_AUTOTUNE`
  - `TEMPCONTROLLER_AUTOTUNE_RELAY_C`
  - `TEMPCONTROLLER_AUTOTUNE_HYST_C`
  - `TEMPCONTROLLER_AUTOTUNE_CYCLES`
  - `TEMPCONTROLLER_AUTOTUNE_TIMEOUT_MS`
