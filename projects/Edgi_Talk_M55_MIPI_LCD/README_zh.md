# Edgi-Talk_M55_MIPI_LCD 示例工程

**中文** | [**English**](./README.md)

## 简介

本示例工程基于 **Edgi-Talk 平台**，演示 **MIPI LCD 屏幕刷屏功能**，运行在 **RT-Thread 实时操作系统** 上。
通过本工程，用户可以快速验证 **MIPI DSI 接口驱动** 和 **LCD 显示初始化**，为后续 GUI 应用或 LVGL 移植提供参考。

## 硬件说明
### 背光接口
![alt text](figures/1.png)
### MIPI接口
![alt text](figures/2.png)
### PWR接口
![alt text](figures/3.png)
### BTB座子
![alt text](figures/4.png)
![alt text](figures/5.png)
### MCU接口
![alt text](figures/6.png)
![alt text](figures/7.png)

## 软件说明

* 工程基于 **Edgi-Talk** 平台开发，运行在 **M55 应用核** 上。
* 示例功能包括：

  * 初始化 MIPI DSI 接口与 LCD 面板
  * 在 LCD 上进行单色填充
  * 循环刷屏以验证 LCD 显示性能
* 工程结构简洁，便于理解 **MIPI DSI 初始化流程** 和 **LCD 刷屏逻辑**。

## 使用方法

### 编译与下载

1. 打开工程并完成编译。
2. 使用 **板载下载器 (DAP)** 将开发板的 USB 接口连接至 PC。
3. 通过编程工具将生成的固件烧录至开发板。

### 运行效果

* 烧录完成后，开发板上电即可运行示例工程。
* LCD 屏幕会依次 **刷单色**，每种颜色保持。
* 刷屏过程持续循环，用户可根据需要修改刷新周期或增加更多测试图案。

## 注意事项

* 如需修改工程的 **图形化配置**，请使用以下工具打开配置文件：

```
tools/device-configurator/device-configurator.exe
libs/TARGET_APP_KIT_PSE84_EVAL_EPC2/config/design.modus
```

* 修改完成后保存配置，并重新生成代码。
* 若屏幕无显示，请检查：

  * MIPI DSI 硬件连接是否正常
  * LCD 电源、背光供电是否开启

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

* 若示例工程无法正常运行，建议先编译并烧录 **Edgi-Talk_M33_S_Template** 工程，确保初始化与核心启动流程正常，再运行本示例。
* 若要开启 M55，需要在 **M33 工程** 中打开配置：

```
RT-Thread Settings --> 硬件 --> select SOC Multi Core Mode --> Enable CM55 Core
```
![config](figures/config.png)