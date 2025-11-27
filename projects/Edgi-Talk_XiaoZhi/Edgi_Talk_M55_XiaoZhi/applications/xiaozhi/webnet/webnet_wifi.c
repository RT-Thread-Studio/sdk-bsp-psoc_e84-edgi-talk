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
#include <webnet.h>
#include <wn_module.h>
#include <wlan_mgnt.h>
#include <dfs_file.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <cJSON.h>
#include "wifi_manager.h"
#include "xiaozhi_ui.h"

/* Declare xiaozhi init function to avoid including xiaozhi.h which has lwIP conflicts */
extern int ws_xiaozhi_init(void);

/*****************************************************************************
 * Macro Definitions
 *****************************************************************************/
#define DBG_TAG    "wifi.mgr"
#define DBG_LVL    DBG_INFO
#include <rtdbg.h>

#define RESULT_BUF_SIZE         4096
#define MAX_SCAN_RESULTS        32
#define WIFI_CONNECT_MAX_RETRY  3
#define WIFI_CONNECT_TIMEOUT_S  15
#define SD_MOUNT_TIMEOUT_S      10

/*****************************************************************************
 * Static Variables
 *****************************************************************************/
static char s_result_buffer[RESULT_BUF_SIZE];
static rt_bool_t s_sta_connected = RT_FALSE;

/* Temporary storage for current WiFi credentials */
static char s_saved_ssid[64] = {0};
static char s_saved_password[64] = {0};

/* WiFi scan results */
static struct rt_wlan_info s_scan_result[MAX_SCAN_RESULTS];
static int s_scan_cnt = 0;
static struct rt_wlan_info *s_scan_filter = RT_NULL;

/*****************************************************************************
 * Private Functions - Configuration
 *****************************************************************************/

/**
 * @brief Save WiFi configuration to SD card
 * @param ssid WiFi SSID
 * @param password WiFi password
 * @return RT_EOK on success, other values on failure
 */
static rt_err_t wifi_config_save(const char *ssid, const char *password)
{
    int fd;
    cJSON *root = RT_NULL;
    char *json_str = RT_NULL;
    rt_err_t ret = -RT_ERROR;

    if (!ssid || rt_strlen(ssid) == 0)
    {
        LOG_E("Save config failed: SSID is empty");
        return -RT_EINVAL;
    }

    /* Create JSON object */
    root = cJSON_CreateObject();
    if (!root)
    {
        LOG_E("Create JSON object failed");
        return -RT_ENOMEM;
    }

    cJSON_AddStringToObject(root, "ssid", ssid);
    cJSON_AddStringToObject(root, "password", password ? password : "");

    /* Convert to JSON string */
    json_str = cJSON_Print(root);
    if (!json_str)
    {
        LOG_E("JSON print failed");
        cJSON_Delete(root);
        return -RT_ENOMEM;
    }

    /* Write to file */
    fd = open(WIFI_CONFIG_FILE, O_WRONLY | O_CREAT | O_TRUNC, 0);
    if (fd >= 0)
    {
        int len = rt_strlen(json_str);
        if (write(fd, json_str, len) == len)
        {
            LOG_I("Config saved to %s", WIFI_CONFIG_FILE);
            ret = RT_EOK;
        }
        else
        {
            LOG_E("Write config file failed");
        }
        close(fd);
    }
    else
    {
        LOG_E("Open config file failed: %s", WIFI_CONFIG_FILE);
    }

    cJSON_free(json_str);
    cJSON_Delete(root);

    return ret;
}

/**
 * @brief Load WiFi configuration from SD card
 * @param ssid Output buffer for WiFi SSID
 * @param ssid_len Length of ssid buffer
 * @param password Output buffer for WiFi password
 * @param password_len Length of password buffer
 * @return RT_EOK on success, other values on failure
 */
static rt_err_t wifi_config_load(char *ssid, int ssid_len, char *password, int password_len)
{
    int fd;
    char buf[256] = {0};
    cJSON *root = RT_NULL;
    cJSON *item = RT_NULL;
    rt_err_t ret = -RT_ERROR;

    if (!ssid || !password)
    {
        return -RT_EINVAL;
    }

    /* Read file */
    fd = open(WIFI_CONFIG_FILE, O_RDONLY);
    if (fd < 0)
    {
        LOG_D("Config file not found: %s", WIFI_CONFIG_FILE);
        return -RT_ENOSYS;
    }

    int len = read(fd, buf, sizeof(buf) - 1);
    close(fd);

    if (len <= 0)
    {
        LOG_E("Read config file failed");
        return -RT_ERROR;
    }

    buf[len] = '\0';

    /* Parse JSON */
    root = cJSON_Parse(buf);
    if (!root)
    {
        LOG_E("Parse config JSON failed");
        return -RT_ERROR;
    }

    /* Get SSID */
    item = cJSON_GetObjectItem(root, "ssid");
    if (item && cJSON_IsString(item) && item->valuestring)
    {
        rt_strncpy(ssid, item->valuestring, ssid_len - 1);
        ssid[ssid_len - 1] = '\0';
    }
    else
    {
        LOG_E("Config missing SSID");
        cJSON_Delete(root);
        return -RT_ERROR;
    }

    /* Get password */
    item = cJSON_GetObjectItem(root, "password");
    if (item && cJSON_IsString(item) && item->valuestring)
    {
        rt_strncpy(password, item->valuestring, password_len - 1);
        password[password_len - 1] = '\0';
    }
    else
    {
        password[0] = '\0';
    }

    LOG_I("Config loaded: SSID=%s", ssid);
    cJSON_Delete(root);

    return RT_EOK;
}

/*****************************************************************************
 * Private Functions - WiFi Scan
 *****************************************************************************/

static void wifi_scan_result_clean(void)
{
    s_scan_cnt = 0;
    rt_memset(s_scan_result, 0, sizeof(s_scan_result));
}

static int wifi_scan_result_cache(struct rt_wlan_info *info)
{
    if (s_scan_cnt >= MAX_SCAN_RESULTS)
        return -RT_EFULL;

    rt_memcpy(&s_scan_result[s_scan_cnt], info, sizeof(struct rt_wlan_info));
    s_scan_cnt++;
    return RT_EOK;
}

static void user_ap_info_callback(int event, struct rt_wlan_buff *buff, void *parameter)
{
    struct rt_wlan_info *info = (struct rt_wlan_info *)buff->data;
    int index = *((int *)parameter);

    if (wifi_scan_result_cache(info) == RT_EOK)
    {
        if (s_scan_filter == RT_NULL ||
            (s_scan_filter->ssid.len == info->ssid.len &&
             rt_memcmp(s_scan_filter->ssid.val, info->ssid.val, s_scan_filter->ssid.len) == 0))
        {
            index++;
            *((int *)parameter) = index;
        }
    }
}

static void cgi_wifi_scan(struct webnet_session *session)
{
    int ret;
    int index = 0;
    struct rt_wlan_info *info = RT_NULL;

    wifi_scan_result_clean();
    s_scan_filter = RT_NULL;

    rt_wlan_register_event_handler(RT_WLAN_EVT_SCAN_REPORT,
                                   user_ap_info_callback, &index);

    ret = rt_wlan_scan_with_info(info);
    if (ret != RT_EOK)
    {
        LOG_W("Scan failed: %d", ret);
    }

    int len = rt_snprintf(s_result_buffer, RESULT_BUF_SIZE, "[");

    for (int i = 0; i < s_scan_cnt; i++)
    {
        len += rt_snprintf(s_result_buffer + len,
                           RESULT_BUF_SIZE - len,
                           "{\"ssid\":\"%s\",\"rssi\":%d}%s",
                           s_scan_result[i].ssid.val,
                           s_scan_result[i].rssi,
                           (i == s_scan_cnt - 1) ? "" : ",");
    }

    len += rt_snprintf(s_result_buffer + len, RESULT_BUF_SIZE - len, "]");
    webnet_session_set_header(session, "application/json", 200, "OK", len);
    webnet_session_write(session, (rt_uint8_t *)s_result_buffer, len);
}

static void wlan_ready_handler(int event, struct rt_wlan_buff *buff, void *parameter)
{
    if (event == RT_WLAN_EVT_READY && !s_sta_connected)
    {
        s_sta_connected = RT_TRUE;
        LOG_I("STA connected to router successfully");

        /* Save WiFi config to SD card */
        if (s_saved_ssid[0] != '\0')
        {
            wifi_config_save(s_saved_ssid, s_saved_password);
        }

        rt_thread_mdelay(3000);

        rt_wlan_ap_stop();
        LOG_I("Soft-AP stopped. Configuration completed");

        xiaozhi_ui_clear_info();
        ws_xiaozhi_init();
    }
}

static void cgi_wifi_connect(struct webnet_session *session)
{
    struct webnet_request *request = session->request;
    const char *ssid     = webnet_request_get_query(request, "ssid");
    const char *password = webnet_request_get_query(request, "password");
    const char *mimetype = mime_get_type(".html");
    int len;

    if (!ssid || rt_strlen(ssid) == 0)
    {
        len = rt_snprintf(s_result_buffer, RESULT_BUF_SIZE,
            "<!DOCTYPE html><html><head><meta charset=\"utf-8\">"
            "<style>body{font-family:Arial;text-align:center;padding:100px;background:#f7f9fc}</style>"
            "</head><body>"
            "<h2 style=\"color:red\">Error: WiFi name (SSID) cannot be empty!</h2>"
            "<p><a href=\"/index.html\">Back</a></p>"
            "</body></html>");
    }
    else
    {
        /* Save SSID and password to temp cache, write to file after connection success */
        rt_strncpy(s_saved_ssid, ssid, sizeof(s_saved_ssid) - 1);
        s_saved_ssid[sizeof(s_saved_ssid) - 1] = '\0';
        if (password && rt_strlen(password) > 0)
        {
            rt_strncpy(s_saved_password, password, sizeof(s_saved_password) - 1);
            s_saved_password[sizeof(s_saved_password) - 1] = '\0';
        }
        else
        {
            s_saved_password[0] = '\0';
        }

        LOG_I("Connecting to SSID: %s", ssid);
        rt_err_t ret = rt_wlan_connect(ssid,
                     (password && rt_strlen(password) > 0) ? password : RT_NULL);

        if (ret == RT_EOK)
        {
            len = rt_snprintf(s_result_buffer, RESULT_BUF_SIZE,
                "<!DOCTYPE html><html><head><meta charset=\"utf-8\">"
                "<style>body{font-family:Arial;text-align:center;padding:80px;background:#f7f9fc}</style>"
                "</head><body>"
                "<h2>Connecting to WiFi...</h2>"
                "<h3><strong>%s</strong></h3>"
                "<p style=\"color:green;font-size:20px\">Connected successfully!</p>"
                "Your Board will switch to the WiFi automatically.</p>"
                "</body></html>", ssid);
        }
        else
        {
            /* Connection failed, clear temp saved config */
            s_saved_ssid[0] = '\0';
            s_saved_password[0] = '\0';

            len = rt_snprintf(s_result_buffer, RESULT_BUF_SIZE,
                "<!DOCTYPE html><html><head><meta charset=\"utf-8\">"
                "<style>body{font-family:Arial;text-align:center;padding:80px;background:#f7f9fc}</style>"
                "</head><body>"
                "<h2 style=\"color:red\">Connection failed!</h2>"
                "<p>Error code: %d<br>"
                "Possible reasons: wrong password, weak signal, or router rejected.</p>"
                "<br><a href=\"/index.html\">Try again</a>"
                "</body></html>", ret);
        }
    }

    session->request->result_code = 200;
    webnet_session_set_header(session, mimetype, 200, "Ok", len);
    webnet_session_write(session, (const rt_uint8_t*)s_result_buffer, len);
}

/**
 * @brief Start AP configuration mode
 */
static void start_ap_config_mode(void)
{
    rt_err_t ret;

    /* Wait for WLAN device ready */
    LOG_I("Waiting for WLAN device ready...");
    for (int i = 0; i < 20; i++)  /* Wait up to 10 seconds */
    {
        ret = rt_wlan_set_mode(RT_WLAN_DEVICE_AP_NAME, RT_WLAN_AP);
        if (ret == RT_EOK)
        {
            break;
        }
        rt_thread_mdelay(500);
    }

    if (ret != RT_EOK)
    {
        LOG_E("WLAN AP device not ready, cannot start AP mode");
        return;
    }

    /* Start AP */
    ret = rt_wlan_start_ap("RT-Thread-AP", "123456789");
    if (ret != RT_EOK)
    {
        LOG_E("Start AP failed: %d", ret);
        return;
    }
    LOG_I("AP Started -> SSID: RT-Thread-AP Password: 123456789");

    /* Wait for AP network interface ready */
    rt_thread_mdelay(1000);

    /* Start HTTP server after AP is fully ready */
    webnet_init();
    LOG_I("HTTP Server started");

    webnet_cgi_register("wifi_connect", cgi_wifi_connect);
    webnet_cgi_register("wifi_scan", cgi_wifi_scan);

    LOG_I("=== WiFi Config Portal Ready ===");
    LOG_I("Open browser -> http://192.168.169.1");
}

/**
 * @brief Disconnect event handler
 */
static void wlan_disconnect_handler(int event, struct rt_wlan_buff *buff, void *parameter)
{
    if (event == RT_WLAN_EVT_STA_DISCONNECTED)
    {
        LOG_W("STA disconnected from router");
        s_sta_connected = RT_FALSE;
    }
}

/**
 * @brief Try to connect WiFi with saved configuration
 * @return RT_EOK on success, other values on failure
 */
static rt_err_t try_connect_with_saved_config(void)
{
    char ssid[64] = {0};
    char password[64] = {0};
    rt_err_t ret;
    int retry_cnt = 0;
    const int max_retry = 3;

    /* Load WiFi config from SD card */
    if (wifi_config_load(ssid, sizeof(ssid), password, sizeof(password)) != RT_EOK)
    {
        LOG_I("No saved config found");
        return -RT_ENOSYS;
    }

    LOG_I("Trying to connect with saved config: %s", ssid);

    /* Show connecting status on UI */
    xiaozhi_ui_show_connecting();

    /* Save to temp cache for re-saving */
    rt_strncpy(s_saved_ssid, ssid, sizeof(s_saved_ssid) - 1);
    rt_strncpy(s_saved_password, password, sizeof(s_saved_password) - 1);

    /* Ensure WLAN STA mode is set */
    rt_wlan_set_mode(RT_WLAN_DEVICE_STA_NAME, RT_WLAN_STATION);
    rt_thread_mdelay(100);

    /* Try to connect WiFi with retry mechanism */
    while (retry_cnt < max_retry)
    {
        ret = rt_wlan_connect(ssid, (password[0] != '\0') ? password : RT_NULL);
        if (ret == RT_EOK)
        {
            break;
        }

        retry_cnt++;
        LOG_W("Connect attempt %d failed: %d, retrying...", retry_cnt, ret);
        rt_thread_mdelay(1000);
    }

    if (ret != RT_EOK)
    {
        LOG_E("Connect with saved config failed after %d retries: %d", max_retry, ret);
        /* Clear temp cache */
        s_saved_ssid[0] = '\0';
        s_saved_password[0] = '\0';
        return ret;
    }

    /* Wait for connection result */
    LOG_I("Waiting for connection...");
    for (int i = 0; i < (WIFI_CONNECT_TIMEOUT_S * 2); i++)
    {
        rt_thread_mdelay(500);
        if (rt_wlan_is_connected())
        {
            LOG_I("Connected to %s successfully", ssid);
            s_sta_connected = RT_TRUE;

            xiaozhi_ui_clear_info();
            ws_xiaozhi_init();
            return RT_EOK;
        }
    }

    LOG_W("Connection timeout");
    rt_wlan_disconnect();
    s_saved_ssid[0] = '\0';
    s_saved_password[0] = '\0';
    return -RT_ETIMEOUT;
}

/*****************************************************************************
 * Public Functions
 *****************************************************************************/

void wifi_manager_init(void)
{
    static rt_bool_t s_inited = RT_FALSE;

    if (s_inited)
    {
        return;
    }
    s_inited = RT_TRUE;

    /* Register event handlers */
    rt_wlan_register_event_handler(RT_WLAN_EVT_READY, wlan_ready_handler, RT_NULL);
    rt_wlan_register_event_handler(RT_WLAN_EVT_STA_DISCONNECTED, wlan_disconnect_handler, RT_NULL);

    /* Wait for SD card filesystem mount */
    LOG_I("Waiting for SD card mount...");
    for (int i = 0; i < (SD_MOUNT_TIMEOUT_S * 2); i++)
    {
        rt_thread_mdelay(500);
        int fd = open(WIFI_CONFIG_FILE, O_RDONLY);
        if (fd >= 0)
        {
            close(fd);
            break;
        }
        /* Check if /sdcard directory exists */
        struct stat st;
        if (stat("/sdcard", &st) == 0)
        {
            break;
        }
    }

    /* Try to connect with saved config */
    if (try_connect_with_saved_config() == RT_EOK)
    {
        LOG_I("Auto connect succeeded, skip AP config mode");
        return;
    }

    /* Connection failed or no config, start AP config mode */
    LOG_I("Starting AP config mode...");

    /* Show AP config info on UI */
    xiaozhi_ui_show_ap_config();

    start_ap_config_mode();
}

rt_bool_t wifi_manager_is_connected(void)
{
    return s_sta_connected;
}

/* Legacy API for backward compatibility */
void wifi_init(void)
{
    wifi_manager_init();
}
