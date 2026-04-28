$ErrorActionPreference = "Stop"

Push-Location $PSScriptRoot
try {
    gcc -std=c11 -Wall -Wextra -Werror `
      -I../Core/Inc -I../Drivers/BSP -I. `
      ../Core/Src/scheduler.c `
      ../Core/Src/pid_ctrl.c `
      ../Core/Src/debug_log.c `
      ../Core/Src/alarm_service.c `
      ../Core/Src/param_store.c `
      ../Core/Src/log_service.c `
      ../Core/Src/heater_ctrl.c `
      ../Core/Src/temp_manager.c `
      ../Core/Src/schedule_service.c `
      ../Core/Src/ui_service.c `
      ../Core/Src/protocol_export.c `
      ../Core/Src/app_main.c `
      ../Drivers/BSP/bsp_ds18b20.c `
      ../Drivers/BSP/bsp_buzzer.c `
      ../Drivers/BSP/bsp_eeprom.c `
      ../Drivers/BSP/bsp_spiflash.c `
      ../Drivers/BSP/bsp_key.c `
      ../Drivers/BSP/bsp_oled.c `
      ../Drivers/BSP/tiny_graphics.c `
      ../Drivers/BSP/bsp_rtc.c `
      ../Drivers/BSP/bsp_relay.c `
      stubs/bsp_uart_test_stub.c `
      test_scheduler.c test_pid.c test_debug_log.c test_alarm_service.c test_param_store.c `
      test_log_service.c test_heater_ctrl.c test_temp_manager.c test_schedule_service.c `
      test_ui_service.c test_protocol_export.c test_app_main.c test_runner.c `
      -o module_tests.exe

    ./module_tests.exe
}
finally {
    Pop-Location
}
