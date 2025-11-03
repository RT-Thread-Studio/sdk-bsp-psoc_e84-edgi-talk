# Edgi-Talk_M55_CoreMark 示例工程

**中文** | [**English**](./README.md)

## 简介

本示例工程基于 **Edgi-Talk 平台**，演示 **M55 核心运行 CoreMark 基准测试** 的功能，运行在 **RT-Thread 实时操作系统** 上。
通过本工程，用户可以快速验证 M55 核心性能，并了解多核协处理器在实时操作系统下的运行情况。

### CoreMark 简介

**CoreMark** 是由 *EEMBC（嵌入式微控制器基准评测联盟）* 开发的一个标准化嵌入式 CPU 性能测试基准。
它主要用于衡量微控制器或处理器的 **核心运算性能**，而不依赖特定的硬件外设。

### 测试内容

CoreMark 通过以下四类典型算法评估 CPU 性能：

* **列表处理（List processing）**
* **矩阵运算（Matrix operations）**
* **状态机（State machine）**
* **CRC 校验（Cyclic Redundancy Check）**

### 测试结果

输出结果以 **CoreMark/MHz** 或 **CoreMark** 表示，用于比较不同处理器或编译优化下的性能。

### 特点

* 开源、可移植、轻量级
* 结果可重复、易于验证
* 专注于 CPU 的整数计算能力

## 软件说明

* 工程基于 **Edgi-Talk** 平台开发。
* 示例功能包括：

  * M55 核心运行 CoreMark 基准测试
  * 串口打印测试结果
* 工程结构简洁，便于用户理解 M55 的启动流程及性能测试方法。

## 使用方法

### 编译与下载

1. 打开工程并完成编译。
2. 使用 **板载下载器 (DAP)** 将开发板的 USB 接口连接至 PC。
3. 通过编程工具将生成的固件烧录至开发板。

### 运行效果

* 烧录完成后，开发板上电即可启动 RT-Thread。
* 串口输出如下内容表示系统正常启动：

```
 \ | /
- RT -     Thread Operating System
 / | \     5.0.2 build Sep  5 2025 15:19:27
 2006 - 2022 Copyright by RT-Thread team
msh >Hello RT-Thread
It's cortex-m55
```

* 用户需在串口命令行手动输入：

```
core_mark
```

* 系统开始 CoreMark 测试，并打印测试结果，例如：

```
Benchmark started, please make sure it runs for at least 10s.

2K performance run parameters for coremark.
CoreMark Size    : 666
Total ticks      : 30218
Total time (secs): 30
Iterations/Sec   : 1200
Iterations       : 36000
Compiler version : GCC10.2.1 20201103 (release)
Compiler flags   :
Memory location  : STACK
seedcrc          : 0xe9f5
[0]crclist       : 0xe714
[0]crcmatrix     : 0x1fd7
[0]crcstate      : 0x8e3a
[0]crcfinal      : 0xcc42
Correct operation validated. See README.md for run and reporting rules.
CoreMark 1.0 : 1200 / GCC10.2.1 20201103 (release)  / STACK
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

* 若示例工程无法正常运行，建议先编译并烧录 **Edgi-Talk\_M33\_S\_Template** 工程，确保初始化与核心启动流程正常，再运行本示例。
* 若要开启 M55，需要在 **M33 工程** 中打开配置：

  ```
  RT-Thread Settings --> 硬件 --> select SOC Multi Core Mode --> Enable CM55 Core
  ```
![config](figures/config.png)