#include "deepcraft_mic.h"

#include <stdint.h>
#include <string.h>

#include <rtthread.h>

#include "protocol/pb_encode.h"
#include "drivers/pdm_pcm.h"

#if defined(CYBSP_PDM_ENABLED)

#define MIC_OPTION_KEY_GAIN      10
#define MIC_OPTION_KEY_STEREO    20
#define MIC_OPTION_KEY_FREQUENCY 30

#define LEFT_CH_INDEX  2u
#define RIGHT_CH_INDEX 3u
#define LEFT_CH_CONFIG  channel_2_config
#define RIGHT_CH_CONFIG channel_3_config

typedef struct
{
    pdm_pcm mic;
    PDM_PCM_CONFIG_t pdm_config;
    int stream_id;
    rt_bool_t running;
} deepcraft_mic_ctx_t;

static deepcraft_mic_ctx_t g_mic;

static void deepcraft_mic_set_pdm_config(PDM_PCM_CONFIG_t *config, bool stereo)
{
    if (stereo)
    {
        config->channel_config[0] = RIGHT_CH_CONFIG;
        config->channel_config[1] = LEFT_CH_CONFIG;
        config->channel_index_list[0] = RIGHT_CH_INDEX;
        config->channel_index_list[1] = LEFT_CH_INDEX;
        config->mode = MODE_STEREO;
        config->pdm_irq_cfg.intrSrc = CYBSP_PDM_CHANNEL_3_IRQ;
    }
    else
    {
        config->channel_config[0] = RIGHT_CH_CONFIG;
        config->channel_index_list[0] = RIGHT_CH_INDEX;
        config->mode = MODE_MONO;
        config->pdm_irq_cfg.intrSrc = CYBSP_PDM_CHANNEL_3_IRQ;
    }
}

static bool deepcraft_mic_write_payload(
    protocol_t *protocol,
    int device_id,
    int stream_id,
    int frame_count,
    int total_bytes,
    pb_ostream_t *ostream,
    void *arg)
{
    deepcraft_mic_ctx_t *mic = (deepcraft_mic_ctx_t *)arg;
    int16_t *buf;

    (void)protocol;
    (void)device_id;
    (void)stream_id;
    (void)frame_count;

    if (mic == RT_NULL || mic->mic == RT_NULL)
    {
        return false;
    }

    buf = pdm_pcm_get_full_buffer(mic->mic);
    if (buf == RT_NULL)
    {
        return false;
    }

    if (!pb_write(ostream, (const pb_byte_t *)buf, (size_t)total_bytes))
    {
        return false;
    }

    pdm_pcm_clear_data_ready_flag(mic->mic);
    return true;
}

static bool deepcraft_mic_configure_streams(protocol_t *protocol, int device, void *arg)
{
    deepcraft_mic_ctx_t *mic = (deepcraft_mic_ctx_t *)arg;
    int status;
    int frequency_index;
    bool stereo;
    int stream;
    uint32_t frames_per_chunk;

    if (mic == RT_NULL || mic->mic == RT_NULL)
    {
        protocol_set_device_status(protocol, device, protocol_DeviceStatus_DEVICE_STATUS_ERROR, "Mic handle not available.");
        return true;
    }

    status = protocol_get_option_oneof(protocol, device, MIC_OPTION_KEY_FREQUENCY, &frequency_index);
    if (status != PROTOCOL_STATUS_SUCCESS)
    {
        protocol_set_device_status(protocol, device, protocol_DeviceStatus_DEVICE_STATUS_ERROR, "Failed to get frequency option.");
        return true;
    }

    status = protocol_get_option_bool(protocol, device, MIC_OPTION_KEY_STEREO, &stereo);
    if (status != PROTOCOL_STATUS_SUCCESS)
    {
        protocol_set_device_status(protocol, device, protocol_DeviceStatus_DEVICE_STATUS_ERROR, "Failed to get stereo option.");
        return true;
    }

    deepcraft_mic_set_pdm_config(&mic->pdm_config, stereo);
    (void)pdm_pcm_update_config(mic->mic, &mic->pdm_config);

    if (protocol_clear_streams(protocol, device) != PROTOCOL_STATUS_SUCCESS)
    {
        protocol_set_device_status(protocol, device, protocol_DeviceStatus_DEVICE_STATUS_ERROR, "Failed to clear streams.");
        return true;
    }

    frames_per_chunk = pdm_pcm_get_frame_count(mic->mic);
    stream = protocol_add_stream(
        protocol,
        device,
        "Audio",
        protocol_StreamDirection_STREAM_DIRECTION_OUTPUT,
        protocol_DataType_DATA_TYPE_S16,
        (float)pdm_pcm_get_frequency_from_frequency_index((SAMPLE_RATE)frequency_index),
        (int)frames_per_chunk,
        "dBFS");

    if (stream < 0)
    {
        protocol_set_device_status(protocol, device, protocol_DeviceStatus_DEVICE_STATUS_ERROR, "Failed to add stream.");
        return true;
    }

    mic->stream_id = stream;

    if (stereo)
    {
        protocol_add_stream_rank(protocol, device, stream, "Channel", 2, (const char *[]) { "Left", "Right" });
    }
    else
    {
        protocol_add_stream_rank(protocol, device, stream, "Channel", 1, (const char *[]) { "Mono" });
    }

    protocol_set_device_status(protocol, device, protocol_DeviceStatus_DEVICE_STATUS_READY, "Device is ready.");
    return true;
}

static void deepcraft_mic_start(protocol_t *protocol, int device, pb_ostream_t *ostream, void *arg)
{
    deepcraft_mic_ctx_t *mic = (deepcraft_mic_ctx_t *)arg;
    int gain_index;
    int sample_rate_index;
    bool stereo;

    (void)ostream;

    if (mic == RT_NULL || mic->mic == RT_NULL)
    {
        protocol_set_device_status(protocol, device, protocol_DeviceStatus_DEVICE_STATUS_ERROR, "Mic handle not available.");
        return;
    }

    if (mic->stream_id < 0)
    {
        protocol_set_device_status(protocol, device, protocol_DeviceStatus_DEVICE_STATUS_ERROR, "Invalid mic stream configuration.");
        return;
    }

    if (protocol_get_option_oneof(protocol, device, MIC_OPTION_KEY_FREQUENCY, &sample_rate_index) != PROTOCOL_STATUS_SUCCESS ||
        protocol_get_option_bool(protocol, device, MIC_OPTION_KEY_STEREO, &stereo) != PROTOCOL_STATUS_SUCCESS ||
        protocol_get_option_oneof(protocol, device, MIC_OPTION_KEY_GAIN, &gain_index) != PROTOCOL_STATUS_SUCCESS)
    {
        protocol_set_device_status(protocol, device, protocol_DeviceStatus_DEVICE_STATUS_ERROR, "Failed to load mic options.");
        return;
    }

    deepcraft_mic_set_pdm_config(&mic->pdm_config, stereo);
    if (pdm_pcm_update_config(mic->mic, &mic->pdm_config) != PDM_PCM_STATUS_SUCCESS)
    {
        protocol_set_device_status(protocol, device, protocol_DeviceStatus_DEVICE_STATUS_ERROR, "Failed to update PDM config.");
        return;
    }

    if (pdm_pcm_init_hw((SAMPLE_RATE)sample_rate_index) != PDM_PCM_STATUS_SUCCESS)
    {
        protocol_set_device_status(protocol, device, protocol_DeviceStatus_DEVICE_STATUS_ERROR, "Failed to configure PDM clock.");
        return;
    }

    (void)pdm_pcm_set_gain(mic->mic, mic->pdm_config.channel_index_list[0], (cy_en_pdm_pcm_gain_sel_t)gain_index);
    if (stereo)
    {
        (void)pdm_pcm_set_gain(mic->mic, mic->pdm_config.channel_index_list[1], (cy_en_pdm_pcm_gain_sel_t)gain_index);
    }

    if (pdm_pcm_start(mic->mic) != PDM_PCM_STATUS_SUCCESS)
    {
        protocol_set_device_status(protocol, device, protocol_DeviceStatus_DEVICE_STATUS_ERROR, "Failed to start PDM capture.");
        return;
    }

    mic->running = RT_TRUE;
    protocol_set_device_status(protocol, device, protocol_DeviceStatus_DEVICE_STATUS_ACTIVE, "Device is streaming.");
}

static void deepcraft_mic_stop(protocol_t *protocol, int device, pb_ostream_t *ostream, void *arg)
{
    deepcraft_mic_ctx_t *mic = (deepcraft_mic_ctx_t *)arg;

    (void)ostream;

    if (mic == RT_NULL || mic->mic == RT_NULL)
    {
        return;
    }

    if (mic->running)
    {
        (void)pdm_pcm_stop(mic->mic);
        mic->running = RT_FALSE;
    }

    protocol_set_device_status(protocol, device, protocol_DeviceStatus_DEVICE_STATUS_READY, "Device stopped.");
}

static void deepcraft_mic_poll(protocol_t *protocol, int device, pb_ostream_t *ostream, void *arg)
{
    deepcraft_mic_ctx_t *mic = (deepcraft_mic_ctx_t *)arg;
    uint32_t frames_per_chunk;

    if (mic == RT_NULL || mic->mic == RT_NULL || !mic->running)
    {
        return;
    }

    if (!pdm_pcm_data_ready(mic->mic))
    {
        return;
    }

    pdm_pcm_discard_samples(mic->mic);
    frames_per_chunk = pdm_pcm_get_frame_count(mic->mic);

    protocol_send_data_chunk(
        protocol,
        device,
        mic->stream_id,
        (int)frames_per_chunk,
        0,
        ostream,
        deepcraft_mic_write_payload);
}

bool deepcraft_mic_register(protocol_t *protocol)
{
    device_manager_t manager;
    int device;

    if (protocol == RT_NULL)
    {
        return false;
    }

    memset(&g_mic, 0, sizeof(g_mic));
    g_mic.stream_id = -1;

    deepcraft_mic_set_pdm_config(&g_mic.pdm_config, false);
    g_mic.mic = pdm_pcm_create(&g_mic.pdm_config);
    if (g_mic.mic == RT_NULL)
    {
        rt_kprintf("deepcraft: failed to create pdm mic handle.\r\n");
        return false;
    }

    manager.arg = &g_mic;
    manager.busy = RT_NULL;
    manager.configure_streams = deepcraft_mic_configure_streams;
    manager.start = deepcraft_mic_start;
    manager.stop = deepcraft_mic_stop;
    manager.poll = deepcraft_mic_poll;
    manager.data_received = RT_NULL;

    device = protocol_add_device(
        protocol,
        protocol_DeviceType_DEVICE_TYPE_SENSOR,
        "Microphone",
        "PDM/PCM Microphone",
        manager);

    if (device < 0)
    {
        return false;
    }

    if (protocol_add_option_oneof(
            protocol,
            device,
            MIC_OPTION_KEY_GAIN,
            "Gain",
            "Microphone gain",
            CY_PDM_PCM_SEL_GAIN_5DB,
            pdm_pcm_get_string_list_of_gain_options(),
            pdm_pcm_get_gain_option_count()) != PROTOCOL_STATUS_SUCCESS)
    {
        return false;
    }

    if (protocol_add_option_bool(
            protocol,
            device,
            MIC_OPTION_KEY_STEREO,
            "Stereo",
            "Stereo or Mono",
            false) != PROTOCOL_STATUS_SUCCESS)
    {
        return false;
    }

    if (protocol_add_option_oneof(
            protocol,
            device,
            MIC_OPTION_KEY_FREQUENCY,
            "Frequency",
            "Sample frequency (Hz)",
            SAMPLE_RATE_16000,
            pdm_pcm_get_string_list_of_sample_rates(),
            pdm_pcm_get_sample_rate_option_count()) != PROTOCOL_STATUS_SUCCESS)
    {
        return false;
    }

    return deepcraft_mic_configure_streams(protocol, device, &g_mic);
}

#else

bool deepcraft_mic_register(protocol_t *protocol)
{
    (void)protocol;
    rt_kprintf("deepcraft: PDM peripheral not enabled, microphone unavailable.\r\n");
    return false;
}

#endif
