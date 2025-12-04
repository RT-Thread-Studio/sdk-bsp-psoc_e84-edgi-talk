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

/* Power Control Pins */
#define PIN_ES8388_PWR      GET_PIN(16, 2)  /* ES8388 power enable */
#define PIN_SPEAKER_EN      GET_PIN(21, 6)  /* Speaker amplifier enable */
#define PIN_WIFI_PWR        GET_PIN(16, 3)  /* WiFi power enable */
#define PIN_WIFI_REG        GET_PIN(11, 6)  /* WiFi register switch */
#define PIN_DCDC_CTRL       GET_PIN(7, 2)   /* 3V3 DCDC power control */
#define PIN_LCD_BL          GET_PIN(15, 7)  /* LCD backlight power */
#define PIN_LCD_PWR         GET_PIN(15, 6)  /* LCD IC power */
#define PIN_LCD_PWM         GET_PIN(20, 6)  /* LCD PWM brightness */

/* UI initialization timeout (ms) */
#define UI_INIT_TIMEOUT_MS  5000

/*****************************************************************************
 * External Function Declarations
 *****************************************************************************/
extern void xiaozhi_ui_init(void);
extern rt_err_t xiaozhi_ui_wait_ready(rt_int32_t timeout);
extern void wifi_manager_init(void);

/*****************************************************************************
 * Private Functions
 *****************************************************************************/

/**
 * @brief Initialize board power control GPIOs
 * @return 0 on success
 */
static int board_power_init(void)
{
    /* WiFi power */
    rt_pin_mode(PIN_WIFI_PWR, PIN_MODE_OUTPUT);
    rt_pin_write(PIN_WIFI_PWR, PIN_HIGH);

    rt_pin_mode(PIN_WIFI_REG, PIN_MODE_OUTPUT);
    rt_pin_write(PIN_WIFI_REG, PIN_HIGH);

    /* Audio power */
    rt_pin_mode(PIN_ES8388_PWR, PIN_MODE_OUTPUT);
    rt_pin_write(PIN_ES8388_PWR, PIN_HIGH);

    rt_pin_mode(PIN_SPEAKER_EN, PIN_MODE_OUTPUT);
    rt_pin_write(PIN_SPEAKER_EN, PIN_HIGH);

    /* LCD power */
    rt_pin_mode(PIN_LCD_BL, PIN_MODE_OUTPUT);
    rt_pin_mode(PIN_LCD_PWR, PIN_MODE_OUTPUT);
    rt_pin_mode(PIN_LCD_PWM, PIN_MODE_OUTPUT);

    return 0;
}
INIT_BOARD_EXPORT(board_power_init);

/*****************************************************************************
 * Main Entry
 *****************************************************************************/

int main(void)
{
    LOG_I("Cortex-M55 started");

    /* Initialize UI subsystem */
    xiaozhi_ui_init();

    /* Wait for UI initialization to complete */
    if (xiaozhi_ui_wait_ready(rt_tick_from_millisecond(UI_INIT_TIMEOUT_MS)) != RT_EOK)
    {
        LOG_W("UI initialization timeout");
    }

    /* Initialize WiFi manager */
    wifi_manager_init();

    return 0;
}
