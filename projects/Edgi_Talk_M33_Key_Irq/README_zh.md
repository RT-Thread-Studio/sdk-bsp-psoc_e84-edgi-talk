# Edgi-Talk_Key_Irq 示例工程

**中文** | [**English**](./README.md)

## 简介

本示例工程基于 **Edgi-Talk 平台**，运行于 **RT-Thread 实时操作系统**，演示 **按键中断 (Key IRQ)** 的使用方法。
通过本工程，用户可以学习如何在 RT-Thread 中配置 GPIO 中断，并实现按键触发事件的响应逻辑，为后续人机交互类应用提供参考。
## MCU 中断体系概述

具有如下中断特性：

1. **NVIC（嵌套向量中断控制器）**
   - 支持 **中断嵌套**：高优先级中断可打断低优先级中断
   - 支持 **优先级分组**（Preemption Priority 与 Subpriority）
   - IRQn 映射到中断向量表，由 NVIC 调用对应 ISR
2. **GPIO 外部中断 (EXTI)**
   - 每个 GPIO 引脚可配置为外部中断输入
   - 支持 **上升沿、下降沿或双沿触发**
   - 中断通道由 **ICU (Interrupt Controller Unit)** 管理
   - 可以通过 Renesas FSP 或底层寄存器配置
   - 中断响应延迟低，适合按键、传感器等事件驱动
3. **RT-Thread PIN 驱动模型**
   - GPIO 在 RT-Thread 中被封装为 **PIN 设备**
   - 提供统一接口：
     - `rt_pin_mode(pin, mode)`：配置输入/输出模式
     - `rt_pin_read(pin)` / `rt_pin_write(pin, value)`：读写 GPIO
     - `rt_pin_attach_irq(pin, mode, callback, args)`：注册中断回调
     - `rt_pin_irq_enable(pin, enable)`：使能/禁用中断

> 使用 RT-Thread PIN 驱动，无需直接操作 NVIC 或 MCU 寄存器即可实现中断响应。
## 硬件说明

### 按钮接口
![alt text](figures/1.png)
### BTB座子
![alt text](figures/2.png)
### MCU接口
![alt text](figures/3.png)
### 实物图位置
![alt text](figures/4.png)

## 软件说明

* 工程基于 **Edgi-Talk** 平台开发。
* 使用 **RT-Thread** 作为操作系统内核。
* 示例功能包括：

  * 配置按键 GPIO 为中断输入模式
  * 按键按下时触发中断回调函数
  * 蓝色 LED 以 500ms 为周期闪烁，表示系统正常运行

## 使用方法

### 编译与下载

1. 打开工程并完成编译。
2. 使用 **板载下载器 (DAP)** 将开发板 USB 接口连接至 PC。
3. 通过编程工具将生成的固件烧录至开发板。

### 运行效果

* 烧录完成后，开发板上电即可运行示例工程。
* **蓝色 LED** 将以 500ms 为周期闪烁，表示系统调度正常。
* 当用户按下按键时，会触发中断回调，并在串口终端输出：

  ```
  The button is pressed
  ```

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

* 若示例工程无法正常运行，建议先编译并烧录 **Edgi_Talk_M33_Blink_LED** 工程，确保初始化与核心启动流程正常，再运行本示例。
* 若要开启 M55，需要在 **M33 工程** 中打开配置：

  ```
  RT-Thread Settings --> 硬件 --> select SOC Multi Core Mode --> Enable CM55 Core
  ```

