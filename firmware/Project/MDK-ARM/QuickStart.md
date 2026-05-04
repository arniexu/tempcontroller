# Keil MDK Quick Start

A project skeleton has been generated:

- WaterTempControl.uvproj
- WaterTempControl.uvopt

## 1) Open project

Open WaterTempControl.uvproj in Keil uVision.

## 2) Create/duplicate target for STM32F103RCT6

Use High-density target settings and make sure:

- Device: STM32F103RC
- Defines: USE_HAL_DRIVER, STM32F10X_HD

## 3) Include paths

Add these include paths in C/C++ settings:

- ..\..\Core\Inc
- ..\..\Drivers\BSP
- ..\..\ThirdParty\STM32CubeF1\Drivers\STM32F1xx_HAL_Driver\Inc
- ..\..\ThirdParty\STM32CubeF1\Drivers\CMSIS\Include
- ..\..\ThirdParty\STM32CubeF1\Drivers\CMSIS\Device\ST\STM32F1xx\Include

## 4) Add files to groups

See SourceList.txt for exact source list.

## 5) Startup/System

Ensure project contains:

- HAL/CMSIS startup and system files for STM32F103 target

## 6) Remove template demo files

Template main.c and eval-board files should not be kept. Use only files from SourceList.txt.
