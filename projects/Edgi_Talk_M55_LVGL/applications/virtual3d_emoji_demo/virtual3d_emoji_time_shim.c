#include <rtthread.h>
#include <time.h>

/*
 * liblx_vglite.a calls time() internally. The default RT-Thread time()
 * queries an RTC device and prints a warning when RTC is disabled, so provide a
 * monotonic wall-clock substitute for this demo.
 */
time_t time(time_t *t)
{
    const time_t base = (time_t)1767225600; /* 2026-01-01 00:00:00 UTC */
    time_t now = base + (time_t)(rt_tick_get_millisecond() / 1000U);

    if (t != RT_NULL)
    {
        *t = now;
    }

    return now;
}
