#include <rtthread.h>

#ifdef RT_USING_DFS
#include <dfs_fs.h>

int mnt_init(void)
{
    rt_device_t device;

    rt_thread_mdelay(500);

    /* 检测sd0设备是否存在 */
    device = rt_device_find("sd0");
    if (device == RT_NULL)
    {
        rt_kprintf("SD card device 'sd0' not found!\n");
        return -1;
    }

    /* 尝试挂载SD卡 */
    if (dfs_mount("sd0", "/", "elm", 0, 0) == 0)
    {
        rt_kprintf("SD card mount to '/' success!\n");
        return 0;
    }

    /* 挂载失败，尝试格式化 */
    rt_kprintf("SD card mount failed, try to mkfs...\n");
    if (dfs_mkfs("elm", "sd0") == 0)
    {
        rt_kprintf("SD card mkfs success!\n");

        /* 格式化成功后再次尝试挂载 */
        if (dfs_mount("sd0", "/", "elm", 0, 0) == 0)
        {
            rt_kprintf("SD card mount to '/' success!\n");
            return 0;
        }
    }

    rt_kprintf("SD card mount to '/' failed!\n");
    return -1;
}
INIT_APP_EXPORT(mnt_init);

#endif
