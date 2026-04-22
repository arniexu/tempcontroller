# Keil MDK Quick Start

A project skeleton has been generated:

- WaterTempControl.uvproj
- WaterTempControl.uvopt

## 1) Open project

Open WaterTempControl.uvproj in Keil uVision.

## 2) Create/duplicate target for STM32F103RCT6

Use High-density target settings and make sure:

- Device: STM32F103RC
- Defines: USE_STDPERIPH_DRIVER, STM32F10X_HD

## 3) Include paths

Add these include paths in C/C++ settings:

- ..\..\Core\Inc
- ..\..\Drivers\BSP
- ..\..\ThirdParty\stm32f10x-stdperiph-lib\Libraries\CMSIS\CM3\CoreSupport
- ..\..\ThirdParty\stm32f10x-stdperiph-lib\Libraries\CMSIS\CM3\DeviceSupport\ST\STM32F10x
- ..\..\ThirdParty\stm32f10x-stdperiph-lib\Libraries\STM32F10x_StdPeriph_Driver\inc

## 4) Add files to groups

See SourceList.txt for exact source list.

## 5) Startup/System

Ensure project contains:

- ..\..\ThirdParty\stm32f10x-stdperiph-lib\Libraries\CMSIS\CM3\DeviceSupport\ST\STM32F10x\system_stm32f10x.c
- ..\..\ThirdParty\stm32f10x-stdperiph-lib\Libraries\CMSIS\CM3\DeviceSupport\ST\STM32F10x\startup\arm\startup_stm32f10x_hd.s

## 6) Remove template demo files

Template main.c and eval-board files should not be kept. Use only files from SourceList.txt.
