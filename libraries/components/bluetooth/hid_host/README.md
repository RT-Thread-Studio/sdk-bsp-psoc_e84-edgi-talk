# BTstack 双模 HID Host

该组件负责 BR/EDR 与 BLE 手柄的扫描、配对和连接。经典 HID 通过 SDP 获取描述符并从 HID 中断通道收报，BLE 通过 GATT 获取报告布局并订阅输入通知。

## 使用

通过 `classic_hid_start_with_uart()` 传入 H4 UART 接口和回调。需要保存绑定时使用 `classic_hid_start_with_store()`，通过回调读取和原子更新设备列表。Edgi-Talk 在 RRAM 中保存最多 8 条 BR/EDR 和 BLE 设备记录及其可复用密钥。[主机接口](classic_hid_host.h) · [板级实现](../../../Common/board/ports/bluetooth/edgi_bluetooth.c)

回调交付的描述符和输入缓冲区只在回调期间有效。
