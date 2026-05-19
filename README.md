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
