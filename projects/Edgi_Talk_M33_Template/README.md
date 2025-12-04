# Edgi-Talk_M33_Template Example Project

[**中文**](./README_zh.md) | **English**

## Introduction

This example project runs on the **M33 core** with **RT-Thread Real-Time Operating System**.
It allows users to quickly experience RT-Thread running on the M33 platform.

After flashing and powering on the board, the **blue LED** will blink periodically, indicating that the system is running normally.
This project can also serve as a **template** for further development or project creation, helping users quickly get started and extend functionalities.

## Software Description

* Developed on the **Edgi-Talk platform**.

* Uses **RT-Thread** as the OS kernel.

* Example features:

  * System initialization
  * LED task (blinking)

* The project structure is clear, making it a good starting point for learning RT-Thread or developing applications.

## Usage

### Build and Download

1. Open and compile the project.
2. Connect the board’s USB interface to your PC using the **onboard debugger (DAP)**.
3. Flash the compiled firmware to the board.

   * During flashing, the following tool will be automatically invoked to merge the signed firmware:

     ```text
     tools/edgeprotecttools/bin/edgeprotecttools.exe
     ```

   * By default, `proj_cm33_s_signed.hex` in the directory will be merged and flashed to the target device.

### Running Result

* After flashing and powering on, the board will run the example project.
* The **blue LED** will blink periodically, indicating that the RT-Thread scheduler has started successfully.

## Notes

* To modify the **graphical configuration**, use the following tools:

```text
tools/device-configurator/device-configurator.exe
libs/TARGET_APP_KIT_PSE84_EVAL_EPC2/config/design.modus
```

* Save changes and regenerate code after editing.

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

⚠️ Follow this flashing order strictly; otherwise, the system may not operate correctly.

* To enable M55, open the configuration in RT-Thread Settings:

```
Hardware --> select SOC Multi Core Mode --> Enable CM55 Core
```
