# Edgi-Talk_RTC Example Project

[**中文**](./README_zh.md) | **English**

## Introduction

This example project runs on the **M33 core** with **RT-Thread RTOS**, demonstrating the **RTC (Real-Time Clock)** functionality.
It allows users to quickly learn how to set, read, and print RTC time, providing a reference for time management and periodic tasks in embedded systems.

## RTC Overview

### 1. Overview

**RTC (Real-Time Clock)** is an electronic module or chip that **keeps track of actual time** (year, month, day, hour, minute, second).
It can be integrated in an **MCU** or exist as an external chip, providing system time, alarm triggers, and timed event functions.

RTC is **low-power and stable**, continuing to operate even when the main power is off, using backup power such as a battery or supercapacitor.

### 2. Working Principle

RTC consists of a **low-power oscillator + counter**:

1. **Clock source**

   * Typically uses a **32.768 kHz crystal**, providing a stable time base.
2. **Frequency division**

   * Divides the crystal signal to produce a 1 Hz second pulse.
   * Counts seconds to generate minutes, hours, days, months, and years.
3. **Registers**

   * Internal registers store current time, date, alarms, etc.
4. **Power backup**

   * Battery or supercapacitor maintains RTC operation during main power loss.

### 3. RTC Types

* **Internal RTC**

  * Integrated inside MCU.
  * Pros: lower cost, fewer components.
  * Cons: crystal accuracy affected by PCB and temperature.
* **External RTC**

  * Standalone chips, e.g., DS3231, PCF8563.
  * Pros: high accuracy, I²C/SPI interface.
  * Cons: increases PCB space and cost.

### 4. Key Parameters

| Parameter          | Description                                                |
| ------------------ | ---------------------------------------------------------- |
| Oscillator         | Typically 32.768 kHz, low-power and stable                 |
| Accuracy           | ppm / seconds per day, affects drift                       |
| Supply voltage     | 1.8~5V, supports backup power                              |
| Power consumption  | 1~5 µA (low-power mode)                                    |
| Interface type     | I²C, SPI, or internal MCU bus                              |
| Extended functions | Alarm, square wave, temperature compensation, timed wakeup |

### 5. RTC Functions

* **Real-time clock**: provides current time and date.

### 6. Applications

* **Embedded devices**: time tracking and event logging.
* **Low-power IoT**: periodic wake-up for data collection.
* **Wearables**: precise timing for watches or fitness devices.
* **Data loggers & industrial control**: timestamps and logs.
* **Automotive electronics**: dashboard timing, recorders, infotainment.

## Software Description

* Developed on the **Edgi-Talk** platform.

* Example features:

  * Initialize RTC device
  * Set date and time
  * Delay and read current time
  * Export RTC functions as `rtc_sample` msh command

* The structure clearly demonstrates RTC driver usage on **M33 core**.

## Usage

### Build and Download

1. Open the project and compile it.
2. Connect the board USB to your PC via **DAP**.
3. Flash the compiled firmware.

### Running Result

* After powering on, open the serial terminal and run:

```
rtc_sample
```

* The system performs:

  1. Initialize RTC device
  2. Set date to `2025-07-01`
  3. Set time to `23:59:50`
  4. Print current time
  5. Delay 3 seconds
  6. Print time again

* Example output:

```
Tue Jul  1 23:59:50 2025
Tue Jul  1 23:59:53 2025
```

## Notes

* Ensure RTC is properly connected and recognized by the system.
* To modify the graphical configuration:

```
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

* If the example does not run, first compile and flash **Edgi_Talk_M33_Blink_LED** to ensure core initialization.
* To enable M55, open:

```
RT-Thread Settings --> Hardware --> select SOC Multi Core Mode --> Enable CM55 Core
```
