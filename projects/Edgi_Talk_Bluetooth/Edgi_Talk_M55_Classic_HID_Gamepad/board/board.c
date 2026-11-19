/*
 * Copyright (c) 2006-2023, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2022-06-29     Rbb666       first version
 * 2025-08-20     Hydevcode
 */

#include "board.h"

void cy_bsp_all_init(void)
{
    cy_rslt_t result;

    /* Initialize the device and board peripherals */
    result = cybsp_init();

    /* Board init failed. Stop program execution */
    if (result != CY_RSLT_SUCCESS)
    {
        CY_ASSERT(0);
    }
}

void _start(void)
{
    extern int entry(void);
    entry();
    while (1);
    __builtin_unreachable();
}

static int enable_base_supply(void)
{
    const rt_base_t enable = GET_PIN(7, 2);
    rt_pin_mode(enable, PIN_MODE_OUTPUT);
    rt_pin_write(enable, PIN_HIGH);
    return RT_EOK;
}
INIT_BOARD_EXPORT(enable_base_supply);
