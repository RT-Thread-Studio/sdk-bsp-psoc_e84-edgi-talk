# Edgi-Talk M55 双模蓝牙手柄示例

本例通过 CYW55512 和 BTstack 连接 BR/EDR 或 BLE HID 手柄，从板载串口输出按键和摇杆事件。默认启用 IINE-1001、IINE-1003 和 Generic HID。

## 构建与烧录

在配置好 Arm GNU 13.3 的 RT-Thread Env 中，从仓库根目录执行：

```powershell
python libraries/components/bluetooth/cyw55512/prepare_firmware.py
cd projects/Edgi_Talk_Bluetooth/Edgi_Talk_M55_Classic_HID_Gamepad
.\mklinks.bat
scons -j8
```

Windows 源码工程首次构建时运行 `mklinks.bat`。通过 Studio 导入时，在导出工程根目录准备 CYW55512 固件。离线准备可为固件脚本传入 `--hcd <固件文件>`。BTstack 源码随仓库提供。[BTstack 许可](../../../libraries/components/bluetooth/btstack/README.md) · [Infineon 固件许可](https://github.com/Infineon/ifx-linux-bluetooth/blob/e5f0a31b95d51bbeb7ea5ef152ab0ff563961ff8/LICENSE.txt)

板上需先具备安全启动固件和启用 M55 的 M33 镜像。通过 KitProg3 烧写生成的 `rtthread.hex`，复位后将板载串口设置为 **115200、8N1**。[M33 启动模板](../../Edgi_Talk_M33_Template/README_zh.md)

## 连接手柄

1. 让手柄进入配对模式，执行 `gamepad scan`，用 `gamepad list` 查看候选。
2. 执行 `gamepad connect <序号>`。IINE-1001 选择 C 档 Gamepad 的 `[BLE]` 端点，IINE-1003 选择 `[BR/EDR] Wireless Controller`。列表中的 `IG05 Gamepad(BLE)` 标记为 `IINE app endpoint`。
3. 执行 `gamepad status` 查看连接状态、手柄型号和接收的报告数。串口输出按键和摇杆事件。[完整命令](../../../libraries/components/gamepad/README.md)

## 配对记录与回连

RRAM 最多保存 8 条 BR/EDR 或 BLE 手柄记录。用 `gamepad bonds` 查看记录，标记为 `bonded` 的手柄可复用密钥。板子启动时优先尝试最近成功连接的手柄，连接失败或掉线后继续尝试其他已配对设备。

手动切换时执行 `gamepad disconnect`，断开后用 `gamepad reconnect <序号>` 选择设备。已配对的 BR/EDR 手柄可在出现 `waiting for bonded Classic HID` 时开机，由手柄主动回连。单独执行 `gamepad reconnect` 可恢复自动回连。

手柄重置后，如果地址或密钥改变，请重新扫描并配对。使用轮换 BLE 私有地址的手柄可能需要重新扫描，连接时仍可复用已保存的密钥。

用 `gamepad forget <序号>` 删除指定记录。绑定区占用 `user_nvm` 的最后 8 KiB（M55 地址 `0x02061000–0x02062FFF`），兼容读取旧版配对记录。
