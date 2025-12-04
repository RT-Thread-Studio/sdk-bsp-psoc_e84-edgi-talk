/* Includes ------------------------------------------------------------------*/
#include "lsm6ds3tr-c_reg.h"
#include <string.h>
#include <stdio.h>
#include <rtthread.h>
#include <rtdevice.h>

/* Private macro -------------------------------------------------------------*/
#define    LSM6DS3_I2C_BUS_NAME "i2c0"
/* Private variables ---------------------------------------------------------*/
static int16_t data_raw_acceleration[3];
static int16_t data_raw_angular_rate[3];
static int16_t data_raw_temperature;
static float_t acceleration_mg[3];
static float_t angular_rate_mdps[3];
static float_t temperature_degC;
static uint8_t whoamI, rst;
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

/* Main Example --------------------------------------------------------------*/
int lsm6ds3tr_c_read_data_sample(void)
{
    /* Initialize mems driver interface */
    stmdev_ctx_t dev_ctx;
    dev_ctx.write_reg = platform_write;
    dev_ctx.read_reg = platform_read;
    dev_ctx.mdelay = platform_delay;
    /* Init test platform */
    platform_init();
    /* Check device ID */
    whoamI = 0;
    lsm6ds3tr_c_device_id_get(&dev_ctx, &whoamI);

    if (whoamI != LSM6DS3TR_C_ID)
        while (1); /*manage here device not found */

    /* Restore default configuration */
    lsm6ds3tr_c_reset_set(&dev_ctx, PROPERTY_ENABLE);

    do
    {
        lsm6ds3tr_c_reset_get(&dev_ctx, &rst);
    }
    while (rst);

    /* Enable Block Data Update */
    lsm6ds3tr_c_block_data_update_set(&dev_ctx, PROPERTY_ENABLE);
    /* Set Output Data Rate */
    lsm6ds3tr_c_xl_data_rate_set(&dev_ctx, LSM6DS3TR_C_XL_ODR_12Hz5);
    lsm6ds3tr_c_gy_data_rate_set(&dev_ctx, LSM6DS3TR_C_GY_ODR_12Hz5);
    /* Set full scale */
    lsm6ds3tr_c_xl_full_scale_set(&dev_ctx, LSM6DS3TR_C_2g);
    lsm6ds3tr_c_gy_full_scale_set(&dev_ctx, LSM6DS3TR_C_2000dps);
    /* Configure filtering chain(No aux interface) */
    /* Accelerometer - analog filter */
    lsm6ds3tr_c_xl_filter_analog_set(&dev_ctx,
                                     LSM6DS3TR_C_XL_ANA_BW_400Hz);
    /* Accelerometer - LPF1 path ( LPF2 not used )*/
    //lsm6ds3tr_c_xl_lp1_bandwidth_set(&dev_ctx, LSM6DS3TR_C_XL_LP1_ODR_DIV_4);
    /* Accelerometer - LPF1 + LPF2 path */
    lsm6ds3tr_c_xl_lp2_bandwidth_set(&dev_ctx,
                                     LSM6DS3TR_C_XL_LOW_NOISE_LP_ODR_DIV_100);
    /* Accelerometer - High Pass / Slope path */
    //lsm6ds3tr_c_xl_reference_mode_set(&dev_ctx, PROPERTY_DISABLE);
    //lsm6ds3tr_c_xl_hp_bandwidth_set(&dev_ctx, LSM6DS3TR_C_XL_HP_ODR_DIV_100);
    /* Gyroscope - filtering chain */
    lsm6ds3tr_c_gy_band_pass_set(&dev_ctx,
                                 LSM6DS3TR_C_HP_260mHz_LP1_STRONG);
//  platform_write(i2c_bus,0x10,&buff,2);

    /* Read samples in polling mode (no int) */
    while (1)
    {
        /* Read output only if new value is available */
        lsm6ds3tr_c_reg_t reg;
        lsm6ds3tr_c_status_reg_get(&dev_ctx, &reg.status_reg);
        if (reg.status_reg.xlda)
        {
            /* Read magnetic field data */
            rt_memset(data_raw_acceleration, 0x00, 3 * sizeof(int16_t));
            lsm6ds3tr_c_acceleration_raw_get(&dev_ctx,
                                             data_raw_acceleration);
            acceleration_mg[0] = lsm6ds3tr_c_from_fs2g_to_mg(
                                     data_raw_acceleration[0]);
            acceleration_mg[1] = lsm6ds3tr_c_from_fs2g_to_mg(
                                     data_raw_acceleration[1]);
            acceleration_mg[2] = lsm6ds3tr_c_from_fs2g_to_mg(
                                     data_raw_acceleration[2]);
            rt_kprintf("Acceleration [mg]:%4.2f\t%4.2f\t%4.2f\r\n",
                       acceleration_mg[0], acceleration_mg[1], acceleration_mg[2]);

        }

        if (reg.status_reg.gda)
        {
            /* Read magnetic field data */
            rt_memset(data_raw_angular_rate, 0x00, 3 * sizeof(int16_t));
            lsm6ds3tr_c_angular_rate_raw_get(&dev_ctx,
                                             data_raw_angular_rate);
            angular_rate_mdps[0] = lsm6ds3tr_c_from_fs2000dps_to_mdps(
                                       data_raw_angular_rate[0]);
            angular_rate_mdps[1] = lsm6ds3tr_c_from_fs2000dps_to_mdps(
                                       data_raw_angular_rate[1]);
            angular_rate_mdps[2] = lsm6ds3tr_c_from_fs2000dps_to_mdps(
                                       data_raw_angular_rate[2]);
            rt_kprintf("Angular rate [mdps]:%4.2f\t%4.2f\t%4.2f\r\n",
                       angular_rate_mdps[0], angular_rate_mdps[1], angular_rate_mdps[2]);
        }

        if (reg.status_reg.tda)
        {
            /* Read temperature data */
            rt_memset(&data_raw_temperature, 0x00, sizeof(int16_t));
            lsm6ds3tr_c_temperature_raw_get(&dev_ctx, &data_raw_temperature);
            temperature_degC = lsm6ds3tr_c_from_lsb_to_celsius(
                                   data_raw_temperature);
            rt_kprintf("Temperature [degC]:%6.2f\r\n",
                       temperature_degC);
        }
        rt_thread_mdelay(500);
    }
}
INIT_APP_EXPORT(lsm6ds3tr_c_read_data_sample);

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

    msg.addr  = LSM6DS3TR_C_ID;
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

    msgs[0].addr = LSM6DS3TR_C_ID;
    msgs[0].flags = RT_I2C_WR;
    msgs[0].buf = &reg;
    msgs[0].len = 1;

    msgs[1].addr = LSM6DS3TR_C_ID;
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
