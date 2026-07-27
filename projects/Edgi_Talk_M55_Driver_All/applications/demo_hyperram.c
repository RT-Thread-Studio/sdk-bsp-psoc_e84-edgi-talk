/*
 * demo_hyperram.c - HyperRAM read/write and bandwidth samples
 *
 * MSH command: demo_hyperram
 *   Writes 4 fixed patterns + 1 address-incremental pattern to HyperRAM,
 *   reads back, and reports pass/fail.
 *
 * MSH command: demo_hyperram_speed [size_kb] [loops]
 *   Measures 32-bit sequential write/read/copy bandwidth. Default is
 *   1024 KB x 4 loops.
 *
 * HyperRAM auto-inits at boot via the shared drv_hyperam.c
 * (INIT_BOARD_EXPORT) when BSP_USING_HYPERAM is on.
 *
 * Kconfig: BSP_USING_HYPERAM, BSP_USING_HYPERAM_SIZE=0xC00000
 */

#include <rtthread.h>
#include <stdint.h>
#include <stdlib.h>

#define DEMO_HYPERRAM_BASE      0x64400000U
#define DEMO_HYPERRAM_MIN_KB    4U
#define DEMO_HYPERRAM_TEST_KB   1024U
#define DEMO_HYPERRAM_LOOPS     4U

#ifndef BSP_USING_HYPERAM_SIZE
#define BSP_USING_HYPERAM_SIZE    0U
#endif

#ifdef BSP_USING_HYPERAM

extern struct rt_memheap *drv_hyperam_get_memheap(void);

static rt_bool_t demo_hyperram_parse_u32(const char *text, rt_uint32_t *value)
{
    char *end = RT_NULL;
    unsigned long parsed;

    if ((text == RT_NULL) || (value == RT_NULL) || (text[0] == '-'))
    {
        return RT_FALSE;
    }

    parsed = strtoul(text, &end, 0);
    if ((end == text) || (end == RT_NULL) || (*end != '\0'))
    {
        return RT_FALSE;
    }

    *value = (rt_uint32_t)parsed;
    return RT_TRUE;
}

static rt_bool_t demo_hyperram_ptr_in_range(const void *ptr, rt_size_t size)
{
    uintptr_t addr = (uintptr_t)ptr;
    uintptr_t start = (uintptr_t)DEMO_HYPERRAM_BASE;
    uintptr_t end = start + (uintptr_t)BSP_USING_HYPERAM_SIZE;

    return (addr >= start) && ((addr + size) >= addr) && ((addr + size) <= end);
}

static void *demo_hyperram_alloc(rt_size_t size)
{
    struct rt_memheap *heap = drv_hyperam_get_memheap();
    void *ptr;

    if (heap == RT_NULL)
    {
        rt_kprintf("HyperRAM memheap is unavailable\n");
        return RT_NULL;
    }

    ptr = rt_memheap_alloc(heap, size);

    if (ptr == RT_NULL)
    {
        rt_kprintf("HyperRAM alloc failed: %u bytes\n", (unsigned)size);
        return RT_NULL;
    }

    if (!demo_hyperram_ptr_in_range(ptr, size))
    {
        rt_kprintf("Warning: buffer 0x%08X is not in HyperRAM range 0x%08X..0x%08X\n",
                   (unsigned)(uintptr_t)ptr,
                   (unsigned)DEMO_HYPERRAM_BASE,
                   (unsigned)(DEMO_HYPERRAM_BASE + BSP_USING_HYPERAM_SIZE));
    }

    return ptr;
}

static void demo_hyperram_free(void *ptr)
{
    if (ptr != RT_NULL)
    {
        rt_memheap_free(ptr);
    }
}

static void demo_hyperram_timer_start(rt_uint32_t *start)
{
    *start = (rt_uint32_t)rt_tick_get_millisecond();
}

static rt_uint32_t demo_hyperram_timer_stop(rt_uint32_t start)
{
    return (rt_uint32_t)rt_tick_get_millisecond() - start;
}

static void demo_hyperram_print_speed(const char *name,
                                        rt_uint32_t bytes,
                                        rt_uint32_t ms)
{
    rt_uint64_t bytes_per_sec;
    rt_uint32_t mbps_x100;

    if (ms == 0U)
    {
        rt_kprintf("%-7s %u bytes, <1 ms, increase size_kb or loops\n",
                   name, (unsigned)bytes);
        return;
    }

    bytes_per_sec = ((rt_uint64_t)bytes * 1000ULL) / ms;
    mbps_x100 = (rt_uint32_t)((bytes_per_sec * 100ULL) / (1024ULL * 1024ULL));

    rt_kprintf("%-7s %u bytes, %u ms, %u.%02u MB/s (%u KB/s)\n",
               name,
               (unsigned)bytes,
               (unsigned)ms,
               (unsigned)(mbps_x100 / 100U),
               (unsigned)(mbps_x100 % 100U),
               (unsigned)(bytes_per_sec / 1024ULL));
}

static int demo_hyperram(int argc, char *argv[])
{
    volatile rt_uint32_t *ram;
    const rt_uint32_t patterns[] = {
        0xAAAA5555U, 0x12345678U, 0xDEADBEEFU, 0xCAFEBABEU
    };
    rt_size_t i, p;
    rt_uint32_t expected, got;
    int errors = 0;

    RT_UNUSED(argc);
    RT_UNUSED(argv);

    ram = (volatile rt_uint32_t *)demo_hyperram_alloc(256U);
    if (ram == RT_NULL)
    {
        return -RT_ENOMEM;
    }

    /* Pattern sweep 1..4: broadcast a fixed value to the first 32 words
     * and verify each one. Catches stuck-at / addressing faults. */
    for (p = 0; p < sizeof(patterns) / sizeof(patterns[0]); p++)
    {
        for (i = 0; i < 32; i++)
        {
            ram[i] = patterns[p];
        }
        for (i = 0; i < 32; i++)
        {
            got = ram[i];
            if (got != patterns[p])
            {
                rt_kprintf("FAIL @ word %d: wrote 0x%08X, read 0x%08X\n",
                           (int)i, (unsigned)patterns[p], (unsigned)got);
                errors++;
            }
        }
    }

    /* Pattern 5: address-keyed incremental, catches row/col decode errors. */
    for (i = 0; i < 64; i++)
    {
        ram[i] = (rt_uint32_t)(0xA0000000U + i);
    }
    for (i = 0; i < 64; i++)
    {
        expected = (rt_uint32_t)(0xA0000000U + i);
        got = ram[i];
        if (got != expected)
        {
            rt_kprintf("FAIL @ word %d: wrote 0x%08X, read 0x%08X\n",
                       (int)i, (unsigned)expected, (unsigned)got);
            errors++;
        }
    }

    if (errors == 0)
    {
        rt_kprintf("HyperRAM test: PASS (96 words, 5 patterns verified)\n");
        rt_kprintf("Buffer: 0x%08X\n", (unsigned)(uintptr_t)ram);
    }
    else
    {
        rt_kprintf("HyperRAM test: FAIL (%d errors)\n", errors);
    }

    demo_hyperram_free((void *)ram);
    return errors ? -RT_ERROR : RT_EOK;
}

static int demo_hyperram_speed(int argc, char *argv[])
{
    rt_uint32_t size_kb = DEMO_HYPERRAM_TEST_KB;
    rt_uint32_t loops = DEMO_HYPERRAM_LOOPS;
    rt_uint32_t bytes;
    rt_uint32_t total_bytes;
    rt_uint32_t words;
    rt_uint32_t start;
    rt_uint32_t elapsed_ms;
    rt_uint32_t i, loop;
    volatile rt_uint32_t *src;
    volatile rt_uint32_t *dst;
    rt_uint32_t checksum = 0U;

    if ((argc > 1) && !demo_hyperram_parse_u32(argv[1], &size_kb))
    {
        rt_kprintf("Usage: demo_hyperram_speed [size_kb] [loops]\n");
        return -RT_ERROR;
    }

    if ((argc > 2) && !demo_hyperram_parse_u32(argv[2], &loops))
    {
        rt_kprintf("Usage: demo_hyperram_speed [size_kb] [loops]\n");
        return -RT_ERROR;
    }

    if ((size_kb < DEMO_HYPERRAM_MIN_KB) || (loops == 0U))
    {
        rt_kprintf("Usage: demo_hyperram_speed [size_kb >= %u] [loops > 0]\n",
                   (unsigned)DEMO_HYPERRAM_MIN_KB);
        return -RT_ERROR;
    }

    bytes = size_kb * 1024U;
    if ((bytes / 1024U) != size_kb)
    {
        rt_kprintf("Invalid size_kb: %u\n", (unsigned)size_kb);
        return -RT_ERROR;
    }

    if ((BSP_USING_HYPERAM_SIZE != 0U) && (bytes > ((rt_uint32_t)BSP_USING_HYPERAM_SIZE / 2U)))
    {
        rt_kprintf("size_kb too large, max about %u KB for src+dst buffers\n",
                   (unsigned)((rt_uint32_t)BSP_USING_HYPERAM_SIZE / 2048U));
        return -RT_ERROR;
    }

    total_bytes = bytes * loops;
    if ((loops != 0U) && ((total_bytes / loops) != bytes))
    {
        rt_kprintf("Total byte count overflow\n");
        return -RT_ERROR;
    }

    src = (volatile rt_uint32_t *)demo_hyperram_alloc(bytes * 2U);
    if (src == RT_NULL)
    {
        return -RT_ENOMEM;
    }
    dst = src + (bytes / sizeof(rt_uint32_t));
    words = bytes / sizeof(rt_uint32_t);

    for (i = 0; i < words; i++)
    {
        src[i] = 0x13572468U ^ i;
        dst[i] = 0U;
    }

    demo_hyperram_timer_start(&start);
    rt_enter_critical();
    for (loop = 0; loop < loops; loop++)
    {
        for (i = 0; i < words; i++)
        {
            dst[i] = 0xA5000000U + i + loop;
        }
    }
    elapsed_ms = demo_hyperram_timer_stop(start);
    rt_exit_critical();
    demo_hyperram_print_speed("write", total_bytes, elapsed_ms);

    demo_hyperram_timer_start(&start);
    rt_enter_critical();
    for (loop = 0; loop < loops; loop++)
    {
        for (i = 0; i < words; i++)
        {
            checksum += dst[i];
        }
    }
    elapsed_ms = demo_hyperram_timer_stop(start);
    rt_exit_critical();
    demo_hyperram_print_speed("read", total_bytes, elapsed_ms);

    demo_hyperram_timer_start(&start);
    rt_enter_critical();
    for (loop = 0; loop < loops; loop++)
    {
        for (i = 0; i < words; i++)
        {
            dst[i] = src[i];
        }
    }
    elapsed_ms = demo_hyperram_timer_stop(start);
    rt_exit_critical();
    demo_hyperram_print_speed("copy", total_bytes, elapsed_ms);

    rt_kprintf("HyperRAM speed: buffer=0x%08X, size=%u KB, loops=%u, checksum=0x%08X\n",
               (unsigned)(uintptr_t)src,
               (unsigned)size_kb,
               (unsigned)loops,
               (unsigned)checksum);

    demo_hyperram_free((void *)src);
    return RT_EOK;
}
#else
static int demo_hyperram(int argc, char *argv[])
{
    RT_UNUSED(argc);
    RT_UNUSED(argv);
    rt_kprintf("HyperRAM not enabled. Enable in menuconfig:\n");
    rt_kprintf("  BSP_USING_HYPERAM + BSP_USING_HYPERAM_SIZE=0xC00000\n");
    return -RT_ERROR;
}

static int demo_hyperram_speed(int argc, char *argv[])
{
    RT_UNUSED(argc);
    RT_UNUSED(argv);
    rt_kprintf("HyperRAM not enabled. Enable in menuconfig:\n");
    rt_kprintf("  BSP_USING_HYPERAM + BSP_USING_HYPERAM_SIZE=0xC00000\n");
    return -RT_ERROR;
}
#endif
MSH_CMD_EXPORT(demo_hyperram, hyperram read/write test);
MSH_CMD_EXPORT(demo_hyperram_speed, hyperram bandwidth test);
