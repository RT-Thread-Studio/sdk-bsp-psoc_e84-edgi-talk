/*
 * Copyright (c) 2006-2024, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2024-01-01     RT-Thread    First version
 */

#include <rtthread.h>
#include <rtdevice.h>
#include <board.h>

/*****************************************************************************
 * Macro Definitions
 *****************************************************************************/
#define DBG_TAG    "main"
#define DBG_LVL    DBG_INFO
#include <rtdbg.h>

/* LED Pin */
#define LED_PIN_GREEN       GET_PIN(16, 6)

/* UI initialization timeout (ms) */
#define UI_INIT_TIMEOUT_MS  5000

/*****************************************************************************
 * External Function Declarations
 *****************************************************************************/
extern void xiaozhi_ui_init(void);
extern rt_err_t xiaozhi_ui_wait_ready(rt_int32_t timeout);
extern void wifi_manager_init(void);

/*****************************************************************************
 * Main Entry
 *****************************************************************************/

int main(void)
{
    LOG_I("Cortex-M55 started");

    /* Initialize UI subsystem */
    // xiaozhi_ui_init();

    // /* Wait for UI initialization to complete */
    // if (xiaozhi_ui_wait_ready(rt_tick_from_millisecond(UI_INIT_TIMEOUT_MS)) != RT_EOK)
    // {
    //     LOG_W("UI initialization timeout");
    // }

    // /* Initialize WiFi manager */
    // wifi_manager_init();

    return 0;
}
