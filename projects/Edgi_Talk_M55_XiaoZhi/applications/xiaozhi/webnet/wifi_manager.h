/*
 * Copyright (c) 2006-2024, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2024-01-01     RT-Thread    First version
 */

#ifndef __WIFI_MANAGER_H__
#define __WIFI_MANAGER_H__

#include <rtthread.h>

#ifdef __cplusplus
extern "C" {
#endif

/* WiFi configuration file path */
#define WIFI_CONFIG_FILE    "/sdcard/wifi_config.json"

/* AP mode default settings */
#define WIFI_AP_SSID        "RT-Thread-AP"
#define WIFI_AP_PASSWORD    "123456789"

/**
 * @brief Initialize WiFi manager
 *
 * This function will:
 * 1. Wait for SD card mount
 * 2. Try to connect with saved configuration
 * 3. Start AP config mode if no saved config or connection failed
 */
void wifi_manager_init(void);

/**
 * @brief Check if WiFi STA is connected
 * @return RT_TRUE if connected, RT_FALSE otherwise
 */
rt_bool_t wifi_manager_is_connected(void);

#ifdef __cplusplus
}
#endif

#endif /* __WIFI_MANAGER_H__ */
