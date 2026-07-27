/******************************************************************************
 * Copyright 2020-2026 The RT-Thread Development Team. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *****************************************************************************/
#include <rtdevice.h>
#include <rtthread.h>
#include <string.h>

#include "board.h"
#include "cy_ipc_pipe.h"

#include "drv_ipc.h"

#define EDGE_IPC_DEVICE_COUNT  (2U)
#define EDGE_IPC_RX_QUEUE_SIZE (128U)

#if defined(COMPONENT_CM33) || ((__CORTEX_M) == 33U)
    #define EDGE_IPC0_LOCAL_EP_ADDR   CM33_IPC_PIPE0_EP_ADDR
    #define EDGE_IPC0_PEER_EP_ADDR    CM55_IPC_PIPE0_EP_ADDR
    #define EDGE_IPC0_LOCAL_CLIENT_ID CM33_IPC_PIPE_CLIENT_ID
    #define EDGE_IPC0_PEER_CLIENT_ID  CM55_IPC_PIPE_CLIENT_ID
    #define EDGE_IPC0_TX_INTR_MASK    CY_IPC_CYPIPE_INTR_MASK_EP1
    #define EDGE_IPC0_LOCAL_CHAN      CY_IPC_CHAN_CYPIPE_EP1
    #define EDGE_IPC0_LOCAL_INTR      CY_IPC_INTR_CYPIPE_EP1
    #define EDGE_IPC0_LOCAL_PRIOR     CY_IPC_INTR_CYPIPE_PRIOR_EP1
    #define EDGE_IPC0_LOCAL_MUX       CY_IPC_INTR_CYPIPE_MUX_EP1
    #define EDGE_IPC0_PEER_CHAN       CY_IPC_CHAN_CYPIPE_EP2
    #define EDGE_IPC0_PEER_INTR       CY_IPC_INTR_CYPIPE_EP2
    #define EDGE_IPC0_PEER_PRIOR      CY_IPC_INTR_CYPIPE_PRIOR_EP2
    #define EDGE_IPC0_PEER_MUX        CY_IPC_INTR_CYPIPE_MUX_EP2

    #define EDGE_IPC1_LOCAL_EP_ADDR   CM33_IPC_PIPE1_EP_ADDR
    #define EDGE_IPC1_PEER_EP_ADDR    CM55_IPC_PIPE1_EP_ADDR
    #define EDGE_IPC1_LOCAL_CLIENT_ID CM33_IPC_PIPE_CLIENT_ID
    #define EDGE_IPC1_PEER_CLIENT_ID  CM55_IPC_PIPE_CLIENT_ID
    #define EDGE_IPC1_TX_INTR_MASK    CY_IPC_CYPIPE_INTR_MASK_EP3
    #define EDGE_IPC1_LOCAL_CHAN      CY_IPC_CHAN_CYPIPE_EP3
    #define EDGE_IPC1_LOCAL_INTR      CY_IPC_INTR_CYPIPE_EP3
    #define EDGE_IPC1_LOCAL_PRIOR     CY_IPC_INTR_CYPIPE_PRIOR_EP3
    #define EDGE_IPC1_LOCAL_MUX       CY_IPC_INTR_CYPIPE_MUX_EP3
    #define EDGE_IPC1_PEER_CHAN       CY_IPC_CHAN_CYPIPE_EP4
    #define EDGE_IPC1_PEER_INTR       CY_IPC_INTR_CYPIPE_EP4
    #define EDGE_IPC1_PEER_PRIOR      CY_IPC_INTR_CYPIPE_PRIOR_EP4
    #define EDGE_IPC1_PEER_MUX        CY_IPC_INTR_CYPIPE_MUX_EP4
#elif defined(COMPONENT_CM55) || ((__CORTEX_M) == 55U)
    #define EDGE_IPC0_LOCAL_EP_ADDR   CM55_IPC_PIPE0_EP_ADDR
    #define EDGE_IPC0_PEER_EP_ADDR    CM33_IPC_PIPE0_EP_ADDR
    #define EDGE_IPC0_LOCAL_CLIENT_ID CM55_IPC_PIPE_CLIENT_ID
    #define EDGE_IPC0_PEER_CLIENT_ID  CM33_IPC_PIPE_CLIENT_ID
    #define EDGE_IPC0_TX_INTR_MASK    CY_IPC_CYPIPE_INTR_MASK_EP2
    #define EDGE_IPC0_LOCAL_CHAN      CY_IPC_CHAN_CYPIPE_EP2
    #define EDGE_IPC0_LOCAL_INTR      CY_IPC_INTR_CYPIPE_EP2
    #define EDGE_IPC0_LOCAL_PRIOR     CY_IPC_INTR_CYPIPE_PRIOR_EP2
    #define EDGE_IPC0_LOCAL_MUX       CY_IPC_INTR_CYPIPE_MUX_EP2
    #define EDGE_IPC0_PEER_CHAN       CY_IPC_CHAN_CYPIPE_EP1
    #define EDGE_IPC0_PEER_INTR       CY_IPC_INTR_CYPIPE_EP1
    #define EDGE_IPC0_PEER_PRIOR      CY_IPC_INTR_CYPIPE_PRIOR_EP1
    #define EDGE_IPC0_PEER_MUX        CY_IPC_INTR_CYPIPE_MUX_EP1

    #define EDGE_IPC1_LOCAL_EP_ADDR   CM55_IPC_PIPE1_EP_ADDR
    #define EDGE_IPC1_PEER_EP_ADDR    CM33_IPC_PIPE1_EP_ADDR
    #define EDGE_IPC1_LOCAL_CLIENT_ID CM55_IPC_PIPE_CLIENT_ID
    #define EDGE_IPC1_PEER_CLIENT_ID  CM33_IPC_PIPE_CLIENT_ID
    #define EDGE_IPC1_TX_INTR_MASK    CY_IPC_CYPIPE_INTR_MASK_EP4
    #define EDGE_IPC1_LOCAL_CHAN      CY_IPC_CHAN_CYPIPE_EP4
    #define EDGE_IPC1_LOCAL_INTR      CY_IPC_INTR_CYPIPE_EP4
    #define EDGE_IPC1_LOCAL_PRIOR     CY_IPC_INTR_CYPIPE_PRIOR_EP4
    #define EDGE_IPC1_LOCAL_MUX       CY_IPC_INTR_CYPIPE_MUX_EP4
    #define EDGE_IPC1_PEER_CHAN       CY_IPC_CHAN_CYPIPE_EP3
    #define EDGE_IPC1_PEER_INTR       CY_IPC_INTR_CYPIPE_EP3
    #define EDGE_IPC1_PEER_PRIOR      CY_IPC_INTR_CYPIPE_PRIOR_EP3
    #define EDGE_IPC1_PEER_MUX        CY_IPC_INTR_CYPIPE_MUX_EP3
#else
    #error "Unsupported core for edge_ipc_device"
#endif

struct edge_ipc_device {
    struct rt_device parent;
    cy_ipc_pipe_callback_ptr_t cb_array[CY_IPC_CYPIPE_CLIENT_CNT];

    uint32_t local_ep_addr;
    uint32_t peer_ep_addr;
    uint32_t local_client_id;
    uint32_t peer_client_id;
    uint32_t tx_intr_mask;
    uint8_t pipe_index;

    rt_uint8_t rx_buffer[EDGE_IPC_RX_QUEUE_SIZE * sizeof(edge_rc_frame_t)];
    struct rt_ringbuffer rx_rb;

    volatile rt_uint32_t tx_pool_idx;
    volatile rt_uint32_t stats_tx_ok;
    volatile rt_uint32_t stats_tx_err;
    volatile rt_uint32_t stats_rx_ok;
    volatile rt_uint32_t stats_rx_err;
    volatile rt_uint32_t stats_rx_drop;
    volatile rt_uint32_t stats_sema_fail;
    volatile rt_uint32_t stats_tx_busy;
    volatile rt_uint32_t stats_tx_retry;
    volatile rt_uint32_t stats_tx_timeout;
    volatile rt_uint32_t stats_tx_release;

    rt_bool_t initialized;
};

static struct edge_ipc_device g_edge_ipc_dev[EDGE_IPC_DEVICE_COUNT];
static rt_bool_t g_edge_ipc_registered;
static rt_bool_t g_edge_ipc_pipe_configured;
static cy_stc_ipc_pipe_ep_t g_edge_ipc_pipe_ep_array[CY_IPC_MAX_ENDPOINTS];
CY_SECTION_SHAREDMEM static edge_rc_frame_t g_edge_ipc_tx_pool[EDGE_IPC_DEVICE_COUNT][EDGE_IPC_FRAME_POOL_SIZE];

static void edge_ipc0_pipe_isr(void);
static void edge_ipc1_pipe_isr(void);

static cy_stc_ipc_pipe_config_t g_edge_ipc0_config = {
    { .ipcNotifierNumber = EDGE_IPC0_LOCAL_INTR,
      .ipcNotifierPriority = EDGE_IPC0_LOCAL_PRIOR,
      .ipcNotifierMuxNumber = EDGE_IPC0_LOCAL_MUX,
      .epAddress = EDGE_IPC0_LOCAL_EP_ADDR,
      { .epChannel = EDGE_IPC0_LOCAL_CHAN,
        .epIntr = EDGE_IPC0_LOCAL_INTR,
        .epIntrmask = CY_IPC_CYPIPE0_INTR_MASK } },
    { .ipcNotifierNumber = EDGE_IPC0_PEER_INTR,
      .ipcNotifierPriority = EDGE_IPC0_PEER_PRIOR,
      .ipcNotifierMuxNumber = EDGE_IPC0_PEER_MUX,
      .epAddress = EDGE_IPC0_PEER_EP_ADDR,
      { .epChannel = EDGE_IPC0_PEER_CHAN,
        .epIntr = EDGE_IPC0_PEER_INTR,
        .epIntrmask = CY_IPC_CYPIPE0_INTR_MASK } },
    .endpointClientsCount = CY_IPC_CYPIPE_CLIENT_CNT,
    .endpointsCallbacksArray = RT_NULL,
    .userPipeIsrHandler = &edge_ipc0_pipe_isr
};

static cy_stc_ipc_pipe_config_t g_edge_ipc1_config = {
    { .ipcNotifierNumber = EDGE_IPC1_LOCAL_INTR,
      .ipcNotifierPriority = EDGE_IPC1_LOCAL_PRIOR,
      .ipcNotifierMuxNumber = EDGE_IPC1_LOCAL_MUX,
      .epAddress = EDGE_IPC1_LOCAL_EP_ADDR,
      { .epChannel = EDGE_IPC1_LOCAL_CHAN,
        .epIntr = EDGE_IPC1_LOCAL_INTR,
        .epIntrmask = CY_IPC_CYPIPE1_INTR_MASK } },
    { .ipcNotifierNumber = EDGE_IPC1_PEER_INTR,
      .ipcNotifierPriority = EDGE_IPC1_PEER_PRIOR,
      .ipcNotifierMuxNumber = EDGE_IPC1_PEER_MUX,
      .epAddress = EDGE_IPC1_PEER_EP_ADDR,
      { .epChannel = EDGE_IPC1_PEER_CHAN,
        .epIntr = EDGE_IPC1_PEER_INTR,
        .epIntrmask = CY_IPC_CYPIPE1_INTR_MASK } },
    .endpointClientsCount = CY_IPC_CYPIPE_CLIENT_CNT,
    .endpointsCallbacksArray = RT_NULL,
    .userPipeIsrHandler = &edge_ipc1_pipe_isr
};

static rt_bool_t edge_ipc_lock_sema(struct edge_ipc_device* dev)
{
    uint32_t retry = 0;

    while (retry++ < EDGE_IPC_SEMA_RETRY_MAX) {
        if (Cy_IPC_Sema_Set(IPC_DEMO_SEMA_NUM, false) == CY_IPC_SEMA_SUCCESS) {
            return RT_TRUE;
        }
    }

    dev->stats_sema_fail++;
    return RT_FALSE;
}

static void edge_ipc_unlock_sema(void)
{
    (void)Cy_IPC_Sema_Clear(IPC_DEMO_SEMA_NUM, false);
}

static void edge_ipc_rx_callback_common(struct edge_ipc_device* dev, uint32_t* msg_data)
{
    edge_rc_frame_t* rx = (edge_rc_frame_t*)msg_data;

    if (rx == RT_NULL) {
        dev->stats_rx_err++;
        return;
    }

    if (rx->client_id != dev->local_client_id
        || rx->magic != RC_MAGIC_WORD
        || edge_rc_checksum(rx) != rx->checksum) {
        dev->stats_rx_err++;
        return;
    }

    if (rt_ringbuffer_space_len(&dev->rx_rb) < sizeof(edge_rc_frame_t)
        || rt_ringbuffer_put(&dev->rx_rb, (const rt_uint8_t*)rx, sizeof(edge_rc_frame_t)) != sizeof(edge_rc_frame_t)) {
        dev->stats_rx_drop++;
        return;
    }

    dev->stats_rx_ok++;

    if (dev->parent.rx_indicate) {
        dev->parent.rx_indicate(&dev->parent, 1);
    }
}

static void edge_ipc0_rx_callback(uint32_t* msg_data)
{
    edge_ipc_rx_callback_common(&g_edge_ipc_dev[0], msg_data);
}

static void edge_ipc1_rx_callback(uint32_t* msg_data)
{
    edge_ipc_rx_callback_common(&g_edge_ipc_dev[1], msg_data);
}

static void edge_ipc_tx_release_common(struct edge_ipc_device* dev)
{
    dev->stats_tx_release++;

    if (dev->parent.tx_complete) {
        dev->parent.tx_complete(&dev->parent, RT_NULL);
    }
}

static void edge_ipc0_tx_release_callback(void)
{
    edge_ipc_tx_release_common(&g_edge_ipc_dev[0]);
}

static void edge_ipc1_tx_release_callback(void)
{
    edge_ipc_tx_release_common(&g_edge_ipc_dev[1]);
}

static void edge_ipc0_pipe_isr(void)
{
    Cy_IPC_Pipe_ExecuteCallback(EDGE_IPC0_LOCAL_EP_ADDR);
}

static void edge_ipc1_pipe_isr(void)
{
    Cy_IPC_Pipe_ExecuteCallback(EDGE_IPC1_LOCAL_EP_ADDR);
}

static edge_rc_frame_t* edge_ipc_alloc_tx_frame(struct edge_ipc_device* dev)
{
    rt_base_t level;
    rt_uint32_t slot;

    level = rt_hw_interrupt_disable();
    slot = dev->tx_pool_idx++ % EDGE_IPC_FRAME_POOL_SIZE;
    rt_hw_interrupt_enable(level);

    return &g_edge_ipc_tx_pool[dev->pipe_index][slot];
}

static cy_ipc_pipe_relcallback_ptr_t edge_ipc_get_release_callback(struct edge_ipc_device* dev)
{
    return (dev->pipe_index == 0U) ? edge_ipc0_tx_release_callback : edge_ipc1_tx_release_callback;
}

static cy_en_ipc_pipe_status_t edge_ipc_send_once(struct edge_ipc_device* dev, edge_rc_frame_t* tx)
{
    cy_en_ipc_pipe_status_t status;

    if (!edge_ipc_lock_sema(dev)) {
        return CY_IPC_PIPE_ERROR_SEND_BUSY;
    }

    status = Cy_IPC_Pipe_SendMessage(dev->peer_ep_addr,
                                     dev->local_ep_addr,
                                     (void*)tx,
                                     edge_ipc_get_release_callback(dev));
    edge_ipc_unlock_sema();

    return status;
}

static cy_en_ipc_pipe_status_t edge_ipc_send_with_retry(struct edge_ipc_device* dev, edge_rc_frame_t* tx)
{
    cy_en_ipc_pipe_status_t status;
    uint32_t retry = 0;

    status = edge_ipc_send_once(dev, tx);
    if (status == CY_IPC_PIPE_ERROR_SEND_BUSY) {
        dev->stats_tx_busy++;
    }

    while (status == CY_IPC_PIPE_ERROR_SEND_BUSY && retry < EDGE_IPC_SEND_BUSY_RETRY_MAX) {
        retry++;
        dev->stats_tx_retry++;
        rt_thread_mdelay(EDGE_IPC_SEND_BUSY_DELAY_MS);
        status = edge_ipc_send_once(dev, tx);
    }

    if (status == CY_IPC_PIPE_ERROR_SEND_BUSY) {
        dev->stats_tx_timeout++;
    }

    return status;
}

static rt_err_t edge_ipc_hw_init(struct edge_ipc_device* dev, cy_stc_ipc_pipe_config_t* config)
{
    cy_en_ipc_pipe_status_t status;
    cy_ipc_pipe_callback_ptr_t rx_callback;

    if (dev == RT_NULL || config == RT_NULL || dev->pipe_index >= EDGE_IPC_DEVICE_COUNT) {
        return -RT_ERROR;
    }

    if (!g_edge_ipc_pipe_configured) {
        Cy_IPC_Pipe_Config(g_edge_ipc_pipe_ep_array);
        g_edge_ipc_pipe_configured = RT_TRUE;
    }

    config->endpointsCallbacksArray = dev->cb_array;
    Cy_IPC_Pipe_Init(config);

    rx_callback = (dev->pipe_index == 0U) ? edge_ipc0_rx_callback : edge_ipc1_rx_callback;
    status = Cy_IPC_Pipe_RegisterCallback(dev->local_ep_addr,
                                          rx_callback,
                                          dev->local_client_id);
    if (status != CY_IPC_PIPE_SUCCESS) {
        return -RT_ERROR;
    }

    dev->initialized = RT_TRUE;
    return RT_EOK;
}

static rt_err_t edge_ipc_dev_init(rt_device_t rt_dev)
{
    struct edge_ipc_device* dev = (struct edge_ipc_device*)rt_dev;

    if (dev->initialized) {
        return RT_EOK;
    }

    switch (dev->pipe_index) {
    case 0:
        return edge_ipc_hw_init(dev, &g_edge_ipc0_config);
    case 1:
        return edge_ipc_hw_init(dev, &g_edge_ipc1_config);
    default:
        return -RT_ERROR;
    }
}

static rt_err_t edge_ipc_dev_open(rt_device_t rt_dev, rt_uint16_t oflag)
{
    (void)rt_dev;
    (void)oflag;
    return RT_EOK;
}

static rt_err_t edge_ipc_dev_close(rt_device_t rt_dev)
{
    (void)rt_dev;
    return RT_EOK;
}

static rt_ssize_t edge_ipc_dev_read(rt_device_t rt_dev, rt_off_t pos, void* buffer, rt_size_t size)
{
    struct edge_ipc_device* dev = (struct edge_ipc_device*)rt_dev;
    edge_rc_frame_t* frame = (edge_rc_frame_t*)buffer;
    rt_size_t read_cnt = 0;

    (void)pos;

    if (buffer == RT_NULL || size == 0) {
        return 0;
    }

    while (read_cnt < size) {
        if (rt_ringbuffer_get(&dev->rx_rb, (rt_uint8_t*)&frame[read_cnt], sizeof(edge_rc_frame_t)) != sizeof(edge_rc_frame_t)) {
            break;
        }
        read_cnt++;
    }

    return (rt_ssize_t)read_cnt;
}

static rt_ssize_t edge_ipc_dev_write(rt_device_t rt_dev, rt_off_t pos, const void* buffer, rt_size_t size)
{
    struct edge_ipc_device* dev = (struct edge_ipc_device*)rt_dev;
    const edge_rc_frame_t* frame = (const edge_rc_frame_t*)buffer;
    rt_size_t write_cnt = 0;
    cy_en_ipc_pipe_status_t status;

    (void)pos;

    if (buffer == RT_NULL || size == 0) {
        return 0;
    }

    while (write_cnt < size) {
        edge_rc_frame_t* tx = edge_ipc_alloc_tx_frame(dev);

        *tx = frame[write_cnt];
        tx->client_id = (uint8_t)dev->peer_client_id;
        tx->intr_mask = (rt_uint16_t)dev->tx_intr_mask;
#if defined(COMPONENT_CM33) || ((__CORTEX_M) == 33U)
        tx->role = RC_ROLE_M33;
#elif defined(COMPONENT_CM55) || ((__CORTEX_M) == 55U)
        tx->role = RC_ROLE_M55_ECHO;
#endif
        tx->magic = RC_MAGIC_WORD;
        tx->checksum = edge_rc_checksum(tx);

        status = edge_ipc_send_with_retry(dev, tx);
        if (status != CY_IPC_PIPE_SUCCESS) {
            dev->stats_tx_err++;
            break;
        }

        dev->stats_tx_ok++;
        write_cnt++;
    }

    return (rt_ssize_t)write_cnt;
}

static rt_err_t edge_ipc_dev_control(rt_device_t rt_dev, int cmd, void* args)
{
    struct edge_ipc_device* dev = (struct edge_ipc_device*)rt_dev;

    if (cmd == EDGE_IPC_CTRL_GET_STATS) {
        edge_ipc_device_stats_t* stats = (edge_ipc_device_stats_t*)args;

        if (stats == RT_NULL) {
            return -RT_EINVAL;
        }

        stats->tx_ok = dev->stats_tx_ok;
        stats->tx_err = dev->stats_tx_err;
        stats->rx_ok = dev->stats_rx_ok;
        stats->rx_err = dev->stats_rx_err;
        stats->rx_drop = dev->stats_rx_drop;
        stats->sema_fail = dev->stats_sema_fail;
        stats->tx_busy = dev->stats_tx_busy;
        stats->tx_retry = dev->stats_tx_retry;
        stats->tx_timeout = dev->stats_tx_timeout;
        stats->tx_release = dev->stats_tx_release;

        return RT_EOK;
    }

    if (cmd == EDGE_IPC_CTRL_GET_RINGBUFFER) {
        if (args == RT_NULL) {
            return -RT_EINVAL;
        }

        *(struct rt_ringbuffer**)args = &dev->rx_rb;
        return RT_EOK;
    }

    return -RT_ENOSYS;
}

#ifdef RT_USING_DEVICE_OPS
static const struct rt_device_ops edge_ipc_dev_ops = {
    edge_ipc_dev_init,
    edge_ipc_dev_open,
    edge_ipc_dev_close,
    edge_ipc_dev_read,
    edge_ipc_dev_write,
    edge_ipc_dev_control
};
#endif

rt_device_t edge_ipc_device_find(const char* name)
{
    return rt_device_find((name != RT_NULL) ? name : EDGE_IPC_DEVICE_NAME);
}

static void edge_ipc_device_setup(struct edge_ipc_device* dev,
                                  uint8_t pipe_index,
                                  uint32_t local_ep_addr,
                                  uint32_t peer_ep_addr,
                                  uint32_t local_client_id,
                                  uint32_t peer_client_id,
                                  uint32_t tx_intr_mask)
{
    rt_memset(dev, 0, sizeof(*dev));

    dev->pipe_index = pipe_index;
    dev->local_ep_addr = local_ep_addr;
    dev->peer_ep_addr = peer_ep_addr;
    dev->local_client_id = local_client_id;
    dev->peer_client_id = peer_client_id;
    dev->tx_intr_mask = tx_intr_mask;

    rt_ringbuffer_init(&dev->rx_rb, dev->rx_buffer, sizeof(dev->rx_buffer));

#ifdef RT_USING_DEVICE_OPS
    dev->parent.ops = &edge_ipc_dev_ops;
#else
    dev->parent.init = edge_ipc_dev_init;
    dev->parent.open = edge_ipc_dev_open;
    dev->parent.close = edge_ipc_dev_close;
    dev->parent.read = edge_ipc_dev_read;
    dev->parent.write = edge_ipc_dev_write;
    dev->parent.control = edge_ipc_dev_control;
#endif
}

int edge_ipc_device_register(void)
{
    rt_err_t result;

    if (g_edge_ipc_registered) {
        return RT_EOK;
    }

    edge_ipc_device_setup(&g_edge_ipc_dev[0],
                          0U,
                          EDGE_IPC0_LOCAL_EP_ADDR,
                          EDGE_IPC0_PEER_EP_ADDR,
                          EDGE_IPC0_LOCAL_CLIENT_ID,
                          EDGE_IPC0_PEER_CLIENT_ID,
                          EDGE_IPC0_TX_INTR_MASK);
    result = rt_device_register(&g_edge_ipc_dev[0].parent,
                                EDGE_IPC0_DEVICE_NAME,
                                RT_DEVICE_FLAG_RDWR | RT_DEVICE_FLAG_INT_RX);
    if (result != RT_EOK) {
        return result;
    }

    edge_ipc_device_setup(&g_edge_ipc_dev[1],
                          1U,
                          EDGE_IPC1_LOCAL_EP_ADDR,
                          EDGE_IPC1_PEER_EP_ADDR,
                          EDGE_IPC1_LOCAL_CLIENT_ID,
                          EDGE_IPC1_PEER_CLIENT_ID,
                          EDGE_IPC1_TX_INTR_MASK);
    result = rt_device_register(&g_edge_ipc_dev[1].parent,
                                EDGE_IPC1_DEVICE_NAME,
                                RT_DEVICE_FLAG_RDWR | RT_DEVICE_FLAG_INT_RX);
    if (result != RT_EOK) {
        return result;
    }

    g_edge_ipc_registered = RT_TRUE;
    return RT_EOK;
}
INIT_PREV_EXPORT(edge_ipc_device_register);
