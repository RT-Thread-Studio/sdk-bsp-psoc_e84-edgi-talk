/* Includes ------------------------------------------------------------------*/
#include "lsm6ds3tr-c_reg.h"
#include "lsm6ds3tr-c_port.h"
#include <string.h>
#include <stdio.h>
#include <rtthread.h>
#include <rtdevice.h>

/* Private macro -------------------------------------------------------------*/
#define    LSM6DS3_I2C_BUS_NAME "i2c0"
#define    LSM6DS3_I2C_ADDR     (LSM6DS3TR_C_I2C_ADD_L >> 1)
/* Private variables ---------------------------------------------------------*/
static int16_t data_raw_acceleration[3];
static int16_t data_raw_angular_rate[3];
static int16_t data_raw_temperature;
static float_t acceleration_mg[3];
static float_t angular_rate_mdps[3];
static float_t temperature_degC;
static uint8_t whoamI, rst;
static stmdev_ctx_t g_dev_ctx;
static rt_bool_t g_lsm6ds3tr_ready = RT_FALSE;
static rt_mutex_t g_lsm6ds3tr_lock = RT_NULL;
static lsm6ds3tr_sample_t g_sample_cache;
static lsm6ds3tr_port_config_t g_active_config = {
    .sample_rate_hz = LSM6DS3TR_DEFAULT_RATE_HZ,
    .accel_range_g = LSM6DS3TR_DEFAULT_ACCEL_RANGE_G,
    .gyro_range_dps = LSM6DS3TR_DEFAULT_GYRO_RANGE_DPS,
};
static struct rt_i2c_bus_device *i2c_bus = RT_NULL;

/* Extern variables ----------------------------------------------------------*/

/* Private functions ---------------------------------------------------------*/

/*
 *   WARNING:
 *   Functions declare in this section are defined at the end of this file
 *   and are strictly related to the hardware platform used.
 *
 */
static int32_t platform_write(void *handle, uint8_t reg, uint8_t *bufp,
                              uint16_t len);
static int32_t platform_read(void *handle, uint8_t reg, uint8_t *bufp,
                             uint16_t len);
static void platform_delay(uint32_t ms);
static void platform_init(void);

static lsm6ds3tr_c_odr_xl_t lsm6ds3tr_map_xl_odr(int sample_rate_hz)
{
    if (sample_rate_hz <= 13) return LSM6DS3TR_C_XL_ODR_12Hz5;
    if (sample_rate_hz <= 26) return LSM6DS3TR_C_XL_ODR_26Hz;
    if (sample_rate_hz <= 52) return LSM6DS3TR_C_XL_ODR_52Hz;
    if (sample_rate_hz <= 104) return LSM6DS3TR_C_XL_ODR_104Hz;
    if (sample_rate_hz <= 208) return LSM6DS3TR_C_XL_ODR_208Hz;
    if (sample_rate_hz <= 416) return LSM6DS3TR_C_XL_ODR_416Hz;
    return LSM6DS3TR_C_XL_ODR_833Hz;
}

static lsm6ds3tr_c_odr_g_t lsm6ds3tr_map_gy_odr(int sample_rate_hz)
{
    if (sample_rate_hz <= 13) return LSM6DS3TR_C_GY_ODR_12Hz5;
    if (sample_rate_hz <= 26) return LSM6DS3TR_C_GY_ODR_26Hz;
    if (sample_rate_hz <= 52) return LSM6DS3TR_C_GY_ODR_52Hz;
    if (sample_rate_hz <= 104) return LSM6DS3TR_C_GY_ODR_104Hz;
    if (sample_rate_hz <= 208) return LSM6DS3TR_C_GY_ODR_208Hz;
    if (sample_rate_hz <= 416) return LSM6DS3TR_C_GY_ODR_416Hz;
    return LSM6DS3TR_C_GY_ODR_833Hz;
}

static lsm6ds3tr_c_fs_xl_t lsm6ds3tr_map_xl_fs(int accel_range_g)
{
    switch (accel_range_g)
    {
    case 2: return LSM6DS3TR_C_2g;
    case 4: return LSM6DS3TR_C_4g;
    case 8: return LSM6DS3TR_C_8g;
    case 16: return LSM6DS3TR_C_16g;
    default: return LSM6DS3TR_C_XL_FS_ND;
    }
}

static lsm6ds3tr_c_fs_g_t lsm6ds3tr_map_gy_fs(int gyro_range_dps)
{
    switch (gyro_range_dps)
    {
    case 125: return LSM6DS3TR_C_125dps;
    case 250: return LSM6DS3TR_C_250dps;
    case 500: return LSM6DS3TR_C_500dps;
    case 1000: return LSM6DS3TR_C_1000dps;
    case 2000: return LSM6DS3TR_C_2000dps;
    default: return LSM6DS3TR_C_GY_FS_ND;
    }
}

static float_t lsm6ds3tr_convert_acc_mg(int16_t raw)
{
    switch (g_active_config.accel_range_g)
    {
    case 4: return lsm6ds3tr_c_from_fs4g_to_mg(raw);
    case 8: return lsm6ds3tr_c_from_fs8g_to_mg(raw);
    case 16: return lsm6ds3tr_c_from_fs16g_to_mg(raw);
    case 2:
    default: return lsm6ds3tr_c_from_fs2g_to_mg(raw);
    }
}

static float_t lsm6ds3tr_convert_gyro_mdps(int16_t raw)
{
    switch (g_active_config.gyro_range_dps)
    {
    case 125: return lsm6ds3tr_c_from_fs125dps_to_mdps(raw);
    case 250: return lsm6ds3tr_c_from_fs250dps_to_mdps(raw);
    case 500: return lsm6ds3tr_c_from_fs500dps_to_mdps(raw);
    case 1000: return lsm6ds3tr_c_from_fs1000dps_to_mdps(raw);
    case 2000:
    default: return lsm6ds3tr_c_from_fs2000dps_to_mdps(raw);
    }
}

static int lsm6ds3tr_port_apply_config_locked(const lsm6ds3tr_port_config_t *config)
{
    lsm6ds3tr_c_fs_xl_t xl_fs;
    lsm6ds3tr_c_fs_g_t gy_fs;
    lsm6ds3tr_c_odr_xl_t xl_odr;
    lsm6ds3tr_c_odr_g_t gy_odr;

    if (config == RT_NULL)
    {
        return -RT_EINVAL;
    }

    xl_fs = lsm6ds3tr_map_xl_fs(config->accel_range_g);
    gy_fs = lsm6ds3tr_map_gy_fs(config->gyro_range_dps);
    xl_odr = lsm6ds3tr_map_xl_odr(config->sample_rate_hz);
    gy_odr = lsm6ds3tr_map_gy_odr(config->sample_rate_hz);

    if (xl_fs == LSM6DS3TR_C_XL_FS_ND || gy_fs == LSM6DS3TR_C_GY_FS_ND)
    {
        return -RT_EINVAL;
    }

    if (lsm6ds3tr_c_xl_data_rate_set(&g_dev_ctx, xl_odr) != 0)
    {
        return -RT_ERROR;
    }
    if (lsm6ds3tr_c_gy_data_rate_set(&g_dev_ctx, gy_odr) != 0)
    {
        return -RT_ERROR;
    }
    if (lsm6ds3tr_c_xl_full_scale_set(&g_dev_ctx, xl_fs) != 0)
    {
        return -RT_ERROR;
    }
    if (lsm6ds3tr_c_gy_full_scale_set(&g_dev_ctx, gy_fs) != 0)
    {
        return -RT_ERROR;
    }

    g_active_config = *config;
    g_sample_cache.valid_mask = 0;
    return RT_EOK;
}

static int lsm6ds3tr_port_ensure_lock(void)
{
    if (g_lsm6ds3tr_lock == RT_NULL)
    {
        g_lsm6ds3tr_lock = rt_mutex_create("lsm6ds3", RT_IPC_FLAG_PRIO);
        if (g_lsm6ds3tr_lock == RT_NULL)
        {
            return -RT_ENOMEM;
        }
    }

    return RT_EOK;
}

/* Main Example --------------------------------------------------------------*/
int lsm6ds3tr_port_init(void)
{
    int result = RT_EOK;

    result = lsm6ds3tr_port_ensure_lock();
    if (result != RT_EOK)
    {
        return result;
    }

    rt_mutex_take(g_lsm6ds3tr_lock, RT_WAITING_FOREVER);

    if (g_lsm6ds3tr_ready)
    {
        rt_mutex_release(g_lsm6ds3tr_lock);
        return RT_EOK;
    }

    /* Initialize mems driver interface */
    g_dev_ctx.write_reg = platform_write;
    g_dev_ctx.read_reg = platform_read;
    g_dev_ctx.mdelay = platform_delay;
    g_dev_ctx.handle = RT_NULL;
    /* Init test platform */
    platform_init();
    if (i2c_bus == RT_NULL)
    {
        result = -RT_ERROR;
        goto exit;
    }

    /* Check device ID */
    whoamI = 0;
    lsm6ds3tr_c_device_id_get(&g_dev_ctx, &whoamI);

    if (whoamI != LSM6DS3TR_C_ID)
    {
        rt_kprintf("lsm6ds3tr: whoami mismatch: 0x%02X\r\n", whoamI);
        result = -RT_ERROR;
        goto exit;
    }

    /* Restore default configuration */
    lsm6ds3tr_c_reset_set(&g_dev_ctx, PROPERTY_ENABLE);

    do
    {
        lsm6ds3tr_c_reset_get(&g_dev_ctx, &rst);
    }
    while (rst);

    /* Enable Block Data Update */
    lsm6ds3tr_c_block_data_update_set(&g_dev_ctx, PROPERTY_ENABLE);
    /* Apply default rate/range configuration after reset. */
    result = lsm6ds3tr_port_apply_config_locked(&g_active_config);
    if (result != RT_EOK)
    {
        goto exit;
    }
    /* Configure filtering chain(No aux interface) */
    /* Accelerometer - analog filter */
    lsm6ds3tr_c_xl_filter_analog_set(&g_dev_ctx,
                                     LSM6DS3TR_C_XL_ANA_BW_400Hz);
    /* Accelerometer - LPF1 path ( LPF2 not used )*/
    //lsm6ds3tr_c_xl_lp1_bandwidth_set(&dev_ctx, LSM6DS3TR_C_XL_LP1_ODR_DIV_4);
    /* Accelerometer - LPF1 + LPF2 path */
    lsm6ds3tr_c_xl_lp2_bandwidth_set(&g_dev_ctx,
                                     LSM6DS3TR_C_XL_LOW_NOISE_LP_ODR_DIV_100);
    /* Accelerometer - High Pass / Slope path */
    //lsm6ds3tr_c_xl_reference_mode_set(&dev_ctx, PROPERTY_DISABLE);
    //lsm6ds3tr_c_xl_hp_bandwidth_set(&dev_ctx, LSM6DS3TR_C_XL_HP_ODR_DIV_100);
    /* Gyroscope - filtering chain */
    lsm6ds3tr_c_gy_band_pass_set(&g_dev_ctx,
                                 LSM6DS3TR_C_HP_260mHz_LP1_STRONG);
    g_lsm6ds3tr_ready = RT_TRUE;
    rt_memset(&g_sample_cache, 0, sizeof(g_sample_cache));
    rt_kprintf("lsm6ds3tr: init done on %s\r\n", LSM6DS3_I2C_BUS_NAME);

exit:
    rt_mutex_release(g_lsm6ds3tr_lock);
    return result;
}

bool lsm6ds3tr_port_is_ready(void)
{
    return (g_lsm6ds3tr_ready == RT_TRUE);
}

int lsm6ds3tr_port_configure(const lsm6ds3tr_port_config_t *config)
{
    int result;
    lsm6ds3tr_port_config_t normalized;

    if (config == RT_NULL)
    {
        return -RT_EINVAL;
    }

    normalized = *config;
    if (normalized.sample_rate_hz <= 0)
    {
        normalized.sample_rate_hz = LSM6DS3TR_DEFAULT_RATE_HZ;
    }

    result = lsm6ds3tr_port_init();
    if (result != RT_EOK)
    {
        return result;
    }

    result = lsm6ds3tr_port_ensure_lock();
    if (result != RT_EOK)
    {
        return result;
    }

    rt_mutex_take(g_lsm6ds3tr_lock, RT_WAITING_FOREVER);
    result = lsm6ds3tr_port_apply_config_locked(&normalized);
    rt_mutex_release(g_lsm6ds3tr_lock);

    return result;
}

int lsm6ds3tr_port_read_sample(lsm6ds3tr_sample_t *sample)
{
    lsm6ds3tr_c_reg_t reg;
    uint8_t new_valid_mask = 0;

    if (sample == RT_NULL)
    {
        return -RT_EINVAL;
    }

    if (!lsm6ds3tr_port_is_ready())
    {
        return -RT_ERROR;
    }

    if (lsm6ds3tr_port_ensure_lock() != RT_EOK)
    {
        return -RT_ENOMEM;
    }

    rt_mutex_take(g_lsm6ds3tr_lock, RT_WAITING_FOREVER);

    lsm6ds3tr_c_status_reg_get(&g_dev_ctx, &reg.status_reg);

    if (reg.status_reg.xlda)
    {
        rt_memset(data_raw_acceleration, 0x00, 3 * sizeof(int16_t));
        lsm6ds3tr_c_acceleration_raw_get(&g_dev_ctx, data_raw_acceleration);
        acceleration_mg[0] = lsm6ds3tr_convert_acc_mg(data_raw_acceleration[0]);
        acceleration_mg[1] = lsm6ds3tr_convert_acc_mg(data_raw_acceleration[1]);
        acceleration_mg[2] = lsm6ds3tr_convert_acc_mg(data_raw_acceleration[2]);

        g_sample_cache.acc_mg[0] = acceleration_mg[0];
        g_sample_cache.acc_mg[1] = acceleration_mg[1];
        g_sample_cache.acc_mg[2] = acceleration_mg[2];
        new_valid_mask |= LSM6DS3TR_SAMPLE_VALID_ACC;
    }

    if (reg.status_reg.gda)
    {
        rt_memset(data_raw_angular_rate, 0x00, 3 * sizeof(int16_t));
        lsm6ds3tr_c_angular_rate_raw_get(&g_dev_ctx, data_raw_angular_rate);
        angular_rate_mdps[0] = lsm6ds3tr_convert_gyro_mdps(data_raw_angular_rate[0]);
        angular_rate_mdps[1] = lsm6ds3tr_convert_gyro_mdps(data_raw_angular_rate[1]);
        angular_rate_mdps[2] = lsm6ds3tr_convert_gyro_mdps(data_raw_angular_rate[2]);

        g_sample_cache.gyro_mdps[0] = angular_rate_mdps[0];
        g_sample_cache.gyro_mdps[1] = angular_rate_mdps[1];
        g_sample_cache.gyro_mdps[2] = angular_rate_mdps[2];
        new_valid_mask |= LSM6DS3TR_SAMPLE_VALID_GYRO;
    }

    if (reg.status_reg.tda)
    {
        rt_memset(&data_raw_temperature, 0x00, sizeof(int16_t));
        lsm6ds3tr_c_temperature_raw_get(&g_dev_ctx, &data_raw_temperature);
        temperature_degC = lsm6ds3tr_c_from_lsb_to_celsius(data_raw_temperature);

        g_sample_cache.temp_degC = temperature_degC;
        new_valid_mask |= LSM6DS3TR_SAMPLE_VALID_TEMP;
    }

    g_sample_cache.valid_mask |= new_valid_mask;

    rt_memcpy(sample, &g_sample_cache, sizeof(*sample));

    rt_mutex_release(g_lsm6ds3tr_lock);
    return RT_EOK;
}

int lsm6ds3tr_c_read_data_sample(void)
{
    return lsm6ds3tr_port_init();
}
INIT_APP_EXPORT(lsm6ds3tr_c_read_data_sample);

static int lsm6ds3tr_c_dump_once(void)
{
    lsm6ds3tr_sample_t sample;

    if (lsm6ds3tr_port_read_sample(&sample) != RT_EOK)
    {
        rt_kprintf("lsm6ds3tr: not ready\r\n");
        return -RT_ERROR;
    }

    if (sample.valid_mask & LSM6DS3TR_SAMPLE_VALID_ACC)
    {
        rt_kprintf("lsm6ds3tr acc [mg]: %4.2f %4.2f %4.2f\r\n",
                   sample.acc_mg[0], sample.acc_mg[1], sample.acc_mg[2]);
    }

    if (sample.valid_mask & LSM6DS3TR_SAMPLE_VALID_GYRO)
    {
        rt_kprintf("lsm6ds3tr gyr [mdps]: %4.2f %4.2f %4.2f\r\n",
                   sample.gyro_mdps[0], sample.gyro_mdps[1], sample.gyro_mdps[2]);
    }

    if (sample.valid_mask & LSM6DS3TR_SAMPLE_VALID_TEMP)
    {
        rt_kprintf("lsm6ds3tr temp [degC]: %6.2f\r\n", sample.temp_degC);
    }

    return RT_EOK;
}
MSH_CMD_EXPORT_ALIAS(lsm6ds3tr_c_dump_once, lsm6ds3_sample, dump one LSM6DS3 sample);

/*
 * @brief  Write generic device register (platform dependent)
 *
 * @param  handle    customizable argument. In this examples is used in
 *                   order to select the correct sensor bus handler.
 * @param  reg       register to write
 * @param  bufp      pointer to data to write in register reg
 * @param  len       number of consecutive register to write
 *
 */
static int32_t platform_write(void *handle, uint8_t reg, uint8_t *bufp,
                              uint16_t len)
{
    struct rt_i2c_msg msg;
    rt_uint8_t buf[1 + len];

    buf[0] = reg;
    rt_memcpy(&buf[1], bufp, len);

    msg.addr  = LSM6DS3_I2C_ADDR;
    msg.flags = RT_I2C_WR;
    msg.buf   = buf;
    msg.len   = len + 1;

    if (rt_i2c_transfer(i2c_bus, &msg, 1) == 1)
    {
        return RT_EOK;
    }
    else
    {
        return -RT_ERROR;
    }
}


/*
 * @brief  Read generic device register (platform dependent)
 *
 * @param  handle    customizable argument. In this examples is used in
 *                   order to select the correct sensor bus handler.
 * @param  reg       register to read
 * @param  bufp      pointer to buffer that store the data read
 * @param  len       number of consecutive register to read
 *
 */
static int32_t platform_read(void *handle, uint8_t reg, uint8_t *bufp,
                             uint16_t len)
{
    struct rt_i2c_msg msgs[2];

    msgs[0].addr = LSM6DS3_I2C_ADDR;
    msgs[0].flags = RT_I2C_WR;
    msgs[0].buf = &reg;
    msgs[0].len = 1;

    msgs[1].addr = LSM6DS3_I2C_ADDR;
    msgs[1].flags = RT_I2C_RD;
    msgs[1].buf = bufp;
    msgs[1].len = len;

    if (rt_i2c_transfer(i2c_bus, msgs, 2) == 2)
    {
        return RT_EOK;
    }
    else
    {
        return -RT_ERROR;
    }
}

/*
 * @brief  platform specific delay (platform dependent)
 *
 * @param  ms        delay in ms
 *
 */
static void platform_delay(uint32_t ms)
{
    rt_thread_mdelay(ms);
}

/*
 * @brief  platform specific initialization (platform dependent)
 */
static void platform_init(void)
{
    i2c_bus = (struct rt_i2c_bus_device *)rt_device_find(LSM6DS3_I2C_BUS_NAME);
    if (i2c_bus == RT_NULL)
    {
        rt_kprintf("Error: I2C bus %s not found!\n", LSM6DS3_I2C_BUS_NAME);
    }
}
