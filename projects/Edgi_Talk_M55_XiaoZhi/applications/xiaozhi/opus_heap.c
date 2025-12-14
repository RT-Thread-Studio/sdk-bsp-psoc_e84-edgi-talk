/*
 * Copyright (c) 2006-2025, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2025-12-14     RT-Thread    First version
 */
#include "rtthread.h"
#include "rtdevice.h"
#include "os_support.h"

extern struct rt_memheap system_heap;

void *opus_heap_malloc(uint32_t size)
{
    return rt_memheap_alloc(&system_heap, size);
}

void opus_heap_free(void *p)
{
    rt_memheap_free(p);
}
