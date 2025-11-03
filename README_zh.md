# sdk-bsp-psoc_e84-edgi-talk

**中文** | [**English**](./README.md)

## 简介

中文页

`sdk-bsp-psoc_e84-edgi-talk` 是针对 **PSoC™ E84 Edgi-Talk 开发板** 的 RT-Thread 支持包，同时也可作为用户开发使用的软件 SDK，让用户可以更方便地开发自己的应用程序。

Edgi-Talk 开发板基于 **PSoC™ E84 MCU**，为工程师提供了一个灵活、全面的开发平台。板上集成了丰富的外设接口和示例模块，助力开发者快速完成多传感器、显示和通信应用的开发。

![Edgi-Talk](docs/figures/board_large.jpg)

## 目录结构

```
$ sdk-bsp-psoc_e84-edgi-talk
├── README.md
├── sdk-bsp-psoc_e84-edgi-talk.yaml
├── docs
│   ├── Edgi-Talk_Board_Schematic.pdf
│   └── Edgi-Talk_User_Manual.pdf
├── libraries
│   ├── HAL_Drivers
├── projects
│   ├── Edgi-Talk_ADC
│   ├── Edgi-Talk_AHT20
│   ├── Edgi-Talk_Audio
│   ├── Edgi-Talk_Blink_Led
│   ├── Edgi-Talk_CoreMark
│   ├── Edgi-Talk_CDC_Echo
│   ├── Edgi-Talk_HyperRam
│   ├── Edgi-Talk_Key_Irq
│   ├── Edgi-Talk_LSM6DS3
│   ├── Edgi-Talk_LVGL
│   ├── Edgi-Talk_M33_S_Template
│   ├── Edgi-Talk_M33_Template
│   ├── Edgi-Talk_MIPI_LCD
│   ├── Edgi-Talk_RTC
│   ├── Edgi-Talk_SDCARD
│   ├── Edgi-Talk_WavPlayer
│   ├── Edgi-Talk_WIFI
│   ├── Edgi-Talk_XiaoZhi
└── rt-thread
```

* `sdk-bsp-psoc_e84-edgi-talk.yaml`：描述 Edgi-Talk 开发板的硬件信息
* `docs`：开发板原理图、用户手册及 datasheet
* `libraries`：Edgi-Talk 通用外设驱动
* `projects`：示例工程文件夹，包含传感器、显示、音频、USB、网络等示例程序
* `rt-thread`：RT-Thread 源码

## 使用方式

`sdk-bsp-psoc_e84-edgi-talk` 支持 **RT-Thread Studio**开发方式。

## **RT-Thread Studio 开发步骤**

1. 打开 RT-Thread Studio，安装 Edgi-Talk 开发板支持包（建议安装最新版本）。
   ![开发板](docs/figures/1.png)
2. 新建 Edgi-Talk 工程：
   文件 -> 新建 -> RT-Thread 项目 -> 基于开发板，可选择示例工程或模板工程。
   ![项目](docs/figures/2.png)
3. 编译和下载工程：
   直接在 RT-Thread Studio 内完成编译、下载和调试。
   ![编译](docs/figures/3.png)

## 注意事项

* 如需修改工程的 **图形化配置**，请使用以下工具打开配置文件：

  ```
  tools/device-configurator/device-configurator.exe
  libs/TARGET_APP_KIT_PSE84_EVAL_EPC2/config/design.modus
  ```
* 修改完成后保存配置，并重新生成代码。

## 启动流程

系统启动顺序如下：

```
+------------------+
|   Secure M33     |
|   (安全内核启动)  |
+------------------+
          |
          v
+------------------+
|       M33        |
|   (非安全核启动)  |
+------------------+
          |
          v
+-------------------+
|       M55         |
|  (应用处理器启动)  |
+-------------------+
```

⚠️ 请严格按照以上顺序烧写固件，否则系统可能无法正常运行。

---

* 若示例工程无法正常运行，建议先编译并烧录 **Edgi-Talk\_M33\_S\_Template** 工程，确保初始化与核心启动流程正常，再运行本示例。
* 若要开启 M55，需要在 **M33 工程** 中打开配置：

  ```
  RT-Thread Settings --> 硬件 --> select SOC Multi Core Mode --> Enable CM55 Core
  ```
![开启M55](docs/figures/config.png)


