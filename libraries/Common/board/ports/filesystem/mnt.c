#include <rtthread.h>

#ifdef RT_USING_DFS
#include <dfs_fs.h>
#include <rtdevice.h>
#ifdef BSP_USING_FLASH
#include <fal.h>
#endif
#include <drivers/mmcsd_core.h>
#include "dfs_romfs.h"

#define DBG_TAG "app.filesystem"
#define DBG_LVL DBG_INFO
#include <rtdbg.h>

#define SDCARD_MOUNT_POINT      "/sdcard"
#define SDCARD_FS_TYPE          "elm"
#define SDCARD_POLL_MS          1000
#define SDCARD_RESCAN_MS        3000
#define SDCARD_REMOVE_ERRORS    2
#define SDCARD_QUIET_MS         8000

extern rt_err_t rt_hw_sdio_rescan(void);
extern rt_err_t rt_hw_sdio_force_change(void);
extern void rt_hw_sdio_quiet_for(rt_uint32_t timeout_ms);
extern void rt_hw_sdio_quiet_begin(void);
extern void rt_hw_sdio_quiet_end(void);

static rt_bool_t g_sdcard_mounted = RT_FALSE;
static rt_bool_t g_sdcard_ejected = RT_FALSE;
static rt_bool_t g_sdcard_rescan_pending = RT_TRUE;
static const char *g_sdcard_mounted_device = RT_NULL;

#ifndef BSP_USING_XiaoZhi
static const struct romfs_dirent _romfs_root[] =
{
#ifdef BSP_USING_FLASH
    {ROMFS_DIRENT_DIR, "flash", RT_NULL, 0},
#endif
#ifdef BSP_USING_SDCARD
    {ROMFS_DIRENT_DIR, "sdcard", RT_NULL, 0},
#endif
};

const struct romfs_dirent romfs_root =
{
    ROMFS_DIRENT_DIR, "/", (rt_uint8_t *)_romfs_root, sizeof(_romfs_root) / sizeof(_romfs_root[0])
};
#endif

static const char *_sdcard_find_device(void)
{
#ifdef BSP_USING_SDCARD
    const char *sd_device_names[] = {"sd", "sd0", "sd1", "sd2"};
    int i;

    for (i = 0; i < sizeof(sd_device_names) / sizeof(sd_device_names[0]); i++)
    {
        if (rt_device_find(sd_device_names[i]) != RT_NULL)
        {
            return sd_device_names[i];
        }
    }
#endif
    return RT_NULL;
}

static rt_bool_t _sdcard_probe_device(const char *device_name)
{
#ifdef BSP_USING_SDCARD
    rt_device_t device;
    rt_uint8_t *sector;
    rt_size_t read_count;

    if (device_name == RT_NULL)
    {
        return RT_FALSE;
    }

    device = rt_device_find(device_name);
    if (device == RT_NULL)
    {
        return RT_FALSE;
    }

    sector = rt_malloc_align(512, 32);
    if (sector == RT_NULL)
    {
        return RT_TRUE;
    }

    rt_hw_sdio_quiet_begin();
    read_count = rt_device_read(device, 0, sector, 1);
    rt_hw_sdio_quiet_end();
    rt_free_align(sector);

    return (read_count == 1) ? RT_TRUE : RT_FALSE;
#else
    RT_UNUSED(device_name);
    return RT_FALSE;
#endif
}

static rt_bool_t _sdcard_mount(const char **mounted_device)
{
#ifdef BSP_USING_SDCARD
    const char *device_name = _sdcard_find_device();

    if (device_name == RT_NULL)
    {
        return RT_FALSE;
    }

    rt_thread_mdelay(200);

    if (dfs_mount(device_name, SDCARD_MOUNT_POINT, SDCARD_FS_TYPE, 0, 0) == RT_EOK)
    {
        *mounted_device = device_name;
        LOG_I("sd card '%s' mount to '%s' success!", device_name, SDCARD_MOUNT_POINT);
        return RT_TRUE;
    }

    LOG_E("sd card mount to '%s' failed. Run 'sdcard_mkfs' manually to format.", SDCARD_MOUNT_POINT);
#endif /* BSP_USING_SDCARD */
    return RT_FALSE;
}

#ifdef BSP_USING_FLASH
static void _fal_mount(void)
{
    struct rt_device *flash_dev = fal_mtd_nor_device_create("filesystem");
    if (flash_dev == NULL)
    {
        LOG_E("Can't create block device for filesystem");
        return;
    }
    else
    {
        LOG_I("Block device created for filesystem");

        rt_thread_mdelay(200);

        /* Try to mount filesystem */
        if (dfs_mount("filesystem", "/flash", "lfs", 0, 0) != 0)
        {
            LOG_E("Mount filesystem failed, try to mkfs");

            /* Format filesystem */
            rt_thread_mdelay(200);

            if (dfs_mkfs("lfs", "filesystem") != 0)
            {
                LOG_E("mkfs failed");
                return;
            }
            else
            {
                LOG_I("Filesystem formatted");

                /* Mount after format */
                if (dfs_mount("filesystem", "/flash", "lfs", 0, 0) != 0)
                {
                    LOG_E("Mount filesystem failed after mkfs");
                    return;
                }
                else
                {
                    LOG_I("Filesystem mounted successfully");
                }
            }
        }
        else
        {
            LOG_I("Filesystem mounted successfully");
        }
    }
}
#endif /* BSP_USING_FLASH */

static void sd_hotplug_thread(void *parameter)
{
    rt_tick_t last_rescan_tick = 0;
    int remove_errors = 0;

    RT_UNUSED(parameter);

    rt_thread_mdelay(200);

    while (1)
    {
        int cd_event;

        if (!g_sdcard_mounted)
        {
            const char *device_name = _sdcard_find_device();

            if ((device_name != RT_NULL) && (g_sdcard_ejected == RT_FALSE))
            {
                g_sdcard_mounted = _sdcard_mount(&g_sdcard_mounted_device);
                remove_errors = 0;
                g_sdcard_rescan_pending = !g_sdcard_mounted;
                rt_thread_mdelay(SDCARD_POLL_MS);
                continue;
            }

            if ((device_name == RT_NULL) && (g_sdcard_ejected == RT_TRUE))
            {
                g_sdcard_ejected = RT_FALSE;
                g_sdcard_rescan_pending = RT_TRUE;
            }

            if (g_sdcard_rescan_pending ||
                ((rt_tick_get() - last_rescan_tick) >= rt_tick_from_millisecond(SDCARD_RESCAN_MS)))
            {
                rt_hw_sdio_quiet_for(SDCARD_QUIET_MS);
                (void)rt_hw_sdio_rescan();
                last_rescan_tick = rt_tick_get();
                g_sdcard_rescan_pending = RT_FALSE;
            }

            cd_event = mmcsd_wait_cd_changed(rt_tick_from_millisecond(SDCARD_POLL_MS));
            if ((cd_event == MMCSD_HOST_PLUGED) && (g_sdcard_ejected == RT_FALSE))
            {
                g_sdcard_mounted = _sdcard_mount(&g_sdcard_mounted_device);
                remove_errors = 0;
                g_sdcard_rescan_pending = !g_sdcard_mounted;
            }
        }
        else
        {
            cd_event = mmcsd_wait_cd_changed(rt_tick_from_millisecond(SDCARD_POLL_MS));
            if (cd_event == MMCSD_HOST_UNPLUGED)
            {
                remove_errors = SDCARD_REMOVE_ERRORS;
            }
            else if (!_sdcard_probe_device(g_sdcard_mounted_device))
            {
                remove_errors++;
            }
            else
            {
                remove_errors = 0;
            }

            if (remove_errors >= SDCARD_REMOVE_ERRORS)
            {
                if (dfs_unmount(SDCARD_MOUNT_POINT) == RT_EOK)
                {
                    LOG_I("sd card unmount from '%s' success!", SDCARD_MOUNT_POINT);
                }
                else
                {
                    LOG_W("sd card unmount from '%s' failed", SDCARD_MOUNT_POINT);
                }

                g_sdcard_mounted = RT_FALSE;
                g_sdcard_mounted_device = RT_NULL;
                remove_errors = 0;
                g_sdcard_rescan_pending = RT_TRUE;
                rt_hw_sdio_quiet_for(SDCARD_QUIET_MS);
                (void)rt_hw_sdio_force_change();
            }
        }
    }
}

#ifdef BSP_USING_SDCARD
static int sdcard_umount(int argc, char **argv)
{
    RT_UNUSED(argc);
    RT_UNUSED(argv);

    if (g_sdcard_mounted == RT_FALSE)
    {
        rt_kprintf("%s is not mounted\n", SDCARD_MOUNT_POINT);
        return -RT_ERROR;
    }

    if (dfs_unmount(SDCARD_MOUNT_POINT) != RT_EOK)
    {
        rt_kprintf("unmount %s failed\n", SDCARD_MOUNT_POINT);
        return -RT_ERROR;
    }

    g_sdcard_mounted = RT_FALSE;
    g_sdcard_mounted_device = RT_NULL;
    g_sdcard_ejected = RT_TRUE;
    g_sdcard_rescan_pending = RT_FALSE;
    rt_hw_sdio_quiet_for(SDCARD_QUIET_MS);

    rt_kprintf("%s unmounted, safe to remove SD card\n", SDCARD_MOUNT_POINT);
    return RT_EOK;
}
MSH_CMD_EXPORT(sdcard_umount, unmount sd card before removing);

static int sdcard_mount(int argc, char **argv)
{
    RT_UNUSED(argc);
    RT_UNUSED(argv);

    if (g_sdcard_mounted == RT_TRUE)
    {
        rt_kprintf("%s is already mounted\n", SDCARD_MOUNT_POINT);
        return RT_EOK;
    }

    g_sdcard_ejected = RT_FALSE;
    if (_sdcard_mount(&g_sdcard_mounted_device) == RT_FALSE)
    {
        g_sdcard_rescan_pending = RT_TRUE;
        rt_kprintf("mount %s failed\n", SDCARD_MOUNT_POINT);
        return -RT_ERROR;
    }

    g_sdcard_mounted = RT_TRUE;
    g_sdcard_rescan_pending = RT_FALSE;
    return RT_EOK;
}
MSH_CMD_EXPORT(sdcard_mount, mount sd card);

static int sdcard_mkfs(int argc, char **argv)
{
    const char *device_name;

    RT_UNUSED(argc);
    RT_UNUSED(argv);

    if (g_sdcard_mounted == RT_TRUE)
    {
        rt_kprintf("unmount %s before formatting\n", SDCARD_MOUNT_POINT);
        return -RT_ERROR;
    }

    device_name = _sdcard_find_device();
    if (device_name == RT_NULL)
    {
        rt_kprintf("sd card device not found\n");
        return -RT_ERROR;
    }

    rt_kprintf("formatting sd card device '%s' as elm filesystem...\n", device_name);
    if (dfs_mkfs("elm", device_name) != 0)
    {
        rt_kprintf("format sd card failed\n");
        return -RT_ERROR;
    }

    rt_kprintf("format sd card success\n");
    g_sdcard_ejected = RT_FALSE;
    g_sdcard_rescan_pending = RT_TRUE;
    return RT_EOK;
}
MSH_CMD_EXPORT(sdcard_mkfs, format sd card manually);
#endif

int mnt_init(void)
{
    if (dfs_mount(RT_NULL, "/", "rom", 0, &(romfs_root)) != 0)
    {
        LOG_E("rom mount to '/' failed!");
        return -RT_ERROR;
    }

#ifdef BSP_USING_FLASH
    fal_init();
    /* Mount FAL filesystem to /flash */
    _fal_mount();
#endif

#ifdef BSP_USING_SDCARD
    rt_thread_t tid;

    tid = rt_thread_create("sd_hotplug", sd_hotplug_thread, RT_NULL,
                           2048, RT_THREAD_PRIORITY_MAX - 2, 20);
    if (tid != RT_NULL)
    {
        rt_thread_startup(tid);
    }
    else
    {
        LOG_E("create sd_mount thread err!");
    }
#endif

    return RT_EOK;
}
INIT_ENV_EXPORT(mnt_init);

#endif
