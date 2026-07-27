# Edgi-Talk_M55_Driver_All Example Project

[**Chinese**](./README_zh.md) | **English**

## Introduction

This project is based on the **Edgi-Talk platform** and runs on **RT-Thread on the Cortex-M55 core**.
It collects common M55 peripheral demos in one firmware, including AHT20, LSM6DS3, audio loopback, WAV playback, ADC, HyperRAM, GPIO key interrupt, RTC, SD card, Flash filesystem, and CoreMark.

The demos are exposed as MSH commands, so peripherals are tested manually instead of all running automatically after boot.

### Command Overview

| Command | Description |
| ---- | ---- |
| `demo_aht20` | Read AHT10/AHT20 temperature and humidity |
| `demo_lsm6ds3 [count] [delay_ms]` | Read LSM6DS3 accelerometer/gyroscope data |
| `demo_audio start/stop/status [speaker_volume] [mic_gain]` | ES8388 speaker and PDM microphone loopback |
| `wavplay -s <file>` | Play a WAV file |
| `demo_adc` | Read ADC1 channel 1 voltage |
| `demo_hyperram` | Run a basic HyperRAM read/write check |
| `demo_hyperram_speed [size_kb] [loops]` | Run a HyperRAM bandwidth test |
| `demo_key` | Register the P8.3 key interrupt and toggle the blue LED |
| `demo_rtc [YYYY MM DD HH MM SS]` | Read or set RTC time |
| `demo_sdcard` | Run an SD card file read/write test |
| `demo_sdcard_speed [total_kb] [block_kb]` | Run an SD card sequential speed test |
| `demo_fs_benchmark [file] [total_kb] [block_kb] [random_ops]` | Run a DFS/POSIX filesystem benchmark |
| `demo_flash_speed [total_kb] [block_kb]` | Run a `/flash` sequential speed test |
| `core_mark` | Run CoreMark |

## Software Description

* The project is developed based on the **Edgi-Talk** platform.
* This project is the Cortex-M55 RT-Thread application. The M33 firmware must enable and start the M55 core during board initialization.
* Each peripheral demo is triggered from the serial shell through an MSH command.
* Audio playback and loopback share `sound0`; stop `demo_audio` before running `wavplay`.
* SD card mounting and `/flash` filesystem mounting are handled by the common filesystem initialization logic.

## Usage Instructions

### Compilation and Download

1. Make sure the M33 boot project has enabled the CM55 core.
2. Open this project and complete the compilation.
3. Connect the board USB port to the PC using the **onboard debugger (DAP)**.
4. Use the programming tool or the RT-Thread Studio launch configuration to flash the generated firmware onto the development board.

### Runtime Behavior

* After the M33 boot flow starts the M55 core, RT-Thread runs on M55 and enters the MSH shell.
* Run the required `demo_*` command from the serial terminal to test a peripheral.
* Only run one resource-heavy demo at a time, especially audio, SD card, Flash, and CoreMark tests.

```text
 \ | /
- RT -     Thread Operating System
 / | \     5.0.2 build Sep  5 2025 14:13:02
 2006 - 2022 Copyright by RT-Thread team
Hello RT-Thread
It's cortex-m55
msh >
```

## Common Demos

### AHT20 Temperature and Humidity Demo

```sh
demo_aht20
```

Reads AHT10/AHT20 temperature and humidity once, then prints the result through the serial console.

### LSM6DS3 6-Axis Sensor Demo

```sh
demo_lsm6ds3
demo_lsm6ds3 10 200
```

`count` is the number of samples, and `delay_ms` is the interval between samples.

### Audio Loopback Demo

```sh
demo_audio start
demo_audio start 60 30
demo_audio status
demo_audio stop
```

`speaker_volume` is the speaker volume, and `mic_gain` is the microphone gain. This demo occupies `mic0` and `sound0`.

### WavPlayer Audio Playback Demo

```sh
wavplay -s /sdcard/16000.wav
wavplay -t
wavplay -p
wavplay -r
wavplay -v 80
```

`wavplay` and `demo_audio` both occupy `sound0`. Run `demo_audio stop` before playing a WAV file.

### ADC Demo

```sh
demo_adc
```

Reads the voltage value of ADC1 Channel 1.

### HyperRAM Demo

```sh
demo_hyperram
demo_hyperram_speed
demo_hyperram_speed 1024 8
```

Validates basic HyperRAM read/write access and sequential bandwidth.

### Key Interrupt Demo

```sh
demo_key
```

Registers a falling-edge interrupt on the P8.3 key and toggles the P16.5 blue LED whenever the key is pressed.

### RTC Demo

```sh
demo_rtc
demo_rtc 2026 7 17 10 30 0
```

Reads the current RTC time, or sets the time and reads it back.

### SD Card Demo

```sh
demo_sdcard
demo_sdcard_speed
demo_sdcard_speed 8192 64
demo_fs_benchmark
demo_fs_benchmark /sdcard/demo_fs_benchmark.bin 5120 64 64
```

The SD card is mounted automatically to `/sdcard` after insertion.

### Flash Filesystem Demo

```sh
demo_flash_speed
demo_flash_speed 512 4
```

This command creates a temporary speed-test file under `/flash` and deletes it after the test.

### CoreMark Benchmark Demo

```sh
core_mark
```

Stop other high-load demos before running CoreMark to avoid SD card, audio, or sensor polling affecting the result.

## Notes

> Note: This project requires **RT-Thread Studio 2.2.9** or higher.

* To modify the graphical configuration of the project, open the configuration file using the following tool:

```text
tools/device-configurator/device-configurator.exe
libs/TARGET_APP_KIT_PSE84_EVAL_EPC2/config/design.modus
```

* After editing, save the configuration and regenerate the code.
* Format the SD card as FAT16 or FAT32.
* `demo_flash_speed` creates and removes a temporary file under `/flash`; avoid running large repeated tests on partitions that contain important data.
* If this project does not run correctly, compile and flash the **Edgi_Talk_M33_Template** project first to ensure proper initialization and core startup sequence.

## Boot Sequence

The system boot sequence is as follows:

```text
+------------------+
|   Secure M33     |
|   (Secure Core)  |
+------------------+
          |
          v
+------------------+
|       M33        |
| (Non-Secure Core)|
+------------------+
          |
          v
+-------------------+
|       M55         |
| (Application Core)|
+-------------------+
```

The M55 application depends on the M33 boot flow. To enable the M55 core, configure the **M33 project** as follows:

```text
RT-Thread Settings --> Hardware --> select SOC Multi Core Mode --> Enable CM55 Core
```
