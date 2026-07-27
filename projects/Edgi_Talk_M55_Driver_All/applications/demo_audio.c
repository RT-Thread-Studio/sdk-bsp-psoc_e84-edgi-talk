/*
 * demo_audio.c - ES8388 speaker + PDM microphone loopback demo
 *
 * MSH command:
 *   demo_audio start [speaker_volume] [mic_gain]
 *   demo_audio stop
 *   demo_audio status
 */

#include <rtthread.h>
#include <rtdevice.h>
#include <stdlib.h>
#include <string.h>

#ifdef RT_USING_AUDIO

#define DEMO_AUDIO_SOUND_DEVICE      "sound0"
#define DEMO_AUDIO_MIC_DEVICE        "mic0"
#define DEMO_AUDIO_THREAD_STACK      4096
#define DEMO_AUDIO_THREAD_PRIORITY   18
#define DEMO_AUDIO_THREAD_TICK       10
#define DEMO_AUDIO_CHUNK_BYTES       1024
#define DEMO_AUDIO_SAMPLE_RATE       16000
#define DEMO_AUDIO_CHANNELS          1
#define DEMO_AUDIO_SAMPLE_BITS       16
#define DEMO_AUDIO_SOUND_VOLUME      30
#define DEMO_AUDIO_MIC_GAIN          15

static rt_thread_t g_audio_thread = RT_NULL;
static volatile rt_bool_t g_audio_running = RT_FALSE;
static volatile rt_bool_t g_audio_stop_req = RT_FALSE;
static rt_uint8_t g_audio_sound_volume = DEMO_AUDIO_SOUND_VOLUME;
static rt_uint8_t g_audio_mic_gain = DEMO_AUDIO_MIC_GAIN;

static rt_err_t demo_audio_set_volume(rt_device_t dev, rt_uint8_t value)
{
    struct rt_audio_caps caps;

    rt_memset(&caps, 0, sizeof(caps));
    caps.main_type = AUDIO_TYPE_MIXER;
    caps.sub_type = AUDIO_MIXER_VOLUME;
    caps.udata.value = value;

    return rt_device_control(dev, AUDIO_CTL_CONFIGURE, &caps);
}

static rt_err_t demo_audio_set_pcm(rt_device_t dev, int main_type)
{
    struct rt_audio_caps caps;

    rt_memset(&caps, 0, sizeof(caps));
    caps.main_type = main_type;
    caps.sub_type = AUDIO_DSP_PARAM;
    caps.udata.config.samplerate = DEMO_AUDIO_SAMPLE_RATE;
    caps.udata.config.channels = DEMO_AUDIO_CHANNELS;
    caps.udata.config.samplebits = DEMO_AUDIO_SAMPLE_BITS;

    return rt_device_control(dev, AUDIO_CTL_CONFIGURE, &caps);
}

static void demo_audio_loopback_entry(void *parameter)
{
    rt_device_t sound_dev;
    rt_device_t mic_dev;
    rt_uint8_t *buffer;

    RT_UNUSED(parameter);

    sound_dev = rt_device_find(DEMO_AUDIO_SOUND_DEVICE);
    mic_dev = rt_device_find(DEMO_AUDIO_MIC_DEVICE);
    if ((sound_dev == RT_NULL) || (mic_dev == RT_NULL))
    {
        rt_kprintf("Audio device not found: %s=%p, %s=%p\n",
                   DEMO_AUDIO_SOUND_DEVICE,
                   sound_dev,
                   DEMO_AUDIO_MIC_DEVICE,
                   mic_dev);
        goto out;
    }

    buffer = (rt_uint8_t *)rt_malloc(DEMO_AUDIO_CHUNK_BYTES);
    if (buffer == RT_NULL)
    {
        rt_kprintf("Audio malloc %u bytes failed\n", DEMO_AUDIO_CHUNK_BYTES);
        goto out;
    }

    if (rt_device_open(sound_dev, RT_DEVICE_OFLAG_WRONLY) != RT_EOK)
    {
        rt_kprintf("Open %s failed\n", DEMO_AUDIO_SOUND_DEVICE);
        rt_free(buffer);
        goto out;
    }

    demo_audio_set_pcm(sound_dev, AUDIO_TYPE_OUTPUT);
    demo_audio_set_volume(sound_dev, g_audio_sound_volume);

    if (rt_device_open(mic_dev, RT_DEVICE_OFLAG_RDONLY) != RT_EOK)
    {
        rt_kprintf("Open %s failed\n", DEMO_AUDIO_MIC_DEVICE);
        rt_device_close(sound_dev);
        rt_free(buffer);
        goto out;
    }

    demo_audio_set_pcm(mic_dev, AUDIO_TYPE_INPUT);
    demo_audio_set_volume(mic_dev, g_audio_mic_gain);
    rt_kprintf("Audio loopback started: %u Hz, %u ch, %u bit, speaker=%u, mic_gain=%u\n",
               DEMO_AUDIO_SAMPLE_RATE,
               DEMO_AUDIO_CHANNELS,
               DEMO_AUDIO_SAMPLE_BITS,
               g_audio_sound_volume,
               g_audio_mic_gain);

    while (g_audio_stop_req == RT_FALSE)
    {
        rt_size_t length;

        length = rt_device_read(mic_dev, 0, buffer, DEMO_AUDIO_CHUNK_BYTES);
        if (length > 0)
        {
            rt_device_write(sound_dev, 0, buffer, length);
        }
        else
        {
            rt_thread_mdelay(1);
        }
    }

    rt_device_close(mic_dev);
    rt_device_close(sound_dev);
    rt_free(buffer);
    rt_kprintf("Audio loopback stopped\n");

out:
    g_audio_running = RT_FALSE;
    g_audio_stop_req = RT_FALSE;
    g_audio_thread = RT_NULL;
}

static int demo_audio(int argc, char **argv)
{
    const char *cmd = "start";

    if (argc > 1)
    {
        cmd = argv[1];
    }

    if (rt_strcmp(cmd, "start") == 0)
    {
        if (g_audio_running == RT_TRUE)
        {
            rt_kprintf("Audio loopback is already running\n");
            return RT_EOK;
        }

        if (argc > 2)
        {
            g_audio_sound_volume = (rt_uint8_t)strtoul(argv[2], RT_NULL, 0);
        }
        else
        {
            g_audio_sound_volume = DEMO_AUDIO_SOUND_VOLUME;
        }

        if (argc > 3)
        {
            g_audio_mic_gain = (rt_uint8_t)strtoul(argv[3], RT_NULL, 0);
        }
        else
        {
            g_audio_mic_gain = DEMO_AUDIO_MIC_GAIN;
        }

        if (argc > 4)
        {
            rt_kprintf("Usage: demo_audio start [speaker_volume] [mic_gain]\n");
            return -RT_ERROR;
        }

        g_audio_stop_req = RT_FALSE;
        g_audio_running = RT_TRUE;
        g_audio_thread = rt_thread_create("demo_audio",
                                          demo_audio_loopback_entry,
                                          RT_NULL,
                                          DEMO_AUDIO_THREAD_STACK,
                                          DEMO_AUDIO_THREAD_PRIORITY,
                                          DEMO_AUDIO_THREAD_TICK);
        if (g_audio_thread == RT_NULL)
        {
            g_audio_running = RT_FALSE;
            rt_kprintf("Create demo_audio thread failed\n");
            return -RT_ERROR;
        }

        rt_thread_startup(g_audio_thread);
        return RT_EOK;
    }

    if (rt_strcmp(cmd, "stop") == 0)
    {
        if (g_audio_running == RT_FALSE)
        {
            rt_kprintf("Audio loopback is not running\n");
            return RT_EOK;
        }

        g_audio_stop_req = RT_TRUE;
        rt_kprintf("Stopping audio loopback...\n");
        return RT_EOK;
    }

    if (rt_strcmp(cmd, "status") == 0)
    {
        rt_kprintf("Audio loopback: %s, speaker=%u, mic_gain=%u\n",
                   (g_audio_running == RT_TRUE) ? "running" : "stopped",
                   g_audio_sound_volume,
                   g_audio_mic_gain);
        return RT_EOK;
    }

    rt_kprintf("Usage:\n");
    rt_kprintf("  demo_audio start [speaker_volume] [mic_gain]\n");
    rt_kprintf("  demo_audio stop\n");
    rt_kprintf("  demo_audio status\n");
    return -RT_ERROR;
}
MSH_CMD_EXPORT(demo_audio, ES8388 speaker and PDM microphone loopback);

#else

static int demo_audio(int argc, char **argv)
{
    RT_UNUSED(argc);
    RT_UNUSED(argv);
    rt_kprintf("Audio demo is disabled. Enable RT_USING_AUDIO + BSP_USING_AUDIO.\n");
    return -RT_ERROR;
}
MSH_CMD_EXPORT(demo_audio, ES8388 speaker and PDM microphone loopback);

#endif
