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

## Build Variant Rule
Treat this as a mandatory delivery rule for every code module.

1. Every code module must support both `debug` and `production` builds.
2. New modules are not considered complete unless both build variants are defined and kept buildable.
3. Changes to an existing module must preserve both build variants; do not add code that only works in one variant unless the other variant is updated in the same task.
4. Debug builds may include diagnostics, assertions, tracing, and verification hooks, but they must not change the module's intended business behavior.
5. Production builds must keep the same functional behavior while disabling or minimizing debug-only overhead.
6. Any module-level build/test/release workflow must explicitly state which steps validate `debug` and which validate `production`.
7. Hardware-dependent tests must live only in a separate Keil-driven debug build and must not be compiled into release builds.
8. Release builds must exclude hardware test scaffolding, diagnostic scenes, and driver-test entry flows unless a dedicated production requirement explicitly says otherwise.

## Debug Workflow Rule
Treat this as a mandatory debugging order for all investigation and bug-fix tasks.

1. In debug work, focus on the driver/API entry point first before inspecting higher-level functions.
2. After the driver/API entry point is verified, continue to the owning function-level logic that consumes or controls that API.
3. API tests are mandatory for debug work when a stable API boundary exists; use them to confirm the driver/API behavior before widening the investigation scope.
4. Do not start debugging from broad business logic when a narrower driver/API breakpoint, call site, or API test can discriminate the fault faster.

## Test Layering Rule
Treat this as a mandatory test design and execution order for firmware modules.

1. Tests must be layered in this order whenever a stable boundary exists: `driver API test` -> `service/function test` -> `app/integration test`.
2. Driver API tests must verify the narrowest hardware-facing or port-facing contract before service-level tests depend on that behavior.
3. Function/service tests must validate module policy and state transitions only after the underlying driver/API contract is covered.
4. App/integration tests must compose validated lower layers; they must not be the first or only evidence for a defect or feature when narrower API or service tests are feasible.
5. New modules and significant refactors must add or update tests at the narrowest applicable layer first, then preserve consistency upward through service and app coverage.
6. CI and local test documentation must state which scripts or targets validate the driver/API layer, the service/function layer, and the app/integration layer.

## Hardware Validation Rule
Treat this as a mandatory, highest-priority hardware debugging and validation rule.

1. Driver API tests must be run on physical hardware whenever the target hardware exists and the task concerns real device behavior, timing, GPIO, buses, display, sensors, relays, buzzer, UART, RTC, or any other hardware-facing effect.
2. Host tests, mocks, and simulations are useful prechecks, but they do not replace physical-hardware validation for driver API behavior.
3. For hardware work, the agent must prefer operating on the real device first through the available hardware workflow, then use host tests only as supporting evidence.
4. If a hardware step requires user assistance such as power, cabling, probe hookup, button presses, serial connection, flash/download confirmation, or observing physical output, the agent must stop at that point and explicitly ask the user to help complete that step before continuing.
5. Do not declare a hardware-facing driver/API task complete until the relevant driver API test has been executed against physical hardware or the user has explicitly accepted that hardware validation is blocked.
6. Hardware-dependent tests must rely on the Keil debug target, not on host-only runners, and must be kept separate from release build entry paths.
7. When the task requires flashing firmware to hardware, prefer flash-only programming and do not enter debugger mode unless the user explicitly asks for a debug session.
8. If a debug session is explicitly required for download or investigation, treat exiting debug mode as a required cleanup step before considering the operation complete.
9. Firmware flashing is mandatory via Keil MDK tooling (`D:\keil MDK\UV4\UV4.exe`) and must not be replaced by non-Keil flashing flows (such as pyOCD, J-Link Commander, ST-LINK Utility, OpenOCD, or custom programmers) unless the user explicitly overrides this rule.
10. For flash operations, the default required path is Keil flash-only download; entering Keil debug mode is allowed only when explicitly requested by the user and must be exited as a required cleanup step.

## LCD UI Design Authority
Treat [docs/issue/lcd-ui-preview.html](docs/issue/lcd-ui-preview.html) as the authoritative LCD visual design document for all TFT/LCD UI work.

1. Any task that changes LCD page layout, spacing, typography, color, or visual hierarchy must follow that HTML preview first, then implement firmware drawing code to match it.
2. The HTML preview defines the target 240x320 visible area. Use LCD pixel coordinates with `(0,0)` at the visible top-left corner, `x` increasing to the right, and `y` increasing downward.
3. Do not improvise new UI structure in firmware before updating the HTML design document.
4. If a UI change request conflicts with the current HTML preview, update the HTML preview first and treat that update as the design review artifact.
5. When implementing LCD UI code, preserve the design's information density, alignment, spacing, and page intent unless the HTML document is revised.
6. Decorative device chrome in the HTML preview is not part of the firmware coordinate system; only the 240x320 `.screen` region maps to LCD drawing coordinates.
