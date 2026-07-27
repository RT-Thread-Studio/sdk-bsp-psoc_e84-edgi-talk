/*
 * demo_lsm6ds3.c - LSM6DS3TR-C accelerometer + gyroscope sample
 *
 * MSH command:
 *   demo_lsm6ds3 [count] [delay_ms]
 *
 * The original LSM6DS3 sample starts automatically and prints forever. This
 * demo keeps the sensor code manual so it can coexist with the other demos.
 */

#include <rtthread.h>
#include <rtdevice.h>
#include <stdlib.h>

#include "lsm6ds3tr-c_reg.h"

#ifndef DEMO_LSM6DS3_I2C_BUS_NAME
#define DEMO_LSM6DS3_I2C_BUS_NAME "i2c0"
#endif

#define DEMO_LSM6DS3_ADDR_LOW       0x6AU
#define DEMO_LSM6DS3_ADDR_HIGH      0x6BU
#define DEMO_LSM6DS3_DEFAULT_COUNT  5U
#define DEMO_LSM6DS3_DEFAULT_DELAY  500U
#define DEMO_LSM6DS3_READY_RETRY    50U

struct demo_lsm6ds3_bus
{
    struct rt_i2c_bus_device *bus;
    rt_uint16_t addr;
};

static struct demo_lsm6ds3_bus g_lsm6ds3_bus;
static stmdev_ctx_t g_lsm6ds3_ctx;
static rt_bool_t g_lsm6ds3_ready = RT_FALSE;

static int32_t demo_lsm6ds3_write(void *handle, uint8_t reg, uint8_t *bufp, uint16_t len)
{
    struct demo_lsm6ds3_bus *dev = (struct demo_lsm6ds3_bus *)handle;
    struct rt_i2c_msg msg;
    rt_uint8_t buf[32];

    if ((dev == RT_NULL) || (dev->bus == RT_NULL) || (len + 1U > sizeof(buf)))
    {
        return -RT_ERROR;
    }

    buf[0] = reg;
    rt_memcpy(&buf[1], bufp, len);

    msg.addr = dev->addr;
    msg.flags = RT_I2C_WR;
    msg.buf = buf;
    msg.len = (rt_uint16_t)(len + 1U);

    return (rt_i2c_transfer(dev->bus, &msg, 1) == 1) ? 0 : -RT_ERROR;
}

static int32_t demo_lsm6ds3_read(void *handle, uint8_t reg, uint8_t *bufp, uint16_t len)
{
    struct demo_lsm6ds3_bus *dev = (struct demo_lsm6ds3_bus *)handle;
    struct rt_i2c_msg msgs[2];

    if ((dev == RT_NULL) || (dev->bus == RT_NULL))
    {
        return -RT_ERROR;
    }

    msgs[0].addr = dev->addr;
    msgs[0].flags = RT_I2C_WR;
    msgs[0].buf = &reg;
    msgs[0].len = 1;

    msgs[1].addr = dev->addr;
    msgs[1].flags = RT_I2C_RD;
    msgs[1].buf = bufp;
    msgs[1].len = len;

    return (rt_i2c_transfer(dev->bus, msgs, 2) == 2) ? 0 : -RT_ERROR;
}

static void demo_lsm6ds3_delay(uint32_t ms)
{
    rt_thread_mdelay(ms);
}

static void demo_lsm6ds3_print_fixed(float value)
{
    int scaled;

    if (value >= 0.0f)
    {
        scaled = (int)(value * 100.0f + 0.5f);
        rt_kprintf("%d.%02d", scaled / 100, scaled % 100);
    }
    else
    {
        scaled = (int)(-value * 100.0f + 0.5f);
        rt_kprintf("-%d.%02d", scaled / 100, scaled % 100);
    }
}

static rt_err_t demo_lsm6ds3_probe_addr(rt_uint16_t addr)
{
    uint8_t whoami = 0;

    g_lsm6ds3_bus.addr = addr;
    if (lsm6ds3tr_c_device_id_get(&g_lsm6ds3_ctx, &whoami) != 0)
    {
        return -RT_ERROR;
    }

    return (whoami == LSM6DS3TR_C_ID) ? RT_EOK : -RT_ERROR;
}

static rt_err_t demo_lsm6ds3_init(void)
{
    uint8_t rst = 0;
    rt_uint32_t retry;

    if (g_lsm6ds3_ready == RT_TRUE)
    {
        return RT_EOK;
    }

    g_lsm6ds3_bus.bus = (struct rt_i2c_bus_device *)rt_device_find(DEMO_LSM6DS3_I2C_BUS_NAME);
    if (g_lsm6ds3_bus.bus == RT_NULL)
    {
        rt_kprintf("LSM6DS3: I2C bus '%s' not found\n", DEMO_LSM6DS3_I2C_BUS_NAME);
        rt_kprintf("Check BSP_USING_I2C + BSP_USING_HW_I2C0 configuration.\n");
        return -RT_ERROR;
    }

    g_lsm6ds3_ctx.write_reg = demo_lsm6ds3_write;
    g_lsm6ds3_ctx.read_reg = demo_lsm6ds3_read;
    g_lsm6ds3_ctx.mdelay = demo_lsm6ds3_delay;
    g_lsm6ds3_ctx.handle = &g_lsm6ds3_bus;

    if ((demo_lsm6ds3_probe_addr(DEMO_LSM6DS3_ADDR_LOW) != RT_EOK) &&
        (demo_lsm6ds3_probe_addr(DEMO_LSM6DS3_ADDR_HIGH) != RT_EOK))
    {
        rt_kprintf("LSM6DS3: device not found on %s (addr 0x%02X/0x%02X)\n",
                   DEMO_LSM6DS3_I2C_BUS_NAME,
                   DEMO_LSM6DS3_ADDR_LOW,
                   DEMO_LSM6DS3_ADDR_HIGH);
        return -RT_ERROR;
    }

    if (lsm6ds3tr_c_reset_set(&g_lsm6ds3_ctx, PROPERTY_ENABLE) != 0)
    {
        return -RT_ERROR;
    }

    for (retry = 0; retry < 100U; retry++)
    {
        if (lsm6ds3tr_c_reset_get(&g_lsm6ds3_ctx, &rst) != 0)
        {
            return -RT_ERROR;
        }
        if (rst == 0U)
        {
            break;
        }
        rt_thread_mdelay(1);
    }

    if (rst != 0U)
    {
        rt_kprintf("LSM6DS3: reset timeout\n");
        return -RT_ETIMEOUT;
    }

    lsm6ds3tr_c_block_data_update_set(&g_lsm6ds3_ctx, PROPERTY_ENABLE);
    lsm6ds3tr_c_xl_data_rate_set(&g_lsm6ds3_ctx, LSM6DS3TR_C_XL_ODR_52Hz);
    lsm6ds3tr_c_gy_data_rate_set(&g_lsm6ds3_ctx, LSM6DS3TR_C_GY_ODR_52Hz);
    lsm6ds3tr_c_xl_full_scale_set(&g_lsm6ds3_ctx, LSM6DS3TR_C_2g);
    lsm6ds3tr_c_gy_full_scale_set(&g_lsm6ds3_ctx, LSM6DS3TR_C_2000dps);
    lsm6ds3tr_c_xl_filter_analog_set(&g_lsm6ds3_ctx, LSM6DS3TR_C_XL_ANA_BW_400Hz);
    lsm6ds3tr_c_xl_lp2_bandwidth_set(&g_lsm6ds3_ctx, LSM6DS3TR_C_XL_LOW_NOISE_LP_ODR_DIV_100);
    lsm6ds3tr_c_gy_band_pass_set(&g_lsm6ds3_ctx, LSM6DS3TR_C_HP_260mHz_LP1_STRONG);

    g_lsm6ds3_ready = RT_TRUE;
    rt_kprintf("LSM6DS3: found on %s addr 0x%02X\n",
               DEMO_LSM6DS3_I2C_BUS_NAME,
               g_lsm6ds3_bus.addr);
    return RT_EOK;
}

static rt_err_t demo_lsm6ds3_wait_ready(lsm6ds3tr_c_status_reg_t *status)
{
    rt_uint32_t retry;

    for (retry = 0; retry < DEMO_LSM6DS3_READY_RETRY; retry++)
    {
        if (lsm6ds3tr_c_status_reg_get(&g_lsm6ds3_ctx, status) != 0)
        {
            return -RT_ERROR;
        }

        if ((status->xlda != 0U) && (status->gda != 0U))
        {
            return RT_EOK;
        }

        rt_thread_mdelay(10);
    }

    return -RT_ETIMEOUT;
}

static rt_err_t demo_lsm6ds3_read_once(void)
{
    lsm6ds3tr_c_status_reg_t status;
    int16_t raw_acc[3] = {0};
    int16_t raw_gyr[3] = {0};
    int16_t raw_temp = 0;
    float acc_mg[3];
    float gyr_mdps[3];
    float temp_c;

    if (demo_lsm6ds3_wait_ready(&status) != RT_EOK)
    {
        rt_kprintf("LSM6DS3: data not ready\n");
        return -RT_ETIMEOUT;
    }

    if ((lsm6ds3tr_c_acceleration_raw_get(&g_lsm6ds3_ctx, raw_acc) != 0) ||
        (lsm6ds3tr_c_angular_rate_raw_get(&g_lsm6ds3_ctx, raw_gyr) != 0) ||
        (lsm6ds3tr_c_temperature_raw_get(&g_lsm6ds3_ctx, &raw_temp) != 0))
    {
        rt_kprintf("LSM6DS3: read data failed\n");
        return -RT_ERROR;
    }

    acc_mg[0] = lsm6ds3tr_c_from_fs2g_to_mg(raw_acc[0]);
    acc_mg[1] = lsm6ds3tr_c_from_fs2g_to_mg(raw_acc[1]);
    acc_mg[2] = lsm6ds3tr_c_from_fs2g_to_mg(raw_acc[2]);
    gyr_mdps[0] = lsm6ds3tr_c_from_fs2000dps_to_mdps(raw_gyr[0]);
    gyr_mdps[1] = lsm6ds3tr_c_from_fs2000dps_to_mdps(raw_gyr[1]);
    gyr_mdps[2] = lsm6ds3tr_c_from_fs2000dps_to_mdps(raw_gyr[2]);
    temp_c = lsm6ds3tr_c_from_lsb_to_celsius(raw_temp);

    rt_kprintf("Acceleration [mg]: ");
    demo_lsm6ds3_print_fixed(acc_mg[0]);
    rt_kprintf("\t");
    demo_lsm6ds3_print_fixed(acc_mg[1]);
    rt_kprintf("\t");
    demo_lsm6ds3_print_fixed(acc_mg[2]);
    rt_kprintf("\n");

    rt_kprintf("Angular rate [mdps]: ");
    demo_lsm6ds3_print_fixed(gyr_mdps[0]);
    rt_kprintf("\t");
    demo_lsm6ds3_print_fixed(gyr_mdps[1]);
    rt_kprintf("\t");
    demo_lsm6ds3_print_fixed(gyr_mdps[2]);
    rt_kprintf("\n");

    rt_kprintf("Temperature [degC]: ");
    demo_lsm6ds3_print_fixed(temp_c);
    rt_kprintf("\n");

    return RT_EOK;
}

static int demo_lsm6ds3(int argc, char **argv)
{
    rt_uint32_t count = DEMO_LSM6DS3_DEFAULT_COUNT;
    rt_uint32_t delay_ms = DEMO_LSM6DS3_DEFAULT_DELAY;
    rt_uint32_t i;

    if (argc > 1)
    {
        count = (rt_uint32_t)strtoul(argv[1], RT_NULL, 0);
    }
    if (argc > 2)
    {
        delay_ms = (rt_uint32_t)strtoul(argv[2], RT_NULL, 0);
    }
    if (argc > 3)
    {
        rt_kprintf("Usage: demo_lsm6ds3 [count] [delay_ms]\n");
        return -RT_ERROR;
    }

    if (demo_lsm6ds3_init() != RT_EOK)
    {
        g_lsm6ds3_ready = RT_FALSE;
        return -RT_ERROR;
    }

    rt_kprintf("LSM6DS3 sample: count=%u, delay=%u ms\n", count, delay_ms);

    for (i = 0; (count == 0U) || (i < count); i++)
    {
        if (demo_lsm6ds3_read_once() != RT_EOK)
        {
            g_lsm6ds3_ready = RT_FALSE;
            return -RT_ERROR;
        }

        if ((count == 0U) || (i + 1U < count))
        {
            rt_thread_mdelay(delay_ms);
        }
    }

    return RT_EOK;
}
MSH_CMD_EXPORT(demo_lsm6ds3, read LSM6DS3 accel gyro temperature);
