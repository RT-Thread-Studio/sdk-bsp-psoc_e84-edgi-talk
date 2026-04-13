#include "pdm_pcm.h"

#if defined(CYBSP_PDM_ENABLED)

#include <stdio.h>
#include <string.h>

#include "cy_pdm_pcm_v2.h"
#include "cycfg_peripheral_clocks.h"

#define MAX_CHANNEL_COUNT_PER_HANDLE 2u
#define MAX_HANDLE_COUNT 2u

#define HW_FIFO_SIZE 64u
#define RX_FIFO_TRIG_LEVEL (HW_FIFO_SIZE / 2u)

/* Keep frame size small to reduce end-to-end latency of audio chunk delivery. */
#define FRAME_SIZE 320u
#define NUMBER_INTERRUPTS_FOR_FRAME (FRAME_SIZE / RX_FIFO_TRIG_LEVEL)

#define PDM_PCM_ISR_PRIORITY 2u

#define DPLL_INPUT_FREQ_HZ 17203200UL
#define DPLL_OUTPUT1_FREQ_HZ 73728000UL
#define DPLL_OUTPUT2_FREQ_HZ 169344000UL
#define DPLL_ENABLE_TIMEOUT_MS 10000U

static const char *g_sample_rate_labels[] = {
    "8 kHz", "16 kHz", "22.05 kHz", "44.1 kHz", "48 kHz"
};

static const char *g_gain_labels[] = {
    "83 dB", "77 dB", "71 dB", "65 dB", "59 dB", "53 dB", "47 dB",
    "41 dB", "35 dB", "29 dB", "23 dB", "17 dB", "11 dB",
    "5 dB", "-1 dB", "-7 dB", "-13 dB", "-19 dB", "-25 dB",
    "-31 dB", "-37 dB", "-43 dB", "-49 dB", "-55 dB", "-61 dB",
    "-67 dB", "-73 dB", "-79 dB", "-85 dB", "-91 dB",
    "-97 dB", "-103 dB"
};

typedef struct pdm_pcm_t
{
    uint8_t index;
    int16_t audio_buffer0[FRAME_SIZE];
    int16_t audio_buffer1[FRAME_SIZE];
    int16_t *active_rx_buffer;
    int16_t *full_rx_buffer;
    PDM_PCM_CONFIG_t *config;
    cy_en_pdm_pcm_gain_sel_t gain[MAX_CHANNEL_COUNT_PER_HANDLE];
    bool is_used;
    bool initialized;
    bool have_data;
    uint16_t frame_counter;
    int init_discard_counter;
} pdm_pcm_t;

static pdm_pcm_t g_mic_pool[MAX_HANDLE_COUNT] = {0};
static SAMPLE_RATE g_sample_rate = SAMPLE_RATE_16000;

static void pdm_pcm_event_handler_0(void);
static void pdm_pcm_event_handler_1(void);

static void (*const g_event_handlers[MAX_HANDLE_COUNT])(void) = {
    pdm_pcm_event_handler_0,
    pdm_pcm_event_handler_1,
};

static void dpll_lp_set_freq(uint32_t freq)
{
    cy_stc_pll_config_t dpll_lp1;

    if (Cy_SysClk_PllGetFrequency(SRSS_DPLL_LP_1_PATH_NUM) == freq)
    {
        return;
    }

    dpll_lp1.inputFreq = DPLL_INPUT_FREQ_HZ;
    dpll_lp1.outputFreq = freq;
    dpll_lp1.outputMode = CY_SYSCLK_FLLPLL_OUTPUT_AUTO;
    dpll_lp1.lfMode = false;

    Cy_SysClk_PllDisable(SRSS_DPLL_LP_1_PATH_NUM);
    if (CY_SYSCLK_SUCCESS != Cy_SysClk_PllConfigure(SRSS_DPLL_LP_1_PATH_NUM, &dpll_lp1))
    {
        CY_ASSERT(0);
    }

    if (CY_SYSCLK_SUCCESS != Cy_SysClk_PllEnable(SRSS_DPLL_LP_1_PATH_NUM, DPLL_ENABLE_TIMEOUT_MS))
    {
        CY_ASSERT(0);
    }
}

PDM_PCM_STATUS_t pdm_pcm_set_gain(pdm_pcm handle, uint8_t channel_num, cy_en_pdm_pcm_gain_sel_t gain)
{
    uint8_t ch_idx;

    if (handle == NULL || handle->config == NULL)
    {
        return PDM_PCM_STATUS_BAD_PARAM;
    }

    for (ch_idx = 0; ch_idx < (uint8_t)(handle->config->mode + 1u); ch_idx++)
    {
        if (handle->config->channel_index_list[ch_idx] == channel_num)
        {
            handle->gain[ch_idx] = gain;
            return PDM_PCM_STATUS_SUCCESS;
        }
    }

    return PDM_PCM_STATUS_BAD_PARAM;
}

PDM_PCM_STATUS_t pdm_pcm_init_hw(SAMPLE_RATE sample_rate)
{
    g_sample_rate = sample_rate;

    if (sample_rate == SAMPLE_RATE_8000 || sample_rate == SAMPLE_RATE_16000 || sample_rate == SAMPLE_RATE_48000)
    {
        dpll_lp_set_freq(DPLL_OUTPUT1_FREQ_HZ);
    }
    else if (sample_rate == SAMPLE_RATE_22050 || sample_rate == SAMPLE_RATE_44100)
    {
        dpll_lp_set_freq(DPLL_OUTPUT2_FREQ_HZ);
    }
    else
    {
        return PDM_PCM_STATUS_BAD_PARAM;
    }

    Cy_SysClk_PeriPclkDisableDivider((en_clk_dst_t)CYBSP_PDM_CLK_DIV_GRP_NUM, CY_SYSCLK_DIV_16_5_BIT, CYBSP_PDM_CLK_DIV_NUM);

    switch (sample_rate)
    {
    case SAMPLE_RATE_8000:
        Cy_SysClk_PeriPclkSetFracDivider((en_clk_dst_t)CYBSP_PDM_CLK_DIV_GRP_NUM, CY_SYSCLK_DIV_16_5_BIT, CYBSP_PDM_CLK_DIV_NUM, 11u, 0u);
        break;
    case SAMPLE_RATE_16000:
        Cy_SysClk_PeriPclkSetFracDivider((en_clk_dst_t)CYBSP_PDM_CLK_DIV_GRP_NUM, CY_SYSCLK_DIV_16_5_BIT, CYBSP_PDM_CLK_DIV_NUM, 5u, 0u);
        break;
    case SAMPLE_RATE_22050:
        Cy_SysClk_PeriPclkSetFracDivider((en_clk_dst_t)CYBSP_PDM_CLK_DIV_GRP_NUM, CY_SYSCLK_DIV_16_5_BIT, CYBSP_PDM_CLK_DIV_NUM, 9u, 0u);
        break;
    case SAMPLE_RATE_44100:
        Cy_SysClk_PeriPclkSetFracDivider((en_clk_dst_t)CYBSP_PDM_CLK_DIV_GRP_NUM, CY_SYSCLK_DIV_16_5_BIT, CYBSP_PDM_CLK_DIV_NUM, 4u, 0u);
        break;
    case SAMPLE_RATE_48000:
        Cy_SysClk_PeriPclkSetFracDivider((en_clk_dst_t)CYBSP_PDM_CLK_DIV_GRP_NUM, CY_SYSCLK_DIV_16_5_BIT, CYBSP_PDM_CLK_DIV_NUM, 1u, 0u);
        break;
    default:
        return PDM_PCM_STATUS_BAD_PARAM;
    }

    Cy_SysClk_PeriPclkEnableDivider((en_clk_dst_t)CYBSP_PDM_CLK_DIV_GRP_NUM, CY_SYSCLK_DIV_16_5_BIT, CYBSP_PDM_CLK_DIV_NUM);
    return PDM_PCM_STATUS_SUCCESS;
}

pdm_pcm pdm_pcm_create(PDM_PCM_CONFIG_t *config)
{
    uint8_t i;
    pdm_pcm mic = NULL;

    if (config == NULL)
    {
        return NULL;
    }

    for (i = 0; i < MAX_HANDLE_COUNT; i++)
    {
        if (!g_mic_pool[i].is_used)
        {
            mic = &g_mic_pool[i];
            memset(mic, 0, sizeof(*mic));
            mic->index = i;
            mic->config = config;
            mic->is_used = true;
            mic->gain[0] = CY_PDM_PCM_SEL_GAIN_5DB;
            mic->gain[1] = CY_PDM_PCM_SEL_GAIN_5DB;
            break;
        }
    }

    return mic;
}

PDM_PCM_STATUS_t pdm_pcm_update_config(pdm_pcm handle, PDM_PCM_CONFIG_t *config)
{
    if (handle == NULL || config == NULL)
    {
        return PDM_PCM_STATUS_BAD_PARAM;
    }

    handle->config = config;
    return PDM_PCM_STATUS_SUCCESS;
}

static void pdm_pcm_event_handler_common(pdm_pcm handle)
{
    PDM_PCM_CONFIG_t *config;
    uint8_t num_channels;
    uint8_t int_channel_index;
    uint32_t intr_status;

    if (handle == NULL || handle->config == NULL)
    {
        return;
    }

    config = handle->config;
    num_channels = (uint8_t)(config->mode + 1u);
    int_channel_index = (uint8_t)(config->pdm_irq_cfg.intrSrc - pdm_0_interrupts_0_IRQn);

    intr_status = Cy_PDM_PCM_Channel_GetInterruptStatusMasked(CYBSP_PDM_HW, int_channel_index);
    if ((intr_status & CY_PDM_PCM_INTR_RX_TRIGGER) != 0u)
    {
        uint32_t i;
        uint32_t index = 0u;

        for (i = 0; i < (RX_FIFO_TRIG_LEVEL / num_channels); i++)
        {
            uint8_t ch_idx;
            for (ch_idx = 0; ch_idx < num_channels; ch_idx++)
            {
                int32_t data = (int32_t)Cy_PDM_PCM_Channel_ReadFifo(CYBSP_PDM_HW, config->channel_index_list[ch_idx]);
                handle->active_rx_buffer[handle->frame_counter * RX_FIFO_TRIG_LEVEL + index] = (int16_t)data;
                index++;
            }
        }

        Cy_PDM_PCM_Channel_ClearInterrupt(CYBSP_PDM_HW, int_channel_index, CY_PDM_PCM_INTR_RX_TRIGGER);
        handle->frame_counter++;
    }

    if (handle->frame_counter >= NUMBER_INTERRUPTS_FOR_FRAME)
    {
        int16_t *tmp = handle->active_rx_buffer;
        handle->active_rx_buffer = handle->full_rx_buffer;
        handle->full_rx_buffer = tmp;
        handle->have_data = true;
        handle->frame_counter = 0u;
    }

    if ((intr_status & (CY_PDM_PCM_INTR_RX_FIR_OVERFLOW |
                        CY_PDM_PCM_INTR_RX_OVERFLOW |
                        CY_PDM_PCM_INTR_RX_IF_OVERFLOW |
                        CY_PDM_PCM_INTR_RX_UNDERFLOW)) != 0u)
    {
        Cy_PDM_PCM_Channel_ClearInterrupt(CYBSP_PDM_HW, int_channel_index, CY_PDM_PCM_INTR_MASK);
    }
}

static void pdm_pcm_event_handler_0(void)
{
    pdm_pcm_event_handler_common(&g_mic_pool[0]);
}

static void pdm_pcm_event_handler_1(void)
{
    pdm_pcm_event_handler_common(&g_mic_pool[1]);
}

PDM_PCM_STATUS_t pdm_pcm_start(pdm_pcm handle)
{
    PDM_PCM_CONFIG_t *config;
    uint8_t int_channel_index;
    uint8_t i;
    cy_rslt_t result;
    cy_en_pdm_pcm_status_t pdm_status;

    if (handle == NULL || handle->config == NULL)
    {
        return PDM_PCM_STATUS_BAD_PARAM;
    }

    config = handle->config;
    int_channel_index = (uint8_t)(config->pdm_irq_cfg.intrSrc - pdm_0_interrupts_0_IRQn);

    (void)pdm_pcm_stop(handle);

    memset(handle->audio_buffer0, 0, sizeof(handle->audio_buffer0));
    memset(handle->audio_buffer1, 0, sizeof(handle->audio_buffer1));
    handle->active_rx_buffer = handle->audio_buffer0;
    handle->full_rx_buffer = handle->audio_buffer1;
    handle->have_data = false;
    handle->frame_counter = 0u;
    handle->init_discard_counter = 4;

    pdm_status = Cy_PDM_PCM_Init(CYBSP_PDM_HW, &CYBSP_PDM_config);
    if (pdm_status != CY_PDM_PCM_SUCCESS)
    {
        return PDM_PCM_STATUS_FAIL;
    }

    for (i = 0; i < (uint8_t)(config->mode + 1u); i++)
    {
        Cy_PDM_PCM_Channel_Enable(CYBSP_PDM_HW, config->channel_index_list[i]);
        Cy_PDM_PCM_Channel_Init(CYBSP_PDM_HW, &config->channel_config[i], config->channel_index_list[i]);
        Cy_PDM_PCM_SetGain(CYBSP_PDM_HW, config->channel_index_list[i], handle->gain[i]);
    }

    Cy_PDM_PCM_Channel_ClearInterrupt(CYBSP_PDM_HW, int_channel_index, CY_PDM_PCM_INTR_MASK);
    Cy_PDM_PCM_Channel_SetInterruptMask(CYBSP_PDM_HW, int_channel_index, CY_PDM_PCM_INTR_MASK);

    config->pdm_irq_cfg.intrPriority = PDM_PCM_ISR_PRIORITY;
    result = Cy_SysInt_Init(&config->pdm_irq_cfg, g_event_handlers[handle->index]);
    if (result != CY_SYSINT_SUCCESS)
    {
        return PDM_PCM_STATUS_FAIL;
    }

    NVIC_ClearPendingIRQ(config->pdm_irq_cfg.intrSrc);
    NVIC_EnableIRQ(config->pdm_irq_cfg.intrSrc);

    for (i = 0; i < (uint8_t)(config->mode + 1u); i++)
    {
        Cy_PDM_PCM_Activate_Channel(CYBSP_PDM_HW, config->channel_index_list[i]);
    }

    handle->initialized = true;
    return PDM_PCM_STATUS_SUCCESS;
}

PDM_PCM_STATUS_t pdm_pcm_stop(pdm_pcm handle)
{
    PDM_PCM_CONFIG_t *config;
    uint8_t i;

    if (handle == NULL || handle->config == NULL)
    {
        return PDM_PCM_STATUS_BAD_PARAM;
    }

    config = handle->config;

    if (handle->initialized)
    {
        for (i = 0; i < (uint8_t)(config->mode + 1u); i++)
        {
            Cy_PDM_PCM_DeActivate_Channel(CYBSP_PDM_HW, config->channel_index_list[i]);
        }

        NVIC_DisableIRQ(config->pdm_irq_cfg.intrSrc);
        handle->initialized = false;
    }

    return PDM_PCM_STATUS_SUCCESS;
}

bool pdm_pcm_data_ready(pdm_pcm handle)
{
    return (handle != NULL) ? handle->have_data : false;
}

void pdm_pcm_discard_samples(pdm_pcm handle)
{
    if (handle == NULL)
    {
        return;
    }

    if (handle->init_discard_counter > 0)
    {
        handle->init_discard_counter--;
        memset(handle->full_rx_buffer, 0, sizeof(handle->audio_buffer0));
    }
}

int16_t *pdm_pcm_get_full_buffer(pdm_pcm handle)
{
    return (handle != NULL) ? handle->full_rx_buffer : NULL;
}

const char **pdm_pcm_get_string_list_of_sample_rates(void)
{
    return g_sample_rate_labels;
}

uint8_t pdm_pcm_get_sample_rate_option_count(void)
{
    return (uint8_t)(SAMPLE_RATE_48000 + 1u);
}

const char **pdm_pcm_get_string_list_of_gain_options(void)
{
    return g_gain_labels;
}

uint8_t pdm_pcm_get_gain_option_count(void)
{
    return (uint8_t)(CY_PDM_PCM_SEL_GAIN_NEGATIVE_103DB + 1u);
}

int pdm_pcm_get_frequency_from_frequency_index(SAMPLE_RATE frequency_index)
{
    static const int freq_options[SAMPLE_RATE_48000 + 1] = {8000, 16000, 22050, 44100, 48000};

    if (frequency_index <= SAMPLE_RATE_48000)
    {
        return freq_options[frequency_index];
    }

    return freq_options[0];
}

void pdm_pcm_clear_data_ready_flag(pdm_pcm handle)
{
    if (handle != NULL)
    {
        handle->have_data = false;
    }
}

uint32_t pdm_pcm_get_frame_count(pdm_pcm handle)
{
    if (handle == NULL || handle->config == NULL)
    {
        return 0u;
    }

    return FRAME_SIZE / ((uint8_t)handle->config->mode + 1u);
}

#else

PDM_PCM_STATUS_t pdm_pcm_set_gain(pdm_pcm handle, uint8_t channel_num, cy_en_pdm_pcm_gain_sel_t gain)
{
    (void)handle;
    (void)channel_num;
    (void)gain;
    return PDM_PCM_STATUS_FAIL;
}

PDM_PCM_STATUS_t pdm_pcm_init_hw(SAMPLE_RATE sample_rate)
{
    (void)sample_rate;
    return PDM_PCM_STATUS_FAIL;
}

pdm_pcm pdm_pcm_create(PDM_PCM_CONFIG_t *config)
{
    (void)config;
    return NULL;
}

PDM_PCM_STATUS_t pdm_pcm_update_config(pdm_pcm handle, PDM_PCM_CONFIG_t *config)
{
    (void)handle;
    (void)config;
    return PDM_PCM_STATUS_FAIL;
}

PDM_PCM_STATUS_t pdm_pcm_start(pdm_pcm handle)
{
    (void)handle;
    return PDM_PCM_STATUS_FAIL;
}

PDM_PCM_STATUS_t pdm_pcm_stop(pdm_pcm handle)
{
    (void)handle;
    return PDM_PCM_STATUS_FAIL;
}

bool pdm_pcm_data_ready(pdm_pcm handle)
{
    (void)handle;
    return false;
}

void pdm_pcm_discard_samples(pdm_pcm handle)
{
    (void)handle;
}

int16_t *pdm_pcm_get_full_buffer(pdm_pcm handle)
{
    (void)handle;
    return NULL;
}

const char **pdm_pcm_get_string_list_of_sample_rates(void)
{
    return NULL;
}

uint8_t pdm_pcm_get_sample_rate_option_count(void)
{
    return 0u;
}

const char **pdm_pcm_get_string_list_of_gain_options(void)
{
    return NULL;
}

uint8_t pdm_pcm_get_gain_option_count(void)
{
    return 0u;
}

int pdm_pcm_get_frequency_from_frequency_index(SAMPLE_RATE frequency_index)
{
    (void)frequency_index;
    return 0;
}

void pdm_pcm_clear_data_ready_flag(pdm_pcm handle)
{
    (void)handle;
}

uint32_t pdm_pcm_get_frame_count(pdm_pcm handle)
{
    (void)handle;
    return 0u;
}

#endif
