/*
 * demo_fs_benchmark.c - RT-Thread DFS filesystem benchmark
 *
 * MSH command:
 *   demo_fs_benchmark [file] [total_kb] [block_kb] [random_ops]
 *
 * Defaults:
 *   file       /sdcard/demo_fs_benchmark.bin
 *   total_kb   5120
 *   block_kb   64
 *   random_ops 64
 */

#include <rtthread.h>

#ifdef RT_USING_DFS
#include <dfs_file.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/unistd.h>

#define DEMO_FS_BENCHMARK_DEFAULT_FILE       "/sdcard/demo_fs_benchmark.bin"
#define DEMO_FS_BENCHMARK_DEFAULT_TOTAL_KB   5120U
#define DEMO_FS_BENCHMARK_DEFAULT_BLOCK_KB   64U
#define DEMO_FS_BENCHMARK_DEFAULT_RAND_OPS   64U
#define DEMO_FS_BENCHMARK_MIN_BLOCK_KB       1U
#define DEMO_FS_BENCHMARK_MAX_BLOCK_KB       64U
#define DEMO_FS_BENCHMARK_ALIGN              32U

static rt_uint32_t g_demo_fsbench_write_kbps;
static rt_uint32_t g_demo_fsbench_read_kbps;
static rt_uint32_t g_demo_fsbench_verify_kbps;

static rt_bool_t demo_fsbench_parse_u32(const char *text, rt_uint32_t *value)
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

static void demo_fsbench_fill(rt_uint8_t *buffer, rt_size_t size, rt_uint32_t offset)
{
    rt_size_t i;

    for (i = 0; i < size; i++)
    {
        buffer[i] = (rt_uint8_t)((offset + i) ^ ((offset + i) >> 8));
    }
}

static rt_bool_t demo_fsbench_verify(const rt_uint8_t *buffer, rt_size_t size, rt_uint32_t offset)
{
    rt_size_t i;

    for (i = 0; i < size; i++)
    {
        rt_uint8_t expected = (rt_uint8_t)((offset + i) ^ ((offset + i) >> 8));

        if (buffer[i] != expected)
        {
            rt_kprintf("verify mismatch at offset %u: expect 0x%02x, got 0x%02x\n",
                       (unsigned)(offset + i),
                       expected,
                       buffer[i]);
            return RT_FALSE;
        }
    }

    return RT_TRUE;
}

static rt_uint32_t demo_fsbench_calc_kbps(rt_uint32_t bytes, rt_uint32_t ms)
{
    if (ms == 0U)
    {
        return 0U;
    }

    return (rt_uint32_t)((rt_uint64_t)bytes * 1000ULL / 1024ULL / ms);
}

static void demo_fsbench_print_speed(const char *name, rt_uint32_t bytes, rt_uint32_t ms)
{
    rt_uint32_t kbps = demo_fsbench_calc_kbps(bytes, ms);
    rt_uint32_t mbps_x100 = (rt_uint32_t)((rt_uint64_t)kbps * 100ULL / 1024ULL);

    if (ms == 0U)
    {
        rt_kprintf("%-8s %u bytes, <1 ms\n", name, (unsigned)bytes);
        return;
    }

    rt_kprintf("%-8s %u bytes, %u ms, %u.%02u MB/s (%u KB/s)\n",
               name,
               (unsigned)bytes,
               (unsigned)ms,
               (unsigned)(mbps_x100 / 100U),
               (unsigned)(mbps_x100 % 100U),
               (unsigned)kbps);
}

static int demo_fsbench_write_file(const char *path,
                              rt_uint8_t *buffer,
                              rt_uint32_t total_bytes,
                              rt_uint32_t block_bytes)
{
    int fd;
    rt_uint32_t done;
    rt_uint32_t start_ms;
    rt_uint32_t elapsed_ms;

    fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0);
    if (fd < 0)
    {
        rt_kprintf("open for write failed: %s\n", path);
        return -RT_ERROR;
    }

    start_ms = (rt_uint32_t)rt_tick_get_millisecond();
    for (done = 0; done < total_bytes; done += block_bytes)
    {
        rt_uint32_t chunk = total_bytes - done;
        int written;

        if (chunk > block_bytes)
        {
            chunk = block_bytes;
        }

        written = write(fd, buffer, chunk);
        if (written != (int)chunk)
        {
            rt_kprintf("write failed at %u/%u, ret=%d\n",
                       (unsigned)done, (unsigned)total_bytes, written);
            close(fd);
            return -RT_ERROR;
        }
    }
    fsync(fd);
    elapsed_ms = (rt_uint32_t)rt_tick_get_millisecond() - start_ms;
    close(fd);

    g_demo_fsbench_write_kbps = demo_fsbench_calc_kbps(total_bytes, elapsed_ms);
    demo_fsbench_print_speed("write", total_bytes, elapsed_ms);
    return RT_EOK;
}

static int demo_fsbench_read_file(const char *path,
                             rt_uint8_t *buffer,
                             rt_uint32_t total_bytes,
                             rt_uint32_t block_bytes,
                             rt_bool_t verify)
{
    int fd;
    rt_uint32_t done;
    rt_uint32_t start_ms;
    rt_uint32_t elapsed_ms;

    fd = open(path, O_RDONLY, 0);
    if (fd < 0)
    {
        rt_kprintf("open for read failed: %s\n", path);
        return -RT_ERROR;
    }

    start_ms = (rt_uint32_t)rt_tick_get_millisecond();
    for (done = 0; done < total_bytes; done += block_bytes)
    {
        rt_uint32_t chunk = total_bytes - done;
        int read_size;

        if (chunk > block_bytes)
        {
            chunk = block_bytes;
        }

        read_size = read(fd, buffer, chunk);
        if (read_size != (int)chunk)
        {
            rt_kprintf("read failed at %u/%u, ret=%d\n",
                       (unsigned)done, (unsigned)total_bytes, read_size);
            close(fd);
            return -RT_ERROR;
        }

        if (verify && !demo_fsbench_verify(buffer, chunk, 0U))
        {
            close(fd);
            return -RT_ERROR;
        }
    }
    elapsed_ms = (rt_uint32_t)rt_tick_get_millisecond() - start_ms;
    close(fd);

    if (verify)
    {
        g_demo_fsbench_verify_kbps = demo_fsbench_calc_kbps(total_bytes, elapsed_ms);
    }
    else
    {
        g_demo_fsbench_read_kbps = demo_fsbench_calc_kbps(total_bytes, elapsed_ms);
    }
    demo_fsbench_print_speed(verify ? "verify" : "read", total_bytes, elapsed_ms);
    return RT_EOK;
}

static int demo_fsbench_random_io(const char *path,
                             rt_uint8_t *buffer,
                             rt_uint32_t total_bytes,
                             rt_uint32_t block_bytes,
                             rt_uint32_t random_ops)
{
    int fd;
    rt_uint32_t i;
    rt_uint32_t seed = 0x12345678U;
    rt_uint32_t start_ms;
    rt_uint32_t write_ms;
    rt_uint32_t read_ms;
    rt_uint32_t random_bytes;
    rt_uint32_t max_pos;

    if (random_ops == 0U)
    {
        return RT_EOK;
    }

    if (total_bytes <= block_bytes)
    {
        rt_kprintf("random: skipped, total size <= block size\n");
        return RT_EOK;
    }

    fd = open(path, O_RDWR, 0);
    if (fd < 0)
    {
        rt_kprintf("open for random I/O failed: %s\n", path);
        return -RT_ERROR;
    }

    max_pos = (total_bytes - block_bytes) / block_bytes;
    demo_fsbench_fill(buffer, block_bytes, 0x55AA0000U);

    start_ms = (rt_uint32_t)rt_tick_get_millisecond();

    for (i = 0; i < random_ops; i++)
    {
        rt_uint32_t block_index;
        rt_uint32_t offset;
        int written;

        seed = seed * 1664525U + 1013904223U;
        block_index = seed % (max_pos + 1U);
        offset = block_index * block_bytes;

        if (lseek(fd, offset, SEEK_SET) < 0)
        {
            rt_kprintf("random seek write failed at %u\n", (unsigned)offset);
            close(fd);
            return -RT_ERROR;
        }

        written = write(fd, buffer, block_bytes);
        if (written != (int)block_bytes)
        {
            rt_kprintf("random write failed at %u, ret=%d\n", (unsigned)offset, written);
            close(fd);
            return -RT_ERROR;
        }
    }

    fsync(fd);
    write_ms = (rt_uint32_t)rt_tick_get_millisecond() - start_ms;

    rt_memset(buffer, 0, block_bytes);
    seed = 0x12345678U;
    start_ms = (rt_uint32_t)rt_tick_get_millisecond();

    for (i = 0; i < random_ops; i++)
    {
        rt_uint32_t block_index;
        rt_uint32_t offset;
        int read_size;

        seed = seed * 1664525U + 1013904223U;
        block_index = seed % (max_pos + 1U);
        offset = block_index * block_bytes;

        if (lseek(fd, offset, SEEK_SET) < 0)
        {
            rt_kprintf("random seek read failed at %u\n", (unsigned)offset);
            close(fd);
            return -RT_ERROR;
        }

        read_size = read(fd, buffer, block_bytes);
        if (read_size != (int)block_bytes)
        {
            rt_kprintf("random read failed at %u, ret=%d\n", (unsigned)offset, read_size);
            close(fd);
            return -RT_ERROR;
        }

        if (!demo_fsbench_verify(buffer, block_bytes, 0x55AA0000U))
        {
            close(fd);
            return -RT_ERROR;
        }
    }

    read_ms = (rt_uint32_t)rt_tick_get_millisecond() - start_ms;
    close(fd);

    random_bytes = random_ops * block_bytes;
    demo_fsbench_print_speed("rand_wr", random_bytes, write_ms);
    demo_fsbench_print_speed("rand_rd", random_bytes, read_ms);
    rt_kprintf("random   %u ops x %u bytes, write %u ms/op, read %u ms/op\n",
               (unsigned)random_ops,
               (unsigned)block_bytes,
               (unsigned)(write_ms / random_ops),
               (unsigned)(read_ms / random_ops));
    return RT_EOK;
}

static int demo_fs_benchmark(int argc, char **argv)
{
    const char *path = DEMO_FS_BENCHMARK_DEFAULT_FILE;
    rt_uint32_t total_kb = DEMO_FS_BENCHMARK_DEFAULT_TOTAL_KB;
    rt_uint32_t block_kb = DEMO_FS_BENCHMARK_DEFAULT_BLOCK_KB;
    rt_uint32_t random_ops = DEMO_FS_BENCHMARK_DEFAULT_RAND_OPS;
    rt_uint32_t total_bytes;
    rt_uint32_t block_bytes;
    rt_uint8_t *buffer;
    int ret = RT_EOK;

    if (argc > 1)
    {
        path = argv[1];
    }

    if ((argc > 2) && !demo_fsbench_parse_u32(argv[2], &total_kb))
    {
        rt_kprintf("Usage: demo_fs_benchmark [file] [total_kb] [block_kb] [random_ops]\n");
        return -RT_ERROR;
    }

    if ((argc > 3) && !demo_fsbench_parse_u32(argv[3], &block_kb))
    {
        rt_kprintf("Usage: demo_fs_benchmark [file] [total_kb] [block_kb] [random_ops]\n");
        return -RT_ERROR;
    }

    if ((argc > 4) && !demo_fsbench_parse_u32(argv[4], &random_ops))
    {
        rt_kprintf("Usage: demo_fs_benchmark [file] [total_kb] [block_kb] [random_ops]\n");
        return -RT_ERROR;
    }

    if ((total_kb == 0U) || (block_kb < DEMO_FS_BENCHMARK_MIN_BLOCK_KB) ||
        (block_kb > DEMO_FS_BENCHMARK_MAX_BLOCK_KB) || (total_kb < block_kb))
    {
        rt_kprintf("Usage: demo_fs_benchmark [file] [total_kb >= block_kb] [block_kb %u..%u] [random_ops]\n",
                   (unsigned)DEMO_FS_BENCHMARK_MIN_BLOCK_KB,
                   (unsigned)DEMO_FS_BENCHMARK_MAX_BLOCK_KB);
        return -RT_ERROR;
    }

    total_bytes = total_kb * 1024U;
    block_bytes = block_kb * 1024U;
    if (((total_bytes / 1024U) != total_kb) || ((block_bytes / 1024U) != block_kb))
    {
        rt_kprintf("Invalid benchmark size\n");
        return -RT_ERROR;
    }

    total_bytes = (total_bytes / block_bytes) * block_bytes;
    buffer = (rt_uint8_t *)rt_malloc_align(block_bytes, DEMO_FS_BENCHMARK_ALIGN);
    if (buffer == RT_NULL)
    {
        rt_kprintf("alloc %u bytes failed\n", (unsigned)block_bytes);
        return -RT_ENOMEM;
    }

    rt_kprintf("\nFilesystem benchmark\n");
    rt_kprintf("file=%s, total=%u KB, block=%u KB, random_ops=%u\n",
               path,
               (unsigned)(total_bytes / 1024U),
               (unsigned)block_kb,
               (unsigned)random_ops);

    g_demo_fsbench_write_kbps = 0U;
    g_demo_fsbench_read_kbps = 0U;
    g_demo_fsbench_verify_kbps = 0U;

    demo_fsbench_fill(buffer, block_bytes, 0U);
    ret = demo_fsbench_write_file(path, buffer, total_bytes, block_bytes);
    if (ret == RT_EOK)
    {
        ret = demo_fsbench_read_file(path, buffer, total_bytes, block_bytes, RT_FALSE);
    }
    if (ret == RT_EOK)
    {
        ret = demo_fsbench_read_file(path, buffer, total_bytes, block_bytes, RT_TRUE);
    }
    if (ret == RT_EOK)
    {
        ret = demo_fsbench_random_io(path, buffer, total_bytes, block_bytes, random_ops);
    }

    rt_kprintf("summary  write=%u KB/s, read=%u KB/s, verify=%u KB/s\n",
               (unsigned)g_demo_fsbench_write_kbps,
               (unsigned)g_demo_fsbench_read_kbps,
               (unsigned)g_demo_fsbench_verify_kbps);

    unlink(path);
    rt_free_align(buffer);
    return ret;
}
#else
static int demo_fs_benchmark(int argc, char **argv)
{
    RT_UNUSED(argc);
    RT_UNUSED(argv);
    rt_kprintf("DFS is not enabled\n");
    return -RT_ERROR;
}
#endif

MSH_CMD_EXPORT(demo_fs_benchmark, filesystem benchmark);
