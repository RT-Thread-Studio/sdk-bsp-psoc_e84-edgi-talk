# Edgi_Talk_M55_DEEPCRAFT_Deploy_Vision

[English (current)](./README.md) | [中文](./README_zh.md)

## 1. Overview

This project is an **RT-Thread implementation** of `PSOC Edge MCU: Machine learning - DEEPCRAFT deploy vision`, running on the Edgi-Talk **CM55** core, using:

- CherryUSB Host (DWC2, Infineon)
- UVC USB camera capture
- DEEPCRAFT/TFLM model inference
- Real-time LCD rendering of bounding boxes and labels

It keeps the same functional goal as the ModusToolbox example (real-time vision detection + UART logs), but the software framework is RT-Thread (MSH, SCons, RT-Thread Studio).

---

## 2. Main Differences vs. ModusToolbox Example

- OS/project framework: ModusToolbox -> RT-Thread project structure and configuration flow
- Runtime control: start/stop UVC stream through MSH commands (`usbh_uvc_start` / `usbh_uvc_stop`)
- USB app layer location: `libraries/Common/board/ports/usb`
- AI app layer location: `libraries/Common/deepcraft_ai` and `applications/uvc_ai_app.c`
- Model files are split out into:
  - `libraries/Common/deepcraft_ai/model/model.c`
  - `libraries/Common/deepcraft_ai/model/model.h`

---

## 3. Key Directory Paths

- App entry: `applications/main.c`
- AI app logic: `applications/uvc_ai_app.c`
- UVC app layer: `libraries/Common/board/ports/usb/usbh_uvc_app.c`
- UVC stream processing: `libraries/Common/board/ports/usb/usbh_uvc_stream.c`
- Model files: `libraries/Common/deepcraft_ai/model/model.c`, `libraries/Common/deepcraft_ai/model/model.h`
- AI core processing: `libraries/Common/deepcraft_ai/src/uvc_ai.c`

---

## 4. Environment and Dependencies

- RT-Thread Studio (2.2.9 or newer recommended)
- GNU Arm toolchain (with Cortex-M55 + Helium/MVE support)
- Board: PSOC Edge E84 family (Edgi-Talk target hardware)
- Camera: UVC USB camera (320x240 YUYV recommended)
- Display: project default LCD configuration

> Note: This project depends on the M33 boot chain to enable CM55. If CM55 does not start, verify M33 project configuration and flashing first.

---

## 5. Important Configuration Items

Check these options in `rtconfig.h` / `.config`:

- `RT_USING_CHERRYUSB`
- `RT_CHERRYUSB_HOST`
- `RT_CHERRYUSB_HOST_DWC2_INFINEON`
- `RT_CHERRYUSB_HOST_VIDEO`
- `BSP_USING_DEEPCRAFT_AI`
- `RT_USING_MSH`, `RT_USING_FINSH`

If you want local changes in `usbh_uvc_stream.c` / `usbh_uvc_port.c` to take effect, enable:

- `RT_CHERRYUSB_HOST_BUILD_FROM_SOURCE`

---

## 6. Build and Flash

### 6.1 RT-Thread Studio

1. Import `projects/Edgi_Talk_M55_DEEPCRAFT_Deploy_Vision`
2. Check configuration (menuconfig / Settings)
3. Build
4. Flash via KitProg3

### 6.2 Command line (SCons)

Run in project directory:

```bash
scons -j8
```

Generated artifacts include:

- `rt-thread.elf`
- `rtthread.hex`

---

## 7. Run Steps

1. Connect UART terminal at 115200 8N1
2. Connect LCD and UVC camera
3. After boot, run in MSH:

```bash
usbh_uvc_start 0 320 240
```

Parameters:

- `0` = YUYV
- `1` = MJPEG

> Note: In AI mode, the app forces `YUYV + 320x240` (other input parameters are overridden).

Stop command:

```bash
usbh_uvc_stop
```

### 7.1 Runtime Screenshots

- Figure 1: System running and detection display

![Runtime-1](figures/pic1.jpg)

- Figure 2: Camera frame with detection boxes

![Runtime-2](figures/pic2.jpg)

- Figure 3: UART logs and detection output

![Runtime-3](figures/pic3.jpg)

---

## 8. Logs and Behavior

- UVC status logs:
  - `UVC fps:...`
  - `start uvc stream...`
- AI logs:
  - `AI inference ...`
  - `AI #... bbox=[...]

---

## 9. Error Handling

If you see:

- `uvc abort1` / `uvc abort2`

The current implementation triggers a simplified auto-recovery flow (restart UVC stream) and prints:

- `UVC stream restart (...)`

If this happens frequently, prioritize checking:

- camera power/cable quality
- UVC bandwidth and resolution settings
- application load (display refresh, AI inference, log volume)

---

## 10. Model Replacement

This project now manages model files in a dedicated folder (aligned with the reference structure):

- `libraries/Common/deepcraft_ai/model/model.c`
- `libraries/Common/deepcraft_ai/model/model.h`

When replacing model files, keep exported interfaces compatible with current code (for example `IMAI_init / IMAI_compute / IMAI_finalize`) and rebuild.

---

## 11. Multi-core Boot Chain

Recommended flashing order:

1. CM33 Secure
2. CM33 Non-Secure (enables CM55)
3. CM55 this project

If CM55 does not run, make sure CM55 is enabled in the M33 project configuration.

