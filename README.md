# tempcontroller

a mcu project used to test ai workflow

## STM32Cube HAL + Keil MDK 工程骨架

已添加一个基于 **STM32Cube HAL** 的最小工程骨架（目标器件：`STM32F103ZE`）：

- Keil 工程：`/home/runner/work/tempcontroller/tempcontroller/MDK-ARM/tempcontroller.uvprojx`
- 主程序：`/home/runner/work/tempcontroller/tempcontroller/Core/Src/main.c`
- MSP/FSMC GPIO：`/home/runner/work/tempcontroller/tempcontroller/Core/Src/stm32f1xx_hal_msp.c`
- CubeMX 配置：`/home/runner/work/tempcontroller/tempcontroller/tempcontroller.ioc`

### 已体现的硬件连接

- FSMC Bank3 (`NE3`) -> SRAM (`IS62WV51216`)
- FSMC Bank4 (`NE4`) -> LCD CS
- `A12` -> LCD RS（命令/数据选择）
- `NOE/NWE` 复用到 LCD_RD/LCD_WR 与 SRAM_NOE/SRAM_NWE
- 16 位数据总线 `D0..D15`
- `NBL0/NBL1` 对应 SRAM `LB/UB`
- `LCD_RESET` 接系统 `NRST`（硬件复位，不占用普通 GPIO）

代码中提供了 LCD 命令/数据访问映射：

- `LCD_REG16`：命令地址
- `LCD_RAM16`：数据地址（A12=1 偏移）

### 使用方式

1. 在 STM32CubeMX 打开 `tempcontroller.ioc`，按需补全/校验完整 pin 与时序配置。
2. 选择 `MDK-ARM` 重新生成代码（可覆盖本骨架）。
3. 在 Keil MDK 打开 `MDK-ARM/tempcontroller.uvprojx`，确认安装 `Keil.STM32F1xx_DFP` pack 后构建。
