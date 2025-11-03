# Edgi-Talk_CDC_Echo Example Project

[**中文**](./README_zh.md) | **English**

## Introduction

This example project is based on the **Edgi-Talk platform**, demonstrating the **USB CDC (Virtual COM Port) echo functionality** running on the **RT-Thread real-time operating system (M33 core)**.
Through this project, users can quickly experience USB CDC device communication and verify data echo functionality, providing a reference for future USB communication and multi-core application development.

## Hardware Description

### USB Interface

![alt text](figures/1.png)

### BTB Socket

![alt text](figures/2.png)

### MCU Interface

![alt text](figures/3.png)

### Physical Board LED/Port Location

![alt text](figures/4.png)

## Software Description

* The project is developed based on the **Edgi-Talk** platform.

* Example features include:

  * USB CDC device initialization
  * Virtual COM port data echo

* The project structure is clear, making it easy to understand how the USB device driver runs on the **M33 core**.

## Usage

### Build and Download

1. Open the project and compile it.
2. Connect the board’s USB interface to the PC using the **onboard debugger (DAP)**.
3. Use the programming tool to flash the generated firmware to the development board.

### Running Result

* After flashing, power on the board to run the example.
* Manually enter the following command in the serial terminal:

```
cdc_sample
```

* The system outputs the following startup information:

```
 \ | /
- RT -     Thread Operating System
 / | \     5.0.2 build Sep  8 2025 09:57:30
 2006 - 2022 Copyright by RT-Thread team
Hello RT-Thread
This core is cortex-m33
msh >cdc_sample
****************** PSOC Edge MCU: CDC echo using emUSB-device******************
```

* On the PC, use any serial terminal tool to connect to the board’s USB virtual COM port:

1. Open the serial terminal tool on your PC and connect to the board’s virtual COM port (baud rate can be any value).
2. Enter a string and **end it with a newline character `\n`**.
3. The board will echo the complete string back to the terminal:

```
> hello
hello
> 12345
12345
```

## Notes

* **Echo trigger condition**:
  Echo occurs only when the last character of the received data is a **newline `\n`**.
  If the input does not include `\n`, the data is stored in the buffer and will not be echoed immediately.

* To modify the **graphical configuration** of the project, open the configuration file using the following tool:

```
tools/device-configurator/device-configurator.exe
libs/TARGET_APP_KIT_PSE84_EVAL_EPC2/config/design.modus
```

* After modification, save the configuration and regenerate the code.

## Startup Sequence

The system starts in the following order:

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

⚠️ Please strictly follow the above flashing sequence; otherwise, the system may fail to run properly.

---

* If the example does not run correctly, first compile and flash the **Edgi-Talk_M33_S_Template** project to ensure proper initialization and core startup, then run this example.
* To enable the M55 core, enable the following configuration in the **M33 project**:

```
RT-Thread Settings --> Hardware --> select SOC Multi Core Mode --> Enable CM55 Core
```
