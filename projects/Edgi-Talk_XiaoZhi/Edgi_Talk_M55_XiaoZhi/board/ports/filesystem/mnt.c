/*
 * Copyright (c) 2006-2018, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author        Notes
 */

#include <rtthread.h>

#ifdef RT_USING_DFS
#include <dfs_fs.h>
#include "dfs_romfs.h"
#include <drivers/mmcsd_core.h>

#define DBG_TAG "app.filesystem"
#define DBG_LVL DBG_INFO
#include <rtdbg.h>

static void _sdcard_mount(void)
{
    rt_device_t device;

    device = rt_device_find("sd0");
    if (device == RT_NULL)
    {
        /* Clear previous CD status and wait for SD card initialization */
        mmcsd_wait_cd_changed(0);
        /* Wait for SD card detection, timeout 5 seconds */
        if (mmcsd_wait_cd_changed(rt_tick_from_millisecond(5000)) == -RT_ETIMEOUT)
        {
            LOG_W("Wait for SD card timeout!");
            return;
        }
        device = rt_device_find("sd0");
    }

    if (device == RT_NULL)
    {
        LOG_W("sd card device 'sd0' not found!");
        return;
    }

    /* Try to mount */
    if (dfs_mount("sd0", "/sdcard", "elm", 0, 0) == RT_EOK)
    {
        LOG_I("sd card mount to '/sdcard' success!");
        return;
    }

    /* Mount failed, try to format */
    LOG_W("sd card mount to '/sdcard' failed, try to mkfs...");
    if (dfs_mkfs("elm", "sd0") == 0)
    {
        LOG_I("sd card mkfs success!");

        /* Try to mount again after formatting */
        if (dfs_mount("sd0", "/sdcard", "elm", 0, 0) == RT_EOK)
        {
            LOG_I("sd card mount to '/sdcard' success!");
        }
        else
        {
            LOG_E("sd card mount to '/sdcard' failed after mkfs!");
        }
    }
    else
    {
        LOG_E("sd card mkfs failed!");
    }
}

static void sd_mount_thread(void *parameter)
{
    rt_thread_mdelay(200);
    _sdcard_mount();
}

int mnt_init(void)
{
    rt_thread_t tid;

    if (dfs_mount(RT_NULL, "/", "rom", 0, &(romfs_root)) != 0)
    {
        LOG_E("rom mount to '/' failed!");
        return -RT_ERROR;
    }

    /* Create a separate thread for SD card mounting to avoid blocking system startup */
    tid = rt_thread_create("sd_mount", sd_mount_thread, RT_NULL,
                           2048, RT_THREAD_PRIORITY_MAX - 2, 20);
    if (tid != RT_NULL)
    {
        rt_thread_startup(tid);
    }
    else
    {
        LOG_E("create sd_mount thread err!");
    }

    return RT_EOK;
}
INIT_APP_EXPORT(mnt_init);

#endif /* RT_USING_DFS */
