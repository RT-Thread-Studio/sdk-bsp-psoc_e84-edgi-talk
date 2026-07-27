/*
 * Copyright (c) 2006-2026, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2026-07-20     RTT          add Cortex-M55 CPU barrier helpers
 */

#ifndef CORTEX_M55_CPUPORT_H__
#define CORTEX_M55_CPUPORT_H__

#if defined(__GNUC__) || defined(__clang__)
#undef rt_hw_isb
#undef rt_hw_dmb
#undef rt_hw_dsb
#define rt_hw_isb()     __asm volatile ("isb sy" ::: "memory")
#define rt_hw_dmb()     __asm volatile ("dmb sy" ::: "memory")
#define rt_hw_dsb()     __asm volatile ("dsb sy" ::: "memory")
#endif

#ifdef RT_USING_SMP
typedef union {
    unsigned long slock;
    struct __arch_tickets {
        unsigned short owner;
        unsigned short next;
    } tickets;
} rt_hw_spinlock_t;
#endif

#endif /* CORTEX_M55_CPUPORT_H__ */
