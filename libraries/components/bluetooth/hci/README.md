# 蓝牙 H4 UART 接口

`struct bluetooth_hci_uart` 定义 BTstack 与控制器间的字节流接口。[接口定义](bluetooth_hci_uart.h)

移植时填入波特率、流控以及 `open/read/write`。`open` 准备控制器和 UART，`read` 阻塞至有数据，`write` 成功时返回完整写入长度，失败时返回负值。Edgi-Talk 的 CYW55512 使用 SCB4。[控制器实现](../cyw55512/cyw55512_hci.c)
