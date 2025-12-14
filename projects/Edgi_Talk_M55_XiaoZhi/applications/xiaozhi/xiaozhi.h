#ifndef XIAOZHI_H
#define XIAOZHI_H

#include "board.h"
#include "lwip/api.h"
#include "lwip/dns.h"
#include "lwip/apps/mqtt_priv.h"
#include "lwip/apps/mqtt.h"
#include "lwip/udp.h"
#include "lwip/tcp.h"
#include "lwip/tcpip.h"
#include "lwip/apps/websocket_client.h"
#include "opus_multistream.h"
#include "opus_types.h"
#include <cJSON.h>
#include <rtthread.h>

#ifdef __cplusplus
extern "C" {
#endif

#define GET_HEADER_BUFSZ        1024
#define GET_RESP_BUFSZ          2048
#define GET_URL_LEN_MAX         256
#define GET_URI                 "https://%s/xiaozhi/ota/"
#define XIAOZHI_HOST            "api.tenclass.net"
#define XIAOZHI_WSPATH          "/xiaozhi/v1/"
#define XIAOZHI_TOKEN           "Bearer 12345678"
#define MAX_WSOCK_HDR_LEN       4096
#define XZ_MIC_FRAME_LEN        (320 * 6 * 2)
#define XZ_SPK_FRAME_LEN        (480 * 6)
#define XZ_EVENT_MIC_RX         (1 << 0)
#define XZ_EVENT_DOWNLINK       (1 << 1)
#define XZ_EVENT_ALL            (XZ_EVENT_MIC_RX | XZ_EVENT_DOWNLINK)
#define XZ_DOWNLINK_QUEUE_NUM   256
#define MAX_CLIENT_ID_LEN 40
#define MAX_MAC_ADDR_LEN 20

#define BUTTON_EVENT_PRESSED    (1 << 3)
#define BUTTON_EVENT_RELEASED   (1 << 4)
#define TIMEOUT_EVENT           (1 << 5)

#define MIC_EVENT_OPEN          (1 << 0)
#define MIC_EVENT_CLOSE         (1 << 1)

#define BUTTON_PIN              GET_PIN(8,3)
#define THREAD_STACK_SIZE       (1024 * 4)
#define THREAD_PRIORITY         16
#define BUTTON_DEBOUNCE_MS      20

#define HELLO_MESSAGE  "{ " \
    "\"type\":\"hello\"," \
    "\"version\":3," \
    "\"features\":{\"mcp\":true}," \
    "\"transport\":\"websocket\"," \
    "\"audio_params\":{" \
        "\"format\":\"opus\", \"sample_rate\":16000, \"channels\":1, " \
        "\"frame_duration\":60" \
    "}}"

#define OTA_VERSION \
    "{\"version\":2,\"flash_size\":4194304,\"psram_size\":0," \
    "\"minimum_free_heap_size\":123456,\"mac_address\":\"%s\"," \
    "\"uuid\":\"%s\",\"chip_model_name\":\"psocedge84\"," \
    "\"chip_info\":{\"model\":1,\"cores\":2,\"revision\":0,\"features\":0}," \
    "\"application\":{\"name\":\"my-app\",\"version\":\"1.0.0\"," \
    "\"compile_time\":\"2021-01-01T00:00:00Z\",\"idf_version\":\"4.2-dev\"," \
    "\"elf_sha256\":\"\"},\"partition_table\":[{\"label\":\"app\"," \
    "\"type\":1,\"subtype\":2,\"address\":10000,\"size\":100000}], " \
    "\"ota\":{\"label\":\"ota_0\"},\"board\":{\"type\":\"hdk563\"," \
    "\"mac\":\"%s\"}}"

enum ListeningMode
{
    kListeningModeAutoStop,
    kListeningModeManualStop,
    kListeningModeAlwaysOn
};

enum AbortReason
{
    kAbortReasonNone,
    kAbortReasonWakeWordDetected
};

enum DeviceState
{
    kDeviceStateUnknown,
    kDeviceStateStarting,
    kDeviceStateWifiConfiguring,
    kDeviceStateIdle,
    kDeviceStateConnecting,
    kDeviceStateListening,
    kDeviceStateSpeaking,
    kDeviceStateUpgrading,
    kDeviceStateActivating,
    kDeviceStateFatalError
};

typedef struct
{
    uint32_t sample_rate;
    uint32_t frame_duration;
    char session_id[12];
    wsock_state_t clnt;
    rt_sem_t sem;
    uint8_t is_connected;
    rt_mutex_t ws_write_mutex;  // WebSocket写入互斥锁
} xiaozhi_ws_t;

/* Application state structure */
typedef struct
{
    rt_thread_t xiaozhi_tid;
    const char *client_id;
    uint8_t websocket_reconnect_flag;
    uint8_t iot_initialized;
    uint32_t last_reconnect_time;
    char mac_address_string[MAX_MAC_ADDR_LEN];
    char client_id_string[MAX_CLIENT_ID_LEN];
    xiaozhi_ws_t ws;
    enum DeviceState state;
    rt_event_t button_event;
    int wakeword_initialized_session;
    rt_bool_t multi_turn_conversation_enabled;  /* 多轮对话开关 */
    rt_timer_t tts_sentence_end_timer;  /* TTS句子结束定时器 */
} xiaozhi_app_t;

extern enum DeviceState g_state;
extern xiaozhi_ws_t g_xz_ws;

/* Structures */
typedef struct
{
    rt_slist_t node;
    uint8_t *data;
    uint16_t data_len;
    uint16_t size;
} xz_decode_queue_t;

typedef struct
{
    rt_thread_t thread;
    uint8_t encode_in[XZ_MIC_FRAME_LEN];
    uint8_t encode_out[XZ_MIC_FRAME_LEN];
    bool inited;
    bool is_rx_enable;
    bool is_tx_enable;
    bool is_exit;
    uint8_t is_websocket;
    rt_event_t event;
    OpusEncoder *encoder;
    OpusDecoder *decoder;
    struct rt_ringbuffer *rb_opus_encode_input;
    xz_decode_queue_t downlink_queue[XZ_DOWNLINK_QUEUE_NUM];
    opus_int16 downlink_decode_out[XZ_SPK_FRAME_LEN / 2];
    rt_slist_t downlink_decode_busy;
    rt_slist_t downlink_decode_idle;
    rt_device_t rt_audio_dev;
    rt_device_t rt_mic_dev;
} xz_audio_t;

extern rt_event_t xiaozhi_button_event;
char *get_mac_address(void);
char *get_client_id(void);
void xz_button_callback(void *arg);
void xz_button_thread_entry(void *param);
void xz_button_init(void);
rt_bool_t ws_send_listen_start(void *ws, char *session_id, enum ListeningMode mode);
void ws_send_listen_stop(void *ws, char *session_id);
void ws_send_hello(void *ws);
void xz_audio_send_using_websocket(uint8_t *data, int len);
void send_iot_states(void);
void send_iot_descriptors(void);
err_t my_wsapp_fn(int code, char *buf, size_t len);
void reconnect_websocket(void);
void xz_ws_audio_init(void);
char *my_json_string(cJSON *json, char *key);
void Message_handle(const uint8_t *data, uint16_t len);
void svr_found_callback(const char *name, const ip_addr_t *ipaddr, void *callback_arg);
int check_internet_access(void);
char *get_xiaozhi_ws(void);
int http_xiaozhi_data_parse_ws(char *json_data);
void xiaozhi_ws_connect(void);
void xiaozhi_entry(void *p);
int ws_xiaozhi_init(void);

void xz_sound_init(void);
int xz_mic_init(void);
void xz_mic_open(xz_audio_t *thiz);
void xz_mic_close(xz_audio_t *thiz);
void xz_mic(int on);
int xz_mic_is_enabled(void);
void xz_speaker_open(xz_audio_t *thiz);
void xz_speaker_close(xz_audio_t *thiz);
void xz_speaker(int on);
void xz_audio_decoder_encoder_open(uint8_t is_websocket);
void xz_audio_decoder_encoder_close(void);
void xz_audio_downlink(uint8_t *data, uint32_t size, uint32_t *aes_value, uint8_t need_aes);

/* Multi-turn conversation control functions */
void xz_enable_multi_turn_conversation(rt_bool_t enable);
rt_bool_t xz_is_multi_turn_conversation_enabled(void);

/* Audio notification functions */
void xz_play_power_on_sound(void);
void xz_play_wake_sound(void);

#ifdef __cplusplus
}
#endif

#endif // XIAOZHI_H
