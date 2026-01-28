# Edgi_Talk_M33_USB_H CherryUSB 示例工程

**中文** | [**English**](./README.md)

## 简介

本工程在 **Edgi-Talk M33 核心**上集成 **CherryUSB**，默认配置为 **USB 主机模式**，并使用 **Infineon DWC2** IP。

## 默认配置

* `RT_USING_CHERRYUSB = y`
* `RT_CHERRYUSB_HOST = y`
* `RT_CHERRYUSB_HOST_SPEED_HS = y`
* `RT_CHERRYUSB_HOST_DWC2_INFINEON = y`
* 主机类驱动：**MSC, CDC, HID**（可配置）

## 编译与下载

1. 使用 RT-Thread Studio 或 SCons 编译工程。
2. 通过 KitProg3 (DAP) 下载固件。
3. 将 USB 设备连接至 Type-C USB 接口进行主机操作。

## 配置方法（切换模式）

在 RT-Thread Studio 中打开：

```
RT-Thread Settings -> USB -> CherryUSB
```

* **主机模式**：开启 `RT_CHERRYUSB_HOST`，在 **CHERRYUSB_HOST_IP** 中选择 IP（默认 `RT_CHERRYUSB_HOST_DWC2_INFINEON`）。
* **主机类驱动**：根据需要开启类驱动（MSC 用于 U 盘，HID 用于键鼠，CDC 用于串口设备等）。
* **设备模式**：关闭主机模式，开启 `RT_CHERRYUSB_DEVICE`，并选择设备模板。

若 IP 或类驱动需要额外参数，请修改：

* `libraries/Common/board/ports/usb/usb_config.h`

## 主机插入 U 盘效果

![usb host u-disk](figures/usb_host_udisk.png)

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

* 本工程面向 M33 核心的 USB 主机模式。
* 设备模式请参考 [projects/Edgi_Talk_CherryUSB/Edgi_Talk_M33_USB_D/README.md](../Edgi_Talk_M33_USB_D/README.md)。
* M55 主机模式请参考 [projects/Edgi_Talk_CherryUSB/Edgi_Talk_M55_USB_H/README.md](../Edgi_Talk_M55_USB_H/README.md)。

