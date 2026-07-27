/*
 * demo_rtc.c - RTC sample (ported from Edgi_Talk_M33_RTC)
 *
 * MSH commands:
 *   demo_rtc                     - print current time
 *   demo_rtc YYYY MM DD HH MM SS - set time, then print it
 *
 * Kconfig: RT_USING_RTC (already enabled) + BSP_USING_RTC (must enable).
 */

#include <rtthread.h>
#include <rtdevice.h>
#include <stdlib.h>
#include <time.h>

#ifdef RT_USING_RTC
static int demo_rtc(int argc, char *argv[])
{
    if (argc == 7)
    {
        int year  = atoi(argv[1]);
        int month = atoi(argv[2]);
        int day   = atoi(argv[3]);
        int hour  = atoi(argv[4]);
        int min   = atoi(argv[5]);
        int sec   = atoi(argv[6]);

        if (set_date(year, month, day) != RT_EOK)
        {
            rt_kprintf("set_date failed\n");
            return -RT_ERROR;
        }
        if (set_time(hour, min, sec) != RT_EOK)
        {
            rt_kprintf("set_time failed\n");
            return -RT_ERROR;
        }
        rt_kprintf("RTC set: %04d-%02d-%02d %02d:%02d:%02d\n",
                   year, month, day, hour, min, sec);
    }
    else if (argc == 1)
    {
        time_t now = time(RT_NULL);
        rt_kprintf("%s\n", ctime(&now));
    }
    else
    {
        rt_kprintf("Usage:\n");
        rt_kprintf("  demo_rtc                      - read current time\n");
        rt_kprintf("  demo_rtc YYYY MM DD HH MM SS  - set time\n");
        return -RT_ERROR;
    }
    return RT_EOK;
}
#else
static int demo_rtc(int argc, char *argv[])
{
    rt_kprintf("RTC framework not enabled. Enable in menuconfig:\n");
    rt_kprintf("  BSP_USING_RTC (RT_USING_RTC is already on in this project)\n");
    return -RT_ERROR;
}
#endif
MSH_CMD_EXPORT(demo_rtc, rtc set or read);
