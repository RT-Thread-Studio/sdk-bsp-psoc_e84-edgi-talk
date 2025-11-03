# Edgi-Talk_M55_Blink_LED Example Project

[**中文**](./README_zh.md) | **English**

## Introduction

This example project is based on the **Edgi-Talk platform** and demonstrates the **green LED blinking** function running on the **RT-Thread real-time operating system**.
Through this project, users can quickly verify the GPIO configuration and LED control logic of the board, providing a basic reference for subsequent hardware control and application development.

## GPIO Overview

**GPIO (General Purpose Input/Output)** is one of the most commonly used peripheral interfaces of MCUs. It can be configured via software as **input mode** or **output mode**:

* **Input Mode**: Used to read external voltage levels, such as button inputs.
* **Output Mode**: Used to control peripheral voltage levels, such as lighting an LED or driving a buzzer.

### GPIO Abstraction in RT-Thread

RT-Thread provides a **PIN device driver framework**, which abstracts hardware differences through a unified interface:

* `rt_pin_mode(pin, mode)`: Set the pin mode (input/output/pull-up/pull-down, etc.)
* `rt_pin_write(pin, value)`: Output a voltage level (high/low)
* `rt_pin_read(pin)`: Read input level

This allows developers to perform GPIO control using RT-Thread APIs without directly manipulating hardware registers.

In this example, the LED pin is configured as **output mode**. The software toggles the output between high and low levels in a loop to achieve LED blinking.

## Hardware Description

![1](figures/1.png)
![2](figures/2.png)
![3](figures/3.png)

As shown above, the Edgi-Talk board provides three user LEDs: USER_LED1 (RED), USER_LED2 (GREEN), and USER_LED3 (BLUE).
Among them, USER_LED2 corresponds to pin P16_7.
When the MCU outputs a **high level**, the LED lights up; when it outputs a **low level**, the LED turns off.

The position of the LEDs on the development board is shown below:

![4](figures/4.png)

## Software Description

* The project is developed based on the **Edgi-Talk** platform.

* Example features include:

  * Periodic blinking of the green LED
  * GPIO output control

* The project structure is simple and helps users understand LED control logic and hardware driver interfaces.

## Usage

### Build and Download

1. Open the project and compile it.
2. Connect the board’s USB interface to the PC using the **onboard debugger (DAP)**.
3. Use the programming tool to flash the generated firmware to the development board.

### Running Result

* After flashing, power on the board to run the example project.
* The **green LED blinks every 500 ms**, indicating that GPIO control and system scheduling are functioning correctly.
* Users can modify the blinking period or LED control logic as needed.

## Notes

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

⚠️ Please follow the above flashing order strictly; otherwise, the system may fail to run properly.

---

* If the example does not run correctly, first compile and flash the **Edgi-Talk_M33_S_Template** project to ensure that initialization and core startup are working properly before running this example.
* To enable the M55 core, enable the following configuration in the **M33 project**:

  ```
  RT-Thread Settings --> Hardware --> select SOC Multi Core Mode --> Enable CM55 Core
  ```
![config](figures/config.png)