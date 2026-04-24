# Firmware Skeleton (STM32F103RCT6)

This folder contains the first implementation baseline for the water temperature controller.

## Current status

- Application architecture and module boundaries are created.
- Main loop, scheduler, PID core, alarm path, and heater percentage control loop are connected.
- Hardware-facing modules are placeholders and ready for real driver integration.

## Folder layout

- Core/Inc: public module interfaces
- Core/Src: module implementations

## Integration notes

1. Connect bsp_ds18b20.c to real OneWire read sequence based on STM32 StdPeriph.
2. Map heater_ctrl.c state to real relay GPIO output.
3. Implement OLED pages and key handling in ui_service.c.
4. Implement RTC-based booking logic in schedule_service.c.
5. Implement NV persistence in param_store.c using internal Flash dual-page journal (A/B) with version, sequence, and CRC32.
6. Enable USE_STDPERIPH_DRIVER and connect bsp_uart.c to your board-level USART IRQ/DMA strategy.

## Runtime model

- 1 ms: scheduler tick and heater window update
- 100 ms: key service
- 200 ms: UI refresh
- 1 s: sampling, PID, alarm, schedule, and logging

## StdPeriph integration

- Integration guide: Project/StdPeriphIntegration.md
- Quick validation script: tools/setup_stdperiph.ps1
- DS18B20 target mode: set APP_USE_MOCK_TEMP_SOURCE to 0 and use PB6/PB7/PB8 OneWire lines.
