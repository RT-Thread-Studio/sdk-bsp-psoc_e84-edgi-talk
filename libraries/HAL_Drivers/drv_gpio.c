/*
 * Copyright (c) 2006-2023, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author            Notes
 * 2022-07-1      Rbb666            first version
 */


#include "drv_gpio.h"


#ifdef RT_USING_PIN
#define INT_PRIORITY        7u
#define PIN_IFXPORT_MAX        22u
#define PIN_IFX_PINS_PER_PORT  8u
#define PIN_IFX_PIN_MAX        (PIN_IFXPORT_MAX * PIN_IFX_PINS_PER_PORT)

static mtb_hal_gpio_t mtb_gpio_irq_tab[PIN_IFX_PIN_MAX];
static rt_uint8_t pin_irq_enabled_tab[PIN_IFX_PIN_MAX];
static rt_uint8_t port_irq_enabled_count[PIN_IFXPORT_MAX];
static rt_uint8_t port_irq_inited_tab[PIN_IFXPORT_MAX];

static struct pin_irq_map pin_irq_map[] =
{
    {CYHAL_PORT_0,  ioss_interrupts_gpio_0_IRQn},
    {CYHAL_PORT_1,  ioss_interrupts_gpio_1_IRQn},
    {CYHAL_PORT_2,  ioss_interrupts_gpio_2_IRQn},
    {CYHAL_PORT_3,  ioss_interrupts_gpio_3_IRQn},
    {CYHAL_PORT_4,  ioss_interrupts_gpio_4_IRQn},
    {CYHAL_PORT_5,  ioss_interrupts_gpio_5_IRQn},
    {CYHAL_PORT_6,  ioss_interrupts_gpio_6_IRQn},
    {CYHAL_PORT_7,  ioss_interrupts_gpio_7_IRQn},
    {CYHAL_PORT_8,  ioss_interrupts_gpio_8_IRQn},
    {CYHAL_PORT_9,  ioss_interrupts_gpio_9_IRQn},
    {CYHAL_PORT_10,  ioss_interrupts_gpio_10_IRQn},
    {CYHAL_PORT_11,  ioss_interrupts_gpio_11_IRQn},
    {CYHAL_PORT_12,  ioss_interrupts_gpio_12_IRQn},
    {CYHAL_PORT_13,  ioss_interrupts_gpio_13_IRQn},
    {CYHAL_PORT_14,  ioss_interrupts_gpio_14_IRQn},
    {CYHAL_PORT_15,  ioss_interrupts_gpio_15_IRQn},
    {CYHAL_PORT_16,  ioss_interrupts_gpio_16_IRQn},
    {CYHAL_PORT_17,  ioss_interrupts_gpio_17_IRQn},
    {CYHAL_PORT_18,  ioss_interrupts_gpio_18_IRQn},
    {CYHAL_PORT_19,  ioss_interrupts_gpio_19_IRQn},
    {CYHAL_PORT_20,  ioss_interrupts_gpio_20_IRQn},
    {CYHAL_PORT_21,  ioss_interrupts_gpio_21_IRQn},
};

static struct rt_pin_irq_hdr pin_irq_handler_tab[PIN_IFX_PIN_MAX];

static rt_bool_t ifx_pin_valid(rt_base_t pin)
{
    return (pin >= 0) && (CYHAL_GET_PORT(pin) < PIN_IFXPORT_MAX);
}

static rt_uint16_t ifx_pin_index(rt_base_t pin)
{
    return (rt_uint16_t)(((rt_uint16_t)CYHAL_GET_PORT(pin) * PIN_IFX_PINS_PER_PORT) + CYHAL_GET_PIN(pin));
}

static const struct pin_irq_map *ifx_pin_get_irq_map(rt_uint16_t port)
{
    rt_size_t i;

    for (i = 0; i < sizeof(pin_irq_map) / sizeof(pin_irq_map[0]); i++)
    {
        if (pin_irq_map[i].port == port)
        {
            return &pin_irq_map[i];
        }
    }

    return RT_NULL;
}

static rt_err_t ifx_pin_irq_mode(rt_uint8_t mode, mtb_hal_gpio_event_t *event)
{
    switch (mode)
    {
    case PIN_IRQ_MODE_RISING:
        *event = MTB_HAL_GPIO_IRQ_RISE;
        return RT_EOK;

    case PIN_IRQ_MODE_FALLING:
        *event = MTB_HAL_GPIO_IRQ_FALL;
        return RT_EOK;

    case PIN_IRQ_MODE_RISING_FALLING:
        *event = MTB_HAL_GPIO_IRQ_BOTH;
        return RT_EOK;

    default:
        return -RT_EINVAL;
    }
}

void gpio_interrupt_handler()
{
    rt_uint16_t index;

    for (index = 0; index < PIN_IFX_PIN_MAX; index++)
    {
        mtb_hal_gpio_t *gpio = &mtb_gpio_irq_tab[index];

        if ((pin_irq_enabled_tab[index] != 0) &&
                (Cy_GPIO_GetInterruptStatusMasked(gpio->port_addr, gpio->pin_num) != 0u))
        {
            Cy_GPIO_ClearInterrupt(gpio->port_addr, gpio->pin_num);

            if (pin_irq_handler_tab[index].hdr != RT_NULL)
            {
                pin_irq_handler_tab[index].hdr(pin_irq_handler_tab[index].args);
            }
        }
    }
}

static void ifx_pin_mode(rt_device_t dev, rt_base_t pin, rt_uint8_t mode)
{
    mtb_hal_gpio_t mtb_gpio;
    if (!ifx_pin_valid(pin))
    {
        return;
    }
    switch (mode)
    {
    case PIN_MODE_OUTPUT:
        Cy_GPIO_Pin_FastInit(Cy_GPIO_PortToAddr(CYHAL_GET_PORT(pin)), CYHAL_GET_PIN(pin), CY_GPIO_DM_STRONG, 1U, HSIOM_SEL_GPIO);
        mtb_hal_gpio_setup(&mtb_gpio, CYHAL_GET_PORT(pin), CYHAL_GET_PIN(pin));
        break;

    case PIN_MODE_INPUT:
        Cy_GPIO_Pin_FastInit(Cy_GPIO_PortToAddr(CYHAL_GET_PORT(pin)), CYHAL_GET_PIN(pin), CY_GPIO_DM_HIGHZ, 1U, HSIOM_SEL_GPIO);
        mtb_hal_gpio_setup(&mtb_gpio, CYHAL_GET_PORT(pin), CYHAL_GET_PIN(pin));
        break;

    case PIN_MODE_INPUT_PULLUP:
        Cy_GPIO_Pin_FastInit(Cy_GPIO_PortToAddr(CYHAL_GET_PORT(pin)), CYHAL_GET_PIN(pin), CY_GPIO_DM_PULLUP, 1U, HSIOM_SEL_GPIO);
        mtb_hal_gpio_setup(&mtb_gpio, CYHAL_GET_PORT(pin), CYHAL_GET_PIN(pin));
        break;

    case PIN_MODE_INPUT_PULLDOWN:
        Cy_GPIO_Pin_FastInit(Cy_GPIO_PortToAddr(CYHAL_GET_PORT(pin)), CYHAL_GET_PIN(pin), CY_GPIO_DM_PULLDOWN, 0U, HSIOM_SEL_GPIO);
        mtb_hal_gpio_setup(&mtb_gpio, CYHAL_GET_PORT(pin), CYHAL_GET_PIN(pin));
        break;

    case PIN_MODE_OUTPUT_OD:
        Cy_GPIO_Pin_FastInit(Cy_GPIO_PortToAddr(CYHAL_GET_PORT(pin)), CYHAL_GET_PIN(pin), CY_GPIO_DM_OD_DRIVESLOW, 0U, HSIOM_SEL_GPIO);
        mtb_hal_gpio_setup(&mtb_gpio, CYHAL_GET_PORT(pin), CYHAL_GET_PIN(pin));
        break;
    }
}

static void ifx_pin_write(rt_device_t dev, rt_base_t pin, rt_uint8_t value)
{
    mtb_hal_gpio_t mtb_gpio;
    if (ifx_pin_valid(pin))
    {
        mtb_hal_gpio_setup(&mtb_gpio, CYHAL_GET_PORT(pin), CYHAL_GET_PIN(pin));
        mtb_hal_gpio_write(&mtb_gpio, value);
    }
    else
    {
        return;
    }
}

static rt_int8_t ifx_pin_read(struct rt_device *device, rt_base_t pin)
{
    mtb_hal_gpio_t mtb_gpio;
    if (ifx_pin_valid(pin))
    {
        mtb_hal_gpio_setup(&mtb_gpio, CYHAL_GET_PORT(pin), CYHAL_GET_PIN(pin));
        return mtb_hal_gpio_read(&mtb_gpio);
    }
    else
    {
        return -RT_EINVAL;
    }
}

static rt_err_t ifx_pin_attach_irq(struct rt_device *device, rt_base_t pin,
                                   rt_uint8_t mode, void (*hdr)(void *args), void *args)
{
    rt_uint16_t index;
    rt_base_t level;
    mtb_hal_gpio_event_t event;

    if (!ifx_pin_valid(pin))
    {
        return -RT_EINVAL;
    }

    if (ifx_pin_irq_mode(mode, &event) != RT_EOK)
    {
        return -RT_EINVAL;
    }

    index = ifx_pin_index(pin);
    level = rt_hw_interrupt_disable();
    if (pin_irq_handler_tab[index].pin == pin &&
            pin_irq_handler_tab[index].hdr == hdr &&
            pin_irq_handler_tab[index].mode == mode &&
            pin_irq_handler_tab[index].args == args)
    {
        rt_hw_interrupt_enable(level);
        return RT_EOK;
    }

    if (pin_irq_handler_tab[index].pin != PIN_IRQ_PIN_NONE)
    {
        rt_hw_interrupt_enable(level);
        return -RT_EBUSY;
    }

    pin_irq_handler_tab[index].pin = pin;
    pin_irq_handler_tab[index].hdr = hdr;
    pin_irq_handler_tab[index].mode = mode;
    pin_irq_handler_tab[index].args = args;
    rt_hw_interrupt_enable(level);

    return RT_EOK;
}

static rt_err_t ifx_pin_dettach_irq(struct rt_device *device, rt_base_t pin)
{
    rt_uint16_t index;
    rt_uint16_t port;
    rt_base_t level;
    const struct pin_irq_map *irqmap;

    if (!ifx_pin_valid(pin))
    {
        return -RT_EINVAL;
    }

    port = CYHAL_GET_PORT(pin);
    index = ifx_pin_index(pin);
    irqmap = ifx_pin_get_irq_map(port);
    if (irqmap == RT_NULL)
    {
        return -RT_EINVAL;
    }

    level = rt_hw_interrupt_disable();

    if (pin_irq_handler_tab[index].pin == PIN_IRQ_PIN_NONE)
    {
        rt_hw_interrupt_enable(level);
        return RT_EOK;
    }

    if (pin_irq_enabled_tab[index] != 0)
    {
        mtb_hal_gpio_enable_event(&mtb_gpio_irq_tab[index], MTB_HAL_GPIO_IRQ_NONE, RT_FALSE);
        pin_irq_enabled_tab[index] = 0;

        if (port_irq_enabled_count[port] > 0)
        {
            port_irq_enabled_count[port]--;
        }

        if (port_irq_enabled_count[port] == 0)
        {
            NVIC_DisableIRQ(irqmap->irqno);
        }
    }

    pin_irq_handler_tab[index].pin = PIN_IRQ_PIN_NONE;
    pin_irq_handler_tab[index].hdr = RT_NULL;
    pin_irq_handler_tab[index].mode = 0;
    pin_irq_handler_tab[index].args = RT_NULL;
    rt_hw_interrupt_enable(level);

    return RT_EOK;
}


static rt_err_t ifx_pin_irq_enable(struct rt_device *device, rt_base_t pin,
                                   rt_uint8_t enabled)
{
    rt_base_t level;
    rt_uint16_t index;
    rt_uint16_t port;
    mtb_hal_gpio_event_t pin_irq_mode;
    const struct pin_irq_map *irqmap;

    if (!ifx_pin_valid(pin))
    {
        return -RT_EINVAL;
    }

    port = CYHAL_GET_PORT(pin);
    index = ifx_pin_index(pin);
    irqmap = ifx_pin_get_irq_map(port);
    if (irqmap == RT_NULL)
    {
        return -RT_EINVAL;
    }

    if (enabled == PIN_IRQ_ENABLE)
    {
        if (ifx_pin_irq_mode(pin_irq_handler_tab[index].mode, &pin_irq_mode) != RT_EOK)
        {
            return -RT_EINVAL;
        }

        level = rt_hw_interrupt_disable();

        if (pin_irq_handler_tab[index].pin == PIN_IRQ_PIN_NONE)
        {
            rt_hw_interrupt_enable(level);
            return -RT_EINVAL;
        }

        mtb_hal_gpio_setup(&mtb_gpio_irq_tab[index], port, CYHAL_GET_PIN(pin));

        if (port_irq_inited_tab[port] == 0)
        {
            cy_stc_sysint_t intr_cfg;

            intr_cfg.intrSrc = irqmap->irqno;
            intr_cfg.intrPriority = INT_PRIORITY;
            Cy_SysInt_Init(&intr_cfg, gpio_interrupt_handler);
            port_irq_inited_tab[port] = 1;
        }

        if (pin_irq_enabled_tab[index] == 0)
        {
            pin_irq_enabled_tab[index] = 1;
            port_irq_enabled_count[port]++;
        }

        mtb_hal_gpio_enable_event(&mtb_gpio_irq_tab[index], pin_irq_mode, RT_TRUE);
        NVIC_EnableIRQ(irqmap->irqno);
        rt_hw_interrupt_enable(level);
    }
    else if (enabled == PIN_IRQ_DISABLE)
    {
        level = rt_hw_interrupt_disable();

        if (pin_irq_enabled_tab[index] != 0)
        {
            mtb_hal_gpio_enable_event(&mtb_gpio_irq_tab[index], MTB_HAL_GPIO_IRQ_NONE, RT_FALSE);
            pin_irq_enabled_tab[index] = 0;

            if (port_irq_enabled_count[port] > 0)
            {
                port_irq_enabled_count[port]--;
            }

            if (port_irq_enabled_count[port] == 0)
            {
                NVIC_DisableIRQ(irqmap->irqno);
            }
        }

        rt_hw_interrupt_enable(level);
    }
    else
    {
        return -RT_EINVAL;
    }

    return RT_EOK;
}

const static struct rt_pin_ops _ifx_pin_ops =
{
    ifx_pin_mode,
    ifx_pin_write,
    ifx_pin_read,
    ifx_pin_attach_irq,
    ifx_pin_dettach_irq,
    ifx_pin_irq_enable,
    RT_NULL,
};

int rt_hw_pin_init(void)
{
    rt_uint16_t index;

    for (index = 0; index < PIN_IFX_PIN_MAX; index++)
    {
        pin_irq_handler_tab[index].pin = PIN_IRQ_PIN_NONE;
    }

    return rt_device_pin_register("pin", &_ifx_pin_ops, RT_NULL);
}

#endif /* RT_USING_PIN */
