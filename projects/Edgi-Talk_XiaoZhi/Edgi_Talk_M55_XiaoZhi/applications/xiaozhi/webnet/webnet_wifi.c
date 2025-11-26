#include <rtthread.h>
#include <webnet.h>
#include <wn_module.h>
#include <wlan_mgnt.h>

#define RESULT_BUF_SIZE 4096
static char result_buffer[RESULT_BUF_SIZE];
static rt_bool_t sta_connected = RT_FALSE;

#define MAX_SCAN_RESULTS 32
static struct rt_wlan_info scan_result[MAX_SCAN_RESULTS];
static int scan_cnt = 0;
static struct rt_wlan_info *scan_filter = RT_NULL;

static void wifi_scan_result_clean(void)
{
    scan_cnt = 0;
    rt_memset(scan_result, 0, sizeof(scan_result));
}

static int wifi_scan_result_cache(struct rt_wlan_info *info)
{
    if (scan_cnt >= MAX_SCAN_RESULTS)
        return -RT_EFULL;

    rt_memcpy(&scan_result[scan_cnt], info, sizeof(struct rt_wlan_info));
    scan_cnt++;
    return RT_EOK;
}

static void user_ap_info_callback(int event, struct rt_wlan_buff *buff, void *parameter)
{
    struct rt_wlan_info *info = (struct rt_wlan_info *)buff->data;
    int index = *((int *)parameter);

    if (wifi_scan_result_cache(info) == RT_EOK)
    {
        if (scan_filter == RT_NULL ||
            (scan_filter->ssid.len == info->ssid.len &&
             rt_memcmp(scan_filter->ssid.val, info->ssid.val, scan_filter->ssid.len) == 0))
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
    scan_filter = RT_NULL;

    rt_wlan_register_event_handler(RT_WLAN_EVT_SCAN_REPORT,
                                   user_ap_info_callback, &index);

    ret = rt_wlan_scan_with_info(info);
    if (ret != RT_EOK)
        rt_kprintf("[WiFi] scan failed: %d\n", ret);

    int len = rt_snprintf(result_buffer, RESULT_BUF_SIZE, "[");

    for (int i = 0; i < scan_cnt; i++)
    {
        len += rt_snprintf(result_buffer + len,
                           RESULT_BUF_SIZE - len,
                           "{\"ssid\":\"%s\",\"rssi\":%d}%s",
                           scan_result[i].ssid.val,
                           scan_result[i].rssi,
                           (i == scan_cnt - 1) ? "" : ",");
    }

    len += rt_snprintf(result_buffer + len, RESULT_BUF_SIZE - len, "]");
    webnet_session_set_header(session, "application/json", 200, "OK", len);
    webnet_session_write(session, (rt_uint8_t *)result_buffer, len);
}

static void wlan_ready_handler(int event, struct rt_wlan_buff *buff, void *parameter)
{
    if (event == RT_WLAN_EVT_READY && !sta_connected)
    {
        sta_connected = RT_TRUE;
        rt_kprintf("[WiFi] STA connected to router successfully!\n");

        rt_thread_mdelay(3000);

        rt_wlan_ap_stop();
        rt_kprintf("[AP] Soft-AP stopped. Configuration completed!\n");

        extern void clean_info(void);
        clean_info();

        extern int ws_xiaozhi_init(void);
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
        len = rt_snprintf(result_buffer, RESULT_BUF_SIZE,
            "<!DOCTYPE html><html><head><meta charset=\"utf-8\">"
            "<style>body{font-family:Arial;text-align:center;padding:100px;background:#f7f9fc}</style>"
            "</head><body>"
            "<h2 style=\"color:red\">Error: WiFi name (SSID) cannot be empty!</h2>"
            "<p><a href=\"/index.html\">Back</a></p>"
            "</body></html>");
    }
    else
    {
        rt_kprintf("[WiFi] Connecting to SSID: %s\n", ssid);
        rt_err_t ret = rt_wlan_connect(ssid,
                     (password && rt_strlen(password) > 0) ? password : RT_NULL);

        if (ret == RT_EOK)
        {
            len = rt_snprintf(result_buffer, RESULT_BUF_SIZE,
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
            len = rt_snprintf(result_buffer, RESULT_BUF_SIZE,
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
    webnet_session_write(session, (const rt_uint8_t*)result_buffer, len);
}

void wifi_init(void)
{
    static rt_bool_t inited = RT_FALSE;
    if (inited) return;

    webnet_init();
    rt_kprintf("[WebNet] HTTP Server started.\n");

    rt_wlan_set_mode(RT_WLAN_DEVICE_AP_NAME, RT_WLAN_AP);
    rt_wlan_start_ap("RT-Thread-AP", "123456789");
    rt_kprintf("[AP] Started → SSID: RT-Thread-AP Password: 123456789\n");

    webnet_cgi_register("wifi_connect", cgi_wifi_connect);
    webnet_cgi_register("wifi_scan", cgi_wifi_scan);

    rt_wlan_register_event_handler(RT_WLAN_EVT_READY, wlan_ready_handler, RT_NULL);

    inited = RT_TRUE;
    rt_kprintf("=== WiFi Config Portal Ready ===\n");
    rt_kprintf("Open browser → http://192.168.169.1\n");
}

