# Edgi_Talk_M33_Template 示例工程

**中文** | [**English**](./README.md)

## 简介

本模板工程基于 **Edgi-Talk 平台**，在 Cortex-M33 核上运行最小 **RT-Thread** 应用。
它用于完成板级初始化、启动 Cortex-M55 核，然后保持空闲；所有可选外设 demo 默认关闭。

共享的 Secure M33 固件包位于：`libraries/components/infineon-pse84-secure-firmware-latest`，同时保留 Template 自己的最小 `.config`。

## 软件说明

* 工程基于 **Edgi-Talk** 平台开发。
* 已启用 `SOC_Enable_CM55`，板级初始化阶段会启动 M55 核。
* AHT20、LSM6DS3、Audio、ADC、RTC、SD 卡、文件系统、LCD、Wi-Fi 等可选外设 demo 均保持关闭。
* 板级初始化会将外部 Wi-Fi/音频电源控制脚拉低，避免 Template 默认打开外设电源。
## 使用方法

### 编译与下载

1. 打开工程并完成编译。
2. 使用 **板载下载器 (DAP)** 将开发板的 USB 接口连接至 PC。
3. 通过编程工具将生成的固件烧录至开发板。

### 运行效果

* 烧录完成后，开发板上电即可运行示例工程。
* M33 初始化 RT-Thread，启动 M55，打印简短启动信息，然后保持空闲。
* Template 不会自动闪灯，也不会自动启动外设 demo。

## 注意事项

> **⚠️ 注意：** 本工程要求使用 **RT-Thread Studio 2.2.9** 或以上版本。

* M33 工程的串口日志不会通过板载 DAP 虚拟串口直接输出。查看 `msh />` 和 demo 日志时，需要额外连接 USB 转串口硬件，例如 CH340。
位置如图下所示，RX 接串口硬件的 TX，TX 接串口硬件的 RX，上位机波特率 115200：

![alt text](figures/m33_uart.png)

* 如需修改工程的 **图形化配置**，请使用以下工具打开配置文件：

```
tools/device-configurator/device-configurator.exe
libs/TARGET_APP_KIT_PSE84_EVAL_EPC2/config/design.modus
```

* 修改完成后保存配置，并重新生成代码。
* 如需运行 M55 工程，建议先烧写 **Edgi_Talk_M33_Template**。该工程是干净的 M33 工程，适合作为 M55 启动前的基础固件。

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

* 若示例工程无法正常运行，建议先编译并烧录 **Edgi_Talk_M33_Template** 工程，确保初始化与核心启动流程正常，再运行本示例。
* 若要开启 M55，需要在 **M33 工程** 中打开配置：

  ```
  RT-Thread Settings --> 硬件 --> select SOC Multi Core Mode --> Enable CM55 Core
  ```

