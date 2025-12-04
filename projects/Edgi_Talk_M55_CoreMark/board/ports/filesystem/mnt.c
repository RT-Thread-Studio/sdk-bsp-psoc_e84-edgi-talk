#include <rtthread.h>

#ifdef RT_USING_DFS
#include <dfs_fs.h>

int mnt_init(void)
{
    rt_err_t ret;

    rt_thread_mdelay(200);

    if (dfs_mount("sd0", "/", "elm", 0, 0) != 0)
    {
        rt_kprintf("Dir / mount failed!\n");
        return -1;
    }

    return 0;
}
INIT_ENV_EXPORT(mnt_init);

#endif
