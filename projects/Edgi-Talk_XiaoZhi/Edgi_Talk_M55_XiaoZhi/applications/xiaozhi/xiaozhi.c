#include "xiaozhi.h"
#include <cJSON.h>
#include <lwip/apps/websocket_client.h>
#include <netdev.h>
#include <webclient.h>
#include <string.h>

#define DBG_TAG "xz.ws"
#define DBG_LVL DBG_LOG
#include <rtdbg.h>

/* Global Variables */
static rt_thread_t xiaozhi_tid = RT_NULL;
static const char *client_id = "af7ac552-9991-4b31-b660-683b210ae95f";
static uint8_t WEBSOCKET_RECONNECT_FLAG = 0;
static uint8_t iot_initialized = 0;  // 添加IoT初始化标志
static uint32_t last_reconnect_time = 0;  // 添加重连时间戳，防止频繁重连
static const char *mode_str[] = {"auto", "manual", "realtime"};
static char mac_address_string[20] = {0};
static char client_id_string[40] = {0};
xiaozhi_ws_t g_xz_ws;
enum DeviceState g_state;
rt_event_t xiaozhi_button_event = RT_NULL;

void voice_rx_init();
extern void iot_invoke(const uint8_t *data, uint16_t len);
extern const char *iot_get_descriptors_json(void);
extern const char *iot_get_states_json(void);
extern void xiaozhi_ui_chat_status(char *string);
extern void xiaozhi_ui_chat_output(char *string);
extern void xiaozhi_ui_update_emoji(char *string);

/* 状态一致性检查函数 */
static void ensure_state_consistency(void)
{
    /* 如果状态是Listening但WebSocket断开，强制重置 */
    if (g_state == kDeviceStateListening && !g_xz_ws.is_connected)
    {
        LOG_W("Inconsistent state detected: Listening but disconnected, fixing...\n");
        xz_mic(0);
        g_state = kDeviceStateIdle;
        xiaozhi_ui_chat_status("   就绪");
        xiaozhi_ui_chat_output("就绪");
    }

    /* 如果状态是Speaking但没有连接，也重置 */
    if (g_state == kDeviceStateSpeaking && !g_xz_ws.is_connected)
    {
        LOG_W("Inconsistent state detected: Speaking but disconnected, fixing...\n");
        xz_speaker(0);
        xz_mic(0);
        g_state = kDeviceStateUnknown;
        xiaozhi_ui_chat_status("   休眠中");
        xiaozhi_ui_chat_output("等待唤醒");
    }
}

/* Utility Functions */
char *get_mac_address(void)
{
    struct netdev *netdev = netdev_get_by_name("w0");
    if (netdev == RT_NULL)
    {
        LOG_E("Cannot find netdev w0");
        return "";
    }

    if (netdev->hwaddr_len != 6)
    {
        LOG_E("Invalid MAC address length: %d", netdev->hwaddr_len);
        return "";
    }

    rt_snprintf(mac_address_string, sizeof(mac_address_string),
                "%02x:%02x:%02x:%02x:%02x:%02x",
                netdev->hwaddr[0], netdev->hwaddr[1], netdev->hwaddr[2],
                netdev->hwaddr[3], netdev->hwaddr[4], netdev->hwaddr[5]);
    return mac_address_string;
}

char *get_client_id(void)
{
    if (client_id_string[0] == '\0')
    {
        uint32_t tick = rt_tick_get();
        uint8_t hash_input[64];
        uint8_t hash_output[32];
        int input_len = rt_snprintf((char *)hash_input, sizeof(hash_input),
                                    "%s%u", client_id, tick);

        for (int i = 0; i < 32; i++)
        {
            hash_output[i] = (uint8_t)(hash_input[i % input_len] ^ (tick >> (i % 32)));
        }

        int j = 0;
        for (int i = 0; i < 16; i++)
        {
            if (i == 4 || i == 6 || i == 8 || i == 10)
            {
                client_id_string[j++] = '-';
            }
            client_id_string[j++] = "0123456789abcdef"[hash_output[i] >> 4];
            client_id_string[j++] = "0123456789abcdef"[hash_output[i] & 0xF];
        }
        client_id_string[j] = '\0';
        LOG_D("Generated Client ID: %s", client_id_string);
    }
    return client_id_string;
}

/* Button Handling */
void xz_button_callback(void *arg)
{
    if (CYBSP_BTN_PRESSED == Cy_GPIO_Read(CYBSP_USER_BTN_PORT, CYBSP_USER_BTN_PIN))
    {
        rt_event_send(xiaozhi_button_event, BUTTON_EVENT_PRESSED);
    }
#ifndef ExBoard_Voice
    else
    {
        rt_event_send(xiaozhi_button_event, BUTTON_EVENT_RELEASED);
    }
#endif
}

void xz_button_thread_entry(void *param)
{
    rt_uint32_t evt;
    while (1)
    {
        rt_event_recv(xiaozhi_button_event,
                      BUTTON_EVENT_PRESSED | BUTTON_EVENT_RELEASED,
                      RT_EVENT_FLAG_OR | RT_EVENT_FLAG_CLEAR,
                      RT_WAITING_FOREVER, &evt);

        /* 首先确保状态一致性 */
        ensure_state_consistency();

        /* 优雅的按键处理：先检查连接状态，再处理具体事件 */
        if (evt & BUTTON_EVENT_PRESSED)
        {
            if (g_state == kDeviceStateUnknown || !g_xz_ws.is_connected)
            {
                /* 检查是否正在重连中 */
                if (WEBSOCKET_RECONNECT_FLAG == 1)
                {
                    LOG_D("Reconnection already in progress, ignoring button press\n");
                    xiaozhi_ui_chat_status("   连接中");
                    xiaozhi_ui_chat_output("仍在连接中...");
                    continue;
                }

                LOG_I("Device not connected, initiating wake up...\n");
                xiaozhi_ui_chat_status("   连接中");
                xiaozhi_ui_chat_output("正在连接小智...");
                reconnect_websocket();
            }
            else
            {
                /* 设备已连接，处理具体功能 */
                if (g_state == kDeviceStateSpeaking)
                {
                    LOG_I("Speaking aborted by user\n");
                    xz_speaker(0);
                }

                /* 按下即对话模式：检查是否已在监听 */
                if (g_state != kDeviceStateListening)
                {
                    LOG_I("Starting listening mode - press once to talk\n");
                    xiaozhi_ui_chat_status("   聆听中");
                    xiaozhi_ui_chat_output("聆听中...");

                    /* 使用AutoStop模式，让系统自动检测语音结束 */
                    xz_mic(1);
                    ws_send_listen_start(&g_xz_ws.clnt, g_xz_ws.session_id,
                                         kListeningModeAutoStop);
                }
                else
                {
                    LOG_D("Already in listening mode\n");
                }
            }
        }
        else if (evt & BUTTON_EVENT_RELEASED)
        {
            /* 按下即对话模式：释放按键时不需要停止监听，让系统自动处理 */
            /* 不做任何处理，让AutoStop模式自动检测语音结束 */
            LOG_D("Button released - letting system auto-detect speech end\n");
        }
    }
}

void xz_button_init(void)
{
    static int initialized = 0;
    if (!initialized)
    {
        rt_pin_mode(BUTTON_PIN, PIN_MODE_INPUT_PULLUP);
        rt_pin_write(BUTTON_PIN, PIN_HIGH);
        xiaozhi_button_event = rt_event_create("btn_evt", RT_IPC_FLAG_FIFO);
        RT_ASSERT(xiaozhi_button_event != RT_NULL);

        rt_thread_t tid = rt_thread_create("btn_thread",
                                           xz_button_thread_entry,
                                           RT_NULL, 3 * 1024, 7, 10);
        RT_ASSERT(tid != RT_NULL);
        if (rt_thread_startup(tid) != RT_EOK)
        {
            LOG_E("Button thread startup failed\n");
            return;
        }
        rt_pin_attach_irq(BUTTON_PIN, PIN_IRQ_MODE_RISING_FALLING,
                          xz_button_callback, NULL);
        rt_pin_irq_enable(BUTTON_PIN, RT_TRUE);
        initialized = 1;
        LOG_I("[Init] Button handler ready\n");
    }
}

/* WebSocket Functions */
void ws_send_listen_start(void *ws, char *session_id, enum ListeningMode mode)
{
    static char message[256];
    rt_snprintf(message, 256,
                "{\"session_id\":\"%s\",\"type\":\"listen\",\"state\":\"start\","
                "\"mode\":\"%s\"}",
                session_id, mode_str[mode]);

    if (g_xz_ws.is_connected && g_xz_ws.ws_write_mutex)
    {
        if (rt_mutex_take(g_xz_ws.ws_write_mutex, RT_WAITING_NO) == RT_EOK)
        {
            if (g_xz_ws.is_connected)
            {
                err_t result = wsock_write((wsock_state_t *)ws, message, strlen(message), OPCODE_TEXT);
                LOG_D("ws_send_listen_start result: %d\n", result);
                if (result == ERR_OK)
                {
                    /* 发送成功才更新状态，确保状态同步 */
                    g_state = kDeviceStateListening;
                    LOG_D("State updated to Listening after successful send\n");
                }
                else
                {
                    LOG_E("Failed to send listen start message: %d\n", result);
                    if (result == ERR_CLSD || result == ERR_RST)
                    {
                        g_xz_ws.is_connected = 0;
                    }
                }
            }
            rt_mutex_release(g_xz_ws.ws_write_mutex);
        }
        else
        {
            LOG_D("WebSocket write busy, cannot send listen start\n");
        }
    }
    else
    {
        LOG_E("WebSocket not connected, cannot send listen start\n");
    }
}

void ws_send_listen_stop(void *ws, char *session_id)
{
    static char message[256];
    rt_snprintf(message, 256,
                "{\"session_id\":\"%s\",\"type\":\"listen\",\"state\":\"stop\"}",
                session_id);

    if (g_xz_ws.is_connected && g_xz_ws.ws_write_mutex)
    {
        if (rt_mutex_take(g_xz_ws.ws_write_mutex, RT_WAITING_NO) == RT_EOK)
        {
            if (g_xz_ws.is_connected)
            {
                err_t result = wsock_write((wsock_state_t *)ws, message, strlen(message), OPCODE_TEXT);
                LOG_D("ws_send_listen_stop result: %d\n", result);
                if (result == ERR_OK)
                {
                    /* 发送成功才更新状态，确保状态同步 */
                    g_state = kDeviceStateIdle;
                    LOG_D("State updated to Idle after successful send\n");
                }
                else
                {
                    LOG_E("Failed to send listen stop message: %d\n", result);
                    if (result == ERR_CLSD || result == ERR_RST)
                    {
                        g_xz_ws.is_connected = 0;
                    }
                }
            }
            rt_mutex_release(g_xz_ws.ws_write_mutex);
        }
        else
        {
            LOG_D("WebSocket write busy, cannot send listen stop\n");
        }
    }
    else
    {
        LOG_D("WebSocket not connected, cannot send listen stop\n");
        /* 即使WebSocket断开，也要确保状态正确 */
        g_state = kDeviceStateIdle;
    }
}

void ws_send_hello(void *ws)
{
    if (g_xz_ws.is_connected && g_xz_ws.ws_write_mutex)
    {
        if (rt_mutex_take(g_xz_ws.ws_write_mutex, RT_WAITING_NO) == RT_EOK)
        {
            if (g_xz_ws.is_connected)
            {
                wsock_write((wsock_state_t *)ws, HELLO_MESSAGE,
                            strlen(HELLO_MESSAGE), OPCODE_TEXT);
            }
            rt_mutex_release(g_xz_ws.ws_write_mutex);
        }
    }
    else
    {
        //LOG_E("websocket is not connected\n");
    }
}

void xz_audio_send_using_websocket(uint8_t *data, int len)
{
    if (g_xz_ws.is_connected)
    {
        // 获取互斥锁，防止并发写入
        if (g_xz_ws.ws_write_mutex && rt_mutex_take(g_xz_ws.ws_write_mutex, RT_WAITING_NO) == RT_EOK)
        {
            // 再次检查连接状态，防止在等待锁的过程中连接断开
            if (g_xz_ws.is_connected)
            {
                err_t err = wsock_write(&g_xz_ws.clnt, (const char *)data, len, OPCODE_BINARY);
                if (err != ERR_OK)
                {
                    LOG_D("wsock_write failed: %d, connection may be closing\n", err);
                    // 写入失败，标记连接断开
                    if (err == ERR_CLSD || err == ERR_RST)
                    {
                        g_xz_ws.is_connected = 0;
                    }
                }
            }
            rt_mutex_release(g_xz_ws.ws_write_mutex);
        }
        else
        {
            // 无法获取锁或未初始化，跳过此次发送
            LOG_D("WebSocket write busy, skip audio data\n");
        }
    }
}

err_t my_wsapp_fn(int code, char *buf, size_t len)
{
    switch (code)
    {
    case WS_CONNECT:
        LOG_I("websocket connected\n");
        if ((uint16_t)(uint32_t)buf == 101)
        {
            g_xz_ws.is_connected = 1;
            LOG_I("g_xz_ws.is_connected = 1\n");
            rt_sem_release(g_xz_ws.sem);
        }
        break;
    case WS_DISCONNECT:
        // 获取写入锁，确保没有正在进行的写入操作
        if (g_xz_ws.ws_write_mutex)
        {
            rt_mutex_take(g_xz_ws.ws_write_mutex, RT_WAITING_FOREVER);
        }

        /* Ignore disconnect callback during reconnection to avoid state confusion */
        if (WEBSOCKET_RECONNECT_FLAG == 1)
        {
            LOG_D("Ignore disconnect during reconnect\n");
            if (g_xz_ws.ws_write_mutex)
            {
                rt_mutex_release(g_xz_ws.ws_write_mutex);
            }
            break;
        }
        /* Ignore disconnect callback if already disconnected */
        if (!g_xz_ws.is_connected)
        {
            LOG_D("Ignore disconnect when not connected\n");
            if (g_xz_ws.ws_write_mutex)
            {
                rt_mutex_release(g_xz_ws.ws_write_mutex);
            }
            break;
        }

        /* 连接断开时确保所有音频资源清理 */
        if (g_state == kDeviceStateListening)
        {
            xz_mic(0);
            LOG_D("Stopped microphone due to disconnection\n");
        }
        else if (g_state == kDeviceStateSpeaking)
        {
            xz_speaker(0);
            LOG_D("Stopped speaker due to disconnection\n");
        }

        xiaozhi_ui_chat_status("   休眠中");
        xiaozhi_ui_chat_output("等待唤醒");
        xiaozhi_ui_update_emoji("sleepy");
        LOG_I("WebSocket closed\n");
        g_xz_ws.is_connected = 0;
        g_state = kDeviceStateUnknown;

        /* 释放写入锁 */
        if (g_xz_ws.ws_write_mutex)
        {
            rt_mutex_release(g_xz_ws.ws_write_mutex);
        }

        /* 清除重连时间戳，允许立即重连 */
        last_reconnect_time = 0;
        break;
    case WS_TEXT:
        Message_handle((const uint8_t *)buf, len);
        break;
    case WS_DATA:
        xz_audio_downlink((uint8_t *)buf, len, NULL, 0);
        break;
    default:
        LOG_E("Unknown error\n");
        break;
    }
    return 0;
}

void reconnect_websocket(void)
{
    err_t result;
    uint32_t retry = 10;
    uint32_t current_time = rt_tick_get();

    /* 防止频繁重连：距离上次重连至少5秒 */
    if (WEBSOCKET_RECONNECT_FLAG == 1)
    {
        LOG_D("Reconnection already in progress, ignoring duplicate request\n");
        return;
    }

    if (current_time - last_reconnect_time < rt_tick_from_millisecond(5000))
    {
        LOG_D("Reconnection too frequent, ignoring request\n");
        return;
    }

    last_reconnect_time = current_time;

    /* Set reconnect flag to ignore disconnect callbacks during reconnection */
    WEBSOCKET_RECONNECT_FLAG = 1;

    while (retry-- > 0)
    {
        /* 检查 WebSocket 的 TCP 控制块状态，避免在不合适时机重连 */
        if (g_xz_ws.clnt.pcb != RT_NULL)
        {
            LOG_D("WebSocket PCB exists, current state: %d\n", g_xz_ws.clnt.pcb->state);

            /* 只有在状态异常时才进行清理 */
            if (g_xz_ws.clnt.pcb->state != CLOSED && g_xz_ws.clnt.pcb->state != CLOSE_WAIT)
            {
                LOG_I("Cleaning up WebSocket connection in state %d\n", g_xz_ws.clnt.pcb->state);

                /* 先重置连接标志，避免重连回调干扰 */
                g_xz_ws.is_connected = 0;

                /* 尝试正常关闭 */
                err_t close_result = wsock_close(&g_xz_ws.clnt, WSOCK_RESULT_LOCAL_ABORT, ERR_OK);
                LOG_D("wsock_close result: %d\n", close_result);

                /* 给系统充分时间清理资源 */
                rt_thread_mdelay(2000);

                /* 检查是否成功关闭 */
                if (g_xz_ws.clnt.pcb != RT_NULL && g_xz_ws.clnt.pcb->state != CLOSED)
                {
                    LOG_W("WebSocket PCB still exists after close, forcing cleanup\n");
                    /* 强制清理，但要小心 */
                    rt_thread_mdelay(1000);
                    memset(&g_xz_ws.clnt, 0, sizeof(wsock_state_t));
                }
            }
        }

        /* 确保连接状态重置 */
        g_xz_ws.is_connected = 0;

        if (!g_xz_ws.sem)
        {
            g_xz_ws.sem = rt_sem_create("xz_ws", 0, RT_IPC_FLAG_FIFO);
        }
        else
        {
            /* Reset semaphore to avoid stale signals */
            while (rt_sem_trytake(g_xz_ws.sem) == RT_EOK);
        }

        char *client_id = get_client_id();

        /* 确保WebSocket结构体完全清理 */
        memset(&g_xz_ws.clnt, 0, sizeof(wsock_state_t));

        wsock_init(&g_xz_ws.clnt, 1, 1, my_wsapp_fn);
        result = wsock_connect(&g_xz_ws.clnt, MAX_WSOCK_HDR_LEN,
                               XIAOZHI_HOST, XIAOZHI_WSPATH,
                               LWIP_IANA_PORT_HTTPS, XIAOZHI_TOKEN, NULL,
                               "Protocol-Version: 1\r\nDevice-Id: %s\r\nClient-Id: %s\r\n",
                               get_mac_address(), client_id);
        LOG_I("Web socket connection attempt %d: %d\n", 10 - retry, result);
        if (result == 0)
        {
            /* 使用更长的超时时间，参考优秀实践 */
            if (rt_sem_take(g_xz_ws.sem, 50000) == RT_EOK)
            {
                if (g_xz_ws.is_connected)
                {
                    /* Reconnection successful, clear reconnect flag */
                    WEBSOCKET_RECONNECT_FLAG = 0;
                    result = wsock_write(&g_xz_ws.clnt, HELLO_MESSAGE,
                                         strlen(HELLO_MESSAGE), OPCODE_TEXT);
                    LOG_I("Web socket write %d\r\n", result);
                    if (result == ERR_OK)
                    {
                        LOG_I("WebSocket reconnection successful\n");
                        return;
                    }
                    else
                    {
                        LOG_E("Failed to send hello message: %d, retrying...\n", result);
                    }
                }
                else
                {
                    LOG_E("Web socket connection established but not properly initialized, retrying...\n");
                }
            }
            else
            {
                LOG_E("Web socket connection timeout after 50 seconds, retrying...\n");
            }
        }
        else
        {
            LOG_E("Web socket connect failed: %d, retry %d remaining...\n", result, retry);
        }

        /* 智能重连间隔：失败次数越多，间隔越长 */
        uint32_t delay_ms = 2000 + (10 - retry) * 500;  // 2s-6s递增
        LOG_D("Waiting %d ms before next retry...\n", delay_ms);
        rt_thread_mdelay(delay_ms);
    }

    /* Reconnection failed, clear reconnect flag */
    WEBSOCKET_RECONNECT_FLAG = 0;
    LOG_E("Web socket reconnect failed after all retries\n");

    // 重置状态
    g_state = kDeviceStateUnknown;
    xiaozhi_ui_chat_status("   连接失败");
    xiaozhi_ui_chat_output("请重试");
}

void xz_ws_audio_init(void)
{
    static uint8_t init_flag = 1;
    if (init_flag)
    {
        xz_audio_decoder_encoder_open(1);
        xz_mic_init();
        xz_button_init();
        voice_rx_init();
        xz_sound_init();
        init_flag = 0;

    }
}

/* IoT Functions */
void send_iot_states(void)
{
    const char *state = iot_get_states_json();
    if (state == NULL)
    {
        rt_kprintf("Failed to get IoT states\n");
        return;
    }

    // Dynamically allocate buffer since state may be long
    int state_len = strlen(state);
    int msg_size = state_len + 256; // Extra space for session_id etc.
    char *msg = (char *)rt_malloc(msg_size);
    if (msg == NULL)
    {
        rt_kprintf("Failed to allocate memory for IoT states\n");
        return;
    }

    snprintf(msg, msg_size,
             "{\"session_id\":\"%s\",\"type\":\"iot\",\"update\":true,"
             "\"states\":%s}",
             g_xz_ws.session_id, state);
    rt_kprintf("Sending IoT states:\n%s\n", msg);
    if (g_xz_ws.is_connected)
    {
        wsock_write(&g_xz_ws.clnt, msg, strlen(msg), OPCODE_TEXT);
    }
    else
    {
        //rt_kprintf("websocket is not connected\n");
    }
    rt_free(msg);
}

void send_iot_descriptors(void)
{
    const char *desc = iot_get_descriptors_json();
    if (desc == NULL)
    {
        rt_kprintf("Failed to get IoT descriptors\n");
        return;
    }

    // Dynamically allocate buffer since descriptor may be long
    int desc_len = strlen(desc);
    int msg_size = desc_len + 256; // Extra space for session_id etc.
    char *msg = (char *)rt_malloc(msg_size);
    if (msg == NULL)
    {
        rt_kprintf("Failed to allocate memory for IoT descriptors\n");
        return;
    }

    snprintf(msg, msg_size,
             "{\"session_id\":\"%s\",\"type\":\"iot\",\"update\":true,"
             "\"descriptors\":%s}",
             g_xz_ws.session_id, desc);
    rt_kprintf("Sending IoT descriptors:\n%s\n", msg);
    if (g_xz_ws.is_connected)
    {
        wsock_write(&g_xz_ws.clnt, msg, strlen(msg), OPCODE_TEXT);
    }
    else
    {
        //rt_kprintf("websocket is not connected\n");
    }
    rt_free(msg);
}

/* Message Handling */
char *my_json_string(cJSON *json, char *key)
{
    cJSON *item = cJSON_GetObjectItem(json, key);
    if (item && cJSON_IsString(item))
    {
        return item->valuestring;
    }
    return "";
}

void Message_handle(const uint8_t *data, uint16_t len)
{
//    rt_kputs(data);
    cJSON *root = cJSON_Parse((const char *)data);
    if (!root)
    {
        LOG_E("Error before: [%s]\n", cJSON_GetErrorPtr());
        return;
    }

    char *type = my_json_string(root, "type");
    if (strcmp(type, "hello") == 0)
    {
        char *session_id = cJSON_GetObjectItem(root, "session_id")->valuestring;
        cJSON *audio_param = cJSON_GetObjectItem(root, "audio_params");
        g_xz_ws.sample_rate = cJSON_GetObjectItem(audio_param, "sample_rate")->valueint;
        g_xz_ws.frame_duration = cJSON_GetObjectItem(audio_param, "frame_duration")->valueint;
        strncpy(g_xz_ws.session_id, session_id, 9);
        g_state = kDeviceStateIdle;
        xz_ws_audio_init();

        /* 优雅处理IoT初始化：确保只在首次连接时初始化 */
        if (!iot_initialized)
        {
            LOG_I("Initializing IoT devices for first time\n");
            extern void iot_initialize(void);
            iot_initialize();
            iot_initialized = 1;
        }
        else
        {
            LOG_D("IoT already initialized, skipping repeated initialization\n");
        }

        /* 每次重连后都需要重新发送设备信息 */
        send_iot_descriptors();
        send_iot_states();
        xiaozhi_ui_chat_status("   待命中");
        xiaozhi_ui_chat_output(" ");
        xiaozhi_ui_update_emoji("neutral");
        LOG_I("Waiting...\n");
    }
    else if (strcmp(type, "goodbye") == 0)
    {
        xiaozhi_ui_chat_status("   休眠中");
        xiaozhi_ui_chat_output("等待唤醒");
        xiaozhi_ui_update_emoji("sleepy");
        g_state = kDeviceStateUnknown;
        LOG_I("session ended\n");
    }
    else if (strcmp(type, "tts") == 0)
    {
        char *state = my_json_string(root, "state");
        if (strcmp(state, "start") == 0)
        {
            if (g_state == kDeviceStateIdle || g_state == kDeviceStateListening)
            {
                /* 确保麦克风在开始TTS前关闭 */
                if (g_state == kDeviceStateListening)
                {
                    xz_mic(0);
                }

                g_state = kDeviceStateSpeaking;
                xiaozhi_ui_chat_status("   说话中");
                xz_speaker(1);
                LOG_D("State transitioned to Speaking, microphone stopped\n");
            }
            else
            {
                LOG_D("Already in Speaking state, ignoring duplicate start\n");
            }
        }
        else if (strcmp(state, "stop") == 0)
        {
            g_state = kDeviceStateIdle;
            xz_speaker(0);
            xiaozhi_ui_chat_status("   就绪");
            xiaozhi_ui_chat_output("就绪");
            LOG_D("TTS stopped, state reset to Idle\n");
        }
        else if (strcmp(state, "sentence_start") == 0)
        {
            LOG_I("tts:%s", my_json_string(root, "text"));
            xiaozhi_ui_chat_output(my_json_string(root, "text"));
        }
    }
    else if (strcmp(type, "llm") == 0)
    {
        // rt_kputs(cJSON_GetObjectItem(root, "emotion")->valuestring);
        rt_kprintf(cJSON_GetObjectItem(root, "emotion")->valuestring);
        xiaozhi_ui_update_emoji(
            cJSON_GetObjectItem(root, "emotion")->valuestring);
    }
    else if (strcmp(type, "stt") == 0)
    {
        LOG_I("stt:%s", cJSON_GetObjectItem(root, "text")->valuestring);
    }
    else if (strcmp(type, "iot") == 0)
    {
        rt_kprintf("iot command\n");
        cJSON *commands = cJSON_GetObjectItem(root, "commands");
        for (int i = 0; i < cJSON_GetArraySize(commands); i++)
        {
            cJSON *cmd = cJSON_GetArrayItem(commands, i);
            char *cmd_str = cJSON_PrintUnformatted(cmd);
            if (cmd_str)
            {
                iot_invoke((uint8_t *)cmd_str, strlen(cmd_str));
                send_iot_states();
                cJSON_free(cmd_str);
            }
        }
    }
    else if (strcmp(type, "mcp") == 0)
    {
        rt_kprintf("mcp command\n");
        cJSON *payload = cJSON_GetObjectItem(root, "payload");
        if (payload && cJSON_IsObject(payload))
        {
            extern void McpServer_ParseMessage(const char *message);
            char *payload_str = cJSON_PrintUnformatted(payload);
            if (payload_str)
            {
                McpServer_ParseMessage(payload_str);
                cJSON_free(payload_str);
            }
        }
    }
    else if (strcmp(type, "error") == 0)
    {
        cJSON *message = cJSON_GetObjectItem(root, "message");
        if (message && cJSON_IsString(message))
        {
            LOG_E("Server error: %s\n", message->valuestring);
        }
        else
        {
            LOG_E("Server returned error\n");
        }
    }
    else
    {
        LOG_E("Unknown type: %s\n", type);
    }
    cJSON_Delete(root);
}

/* Network Functions */
void svr_found_callback(const char *name, const ip_addr_t *ipaddr, void *callback_arg)
{
    if (ipaddr != NULL)
    {
        rt_kprintf("DNS lookup succeeded, IP: %s\n", ipaddr_ntoa(ipaddr));
    }
}

int check_internet_access(void)
{
    const char *hostname = XIAOZHI_HOST;
    ip_addr_t addr = {0};
    err_t err = dns_gethostbyname(hostname, &addr, svr_found_callback, NULL);
    return (err == ERR_OK || err == ERR_INPROGRESS) ? 1 : 0;
}

char *get_xiaozhi_ws(void)
{
    char *buffer = RT_NULL;
    int resp_status;
    struct webclient_session *session = RT_NULL;
    char *xiaozhi_url = RT_NULL;
    int content_length = -1, bytes_read = 0, content_pos = 0;

    if (!check_internet_access())
    {
        return buffer;
    }

    int size = strlen(OTA_VERSION) + sizeof(client_id_string) +
               sizeof(mac_address_string) * 2 + 16;
    char *ota_formatted = rt_malloc(size);
    if (!ota_formatted)
    {
        goto __exit;
    }

    rt_snprintf(ota_formatted, size, OTA_VERSION, get_mac_address(),
                get_client_id(), get_mac_address());
    xiaozhi_url = rt_calloc(1, GET_URL_LEN_MAX);
    if (!xiaozhi_url)
    {
        LOG_E("No memory for xiaozhi_url!\n");
        goto __exit;
    }

    rt_snprintf(xiaozhi_url, GET_URL_LEN_MAX, GET_URI, XIAOZHI_HOST);
    session = webclient_session_create(1024);
    if (!session)
    {
        LOG_E("No memory for get header!\n");
        goto __exit;
    }

    webclient_header_fields_add(session, "Device-Id: %s \r\n", get_mac_address());
    webclient_header_fields_add(session, "Client-Id: %s \r\n", get_client_id());
    webclient_header_fields_add(session, "Content-Type: application/json \r\n");
    webclient_header_fields_add(session, "Content-length: %d \r\n",
                                strlen(ota_formatted));

    if ((resp_status = webclient_post(session, xiaozhi_url, ota_formatted,
                                      strlen(ota_formatted))) != 200)
    {
        LOG_E("webclient Post request failed, response(%d) error.\n", resp_status);
    }

    buffer = rt_calloc(1, GET_RESP_BUFSZ);
    if (!buffer)
    {
        LOG_E("No memory for data receive buffer!\n");
        goto __exit;
    }

    content_length = webclient_content_length_get(session);
    if (content_length > 0)
    {
        do
        {
            bytes_read = webclient_read(session, buffer + content_pos,
                                        content_length - content_pos > GET_RESP_BUFSZ
                                        ? GET_RESP_BUFSZ
                                        : content_length - content_pos);
            if (bytes_read <= 0)
            {
                break;
            }
            content_pos += bytes_read;
        }
        while (content_pos < content_length);
    }
    else
    {
        rt_free(buffer);
        buffer = NULL;
    }

__exit:
    if (xiaozhi_url) rt_free(xiaozhi_url);
    if (session) webclient_close(session);
    if (ota_formatted) rt_free(ota_formatted);
    return buffer;
}

int http_xiaozhi_data_parse_ws(char *json_data)
{
    cJSON *root = cJSON_Parse(json_data);
    if (!root)
    {
        LOG_E("Error before: [%s]\n", cJSON_GetErrorPtr());
        return -1;
    }

    xiaozhi_ws_connect();
    cJSON_Delete(root);
    return 0;
}

void xiaozhi_ws_connect(void)
{
    err_t err;
    uint32_t retry = 10;

    while (retry-- > 0)
    {
        /* 检查网络连接状态 */
        if (!check_internet_access())
        {
            LOG_I("Waiting internet ready... (%d retries remaining)\n", retry);
            xiaozhi_ui_chat_status("   等待网络");
            xiaozhi_ui_chat_output("检查网络连接...");
            rt_thread_mdelay(2000);
            continue;
        }

        /* 确保WebSocket处于正确的状态 */
        if (g_xz_ws.clnt.pcb != RT_NULL && g_xz_ws.clnt.pcb->state != CLOSED)
        {
            LOG_D("Cleaning up existing WebSocket connection\n");
            wsock_close(&g_xz_ws.clnt, WSOCK_RESULT_LOCAL_ABORT, ERR_OK);
            rt_thread_mdelay(1000);
        }

        if (!g_xz_ws.sem)
        {
            g_xz_ws.sem = rt_sem_create("xz_ws", 0, RT_IPC_FLAG_FIFO);
        }

        // 创建WebSocket写入互斥锁
        if (!g_xz_ws.ws_write_mutex)
        {
            g_xz_ws.ws_write_mutex = rt_mutex_create("xz_ws_write", RT_IPC_FLAG_FIFO);
            if (!g_xz_ws.ws_write_mutex)
            {
                LOG_E("Failed to create WebSocket write mutex\n");
                rt_thread_mdelay(1000);
                continue;
            }
        }

        wsock_init(&g_xz_ws.clnt, 1, 1, my_wsapp_fn);
        char *client_id = get_client_id();
        err = wsock_connect(&g_xz_ws.clnt, MAX_WSOCK_HDR_LEN,
                            XIAOZHI_HOST, XIAOZHI_WSPATH,
                            LWIP_IANA_PORT_HTTPS, XIAOZHI_TOKEN, NULL,
                            "Protocol-Version: 1\r\nDevice-Id: %s\r\nClient-Id: %s\r\n",
                            get_mac_address(), client_id);
        LOG_I("Web socket connection attempt %d: %d\n", 10 - retry, err);

        if (err == 0)
        {
            /* 连接成功，等待握手完成 */
            if (rt_sem_take(g_xz_ws.sem, 50000) == RT_EOK)
            {
                LOG_I("WebSocket handshake completed, connected=%d\n", g_xz_ws.is_connected);
                if (g_xz_ws.is_connected)
                {
                    err = wsock_write(&g_xz_ws.clnt, HELLO_MESSAGE,
                                      strlen(HELLO_MESSAGE), OPCODE_TEXT);
                    if (err == ERR_OK)
                    {
                        LOG_I("Initial WebSocket connection established successfully\n");
                        return;
                    }
                    else
                    {
                        LOG_E("Failed to send hello message: %d\n", err);
                    }
                }
                else
                {
                    LOG_E("WebSocket connected but not properly initialized\n");
                }
            }
            else
            {
                LOG_E("WebSocket connection timeout after 50 seconds\n");
            }
        }
        else
        {
            LOG_E("WebSocket connection failed: %d, %d retries remaining\n", err, retry);
        }

        /* 连接失败，更新UI状态 */
        if (retry > 0)
        {
            xiaozhi_ui_chat_status("   连接失败");
            char retry_msg[64];
            rt_snprintf(retry_msg, sizeof(retry_msg), "Retrying... (%d)", 10 - retry);
            xiaozhi_ui_chat_output(retry_msg);
            rt_thread_mdelay(3000);  /* 增加初始连接失败的重试间隔 */
        }
    }

    /* 所有重试都失败了 */
    LOG_E("WebSocket connection failed after all attempts\n");
    xiaozhi_ui_chat_status("   连接失败");
    xiaozhi_ui_chat_output("请检查网络并重试");
}

/* Main Entry */
void xiaozhi_entry(void *p)
{
    char *my_ota_version;
    while (1)
    {
        my_ota_version = get_xiaozhi_ws();
        if (my_ota_version)
        {
            http_xiaozhi_data_parse_ws(my_ota_version);
            rt_free(my_ota_version);
            break;
        }
        else
        {
            LOG_E("Waiting internet... \n");
            rt_thread_mdelay(1000);
        }
    }
}

int ws_xiaozhi_init(void)
{
    xiaozhi_tid = rt_thread_create("xiaozhi_thread", xiaozhi_entry,
                                   (void *)0x01, 1024 * 30, 15, 5);
    if (!xiaozhi_tid)
    {
        LOG_E("[%s] Create failed!\n", __FUNCTION__);
        return -RT_ENOMEM;
    }

    if (rt_thread_startup(xiaozhi_tid) != RT_EOK)
    {
        LOG_E("[%s] Startup failed!\n", __FUNCTION__);
        return -RT_ERROR;
    }

    LOG_I("[%s] Created successfully\n", __FUNCTION__);
    return RT_EOK;
}

MSH_CMD_EXPORT(ws_xiaozhi_init, Xiaozhi demo by websocket);

static char msg_pool[256];
struct rx_msg
{
    rt_device_t dev;
    rt_size_t size;
};
static rt_device_t serial;
static struct rt_messagequeue rx_mq;
static rt_err_t uart_input(rt_device_t dev, rt_size_t size)
{
    struct rx_msg msg;
    rt_err_t result;
    msg.dev = dev;
    msg.size = size;

    result = rt_mq_send(&rx_mq, &msg, sizeof(msg));
    if (result == -RT_EFULL)
    {

        rt_kprintf("message queue full！\n");
    }
    return result;
}

static void serial_thread_entry(void *parameter)
{
    struct rx_msg msg;
    rt_err_t result;
    rt_uint32_t rx_length;
    static char rx_buffer[RT_SERIAL_RB_BUFSZ + 1];
    char cmd[] = {0xAA, 0x55, 0x01, 0x55, 0xAA};

    while (1)
    {
        rt_memset(&msg, 0, sizeof(msg));

        result = rt_mq_recv(&rx_mq, &msg, sizeof(msg), RT_WAITING_FOREVER);
        if (result > 0)
        {
            rx_length = rt_device_read(msg.dev, 0, rx_buffer, msg.size);
            rx_buffer[rx_length] = '\0';
            rt_kprintf("%s", rx_buffer);
            if (rx_buffer[0] == 0x01)
            {
                rt_device_write(serial, 0, cmd, (sizeof(cmd)));
                rt_event_send(xiaozhi_button_event, BUTTON_EVENT_PRESSED);
            }
        }
    }
}

void voice_rx_init()
{
    serial = rt_device_find("uart1");
    if (serial == RT_NULL)
    {
        rt_kprintf("device %s not find", "uart1");
        return;
    }

    rt_device_open(serial, RT_DEVICE_FLAG_INT_RX);
    rt_device_set_rx_indicate(serial, uart_input);

    rt_mq_init(&rx_mq, "rx_mq",
               msg_pool,
               sizeof(struct rx_msg),
               sizeof(msg_pool),
               RT_IPC_FLAG_FIFO);

    rt_thread_t thread_t = rt_thread_create("serial_voice", serial_thread_entry, RT_NULL, 1024, 7, 10);
    rt_thread_startup(thread_t);
}
