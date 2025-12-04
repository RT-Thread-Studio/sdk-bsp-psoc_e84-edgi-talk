#include <rtdevice.h>
#include <rtthread.h>
#include "board.h"
#include "lwip/api.h"
#include "lwip/dns.h"
#include "lwip/tcpip.h"
#include "xiaozhi.h"

#define DBG_TAG "xz.audio"
#define DBG_LVL DBG_LOG
#include <rtdbg.h>

/* Global Variables */
static xz_audio_t xz_audio;
static rt_event_t mic_event = RT_NULL;

/* Audio Initialization */
void xz_sound_init(void)
{
    xz_audio_t *thiz = &xz_audio;
    thiz->rt_audio_dev = rt_device_find("sound0");
    struct rt_audio_caps sound_dev_arg;
    sound_dev_arg.main_type = AUDIO_TYPE_OUTPUT;
    sound_dev_arg.sub_type = AUDIO_DSP_PARAM;
    struct rt_audio_configure audio_configure;
    audio_configure.channels = 1;
    audio_configure.samplebits = 16;
    audio_configure.samplerate = 16000;
    sound_dev_arg.udata.config = audio_configure;
    rt_device_control(thiz->rt_audio_dev, AUDIO_CTL_CONFIGURE, &sound_dev_arg);

    rt_device_open(thiz->rt_audio_dev, RT_DEVICE_OFLAG_WRONLY);
}

int xz_mic_init(void)
{
    xz_audio_t *thiz = &xz_audio;
    thiz->rt_mic_dev = rt_device_find("mic0");

    mic_event = rt_event_create("mic_evt", RT_IPC_FLAG_FIFO);
    RT_ASSERT(mic_event != RT_NULL);
    rt_kprintf("xz_mic_init\r\n");

    rt_thread_t tid = rt_thread_create("mic_thread", mic_thread_entry,
                                       RT_NULL, 1024 * 2, 6, 10);
    RT_ASSERT(tid != RT_NULL);
    rt_thread_startup(tid);

    return RT_EOK;
}

/* Microphone Functions */
void xz_mic_open(xz_audio_t *thiz)
{
    rt_device_open(thiz->rt_mic_dev, RT_DEVICE_OFLAG_RDONLY);
    rt_event_send(mic_event, MIC_EVENT_OPEN);
    LOG_D("Microphone started");
    thiz->is_rx_enable = 1;
}

void xz_mic_close(xz_audio_t *thiz)
{
    rt_event_send(mic_event, MIC_EVENT_CLOSE);
    rt_device_close(thiz->rt_mic_dev);
    rt_kprintf("Microphone stopped\n");
    thiz->is_rx_enable = 0;
}

void xz_mic(int on)
{
    xz_audio_t *thiz = &xz_audio;
    if (on)
    {
        xz_mic_open(thiz);
    }
    else
    {
        xz_mic_close(thiz);
    }
}

/* Speaker Functions */
void xz_speaker_open(xz_audio_t *thiz)
{
    thiz->is_tx_enable = 1;
}

void xz_speaker_close(xz_audio_t *thiz)
{
    LOG_D("speaker off");
    LOG_D("\u5f85\u547d\u4e2d...");
    thiz->is_tx_enable = 0;
    rt_slist_t *idle;
    rt_enter_critical();
    while (1)
    {
        idle = rt_slist_first(&thiz->downlink_decode_busy);
        if (idle)
        {
            rt_slist_remove(&thiz->downlink_decode_busy, idle);
            rt_slist_append(&thiz->downlink_decode_idle, idle);
        }
        else
        {
            break;
        }
    }
    rt_exit_critical();
}

void xz_speaker(int on)
{
    xz_audio_t *thiz = &xz_audio;
    if (on)
    {
        xz_speaker_open(thiz);
    }
    else
    {
        xz_speaker_close(thiz);
    }
}

#define CHANGED         0
#define ACTIVITY        1
#define ENDPOINT        2
/* Audio Processing */
void mic_thread_entry(void *param)
{
    rt_uint32_t evt = 0;
    uint8_t buffer[1024] = {0};
    int length = 0, total_length = 0;

    while (1)
    {
        if (rt_event_recv(mic_event, MIC_EVENT_OPEN,
                          RT_EVENT_FLAG_OR | RT_EVENT_FLAG_CLEAR,
                          RT_WAITING_FOREVER, &evt) == RT_EOK)
        {
            while (1)
            {
                rt_event_recv(mic_event, MIC_EVENT_CLOSE,
                              RT_EVENT_FLAG_OR | RT_EVENT_FLAG_CLEAR,
                              RT_WAITING_NO, &evt);
                if (evt & MIC_EVENT_CLOSE)
                {
                    break;
                }
                length = rt_device_read(xz_audio.rt_mic_dev, 0, &buffer, 1024);
                if (length > 0)
                {
#ifdef ExBoard_Voice
                    switch (webrtc_vad_run((short *)buffer, 160))
                    {
                    case ENDPOINT:
                        if (g_state == kDeviceStateListening)
                        {
                            ws_send_listen_stop(&g_xz_ws.clnt, g_xz_ws.session_id);
                            xz_mic(0);
                            g_state = kDeviceStateIdle;
                        }
                        break;
                    default:
                        break;
                    }
#endif
                    total_length += length;
                    rt_ringbuffer_put(xz_audio.rb_opus_encode_input, (uint8_t *)buffer, length);
                }
                if (total_length >= XZ_MIC_FRAME_LEN)
                {
                    rt_event_send(xz_audio.event, XZ_EVENT_MIC_RX);
                    total_length = 0;
                }
            }
        }
    }
}

void xz_opus_thread_entry(void *p)
{
    int err;
    xz_audio_t *thiz = &xz_audio;
    thiz->decoder = opus_decoder_create(24000, 1, &err);
    RT_ASSERT(thiz->decoder);
    thiz->encoder = opus_encoder_create(16000, 2, OPUS_APPLICATION_VOIP, &err);
    RT_ASSERT(thiz->encoder);

    opus_encoder_ctl(thiz->encoder, OPUS_SET_EXPERT_FRAME_DURATION(OPUS_FRAMESIZE_60_MS));
    opus_encoder_ctl(thiz->encoder, OPUS_SET_VBR(1));
    opus_encoder_ctl(thiz->encoder, OPUS_SET_VBR_CONSTRAINT(1));
    opus_encoder_ctl(thiz->encoder, OPUS_SET_BITRATE(16000));
    opus_encoder_ctl(thiz->encoder, OPUS_SET_SIGNAL(OPUS_SIGNAL_VOICE));
    opus_encoder_ctl(thiz->encoder, OPUS_SET_COMPLEXITY(0));
    opus_encoder_ctl(thiz->encoder, OPUS_SET_LSB_DEPTH(16));
    opus_encoder_ctl(thiz->encoder, OPUS_SET_DTX(0));
    opus_encoder_ctl(thiz->encoder, OPUS_SET_INBAND_FEC(0));
    opus_encoder_ctl(thiz->encoder, OPUS_SET_PACKET_LOSS_PERC(0));
    opus_encoder_ctl(thiz->encoder, OPUS_SET_PREDICTION_DISABLED(0));
    opus_encoder_ctl(thiz->encoder, OPUS_SET_MAX_BANDWIDTH(OPUS_BANDWIDTH_WIDEBAND));
    opus_encoder_ctl(thiz->encoder, OPUS_SET_BANDWIDTH(OPUS_AUTO));

    while (!thiz->is_exit)
    {
        rt_uint32_t evt = 0;
        rt_event_recv(thiz->event, XZ_EVENT_ALL,
                      RT_EVENT_FLAG_OR | RT_EVENT_FLAG_CLEAR,
                      RT_WAITING_FOREVER, &evt);

        if ((evt & XZ_EVENT_MIC_RX) && thiz->is_rx_enable)
        {
            rt_ringbuffer_get(thiz->rb_opus_encode_input, (uint8_t *)&thiz->encode_in[0], XZ_MIC_FRAME_LEN);
            opus_int32 len = opus_encode(thiz->encoder, (const opus_int16 *)thiz->encode_in,
                                         XZ_MIC_FRAME_LEN / 2, thiz->encode_out, XZ_MIC_FRAME_LEN);
            if (len < 0 || len > XZ_MIC_FRAME_LEN)
            {
                LOG_E("opus_encode len=%d", len);
                RT_ASSERT(0);
            }
            xz_audio_send_using_websocket(thiz->encode_out, len);
        }

        if ((evt & XZ_EVENT_DOWNLINK) && thiz->is_tx_enable)
        {
            rt_slist_t *decode;
            rt_enter_critical();
            decode = rt_slist_first(&thiz->downlink_decode_busy);
            rt_exit_critical();
            xz_decode_queue_t *queue = rt_container_of(decode, xz_decode_queue_t, node);
            opus_int32 res = opus_decode(thiz->decoder, (const uint8_t *)queue->data,
                                         queue->data_len, thiz->downlink_decode_out,
                                         XZ_SPK_FRAME_LEN / 2, 0);
            if (res < 0)
            {
                LOG_E("Opus decode error: %d", res);
            }
            rt_device_write(thiz->rt_audio_dev, 0, (char *)thiz->downlink_decode_out, XZ_SPK_FRAME_LEN);

            uint8_t need_decode_again = 0;
            rt_enter_critical();
            rt_slist_remove(&thiz->downlink_decode_busy, decode);
            rt_slist_append(&thiz->downlink_decode_idle, decode);
            if (rt_slist_first(&thiz->downlink_decode_busy))
            {
                need_decode_again = true;
            }
            rt_exit_critical();

            if (need_decode_again) rt_event_send(thiz->event, XZ_EVENT_DOWNLINK);
        }
    }

    if (thiz->encoder) opus_encoder_destroy(thiz->encoder);
    if (thiz->decoder) opus_decoder_destroy(thiz->decoder);

    LOG_D("---xz thread exit---");
}

void xz_audio_decoder_encoder_open(uint8_t is_websocket)
{
    xz_audio_t *thiz = &xz_audio;
    if (!thiz->inited)
    {
        thiz->is_websocket = is_websocket;
        thiz->event = rt_event_create("xzaudio", RT_IPC_FLAG_FIFO);
        RT_ASSERT(thiz->event);
        rt_slist_init(&thiz->downlink_decode_idle);
        rt_slist_init(&thiz->downlink_decode_busy);
        for (int i = 0; i < XZ_DOWNLINK_QUEUE_NUM; i++)
        {
            thiz->downlink_queue[i].size = 256;
            thiz->downlink_queue[i].data =
                opus_heap_malloc(thiz->downlink_queue[i].size);
            RT_ASSERT(thiz->downlink_queue[i].data);

            // 将新分配的队列节点添加到空闲队列中
            rt_slist_append(&thiz->downlink_decode_idle,
                            &thiz->downlink_queue[i].node);
        }

        thiz->rb_opus_encode_input = rt_ringbuffer_create(XZ_MIC_FRAME_LEN * 2);
        RT_ASSERT(thiz->rb_opus_encode_input);

        thiz->thread = rt_thread_create("opus", xz_opus_thread_entry, NULL,
                                        1024 * 35, 8, 10);
        RT_ASSERT(thiz->thread != RT_NULL);
        rt_thread_startup(thiz->thread);

        thiz->inited = 1;
    }
}

void xz_audio_decoder_encoder_close(void)
{
    xz_audio_t *thiz = &xz_audio;
    thiz->is_exit = 1;
    while (rt_thread_find("opus"))
    {
        LOG_D("wait thread opus exit");
        rt_thread_mdelay(100);
    }

    rt_ringbuffer_destroy(thiz->rb_opus_encode_input);
    rt_event_delete(thiz->event);

    for (int i = 0; i < XZ_DOWNLINK_QUEUE_NUM; i++)
    {
        if (thiz->downlink_queue[i].data)
        {
            opus_heap_free(thiz->downlink_queue[i].data);
            thiz->downlink_queue[i].data = NULL;
        }
    }
}

/* Downlink Handling */
void xz_audio_downlink(uint8_t *data, uint32_t size, uint32_t *aes_value, uint8_t need_aes)
{
    xz_audio_t *thiz = &xz_audio;
    rt_slist_t *idle;
    int try_times = 0;

    if (!thiz->inited) return;

wait_speaker:
    rt_enter_critical();
    idle = rt_slist_first(&thiz->downlink_decode_idle);
    if (!idle)
    {
        rt_exit_critical();
        LOG_I("speaker busy");
        try_times++;
        if (try_times < 20)
        {
            rt_thread_mdelay(5);
            goto wait_speaker;
        }
        return;
    }
    rt_slist_remove(&thiz->downlink_decode_idle, idle);
    rt_exit_critical();

    xz_decode_queue_t *queue = rt_container_of(idle, xz_decode_queue_t, node);
    if (queue->size < size + 16)
    {
        if (queue->data) opus_heap_free(queue->data);
        queue->size = size + 16;
        queue->data = opus_heap_malloc(queue->size);
        if (queue->data == RT_NULL)
        {
            LOG_E("malloc failed");
            rt_enter_critical();
            rt_slist_append(&thiz->downlink_decode_idle, idle);
            rt_exit_critical();
            return;
        }
    }
    queue->data_len = size;
    rt_memcpy(queue->data, data, size);
    
    rt_enter_critical();
    rt_slist_append(&thiz->downlink_decode_busy, idle);
    rt_exit_critical();
    rt_event_send(thiz->event, XZ_EVENT_DOWNLINK);
}
