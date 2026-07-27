# Edgi_Talk_M33_USB_RoleSwitch CherryUSB 示例工程

**中文** | [**English**](./README.md)

## 简介

本工程在 **Edgi-Talk M33 核心** 上集成 **CherryUSB**，用于 **USB 设备模式**，并使用 Infineon **DWC2** IP。

## 默认配置

* `RT_USING_CHERRYUSB = y`
* `RT_CHERRYUSB_DEVICE = y`
* `RT_CHERRYUSB_DEVICE_SPEED_HS = y`
* `RT_CHERRYUSB_DEVICE_DWC2_INFINEON = y`
* 设备模板：**CDC**（用户应用）

## 编译与下载

1. 使用 RT-Thread Studio 或 SCons 编译工程。
2. 通过 KitProg3 (DAP) 下载固件。
3. 使用 Type-C USB 接口连接主机，进行 USB 设备枚举。
4. 可以使用 CherryUSB 测试脚本对 CDC 设备进行性能测试。

## 配置方法（切换模式）

通过板级 Kconfig 入口切换 USB 角色。USB Host/Device 的选择不需要重新生成图形化板级配置。

```
RT-Thread Settings ->
Hardware Drivers Config ->
Onboard Peripheral Drivers ->
Enable USB ->
USB role
```

* **Device CDC ACM**：选择 `RT_CHERRYUSB_DEVICE`、高速模式、Infineon DWC2、CDC ACM 以及 CDC ACM 设备模板。
* **Host MSC**：选择 `RT_CHERRYUSB_HOST`、Infineon DWC2、MSC、DFS 以及 ElmFat。

如果 IP 或类驱动需要额外参数，请修改：

* `libraries/Common/board/ports/usb/usb_config.h`

## USB CDC 设备测试效果

![config](figures/test.png)

## 主机插入 U 盘效果

![usb host u-disk](figures/usb_host_udisk.png)

## 启动流程

M55 依赖 M33 启动流程，烧录顺序如下：

```
+------------------+
|   Secure M33     |
|    (安全核心)    |
+------------------+
          |
          v
+------------------+
|       M33        |
|   (非安全核心)   |
+------------------+
          |
          v
+-------------------+
|       M55         |
|   (应用处理器)    |
+-------------------+
```

## 说明

> 注意：本工程要求使用 **RT-Thread Studio 2.2.9** 或以上版本。

* M33 工程的串口日志不会通过板载 DAP 虚拟串口直接输出。查看 `msh />` 和 demo 日志时，需要额外连接 USB 转串口硬件，例如 CH340。
位置如图下所示，RX 接串口硬件的 TX，TX 接串口硬件的 RX，上位机波特率 115200：

![alt text](figures/m33_uart.png)

* 本工程面向 M33 核心，同时支持 USB 设备 CDC ACM 和 USB 主机 MSC 两种角色。
* M55 USB 角色切换示例请参考 [projects/Edgi_Talk_CherryUSB/Edgi_Talk_M55_USB_RoleSwitch/README.md](../Edgi_Talk_M55_USB_RoleSwitch/README.md)。
* 如需运行 M55 工程，建议先烧写 **Edgi_Talk_M33_Template**。该工程是干净的 M33 工程，适合作为 M55 启动前的基础固件。
