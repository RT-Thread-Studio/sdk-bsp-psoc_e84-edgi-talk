# Edgi_Talk_M55_BT_Speaker Example Project

[**中文**](./README_zh.md) | **English**

## Introduction

This example runs on the **M55 core** of the Edgi-Talk board and turns it into a **Bluetooth speaker**:

- A phone streams music to the board over Classic Bluetooth **A2DP**;
- The board runs **BlueKitchen BTstack** for the Classic Bluetooth stack, A2DP Sink, AVRCP and SBC decoding;
- Decoded PCM is sent through the RT-Thread `sound0` audio device to the **ES8388 + I2S/TDM** codec and the on-board speaker;
- AVRCP absolute volume and play/pause controls from the phone are supported.

> **License notice**: the RT-Thread `btstack` package is based on BlueKitchen BTstack. It is **free for non-commercial use**; commercial products require a license from BlueKitchen. See `packages/btstack-latest/LICENSE`. For a commercial product, consider replacing it with Infineon's AIROC Bluetooth stack.

![Edgi-Talk board](figures/board.jpg)

## Hardware Overview

| Function | Hardware |
| :--- | :--- |
| Bluetooth controller | On-board **CYW55512** Wi-Fi + Bluetooth combo (55500A1 die), BT connected over HCI UART |
| BT HCI UART | SCB4, RT-Thread device `uart4`, pins P10.0(RX) / P10.1(TX) / P10.2(CTS) / P10.3(RTS) |
| Audio output | ES8388 codec (I2C0, address 0x10) + I2S/TDM + on-board speaker |
| Speaker enable | `SPEAKER_OE_CTRL` (P21.6), driven high by `en_gpio()` in `board.c` |
| Module power | `WIFI_OE_CTRL` / `WIFI_WL_REG_OE_CTRL`, driven high by `en_gpio()` in `board.c` |

The Bluetooth firmware is the `wlbga_iPA_sLNA_ANT0_LHL_XTAL_IN` variant of Infineon
`bt-fw-ifx-cyw55500a1` (release-v2.2.0, version
`CYW55500A1_001.002.032.0145.0000_Generic_UART_47MHz`; matching `BSP_COMPONENTS` /
`DEVICE_COMPONENTS` in the BSP `bsp.mk`). It is embedded in
`applications/bt_stack/btfw.c`, so no separate firmware partition is needed.

## Software Description

```
Edgi_Talk_M55_BT_Speaker
├── applications
│   ├── bt_speaker.c              // A2DP Sink + AVRCP application and MSH commands
│   └── bt_stack
│       ├── btstack_config.h      // Classic-only BTstack configuration
│       ├── btstack_run_loop_rtt.c    // RT-Thread run loop
│       ├── btstack_uart_block_rtt.c  // uart4 HCI H4 transport
│       ├── btstack_audio_rtt.c       // sound0 audio sink
│       ├── btfw.c                // CYW55500A1 firmware
│       └── SConscript
├── board                         // Board init (power, ES8388, speaker amp)
├── packages
│   └── btstack-latest            // Trimmed BTstack sources (Classic + A2DP + SBC)
├── figures
└── Kconfig / rtconfig.h / .config
```

Key implementation details:

- **BT power/download sequence**: keep P11.0 (`BT_REG_ON`) low, initialize `uart4` (SCB4 asserts
  RTS, i.e. the module CTS is low), then release `BT_REG_ON`. The boot ROM enters auto-baud mode;
  the first HCI Reset must follow within 3 seconds.
- **UART**: firmware download is fixed at **115200** (the CYW55512 boot ROM does not answer the
  `0xFC18` baud command until the minidriver has been downloaded). Once the patched firmware is
  running, the example sends `0xFC18` at HCI `WORKING` to switch the controller and host UART to
  **3 Mbps**. The 127 KB patch takes about 11 s at 115200.
- **Audio**: each A2DP media packet is decoded immediately and resampled to 48 kHz into an
  8192-frame (~170 ms) PCM FIFO. The playback timer uses an **absolute deadline**, preventing
  BTstack run-loop wake-up latency from accumulating, and a 4-buffer ring absorbs writer jitter.
  `sound0` runs at 48 kHz / 16-bit; with `BSP_USING_AUDIO_STEREO`, `drv_i2s.c` accepts
  interleaved stereo directly instead of duplicating a mono stream.
- **I2S clock calibration**: the on-board TDM 48 kHz divider uses `7 + 0/32` (measured ~47.96 kHz),
  about 0.08% below the 48 kHz playback timer; the PCM FIFO drift control absorbs the difference.
- **Run loop**: instead of POSIX select/pthreads, a custom `btstack_run_loop_t` uses an RT-Thread
  thread, semaphore and tick, reducing the POSIX configuration requirements.

## Usage

### Build and Download

1. Open/import the project with **RT-Thread Studio 2.2.9** or newer (or run `scons` in the project
   directory; the ARM GNU path is taken from `rtconfig.py`).
2. Build `Debug/rtthread.hex`.
3. **Flash `Edgi_Talk_M33_Template` first** (it must have CM55 enabled), then flash the M55 image
   of this project.
4. Reset the board. The serial console should show:

   ```
   Edgi-Talk Bluetooth speaker example
   Phone pairing name: Edgi-Talk BT Speaker
   [bt] starting Bluetooth speaker
   [bt] firmware downloaded, starting HCI init
   [bt] stack up, local address xx:xx:xx:xx:xx:xx
   [bt] host UART switched to 3000000 baud
   ```

### Pairing and Playback

1. Open Bluetooth settings on the phone and search for **Edgi-Talk BT Speaker**.
2. Pair. The device has no display, so Just Works pairing is accepted automatically; older phones
   asking for a PIN can use `0000`.
3. Play music on the phone; audio comes out of the board speaker.
4. Phone volume keys control the ES8388 output through AVRCP absolute volume.

### MSH Commands

```text
bt_speaker start          # Start the speaker (main starts it automatically)
bt_speaker status         # Show state, A2DP/AVRCP connections and stream state
bt_speaker name <name>    # Change the Bluetooth name
bt_speaker stop           # Power off Bluetooth and exit the stack thread (reboot to start again)
```

## Notes

- **BTstack license**: free for non-commercial use; commercial use requires a BlueKitchen license.
- **Firmware variant**: `btfw.c` targets `wlbga_iPA_sLNA_ANT0_LHL_XTAL_IN`. If the board hardware
  revision differs, replace it with the matching `btfw.c` from `bt-fw-ifx-cyw55500a1`.
- **3 Mbps switch**: firmware download runs at 115200; once the patched firmware is running and
  the stack reaches WORKING, the example sends HCI `0xFC18` to switch the controller and host UART
  to 3 Mbps. If `[bt] host UART switched to 3000000 baud` does not appear, check `uart4` flow
  control and baud configuration.
- **M55 boot chain**: the M55 project does not contain an M33 image. Flash the M33 Template
  (`SOC_Enable_CM55=y`) first.
- Link keys are kept in RAM; re-pairing is needed after reset. Persistent storage can be added
  later with a TLV/Flash backend.

## Startup Sequence

```
+------------------+
|   Secure M33     |
|   (secure boot)  |
+------------------+
          |
          v
+------------------+
|       M33        |
|  (non-secure)    |
+------------------+
          |
          v
+-------------------+
|       M55         |
|  (this example)   |
+-------------------+
```

> If the example does not run, first build and flash **Edgi_Talk_M33_Template** and make sure
> CM55 is started.
