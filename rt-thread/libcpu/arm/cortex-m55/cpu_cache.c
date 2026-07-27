/*
 * Copyright (c) 2006-2023, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2018-04-02     tanek        first implementation
 * 2026-07-20     RTT          adapt cache ops for Cortex-M55
 */

#include <rtthread.h>
#include <rthw.h>
#include <rtdef.h>
#include <board.h>

#ifdef RT_USING_CACHE

#ifdef __SCB_DCACHE_LINE_SIZE
#define L1CACHE_LINESIZE_BYTE       (__SCB_DCACHE_LINE_SIZE)
#else
#define L1CACHE_LINESIZE_BYTE       (32U)
#endif

void rt_hw_cpu_icache_enable(void)
{
    SCB_EnableICache();
}

void rt_hw_cpu_icache_disable(void)
{
    SCB_DisableICache();
}

rt_base_t rt_hw_cpu_icache_status(void)
{
#if defined(__ICACHE_PRESENT) && (__ICACHE_PRESENT == 1U)
    return (SCB->CCR & SCB_CCR_IC_Msk) ? 1 : 0;
#else
    return 0;
#endif
}

void rt_hw_cpu_icache_ops(int ops, void *addr, int size)
{
    rt_uint32_t address;
    rt_int32_t size_byte;

    if ((addr == RT_NULL) || (size <= 0))
    {
        return;
    }

    address = (rt_uint32_t)addr & (rt_uint32_t)~(L1CACHE_LINESIZE_BYTE - 1U);
    size_byte = (rt_int32_t)(size + (rt_int32_t)((rt_uint32_t)addr - address));

    if (ops & RT_HW_CACHE_INVALIDATE)
    {
        __DSB();
        while (size_byte > 0)
        {
            SCB->ICIMVAU = address;
            address += L1CACHE_LINESIZE_BYTE;
            size_byte -= L1CACHE_LINESIZE_BYTE;
        }
        __DSB();
        __ISB();
    }
}

void rt_hw_cpu_dcache_enable(void)
{
    SCB_EnableDCache();
}

void rt_hw_cpu_dcache_disable(void)
{
    SCB_DisableDCache();
}

rt_base_t rt_hw_cpu_dcache_status(void)
{
#if defined(__DCACHE_PRESENT) && (__DCACHE_PRESENT == 1U)
    return (SCB->CCR & SCB_CCR_DC_Msk) ? 1 : 0;
#else
    return 0;
#endif
}

void rt_hw_cpu_dcache_ops(int ops, void *addr, int size)
{
    rt_uint32_t start_addr;
    rt_uint32_t size_byte;
    rt_uint32_t clean_invalid = RT_HW_CACHE_FLUSH | RT_HW_CACHE_INVALIDATE;

    if ((addr == RT_NULL) || (size <= 0))
    {
        return;
    }

    start_addr = (rt_uint32_t)addr & (rt_uint32_t)~(L1CACHE_LINESIZE_BYTE - 1U);
    size_byte = (rt_uint32_t)size + (rt_uint32_t)addr - start_addr;

    if ((ops & clean_invalid) == clean_invalid)
    {
        SCB_CleanInvalidateDCache_by_Addr((rt_uint32_t *)start_addr, (int32_t)size_byte);
    }
    else if (ops & RT_HW_CACHE_FLUSH)
    {
        SCB_CleanDCache_by_Addr((rt_uint32_t *)start_addr, (int32_t)size_byte);
    }
    else if (ops & RT_HW_CACHE_INVALIDATE)
    {
        SCB_InvalidateDCache_by_Addr((rt_uint32_t *)start_addr, (int32_t)size_byte);
    }
    else
    {
        RT_ASSERT(0);
    }
}

#endif /* RT_USING_CACHE */
