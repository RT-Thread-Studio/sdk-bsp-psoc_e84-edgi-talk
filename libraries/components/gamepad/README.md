# 手柄适配组件

该组件提供手柄连接和输入 API。型号驱动识别设备并映射实体键，`hid_report/` 解析报告中的按钮、轴和方向帽。[公共接口](gamepad.h) · [事件类型](hid_report/gamepad_report.h)

## 使用

在 `menuconfig` 的 **Hardware Drivers Config → Board extended module Drivers** 启用 `BSP_USING_BTSTACK_HID_HOST`，并在 **Gamepad models** 选择手柄型号。IINE-1001 的专用键位映射适用于 C 档 Gamepad 的 BLE 端点，IINE-1003 适用于 `[BR/EDR] Wireless Controller`。扫描列表将 `IG05 Gamepad(BLE)` 标记为 `IINE app endpoint`。[型号配置](Kconfig) · [双模示例](../../../projects/Edgi_Talk_Bluetooth/Edgi_Talk_M55_Classic_HID_Gamepad/README.md)

| 命令 | 用途 |
| --- | --- |
| `gamepad models`、`gamepad status` | 查看已启用型号、首选和当前连接 |
| `gamepad scan`、`gamepad list` | 开始扫描、查看本轮候选 |
| `gamepad connect <序号>` | 连接所选设备并自动配对 |
| `gamepad connect <地址> [类型]` | 按地址连接，明确指定 BLE 地址类型时用 `0`（public）或 `1`（random） |
| `gamepad pair` | 对当前连接请求配对 |
| `gamepad bonds` | 按最近成功连接顺序列出已保存设备及其配对状态 |
| `gamepad disconnect` | 断开当前连接并暂停自动回连 |
| `gamepad reconnect [序号]` | 恢复自动回连，或按 `bonds` 列表的序号指定设备 |
| `gamepad forget [序号]` | 清除当前或首选设备的绑定，也可用 `bonds` 的序号指定 |
| `gamepad raw on`、`gamepad raw off` | 开启或关闭逐包原始输出 |

自动回连优先尝试最近成功连接的设备，并轮询其他带可复用密钥的记录。手柄重置后，如果地址或配对密钥改变，请重新扫描并配对。

移植时用 `gamepad_register_platform()` 注册平台操作，再调用 `gamepad_demo_run()`，或先注册型号再通过 `gamepad_start()` 接收输入回调。新增型号用 `gamepad_register_model()` 注册识别条件和键位表。[平台示例](../../Common/board/ports/bluetooth/edgi_bluetooth.c) · [型号示例](iine_1001.c)
