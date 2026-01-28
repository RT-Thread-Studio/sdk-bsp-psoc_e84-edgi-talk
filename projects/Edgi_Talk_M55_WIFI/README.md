# Edgi-Talk_WIFI Example Project

[**中文**](./README_zh.md) | **English**

## Introduction

This example demonstrates **Wi-Fi functionality** on the **M55 core** using **RT-Thread RTOS**.
It allows users to quickly test Wi-Fi scanning, connection, and performance, verifying the Wi-Fi module interface.

## Hardware Overview

### Wi-Fi Interface

![alt text](figures/1.png)

### BTB Socket

![alt text](figures/2.png)

### MCU Interface

![alt text](figures/3.png)

## Software Description

* Developed on **Edgi-Talk** platform.

* Example features:

  * Wi-Fi scanning
  * Wi-Fi connection
  * Iperf performance test

* Provides a clear example of **Wi-Fi driver integration with RT-Thread**.

## Usage

### Build and Download

1. Open and compile the project.
2. Connect the board USB to PC via **DAP**.
3. Flash the compiled firmware.

### Prepare Wi-Fi resources (first-time setup)

The Wi-Fi host driver loads three blobs (firmware `.bin`, regulatory `.clm_blob`, and board-specific `nvram.txt`) from FAL before it can power up the radio. These files live outside the application image, so flashing a new binary will not refresh them automatically. The default bundles for Edgi-Talk live in the workspace root under `resources/`.

- Keep `WHD_RESOURCES_IN_EXTERNAL_STORAGE_FAL` enabled in menuconfig and make sure the FAL table provides the `whd_firmware`, `whd_clm`, and `whd_nvram` partitions (the defaults reserve 512 KB + 32 KB + 32 KB of on-chip flash).
- Attach a serial terminal, reboot into the `msh` prompt, and run the download helper for each partition:

```
whd_res_download whd_firmware
whd_res_download whd_clm
whd_res_download whd_nvram
```

Each command switches to YMODEM mode. Use a terminal that supports YMODEM upload (Xshell) to send the matching files from the top-level `resources/` directory .
- Wait for the `Download … success` message before moving to the next partition.
- Power-cycle or reset the board after the three transfers so Wi-Fi starts with the freshly stored blobs. Re-run the command whenever you update the firmware/CLM/NVRAM bundle.

![wifi](figures/wifi.gif)

### Running Result

* After power-on, the system initializes the Wi-Fi device.
* Connect to a Wi-Fi network via serial terminal:

```
wifi scan
```
![alt text](figures/5.png)
```
wifi join <SSID> <PASSWORD>
```
![alt text](figures/6.png)
```
ping www.rt-thread.org
```
![alt text](figures/7.png)
* After connection, perform throughput test with iperf.
* A GUI tool (`jperf`) is provided under `packages/netutils-latest/tools`.

  * Extract `jperf.rar` and run the `.bat` file to launch the tool.
* Start iperf test from the board (replace `<PC_IP>` with actual PC IP):

```
iperf -c <PC_IP>
```

* Prefer 2.4 GHz network for testing (can use PC hotspot).

### Notes

* Ensure the Wi-Fi module is correctly connected.
* Serial terminal commands allow scanning, joining, and testing Wi-Fi.

## Startup Sequence

```
+------------------+
|   Secure M33     |
|  (Secure Core)   |
+------------------+
          |
          v
+------------------+
|       M33        |
| (Non-Secure Core)|
+------------------+
          |
          v
+-------------------+
|       M55         |
| (Application Core)|
+-------------------+
```

⚠️ Flash in this order strictly.

---

* If the example does not run, first compile and flash **Edgi_Talk_M33_Blink_LED** and **Edgi-Talk_M33_Template**.
* To enable M55:

```
RT-Thread Settings --> Hardware --> select SOC Multi Core Mode --> Enable CM55 Core
```
![config](figures/config.png)