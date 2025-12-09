/* Edge Impulse Wakeword Detection Test for RT-Thread
 * Copyright (c) 2025 RT-Thread
 *
 * This example demonstrates wake word detection using the microphone
 * on PSoC Edge platform with RT-Thread OS.
 */

#include <rtthread.h>
#include <rtdevice.h>
#include <string.h>
#include <rtconfig.h>

// Edge Impulse SDK headers
#include "edge-impulse-sdk/classifier/ei_run_classifier.h"
#include "edge-impulse-sdk/dsp/numpy.hpp"
#include "model-parameters/model_metadata.h"

/* Audio configuration */
#define AUDIO_SAMPLE_RATE       EI_CLASSIFIER_FREQUENCY     /* Sample rate in Hz */
#define AUDIO_CHANNELS          1                            /* Mono audio */
#define AUDIO_BITS_PER_SAMPLE   16                           /* 16-bit samples */

/* Buffer configuration */
#define AUDIO_BUFFER_SIZE       (EI_CLASSIFIER_RAW_SAMPLE_COUNT * sizeof(int16_t))
#define AUDIO_SLICE_SIZE        (EI_CLASSIFIER_SLICE_SIZE)

/* Audio device name */
#define AUDIO_DEVICE_NAME       "mic0"

/* Global variables */
static rt_device_t audio_device = RT_NULL;
static int16_t audio_buffer[EI_CLASSIFIER_RAW_SAMPLE_COUNT];
static volatile rt_bool_t audio_ready = RT_FALSE;

/* Inference result callback */
static void print_inference_result(ei_impulse_result_t *result)
{
    rt_kprintf("\n--- Inference Results ---\n");
    rt_kprintf("Timing: DSP %d ms, inference %d ms, anomaly %d ms\n",
               result->timing.dsp,
               result->timing.classification,
               result->timing.anomaly);

#if EI_CLASSIFIER_OBJECT_DETECTION == 1
    rt_kprintf("Object detection bounding boxes:\n");
    for (uint32_t i = 0; i < result->bounding_boxes_count; i++) {
        ei_impulse_result_bounding_box_t bb = result->bounding_boxes[i];
        if (bb.value == 0) {
            continue;
        }
        rt_kprintf("  %s (%.2f) [x:%u, y:%u, w:%u, h:%u]\n",
                   bb.label, bb.value, bb.x, bb.y, bb.width, bb.height);
    }
#else
    rt_kprintf("Predictions:\n");
    for (uint16_t i = 0; i < EI_CLASSIFIER_LABEL_COUNT; i++) {
        rt_kprintf("  %s: %.5f\n", 
                   ei_classifier_inferencing_categories[i],
                   result->classification[i].value);
    }
#endif

#if EI_CLASSIFIER_HAS_ANOMALY
    rt_kprintf("Anomaly prediction: %.3f\n", result->anomaly);
#endif

    /* Find best prediction */
    int best_idx = 0;
    float best_score = result->classification[0].value;
    for (uint16_t i = 1; i < EI_CLASSIFIER_LABEL_COUNT; i++) {
        if (result->classification[i].value > best_score) {
            best_score = result->classification[i].value;
            best_idx = i;
        }
    }
    
    rt_kprintf("==> Best: %s (%.2f%%)\n", 
               ei_classifier_inferencing_categories[best_idx],
               best_score * 100.0f);
}

/**
 * @brief Get audio data callback for Edge Impulse
 */
static int get_audio_signal_data(size_t offset, size_t length, float *out_ptr)
{
    /* Convert int16_t to float */
    for (size_t i = 0; i < length; i++) {
        out_ptr[i] = (float)audio_buffer[offset + i] / 32768.0f;
    }
    return 0;
}

/**
 * @brief Static buffer inference test
 */
static void ei_static_buffer_test(void)
{
    rt_kprintf("\n========================================\n");
    rt_kprintf("Edge Impulse Static Buffer Test\n");
    rt_kprintf("========================================\n");
    
    rt_kprintf("Model: %s\n", EI_CLASSIFIER_PROJECT_NAME);
    rt_kprintf("Sample rate: %d Hz\n", EI_CLASSIFIER_FREQUENCY);
    rt_kprintf("Frame size: %d samples\n", EI_CLASSIFIER_RAW_SAMPLE_COUNT);
    rt_kprintf("Label count: %d\n", EI_CLASSIFIER_LABEL_COUNT);
    
    /* Generate test audio data - sine wave for testing */
    rt_kprintf("\nGenerating test audio data...\n");
    for (int i = 0; i < EI_CLASSIFIER_RAW_SAMPLE_COUNT; i++) {
        /* Generate a simple tone at 1000 Hz */
        float t = (float)i / (float)EI_CLASSIFIER_FREQUENCY;
        float sample = 0.5f * arm_sin_f32(2.0f * 3.14159f * 1000.0f * t);
        audio_buffer[i] = (int16_t)(sample * 32767.0f);
    }
    
    /* Setup signal */
    signal_t signal;
    signal.total_length = EI_CLASSIFIER_RAW_SAMPLE_COUNT;
    signal.get_data = &get_audio_signal_data;
    
    /* Run inference */
    ei_impulse_result_t result = { 0 };
    EI_IMPULSE_ERROR err = run_classifier(&signal, &result, false);
    
    if (err != EI_IMPULSE_OK) {
        rt_kprintf("ERROR: Failed to run classifier (%d)\n", err);
        return;
    }
    
    print_inference_result(&result);
}

/* 
 * PDM driver configuration:
 * - With ENABLE_STEREO_INPUT_FEED: Stereo mode, 320 samples per 10ms (160 L + 160 R interleaved)
 * - Without ENABLE_STEREO_INPUT_FEED: Mono mode, 160 samples per 10ms frame
 * - Data format (stereo): L0, R0, L1, R1, L2, R2, ... (interleaved)
 * - We need to extract/use mono channel for Edge Impulse model
 */
#ifdef ENABLE_STEREO_INPUT_FEED
/* Stereo mode: dual microphone */
#define PDM_FRAME_SAMPLES           320                                    /* Total stereo samples per frame */
#define PDM_MONO_FRAME_SAMPLES      (PDM_FRAME_SAMPLES / 2)                /* Mono samples after extraction */
#define PDM_IS_STEREO               1
#else
/* Mono mode: single microphone */
#define PDM_FRAME_SAMPLES           160                                    /* Total mono samples per frame */
#define PDM_MONO_FRAME_SAMPLES      PDM_FRAME_SAMPLES                      /* Mono samples (no extraction needed) */
#define PDM_IS_STEREO               0
#endif
#define PDM_FRAME_SIZE              (PDM_FRAME_SAMPLES * sizeof(int16_t))  /* Bytes per frame */

#ifdef RT_USING_AUDIO
/**
 * @brief Microphone inference thread
 * 
 * With ENABLE_STEREO_INPUT_FEED (dual mic):
 *   - PDM driver produces 320 stereo samples (10ms) per read (L,R interleaved)
 *   - We extract right channel and accumulate 16000 samples for inference
 * Without ENABLE_STEREO_INPUT_FEED (single mic):
 *   - PDM driver produces 160 mono samples (10ms) per read
 *   - We directly accumulate 16000 samples for inference
 */
static void ei_microphone_thread(void *parameter)
{
    rt_kprintf("\n========================================\n");
    rt_kprintf("Edge Impulse Microphone Inference\n");
    rt_kprintf("========================================\n");
    
    /* Open audio device */
    audio_device = rt_device_find(AUDIO_DEVICE_NAME);
    if (audio_device == RT_NULL) {
        rt_kprintf("ERROR: Cannot find audio device '%s'\n", AUDIO_DEVICE_NAME);
        return;
    }
    
    rt_err_t ret = rt_device_open(audio_device, RT_DEVICE_FLAG_RDONLY);
    if (ret != RT_EOK) {
        rt_kprintf("ERROR: Cannot open audio device (%d)\n", ret);
        return;
    }
    
    rt_kprintf("Audio device opened successfully\n");
    rt_kprintf("Model requires %d mono samples (%d ms)\n", 
               EI_CLASSIFIER_RAW_SAMPLE_COUNT,
               EI_CLASSIFIER_RAW_SAMPLE_COUNT * 1000 / EI_CLASSIFIER_FREQUENCY);
#if PDM_IS_STEREO
    rt_kprintf("PDM driver: STEREO mode (dual mic), %d samples/frame -> %d mono samples\n", 
               PDM_FRAME_SAMPLES, PDM_MONO_FRAME_SAMPLES);
#else
    rt_kprintf("PDM driver: MONO mode (single mic), %d samples/frame\n", 
               PDM_FRAME_SAMPLES);
#endif
    rt_kprintf("Listening for wake words...\n\n");
    
    signal_t signal;
    signal.total_length = EI_CLASSIFIER_RAW_SAMPLE_COUNT;
    signal.get_data = &get_audio_signal_data;
    
    /* Temporary buffer for PDM frame */
    int16_t pdm_frame[PDM_FRAME_SAMPLES];
    
    while (1) {
        /* Accumulate mono audio samples until we have enough for inference */
        rt_size_t samples_collected = 0;
        
        while (samples_collected < EI_CLASSIFIER_RAW_SAMPLE_COUNT) {
            /* Read one PDM frame */
            rt_size_t read_size = rt_device_read(audio_device, 0, 
                                                 pdm_frame, PDM_FRAME_SIZE);
            
            if (read_size == 0) {
                /* No data available yet, wait a bit */
                rt_thread_mdelay(5);
                continue;
            }
            
            /* Calculate how many samples we got */
            rt_size_t total_samples = read_size / sizeof(int16_t);
            
#if PDM_IS_STEREO
            /* Stereo mode: extract mono channel from interleaved stereo data */
            rt_size_t mono_samples = total_samples / 2;
            
            /* Extract right channel from stereo data (use right channel - index 1, 3, 5...) */
            /* The right channel typically has better audio in this hardware */
            for (rt_size_t i = 0; i < mono_samples && samples_collected < EI_CLASSIFIER_RAW_SAMPLE_COUNT; i++) {
                /* Extract right channel: samples at odd indices (1, 3, 5, ...) */
                audio_buffer[samples_collected++] = pdm_frame[i * 2 + 1];
            }
#else
            /* Mono mode: directly copy samples */
            for (rt_size_t i = 0; i < total_samples && samples_collected < EI_CLASSIFIER_RAW_SAMPLE_COUNT; i++) {
                audio_buffer[samples_collected++] = pdm_frame[i];
            }
#endif
        }
        
        /* Run inference */
        ei_impulse_result_t result = { 0 };
        EI_IMPULSE_ERROR err = run_classifier(&signal, &result, false);
        
        if (err != EI_IMPULSE_OK) {
            rt_kprintf("ERROR: Inference failed (%d)\n", err);
            continue;
        }
        
        /* Check for wake word detection */
        for (uint16_t i = 0; i < EI_CLASSIFIER_LABEL_COUNT; i++) {
            /* Threshold: 80% confidence */
            if (result.classification[i].value > 0.8f) {
                rt_kprintf("\n*** DETECTED: %s (%.2f%%) ***\n",
                           ei_classifier_inferencing_categories[i],
                           result.classification[i].value * 100.0f);
            }
        }
        
        /* Print timing info periodically */
        static int inference_count = 0;
        if (++inference_count % 10 == 0) {
            rt_kprintf("Inference #%d: DSP=%dms, NN=%dms\n", 
                       inference_count,
                       result.timing.dsp,
                       result.timing.classification);
        }
    }
    
    rt_device_close(audio_device);
}

/**
 * @brief Start microphone inference
 */
static int ei_microphone_start(void)
{
    rt_thread_t tid = rt_thread_create("ei_mic",
                                       ei_microphone_thread,
                                       RT_NULL,
                                       4096,
                                       20,
                                       10);
    if (tid != RT_NULL) {
        rt_thread_startup(tid);
        return 0;
    }
    return -1;
}
#endif /* RT_USING_AUDIO */

/**
 * @brief Edge Impulse info command
 */
static int ei_info(int argc, char **argv)
{
    rt_kprintf("\n========================================\n");
    rt_kprintf("Edge Impulse Model Information\n");
    rt_kprintf("========================================\n");
    rt_kprintf("Project ID:    %d\n", EI_CLASSIFIER_PROJECT_ID);
    rt_kprintf("Project name:  %s\n", EI_CLASSIFIER_PROJECT_NAME);
    rt_kprintf("Project owner: %s\n", EI_CLASSIFIER_PROJECT_OWNER);
    rt_kprintf("Deploy version: %d\n", EI_CLASSIFIER_PROJECT_DEPLOY_VERSION);
    rt_kprintf("\n");
    rt_kprintf("Input frame size:   %d\n", EI_CLASSIFIER_RAW_SAMPLE_COUNT);
    rt_kprintf("Sample frequency:   %d Hz\n", EI_CLASSIFIER_FREQUENCY);
    rt_kprintf("Frame interval:     %.4f ms\n", EI_CLASSIFIER_INTERVAL_MS);
    rt_kprintf("NN input frame size: %d\n", EI_CLASSIFIER_NN_INPUT_FRAME_SIZE);
    rt_kprintf("NN output count:    %d\n", EI_CLASSIFIER_NN_OUTPUT_COUNT);
    rt_kprintf("Label count:        %d\n", EI_CLASSIFIER_LABEL_COUNT);
    rt_kprintf("\n");
    rt_kprintf("Labels:\n");
    for (int i = 0; i < EI_CLASSIFIER_LABEL_COUNT; i++) {
        rt_kprintf("  [%d] %s\n", i, ei_classifier_inferencing_categories[i]);
    }
    rt_kprintf("========================================\n");
    
    return 0;
}

/**
 * @brief Edge Impulse test command
 */
static int ei_test(int argc, char **argv)
{
    if (argc < 2) {
        rt_kprintf("Usage: ei_test <command>\n");
        rt_kprintf("Commands:\n");
        rt_kprintf("  info   - Show model information\n");
        rt_kprintf("  static - Run inference with static test data\n");
#ifdef RT_USING_AUDIO
        rt_kprintf("  mic    - Start microphone inference\n");
#endif
        return -1;
    }
    
    if (strcmp(argv[1], "info") == 0) {
        return ei_info(argc - 1, argv + 1);
    }
    else if (strcmp(argv[1], "static") == 0) {
        ei_static_buffer_test();
        return 0;
    }
#ifdef RT_USING_AUDIO
    else if (strcmp(argv[1], "mic") == 0) {
        return ei_microphone_start();
    }
#endif
    else {
        rt_kprintf("Unknown command: %s\n", argv[1]);
        return -1;
    }
    
    return 0;
}

/* RT-Thread MSH commands */
#ifdef RT_USING_FINSH
#include <finsh.h>
MSH_CMD_EXPORT(ei_info, Show Edge Impulse model information);
MSH_CMD_EXPORT(ei_test, Edge Impulse inference test);
#endif

