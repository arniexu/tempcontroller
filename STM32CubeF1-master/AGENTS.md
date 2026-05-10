# STM32 TempController Agent Profile

This file captures stable hardware facts and workflow defaults for this repository.
Use these values as the default context unless the user explicitly overrides them.

## Project Identity
- Project name: TempController
- Active project tree: Projects/MiniRBT6
- Toolchain: Keil MDK5 (ARMCC)

## Development Board
- MCU: STM32F103RB
- Board family in this repo: MiniRBT6 target
- Debug/programmer default: J-Link over SWD

## Pin Definitions (Current Wiring)
- LED0: PA8
- LED1: PD2

### LCD 8080 Bus
- Data bus D0..D15: PB0..PB15
- Backlight (BL): PC10
- Chip select (CS): PC9
- Register select / RS / DC: PC8
- Write strobe (WR): PC7
- Read strobe (RD): PC6

## LCD Parameters
- Interface: 8080 parallel bus
- Bus width: 16-bit
- Resolution: 240 x 320 (pixels)
- Pixel format: RGB565
- Driver role boundary: LCD low-level driver belongs to BSP layer

## Layering Rules
- BSP layer: LCD 8080 driver and board-level peripheral drivers
- Third-party layer: graphics library and font library
- App layer: test/demo/business sequences only

## Flash/Debug Workflow Defaults
- Prefer flash-only operations unless the user explicitly asks for debug mode.
- Keep SWD available; do not require JTAG.

## Source of Truth in Code
- Pin macros: Projects/MiniRBT6/Inc/main.h
- LCD public config/API: Projects/MiniRBT6/BSP/Inc/lcd8080.h
- LCD implementation: Projects/MiniRBT6/BSP/Src/lcd8080.c
