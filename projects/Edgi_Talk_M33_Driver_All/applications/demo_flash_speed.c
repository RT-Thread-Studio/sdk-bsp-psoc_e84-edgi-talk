/*
 * demo_flash_speed.c - /flash filesystem sequential speed demo
 *
 * MSH command:
 *   demo_flash_speed [total_kb] [block_kb]
 *
 * Defaults:
 *   file     /flash/flash_speed.bin
 *   total_kb 512
 *   block_kb 4
 */

#include <rtthread.h>
#include <stdlib.h>

#ifdef RT_USING_DFS
#include <dfs_fs.h>
#include <dfs_file.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/unistd.h>
#ifdef RT_USING_FAL
#include <fal.h>
#endif

#define DEMO_FLASH_SPEED_FILE       "/flash/flash_speed.bin"
#define DEMO_FLASH_SPEED_TOTAL_KB   512U
#define DEMO_FLASH_SPEED_BLOCK_KB   4U
#define DEMO_FLASH_SPEED_MIN_KB     1U
#define DEMO_FLASH_SPEED_MAX_KB     64U
#define DEMO_FLASH_SPEED_ALIGN      32U

static rt_bool_t demo_flash_parse_u32(const char *text, rt_uint32_t *value)
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

static rt_bool_t demo_flash_is_mounted(void)
{
    struct stat stat_buf;

    return (stat("/flash", &stat_buf) == 0) ? RT_TRUE : RT_FALSE;
}

static void demo_flash_fill(rt_uint8_t *buffer, rt_size_t size)
{
    rt_size_t i;

    for (i = 0; i < size; i++)
    {
        buffer[i] = (rt_uint8_t)(i ^ (i >> 8) ^ 0x5AU);
    }
}

static rt_bool_t demo_flash_verify(const rt_uint8_t *buffer, rt_size_t size)
{
    rt_size_t i;

    for (i = 0; i < size; i++)
    {
        rt_uint8_t expected = (rt_uint8_t)(i ^ (i >> 8) ^ 0x5AU);

        if (buffer[i] != expected)
        {
            rt_kprintf("verify mismatch at %u: expect 0x%02x, got 0x%02x\n",
                       (unsigned)i, expected, buffer[i]);
            return RT_FALSE;
        }
    }

    return RT_TRUE;
}

static rt_uint32_t demo_flash_kbps(rt_uint32_t bytes, rt_uint32_t ms)
{
    if (ms == 0U)
    {
        return 0U;
    }

    return (rt_uint32_t)((rt_uint64_t)bytes * 1000ULL / 1024ULL / ms);
}

static void demo_flash_print_speed(const char *name, rt_uint32_t bytes, rt_uint32_t ms)
{
    rt_uint32_t kbps = demo_flash_kbps(bytes, ms);
    rt_uint32_t mbps_x100 = (rt_uint32_t)((rt_uint64_t)kbps * 100ULL / 1024ULL);

    if (ms == 0U)
    {
        rt_kprintf("%-6s %u bytes, <1 ms\n", name, (unsigned)bytes);
        return;
    }

    rt_kprintf("%-6s %u bytes, %u ms, %u.%02u MB/s (%u KB/s)\n",
               name,
               (unsigned)bytes,
               (unsigned)ms,
               (unsigned)(mbps_x100 / 100U),
               (unsigned)(mbps_x100 % 100U),
               (unsigned)kbps);
}

static void demo_flash_print_info(rt_uint32_t test_bytes)
{
    struct statfs fs_stat;

#ifdef RT_USING_FAL
    const struct fal_partition *part = fal_partition_find("filesystem");

    if (part != RT_NULL)
    {
        const struct fal_flash_dev *flash = fal_flash_device_find(part->flash_name);

        rt_kprintf("FAL    part=%s, dev=%s, offset=0x%08x, size=%u KB",
                   part->name,
                   part->flash_name,
                   (unsigned)part->offset,
                   (unsigned)(part->len / 1024U));
        if (flash != RT_NULL)
        {
            rt_kprintf(", flash=%u KB, erase=%u KB",
                       (unsigned)(flash->len / 1024U),
                       (unsigned)(flash->blk_size / 1024U));
        }
        rt_kprintf("\n");
    }
#endif

    if (statfs("/flash", &fs_stat) == 0)
    {
        rt_uint64_t total = (rt_uint64_t)fs_stat.f_bsize * fs_stat.f_blocks;
        rt_uint64_t free = (rt_uint64_t)fs_stat.f_bsize * fs_stat.f_bfree;

        rt_kprintf("LFS    block=%u KB, blocks=%u, total=%u KB, free=%u KB\n",
                   (unsigned)(fs_stat.f_bsize / 1024U),
                   (unsigned)fs_stat.f_blocks,
                   (unsigned)(total / 1024ULL),
                   (unsigned)(free / 1024ULL));

        if ((fs_stat.f_bsize >= 32768U) && (test_bytes >= (free / 2ULL)))
        {
            rt_kprintf("note   test file is large for this LittleFS volume; write includes erase/GC/metadata cost\n");
        }
    }
}

static int demo_flash_speed(int argc, char **argv)
{
    rt_uint32_t total_kb = DEMO_FLASH_SPEED_TOTAL_KB;
    rt_uint32_t block_kb = DEMO_FLASH_SPEED_BLOCK_KB;
    rt_uint32_t total_bytes;
    rt_uint32_t block_bytes;
    rt_uint32_t done;
    rt_uint32_t start_ms;
    rt_uint32_t write_loop_ms;
    rt_uint32_t sync_ms;
    rt_uint32_t elapsed_ms;
    rt_uint8_t *buffer;
    int fd;
    int ret = RT_EOK;

    if ((argc > 1) && !demo_flash_parse_u32(argv[1], &total_kb))
    {
        rt_kprintf("Usage: demo_flash_speed [total_kb] [block_kb]\n");
        return -RT_ERROR;
    }

    if ((argc > 2) && !demo_flash_parse_u32(argv[2], &block_kb))
    {
        rt_kprintf("Usage: demo_flash_speed [total_kb] [block_kb]\n");
        return -RT_ERROR;
    }

    if ((total_kb == 0U) || (block_kb < DEMO_FLASH_SPEED_MIN_KB) ||
        (block_kb > DEMO_FLASH_SPEED_MAX_KB) || (total_kb < block_kb))
    {
        rt_kprintf("Usage: demo_flash_speed [total_kb >= block_kb] [block_kb %u..%u]\n",
                   (unsigned)DEMO_FLASH_SPEED_MIN_KB,
                   (unsigned)DEMO_FLASH_SPEED_MAX_KB);
        return -RT_ERROR;
    }

    total_bytes = total_kb * 1024U;
    block_bytes = block_kb * 1024U;
    if (((total_bytes / 1024U) != total_kb) || ((block_bytes / 1024U) != block_kb))
    {
        rt_kprintf("Invalid size\n");
        return -RT_ERROR;
    }

    total_bytes = (total_bytes / block_bytes) * block_bytes;
    if (!demo_flash_is_mounted())
    {
        rt_kprintf("/flash not mounted\n");
        return -RT_ERROR;
    }

    buffer = (rt_uint8_t *)rt_malloc_align(block_bytes, DEMO_FLASH_SPEED_ALIGN);
    if (buffer == RT_NULL)
    {
        rt_kprintf("alloc %u bytes failed\n", (unsigned)block_bytes);
        return -RT_ENOMEM;
    }

    demo_flash_fill(buffer, block_bytes);

    rt_kprintf("\nFlash speed test\n");
    rt_kprintf("file=%s, total=%u KB, block=%u KB\n",
               DEMO_FLASH_SPEED_FILE,
               (unsigned)(total_bytes / 1024U),
               (unsigned)block_kb);
    demo_flash_print_info(total_bytes);

    fd = open(DEMO_FLASH_SPEED_FILE, O_WRONLY | O_CREAT | O_TRUNC, 0);
    if (fd < 0)
    {
        rt_kprintf("open %s for write failed\n", DEMO_FLASH_SPEED_FILE);
        rt_free_align(buffer);
        return -RT_ERROR;
    }

    start_ms = (rt_uint32_t)rt_tick_get_millisecond();
    for (done = 0; done < total_bytes; done += block_bytes)
    {
        int written = write(fd, buffer, block_bytes);
        if (written != (int)block_bytes)
        {
            rt_kprintf("write failed at %u/%u, ret=%d\n",
                       (unsigned)done, (unsigned)total_bytes, written);
            ret = -RT_ERROR;
            break;
        }
    }
    write_loop_ms = (rt_uint32_t)rt_tick_get_millisecond() - start_ms;
    fsync(fd);
    sync_ms = (rt_uint32_t)rt_tick_get_millisecond() - start_ms - write_loop_ms;
    elapsed_ms = (rt_uint32_t)rt_tick_get_millisecond() - start_ms;
    close(fd);

    if (ret != RT_EOK)
    {
        unlink(DEMO_FLASH_SPEED_FILE);
        rt_free_align(buffer);
        return ret;
    }
    demo_flash_print_speed("write", total_bytes, elapsed_ms);
    rt_kprintf("detail write_loop=%u ms, fsync=%u ms\n",
               (unsigned)write_loop_ms,
               (unsigned)sync_ms);

    rt_memset(buffer, 0, block_bytes);
    fd = open(DEMO_FLASH_SPEED_FILE, O_RDONLY, 0);
    if (fd < 0)
    {
        rt_kprintf("open %s for read failed\n", DEMO_FLASH_SPEED_FILE);
        unlink(DEMO_FLASH_SPEED_FILE);
        rt_free_align(buffer);
        return -RT_ERROR;
    }

    start_ms = (rt_uint32_t)rt_tick_get_millisecond();
    for (done = 0; done < total_bytes; done += block_bytes)
    {
        int read_size = read(fd, buffer, block_bytes);
        if (read_size != (int)block_bytes)
        {
            rt_kprintf("read failed at %u/%u, ret=%d\n",
                       (unsigned)done, (unsigned)total_bytes, read_size);
            ret = -RT_ERROR;
            break;
        }

        if (!demo_flash_verify(buffer, block_bytes))
        {
            ret = -RT_ERROR;
            break;
        }
    }
    elapsed_ms = (rt_uint32_t)rt_tick_get_millisecond() - start_ms;
    close(fd);

    if (ret == RT_EOK)
    {
        demo_flash_print_speed("read", total_bytes, elapsed_ms);
    }

    unlink(DEMO_FLASH_SPEED_FILE);
    rt_free_align(buffer);
    return ret;
}
#else
static int demo_flash_speed(int argc, char **argv)
{
    RT_UNUSED(argc);
    RT_UNUSED(argv);
    rt_kprintf("DFS is not enabled\n");
    return -RT_ERROR;
}
#endif

MSH_CMD_EXPORT(demo_flash_speed, flash filesystem sequential speed test);
