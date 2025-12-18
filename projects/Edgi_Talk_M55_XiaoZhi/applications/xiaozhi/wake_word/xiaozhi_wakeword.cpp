/*
 * Copyright (c) 2006-2025, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2025-12-14     RT-Thread    First version
 */

#include "xiaozhi_wakeword.h"
#include <rtthread.h>
#include <rtdevice.h>
#include <string.h>
#include <rtconfig.h>
#include "xiaozhi.h"  /* Include for xz_audio_t definition */

// Edge Impulse SDK headers
#include "edge-impulse-sdk/classifier/ei_run_classifier.h"
#include "edge-impulse-sdk/dsp/numpy.hpp"
#include "model-parameters/model_metadata.h"

#define DBG_TAG "xz.wakeword"
#define DBG_LVL DBG_LOG
#include <rtdbg.h>

/* Audio device name */
#ifndef BSP_XIAOZHI_MIC_DEVICE_NAME
#define BSP_XIAOZHI_MIC_DEVICE_NAME "mic0"
#endif
#define AUDIO_DEVICE_NAME BSP_XIAOZHI_MIC_DEVICE_NAME

/* Wake word detection configuration */
#define WAKE_WORD_CONFIDENCE_THRESHOLD 0.80f /* 80% confidence threshold */
#define WAKE_WORD_COOLDOWN_MS 1000           /* 1 seconds cooldown between detections */

/* Global variables */
static rt_device_t audio_device = RT_NULL;
static int16_t audio_buffer[EI_CLASSIFIER_RAW_SAMPLE_COUNT];
static volatile rt_bool_t wakeword_enabled = RT_FALSE;
static rt_bool_t wakeword_initialized = RT_FALSE;
static rt_thread_t wakeword_tid = RT_NULL;
static uint32_t last_detection_time = 0;
static rt_event_t wakeword_event = RT_NULL;  /* Event for thread control */

/* Statistics */
static uint32_t total_inferences = 0;

/* Initialize audio buffer to prevent undefined behavior */
static void audio_buffer_init(void)
{
    memset(audio_buffer, 0, sizeof(audio_buffer));
}

/**
 * @brief Get audio data callback for Edge Impulse
 */
static int get_audio_signal_data(size_t offset, size_t length, float *out_ptr)
{
    /* Validate bounds to prevent buffer overflow */
    if (offset + length > EI_CLASSIFIER_RAW_SAMPLE_COUNT)
    {
        LOG_E("Invalid audio data request: offset=%zu, length=%zu, buffer_size=%d",
              offset, length, EI_CLASSIFIER_RAW_SAMPLE_COUNT);
        return -1;
    }

    /* Convert int16_t to float */
    numpy::int16_to_float(&audio_buffer[offset], out_ptr, length);
    return 0;
}

/*
 * PDM driver configuration:
 */
#ifdef ENABLE_STEREO_INPUT_FEED
    #define PDM_FRAME_SAMPLES 320
    #define PDM_MONO_FRAME_SAMPLES (PDM_FRAME_SAMPLES / 2)
    #define PDM_IS_STEREO 1
#else
    #define PDM_FRAME_SAMPLES 160
    #define PDM_MONO_FRAME_SAMPLES PDM_FRAME_SAMPLES
    #define PDM_IS_STEREO 0
#endif
#define PDM_FRAME_SIZE (PDM_FRAME_SAMPLES * sizeof(int16_t))

#ifdef RT_USING_AUDIO
/**
 * @brief Wake word detection thread
 *
 * Permanent thread that listens for wake words using the microphone
 * and triggers callback when detected with sufficient confidence
 */
static void xz_wakeword_thread(void *parameter)
{
    LOG_I("Starting wake word detection thread");
    LOG_I("Model: %s", EI_CLASSIFIER_PROJECT_NAME);
    LOG_I("Sample rate: %d Hz", EI_CLASSIFIER_FREQUENCY);
    LOG_I("Frame size: %d samples", EI_CLASSIFIER_RAW_SAMPLE_COUNT);
    LOG_I("Labels: %d", EI_CLASSIFIER_LABEL_COUNT);

    /* Print available labels for debugging */
    for (int i = 0; i < EI_CLASSIFIER_LABEL_COUNT; i++)
    {
        LOG_I("  [%d] %s", i, ei_classifier_inferencing_categories[i]);
    }

    signal_t signal;
    signal.total_length = EI_CLASSIFIER_RAW_SAMPLE_COUNT;
    signal.get_data = &get_audio_signal_data;

    /* Temporary buffer for PDM frame */
    int16_t pdm_frame[PDM_FRAME_SAMPLES];

    /* Main thread loop - permanent */
    while (RT_TRUE)
    {
        rt_uint32_t received_event;

        /* Wait for control event */
        if (rt_event_recv(wakeword_event,
                          XZ_WAKEWORD_EVENT_START | XZ_WAKEWORD_EVENT_EXIT,
                          RT_EVENT_FLAG_OR | RT_EVENT_FLAG_CLEAR,
                          RT_WAITING_FOREVER,
                          &received_event) != RT_EOK)
        {
            continue;
        }

        /* Check for exit event */
        if (received_event & XZ_WAKEWORD_EVENT_EXIT)
        {
            LOG_I("Wake word thread received exit event");
            break;
        }

        /* Start detection */
        if (received_event & XZ_WAKEWORD_EVENT_START)
        {
            LOG_I("Starting wake word detection loop");

            /* Open audio device if needed */
            if (audio_device == RT_NULL)
            {
                audio_device = rt_device_find(AUDIO_DEVICE_NAME);
                if (audio_device == RT_NULL)
                {
                    LOG_E("Cannot find audio device '%s'", AUDIO_DEVICE_NAME);
                    continue;
                }

                if (rt_device_open(audio_device, RT_DEVICE_FLAG_RDONLY) != RT_EOK)
                {
                    LOG_E("Cannot open audio device");
                    audio_device = RT_NULL;
                    continue;
                }

                LOG_I("Wakeword: Audio device opened successfully");
            }

            LOG_I("Model requires %d mono samples (%d ms)",
                  EI_CLASSIFIER_RAW_SAMPLE_COUNT,
                  EI_CLASSIFIER_RAW_SAMPLE_COUNT * 1000 / EI_CLASSIFIER_FREQUENCY);

#if PDM_IS_STEREO
            LOG_I("PDM driver: STEREO mode (dual mic), %d samples/frame -> %d mono samples",
                  PDM_FRAME_SAMPLES, PDM_MONO_FRAME_SAMPLES);
#else
            LOG_I("PDM driver: MONO mode (single mic), %d samples/frame", PDM_FRAME_SAMPLES);
#endif
            LOG_I("Listening for wake words...");

            /* Initialize audio buffer before starting */
            audio_buffer_init();
            wakeword_enabled = RT_TRUE;

            /* Detection loop */
            while (wakeword_enabled)
            {
                /* Check for stop event at the beginning of each iteration */
                if (rt_event_recv(wakeword_event, XZ_WAKEWORD_EVENT_STOP,
                                  RT_EVENT_FLAG_OR | RT_EVENT_FLAG_CLEAR,
                                  RT_WAITING_NO, &received_event) == RT_EOK)
                {
                    if (received_event & XZ_WAKEWORD_EVENT_STOP)
                    {
                        LOG_D("Received stop event at loop start");
                        wakeword_enabled = RT_FALSE;
                        break;
                    }
                }

                /* Accumulate mono audio samples until we have enough for inference */
                rt_size_t samples_collected = 0;

                while (samples_collected < EI_CLASSIFIER_RAW_SAMPLE_COUNT && wakeword_enabled)
                {
                    /* Check for stop event while collecting samples */
                    if (rt_event_recv(wakeword_event, XZ_WAKEWORD_EVENT_STOP,
                                      RT_EVENT_FLAG_OR | RT_EVENT_FLAG_CLEAR,
                                      RT_WAITING_NO, &received_event) == RT_EOK)
                    {
                        if (received_event & XZ_WAKEWORD_EVENT_STOP)
                        {
                            LOG_D("Received stop event during sample collection");
                            wakeword_enabled = RT_FALSE;
                            break;
                        }
                    }

                    /* Read one PDM frame */
                    rt_size_t read_size = rt_device_read(audio_device, 0,
                                                         pdm_frame, PDM_FRAME_SIZE);

                    if (read_size == 0)
                    {
                        /* No data available yet, brief delay */
                        rt_thread_mdelay(1);
                        continue;
                    }

                    /* Calculate how many samples we got */
                    rt_size_t total_samples = read_size / sizeof(int16_t);

#if PDM_IS_STEREO
                    /* Stereo mode: extract mono channel from interleaved stereo data */
                    rt_size_t mono_samples = total_samples / 2;

                    /* Extract right channel from stereo data (use right channel - index 1, 3, 5...) */
                    /* The right channel typically has better audio in this hardware */
                    for (rt_size_t i = 0; i < mono_samples && samples_collected < EI_CLASSIFIER_RAW_SAMPLE_COUNT; i++)
                    {
                        /* Extract right channel: samples at odd indices (1, 3, 5, ...) */
                        audio_buffer[samples_collected++] = pdm_frame[i * 2 + 1];
                    }
#else
                    /* Mono mode: directly copy samples */
                    for (rt_size_t i = 0; i < total_samples && samples_collected < EI_CLASSIFIER_RAW_SAMPLE_COUNT; i++)
                    {
                        audio_buffer[samples_collected++] = pdm_frame[i];
                    }
#endif
                }

                /* Check if we should continue */
                if (!wakeword_enabled)
                {
                    break;
                }

                /* Run inference */
                ei_impulse_result_t result = {0};
                memset(&result, 0, sizeof(result));

                EI_IMPULSE_ERROR err = run_classifier(&signal, &result, false);

                if (err != EI_IMPULSE_OK)
                {
                    LOG_E("Inference failed (%d)", err);
                    if (err == EI_IMPULSE_INVALID_SIZE)
                    {
                        LOG_E("Tensor size mismatch - resetting audio buffer");
                        audio_buffer_init();
                    }
                    continue;
                }

                total_inferences++;

                /* Check for wake word detection */
                for (uint16_t i = 0; i < EI_CLASSIFIER_LABEL_COUNT; i++)
                {
                    /* Skip noise/negative labels and check confidence threshold */
                    if (result.classification[i].value > WAKE_WORD_CONFIDENCE_THRESHOLD)
                    {
                        const char *label = ei_classifier_inferencing_categories[i];
                        float confidence = result.classification[i].value;

                        /* Only process actual wake words, ignore noise/negative */
                        if (strstr(label, "xiaorui") != NULL)
                        {
                            uint32_t current_time = rt_tick_get();

                            /* Check cooldown period to avoid multiple rapid detections */
                            if ((current_time - last_detection_time) >= rt_tick_from_millisecond(WAKE_WORD_COOLDOWN_MS))
                            {
                                last_detection_time = current_time;

                                LOG_W("*** WAKE WORD DETECTED: %s (%.2f%%) ***",
                                      label, confidence * 100.0f);

                                /* Call the callback function in xiaozhi.cpp */
                                xz_wakeword_detected_callback(label, confidence);
                            }
                        }
                    }
                }

                /* Print timing info periodically */
                if (total_inferences % 50 == 0)
                {
                    LOG_D("Inference #%d: DSP=%dms, NN=%dms",
                          total_inferences,
                          result.timing.dsp,
                          result.timing.classification);
                }
            }

            /* Close audio device when stopping detection */
            if (audio_device != RT_NULL)
            {
                LOG_I("Wakeword: Closing audio device after detection loop exit");
                rt_device_close(audio_device);
                audio_device = RT_NULL;
                LOG_I("Wakeword: Audio device closed after detection");
            }
        }
    }

    LOG_I("Wake word detection thread exiting");
    wakeword_initialized = RT_FALSE;
}

/**
 * @brief Initialize wake word detection
 */
int xz_wakeword_init(void)
{
    /* Already initialized */
    if (wakeword_initialized)
    {
        return 0;
    }

#ifdef RT_USING_AUDIO
    LOG_I("Initializing wake word detection");

    /* Create event control */
    wakeword_event = rt_event_create("wakeword_evt", RT_IPC_FLAG_FIFO);
    if (wakeword_event == RT_NULL)
    {
        LOG_E("Failed to create wake word event");
        return -RT_ENOMEM;
    }

    /* Create permanent thread */
    wakeword_tid = rt_thread_create("xz_wakeword",
                                    xz_wakeword_thread,
                                    RT_NULL,
                                    4096,
                                    20,  /* Priority */
                                    5);   /* Smaller tick slice for better responsiveness */

    if (wakeword_tid == RT_NULL)
    {
        LOG_E("Failed to create wake word thread");
        rt_event_delete(wakeword_event);
        wakeword_event = RT_NULL;
        return -RT_ENOMEM;
    }

    /* Start the thread */
    if (rt_thread_startup(wakeword_tid) != RT_EOK)
    {
        LOG_E("Failed to start wake word thread");
        rt_thread_delete(wakeword_tid);
        wakeword_tid = RT_NULL;
        rt_event_delete(wakeword_event);
        wakeword_event = RT_NULL;
        return -RT_ERROR;
    }

    wakeword_initialized = RT_TRUE;
    LOG_I("Wake word detection initialized successfully");
    return 0;
#else
    LOG_E("Audio device not enabled");
    return -RT_ERROR;
#endif
}

/**
 * @brief Deinitialize wake word detection
 */
int xz_wakeword_deinit(void)
{
    if (!wakeword_initialized)
    {
        return 0;
    }

    LOG_I("Deinitializing wake word detection...");

    /* Stop detection if running */
    xz_wakeword_stop();

    /* Send exit event to thread */
    if (wakeword_event != RT_NULL)
    {
        rt_event_send(wakeword_event, XZ_WAKEWORD_EVENT_EXIT);
    }

    /* Give thread time to exit */
    rt_thread_mdelay(100);

    /* Clean up resources */
    if (wakeword_event != RT_NULL)
    {
        rt_event_delete(wakeword_event);
        wakeword_event = RT_NULL;
    }

    wakeword_initialized = RT_FALSE;
    wakeword_tid = RT_NULL;
    audio_device = RT_NULL;

    LOG_I("Wake word detection deinitialized");
    return 0;
}

/**
 * @brief Start wake word detection
 */
int xz_wakeword_start(void)
{
    if (!wakeword_initialized)
    {
        LOG_E("Wake word not initialized");
        return -RT_ERROR;
    }

    if (wakeword_enabled)
    {
        LOG_D("Wake word already enabled");
        return 0;
    }

    if (wakeword_event != RT_NULL)
    {
        /* Clear any pending events first */
        rt_uint32_t recv_event;
        rt_event_recv(wakeword_event, 0xFFFFFFFF, RT_EVENT_FLAG_OR | RT_EVENT_FLAG_CLEAR, RT_WAITING_NO, &recv_event);

        /* Now send START event */
        rt_event_send(wakeword_event, XZ_WAKEWORD_EVENT_START);
    }

    return 0;
}

/**
 * @brief Stop wake word detection
 */
int xz_wakeword_stop(void)
{
    if (!wakeword_initialized || !wakeword_enabled)
    {
        LOG_D("Wake word already stopped or not initialized");
        return 0;
    }

    LOG_I("Stopping wake word detection...");

    wakeword_enabled = RT_FALSE;

    if (wakeword_event != RT_NULL)
    {
        rt_event_send(wakeword_event, XZ_WAKEWORD_EVENT_STOP);
    }

    /* Wait a bit for thread to respond to STOP event before closing device */
    rt_thread_mdelay(50);

    /* Close audio device - thread should have already closed it, but ensure it's closed */
    if (audio_device != RT_NULL)
    {
        LOG_D("Closing audio device in stop function");
        rt_device_close(audio_device);
        audio_device = RT_NULL;
    }

    LOG_I("Wake word detection stopped");
    return 0;
}

/**
 * @brief Check if wake word detection is enabled
 */
rt_bool_t xz_wakeword_is_enabled(void)
{
    return wakeword_enabled;
}

/**
 * @brief Print wake word detection information
 */
void xz_wakeword_info(void)
{
    LOG_I("=== XiaoZhi Wake Word Detection Info ===");
    LOG_I("Status: %s", wakeword_enabled ? "Enabled" : "Disabled");
    LOG_I("Model: %s", EI_CLASSIFIER_PROJECT_NAME);
    LOG_I("Project ID: %d", EI_CLASSIFIER_PROJECT_ID);
    LOG_I("Owner: %s", EI_CLASSIFIER_PROJECT_OWNER);
    LOG_I("Sample rate: %d Hz", EI_CLASSIFIER_FREQUENCY);
    LOG_I("Frame size: %d samples (%d ms)",
          EI_CLASSIFIER_RAW_SAMPLE_COUNT,
          EI_CLASSIFIER_RAW_SAMPLE_COUNT * 1000 / EI_CLASSIFIER_FREQUENCY);
    LOG_I("Labels:");
    for (int i = 0; i < EI_CLASSIFIER_LABEL_COUNT; i++)
    {
        LOG_I("  [%d] %s", i, ei_classifier_inferencing_categories[i]);
    }
    LOG_I("===========================================");
}

#else /* RT_USING_AUDIO */

int xz_wakeword_init(void)
{
    LOG_E("Audio not enabled (RT_USING_AUDIO not defined)");
    return -1;
}

int xz_wakeword_deinit(void)
{
    return 0;
}

rt_bool_t xz_wakeword_is_enabled(void)
{
    return RT_FALSE;
}

void xz_wakeword_info(void)
{
    LOG_E("Audio not enabled (RT_USING_AUDIO not defined)");
}

#endif /* RT_USING_AUDIO */

/* RT-Thread MSH commands for testing */
#ifdef RT_USING_FINSH
    #include <finsh.h>
    MSH_CMD_EXPORT(xz_wakeword_init, Enable XiaoZhi wake word detection);
    MSH_CMD_EXPORT(xz_wakeword_deinit, Disable XiaoZhi wake word detection);
    MSH_CMD_EXPORT(xz_wakeword_info, Show XiaoZhi wake word detection info);
#endif