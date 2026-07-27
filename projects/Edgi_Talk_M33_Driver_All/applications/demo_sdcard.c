/*
 * demo_sdcard.c - SD card file I/O sample (ported from Edgi_Talk_M33_SDCARD)
 *
 * MSH command: demo_sdcard
 *   Writes a small text file to /sdcard, reads it back, prints contents.
 *
 * MSH command: demo_sdcard_speed [total_kb] [block_kb]
 *   Measures sequential file write/read speed. Default is 4096 KB total
 *   with a 64 KB aligned buffer.
 *
 *   The mount itself is handled by the shared mnt.c (INIT_ENV_EXPORT) which
 *   activates automatically when BSP_USING_SDCARD + DFS are enabled.
 *
 * Kconfig: BSP_USING_FILESYSTEM + BSP_USING_SDCARD + BSP_USING_FS
 *          + RT_USING_DFS + RT_USING_DFS_V1 + DFS_USING_POSIX
 *          + RT_USING_DFS_ELMFAT + RT_USING_DFS_DEVFS + RT_USING_DFS_ROMFS
 *          + RT_USING_SDIO
 */

#include <rtthread.h>
#include <stdlib.h>

#ifdef RT_USING_DFS_ELMFAT
#include <dfs_file.h>
#include <fcntl.h>
#include <sys/unistd.h>
#include <sys/stat.h>

#define DEMO_SDCARD_FILE   "/sdcard/demo_test.txt"
#define DEMO_SDCARD_SPEED_FILE "/sdcard/speed_test.bin"
#define DEMO_SDCARD_SPEED_TOTAL_KB 4096U
#define DEMO_SDCARD_SPEED_BLOCK_KB 64U
#define DEMO_SDCARD_MIN_BLOCK_KB   1U
#define DEMO_SDCARD_MAX_BLOCK_KB   64U

static rt_bool_t demo_sdcard_parse_u32(const char *text, rt_uint32_t *value)
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

static rt_bool_t demo_sdcard_is_mounted(void)
{
    struct stat stat_buf;

    return (stat("/sdcard", &stat_buf) == 0) ? RT_TRUE : RT_FALSE;
}

static void demo_sdcard_print_speed(const char *name, rt_uint32_t bytes, rt_uint32_t ms)
{
    rt_uint64_t bytes_per_sec;
    rt_uint32_t mbps_x100;

    if (ms == 0U)
    {
        rt_kprintf("%s: %u bytes, <1 ms, increase total_kb\n", name, (unsigned)bytes);
        return;
    }

    bytes_per_sec = ((rt_uint64_t)bytes * 1000ULL) / ms;
    mbps_x100 = (rt_uint32_t)((bytes_per_sec * 100ULL) / (1024ULL * 1024ULL));

    rt_kprintf("%-5s %u bytes, %u ms, %u.%02u MB/s (%u KB/s)\n",
               name,
               (unsigned)bytes,
               (unsigned)ms,
               (unsigned)(mbps_x100 / 100U),
               (unsigned)(mbps_x100 % 100U),
               (unsigned)(bytes_per_sec / 1024ULL));
}

static int demo_sdcard(int argc, char *argv[])
{
    static const char write_buf[] = "Hello from Edgi_Talk_M33_Driver_All SDCARD sample.\n";
    char read_buf[128];
    int fd, written, read_size;

    RT_UNUSED(argc);
    RT_UNUSED(argv);

    if (!demo_sdcard_is_mounted())
    {
        rt_kprintf("/sdcard not mounted. Insert SD card and verify:\n");
        rt_kprintf("  BSP_USING_SDCARD + RT_USING_DFS_ELMFAT + RT_USING_SDIO\n");
        return -RT_ERROR;
    }
    rt_kprintf("/sdcard mounted.\n");

    /* 2. Write */
    fd = open(DEMO_SDCARD_FILE, O_WRONLY | O_CREAT | O_TRUNC, 0);
    if (fd < 0)
    {
        rt_kprintf("Failed to open %s for write (fd=%d)\n", DEMO_SDCARD_FILE, fd);
        return -RT_ERROR;
    }
    written = write(fd, write_buf, sizeof(write_buf) - 1);
    close(fd);
    if (written < 0)
    {
        rt_kprintf("Write failed\n");
        return -RT_ERROR;
    }
    rt_kprintf("Wrote %d bytes to %s\n", written, DEMO_SDCARD_FILE);

    /* 3. Read back */
    fd = open(DEMO_SDCARD_FILE, O_RDONLY, 0);
    if (fd < 0)
    {
        rt_kprintf("Failed to open %s for read\n", DEMO_SDCARD_FILE);
        return -RT_ERROR;
    }
    read_size = read(fd, read_buf, sizeof(read_buf) - 1);
    close(fd);
    if (read_size < 0)
    {
        rt_kprintf("Read failed\n");
        return -RT_ERROR;
    }
    read_buf[read_size] = '\0';
    rt_kprintf("Read back %d bytes:\n%s\n", read_size, read_buf);
    return RT_EOK;
}

static int demo_sdcard_speed(int argc, char *argv[])
{
    rt_uint32_t total_kb = DEMO_SDCARD_SPEED_TOTAL_KB;
    rt_uint32_t block_kb = DEMO_SDCARD_SPEED_BLOCK_KB;
    rt_uint32_t total_bytes;
    rt_uint32_t block_bytes;
    rt_uint32_t done;
    rt_uint32_t start;
    rt_uint32_t elapsed_ms;
    rt_uint8_t *buf;
    int fd;
    int ret = RT_EOK;
    rt_uint32_t i;
    rt_uint32_t checksum = 0U;

    if ((argc > 1) && !demo_sdcard_parse_u32(argv[1], &total_kb))
    {
        rt_kprintf("Usage: demo_sdcard_speed [total_kb] [block_kb]\n");
        return -RT_ERROR;
    }

    if ((argc > 2) && !demo_sdcard_parse_u32(argv[2], &block_kb))
    {
        rt_kprintf("Usage: demo_sdcard_speed [total_kb] [block_kb]\n");
        return -RT_ERROR;
    }

    if ((total_kb == 0U) || (block_kb < DEMO_SDCARD_MIN_BLOCK_KB) ||
        (block_kb > DEMO_SDCARD_MAX_BLOCK_KB) || (total_kb < block_kb))
    {
        rt_kprintf("Usage: demo_sdcard_speed [total_kb >= block_kb] [block_kb %u..%u]\n",
                   (unsigned)DEMO_SDCARD_MIN_BLOCK_KB,
                   (unsigned)DEMO_SDCARD_MAX_BLOCK_KB);
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
    if (total_bytes == 0U)
    {
        rt_kprintf("Invalid total size\n");
        return -RT_ERROR;
    }

    if (!demo_sdcard_is_mounted())
    {
        rt_kprintf("/sdcard not mounted\n");
        return -RT_ERROR;
    }

    buf = (rt_uint8_t *)rt_malloc_align(block_bytes, 32);
    if (buf == RT_NULL)
    {
        rt_kprintf("alloc %u bytes failed\n", (unsigned)block_bytes);
        return -RT_ENOMEM;
    }

    for (i = 0; i < block_bytes; i++)
    {
        buf[i] = (rt_uint8_t)(i + (i >> 8));
    }

    fd = open(DEMO_SDCARD_SPEED_FILE, O_WRONLY | O_CREAT | O_TRUNC, 0);
    if (fd < 0)
    {
        rt_kprintf("open %s for write failed\n", DEMO_SDCARD_SPEED_FILE);
        rt_free_align(buf);
        return -RT_ERROR;
    }

    start = (rt_uint32_t)rt_tick_get_millisecond();
    for (done = 0; done < total_bytes; done += block_bytes)
    {
        int written = write(fd, buf, block_bytes);
        if (written != (int)block_bytes)
        {
            rt_kprintf("write failed at %u/%u, ret=%d\n",
                       (unsigned)done, (unsigned)total_bytes, written);
            ret = -RT_ERROR;
            break;
        }
    }
    fsync(fd);
    elapsed_ms = (rt_uint32_t)rt_tick_get_millisecond() - start;
    close(fd);

    if (ret == RT_EOK)
    {
        demo_sdcard_print_speed("write", total_bytes, elapsed_ms);
    }
    else
    {
        rt_free_align(buf);
        return ret;
    }

    rt_memset(buf, 0, block_bytes);
    fd = open(DEMO_SDCARD_SPEED_FILE, O_RDONLY, 0);
    if (fd < 0)
    {
        rt_kprintf("open %s for read failed\n", DEMO_SDCARD_SPEED_FILE);
        rt_free_align(buf);
        return -RT_ERROR;
    }

    start = (rt_uint32_t)rt_tick_get_millisecond();
    for (done = 0; done < total_bytes; done += block_bytes)
    {
        int read_size = read(fd, buf, block_bytes);
        if (read_size != (int)block_bytes)
        {
            rt_kprintf("read failed at %u/%u, ret=%d\n",
                       (unsigned)done, (unsigned)total_bytes, read_size);
            ret = -RT_ERROR;
            break;
        }

        checksum += buf[0];
        checksum += buf[block_bytes - 1U];
    }
    elapsed_ms = (rt_uint32_t)rt_tick_get_millisecond() - start;
    close(fd);

    if (ret == RT_EOK)
    {
        demo_sdcard_print_speed("read", total_bytes, elapsed_ms);
        rt_kprintf("SD speed test: file=%s, total=%u KB, block=%u KB, checksum=0x%08X\n",
                   DEMO_SDCARD_SPEED_FILE,
                   (unsigned)(total_bytes / 1024U),
                   (unsigned)block_kb,
                   (unsigned)checksum);
    }

    unlink(DEMO_SDCARD_SPEED_FILE);
    rt_free_align(buf);
    return ret;
}
#else
static int demo_sdcard(int argc, char *argv[])
{
    RT_UNUSED(argc);
    RT_UNUSED(argv);
    rt_kprintf("DFS not enabled. Enable in menuconfig:\n");
    rt_kprintf("  BSP_USING_FILESYSTEM + BSP_USING_SDCARD + RT_USING_DFS +\n");
    rt_kprintf("  RT_USING_DFS_ELMFAT + DFS_USING_POSIX + RT_USING_SDIO\n");
    return -RT_ERROR;
}

static int demo_sdcard_speed(int argc, char *argv[])
{
    RT_UNUSED(argc);
    RT_UNUSED(argv);
    rt_kprintf("DFS not enabled. Enable in menuconfig:\n");
    rt_kprintf("  RT_USING_DFS_ELMFAT + DFS_USING_POSIX + RT_USING_SDIO\n");
    return -RT_ERROR;
}
#endif
MSH_CMD_EXPORT(demo_sdcard, sdcard write/read test);
MSH_CMD_EXPORT(demo_sdcard_speed, sdcard sequential read/write speed test);
