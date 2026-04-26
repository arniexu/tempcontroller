# WaterTempControl Firmware (STM32F103)

This folder contains the firmware implementation for the STM32F103-based water temperature controller, including application logic, hardware abstraction ports, BSP drivers, and host-side unit tests.

## Current status

- Core services are implemented: scheduler, temperature manager, heater control, PID, alarm, schedule, UI, logging, parameter store, and protocol export.
- Hardware separation is in place: business logic uses HW ports, while GPIO/I2C/UART details stay in BSP.
- Host tests are available for module-level verification.

## Folder layout

- `Core/Inc`: public interfaces and service contracts.
- `Core/Src`: business logic and orchestration.
- `Drivers/BSP`: STM32 StdPeriph hardware drivers and host mock paths.
- `tests`: host-side test runner, module tests, and stubs.
- `Project/MDK-ARM`: Keil project and build outputs.

## Runtime model

- 1 ms task: scheduler tick and heater window update.
- 100 ms task: key scan and input service.
- 200 ms task: UI render/refresh.
- 1 s task: sensor sampling, control loop, alarm check, schedule evaluation, and log processing.

## Detailed Dysfunction Description (Failure Modes and Safe Behavior)

This section describes expected behavior under abnormal input, hardware faults, and edge conditions.

### 1) Sensor path dysfunction

- Symptom: DS18B20 timeout, CRC error, or repeated read failure.
- Detection: driver diagnostics counters and consecutive failure logic.
- System behavior: invalid samples are rejected, degraded/fault state is raised, and control uses safe logic rather than trusting bad sensor values.
- Risk controlled: avoids unsafe heating decisions from corrupted or stale temperature data.

### 2) Protocol command dysfunction

- Symptom: malformed command syntax, trailing extra payload, NaN/Inf numeric values, or out-of-range configuration values.
- Detection: strict parser validation in protocol command handlers.
- System behavior: command is rejected and explicit error response is returned (for example BAD_xxx, OUT_OF_RANGE, UNKNOWN_CMD).
- Risk controlled: prevents invalid runtime configuration from entering control/alarm/schedule parameters.

### 3) Storage dysfunction (EEPROM)

- Symptom: I2C transfer error, timeout, or busy/retry exhaustion while loading or saving parameters.
- Detection: EEPROM status path and write/read return codes.
- System behavior: failed transaction is rejected; previous valid configuration remains effective.
- Risk controlled: avoids applying partially written or invalid persistent data.

### 4) RTC/timebase dysfunction

- Symptom: RTC source not ready or invalid time read.
- Detection: RTC readiness checks and return status from time query.
- System behavior: schedule logic handles invalid time gracefully instead of forcing an unsafe state transition.
- Risk controlled: avoids unpredictable schedule-triggered heater operation when clock source is unreliable.

### 5) UI/key input dysfunction

- Symptom: key bounce/noise or edge-trigger anomalies.
- Detection: periodic scan plus state tracking.
- System behavior: key actions are interpreted via controlled input flow; invalid transitions do not directly force critical outputs.
- Risk controlled: prevents accidental configuration jumps due to electrical noise.

### 6) Actuator path dysfunction

- Symptom: relay control request under fault/degraded context.
- Detection: control mode and alarm/safety checks in service layer.
- System behavior: safety conditions dominate and can force conservative actuator state.
- Risk controlled: heater output does not remain active when fault criteria are met.

### 7) Build-path dysfunction (host vs target drift)

- Symptom: behavior mismatch between host test and MCU target builds.
- Detection: dual-path compilation and module self-tests.
- System behavior: shared state visibility and interfaces are aligned to reduce path-specific divergence.
- Risk controlled: lowers regression risk where one build passes but another fails.

## Hardware Pinout Description (from current BSP implementation)

The following pin map reflects the current firmware source. Verify against your PCB revision before production flashing.

### GPIO and peripheral mapping

- `PA0`: KEY_SET input, EXTI0 (active-low with pull-up).
- `PA1`: KEY_UP input, EXTI1 (active-low with pull-up).
- `PA2`: KEY_DOWN input, EXTI2 (active-low with pull-up).
- `PA3`: KEY_BACK input, EXTI3 (active-low with pull-up).
- `PA4`: DS18B20 sensor #1 OneWire data (with EXTI4 fall detection path).
- `PA5`: DS18B20 sensor #2 OneWire data (with EXTI5 fall detection path).
- `PA6`: DS18B20 sensor #3 OneWire data (with EXTI6 fall detection path).
- `PA9`: USART1_TX (protocol/debug output).
- `PA10`: USART1_RX (protocol command input).
- `PB6`: I2C1_SCL for EEPROM.
- `PB7`: I2C1_SDA for EEPROM.
- `PB10`: I2C2_SCL for OLED.
- `PB11`: I2C2_SDA for OLED.
- `PB12`: Relay output.
- `PB13`: Buzzer output.

### Logical peripheral summary

- Temperature sensors: 3 x DS18B20 on PA4/PA5/PA6.
- EEPROM: I2C1, 7-bit address from `APP_EEPROM_I2C_ADDR_7BIT` (default 0x50).
- OLED: I2C2, fixed 7-bit device address 0x3C.
- UART protocol channel: USART1 at `APP_UART_BAUDRATE` (default 115200).
- RTC: backup-domain RTC (LSE preferred, LSI fallback), no dedicated GPIO in driver API.

### Pin conflict notes

- Current DS18B20 lines are on `PA4/PA5/PA6`.
- Current EEPROM lines are on `PB6/PB7`.
- This avoids DS18B20 and EEPROM bus overlap present in older mappings.

## Integration notes

1. Build target mode with `USE_STDPERIPH_DRIVER` for MCU deployment.
2. Set `APP_USE_MOCK_TEMP_SOURCE` to `0` for real DS18B20 hardware.
3. Confirm board pull-ups for OneWire and both I2C buses.
4. Validate relay/buzzer active level on your hardware revision.
5. For persistence wear reduction, keep deferred parameter flush enabled (`APP_PARAM_STORE_FLUSH_DELAY_S`).

## Configuration defaults (selected)

- Temperature setpoint default: 45.0 C.
- Alarm threshold default: 60.0 C.
- PID window: 10000 ms.
- UART baudrate: 115200.
- EEPROM I2C speed: 100 kHz.
- Scheduler periods: 100 ms (keys), 200 ms (UI), 1000 ms (control).

## References

- Integration guide: `Project/StdPeriphIntegration.md`.
- Keil project: `Project/MDK-ARM/WaterTempControl.uvprojx`.
- Host tests: `tests/run_tests.ps1`.
