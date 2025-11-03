# Retarget IO

### Overview

A utility library to retarget the standard input/output (STDIO) messages to a UART port. With this library, you can directly print messages on a UART terminal using `printf()`. You can specify the TX pin, RX pin, and the baud rate when configuring the UART.

**NOTE:** The standard library is not standard in how it treats an I/O stream. Some implement a data buffer by default. The buffer is not flushed until it is full. In that case it may appear that your I/O is not working. You should be aware of how the library buffers data, and you should identify a buffering strategy and buffer size for a specified stream. If you supply a buffer, it must exist until the stream is closed. The following line of code disables the buffer for the standard library that accompanies the GCC compiler:

    setvbuf( stdin, NULL, _IONBF, 0 );

**NOTE:** If the application is built using newlib-nano, by default, floating point format strings (%f) are not supported. To enable this support, you must add `-u _printf_float` to the linker command line.

**NOTE:** In general, console prints such as printf() should not be performed in ISR context. It must definitely not be called in ISR context when `CY_RTOS_AWARE` is defined, as the threat safety implementation disallows such calls.

NOTE: If the application is built without HAL support (i.e., neither `COMPONENT_MTB_HAL` nor `CY_USING_HAL` is defined), then the UART must be initialized using PDL function calls prior to being passed into the `cy_retarget_io_init()` function.  See *Quick Start (PDL Only)* section below.

### RTOS Integration
To avoid concurrent access to the UART peripheral in a RTOS environment, the ARM and IAR libraries use mutexes to control access to stdio streams. For Newlib (GCC_ARM), the mutex must be implemented in _write() and can be enabled by adding `DEFINES+=CY_RTOS_AWARE` to the Makefile. For all libraries, the program must start the RTOS kernel before calling any stdio functions.

HAL support is required for retarget-io in an RTOS environment.  If your project does not include HAL support, you must manually manage concurrency in your application.

### Quick Start (with MTB-HAL(COMPONENT_MTB_HAL) Support)
1. Configure the UART using the device configurator generated structures or through manually written config structures. Configuration includes the UART TX and RX pins, CTS/RTS pins if flow control is desired, Baud Rate and other UART config parameters
2. Set up the clock source to the UART peripheral. This could be done using the device configurator or manually. Set up the clock divider value depending on the desired baud rate.
3. Initialize the UART Hardware using PDL function calls. For example, if using a device where UART is provided by an SCB:
```
    result = Cy_SCB_UART_Init(DEBUG_UART_HW, &DEBUG_UART_config, &DEBUG_UART_context);
    // Check for correct result
```
4. Set up the HAL UART object.
```
    result = mtb_hal_uart_setup(&DEBUG_UART_hal_obj,
                                &CYBSP_DEBUG_UART_hal_config,
                                &DEBUG_UART_context, NULL);
    // Check for correct result
```
5. Enable the UART peripheral
```
    Cy_SCB_UART_Enable(DEBUG_UART_HW);
```
6. Add `#include "cy_retarget_io.h"`.
7. Call `cy_retarget_io_init(&DEBUG_UART_hal_obj);`.
8. Start printing using `printf()`.
9. If needed, register a callback to handle the transitions to deepsleep (it's automatically registered in CY-HAL case). This can be done by registering with `Cy_SysPm_RegisterCallback` either the predefined mtb_syspm_scb_uart_deepsleep_callback from the syspm-callbacks asset or a new callback function specific to the needs of the user. For more info refer to `Cy_SysPm_RegisterCallback` in PDL documentation.

### Quick Start (with CY-HAL(CY_USING_HAL) Support)
1. Add `#include "cy_retarget_io.h"`
2. Call `cy_retarget_io_init(CYBSP_DEBUG_UART_TX, CYBSP_DEBUG_UART_RX, CY_RETARGET_IO_BAUDRATE);`

    `CYBSP_DEBUG_UART_TX` and `CYBSP_DEBUG_UART_RX` pins are defined in the BSP and `CY_RETARGET_IO_BAUDRATE` is set to 115200. You can use a different baud rate if you prefer.

3. Start printing using `printf()`

### Quick Start (PDL Only)
These instructions apply when the UART interface is provided by the SCB peripheral directly using a PDL configured and initialized SCB object.  Only relevant when HAL is unavailable.  UART interfaces other than SCB are not supported at this time.

1. Add `#include "cy_retarget_io.h"`
2. Initialize and enable your UART hardware using PDL function calls.  The DEBUG_UART must be defined and configured in the Device Configurator tool.
```
    Cy_SCB_UART_Init(DEBUG_UART_HW, &DEBUG_UART_config, NULL);
    Cy_SCB_UART_Enable(DEBUG_UART_HW);
```
3. Call `cy_retarget_io_init(DEBUG_UART_HW);`
4. Start printing using `printf()`

### Enabling Conversion of '\\n' into "\r\n"
If you want to use only '\\n' instead of "\r\n" for printing a new line using printf(), define the macro `CY_RETARGET_IO_CONVERT_LF_TO_CRLF` using the *DEFINES* variable in the application Makefile. The library will then append '\\r' before '\\n' character on the output direction (STDOUT). No conversion occurs if "\r\n" is already present.

### Floating Point Support
By default, floating point support is enabled in printf. If floating point values will not be used in printed strings, this functionality can be disabled to reduece flash consumption. To disable floating support, add the following to the application makefile: `DEFINES += CY_RETARGET_IO_NO_FLOAT`.

### More information

* [API Reference Guide](https://infineon.github.io/retarget-io/html/index.html)
* [Infineon](http://www.infineon.com)
* [Infineon GitHub](https://github.com/infineon)
* [ModusToolbox Software Environment, Quick Start Guide, Documentation, and Videos](https://www.infineon.com/cms/en/design-support/tools/sdk/modustoolbox-software/)
* [PSoC™ 6 Code Examples using ModusToolbox™ IDE](https://github.com/infineon/Code-Examples-for-ModusToolbox-Software)
* [ModusToolbox™ Software](https://github.com/Infineon/modustoolbox-software)

---
© Cypress Semiconductor Corporation (an Infineon company) or an affiliate of Cypress Semiconductor Corporation, 2019-2025.
