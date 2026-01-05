## RT-Thread Wi-Fi Host Driver (WHD)

中文 | [English](./README.md)

### 概述
WHD是一个独立的嵌入式Wi-Fi主机驱动程序，它提供了一组与英飞凌WLAN芯片交互的api。WHD是一个独立的固件产品，可以很容易地移植到任何嵌入式软件环境，包括流行的物联网框架，如Mbed OS和Amazon FreeRTOS。因此，WHD包含了RTOS和TCP/IP网络抽象层的钩子。

有关Wi-Fi主机驱动程序的详细信息可在[Wi-Fi Host Driver readme](./wifi-host-driver/README.md)文件中找到。<br>
[release notes](./wifi-host-driver/RELEASE.md)详细说明了当前的版本。您还可以找到有关以前版本的信息。

该存储库已将WHD适应于RT-Thread系统，目前仅支持SDIO总线协议，并使用RT-Thread的mmcsd进行SDIO总线操作。<br>
欢迎大家`PR`支持更多总线接口和芯片。

### 移植结构
- [porting/inc](porting/inc) 存放与英飞凌 BSP/HAL 对齐的最小公共头文件，避免必须引入完整的 ModusToolbox SDK 才能编译。
- [porting/src/bsp](porting/src/bsp) 负责 REG_ON、HOST_WAKE 与 SDIO 资源的绑定（参考 [porting/src/bsp/whd_bsp.c](porting/src/bsp/whd_bsp.c)）以及网络缓冲区适配代码 [porting/src/bsp/whd_network_buffer.c](porting/src/bsp/whd_network_buffer.c)。
- [porting/src/hal](porting/src/hal) 提供 `cyhal_*` 系列 GPIO/SDIO 实现，把 WHD 的 HAL 调用映射到 RT-Thread 驱动。
- [porting/src/rtos/whd_rtos.c](porting/src/rtos/whd_rtos.c) 将 cyabs RTOS 抽象转换为 RT-Thread 的线程、互斥量、信号量与定时器。
- [porting/src/resources](porting/src/resources) 包含运行时资源加载器 (`resources.c`) 与 `whd_res_download` CLI (`download.c`)。
- [porting/src/wlan/whd_wlan.c](porting/src/wlan/whd_wlan.c) 负责把驱动注册到 RT-Thread Wi-Fi 框架，同时根据 `WHD_PORTING_*` Kconfig 选项裁剪功能。

### 使用

**在软件包选中`Wifi-Host-Driver(WHD) for RT-Thread`**
```
RT-Thread online packages  --->                         # 在线软件包
    IoT - internet of things  --->                      # IOT菜单栏中
        [*] Wifi-Host-Driver(WHD) for RT-Thread.  --->  # 选中该软件包
```

**软件包配置**
```
--- Wifi-Host-Driver(WHD) for RT-Thread
    --- WHD Configuration
        [*] Set country code from host
            (AU) Set the default country code
            (0)  Set the default country code revision
        [ ] Default enable powersave mode
            (200) Set return to sleep delay.(PM2)
        [ ] Using thread initialization
            (10) The priority level value of init thread
            (2048) The stack size for init thread
        --- WHD Thread Configuration
            (8)   The priority level value of WHD thread
            (5120) The stack size for WHD thread
        --- WHD Resources Configuration
            ( ) File System
                (/sdcard/whd/43438A1.bin) Set the file path of the firmware files
                (/sdcard/whd/43438A1.clm_blob) Set the file path of the clm files
                (/sdcard/whd/nvram.txt) Set the file path of the nvram files
            (X) Flash Abstraction Layer(FAL)
                ("whd_firmware") Set the partition name of the firmware files
                ("whd_clm") Set the partition name of the clm files
                ("whd_nvram") Set the partition name of the nvram files
            (1024) Set the block size for resources
    --- Hardware Configuration
        (X) CYW43438
        ( ) CYW4373
        ( ) CYW43012
        ( ) CYW43439
        ( ) CYW43022
        ( ) CYW4343W
        ( ) CYW55500
        ( ) CYW55572
        --- Pin Configuration
            (Number) Select the pin name or number  --->
                (-1) Set the WiFi_REG ON pin number
                (-1) Set the HOST_WAKE_IRQ pin number
            (falling) Select HOST_WAKE_IRQ event type  --->
            (2) Set the interrupt priority for HOST_WAKE_IRQ pin
    --- Porting options
        [*] Using BSP porting
        [*] Using HAL porting
        [*] Using RTOS porting
        [*] Using the malloc/free of RT-Thread
    --- WHD log level
        (Error) Select the log level
```

- `WHD_SET_COUNTRY_FROM_HOST` 允许主机在运行时下发与目标市场匹配的法规配置 (`WHD_COUNTRY_CODE` 与 `WHD_COUNTRY_CODE_REVISION`)，无需重新打包固件资源。
- `CY_WIFI_DEFAULT_ENABLE_POWERSAVE_MODE`、`CY_WIFI_DEFAULT_PM2_SLEEP_RET_TIME`、`CY_WIFI_USING_THREAD_INIT`、`CY_WIFI_INIT_THREAD_PRIORITY/STACK_SIZE` 以及 WHD 线程菜单帮助你根据 BSP 的调度限制选择合适的启动模型。
- 新的资源菜单取代了旧的“外部存储”开关：若选择文件系统，请将 `*.bin`、`*.clm_blob`、`nvram.txt` 拷贝到对应路径；若选择 FAL，请保证分区存在，并调整 `WHD_RESOURCES_BLOCK_SIZE` 以匹配吞吐需求。若文件系统挂载较慢，可在 [porting/src/resources/resources.c](porting/src/resources/resources.c) 中重写弱符号 `whd_wait_fs_mount()`。
- `WHD_PORTING_BSP`、`WHD_PORTING_HAL`、`WHD_PORTING_RTOS` 与 `WHD_USE_CUSTOM_MALLOC_IMPL` 对应 [porting](porting) 目录下的适配代码：保持打开即可使用 RT-Thread Glue，如需对接英飞凌原生 BSP/HAL/RTOS，可按需关闭。
- 日志级别开关会为 WHD 的 `WPRINT` 宏启用不同的输出（仅错误/包含信息/调试/数据追踪），便于排障。

当资源文件存放于文件系统时，请把对应芯片的 `firmware/clm/nvram` 文件放到上面配置的路径；若使用 FAL，请提前写入相应分区，并通过 `whd_res_download` 命令刷新内容。

新的 WHD 发行版可能不再在 [wifi-host-driver/WHD/COMPONENT_WIFI5/resources](wifi-host-driver/WHD/COMPONENT_WIFI5/resources) 或 [wifi-host-driver/WHD/COMPONENT_WIFI6/resources](wifi-host-driver/WHD/COMPONENT_WIFI6/resources) 中附带 `*.clm_blob` 与 `nvram.txt`。若仓库缺失这些文件，请向模组/供应商索取官方认证的固件与 NVRAM 组合，避免违背本地射频法规。

当选择 `WHD_RESOURCES_IN_EXTERNAL_STORAGE_FS` 时，WHD 会直接从文件系统（如 SD 卡、SPI Flash 文件系统、U 盘等）读取资源；只需在启动前确保配置的路径（如 `/sdcard/whd/43438A1.bin`、`/sdcard/whd/43438A1.clm_blob`、`/sdcard/whd/nvram.txt`）已经存在即可，无需再为 FAL 分区。请实现或重写 [porting/src/resources/resources.c](porting/src/resources/resources.c) 中的弱符号 `whd_wait_fs_mount()`，或在系统启动顺序中保证文件系统驱动已挂载完成，再启动 WHD。

引脚菜单支持使用逻辑名称（例如 “PA.0”）或数字，同时可配置 HOST_WAKE 的触发沿（下降/上升/双沿）及中断优先级，以匹配模组硬件设计。

**注意**<br>
SDIO驱动需要支持数据流传输，在RT-Thread的bsp中，大多数芯片都未适配数据流传输的功能。<br>
`Cortex-M4`内核需要软件来计算`CRC16`并在数据后面发送它，参考 [数据流传输解决方案](./docs/SDIO数据流传输.md)。<br>
对于`Cortex-M7`内核，只需要修改`drv_sdio.c`文件的一处地方即可，示例如下: <br>
```c
/* 该示例是STM32H750的SDIO驱动程序 */
SCB_CleanInvalidateDCache();

reg_cmd |= SDMMC_CMD_CMDTRANS;
hw_sdio->mask &= ~(SDMMC_MASK_CMDRENDIE | SDMMC_MASK_CMDSENTIE);
hw_sdio->dtimer = HW_SDIO_DATATIMEOUT;
hw_sdio->dlen = data->blks * data->blksize;
hw_sdio->dctrl = (get_order(data->blksize)<<4) |
                    (data->flags & DATA_DIR_READ ? SDMMC_DCTRL_DTDIR : 0) | \
                    /* 增加数据流标志的检测 */
                    ((data->flags & DATA_STREAM) ? SDMMC_DCTRL_DTMODE_0 : 0);
hw_sdio->idmabase0r = (rt_uint32_t)sdio->cache_buf;
hw_sdio->idmatrlr = SDMMC_IDMA_IDMAEN;
```

### 资源下载
可以通过`ymodem`协议下载资源文件。驱动会使用FAL组件来加载资源文件。<br>
资源下载功能依赖于`ymodem`组件，请确保打开`RT_USING_RYM`，并在 `WHD Resources Configuration` 中选择 `Flash Abstraction Layer(FAL)`。<br>
- 在终端上执行`whd_res_download`命令开始下载资源。
- 该命令需要输入资源文件的分区名。
- 下载资源文件的实例(使用默认分区名，输入自己的分区名):
```c
/* partition table */
/*      magic_word          partition name      flash name          offset          size            reserved        */
#define FAL_PART_TABLE                                                                                              \
{                                                                                                                   \
    { FAL_PART_MAGIC_WORD,  "whd_firmware",     "onchip_flash",              0,     512 * 1024,         0 },        \
    { FAL_PART_MAGIC_WORD,  "whd_clm",          "onchip_flash",     512 * 1024,      32 * 1024,         0 },        \
    { FAL_PART_MAGIC_WORD,  "whd_nvram",        "onchip_flash",     544 * 1024,      32 * 1024,         0 },        \
}
```
```shell
# 下载固件文件
whd_res_download whd_firmware

# 下载clm文件
whd_res_download whd_clm

# 下载nvram文件
whd_res_download whd_nvram
```
- `ymodem`可以使用`xshell`工具，在完成命令输入后，等待`xshell`启动文件传输。
```
msh >whd_res_download whd_firmware
Please select the whd_firmware file and use Ymodem to send.
```
- 此时，在`xshell`中右键单击鼠标，选择`文件传输`到`使用ymodem发送`。
- 在 [wifi-host-driver/WHD/COMPONENT_WIFI5/resources](wifi-host-driver/WHD/COMPONENT_WIFI5/resources) 目录中，选择和 menuconfig 里芯片一致的 `firmware/clm/nvram` 文件。
- `CYW55500` 与 `CYW55572` 属于 Wi-Fi 6 器件，需要从 [wifi-host-driver/WHD/COMPONENT_WIFI6/resources](wifi-host-driver/WHD/COMPONENT_WIFI6/resources) 目录拷贝对应的资源文件。
- 传输完成后，msh将输出如下日志：
```
Download whd_firmware to flash success. file size: 419799
```
- 下载完固件、clm 和 nvram 资源文件后，复位重启即可正常加载。
- 若在 menuconfig 中选择了 "File System" 模式，可直接把同名的固件/CLM/NVRAM 文件拷贝进配置路径（如将 SD 卡接到电脑并复制至 `/whd/`），待文件系统挂载完成且 `whd_wait_fs_mount()` 返回后，WHD 会直接从该路径加载，无需执行 `whd_res_download`。

#### 资源文件的校验功能（建议打开）
- 在软件包选中`TinyCrypt: A tiny and configurable crypt library`
```
RT-Thread online packages  --->                                   # 在线软件包
    security packages  --->                                       # 安全软件包中
        TinyCrypt: A tiny and configurable crypt library  --->    # 选中TinyCrypt
```
- 打开`TinyCrypt`软件包后，资源文件下载完成会自动计算并打印文件的`MD5`校验和。

### 芯片支持

| **CHIP**  |**SDIO**|**SPI**|**M2M**|
|-----------|--------|-------|-------|
| CYW4343W  |   *    |   x   |   x   |
| CYW43438  |   o    |   x   |   x   |
| CYW4373   |   *    |   x   |   x   |
| CYW43012  |   o    |   x   |   x   |
| CYW43439  |   o    |   x   |   x   |
| CYW43022  |   *    |   x   |   x   |
| CYW55500  |   o    |   x   |   x   |
| CYW55572  |   *    |   x   |   x   |

'x' 表示不支持<br>
'o' 表示已测试和支持<br>
'*' 理论上支持，但未经过测试

### 模组选择
该项目使用`新联鑫威(杭州)科技有限公司`的英飞凌模组进行测试，以下是模组型号和对应的芯片型号：
| **模组型号** | **芯片型号** |
| ------------ | ------------ |
| CYWL6208     | CYW43438     |
| CYWL6312     | CYW43012     |
| CYWL6209     | CYW43439     |
| CYWL6373     | CYW4373      |

如需模组测试可到[新联鑫威淘宝](https://m.tb.cn/h.6ZgYzwpJFecDrhN)购买。<br>
这家有TF卡转接板，可以很方便的插到开发板的TF卡槽进行测试。

### 联系方式&支持
- 邮箱: 1425295900@qq.com
- 微信：Evlers
- 如果驱动上有什么问题欢迎提交PR或者联系我一起交流
- 如果你觉得这个项目不错，并且能够满足您的需求，请打赏打赏我吧~ 感谢支持！！<br>
<img src="docs/images/qrcode.png" alt="image1" style="zoom:50%;" />

### 更多信息
* [Wi-Fi Host Driver API Reference Manual and Porting Guide](https://infineon.github.io/wifi-host-driver/html/index.html)
* [Wi-Fi Host Driver Release Notes](./wifi-host-driver/RELEASE.md)
* [Infineon Technologies](http://www.infineon.com)
