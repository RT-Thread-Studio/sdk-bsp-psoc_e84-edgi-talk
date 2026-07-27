# Edgi-Talk_M55_Driver_All 多功能 Demo 工程

**中文** | [**English**](./README.md)

## 简介

本工程基于 **Edgi-Talk 平台**，运行于 **RT-Thread 实时操作系统 (M55 核)**，集中集成多个常用外设 demo，便于在同一个固件中验证传感器、ADC、HyperRAM、按键中断、RTC、SD 卡文件系统、片外 Flash 文件系统、音频播放和 CoreMark 性能测试功能。

所有 demo 都通过 MSH 命令手动触发，避免多个外设测试在上电后同时运行。

### 命令总览

| 命令 | 功能 |
| ---- | ---- |
| `demo_aht20` | 读取 AHT10/AHT20 温湿度 |
| `demo_lsm6ds3 [count] [delay_ms]` | 读取 LSM6DS3 六轴传感器数据 |
| `demo_audio start/stop/status [speaker_volume] [mic_gain]` | 音频录音到播放实时回环 |
| `wavplay -s <file>` | 播放 WAV 音频文件 |
| `demo_adc` | 读取 ADC1 Channel 1 电压 |
| `demo_hyperram` | HyperRAM 基础读写校验 |
| `demo_hyperram_speed [size_kb] [loops]` | HyperRAM 带宽测试 |
| `demo_key` | 注册 P8.3 按键中断，按下后翻转蓝色 LED |
| `demo_rtc [YYYY MM DD HH MM SS]` | 读取或设置 RTC 时间 |
| `demo_sdcard` | SD 卡文件写入和读取测试 |
| `demo_sdcard_speed [total_kb] [block_kb]` | SD 卡顺序读写测速 |
| `demo_fs_benchmark [file] [total_kb] [block_kb] [random_ops]` | 基于 DFS/POSIX 的文件系统综合测速 |
| `demo_flash_speed [total_kb] [block_kb]` | `/flash` 文件系统顺序读写测速 |
| `core_mark` | CoreMark CPU 性能基准测试 |

## 软件说明

* 工程基于 **Edgi-Talk** 平台开发。
* 本工程是运行在 Cortex-M55 核上的 RT-Thread 应用；M33 工程需要在板级初始化阶段开启并启动 M55 核。
* 各 demo 通过串口 MSH 命令手动触发。
* Audio demo 使用 RT-Thread Audio 框架、PDM 麦克风和 ES8388 Codec；启动后会占用 `mic0` 与 `sound0`。
* WavPlayer demo 使用 RT-Thread `wavplayer` 软件包播放 WAV 文件；播放时会占用 `sound0`，不能和 `demo_audio` 回环同时运行。
* SD 卡挂载由公共文件系统初始化逻辑处理，插卡后自动挂载到 `/sdcard`。
* 片外 Flash 通过 FAL 注册为 `norflash0`，其中 `filesystem` 分区会以 littlefs 挂载到 `/flash`。

## 使用方法

### 编译与下载

1. 确认 M33 启动工程已经开启 CM55 Core。
2. 打开本工程并完成编译。
3. 使用 **板载下载器 (DAP)** 将开发板 USB 接口连接到 PC。
4. 通过编程工具或 RT-Thread Studio 的 launch 配置将生成的固件烧录到开发板。

### 运行效果

* M33 启动流程拉起 M55 后，RT-Thread 在 M55 核上运行并进入 MSH。
* 在串口终端进入 `msh />` 后执行对应 demo 命令。
* 音频、SD 卡、Flash、CoreMark 等占用资源较多的 demo 建议单独运行。

```text
 \ | /
- RT -     Thread Operating System
 / | \     5.0.2 build Sep  5 2025 14:13:02
 2006 - 2022 Copyright by RT-Thread team
Hello RT-Thread
It's cortex-m55
msh >
```

## 常用 Demo

### AHT20 温湿度 Demo

```sh
demo_aht20
```

读取一次 AHT10/AHT20 温度和湿度，并通过串口打印结果。

### LSM6DS3 六轴传感器 Demo

```sh
demo_lsm6ds3
demo_lsm6ds3 10 200
```

`count` 为采样次数，`delay_ms` 为每次采样间隔。

### Audio 录放 Demo

```sh
demo_audio start
demo_audio start 60 30
demo_audio status
demo_audio stop
```

`speaker_volume` 为扬声器音量，`mic_gain` 为麦克风增益。该 demo 会占用 `mic0` 与 `sound0`。

### WavPlayer 音频播放 Demo

```sh
wavplay -s /sdcard/16000.wav
wavplay -t
wavplay -p
wavplay -r
wavplay -v 80
```

`wavplay` 和 `demo_audio` 都会占用 `sound0`。播放 WAV 前请先执行 `demo_audio stop`。

### ADC Demo

```sh
demo_adc
```

读取 ADC1 Channel 1 的电压值。

### HyperRAM Demo

```sh
demo_hyperram
demo_hyperram_speed
demo_hyperram_speed 1024 8
```

用于验证 HyperRAM 基础读写和顺序带宽。

### 按键中断 Demo

```sh
demo_key
```

注册 P8.3 按键下降沿中断，每次按键触发后翻转 P16.5 蓝色 LED。

### RTC Demo

```sh
demo_rtc
demo_rtc 2026 7 17 10 30 0
```

读取当前 RTC 时间，或设置时间后再读取。

### SD 卡 Demo

```sh
demo_sdcard
demo_sdcard_speed
demo_sdcard_speed 8192 64
demo_fs_benchmark
demo_fs_benchmark /sdcard/demo_fs_benchmark.bin 5120 64 64
```

SD 卡插入后会自动挂载到 `/sdcard`。

### Flash 文件系统 Demo

```sh
demo_flash_speed
demo_flash_speed 512 4
```

该命令会在 `/flash` 下创建临时测速文件，测试完成后自动删除。

### CoreMark 性能测试 Demo

```sh
core_mark
```

运行 CoreMark 前建议停止其他高负载 demo，避免 SD 卡、音频或传感器轮询影响结果。

## 注意事项

> 注意：本工程要求使用 **RT-Thread Studio 2.2.9** 或以上版本。

* 如需修改工程的图形化配置，请使用以下工具打开配置文件：

```text
tools/device-configurator/device-configurator.exe
libs/TARGET_APP_KIT_PSE84_EVAL_EPC2/config/design.modus
```

* 修改完成后保存配置，并重新生成代码。
* SD 卡建议格式化为 FAT16/FAT32。
* `/flash` 使用片外 Flash 的 `filesystem` 分区，首次挂载失败时会自动格式化；如果需要保留该分区数据，请不要随意执行整片擦除。
* `demo_flash_speed` 会触发 Flash 擦写，请避免在有重要数据的 `/flash` 分区上反复运行大容量测试。
* 若本工程无法正常运行，建议先编译并烧录 **Edgi_Talk_M33_Template** 工程，确保初始化与核心启动流程正常。

## 启动流程

系统启动顺序如下：

```text
+------------------+
|   Secure M33     |
|   (安全核心启动) |
+------------------+
          |
          v
+------------------+
|       M33        |
|   (非安全核启动) |
+------------------+
          |
          v
+-------------------+
|       M55         |
|  (应用处理器启动) |
+-------------------+
```

M55 应用依赖 M33 启动流程。若要开启 M55，需要在 **M33 工程** 中打开配置：

```text
RT-Thread Settings --> 硬件 --> select SOC Multi Core Mode --> Enable CM55 Core
```
