/*
 * Copyright (c) 2006-2026, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2026-07-20     RTT          add Cortex-M55 architecture name
 */

#include "cpuport.h"
#include <rtthread.h>
#include <board.h>

#ifdef RT_USING_CACHE
rt_base_t rt_hw_cpu_icache_status(void);
rt_base_t rt_hw_cpu_dcache_status(void);
#endif

const char *rt_hw_cpu_arch(void)
{
#if defined(__ARM_FEATURE_MVE) && (__ARM_FEATURE_MVE != 0)
    return "armv8.1-m.main/cortex-m55+fp+mve";
#elif defined(__VFP_FP__) && !defined(__SOFTFP__)
    return "armv8.1-m.main/cortex-m55+fp";
#else
    return "armv8.1-m.main/cortex-m55";
#endif
}

#ifdef RT_USING_FINSH
static void m55_cpu_test(void)
{
    rt_kprintf("arch: %s\n", rt_hw_cpu_arch());

#if defined(__ARM_FEATURE_MVE)
    rt_kprintf("mve: %d\n", __ARM_FEATURE_MVE);
#else
    rt_kprintf("mve: 0\n");
#endif

#if defined(__VFP_FP__) && !defined(__SOFTFP__)
    rt_kprintf("fpu: hard\n");
#else
    rt_kprintf("fpu: soft/none\n");
#endif

#if defined(__ARM_FEATURE_DSP)
    rt_kprintf("dsp: %d\n", __ARM_FEATURE_DSP);
#else
    rt_kprintf("dsp: 0\n");
#endif

#if defined(__ICACHE_PRESENT)
    rt_kprintf("icache present: %d\n", __ICACHE_PRESENT);
#else
    rt_kprintf("icache present: unknown\n");
#endif

#if defined(__DCACHE_PRESENT)
    rt_kprintf("dcache present: %d\n", __DCACHE_PRESENT);
#else
    rt_kprintf("dcache present: unknown\n");
#endif

#ifdef RT_USING_CACHE
    rt_kprintf("icache enabled: %d\n", rt_hw_cpu_icache_status());
    rt_kprintf("dcache enabled: %d\n", rt_hw_cpu_dcache_status());
#else
    rt_kprintf("rt cache api: disabled\n");
#endif

    rt_hw_dsb();
    rt_hw_dmb();
    rt_hw_isb();
    rt_kprintf("barrier: ok\n");
}
MSH_CMD_EXPORT(m55_cpu_test, test Cortex-M55 libcpu hooks);
#endif
