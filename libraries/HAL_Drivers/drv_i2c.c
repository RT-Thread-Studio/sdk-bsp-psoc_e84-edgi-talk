/*
 * Copyright (c) 2006-2023, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2022-07-08     Rbb666       first implementation.
 */

#include "board.h"

#if defined(RT_USING_I2C)
#include <rtdevice.h>

#include "cy_scb_i2c.h"
#include "mtb_hal_i2c.h"

#ifdef BSP_USING_HW_I2C0
extern const cy_stc_scb_i2c_config_t CYBSP_I2C_CONTROLLER_config;
extern const mtb_hal_i2c_configurator_t CYBSP_I2C_CONTROLLER_hal_config;
#endif
#ifdef BSP_USING_HW_I2C5
extern const cy_stc_scb_i2c_config_t CYBSP_I2C5_CONTROLLER_config;
extern const mtb_hal_i2c_configurator_t CYBSP_I2C5_CONTROLLER_hal_config;
#endif
#ifdef BSP_USING_HW_I2C8
extern const cy_stc_scb_i2c_config_t scb_8_config;
extern const mtb_hal_i2c_configurator_t scb_8_hal_config;
#endif

#ifndef I2C0_CONFIG
#define I2C0_CONFIG                                                     \
    {                                                                   \
        .name = "i2c0",                                                 \
        .base = CYBSP_I2C_CONTROLLER_HW,                              \
        .cy_stc_scb_i2c_config = &CYBSP_I2C_CONTROLLER_config,        \
        .mtb_hal_i2c_configurator = &CYBSP_I2C_CONTROLLER_hal_config, \
    }
#endif /* I2C0_CONFIG */

#ifndef I2C3_CONFIG
#define I2C3_CONFIG                              \
    {                                            \
        .name = "i2c3",                        \
        .base = SCB3,                           \
        .cy_stc_scb_i2c_config = RT_NULL,       \
        .mtb_hal_i2c_configurator = RT_NULL,    \
    }
#endif /* I2C3_CONFIG */

#ifndef I2C4_CONFIG
#define I2C4_CONFIG                              \
    {                                            \
        .name = "i2c4",                        \
        .base = SCB4,                           \
        .cy_stc_scb_i2c_config = RT_NULL,       \
        .mtb_hal_i2c_configurator = RT_NULL,    \
    }
#endif /* I2C4_CONFIG */

#ifndef I2C5_CONFIG
#define I2C5_CONFIG                                                     \
    {                                                                   \
        .name = "i2c5",                                               \
        .base = SCB5,                                                   \
        .cy_stc_scb_i2c_config = &CYBSP_I2C5_CONTROLLER_config,         \
        .mtb_hal_i2c_configurator = &CYBSP_I2C5_CONTROLLER_hal_config,  \
    }
#endif /* I2C5_CONFIG */

#ifndef I2C6_CONFIG
#define I2C6_CONFIG                              \
    {                                            \
        .name = "i2c6",                        \
        .base = SCB6,                           \
        .cy_stc_scb_i2c_config = RT_NULL,       \
        .mtb_hal_i2c_configurator = RT_NULL,    \
    }
#endif /* I2C6_CONFIG */

#ifndef I2C8_CONFIG
#define I2C8_CONFIG                                  \
    {                                                \
        .name = "i2c8",                            \
        .base = SCB8,                                \
        .cy_stc_scb_i2c_config = &scb_8_config,      \
        .mtb_hal_i2c_configurator = &scb_8_hal_config, \
    }
#endif /* I2C8_CONFIG */

struct ifx_i2c
{
    const char *name;
    CySCB_Type *base;
    const cy_stc_scb_i2c_config_t *cy_stc_scb_i2c_config;
    const mtb_hal_i2c_configurator_t *mtb_hal_i2c_configurator;
    mtb_hal_i2c_t hal_obj;
    cy_stc_scb_i2c_context_t context;
    struct rt_i2c_bus_device i2c_bus;
};

static struct ifx_i2c i2c_objs[] =
{
#ifdef BSP_USING_HW_I2C0
    I2C0_CONFIG,
#endif

#ifdef BSP_USING_HW_I2C3
    I2C3_CONFIG,
#endif

#ifdef BSP_USING_HW_I2C4
    I2C4_CONFIG,
#endif

#ifdef BSP_USING_HW_I2C5
    I2C5_CONFIG,
#endif

#ifdef BSP_USING_HW_I2C6
    I2C6_CONFIG,
#endif

#ifdef BSP_USING_HW_I2C8
    I2C8_CONFIG,
#endif
};

#define I2C_BUS_NUM (sizeof(i2c_objs) / sizeof(i2c_objs[0]))

static int ifx_i2c_read(struct ifx_i2c *hi2c, rt_uint16_t slave_address, rt_uint8_t *p_buffer, rt_uint16_t data_byte)
{
    if (mtb_hal_i2c_controller_read(&hi2c->hal_obj, slave_address, p_buffer, data_byte, 10, true) != CY_RSLT_SUCCESS)
    {
        return -RT_ERROR;
    }

    return 0;
}

static int ifx_i2c_write(struct ifx_i2c *hi2c, uint16_t slave_address, uint8_t *p_buffer, uint16_t data_byte)
{
    if (mtb_hal_i2c_controller_write(&hi2c->hal_obj, slave_address, p_buffer, data_byte, 10, true) != CY_RSLT_SUCCESS)
    {
        return -RT_ERROR;
    }

    return 0;
}

static rt_ssize_t _i2c_xfer(struct rt_i2c_bus_device *bus, struct rt_i2c_msg msgs[], rt_uint32_t num)
{
    struct rt_i2c_msg *msg;
    rt_uint32_t i;
    struct ifx_i2c *i2c_obj;

    RT_ASSERT(bus != RT_NULL);
    RT_ASSERT(msgs != RT_NULL);

    i2c_obj = rt_container_of(bus, struct ifx_i2c, i2c_bus);

    for (i = 0; i < num; i++)
    {
        msg = &msgs[i];

        if (msg->flags & RT_I2C_RD)
        {
            if (ifx_i2c_read(i2c_obj, msg->addr, msg->buf, msg->len) != 0)
            {
                goto out;
            }
        }
        else
        {
            if (ifx_i2c_write(i2c_obj, msg->addr, msg->buf, msg->len) != 0)
            {
                goto out;
            }
        }
    }

out:

    return i;
}

static const struct rt_i2c_bus_device_ops i2c_ops =
{
    _i2c_xfer,
    RT_NULL,
    RT_NULL
};

static rt_err_t ifx_i2c_hw_init(struct ifx_i2c *obj)
{
    RT_ASSERT(obj != RT_NULL);

    if ((obj->mtb_hal_i2c_configurator == RT_NULL) || (obj->cy_stc_scb_i2c_config == RT_NULL))
    {
        rt_kprintf("I2C %s config is missing, skip register\n", obj->name);
        return -RT_ERROR;
    }

    cy_rslt_t rslt;
    cy_en_scb_i2c_status_t result;

    result = Cy_SCB_I2C_Init(obj->base, obj->cy_stc_scb_i2c_config, &obj->context);
    if (result != CY_SCB_I2C_SUCCESS)
    {
        rt_kprintf("Cy_SCB_I2C_Init failed for %s, code: 0x%08x\n", obj->name, result);
        return -RT_ERROR;
    }

    Cy_SCB_I2C_Enable(obj->base);

    rslt = mtb_hal_i2c_setup(&obj->hal_obj, obj->mtb_hal_i2c_configurator, &obj->context, NULL);
    if (rslt != CY_RSLT_SUCCESS)
    {
        rt_kprintf("I2C setup failed for %s, code: 0x%08x\n", obj->name, rslt);
        return -RT_ERROR;
    }

    mtb_hal_i2c_cfg_t i2c_controller_config =
    {
        MTB_HAL_I2C_MODE_CONTROLLER,
        0,
        100000,
        MTB_HAL_I2C_DEFAULT_ADDR_MASK,
        false,
    };

    rslt = mtb_hal_i2c_configure(&obj->hal_obj, &i2c_controller_config);
    if (rslt != CY_RSLT_SUCCESS)
    {
        rt_kprintf("I2C configure failed for %s, code: 0x%08x\n", obj->name, rslt);
        return -RT_ERROR;
    }

    return RT_EOK;
}

int rt_hw_i2c_init(void)
{
    rt_err_t result = RT_EOK;
    size_t i2c_num = I2C_BUS_NUM;

    for (size_t i = 0; i < i2c_num; i++)
    {
        i2c_objs[i].i2c_bus.ops = &i2c_ops;

        if (ifx_i2c_hw_init(&i2c_objs[i]) != RT_EOK)
        {
            continue;
        }

        result = rt_i2c_bus_device_register(&i2c_objs[i].i2c_bus, i2c_objs[i].name);
        RT_ASSERT(result == RT_EOK);
    }

    return 0;
}
INIT_PREV_EXPORT(rt_hw_i2c_init);

#endif /* RT_USING_I2C */
