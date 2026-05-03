# PlatformIO HAL 支持

本目录提供基于 STM32 HAL (`framework = stm32cube`) 的 PlatformIO 构建入口。

## 目标

- 复用现有业务层（`Core/Src`）代码。
- 不破坏现有 Keil + StdPeriph 路径。
- 使用 `APP_HW_PLATFORM_PORT_CUSTOM` / `APP_HW_TEMP_PORT_CUSTOM` 接入 HAL 端口实现。
- 平台端口优先复用现有 BSP HAL 驱动能力。

## 文件说明

- `../../platformio.ini`：PlatformIO 环境定义。
- `../../ProjectConfig/project_config_select.h`：项目配置选择器，根据构建宏载入项目配置。
- `../../ProjectConfig/project_configs/`：每个项目独立配置头（如 `project_stm32f103.h`、`project_edgegateway.h`）。
- `../../ProjectConfig/bsp_config_select.h`：BSP 配置选择器，根据构建宏载入 BSP 配置。
- `../../ProjectConfig/bsp_configs/`：每个项目独立 BSP 配置头（引脚映射、外设时钟使能、UART/EEPROM/SPI Flash 等板级参数）。
- `hal/src/main_hal.c`：HAL 启动入口和时钟配置。
- `hal/src/hw_platform_port_hal.c`：平台端口 HAL 实现（调用 BSP HAL 驱动）。
- `hal/src/hw_temp_port_hal.c`：温度端口实现（3 路 DS18B20 OneWire HAL 位操作）。
- `hal/include/stm32f1xx_hal_conf.h`：HAL 模块配置头。

## 配置分层（PlatformIO）

构建配置按三层组织：

- 公共层：`[env]`，仅放通用构建项与源文件清单。
- 芯片层：`[env:chip-stm32f103]`、`[env:chip-stm32f407vgt6]`，放板卡/芯片与芯片相关 flags。
- 项目+构建层：`temperatureSensor-*`、`edgegateway-*`，放项目宏与 Debug/Release 开关。

这种方式能保证 `build / project / chip` 职责清晰，新增项目或芯片时不需要复制整段配置。

## 制度化约束（执行清单）

每次修改 `platformio.ini` 必须逐项自检：

1. 是否仍保持三层结构：公共层、芯片层、项目+构建层。
2. `APP_PROJECT_*` 仅出现在项目+构建层。
3. `APP_BUILD_*` 仅出现在项目+构建层。
4. `APP_CHIP_*` 仅出现在芯片层。
5. 每个项目是否同时存在 `Debug` 与 `Release` 环境。
6. 历史别名环境是否仅通过 `extends` 指向规范环境。
7. 产物命名是否在 `firmware/platformio/hal/pio_progname.py` 中保持可映射。
8. 组件开关是否只在项目配置头定义（`COMP_*`），且 APP 兼容宏仅作映射。
9. 组件依赖与互斥是否满足规则（当前要求 EEPROM/SPIFLASH 不可同时启用）。

禁止项（出现任一条即判定为不合规）：

1. 在 `[env]` 放项目身份宏或构建形态宏。
2. 通过复制粘贴创建“完整重复环境块”而不使用 `extends`。
3. 只增加 `Debug` 或只增加 `Release` 单边环境。
4. 在多个文件分散定义产物命名规则。
5. 在 `platformio.ini` 或业务代码中直接新增组件身份宏，而不是通过项目配置头管理 `COMP_*`。

## 构建验证矩阵（提交模板）

涉及 PlatformIO 配置变更时，提交说明必须包含以下矩阵：

| Environment | Status | Artifact | Note |
| --- | --- | --- | --- |
| temperatureSensor-Debug | SUCCESS/FAILED | `WaterTempControl_temperatureSensor-Debug.*` |  |
| temperatureSensor-Release | SUCCESS/FAILED | `WaterTempControl_temperatureSensor-Release.*` |  |
| edgegateway-Debug | SUCCESS/FAILED | `WaterTempControl_edgegateway-Debug.*` |  |
| edgegateway-Release | SUCCESS/FAILED | `WaterTempControl_edgegateway-Release.*` |  |

## 项目配置宏

- F103 环境默认定义 `APP_PROJECT_STM32F103=1`。
- EdgeGateway 环境定义 `APP_PROJECT_EDGEGATEWAY=1`。
- 新增项目时：
	1. 在 `ProjectConfig/project_configs/` 新建一个配置头。
	2. 在 `project_config_select.h` 增加对应分支。
	3. 在 `platformio.ini` 新建环境并传入该项目宏。

## 使用

temperatureSensor 调试构建：

```bash
pio run -e temperatureSensor-Debug
```

temperatureSensor 发布构建：

```bash
pio run -e temperatureSensor-Release
```

EdgeGateway 调试构建：

```bash
pio run -e edgegateway-Debug
```

EdgeGateway 发布构建：

```bash
pio run -e edgegateway-Release
```

兼容旧命令（别名，建议逐步迁移到新环境名）：

```bash
pio run -e stm32f103rb_hal
pio run -e edgegateway_hal
```

串口监视：

```bash
pio device monitor -b 115200
```

下载：

```bash
pio run -e temperatureSensor-Debug -t upload
```

## 说明

- OLED/EEPROM/SPI Flash/UART/按键/继电器/蜂鸣器/RTC 通过 BSP HAL 驱动路径运行。
- 温度采集使用 `hw_temp_port_hal.c` 中的 DS18B20 OneWire HAL 实现（PA1/PA3/PA4）。