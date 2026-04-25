# AGENTS

## Purpose
This repository enforces a strict separation between business logic and hardware logic.

## Harvard Logic Separator Base Rule
Treat this as a mandatory architecture rule for all implementation and refactoring tasks.

1. Business layer must not directly include or call BSP/MCU drivers.
2. Hardware interaction must go through substrate ports (hardware abstraction interfaces).
3. Business modules may depend on Core domain/service headers and hardware port headers only.
4. BSP/MCU-specific types, constants, and register details must stay in hardware substrate implementation.
5. No register-level logic in business modules.
6. No scheduling/control policy decisions inside BSP modules.

## Layer Contract
1. Business layer owns: control policy, PID behavior, alarm policy, schedule policy, parameter policy, UI behavior, protocol behavior.
2. Hardware substrate owns: GPIO/TIM/UART/I2C/RTC/OneWire/EEPROM access and ISR-facing mechanics.
3. App orchestration may coordinate modules but should still call hardware through substrate ports.

## Allowed Dependencies
1. Core business files in firmware/Core/Src may include:
- Core headers in firmware/Core/Inc
- Port headers such as hw_platform_port.h and hw_temp_port.h
2. Core business files must not include:
- bsp_*.h directly
- stm32f10x*.h directly

## Refactoring Rule
When a business module currently calls BSP directly, introduce or extend a hardware substrate port and replace direct calls with port calls.

## Testability Rule
1. Business logic tests must run without target MCU registers.
2. Hardware substrate should provide mockable or swappable implementations for host tests.

## Pull Request Acceptance Rule
Any new feature or fix that crosses business/hardware boundaries must preserve this separator contract.
