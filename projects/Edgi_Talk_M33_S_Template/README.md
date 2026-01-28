# Edgi_Talk_M33_Blink_LED Example Project

[**中文**](./README_zh.md) | **English**

## Introduction

This example project is based on a **bare-metal architecture** and demonstrates the configuration and usage of the **Secure M33 core**.
It can also serve as a **template** for further development or project creation, helping users quickly get started and extend functionalities.

## Software Description

* Developed on the **Edgi-Talk platform**.

* Example features include:

  * **Secure region configuration**
  * **Basic startup flow demonstration**

* The project code is structured clearly, making it easy to understand and port.

## Usage

### Build and Download

1. Open and compile the project.
2. Connect the board’s USB interface to your PC using the **onboard debugger (DAP)**.
3. Flash the compiled firmware to the board using your programming tool.

### Running Result

* After flashing and powering on, the board will start the system normally.
* It will successfully boot into the **M33 core**, indicating that the secure configuration is effective.

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
