# Edgi-Talk_WIFI 示例工程

**中文** | [**English**](./README.md)

## 简介

本示例工程基于 **Edgi-Talk 平台**，演示 **WIFI功能**，运行在 **RT-Thread 实时操作系统 (M55 核)** 上。
通过本工程，用户可以快速体验 WIFI的联网功能，并验证WIFI模块的接口，为后续WIFI的开发提供参考。

## 硬件说明
### WIFI接口
![alt text](figures/1.png)
### BTB座子
![alt text](figures/2.png)
### MCU接口
![alt text](figures/3.png)
## 软件说明

* 工程基于 **Edgi-Talk** 平台开发。

* 示例功能包括：

  * WIFI的扫描
  * WIFI的连接
  * Iperf测速
  
* 工程结构清晰，便于理解WIFI驱动和 RT-Thread 系统的配合使用。

## 使用方法

### 编译与下载

1. 打开工程并完成编译。
2. 使用 **板载下载器 (DAP)** 将开发板的 USB 接口连接至 PC。
3. 通过编程工具将生成的固件烧录至开发板。

### 准备 WiFi 资源（首次必做）

WHD 在启动时需要从 FAL 读取三个外部资源文件：`firmware`（二进制固件）、`clm_blob`（射频法规表）以及 `nvram.txt`（模组校准参数）。这些文件独立于应用固件，重新烧录应用不会自动更新它们，因此首次使用或更换固件包时必须手动写入。Edgi-Talk 默认提供的资源文件位于工程根目录的 `resources/` 文件夹中。

- 在 menuconfig 中保持 `WHD_RESOURCES_IN_EXTERNAL_STORAGE_FAL` 选项开启，并在 FAL 分区表中保留默认的 `whd_firmware`、`whd_clm`、`whd_nvram` 分区（默认分别占用 512 KB + 32 KB + 32 KB 的片上 Flash）。
- 打开串口终端，复位进入 `msh` 命令行，依次执行以下命令：

```
whd_res_download whd_firmware
whd_res_download whd_clm
whd_res_download whd_nvram
```

命令会切换到 YMODEM 传输模式，请使用支持 YMODEM 上传的终端软件（如 Xshell）选择根目录 `resources/` 下对应芯片的文件并发送。

- 每次收到 `Download ... success` 提示后再进行下一项
- 三者写入完成后重启开发板即可让 WiFi 读取新的资源；若后续更新固件包，同样需要重新执行 `whd_res_download`。

![wifi](figures/wifi.gif)

### 运行效果

* 烧录完成后，开发板上电即可运行示例工程。
* 系统会自动初始化 WIFI设备。
* 用户可在 **串口终端**使用以下命令连接WIFI：

```
wifi scan
```
![alt text](figures/5.png)
```
wifi join 名称 密码
```
![alt text](figures/6.png)
```
ping www.rt-thread.org
```
![alt text](figures/7.png)

* 网络连接完成后，可使用 iperf 进行性能测试。
* 在 packages\netutils-latest\tools 目录下提供了 jperf.rar 测速工具。
* 将其解压后，双击其中的 .bat 文件，即可启动工具，界面如下图所示：

![工具演示](figures/4.png)

* 在开发板终端输入以下命令（其中 电脑的 IP 请替换为实际地址），即可开始测速：

```
iperf -c <电脑的IP>
```

### 注意事项

* 可以使用电脑开热点进行测试，频段最好为2.4G。

## 启动流程

系统启动顺序如下：

```
+------------------+
|   Secure M33     |
|   (安全内核启动) |
+------------------+
          |
          v
+------------------+
|       M33        |
|   (非安全核启动) |
+------------------+
          |
          v
+-------------------+
|       M55         |
|  (应用处理器启动) |
+-------------------+
```

⚠️ 请严格按照以上顺序烧写固件，否则系统可能无法正常运行。

---

* 若示例工程无法正常运行，建议先编译并烧录 **Edgi_Talk_M33_Blink_LED** 工程和 **Edgi-Talk\_M33\_Template** 工程，确保初始化与核心启动流程正常，再运行本示例。
* 若要开启 M55，还需要在 **Edgi-Talk\_M33\_Template工程** 中打开配置：

```
RT-Thread Settings --> 硬件 --> select SOC Multi Core Mode --> Enable CM55 Core
```
![config](figures/config.png)
