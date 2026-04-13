# Edgi_Talk_USB_Deepcraft 示例工程

**中文** | [**English**](./README.md)

## 简介

本工程运行在 **Edgi-Talk M55 核心**，通过 **CherryUSB Device CDC ACM** 将板端传感器数据以 **Protocol Buffers 协议**对接到上位机 Deepcraft/Imagimob 流式工具链。

程序启动后会：

1. 创建协议上下文（`protocol_create`）并生成 16 字节设备序列号。
2. 注册设备驱动（当前包含麦克风与 LSM6DS3 IMU）。
3. 初始化 USB CDC 设备并等待主机连接。
4. 在主循环中执行 `protocol_process_request`，处理主机请求与数据流。

## 默认配置

当前工程关键宏（见 `rtconfig.h`）：

* `RT_USING_CHERRYUSB = y`
* `RT_CHERRYUSB_DEVICE = y`
* `RT_CHERRYUSB_DEVICE_DWC2_INFINEON = y`
* `RT_CHERRYUSB_DEVICE_CDC_ACM = y`
* `RT_CHERRYUSB_DEVICE_TEMPLATE_CDC_ACM = y`

USB 标识（见 `applications/deepcraft/deepcraft_usbd.c`）：

* VID: `0x058B`
* PID: `0x027D`
* Product String: `Imagimob Streaming Device`

## 已对接设备能力

当前 `system_load_device_drivers()` 中加载：

* **Microphone（PDM/PCM）**
  * 选项：增益、单/双声道、采样率
  * 流类型：`S16` 音频输出流
* **LSM6DS3 六轴传感器**
  * 选项：采样率、加速度量程、陀螺仪量程、输出模式
  * 支持 Combined / Split / Only Accel / Only Gyro

## 编译与下载

1. 使用 RT-Thread Studio 或 SCons 编译工程。
2. 通过 KitProg3 (DAP) 下载固件。
3. 使用 Type-C 将板卡连接到主机，等待 CDC 设备枚举。

启动成功后串口会打印：

* `deepcraft: ready, waiting for host commands.`

 ![](figures/log1.png)



## 使用说明

1. 下载并安装deepcraftstudio工具，详情参考教程：[Install Download Studio](https://developer.imagimob.com/deepcraft-studio/install-download-studio)
2. 创建空白项目，详情参考教程：[Real-Time Data Streaming with PSOC™ Edge E84 HMI](https://developer.imagimob.com/deepcraft-studio/data-preparation/data-collection/collect-data-infineon-boards/collect-data-hmi-kit)
3. 开发板和电脑使用USB数据线连接后，在deepcraftstudio中可以查看到设备

![](figures/log2.png)

4. 点击编译运行选项，进入采集窗口界面

 ![](figures/log3.png)

5. 点击录制选项后，可以看到麦克风和3轴数据实时的被显示出来了

 ![](figures/log4.png)

> 更多教程文档请参考：[Data Collection Microphone](https://developer.imagimob.com/deepcraft-studio/data-preparation/data-collection/collect-data-infineon-boards/collect-data-edge-evaluation-kit/data-collection-microphone)

## 主机通信说明

* 传输通道：USB CDC ACM
* 编码格式：nanopb / protobuf
* 主循环错误处理：
  * 当解析请求失败（`PROTOCOL_STATUS_FAILED_TO_DECODE_REQUEST`）时，会调用 `deepcraft_usbd_resync_rx()` 清空接收环形缓冲并重同步。

## 启动流程

M55 依赖 M33 启动流程，建议烧录顺序如下：

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

## 说明

> **注意：** 推荐使用 **RT-Thread Studio 2.2.9** 或以上版本。

* 若 M55 工程无法正常运行，建议先编译并烧录 `Edgi_Talk_M33_Blink_LED`。
* 在 M33 工程中开启 CM55：

  ```
  RT-Thread Settings -> 硬件 -> select SOC Multi Core Mode -> Enable CM55 Core
  ```

![config](figures/config.png)
