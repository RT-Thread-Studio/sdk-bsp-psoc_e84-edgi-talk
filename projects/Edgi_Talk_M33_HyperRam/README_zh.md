# Edgi-Talk_M33_HyperRam 示例工程

**中文** | [**English**](./README.md)

## 简介

本示例工程基于 **Edgi-Talk 平台**，演示 **HyperRam** 功能，运行在 **RT-Thread 实时操作系统** 上。
通过本工程，用户可以快速验证HyperRam，为后续硬件控制和应用开发提供基础参考。

## 软件说明

* 工程基于 **Edgi-Talk** 平台开发。
* 示例功能包括：

* 工程结构简洁，便于理解 HyperRam 控制逻辑及硬件驱动接口。

```c
#include "rtthread.h"

#define DRV_DEBUG
#define LOG_TAG         "drv_hyperam"
#include <drv_log.h>

#define PSRAM_ADDRESS                 (0x64800000)

#ifdef BSP_USING_HYPERAM
#ifdef RT_USING_MEMHEAP_AS_HEAP
    struct rt_memheap system_heap;
#endif

static int hyperam_init(void)
{
    LOG_D("hyperam init success, mapped at 0x%X, size is %d bytes, data width is %d", PSRAM_ADDRESS, BSP_USING_HYPERAM_SIZE, 16);
#ifdef RT_USING_MEMHEAP_AS_HEAP
    /* If RT_USING_MEMHEAP_AS_HEAP is enabled, HYPERAM is initialized to the heap */
    rt_memheap_init(&system_heap, "hyperam", (void *)PSRAM_ADDRESS, BSP_USING_HYPERAM_SIZE);
#endif
    return RT_EOK;
}
INIT_BOARD_EXPORT(hyperam_init);
#endif
```
## 使用方法

### 编译与下载

1. 打开工程并完成编译。
2. 使用 **板载下载器 (DAP)** 将开发板的 USB 接口连接至 PC。
3. 通过编程工具将生成的固件烧录至开发板。

### 运行效果

* 烧录完成后，开发板上电即可运行示例工程。

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

