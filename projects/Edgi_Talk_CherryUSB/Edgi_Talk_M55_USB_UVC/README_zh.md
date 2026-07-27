# Edgi_Talk_M55_USB_UVC CherryUSB 示例工程

**中文** | [**English**](./README.md)

## 简介

本工程运行在 **Edgi-Talk M55 核心**，基于 **CherryUSB Host + Infineon DWC2**，并默认开启 **USB Video Class (UVC) 主机类驱动**。

工程上电后会自动调用 `usbh_initialize(0, USBHS_BASE, NULL)` 初始化 USB Host，主循环中闪烁绿色 LED（P16_6）作为运行指示。

## 默认配置

当前工程关键宏（见 `rtconfig.h`）：

* `RT_USING_CHERRYUSB = y`
* `RT_CHERRYUSB_HOST = y`
* `RT_CHERRYUSB_HOST_DWC2_INFINEON = y`
* `RT_CHERRYUSB_HOST_MSC = y`
* `RT_CHERRYUSB_HOST_VIDEO = y`

说明：该工程已具备 UVC 主机枚举所需的类驱动配置，便于后续对接 UVC 设备应用逻辑。

## 编译与下载

1. 使用 RT-Thread Studio 或 SCons 编译工程。
2. 通过 KitProg3 (DAP) 下载固件。
3. 将 UVC 摄像头或其他 USB 设备连接到 Type-C 口进行测试。

## 配置方法

在 RT-Thread Studio 中打开：

```
RT-Thread Settings -> USB -> CherryUSB
```

建议确认：

* 开启 `RT_CHERRYUSB_HOST`
* 在 Host IP 中选择 `RT_CHERRYUSB_HOST_DWC2_INFINEON`
* 开启 `RT_CHERRYUSB_HOST_VIDEO`（UVC）
* 按需保留/关闭 `MSC` 等其它 Host 类驱动

若需调整 USB 参数，可修改：

* `libraries/Common/board/ports/usb/usb_config.h`

## 运行效果

1. 插入USB摄像头到开发板USB-OTG接口

 <img src="figures/USB-Connect.png" style="zoom:50%;" />

2. 系统启动会自动识别摄像头（分辨率，格式等信息）

 <img src="figures/usb-uvc.png" style="zoom:50%;" />

3. 在MSH终端，输入`usbh_uvc_start 0 320 240`（0 = YUYV, 1 = MJPEG，320x240 分辨率）

**需要注意：需要根据实际的USB摄像头支持的情况来选择，分辨率最大仅支持432x240**

 <img src="figures/usb-lcd.png" style="zoom:80%;" />

4. 可以查看串口LOG信息，LOG会1S打印一次帧率信息

    ![](figures/usb-log.png)

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

* 若 M55 工程无法正常运行，建议先编译并烧录 `Edgi_Talk_M33_Template`。
* 在 M33 工程中开启 CM55：

  ```
  RT-Thread Settings -> 硬件 -> select SOC Multi Core Mode -> Enable CM55 Core
  ```

![config](figures/config.png)
