#include "xiaozhi.h"
#include <cJSON.h>
#include <lwip/api.h>
#include <lwip/apps/websocket_client.h>
#include <lwip/dns.h>
#include <lwip/tcpip.h>
#include <netdev.h>
#include <rtdevice.h>
#include <rtthread.h>
#include <webclient.h>

#define DBG_TAG "xz.ws"
#define DBG_LVL DBG_LOG
#include <rtdbg.h>

/* Global Variables */
static rt_thread_t xiaozhi_tid = RT_NULL;
static const char *client_id = "af7ac552-9991-4b31-b660-683b210ae95f";
static uint8_t WEBSOCKET_RECONNECT_FLAG = 0;
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

        if (g_state == kDeviceStateUnknown || !g_xz_ws.is_connected)
        {
            LOG_I("Wake up...\n");
            if (evt & BUTTON_EVENT_PRESSED)
            {
                xiaozhi_ui_chat_status("   Connecting");
                reconnect_websocket();
                WEBSOCKET_RECONNECT_FLAG = 1;
            }
        }
        else
        {
            if (evt & BUTTON_EVENT_PRESSED)
            {
                if (g_state == kDeviceStateSpeaking)
                {
                    LOG_I("Speaking aborted\n");
                    xz_speaker(0);
                }
                if (g_state != kDeviceStateListening)
                {
                    ws_send_listen_start(&g_xz_ws.clnt, g_xz_ws.session_id,
                                         kListeningModeAutoStop);
                    LOG_I("Listening...\n");
                    xiaozhi_ui_chat_status("   Listening");
                    xz_mic(1);
                    g_state = kDeviceStateListening;
                }
            }
            if (evt & BUTTON_EVENT_RELEASED)
            {
                if (g_state == kDeviceStateListening)
                {
                    ws_send_listen_stop(&g_xz_ws.clnt, g_xz_ws.session_id);
                    xz_mic(0);
                    g_state = kDeviceStateIdle;
                }
            }
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
    if (g_xz_ws.is_connected)
    {
        wsock_write((wsock_state_t *)ws, message, strlen(message), OPCODE_TEXT);
    }
    else
    {
        //LOG_E("websocket is not connected\n");
    }
}

void ws_send_listen_stop(void *ws, char *session_id)
{
    static char message[256];
    rt_snprintf(message, 256,
                "{\"session_id\":\"%s\",\"type\":\"listen\",\"state\":\"stop\"}",
                session_id);
    if (g_xz_ws.is_connected)
    {
        err_t err = wsock_write((wsock_state_t *)ws, message, strlen(message),
                                OPCODE_TEXT);
        LOG_E("ws_send_listen_stop = %d \n", err);
    }
    else
    {
        //LOG_E("websocket is not connected\n");
    }
}

void ws_send_hello(void *ws)
{
    if (g_xz_ws.is_connected)
    {
        wsock_write((wsock_state_t *)ws, HELLO_MESSAGE,
                    strlen(HELLO_MESSAGE), OPCODE_TEXT);
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
        wsock_write(&g_xz_ws.clnt, (const char *)data, len, OPCODE_BINARY);
    }
    else
    {
        // LOG_E("Websocket disconnected\n");
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
            rt_sem_release(g_xz_ws.sem);
            g_xz_ws.is_connected = 1;
        }
        break;
    case WS_DISCONNECT:
        if (WEBSOCKET_RECONNECT_FLAG == 1 ||
                (!g_xz_ws.is_connected && WEBSOCKET_RECONNECT_FLAG == 0))
        {
            WEBSOCKET_RECONNECT_FLAG = 0;
            g_xz_ws.is_connected = 0;
            g_state = kDeviceStateUnknown;
            break;
        }
        // extern void poweroff(void);
        // poweroff();
        xiaozhi_ui_chat_status("   Sleeping");
        xiaozhi_ui_chat_output("Waiting for wake-up");
        xiaozhi_ui_update_emoji("sleepy");
        LOG_I("WebSocket closed\n");
        g_xz_ws.is_connected = 0;
        g_state = kDeviceStateUnknown;
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
    while (retry-- > 0)
    {
        wsock_close(&g_xz_ws.clnt, WSOCK_RESULT_LOCAL_ABORT, ERR_OK);
        if (!g_xz_ws.sem)
        {
            g_xz_ws.sem = rt_sem_create("xz_ws", 0, RT_IPC_FLAG_FIFO);
        }
        char *client_id = get_client_id();
        wsock_init(&g_xz_ws.clnt, 1, 1, my_wsapp_fn);
        result = wsock_connect(&g_xz_ws.clnt, MAX_WSOCK_HDR_LEN,
                               XIAOZHI_HOST, XIAOZHI_WSPATH,
                               LWIP_IANA_PORT_HTTPS, XIAOZHI_TOKEN, NULL,
                               "Protocol-Version: 1\r\nDevice-Id: %s\r\nClient-Id: %s\r\n",
                               get_mac_address(), client_id);
        if (result == 0)
        {
            if (rt_sem_take(g_xz_ws.sem, 10000) == RT_EOK)
            {
                if (g_xz_ws.is_connected)
                {
                    result = wsock_write(&g_xz_ws.clnt, HELLO_MESSAGE,
                                         strlen(HELLO_MESSAGE), OPCODE_TEXT);
                    LOG_I("Web socket write %d\r\n", result);
                    break;
                }
                else
                {
                    LOG_E("Web socket disconnected\n");
                }
            }
            else
            {
                LOG_E("Web socket connected timeout\n");
            }
        }
        else
        {
            rt_thread_mdelay(1000);
        }
    }
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

    // 动态分配缓冲区，因为状态可能很长
    int state_len = strlen(state);
    int msg_size = state_len + 256; // 额外空间用于session_id等
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

    // 动态分配缓冲区，因为描述符可能很长
    int desc_len = strlen(desc);
    int msg_size = desc_len + 256; // 额外空间用于session_id等
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
        send_iot_descriptors();
        send_iot_states();
        xiaozhi_ui_chat_status("   On standby");
        xiaozhi_ui_chat_output(" ");
        xiaozhi_ui_update_emoji("neutral");
        LOG_I("Waiting...\n");
    }
    else if (strcmp(type, "goodbye") == 0)
    {
        xiaozhi_ui_chat_status("   Sleeping");
        xiaozhi_ui_chat_output("Waiting for wake-up");
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
                g_state = kDeviceStateSpeaking;
                xiaozhi_ui_chat_status("   Speaking");
                xz_speaker(1);
            }
        }
        else if (strcmp(state, "stop") == 0)
        {
            g_state = kDeviceStateIdle;
            xz_speaker(0);
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
    while (1)
    {
        wsock_close(&g_xz_ws.clnt, WSOCK_RESULT_LOCAL_ABORT, ERR_OK);
        if (!g_xz_ws.sem)
        {
            g_xz_ws.sem = rt_sem_create("xz_ws", 0, RT_IPC_FLAG_FIFO);
        }
        wsock_init(&g_xz_ws.clnt, 1, 1, my_wsapp_fn);
        char *client_id = get_client_id();
        err = wsock_connect(&g_xz_ws.clnt, MAX_WSOCK_HDR_LEN,
                            XIAOZHI_HOST, XIAOZHI_WSPATH,
                            LWIP_IANA_PORT_HTTPS, XIAOZHI_TOKEN, NULL,
                            "Protocol-Version: 1\r\nDevice-Id: %s\r\nClient-Id: %s\r\n",
                            get_mac_address(), client_id);
        LOG_I("Web socket connection %d\r\n", err);
        if (err == 0)
        {
            if (rt_sem_take(g_xz_ws.sem, 10000) == RT_EOK)
            {
                LOG_I("g_xz_ws.is_connected = %d\n", g_xz_ws.is_connected);
                if (g_xz_ws.is_connected)
                {
                    err = wsock_write(&g_xz_ws.clnt, HELLO_MESSAGE,
                                      strlen(HELLO_MESSAGE), OPCODE_TEXT);
                    break;
                }
                else
                {
                    LOG_E("Web socket disconnected\n");
                }
            }
            else
            {
                LOG_E("Web socket connected timeout\n");
            }
        }
        else
        {
            LOG_I("Waiting internet ready...\n");
            rt_thread_mdelay(1000);
        }
    }
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
            extern void iot_initialize(void);
            iot_initialize();
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
