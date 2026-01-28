# Edgi-Talk_LSM6DS3 Example Project

[**中文**](./README_zh.md) | **English**

## Introduction

This example is based on the **Edgi-Talk platform**, running on the **RT-Thread real-time operating system**, demonstrating how to drive the **LSM6DS3 six-axis sensor (accelerometer + gyroscope + temperature)**.

### LSM6DS3TR Overview

**LSM6DS3TR** is a **low-power six-axis Inertial Measurement Unit (IMU)** from STMicroelectronics, integrating a three-axis accelerometer and a three-axis gyroscope.

### Key Features

* **Three-axis accelerometer**: ±2/±4/±8/±16 g
* **Three-axis gyroscope**: ±125/±245/±500/±1000/±2000 dps
* **Operating voltage**: 1.71 V – 3.6 V
* Low power consumption, supporting multiple power-saving modes
* Built-in FIFO buffer (up to 8 KB)
* Supports **I²C** and **SPI** communication interfaces

### Applications

* Smartphones and wearable devices
* Motion detection and posture recognition
* Gesture recognition and gait analysis
* Robotics and drone attitude control

Through this example, users can learn to:

* Use RT-Thread’s **I²C device driver framework**
* Initialize and configure LSM6DS3 registers
* Read **three-axis acceleration**, **three-axis angular velocity**, and **temperature data**
* Output sensor data via the serial port

## Hardware Description

### LSM6DS3TR Interface

![alt text](figures/1.png)

### BTB Socket

![alt text](figures/2.png)

### MCU Interface

![alt text](figures/3.png)

## Software Description

* Developed on the **Edgi-Talk** platform
* Uses **RT-Thread** as the OS kernel
* Example features include:

  * Detect and verify device ID
  * Reset sensor and restore default configuration
  * Configure output data rate and full-scale range
  * Polling mode to read acceleration, angular rate, and temperature
  * Print sensor data to the serial terminal

## Usage

### Build and Download

1. Open the project and compile it.
2. Connect the board’s USB interface to the PC using the **onboard debugger (DAP)**.
3. Flash the generated firmware to the board using a programming tool.

### Running Result

* After flashing, power on the board to run the example:

```
Acceleration [mg]:  15.23   -3.12   1000.45
Angular rate [mdps]: 2.50   -1.25    0.75
Temperature [degC]:  26.54
```

* The **blue LED** will blink at a 500ms period, indicating that the system scheduler is running normally.

---

## Notes

* To modify the **graphical configuration**, open the configuration file using the following tool:

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

* If the example does not run correctly, first compile and flash the **Edgi_Talk_M33_Blink_LED** project to ensure proper initialization and core startup, then run this example.
* To enable the M55 core, enable the following configuration in the **M33 project**:

```
RT-Thread Settings --> Hardware --> select SOC Multi Core Mode --> Enable CM55 Core
```
