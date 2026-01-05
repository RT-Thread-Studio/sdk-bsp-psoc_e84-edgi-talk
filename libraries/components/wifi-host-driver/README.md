## RT-Thread Wi-Fi Host Driver (WHD)

[中文](./README_CN.md) | English

### Overview
The WHD is an independent, embedded Wi-Fi Host Driver that provides a set of APIs to interact with Infineon WLAN chips. The WHD is an independent firmware product that is easily portable to any embedded software environment, including popular IoT frameworks such as Mbed OS and Amazon FreeRTOS. Therefore, the WHD includes hooks for RTOS and TCP/IP network abstraction layers.

Details about Wi-Fi Host Driver can be found in the [Wi-Fi Host Driver readme](./wifi-host-driver/README.md).<br>
The [release notes](./wifi-host-driver/RELEASE.md) detail the current release. You can also find information about previous versions.

The repository has adapted WHD to the RT-Thread system, currently only supports the SDIO bus protocol, and uses the mmcsd of RT-Thread for sdio bus operations.<br>
Welcome everyone `PR` to support more bus interface and chips.

### Porting layout
- [porting/inc](porting/inc) holds the minimal public headers mirrored from Infineon's BSP/HAL so that WHD can be built without pulling the whole ModusToolbox SDK.
- [porting/src/bsp](porting/src/bsp) wires REG_ON, HOST_WAKE and SDIO resources (see [porting/src/bsp/whd_bsp.c](porting/src/bsp/whd_bsp.c)) and exposes the helper network-buffer glue in [porting/src/bsp/whd_network_buffer.c](porting/src/bsp/whd_network_buffer.c).
- [porting/src/hal](porting/src/hal) provides the `cyhal_*` implementations for GPIO and SDIO so the upstream WHD HAL calls map directly onto RT-Thread drivers.
- [porting/src/rtos/whd_rtos.c](porting/src/rtos/whd_rtos.c) adapts the cyabs RTOS abstraction to native RT-Thread mutexes, semaphores, threads and timers.
- [porting/src/resources](porting/src/resources) contains the runtime loader (`resources.c`) and the `whd_res_download` CLI implementation (`download.c`).
- [porting/src/wlan/whd_wlan.c](porting/src/wlan/whd_wlan.c) registers the driver with RT-Thread's Wi-Fi stack and is the main entry point that honors the `WHD_PORTING_*` Kconfig switches.

### Using
**In the package, select `Wifi-Host-Driver(WHD) for RT-Thread`**
```
RT-Thread online packages  --->                         # Online software package
    IoT - internet of things  --->                      # IOT menu bar
        [*] Wifi-Host-Driver(WHD) for RT-Thread.  --->  # Select the package
```

### Package configuration
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
- `WHD_SET_COUNTRY_FROM_HOST` together with `WHD_COUNTRY_CODE`/`WHD_COUNTRY_CODE_REVISION` lets the host push the regulatory table that matches your deployment without rebuilding the firmware blob.
- `CY_WIFI_DEFAULT_ENABLE_POWERSAVE_MODE`, `CY_WIFI_DEFAULT_PM2_SLEEP_RET_TIME`, `CY_WIFI_USING_THREAD_INIT`, `CY_WIFI_INIT_THREAD_PRIORITY/STACK_SIZE` and the WHD thread menu let you pick the startup model that best matches your BSP scheduling limits.
- The resource menu replaces the old "external storage" toggle: select file paths when you boot from a filesystem, or select FAL partitions when you store the blobs in flash. `WHD_RESOURCES_BLOCK_SIZE` controls the read buffer size, and you can still override the weak `whd_wait_fs_mount()` in [porting/src/resources/resources.c](porting/src/resources/resources.c) if your filesystem needs extra time to get ready.
- `WHD_PORTING_BSP`, `WHD_PORTING_HAL`, `WHD_PORTING_RTOS` and `WHD_USE_CUSTOM_MALLOC_IMPL` map directly onto the code inside the [porting](porting) tree. Leave them enabled for the RT-Thread glue, or turn off the relevant switch when you must link against Infineon's original BSP/HAL/RTOS libraries.
- The log level selector wires up the WHD `WPRINT` macros so you can promote errors only, enable info/debug prints, or capture data traces while diagnosing SDIO traffic.

Recent WHD drops may ship without the regulatory `*.clm_blob` files or the board-specific `nvram.txt` snippets inside [wifi-host-driver/WHD/COMPONENT_WIFI5/resources](wifi-host-driver/WHD/COMPONENT_WIFI5/resources) and [wifi-host-driver/WHD/COMPONENT_WIFI6/resources](wifi-host-driver/WHD/COMPONENT_WIFI6/resources). If they are missing, request the firmware+NVRAM bundle from your module vendor to ensure you stay aligned with the certified radio profile.

`WHD_RESOURCES_IN_EXTERNAL_STORAGE_FS` enables loading every blob directly from a filesystem (SD card, SPI flash FS, USB mass storage, etc.). Just keep the configured paths (for example `/sdcard/whd/43438A1.bin`, `/sdcard/whd/43438A1.clm_blob`, `/sdcard/whd/nvram.txt`) accessible before WHD starts and there is no need to repartition on-chip flash. Make sure `whd_wait_fs_mount()` (provided as a weak symbol in [porting/src/resources/resources.c](porting/src/resources/resources.c)) blocks until the filesystem is mounted, or reorder your startup sequence so the filesystem driver finishes initialization before WHD tries to open the files.

When you keep the resource files on a filesystem, copy the matching `*.bin`, `*.clm_blob` and `nvram.txt` into the paths configured above. When you use FAL, make sure the named partitions exist and write the blobs once with the `whd_res_download` command.

The pin configuration menu also allows setting logical pin names (such as "PA.0") instead of numbers when your BSP exposes them, and the HOST_WAKE IRQ trigger can be switched between falling/rising/both edges to fit your module design.

**Note**<br>
sdio driver needs to support stream transfer. In the bsp of RT-Thread, most chips do not have the function of adapting stream transfer. <br>
The `Cortex-M4` core also requires software to compute `CRC16` and send it after the data, reference [stream transmission solution](./docs/SDIO数据流传输.md).<br>
For the `Cortex-M7` core, modify the "drv_sdio.c" file as shown in the following example: <br>
```c
/* The example is an sdio driver for the STM32H750 */
SCB_CleanInvalidateDCache();

reg_cmd |= SDMMC_CMD_CMDTRANS;
hw_sdio->mask &= ~(SDMMC_MASK_CMDRENDIE | SDMMC_MASK_CMDSENTIE);
hw_sdio->dtimer = HW_SDIO_DATATIMEOUT;
hw_sdio->dlen = data->blks * data->blksize;
hw_sdio->dctrl = (get_order(data->blksize)<<4) |
                    (data->flags & DATA_DIR_READ ? SDMMC_DCTRL_DTDIR : 0) | \
                    /* Adds detection of the DATA_STREAM flag */
                    ((data->flags & DATA_STREAM) ? SDMMC_DCTRL_DTMODE_0 : 0);
hw_sdio->idmabase0r = (rt_uint32_t)sdio->cache_buf;
hw_sdio->idmatrlr = SDMMC_IDMA_IDMAEN;
```

### Resource download
You can download resource files in ymodem mode. The resource files use the FAL component.<br>
The resource download function depends on the ymodem components.<br>
Make sure that `RT_USING_RYM` is enabled and that `WHD_RESOURCES_IN_EXTERNAL_STORAGE_FAL` is selected so the resources are read from flash partitions.
- Run the "whd_res_download" command on the terminal to download the resources.
- This command requires you to enter the partition name of the resource file.
- For example resource download(Use the default partition name, Enter your own partition name):
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
# Download firmware files
whd_res_download whd_firmware

# Download clm files
whd_res_download whd_clm

# Download nvram files
whd_res_download whd_nvram
```
- The ymodem tool can use xshell, after completing the command input, wait for xshell to initiate the file transfer.
```
msh >whd_res_download whd_firmware
Please select the whd_firmware file and use Ymodem to send.
```
- At this point, right-click the mouse in xshell and select "file transfer" to "using ymodem send".
- In the [wifi-host-driver/WHD/COMPONENT_WIFI5/resources](wifi-host-driver/WHD/COMPONENT_WIFI5/resources) directory (depending on the chip you chose in menuconfig), select the matching firmware, clm, and nvram files.
- `CYW55500` and `CYW55572` use the Wi-Fi 6 resource blobs located under [wifi-host-driver/WHD/COMPONENT_WIFI6/resources](wifi-host-driver/WHD/COMPONENT_WIFI6/resources).
- After the transmission is complete, msh will output the following log.
```
Download whd_firmware to flash success. file size: 419799
```
- After downloading the firmware and clm resource files, reset and restart.
- If you selected the filesystem option instead of FAL, copy the same firmware/CLM/NVRAM trio into the paths configured under "WHD Resources Configuration" (for example mount the SD card on your PC and drag the files into `/whd/`). Once the filesystem is mounted in RT-Thread—and `whd_wait_fs_mount()` confirms it is ready—WHD will consume the blobs directly without going through `whd_res_download`.

#### Resource file verification function (Recommended)
- In the package, select `TinyCrypt: A tiny and configurable crypt library`
```
RT-Thread online packages  --->                                   # Online software package
    security packages  --->                                       # In the security package
        TinyCrypt: A tiny and configurable crypt library  --->    # Select TinyCrypt
```
- Select the `TinyCrypt` package, after the file is downloaded complete, the md5 checksum is calculated and printed automatically.

### Supported Chip

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

'x' indicates no support<br>
'o' indicates tested and supported<br>
'*' means theoretically supported, but not tested

### Module selection
The project uses modules from `Xinlian Xinwei (Hangzhou) Technology Co., Ltd` for testing.<br>
The module models and corresponding chip models are as follows:
| **Module type** | **Chip type** |
| ------------ | ------------ |
| CYWL6208     | CYW43438     |
| CYWL6312     | CYW43012     |
| CYWL6209     | CYW43439     |
| CYWL6373     | CYW4373      |

If you need module testing, you can buy it from [Xinlian Xinwei Taobao](https://m.tb.cn/h.6ZgYzwpJFecDrhN).<br>
This TF card adapter board can be easily inserted into the TF card slot of the development board for testing.

### Contact & Suppor
- mail: 1425295900@qq.com
- WeChat: Evlers
- If there is any problem on the driver, welcome to submit PR or contact me to communicate with you.
- If you think this project is good and can meet your needs, please tip me~ Thank you for your support!<br>
<img src="docs/images/qrcode.png" alt="image1" style="zoom:50%;" />

### More information
* [Wi-Fi Host Driver API Reference Manual and Porting Guide](https://infineon.github.io/wifi-host-driver/html/index.html)
* [Wi-Fi Host Driver Release Notes](./wifi-host-driver/RELEASE.md)
* [Infineon Technologies](http://www.infineon.com)
