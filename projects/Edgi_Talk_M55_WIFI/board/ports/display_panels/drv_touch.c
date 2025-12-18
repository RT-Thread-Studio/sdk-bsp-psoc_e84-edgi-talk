/*
 * Copyright (c) 2006-2018, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2021-01-13     RiceChen     the first version
 */

#define DBG_TAG "ST7102"
#define DBG_LVL DBG_INFO
#include <rtdbg.h>
#include "drv_touch.h"

static struct rt_i2c_client ST7102_client;
rt_uint8_t cmd[2];
rt_uint8_t read_index;
int8_t read_id = 0;
rt_uint8_t point_status = 0;
rt_uint8_t touch_num = 0;
rt_uint8_t write_buf[3];
int16_t input_x = 0;
int16_t input_y = 0;
int16_t Last_input_x = 0;
int16_t Last_input_y = 0;
static uint16_t count = 0;
static uint16_t Last_Touch_Intn = 0;
static uint16_t Touch_Intn = 0;
rt_uint8_t read_buf[8 * ST7102_MAX_TOUCH] = {0};
rt_uint8_t Last_read_buf[8 * ST7102_MAX_TOUCH] = {0};


static rt_err_t ST7102_write_reg(struct rt_i2c_client *dev, rt_uint8_t *data, rt_uint8_t len)
{
    struct rt_i2c_msg msgs[2];

    msgs[0].addr = dev->client_addr;
    msgs[0].flags = RT_I2C_WR;
    msgs[0].buf = data;
    msgs[0].len = len;

    if (rt_i2c_transfer(dev->bus, msgs, 1) == 1)
    {
        return RT_EOK;
    }
    else
    {
        return -RT_ERROR;
    }
}

static rt_err_t ST7102_read_regs(struct rt_i2c_client *dev, rt_uint8_t *reg, rt_uint8_t *data, rt_uint8_t len)
{
    struct rt_i2c_msg msgs[2];

    msgs[0].addr = dev->client_addr;
    msgs[0].flags = RT_I2C_WR;
    msgs[0].buf = reg;
    msgs[0].len = ST7102_REGITER_LEN;

    msgs[1].addr = dev->client_addr;
    msgs[1].flags = RT_I2C_RD;
    msgs[1].buf = data;
    msgs[1].len = len;

    if (rt_i2c_transfer(dev->bus, msgs, 2) == 2)
    {
        return RT_EOK;
    }
    else
    {
        return -RT_ERROR;
    }
}

static rt_err_t ST7102_get_product_id(struct rt_i2c_client *dev, rt_uint8_t *data, rt_uint8_t len)
{
    rt_uint8_t reg[2];

    reg[0] = (rt_uint8_t)(ST7102_Producer_ID >> 8);
    reg[1] = (rt_uint8_t)(ST7102_Producer_ID & 0xff);

    if (ST7102_read_regs(dev, reg, data, len) != RT_EOK)
    {
        LOG_E("read id failed");
        return -RT_ERROR;
    }
    return RT_EOK;
}

static rt_err_t ST7102_get_info(struct rt_i2c_client *dev, struct rt_touch_info *info)
{
    rt_uint8_t Reg_High[2];
    rt_uint8_t Reg_Low[2];

    cmd[0] = (rt_uint8_t)((ST7102_MAX_X_Coord_High >> 8) & 0xFF);
    cmd[1] = (rt_uint8_t)(ST7102_MAX_X_Coord_High & 0xFF);
    ST7102_read_regs(dev, cmd, Reg_High, 1);

    cmd[0] = (rt_uint8_t)((ST7102_MAX_X_Coord_Low >> 8) & 0xFF);
    cmd[1] = (rt_uint8_t)(ST7102_MAX_X_Coord_Low & 0xFF);
    ST7102_read_regs(dev, cmd, Reg_Low, 1);
    info->range_x = (Reg_High[0] & 0x3F) << 8 | Reg_Low[0];

    cmd[0] = (rt_uint8_t)((ST7102_MAX_Y_Coord_High >> 8) & 0xFF);
    cmd[1] = (rt_uint8_t)(ST7102_MAX_Y_Coord_High & 0xFF);
    ST7102_read_regs(dev, cmd, Reg_High, 1);

    cmd[0] = (rt_uint8_t)((ST7102_MAX_Y_Coord_Low >> 8) & 0xFF);
    cmd[1] = (rt_uint8_t)(ST7102_MAX_Y_Coord_Low & 0xFF);
    ST7102_read_regs(dev, cmd, Reg_Low, 1);
    info->range_y = (Reg_High[0] & 0x3F) << 8 | Reg_Low[0];

    cmd[0] = (rt_uint8_t)((ST7102_MAX_Touches >> 8) & 0xFF);
    cmd[1] = (rt_uint8_t)(ST7102_MAX_Touches & 0xFF);
    ST7102_read_regs(dev, cmd, Reg_Low, 1);
    info->point_num = Reg_Low[0];

    return RT_EOK;
}

static rt_uint8_t buf[3] = {-1};
static rt_err_t ST7102_soft_reset(struct rt_i2c_client *dev)
{
    buf[0] = (rt_uint8_t)(ST7102_Device_Control >> 8);
    buf[1] = (rt_uint8_t)(ST7102_Device_Control & 0xFF);
    buf[2] = 0x01; // Reset CMD

    if (ST7102_write_reg(dev, buf, 3) != RT_EOK)
    {
        LOG_E("soft reset failed");
        return -RT_ERROR;
    }
    return RT_EOK;
}
MSH_CMD_EXPORT(ST7102_soft_reset, soft reset st7102);

static rt_size_t ST7102_read_point(struct rt_touch_device *touch, void *buf, rt_size_t read_num)
{
    /* point status register */
    cmd[0] = (rt_uint8_t)((ST7102_READ_STATUS >> 8) & 0xFF);
    cmd[1] = (rt_uint8_t)(ST7102_READ_STATUS & 0xFF);

    if (ST7102_read_regs(&ST7102_client, cmd, &point_status, 1) != RT_EOK)
    {
        LOG_D("read point failed\n");
        read_num = 0;
        goto exit_;
    }

    if ((point_status & 0x08) == 0) /* No Touch deceted */
    {
        read_num = 0;
        goto exit_;
    }

    cmd[0] = (rt_uint8_t)((ST7102_Read_Start_Position >> 8) & 0xFF);
    cmd[1] = (rt_uint8_t)(ST7102_Read_Start_Position & 0xFF);

    if (ST7102_read_regs(&ST7102_client, cmd, read_buf, 8 * ST7102_MAX_TOUCH) != RT_EOK)
    {
        LOG_D("read point failed\n");
        read_num = 0;
        goto exit_;
    }
    //Read all Point_Data
    for (count = 0; count < ST7102_MAX_TOUCH; count++)
    {
        if (read_buf[0x09 + count * 7] > 0 && read_buf[0] == 0x08)// Touch Detected
        {
            Last_input_x = (Last_read_buf[(7 * count) + 0x04] & 0x3F) << 8 | Last_read_buf[(7 * count) + 0x05];
            Last_input_y = (Last_read_buf[(7 * count) + 0x06] & 0x3F) << 8 | Last_read_buf[(7 * count) + 0x07];
            Last_Touch_Intn = Last_read_buf[(7 * count) + 0x09];

            input_x = (read_buf[(7 * count) + 0x04] & 0x3F) << 8 | read_buf[(7 * count) + 0x05];
            input_y = (read_buf[(7 * count) + 0x06] & 0x3F) << 8 | read_buf[(7 * count) + 0x07];
            Touch_Intn = read_buf[(7 * count) + 0x09];

            if (Last_input_x == input_x && Last_input_y == input_y && Last_Touch_Intn == Touch_Intn)// Touch the same Point
            {
                // break;
            }
            else
            {
                rt_kprintf("\nPoint_Count = %d, X_Coodr = %d, Y_Coodr = %d\n", count, input_x, input_y);
            }
        }
    }
    rt_memcpy(Last_read_buf, read_buf, 8 * ST7102_MAX_TOUCH);

exit_:


    return read_num;
}

static rt_err_t ST7102_control(struct rt_touch_device *touch, int cmd, void *arg)
{
    if (cmd == RT_TOUCH_CTRL_GET_ID)
    {
        return ST7102_get_product_id(&ST7102_client, arg, 2);
    }

    if (cmd == RT_TOUCH_CTRL_GET_INFO)
    {
        return ST7102_get_info(&ST7102_client, arg);
    }

    return RT_EOK;
}

static struct rt_touch_ops ST7102_touch_ops =
{
    .touch_readpoint = ST7102_read_point,
    .touch_control = ST7102_control,
};

int rt_hw_ST7102_init(const char *name, struct rt_touch_config *cfg)
{
    struct rt_touch_device *touch_device = RT_NULL;
    static int ret = 0;

    touch_device = (struct rt_touch_device *)rt_malloc(sizeof(struct rt_touch_device));
    if (touch_device == RT_NULL)
    {
        LOG_E("touch device malloc fail");
        return -RT_ERROR;
    }
    rt_memset((void *)touch_device, 0, sizeof(struct rt_touch_device));


    /* hw init*/
    rt_pin_mode(*(rt_uint8_t *)cfg->user_data, PIN_MODE_OUTPUT);// Reset Pin
    rt_pin_mode(cfg->irq_pin.pin, PIN_MODE_OUTPUT);
    rt_pin_write(cfg->irq_pin.pin, PIN_LOW);
    rt_pin_write(*(rt_uint8_t *)cfg->user_data, PIN_LOW);
    rt_thread_mdelay(10);

    rt_pin_write(*(rt_uint8_t *)cfg->user_data, PIN_HIGH);
    rt_thread_mdelay(10);
    rt_pin_mode(cfg->irq_pin.pin, PIN_MODE_INPUT_PULLUP);
    rt_pin_write(cfg->irq_pin.pin, PIN_HIGH);
    rt_thread_mdelay(10);

    ST7102_client.client_addr = ST7102_ADDRESS;

    ST7102_client.bus = (struct rt_i2c_bus_device *)rt_device_find(cfg->dev_name);

    if (ST7102_client.bus == RT_NULL)
    {
        LOG_E("Can't find %s device", cfg->dev_name);
        return -RT_ERROR;
    }

    if (rt_device_open((rt_device_t)ST7102_client.bus, RT_DEVICE_FLAG_RDWR) != RT_EOK)
    {
        LOG_E("open %s device failed", cfg->dev_name);
        return -RT_ERROR;
    }

    /* register touch device */
    touch_device->info.type = RT_TOUCH_TYPE_CAPACITANCE;
    touch_device->info.vendor = RT_TOUCH_VENDOR_GT;
    rt_memcpy(&touch_device->config, cfg, sizeof(struct rt_touch_config));
    touch_device->ops = &ST7102_touch_ops;

    rt_hw_touch_register(touch_device, name, RT_DEVICE_FLAG_INT_RX, RT_NULL);

    ret = rt_pin_attach_irq(ST7102_IRQ_PIN, PIN_IRQ_MODE_FALLING, ST7102_read_point, 0);

    if (ret != RT_EOK)
    {
        rt_kprintf("Failed to attach IRQ for pin %d, error: %d\n", ST7102_IRQ_PIN, ret);
        return 0;
    }

    ret = rt_pin_irq_enable(ST7102_IRQ_PIN, PIN_IRQ_ENABLE);
    if (ret != RT_EOK)
    {
        rt_kprintf("Failed to enable IRQ for pin %d, error: %d\n", ST7102_IRQ_PIN, ret);
        return 0;
    }

    LOG_I("Touch device ST7102 init success");

    return RT_EOK;
}
int rt_hw_ST7102_port(void)
{
    struct rt_touch_config cfg;
    rt_uint8_t rst_pin;

    rst_pin = ST7102_RST_PIN;
    cfg.dev_name = "i2c1";
    cfg.irq_pin.pin = ST7102_IRQ_PIN;
    cfg.irq_pin.mode = PIN_MODE_INPUT_PULLDOWN;
    cfg.user_data = &rst_pin;

    rt_hw_ST7102_init("ST7102", &cfg);

    return 0;
}
INIT_ENV_EXPORT(rt_hw_ST7102_port);

