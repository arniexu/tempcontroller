# Module Self-Tests

This folder contains per-module self-tests for the firmware architecture.

## Full test plan

- Overall plan: ../../docs/整体测试计划.md
- Recommended order:
  1. Unit self-tests: run_tests.ps1
  2. Stress checks: run_stress_tests.ps1
  3. Performance checks: run_perf_checks.ps1

## Covered modules

- scheduler
- pid_ctrl
- debug_log
- alarm_service
- param_store
- log_service
- heater_ctrl
- temp_manager
- schedule_service
- ui_service
- protocol_export
- app_main (smoke)

## Example host build (MinGW GCC)

```powershell
gcc -std=c11 -Wall -Wextra -Werror \
  -I../Core/Inc -I../Drivers/BSP -I. \
  ../Core/Src/scheduler.c \
  ../Core/Src/pid_ctrl.c \
  ../Core/Src/alarm_service.c \
  ../Core/Src/param_store.c \
  ../Core/Src/log_service.c \
  ../Core/Src/heater_ctrl.c \
  ../Core/Src/temp_manager.c \
  ../Core/Src/schedule_service.c \
  ../Core/Src/ui_service.c \
  ../Core/Src/protocol_export.c \
  ../Core/Src/app_main.c \
  ../Drivers/BSP/bsp_ds18b20.c \
  ../Drivers/BSP/bsp_spiflash.c \
  ../Drivers/BSP/bsp_rtc.c \
  ../Drivers/BSP/bsp_relay.c \
  stubs/bsp_uart_test_stub.c \
  test_scheduler.c test_pid.c test_alarm_service.c test_param_store.c \
  test_log_service.c test_heater_ctrl.c test_temp_manager.c test_schedule_service.c \
  test_ui_service.c test_protocol_export.c test_app_main.c test_runner.c \
  -o module_tests.exe
```

Run:

```powershell
.\module_tests.exe
```

## Script shortcuts

```powershell
.\run_tests.ps1
.\run_stress_tests.ps1
.\run_perf_checks.ps1
```
