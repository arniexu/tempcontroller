# STM32F10x StdPeriph Integration Guide

This project uses STM32F103R8T6 and should be built with STM32F10x Standard Peripheral Library as the hardware driver base.

## 1. Add preprocessor defines

- USE_STDPERIPH_DRIVER
- STM32F10X_MD

## 2. Add include paths

- Core/Inc
- Drivers/BSP
- ThirdParty/stm32f10x-stdperiph-lib/Libraries/CMSIS/CM3/CoreSupport
- ThirdParty/stm32f10x-stdperiph-lib/Libraries/CMSIS/CM3/DeviceSupport/ST/STM32F10x
- ThirdParty/stm32f10x-stdperiph-lib/Libraries/STM32F10x_StdPeriph_Driver/inc

## 3. Add required CMSIS startup/system sources

- ThirdParty/stm32f10x-stdperiph-lib/Libraries/CMSIS/CM3/DeviceSupport/ST/STM32F10x/system_stm32f10x.c
- ThirdParty/stm32f10x-stdperiph-lib/Libraries/CMSIS/CM3/DeviceSupport/ST/STM32F10x/startup/arm/startup_stm32f10x_md.s

## 4. Add StdPeriph driver sources used by current firmware

- ThirdParty/stm32f10x-stdperiph-lib/Libraries/STM32F10x_StdPeriph_Driver/src/misc.c
- ThirdParty/stm32f10x-stdperiph-lib/Libraries/STM32F10x_StdPeriph_Driver/src/stm32f10x_bkp.c
- ThirdParty/stm32f10x-stdperiph-lib/Libraries/STM32F10x_StdPeriph_Driver/src/stm32f10x_flash.c
- ThirdParty/stm32f10x-stdperiph-lib/Libraries/STM32F10x_StdPeriph_Driver/src/stm32f10x_fsmc.c
- ThirdParty/stm32f10x-stdperiph-lib/Libraries/STM32F10x_StdPeriph_Driver/src/stm32f10x_gpio.c
- ThirdParty/stm32f10x-stdperiph-lib/Libraries/STM32F10x_StdPeriph_Driver/src/stm32f10x_i2c.c
- ThirdParty/stm32f10x-stdperiph-lib/Libraries/STM32F10x_StdPeriph_Driver/src/stm32f10x_pwr.c
- ThirdParty/stm32f10x-stdperiph-lib/Libraries/STM32F10x_StdPeriph_Driver/src/stm32f10x_rcc.c
- ThirdParty/stm32f10x-stdperiph-lib/Libraries/STM32F10x_StdPeriph_Driver/src/stm32f10x_rtc.c
- ThirdParty/stm32f10x-stdperiph-lib/Libraries/STM32F10x_StdPeriph_Driver/src/stm32f10x_spi.c
- ThirdParty/stm32f10x-stdperiph-lib/Libraries/STM32F10x_StdPeriph_Driver/src/stm32f10x_tim.c
- ThirdParty/stm32f10x-stdperiph-lib/Libraries/STM32F10x_StdPeriph_Driver/src/stm32f10x_usart.c

## 5. Add application sources

Add all sources under:
- Core/Src
- Drivers/BSP

Do not add host test stubs in firmware/tests/stubs when building target firmware.

## 6. Notes

- File Core/Inc/stm32f10x_conf.h has been added and is used by StdPeriph drivers.
- Current BSP drivers already contain USE_STDPERIPH_DRIVER branches.
- DS18B20 StdPeriph bit-bang implementation is enabled when APP_USE_MOCK_TEMP_SOURCE is set to 0.
- DS18B20 remapped pin mapping: PA4 -> T1, PA5 -> T2, PA6 -> T3 (external 4.7k pull-up required for each line).
- Display target is now the ALIENTEK 2.4/2.8 inch TFTLCD parallel module rather than the SSD1306 OLED.
- The board-side TFTLCD bus occupies PB0..PB15 and PC6..PC10, with touch on PC0..PC3 and PC13.
- This collides with the current relay, buzzer, and historical OLED/EEPROM pin map, so display replacement must be coordinated with a full pin-map update rather than a driver-only swap.
