# Edgi_Talk_M55_USB_D CherryUSB 示例工程

**中文** | [**English**](./README.md)

## 简介

本工程在 **Edgi-Talk M55 核心**上集成 **CherryUSB**，默认配置为 **USB 设备模式**，并使用 **Infineon DWC2** IP。

## 默认配置

* `RT_USING_CHERRYUSB = y`
* `RT_CHERRYUSB_DEVICE = y`
* `RT_CHERRYUSB_DEVICE_SPEED_HS = y`
* `RT_CHERRYUSB_DEVICE_DWC2_INFINEON = y`
* 设备模板：**none**（由用户自行实现）

## 编译与下载

1. 使用 RT-Thread Studio 或 SCons 编译工程。
2. 通过 KitProg3 (DAP) 下载固件。
3. 使用 Type-C 接口连接 USB 进行枚举。

## 配置方法（切换模式）

在 RT-Thread Studio 中打开：

```
RT-Thread Settings -> USB -> CherryUSB
```

* **设备模式**：开启 `RT_CHERRYUSB_DEVICE`，在 **CHERRYUSB_DEVICE_IP** 中选择 IP（默认 `RT_CHERRYUSB_DEVICE_DWC2_INFINEON`）。
* **主机模式**：关闭设备模式，开启 `RT_CHERRYUSB_HOST`，选择主机 IP 并勾选所需类驱动（MSC/HID/CDC 等）。
* **设备类模板**：开启对应类驱动后，在 **Select usb device template** 中选择模板。

若 IP 或类驱动需要额外参数，请修改：

* `libraries/Common/board/ports/usb/usb_config.h`

## 启动流程

M55 依赖 M33 启动流程，烧录顺序如下：

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

* 本工程面向 M55 核心的 USB 设备模式。
* 主机模式请参考 [projects/Edgi_Talk_CherryUSB/Edgi_Talk_M55_USB_H/README.md](../Edgi_Talk_M55_USB_H/README.md)。
* M33 设备模式请参考 [projects/Edgi_Talk_CherryUSB/Edgi_Talk_M33_USB_D/README.md](../Edgi_Talk_M33_USB_D/README.md)。
* 若 M55 工程无法正常运行，建议先编译并烧录 **Edgi_Talk_M33_Blink_LED** 工程。
* 在 **M33 工程** 中开启 CM55：

  ```
  RT-Thread Settings -> 硬件 -> select SOC Multi Core Mode -> Enable CM55 Core
  ```
![config](figures/config.png)
