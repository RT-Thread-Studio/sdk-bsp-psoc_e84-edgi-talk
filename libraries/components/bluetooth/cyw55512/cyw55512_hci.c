/* SPDX-License-Identifier: Apache-2.0 */
#include "cyw55512_hci.h"
#include "board.h"
#include "cycfg_peripherals.h"
#include "cycfg_pins.h"
#include <limits.h>

#define RADIO_RAIL_ENABLE       GET_PIN(16, 3)
#define HCI_UART                CYBSP_BT_UART_HW
#define H4_COMMAND              0x01
#define H4_EVENT                0x04
#define HCI_COMMAND_COMPLETE    0x0e
#define HCI_RESET               0x0c03
#define HCI_READ_LOCAL_VERSION  0x1001
#define RUNTIME_BAUD            115200U
#define DOWNLOAD_BAUD           3000000U
#define COMMAND_TIMEOUT_MS      2000

static struct rt_semaphore rx_ready;
static const cy_stc_sysint_t uart_irq = { .intrSrc = CYBSP_BT_UART_IRQ, .intrPriority = 5 };

static void uart_rx_irq(void)
{
    rt_interrupt_enter();
    if (Cy_SCB_GetRxInterruptStatusMasked(HCI_UART) & CY_SCB_RX_INTR_NOT_EMPTY)
    {
        /* The reader drains the FIFO before acknowledging this source. */
        Cy_SCB_SetRxInterruptMask(HCI_UART, 0U);
        rt_sem_release(&rx_ready);
    }
    rt_interrupt_leave();
}

static int read_uart(uint8_t *buf, size_t len, rt_int32_t timeout)
{
    rt_tick_t start = rt_tick_get();
    for (;;)
    {
        uint32_t error = Cy_SCB_UART_GetRxFifoStatus(HCI_UART) &
                         (CY_SCB_UART_RX_OVERFLOW | CY_SCB_UART_RX_ERR_FRAME);
        if (error)
        {
            Cy_SCB_UART_ClearRxFifoStatus(HCI_UART, error);
            return -RT_EIO;
        }
        uint32_t count = Cy_SCB_UART_GetArray(HCI_UART, buf, len);
        if (count)
            return (int)count;

        rt_int32_t remaining = timeout;
        if (timeout != RT_WAITING_FOREVER)
        {
            rt_tick_t elapsed = rt_tick_get() - start;
            if (elapsed >= (rt_tick_t)timeout)
                return -RT_ETIMEOUT;
            remaining -= (rt_int32_t)elapsed;
        }

        /* NOT_EMPTY is latched. After draining the FIFO, clear the old request
         * before rearming. The SCB keeps it asserted if a new byte has arrived. */
        Cy_SCB_ClearRxInterrupt(HCI_UART, CY_SCB_RX_INTR_NOT_EMPTY);
        Cy_SCB_SetRxInterruptMask(HCI_UART, CY_SCB_RX_INTR_NOT_EMPTY);
        if (rt_sem_take(&rx_ready, remaining) != RT_EOK)
        {
            Cy_SCB_SetRxInterruptMask(HCI_UART, 0U);
            return -RT_ETIMEOUT;
        }
    }
}

int cyw55512_hci_read(uint8_t *buf, size_t len)
{
    if (!buf || !len || len > INT_MAX)
        return -RT_EINVAL;
    return read_uart(buf, len, RT_WAITING_FOREVER);
}

static int write_uart(const uint8_t *buf, size_t len, bool fast_download)
{
    if (!buf || len > INT_MAX)
        return -RT_EINVAL;
    size_t sent = 0;
    rt_tick_t last_progress = rt_tick_get();
    while (sent < len)
    {
        uint32_t count = Cy_SCB_UART_PutArray(HCI_UART, (void *)(buf + sent), len - sent);
        if (count)
        {
            sent += count;
            last_progress = rt_tick_get();
        }
        else if (rt_tick_get() - last_progress >= rt_tick_from_millisecond(COMMAND_TIMEOUT_MS))
            return -RT_ETIMEOUT;
        else if (fast_download && rt_tick_get() == last_progress)
        {
            /* At 3 Mbaud the FIFO frees several slots in tens of microseconds.
             * Yield a tick if CTS stalls rather than polling for two seconds. */
            Cy_SysLib_DelayUs(32);
        }
        else
            rt_thread_delay(1);
    }
    return (int)sent;
}

int cyw55512_hci_write(const uint8_t *buf, size_t len)
{
    return write_uart(buf, len, false);
}

static int read_exact(uint8_t *buf, size_t len, rt_tick_t start)
{
    rt_tick_t timeout = rt_tick_from_millisecond(COMMAND_TIMEOUT_MS);
    while (len)
    {
        rt_tick_t elapsed = rt_tick_get() - start;
        if (elapsed >= timeout)
            return -RT_ETIMEOUT;
        int n = read_uart(buf, len, timeout - elapsed);
        if (n < 0)
            return n;
        buf += n;
        len -= n;
    }
    return RT_EOK;
}

static int send_command(uint16_t opcode, const uint8_t *params, uint8_t length, bool fast_download)
{
    uint8_t header[] = { H4_COMMAND, (uint8_t)opcode, (uint8_t)(opcode >> 8), length };
    int rc = cyw55512_hci_write(header, sizeof(header));
    if (rc >= 0 && length)
        rc = write_uart(params, length, fast_download);
    if (rc < 0)
        return rc;

    rt_tick_t start = rt_tick_get();
    for (;;)
    {
        uint8_t event[3], payload[255];
        rc = read_exact(event, sizeof(event), start);
        if (rc < 0 || event[0] != H4_EVENT)
            return rc < 0 ? rc : -RT_EIO;
        rc = read_exact(payload, event[2], start);
        if (rc < 0)
            return rc;
        if (event[1] == HCI_COMMAND_COMPLETE && event[2] >= 4 &&
            payload[1] == header[1] && payload[2] == header[2])
        {
            if (payload[3])
                rt_kprintf("CYW55512 opcode 0x%04x status 0x%02x\n", opcode, payload[3]);
            return payload[3] ? -RT_EIO : RT_EOK;
        }
    }
}

static int power_controller(void)
{
    /* Hold controller CTS low across BT_REG_ON to select CYW55500A1 autobaud. */
    Cy_GPIO_Pin_FastInit(CYBSP_BT_UART_RTS_PORT, CYBSP_BT_UART_RTS_PIN,
                         CY_GPIO_DM_STRONG_IN_OFF, 0, HSIOM_SEL_GPIO);
    Cy_GPIO_Pin_FastInit(CYBSP_BT_POWER_PORT, CYBSP_BT_POWER_PIN,
                         CY_GPIO_DM_STRONG_IN_OFF, 0, HSIOM_SEL_GPIO);
    /* The baseboard switches both shared radio supplies with P16.3. */
    rt_pin_mode(RADIO_RAIL_ENABLE, PIN_MODE_OUTPUT);
    rt_pin_write(RADIO_RAIL_ENABLE, PIN_HIGH);
    rt_thread_mdelay(200);
    Cy_GPIO_Pin_FastInit(CYBSP_BT_DEVICE_WAKE_PORT, CYBSP_BT_DEVICE_WAKE_PIN,
                         CY_GPIO_DM_STRONG_IN_OFF, 0, HSIOM_SEL_GPIO);
    Cy_GPIO_Write(CYBSP_BT_POWER_PORT, CYBSP_BT_POWER_PIN, 1);
    rt_thread_mdelay(200);
    Cy_GPIO_Pin_Init(CYBSP_BT_UART_RTS_PORT, CYBSP_BT_UART_RTS_PIN, &CYBSP_BT_UART_RTS_config);
    rt_tick_t start = rt_tick_get();
    while (Cy_GPIO_Read(CYBSP_BT_UART_CTS_PORT, CYBSP_BT_UART_CTS_PIN))
    {
        if (rt_tick_get() - start >= rt_tick_from_millisecond(COMMAND_TIMEOUT_MS))
        {
            rt_kprintf("CYW55512 controller did not assert CTS on P10.2\n");
            return -RT_ETIMEOUT;
        }
        rt_thread_mdelay(10);
    }
    return RT_EOK;
}

/* The build-time importer validates record lengths and the final Launch RAM. */
#if !__has_include("cyw55512_hcd.inc")
#error "Prepare the controller firmware first: python libraries/components/bluetooth/cyw55512/prepare_firmware.py"
#endif
static const uint8_t controller_image[] = {
#include "cyw55512_hcd.inc"
};

static int download_image(bool fast_download)
{
    size_t offset = 0;
    while (offset < sizeof(controller_image))
    {
        const uint8_t *record = controller_image + offset;
        uint16_t opcode = record[0] | (uint16_t)(record[1] << 8);
        int rc = send_command(opcode, record + 3, record[2], fast_download);
        if (rc < 0)
        {
            rt_kprintf("CYW55512 HCD offset %u, opcode 0x%04x failed\n", (unsigned)offset, opcode);
            return rc;
        }
        offset += 3U + record[2];
    }
    return RT_EOK;
}

static int boot_controller(mtb_hal_uart_t *uart, uint32_t download_baud)
{
    /* A retry restarts the ROM and the HCD at record zero. The radio supply
     * is shared with Wi-Fi; only BT_REG_ON is reset inside power_controller. */
    Cy_SCB_SetRxInterruptMask(HCI_UART, 0U);
    Cy_SCB_UART_Disable(HCI_UART, NULL);
    Cy_SCB_UART_ClearTxFifo(HCI_UART);
    Cy_SCB_UART_ClearRxFifo(HCI_UART);
    Cy_SCB_UART_Enable(HCI_UART);
    int rc = power_controller();
    if (rc != RT_EOK)
        return rc;
    Cy_SCB_UART_ClearRxFifo(HCI_UART);
    Cy_SCB_ClearRxInterrupt(HCI_UART, CY_SCB_RX_INTR_NOT_EMPTY |
                            CY_SCB_UART_RX_OVERFLOW | CY_SCB_UART_RX_ERR_FRAME);
    NVIC_ClearPendingIRQ(uart_irq.intrSrc);
    rt_sem_control(&rx_ready, RT_IPC_CMD_RESET, RT_NULL);

    /* The generated HAL config targets SCB4's dedicated fractional divider.
     * The ROM measures the first Reset at the download baud after sampling RTS low. */
    uint32_t actual_baud = 0;
    if (mtb_hal_uart_set_baud(uart, download_baud, &actual_baud) != CY_RSLT_SUCCESS)
        return -RT_EIO;
    bool fast = download_baud != RUNTIME_BAUD;
    rc = send_command(HCI_RESET, NULL, 0, false);
    if (rc != RT_EOK)
        return rc;
    rc = download_image(fast);
    if (rc != RT_EOK)
        return rc;

    /* Launch RAM's Command Complete arrives at the download speed.
     * The launched firmware resets its UART to 115200. */
    if (fast && mtb_hal_uart_set_baud(uart, RUNTIME_BAUD, &actual_baud) != CY_RSLT_SUCCESS)
        return -RT_EIO;
    rt_thread_mdelay(250);
    rc = send_command(HCI_READ_LOCAL_VERSION, NULL, 0, false);
    if (rc == RT_EOK)
        rt_kprintf("CYW55512 HCI ready at %u baud\n", (unsigned)actual_baud);
    return rc;
}

int cyw55512_hci_open(void)
{
    mtb_hal_uart_t uart;
    cy_stc_scb_uart_context_t context;
    int rc = rt_sem_init(&rx_ready, "bt_rx", 0, RT_IPC_FLAG_FIFO);
    if (rc != RT_EOK)
        return rc;
    if (Cy_SCB_UART_Init(HCI_UART, &CYBSP_BT_UART_config, &context) != CY_SCB_UART_SUCCESS ||
        mtb_hal_uart_setup(&uart, &CYBSP_BT_UART_hal_config, &context, NULL) != CY_RSLT_SUCCESS ||
        Cy_SysInt_Init(&uart_irq, uart_rx_irq) != CY_SYSINT_SUCCESS)
    {
        rt_sem_detach(&rx_ready);
        return -RT_ERROR;
    }
    /* Leave room for bytes already on the wire when the SCB raises RTS. */
    Cy_SCB_UART_SetRtsFifoLevel(HCI_UART, 32);
    Cy_SCB_UART_Enable(HCI_UART);
    NVIC_ClearPendingIRQ(uart_irq.intrSrc);
    NVIC_EnableIRQ(uart_irq.intrSrc);
    rc = boot_controller(&uart, DOWNLOAD_BAUD);
    if (rc != RT_EOK)
    {
        rt_kprintf("CYW55512 fast download failed (%d), restarting at 115200 baud\n", rc);
        rc = boot_controller(&uart, RUNTIME_BAUD);
    }
    if (rc == RT_EOK)
        return RT_EOK;
    rt_kprintf("CYW55512 startup failed (%d)\n", rc);
    Cy_GPIO_Write(CYBSP_BT_POWER_PORT, CYBSP_BT_POWER_PIN, 0);
    NVIC_DisableIRQ(uart_irq.intrSrc);
    Cy_SCB_UART_Disable(HCI_UART, NULL);
    rt_sem_detach(&rx_ready);
    return rc;
}

const struct bluetooth_hci_uart cyw55512_hci_uart = {
    .baudrate = RUNTIME_BAUD,
    .flow_control = true,
    .open = cyw55512_hci_open,
    .read = cyw55512_hci_read,
    .write = cyw55512_hci_write,
};
