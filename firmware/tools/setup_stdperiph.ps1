$ErrorActionPreference = "Stop"

$root = Split-Path -Parent $PSScriptRoot
$lib = Join-Path $root "ThirdParty/stm32f10x-stdperiph-lib"

if (-not (Test-Path $lib)) {
    throw "StdPeriph library not found: $lib"
}

$conf = Join-Path $root "Core/Inc/stm32f10x_conf.h"
if (-not (Test-Path $conf)) {
    throw "Missing config file: $conf"
}

$sys = Join-Path $lib "Libraries/CMSIS/CM3/DeviceSupport/ST/STM32F10x/system_stm32f10x.c"
$startup = Join-Path $lib "Libraries/CMSIS/CM3/DeviceSupport/ST/STM32F10x/startup/arm/startup_stm32f10x_hd.s"
if (-not (Test-Path $sys)) { throw "Missing system file: $sys" }
if (-not (Test-Path $startup)) { throw "Missing startup file: $startup" }

Write-Host "StdPeriph prerequisite files are present."
Write-Host "Add defines: USE_STDPERIPH_DRIVER, STM32F10X_HD"
Write-Host "See integration guide: Project/StdPeriphIntegration.md"
