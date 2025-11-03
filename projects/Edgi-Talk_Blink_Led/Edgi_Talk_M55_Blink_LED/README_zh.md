# Edgi-Talk_M55_Blink_LED 示例工程

**中文** | [**English**](./README.md)

## 简介

本示例工程基于 **Edgi-Talk 平台**，演示 **绿色 LED 灯闪烁** 功能，运行在 **RT-Thread 实时操作系统** 上。
通过本工程，用户可以快速验证板级 GPIO 配置及 LED 控制逻辑，为后续硬件控制和应用开发提供基础参考。

## GPIO 简介

**GPIO (General Purpose Input/Output)** 是 MCU 最常用的外设接口之一，能够在软件控制下配置为 **输入模式** 或 **输出模式**：

- **输入模式**：用于读取外部电平状态，例如按键输入。
- **输出模式**：用于控制外设电平，例如点亮 LED、驱动蜂鸣器。
### RT-Thread 对 GPIO 的抽象

RT-Thread 提供了 **PIN 设备驱动框架**，通过统一的接口屏蔽底层硬件差异：

- `rt_pin_mode(pin, mode)` ：设置引脚工作模式（输入/输出/上拉/下拉等）
- `rt_pin_write(pin, value)`：输出电平（高/低）
- `rt_pin_read(pin)`：读取输入电平

这样开发者不需要直接操作寄存器，而是通过 RT-Thread 的 API 即可完成 GPIO 控制。

在本示例中，LED 引脚被配置为 **输出模式**，软件循环输出高低电平，从而实现 LED 闪烁。

## 硬件说明

![1](figures/1.png)
![2](figures/2.png)
![3](figures/3.png)

如上图所示，Edgi-Talk 提供三个用户LED，分别为USER_LED1（RED）、USER_LED2（GREEN）、USER_LED3（BLUE），其中 USER_LED2 对应引脚P16_7。单片机引脚输出高电平即可点亮LED ，输出低电平则会熄灭LED。

LED在开发板中的位置如下图所示： 

![4](figures/4.png)

## 软件说明

* 工程基于 **Edgi-Talk** 平台开发。
* 示例功能包括：

  * 绿色 LED 灯周期性闪烁
  * GPIO 输出控制
* 工程结构简洁，便于理解 LED 控制逻辑及硬件驱动接口。

## 使用方法

### 编译与下载

1. 打开工程并完成编译。
2. 使用 **板载下载器 (DAP)** 将开发板的 USB 接口连接至 PC。
3. 通过编程工具将生成的固件烧录至开发板。

### 运行效果

* 烧录完成后，开发板上电即可运行示例工程。
* **绿色 LED 灯每 500ms 闪烁一次**，表示系统 GPIO 控制和调度正常。
* 用户可根据需求修改闪烁周期或 LED 控制逻辑。

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
![config](figures/config.png)
