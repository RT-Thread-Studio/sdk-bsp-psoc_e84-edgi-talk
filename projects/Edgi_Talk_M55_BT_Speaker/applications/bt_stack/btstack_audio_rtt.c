/*
 * BTstack audio sink backed by the RT-Thread "sound0" audio device
 * (I2S/TDM + ES8388).
 *
 * The A2DP sink demo expects the playback callback to run on the BTstack
 * thread.  A BTstack timer therefore fills one of four PCM buffers every
 * ~21 ms; a separate writer thread hands the buffer to sound0, so a blocking
 * audio write never stalls HCI processing.
 */

#include <rtthread.h>
#include <rtdevice.h>
#include <string.h>

#include "btstack_audio.h"
#include "btstack_run_loop.h"

#include "btstack_audio_rtt.h"

#define BT_AUDIO_DEVICE_NAME   "sound0"
#define BT_AUDIO_SAMPLE_RATE   48000
#define BT_AUDIO_CHANNELS      2
#define BT_AUDIO_FRAMES        1024
#define BT_AUDIO_BYTES         (BT_AUDIO_FRAMES * BT_AUDIO_CHANNELS * sizeof(int16_t))
#define BT_AUDIO_TIMER_MS      ((BT_AUDIO_FRAMES * 1000) / BT_AUDIO_SAMPLE_RATE)

#define BT_AUDIO_THREAD_STACK  4096
/*
 * Must be above the BTstack thread (priority 8) and below the I2S task
 * (priority 6).  Otherwise the continuously-running BT stack thread can
 * starve the writer, the audio timer drops periods and the PCM FIFO
 * overflows.
 */
#define BT_AUDIO_THREAD_PRIO   7
#define BT_AUDIO_THREAD_TICK   10

/*
 * Four playback buffers form a single-producer/single-consumer ring.  The
 * audio timer can run up to three buffers (about 64 ms) ahead of the writer,
 * so a late rt_device_write() no longer causes the timer to drop a decode
 * period (which used to make the PCM FIFO overflow).
 */
#define BT_AUDIO_BUFFER_COUNT  4

static struct rt_device *audio_dev;
static void (*audio_playback)(int16_t *buffer, uint16_t num_samples);

static int16_t audio_buffers[BT_AUDIO_BUFFER_COUNT][BT_AUDIO_FRAMES * BT_AUDIO_CHANNELS];
static volatile int audio_write_index;
static volatile int audio_read_index;

static rt_sem_t audio_ready_sem;
static rt_mutex_t audio_buffer_lock;
static rt_thread_t audio_writer_thread;
static btstack_timer_source_t audio_timer;
static uint32_t audio_timer_remainder;
static uint32_t audio_timer_deadline;

static volatile int audio_stream_started;
static volatile int audio_writer_running;
static int audio_device_opened;

static void audio_writer_entry(void *parameter)
{
    RT_UNUSED(parameter);

    while (audio_writer_running)
    {
        int index = -1;

        rt_sem_take(audio_ready_sem, RT_WAITING_FOREVER);
        if (!audio_writer_running)
        {
            break;
        }

        rt_mutex_take(audio_buffer_lock, RT_WAITING_FOREVER);
        if (audio_read_index != audio_write_index)
        {
            index = audio_read_index;
            audio_read_index = (audio_read_index + 1) % BT_AUDIO_BUFFER_COUNT;
        }
        rt_mutex_release(audio_buffer_lock);

        if (index >= 0 && audio_dev != RT_NULL)
        {
            rt_device_write(audio_dev, 0, audio_buffers[index], BT_AUDIO_BYTES);
        }
    }
}

static void audio_timer_handler(btstack_timer_source_t *timer)
{
    if (audio_stream_started && audio_playback != RT_NULL)
    {
        int index = -1;
        int next = (audio_write_index + 1) % BT_AUDIO_BUFFER_COUNT;

        rt_mutex_take(audio_buffer_lock, RT_WAITING_FOREVER);
        if (next != audio_read_index)
        {
            index = audio_write_index;
        }
        rt_mutex_release(audio_buffer_lock);

        /* Drop this period only if the writer is three buffers (>64 ms) late. */
        if (index >= 0)
        {
            audio_playback(audio_buffers[index], BT_AUDIO_FRAMES);

            rt_mutex_take(audio_buffer_lock, RT_WAITING_FOREVER);
            audio_write_index = next;
            rt_mutex_release(audio_buffer_lock);
            rt_sem_release(audio_ready_sem);
        }
    }

    if (audio_stream_started)
    {
        /*
         * 1024 frames at 48 kHz is 21.333 ms.  Use a fractional accumulator
         * so the timer averages 21.333 ms instead of drifting at 21 ms.
         */
        uint32_t period_ms;

        audio_timer_remainder += BT_AUDIO_FRAMES * 1000u;
        period_ms = audio_timer_remainder / BT_AUDIO_SAMPLE_RATE;
        audio_timer_remainder %= BT_AUDIO_SAMPLE_RATE;

        /*
         * Schedule against an absolute deadline.  btstack_run_loop_set_timer()
         * computes now + period, so every millisecond the run loop wakes late
         * would otherwise be added to the next period and the average playback
         * rate slowly drops below 48 kHz.
         */
        audio_timer_deadline += period_ms;
        if ((int32_t)(audio_timer_deadline - btstack_run_loop_get_time_ms()) < 0)
        {
            /* We fell more than one period behind; resynchronise. */
            audio_timer_deadline = btstack_run_loop_get_time_ms() + period_ms;
        }
        timer->timeout = audio_timer_deadline;
        btstack_run_loop_add_timer(timer);
    }
}

static void audio_set_volume_value(uint8_t volume)
{
    struct rt_audio_caps caps;

    if (audio_dev == RT_NULL)
    {
        return;
    }

    rt_memset(&caps, 0, sizeof(caps));
    caps.main_type = AUDIO_TYPE_MIXER;
    caps.sub_type = AUDIO_MIXER_VOLUME;
    caps.udata.value = (volume * 100u) / 127u;

    rt_device_control(audio_dev, AUDIO_CTL_CONFIGURE, &caps);
}

static int audio_sink_init(uint8_t channels, uint32_t samplerate,
                           void (*playback)(int16_t *buffer, uint16_t num_samples))
{
    struct rt_audio_caps caps;

    RT_UNUSED(channels);
    RT_UNUSED(samplerate);

    audio_playback = playback;

    if (audio_dev == RT_NULL)
    {
        audio_dev = rt_device_find(BT_AUDIO_DEVICE_NAME);
        if (audio_dev == RT_NULL)
        {
            rt_kprintf("[bt] cannot find %s\n", BT_AUDIO_DEVICE_NAME);
            return 0;
        }
    }

    if (!audio_device_opened)
    {
        if (rt_device_open(audio_dev, RT_DEVICE_OFLAG_WRONLY) != RT_EOK)
        {
            rt_kprintf("[bt] open %s failed\n", BT_AUDIO_DEVICE_NAME);
            return 0;
        }
        audio_device_opened = 1;
    }

    /*
     * sound0's divider table is correct for the on-board TDM only when the
     * channels field is 1.  The actual data path is still interleaved stereo:
     * drv_i2s.c stereo passthrough consumes both L and R samples.
     */
    rt_memset(&caps, 0, sizeof(caps));
    caps.main_type = AUDIO_TYPE_OUTPUT;
    caps.sub_type = AUDIO_DSP_PARAM;
    caps.udata.config.samplerate = BT_AUDIO_SAMPLE_RATE;
    caps.udata.config.channels = 1;
    caps.udata.config.samplebits = 16;
    rt_device_control(audio_dev, AUDIO_CTL_CONFIGURE, &caps);

    audio_set_volume_value(100);

    if (audio_ready_sem == RT_NULL)
    {
        audio_ready_sem = rt_sem_create("bt_audio", 0, RT_IPC_FLAG_FIFO);
    }
    if (audio_buffer_lock == RT_NULL)
    {
        audio_buffer_lock = rt_mutex_create("bt_abuf", RT_IPC_FLAG_FIFO);
    }
    if (audio_ready_sem == RT_NULL || audio_buffer_lock == RT_NULL)
    {
        rt_kprintf("[bt] cannot create audio IPC objects\n");
        return 0;
    }

    if (audio_writer_thread == RT_NULL)
    {
        audio_writer_running = 1;
        audio_writer_thread = rt_thread_create("bt_writer",
                                               audio_writer_entry,
                                               RT_NULL,
                                               BT_AUDIO_THREAD_STACK,
                                               BT_AUDIO_THREAD_PRIO,
                                               BT_AUDIO_THREAD_TICK);
        if (audio_writer_thread != RT_NULL)
        {
            rt_thread_startup(audio_writer_thread);
        }
        else
        {
            rt_kprintf("[bt] cannot create audio writer thread\n");
            return 0;
        }
    }

    audio_write_index = 0;
    audio_read_index = 0;
    audio_timer_remainder = 0;
    audio_stream_started = 0;

    if (audio_ready_sem != RT_NULL)
    {
        while (rt_sem_take(audio_ready_sem, 0) == RT_EOK)
        {
        }
    }

    return 1;
}

static void audio_sink_set_volume(uint8_t volume)
{
    audio_set_volume_value(volume);
}

static void audio_sink_start_stream(void)
{
    if (audio_stream_started)
    {
        return;
    }

    /* Discard any stale queued buffers from a previous stream. */
    if (audio_buffer_lock != RT_NULL)
    {
        rt_mutex_take(audio_buffer_lock, RT_WAITING_FOREVER);
        audio_write_index = 0;
        audio_read_index = 0;
        rt_mutex_release(audio_buffer_lock);
    }
    if (audio_ready_sem != RT_NULL)
    {
        while (rt_sem_take(audio_ready_sem, 0) == RT_EOK)
        {
        }
    }

    audio_stream_started = 1;
    btstack_run_loop_set_timer_handler(&audio_timer, &audio_timer_handler);
    btstack_run_loop_set_timer(&audio_timer, BT_AUDIO_TIMER_MS);
    audio_timer_deadline = audio_timer.timeout;
    btstack_run_loop_add_timer(&audio_timer);
}

static void audio_sink_stop_stream(void)
{
    audio_stream_started = 0;
    btstack_run_loop_remove_timer(&audio_timer);
}

static void audio_sink_close(void)
{
    audio_sink_stop_stream();

    /*
     * Keep sound0 open for the lifetime of the sink.  Closing it while the
     * writer thread is inside a blocking rt_device_write() is not safe, and
     * the next A2DP stream can simply reuse the open device.
     */
    if (audio_buffer_lock != RT_NULL)
    {
        rt_mutex_take(audio_buffer_lock, RT_WAITING_FOREVER);
        audio_write_index = 0;
        audio_read_index = 0;
        rt_mutex_release(audio_buffer_lock);
    }
}

static const btstack_audio_sink_t btstack_audio_rtt_sink = {
    &audio_sink_init,
    &audio_sink_set_volume,
    &audio_sink_start_stream,
    &audio_sink_stop_stream,
    &audio_sink_close,
};

void btstack_audio_rtt_init_instance(void)
{
    btstack_audio_sink_set_instance(&btstack_audio_rtt_sink);
}
