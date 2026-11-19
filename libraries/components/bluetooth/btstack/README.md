# BlueKitchen BTstack

BTstack v1.6.2 为双模 HID Host 提供 BR/EDR HID 和 BLE GATT。BLE Secure Connections 使用同版源码附带的 micro-ecc。上游提交记录在 [upstream/.revision](upstream/.revision)。

## 使用

SCons 直接编译仓库中的 `upstream/`。其中 `src/classic/hid_host.c` 接入已加密手柄的描述符缓存。[缓存补丁](patches/btstack_hid_descriptor_cache.inc)

移植时提供 `btstack_config.h` 和 H4 UART 接口，通过 `rtthread_btstack_start()` 启动协议栈。[RT-Thread 端口](port/rtthread/rtthread_btstack.h)

## 注意事项

BTstack 的公开许可限个人非商业使用，商用需向 BlueKitchen 取得许可。[BTstack 许可](upstream/LICENSE) · [micro-ecc 许可](upstream/3rd-party/micro-ecc/LICENSE.txt)
