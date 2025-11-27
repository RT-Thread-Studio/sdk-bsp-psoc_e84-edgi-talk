/*
 * Copyright (c) 2006-2024, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2024-01-01     RT-Thread    First version
 */

#ifndef __XIAOZHI_UI_H__
#define __XIAOZHI_UI_H__

#include <rtthread.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * UI Initialization and Synchronization
 */

/**
 * @brief Initialize UI subsystem
 *
 * This function creates the UI thread and initializes message queue.
 * Call wait_ui_ready() after this to ensure UI is fully initialized.
 */
void xiaozhi_ui_init(void);

/**
 * @brief Wait for UI initialization to complete
 * @param timeout Timeout in OS ticks, RT_WAITING_FOREVER for infinite wait
 * @return RT_EOK on success, -RT_ETIMEOUT on timeout
 */
rt_err_t xiaozhi_ui_wait_ready(rt_int32_t timeout);

/*
 * UI Update Functions
 */

/**
 * @brief Update chat status label
 * @param status Status string to display
 */
void xiaozhi_ui_set_status(const char *status);

/**
 * @brief Update chat output label
 * @param output Output string to display
 */
void xiaozhi_ui_set_output(const char *output);

/**
 * @brief Update emoji display
 * @param emoji Emoji name (e.g., "happy", "sad", "neutral")
 */
void xiaozhi_ui_set_emoji(const char *emoji);

/**
 * @brief Update ADC display label
 * @param adc_str ADC value string to display
 */
void xiaozhi_ui_set_adc(const char *adc_str);

/**
 * @brief Clear info label (label2)
 */
void xiaozhi_ui_clear_info(void);

/**
 * @brief Show AP config mode info on screen
 */
void xiaozhi_ui_show_ap_config(void);

/**
 * @brief Show connecting status (for auto-connect from saved config)
 */
void xiaozhi_ui_show_connecting(void);

#ifdef __cplusplus
}
#endif

#endif /* __XIAOZHI_UI_H__ */
