# Edgi_Talk_M55_USB_D CherryUSB USB Extend Screen Example

[**中文**](./README_zh.md) | **English**

## Overview

This project integrates **CherryUSB** on the **M55 core** of the Edgi-Talk board. It is prepared for **USB device mode**, uses the Infineon **DWC2** IP, and implements a **Windows USB extend screen**.

## Default Configuration

* `RT_USING_CHERRYUSB = y`
* `RT_CHERRYUSB_DEVICE = y`
* `RT_CHERRYUSB_DEVICE_SPEED_HS = y`
* `RT_CHERRYUSB_DEVICE_DWC2_INFINEON = y`
* Device template: **none** (user application)
* Display transport: vendor interface with RGB565 frames

## Build and Flash

1. Build the project in RT-Thread Studio or with SCons.
2. Flash the firmware via KitProg3 (DAP).
3. Connect the Type-C USB port for device enumeration.

## Configuration (Switching Modes)

Open RT-Thread Studio and go to:

```
RT-Thread Settings -> USB -> CherryUSB
```

* **Device mode**: enable `RT_CHERRYUSB_DEVICE`, select device IP under **CHERRYUSB_DEVICE_IP** (default: `RT_CHERRYUSB_DEVICE_DWC2_INFINEON`).
* **Host mode**: disable device mode, enable `RT_CHERRYUSB_HOST`, choose host IP and required class drivers (MSC/HID/CDC, etc.).
* **Device classes**: enable class drivers and select a template under **Select usb device template**.

If an IP/class requires extra parameters, edit:

* `libraries/Common/board/ports/usb/usb_config.h`

## Startup Sequence

The M55 core depends on the M33 boot flow. Flash in this order:

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

## Windows Driver and Usage

This project relies on a **Windows IDD driver** to present the device as an extended display. For background and driver model details, see:

* https://learn.microsoft.com/windows-hardware/drivers/display/indirect-display-driver-model-overview

Use a compatible IDD driver implementation that matches the device VID/PID and vendor interface.

## Notes

* This project targets the M55 core in USB device mode.
* For host mode, see [projects/Edgi_Talk_CherryUSB/Edgi_Talk_M55_USB_H/README.md](../Edgi_Talk_M55_USB_H/README.md).
* For M33 device mode, see [projects/Edgi_Talk_CherryUSB/Edgi_Talk_M33_USB_D/README.md](../Edgi_Talk_M33_USB_D/README.md).
* If the M55 example does not run, flash **Edgi_Talk_M33_Blink_LED** first.
* Enable CM55 in the M33 project:

  ```
  RT-Thread Settings -> Hardware -> select SOC Multi Core Mode -> Enable CM55 Core
  ```
![config](figures/config.png)