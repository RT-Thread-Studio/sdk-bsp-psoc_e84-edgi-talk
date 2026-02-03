# Edgi_Talk_M55_USB_D CherryUSB USB 拓展屏示例工程

**中文** | [**English**](./README.md)

## 简介

本工程在 **Edgi-Talk M55 核心**上集成 **CherryUSB**，默认配置为 **USB 设备模式**，并使用 **Infineon DWC2** IP，实现 **Windows 下 USB 拓展屏幕**。

## 默认配置

* `RT_USING_CHERRYUSB = y`
* `RT_CHERRYUSB_DEVICE = y`
* `RT_CHERRYUSB_DEVICE_SPEED_HS = y`
* `RT_CHERRYUSB_DEVICE_DWC2_INFINEON = y`
* 设备模板：**none**（由用户自行实现）
* 显示协议：Vendor 接口传输 RGB565 帧

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
* **设备类模板**：开启对应类驱动后，在 **Select usb device template** 中选择模板。

若 IP 或类驱动需要额外参数，请修改：

* `libraries/Common/board/ports/usb/usb_config.h`

## Windows IDD 驱动程序

Windows 间接显示驱动程序 (IDD) 模型提供简单的用户模式驱动程序模型，以支持未连接到传统 GPU 显示输出的监视器。

参考：https://learn.microsoft.com/zh-cn/windows-hardware/drivers/display/indirect-display-driver-model-overview

本驱动参考自：https://github.com/chuanjinpang/win10_idd_xfz1986_usb_graphic_driver_display

如需修改，请自行下载后重新编译。

### 如何使用

1. 安装 `resources\USB_Graphic\xfz1986_usb_graphic_250224_rc_sign.exe` 签名驱动
2. 安装完毕后，在显示适配器下面出现一个新的显示器，即安装成功

  ![new_screen](figures/new_screen.png)

### 注意事项

* 该驱动通过 VENDOR 接口与设备进行通信，支持多种分辨率和图片格式，通过接口字符描述符来控制，具体参考：https://github.com/chuanjinpang/win10_idd_xfz1986_usb_graphic_driver_display/blob/main/README.md
* 驱动仅支持 Windows 10 及 Windows 11 系统，其他系统请自行测试。

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
