/*
 * demo_aht20.c - AHT10/AHT20 humidity + temperature sensor sample
 *
 * MSH command: demo_aht20
 *   Reads humidity + temperature from the AHT10 sensor on i2c1 once and
 *   prints both. The sensor handle is cached across calls so re-running
 *   the command doesn't re-initialize the device.
 *
 * NOTE: This replaces the auto-start loop thread that ships with the
 * aht10 package sample. To prevent the loop thread from running, disable
 * PKG_USING_AHT10_SAMPLE in menuconfig (RT-Thread online packages â†? * peripheral libraries â†?aht10 â†?using aht10 sample).
 *
 * Kconfig: PKG_USING_AHT10 (already enabled in this project)
 */

#include <rtthread.h>
#include "aht10.h"

#ifndef PKG_AHT10_I2C_BUS_NAME
#define PKG_AHT10_I2C_BUS_NAME "i2c1"
#endif

static aht10_device_t s_aht10_dev = RT_NULL;

static int demo_aht20(int argc, char *argv[])
{
    if (s_aht10_dev == RT_NULL)
    {
        s_aht10_dev = aht10_init(PKG_AHT10_I2C_BUS_NAME);
        if (s_aht10_dev == RT_NULL)
        {
            rt_kprintf("AHT10 init failed (i2c bus '%s')\n",
                       PKG_AHT10_I2C_BUS_NAME);
            return -RT_ERROR;
        }
    }

    float humidity    = aht10_read_humidity(s_aht10_dev);
    float temperature = aht10_read_temperature(s_aht10_dev);

    rt_kprintf("AHT10: temp %d.%d C, humidity %d.%d %%\n",
               (int)temperature, (int)(temperature * 10) % 10,
               (int)humidity,    (int)(humidity * 10) % 10);
    return RT_EOK;
}
MSH_CMD_EXPORT(demo_aht20, read AHT10 temperature and humidity);
