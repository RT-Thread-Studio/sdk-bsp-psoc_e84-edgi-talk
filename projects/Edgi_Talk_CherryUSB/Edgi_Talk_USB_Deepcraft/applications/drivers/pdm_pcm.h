#ifndef APP_PDM_PCM_H
#define APP_PDM_PCM_H

#include <stdbool.h>

#include "cy_result.h"
#include "cycfg_peripherals.h"

typedef enum
{
    PDM_PCM_STATUS_SUCCESS = 0u,
    PDM_PCM_STATUS_BAD_PARAM,
    PDM_PCM_STATUS_FAIL,
} PDM_PCM_STATUS_t;

typedef enum
{
    MODE_MONO = 0u,
    MODE_STEREO,
} MODE_t;

typedef enum
{
    SAMPLE_RATE_8000 = 0u,
    SAMPLE_RATE_16000,
    SAMPLE_RATE_22050,
    SAMPLE_RATE_44100,
    SAMPLE_RATE_48000,
} SAMPLE_RATE;

typedef struct
{
    MODE_t mode;
    uint8_t channel_index_list[2];
    cy_stc_pdm_pcm_channel_config_t channel_config[2];
    cy_stc_sysint_t pdm_irq_cfg;
} PDM_PCM_CONFIG_t;

typedef struct pdm_pcm_t *pdm_pcm;

PDM_PCM_STATUS_t pdm_pcm_set_gain(pdm_pcm handle, uint8_t channel_num, cy_en_pdm_pcm_gain_sel_t gain);
PDM_PCM_STATUS_t pdm_pcm_init_hw(SAMPLE_RATE sample_rate);
pdm_pcm pdm_pcm_create(PDM_PCM_CONFIG_t *config);
PDM_PCM_STATUS_t pdm_pcm_update_config(pdm_pcm handle, PDM_PCM_CONFIG_t *config);
PDM_PCM_STATUS_t pdm_pcm_start(pdm_pcm handle);
PDM_PCM_STATUS_t pdm_pcm_stop(pdm_pcm handle);

bool pdm_pcm_data_ready(pdm_pcm handle);
void pdm_pcm_discard_samples(pdm_pcm handle);
int16_t *pdm_pcm_get_full_buffer(pdm_pcm handle);

const char **pdm_pcm_get_string_list_of_sample_rates(void);
uint8_t pdm_pcm_get_sample_rate_option_count(void);
const char **pdm_pcm_get_string_list_of_gain_options(void);
uint8_t pdm_pcm_get_gain_option_count(void);

int pdm_pcm_get_frequency_from_frequency_index(SAMPLE_RATE frequency_index);
void pdm_pcm_clear_data_ready_flag(pdm_pcm handle);
uint32_t pdm_pcm_get_frame_count(pdm_pcm handle);

#endif
