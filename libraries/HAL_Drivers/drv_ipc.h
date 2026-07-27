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
#ifndef DRV_IPC_H
#define DRV_IPC_H

#include "ipc_common.h"
#include <rtthread.h>

#ifdef __cplusplus
extern "C" {
#endif

#define EDGE_IPC0_DEVICE_NAME        "ipc1"
#define EDGE_IPC1_DEVICE_NAME        "ipc0"
#define EDGE_IPC_DEVICE_NAME         EDGE_IPC1_DEVICE_NAME

#define EDGE_IPC_CTRL_GET_STATS      (0x1000)
#define EDGE_IPC_CTRL_GET_RINGBUFFER (0x1002)

typedef struct
{
    rt_uint32_t tx_ok;
    rt_uint32_t tx_err;
    rt_uint32_t rx_ok;
    rt_uint32_t rx_err;
    rt_uint32_t rx_drop;
    rt_uint32_t sema_fail;
    rt_uint32_t tx_busy;
    rt_uint32_t tx_retry;
    rt_uint32_t tx_timeout;
    rt_uint32_t tx_release;
} edge_ipc_device_stats_t;

rt_device_t edge_ipc_device_find(const char* name);
int edge_ipc_device_register(void);

#ifdef __cplusplus
}
#endif

#endif
