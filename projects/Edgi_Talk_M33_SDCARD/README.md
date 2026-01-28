# Edgi-Talk_SDCARD Example Project

[**中文**](./README_zh.md) | **English**

## Introduction

This example project runs on the **M33 core** with **RT-Thread RTOS**, demonstrating **SD card file operations**.
It allows users to quickly learn how to mount an SD card, write and read files, and operate files through the **serial command line**.

## SD Card Overview

### 1. Overview

**SD Card (Secure Digital Card)** is a compact, portable non-volatile storage device widely used in **embedded systems, cameras, smartphones, and data loggers**.
It consists of a **controller + NAND Flash memory** and communicates with the host through a standard interface.

Key features:

* Small size: typically **32 × 24 × 2.1 mm** (standard)
* Non-volatile NAND Flash storage
* Supports hot-plug and power-loss protection

### 2. SD Card Types

**By size:**

| Type          | Dimensions              |
| ------------- | ----------------------- |
| Standard SD   | 32 × 24 mm              |
| Mini SD       | 21.5 × 20 mm            |
| Micro SD (TF) | 15 × 11 mm, most common |

**By capacity:**

| Type | Capacity      |
| ---- | ------------- |
| SDSC | 1 MB ~ 2 GB   |
| SDHC | 4 GB ~ 32 GB  |
| SDXC | 32 GB ~ 2 TB  |
| SDUC | 2 TB ~ 128 TB |

**By speed class:**

* **Class 2/4/6/10**: minimum write speed 2/4/6/10 MB/s
* **UHS-I/II/III**: up to 312 MB/s
* **Video Speed Class V6/V10/V30/V60/V90**: suitable for video recording

### 3. SD Card Interface

1. **SPI mode**: uses SPI bus (MISO, MOSI, SCK, CS), simple but slower.
2. **SD mode (1-bit/4-bit)**: dedicated SD bus, faster data transfer.
3. **UHS mode**: high-speed interface, used in cameras and high-performance embedded systems.

### 4. Working Principle

1. **Command/Data transfer**: host sends commands (CMD), card responds (R1, R2, etc.), read/write blocks (512 bytes each).
2. **Controller management**: handles bad blocks, ECC, logical-to-physical mapping.
3. **Data storage**: stored in NAND Flash, supports multiple erasures (typical 100,000 cycles).

### 5. Performance

| Parameter       | Description                            |
| --------------- | -------------------------------------- |
| Capacity        | 1 GB ~ 128 TB                          |
| Block size      | 512 Byte                               |
| Interface speed | SPI/SD 1-bit/4-bit/UHS                 |
| Max transfer    | 25 MB/s (standard), 312 MB/s (UHS-III) |
| Voltage         | 3.3 V (some Micro SD support 1.8 V)    |
| Temperature     | -25 ℃ ~ 85 ℃                           |
| Endurance       | 10^4 ~ 10^5 write cycles               |

### 6. Applications

* **Consumer electronics**: phones, tablets, cameras
* **Embedded systems**: MCU/FPGA storage, logging, configuration
* **Industrial**: data acquisition, controllers
* **Audio/Video**: high-speed video recording
* **Automotive**: dashcams, navigation storage

## Software Description

* Developed on **Edgi-Talk** platform.

* Example features:

  * Mount and initialize SD card
  * Write file using `echo` command
  * Read file using `cat` command
  * Print results on serial terminal

* Provides a clear example of **RT-Thread filesystem and SD card interface usage**.

## Usage

### Build and Download

1. Open and compile the project.
2. Connect board USB to PC via **DAP**.
3. Flash the compiled firmware.
4. Insert SD card into the board.

### Running Result

* After powering on, the system mounts the SD card and initializes the filesystem.
* Use the **serial terminal** to perform file operations:

1. Write `test.txt`:

```sh
echo "Hello RT-Thread SDCARD!" ./test.txt
```

2. Read file content:

```sh
cat ./test.txt
```

* Sample serial output:

```
 \ | /
- RT -     Thread Operating System
 / | \     5.0.2 build Sep  8 2025 11:02:30
 2006 - 2022 Copyright by RT-Thread team
found part[0], begin: 1048576, size: 29.739GB
Hello RT-Thread
This core is cortex-m33
Mount SD card success!

> echo "Hello RT-Thread SDCARD!" ./test.txt
> cat ./test.txt
Hello RT-Thread SDCARD!
```

* `echo` writes strings to files, `cat` reads and displays file content.

## Notes

* Ensure the SD card is inserted and formatted with FAT filesystem (FAT16/FAT32).
* To modify graphical configuration:

```
tools/device-configurator/device-configurator.exe
libs/TARGET_APP_KIT_PSE84_EVAL_EPC2/config/design.modus
```

* Save changes and regenerate code.
* If SD card fails to mount, check hardware connections and power supply.

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

⚠️ Follow this flashing order strictly.

---

* If the example does not run, first compile and flash **Edgi_Talk_M33_Blink_LED** to ensure core initialization.
* To enable M55, open:

```
RT-Thread Settings --> Hardware --> select SOC Multi Core Mode --> Enable CM55 Core
```
