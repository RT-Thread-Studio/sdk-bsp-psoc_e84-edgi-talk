#include <rtthread.h>
#include <rtdevice.h>
#include <rtconfig.h>

#include "cy_pdl.h"
#include "cybsp.h"

#include "drv_pdm.h"

#include <math.h>

#define DBG_TAG              "drv.mic"
#define DBG_LVL              DBG_INFO
#include <rtdbg.h>

/*******************************************************************************
* Macros
*******************************************************************************/
#ifdef ENABLE_STEREO_INPUT_FEED
#define MIC_MODE                         (2u)
#else
#define MIC_MODE                         (1u)
#endif /* ENABLE_STEREO_INPUT_FEED */
rt_device_t device_sound;

#define PDM_PCM_HW_FIFO_SIZE             (64u)

/* The PDM decimation chain is derived from the 12.288 MHz ECO clock:
 *   pdm_clk   = clk_if / (CLOCK_DIV + 1)          (PDM_PCM_CLOCK_CTL.CLOCK_DIV)
 *   rate/ch   = pdm_clk / (CIC * FIR0 * FIR1)      (channel config)
 * A delivered frame is 10 ms of audio. The per-rate parameters below
 * (samples_per_intr / intr_per_frame) keep the FIFO-trigger based ISR
 * running at a fixed 10 ms frame cadence for every supported rate.
 */
#define PDM_FRAME_MS                     (10u)

/* Maximum per-channel samples in one 10 ms frame (48 kHz -> 480) */
#define PDM_MAX_RX_SAMPLES_COUNT         (480u * MIC_MODE)
#define PDM_MAX_RX_FIFO_SIZE             (PDM_MAX_RX_SAMPLES_COUNT * 2u)

#define PDM_FIR1_GAIN_CONST              (13921L)
#define PDM_FIR_MAX_SCALE_VALUE          (31)
#define PDM_GAIN_DB_STEPS_CONST          (0.5f)
#define PDM_GAIN_CONVERT_CONST_2         (2)
#define PDM_GAIN_CONVERT_CONST_10        (10)
#define PDM_GAIN_CONVERT_CONST_20        (20)
#define PDM_SET_GAIN_ERROR               (-1)

/*******************************************************************************
* Data Structures
*******************************************************************************/

/* Per-rate hardware configuration (clock divider + decimation chain) and the
 * ISR frame parameters that must change together with the sample rate. */
typedef struct
{
    rt_uint32_t rate;                          /* per-channel sample rate, Hz */
    uint8_t     clk_div;                       /* PDM_PCM_CLOCK_CTL.CLOCK_DIV */
    cy_en_pdm_pcm_ch_cic_decimcode_t  cic;     /* CIC decimation code         */
    cy_en_pdm_pcm_ch_fir0_decimcode_t fir0;    /* FIR0 decimation code        */
    cy_en_pdm_pcm_ch_fir1_decimcode_t fir1;    /* FIR1 decimation code        */
    rt_uint16_t samples_per_intr;              /* per-channel FIFO reads per interrupt */
    rt_uint16_t intr_per_frame;                /* FIFO interrupts per 10 ms frame      */
    rt_uint16_t samples_per_ch_frame;          /* per-channel samples per 10 ms frame  */
} pdm_rate_cfg_t;

/* Rate table. Entries must satisfy:
 *   rate == clk_if / (clk_div+1) / (cic * fir0 * fir1)
 *   samples_per_intr * intr_per_frame == samples_per_ch_frame == rate/100
 */
static const pdm_rate_cfg_t pdm_rate_table[] =
{
    /* rate    clkDiv  CIC                       FIR0                      FIR1                       s/intr  intr/frame  s/ch/frame */
    {  8000u,  15u,    CY_PDM_PCM_CHAN_CIC_DECIM_32,  CY_PDM_PCM_CHAN_FIR0_DECIM_1,  CY_PDM_PCM_CHAN_FIR1_DECIM_3,  16u,   5u,   80u  },
    { 16000u,   7u,    CY_PDM_PCM_CHAN_CIC_DECIM_32,  CY_PDM_PCM_CHAN_FIR0_DECIM_1,  CY_PDM_PCM_CHAN_FIR1_DECIM_3,  32u,   5u,  160u  },
    { 24000u,   7u,    CY_PDM_PCM_CHAN_CIC_DECIM_32,  CY_PDM_PCM_CHAN_FIR0_DECIM_1,  CY_PDM_PCM_CHAN_FIR1_DECIM_2,  48u,   5u,  240u  },
    { 48000u,   7u,    CY_PDM_PCM_CHAN_CIC_DECIM_32,  CY_PDM_PCM_CHAN_FIR0_DECIM_1,  CY_PDM_PCM_CHAN_FIR1_DECIM_1,  32u,  15u,  480u  },
};
#define PDM_RATE_TABLE_SIZE ((uint32_t)(sizeof(pdm_rate_table) / sizeof(pdm_rate_table[0])))

/* Runtime-editable copies of the BSP PDM configs (the originals in
 * cycfg_peripherals.c are const and live in flash). */
static cy_stc_pdm_pcm_config_v2_t      pdm_runtime_cfg;
static cy_stc_pdm_pcm_channel_config_t ch_left_cfg;
static cy_stc_pdm_pcm_channel_config_t ch_right_cfg;

/*******************************************************************************
* Global Variables
*******************************************************************************/

/* Sized for the maximum supported rate (48 kHz, 10 ms frame). */
int16_t mic_audio_app_buffer_ping[PDM_MAX_RX_SAMPLES_COUNT] = {0};
int16_t mic_audio_app_buffer_pong[PDM_MAX_RX_SAMPLES_COUNT] = {0};

volatile uint8_t pdm_pcm_intr_cnt = 0;

int16_t *ping_pong_local_pointer = NULL;
int16_t *ping_pong_buffer_pointer = NULL;

const cy_stc_sysint_t PDM_IRQ_cfg =
{
    .intrSrc = (IRQn_Type)CYBSP_PDM_CHANNEL_3_IRQ,
    .intrPriority = PDM_PCM_INTR_PRIORITY
};
static inline int32_t convert_pdm_pcm_gain_to_scale(int16_t gain_val);

struct mic_device
{
    struct rt_audio_device audio;           /* RT-Thread audio device */
    struct rt_audio_configure record_config;/* Audio configuration (real values) */
    rt_uint8_t *rx_fifo;                    /* Receive FIFO buffer */
    rt_uint8_t volume;                      /* Volume level */
    rt_bool_t is_running;                   /* Running state */
    const pdm_rate_cfg_t *rate_cfg;         /* Current rate configuration */
    rt_uint16_t samples_per_intr;           /* Per-channel FIFO reads per interrupt */
    rt_uint16_t intr_per_frame;             /* FIFO interrupts per delivered frame */
    rt_uint16_t frame_bytes;                /* Bytes delivered per frame to rt_audio */
};

/* Static device instance */
static struct mic_device mic_dev = {0};

static const pdm_rate_cfg_t *pdm_rate_find(rt_uint32_t rate)
{
    uint32_t i;
    for (i = 0; i < PDM_RATE_TABLE_SIZE; i++)
    {
        if (pdm_rate_table[i].rate == rate)
        {
            return &pdm_rate_table[i];
        }
    }
    return RT_NULL;
}

void pdm_interrupt_handler(void)
{
    static bool ping_pong = false;
    volatile uint32_t int_stat;
    rt_uint16_t per_intr = mic_dev.samples_per_intr;

    rt_interrupt_enter();

    if (pdm_pcm_intr_cnt == 0)
    {
        if (ping_pong)
        {
            ping_pong_local_pointer = mic_audio_app_buffer_ping;
        }
        else
        {
            ping_pong_local_pointer = mic_audio_app_buffer_pong;
        }
        ping_pong_buffer_pointer = ping_pong_local_pointer;
    }

    int_stat = Cy_PDM_PCM_Channel_GetInterruptStatusMasked(PDM0, RIGHT_CH_INDEX);
    if (CY_PDM_PCM_INTR_RX_TRIGGER & int_stat)
    {
        for (rt_uint16_t i = 0; i < per_intr; i++)
        {
#ifdef ENABLE_STEREO_INPUT_FEED
            int32_t data = (int32_t)Cy_PDM_PCM_Channel_ReadFifo(PDM0, LEFT_CH_INDEX);
            *(ping_pong_buffer_pointer) = (int16_t)(data);
            ping_pong_buffer_pointer++;

            data = (int32_t)Cy_PDM_PCM_Channel_ReadFifo(PDM0, RIGHT_CH_INDEX);
            *(ping_pong_buffer_pointer) = (int16_t)(data);
            ping_pong_buffer_pointer++;
#else
            int32_t data = (int32_t)Cy_PDM_PCM_Channel_ReadFifo(PDM0, LEFT_CH_INDEX);
            data = (int32_t)Cy_PDM_PCM_Channel_ReadFifo(PDM0, RIGHT_CH_INDEX);
            *(ping_pong_buffer_pointer) = (int16_t)(data);
            ping_pong_buffer_pointer++;
#endif
        }

        if (pdm_pcm_intr_cnt < mic_dev.intr_per_frame)
        {
            pdm_pcm_intr_cnt++;
        }

        if (mic_dev.intr_per_frame == pdm_pcm_intr_cnt)
        {
            pdm_pcm_intr_cnt = 0;
            rt_memcpy(mic_dev.rx_fifo, ping_pong_local_pointer, mic_dev.frame_bytes);
            rt_audio_rx_done(&mic_dev.audio, mic_dev.rx_fifo, mic_dev.frame_bytes);
            ping_pong = !ping_pong;
            if (ping_pong)
            {
                ping_pong_local_pointer = mic_audio_app_buffer_ping;
            }
            else
            {
                ping_pong_local_pointer = mic_audio_app_buffer_pong;
            }
            ping_pong_buffer_pointer = ping_pong_local_pointer;
        }

        Cy_PDM_PCM_Channel_ClearInterrupt(PDM0, RIGHT_CH_INDEX, CY_PDM_PCM_INTR_RX_TRIGGER);
    }

    if ((CY_PDM_PCM_INTR_RX_FIR_OVERFLOW | CY_PDM_PCM_INTR_RX_OVERFLOW |
            CY_PDM_PCM_INTR_RX_IF_OVERFLOW | CY_PDM_PCM_INTR_RX_UNDERFLOW) & int_stat)
    {
        Cy_PDM_PCM_Channel_ClearInterrupt(PDM0, RIGHT_CH_INDEX, CY_PDM_PCM_INTR_MASK);
    }

    rt_interrupt_leave();
}

cy_rslt_t pdm_mic_interface_deinit(void)
{
    return CY_RSLT_SUCCESS;
}

void app_pdm_pcm_activate(void)
{
    /* Activate recording from channel after init Activate Channel */
    Cy_PDM_PCM_Activate_Channel(PDM0, LEFT_CH_INDEX);
    Cy_PDM_PCM_Activate_Channel(PDM0, RIGHT_CH_INDEX);
}

void app_pdm_pcm_deactivate(void)
{
    Cy_PDM_PCM_DeActivate_Channel(PDM0, LEFT_CH_INDEX);
    Cy_PDM_PCM_DeActivate_Channel(PDM0, RIGHT_CH_INDEX);
}

static inline int32_t convert_pdm_pcm_gain_to_scale(int16_t gain_val)
{
    /* The formula for gain in db is db = 20 * log_10(FIR1_GAIN / 2^scale),
    *  where FIR1_GAIN = 13921.
     * Solving for scale, we get: scale = log_2(FIR1_GAIN / 10^(db / 20))
     */

    /* Cmath only provides ln and log10, need to compute log_2 in terms of those */
    /* Gain is specified in 0.5 db steps in the interface */
    float desired_gain = ((float)gain_val) * PDM_GAIN_DB_STEPS_CONST;
    float inner_value = PDM_FIR1_GAIN_CONST / (powf(PDM_GAIN_CONVERT_CONST_10,
                        (desired_gain / PDM_GAIN_CONVERT_CONST_20)));
    float scale = logf(inner_value) / logf(PDM_GAIN_CONVERT_CONST_2);
    int32_t scale_rounded = (uint8_t)(scale + PDM_GAIN_DB_STEPS_CONST);
    return scale_rounded;
}

cy_rslt_t set_pdm_pcm_gain(int16_t gain)
{
    cy_rslt_t result = CY_RSLT_SUCCESS;
    cy_en_pdm_pcm_ch_fir1_decimcode_t decim_code;
    int32_t fir1_scale_value;

    if (gain < PDM_PCM_MIN_GAIN || gain > PDM_PCM_MAX_GAIN)
    {
        result = PDM_SET_GAIN_ERROR;
    }

    if (CY_RSLT_SUCCESS == result)
    {
        fir1_scale_value = convert_pdm_pcm_gain_to_scale(gain);
        if ((fir1_scale_value < 0) || (fir1_scale_value > PDM_FIR_MAX_SCALE_VALUE))
        {
            result = PDM_SET_GAIN_ERROR;
        }
    }

    if (CY_RSLT_SUCCESS == result)
    {
        /* Cy_PDM_PCM_Channel_Set_Fir1() programs both DECIM2 and SCALE, so use
         * the FIR1 decimation of the currently applied rate, otherwise setting
         * the gain would silently switch the decimation back to DECIM_3. */
        decim_code = (mic_dev.rate_cfg != RT_NULL) ?
                     mic_dev.rate_cfg->fir1 : CY_PDM_PCM_CHAN_FIR1_DECIM_3;

        Cy_PDM_PCM_Channel_Set_Fir1(PDM0, LEFT_CH_INDEX, decim_code,
                                    fir1_scale_value);
        Cy_PDM_PCM_Channel_Set_Fir1(PDM0, RIGHT_CH_INDEX, decim_code,
                                    fir1_scale_value);
    }

    return result;
}

/* Apply a new sample rate to the PDM hardware and to the ISR frame logic.
 * Must not be called from interrupt context. */
static rt_err_t pdm_set_rate(const pdm_rate_cfg_t *cfg)
{
    rt_err_t result = RT_EOK;
    rt_bool_t was_running;

    if (cfg == RT_NULL)
    {
        return -RT_EINVAL;
    }

    /* Remember whether capture is active: the audio framework starts the PDM
     * on device open (mic_start) BEFORE wavrecorder issues AUDIO_CTL_CONFIGURE,
     * so a rate change must re-activate the channels afterwards, otherwise no
     * data is delivered and the upper layer reads (and the record stop) block
     * forever. */
    was_running = mic_dev.is_running;

    /* Stop capture and block the PDM IRQ while the clock/decimators change. */
    app_pdm_pcm_deactivate();
    NVIC_DisableIRQ(PDM_IRQ_cfg.intrSrc);

    /* Start from the BSP defaults so sampledelay/signExtension/wordSize and
     * the FIR/DC-block coefficients are kept, then override the fields that
     * depend on the sample rate. */
    ch_left_cfg  = channel_2_config;
    ch_right_cfg = channel_3_config;

    pdm_runtime_cfg.clkDiv = cfg->clk_div;
    Cy_PDM_PCM_Init(PDM0, &pdm_runtime_cfg);

    ch_left_cfg.cic_decim_code       = cfg->cic;
    ch_left_cfg.fir0_decim_code      = cfg->fir0;
    ch_left_cfg.fir1_decim_code      = cfg->fir1;
    ch_left_cfg.rxFifoTriggerLevel   = (uint8_t)(cfg->samples_per_intr - 1u);

    ch_right_cfg.cic_decim_code      = cfg->cic;
    ch_right_cfg.fir0_decim_code     = cfg->fir0;
    ch_right_cfg.fir1_decim_code     = cfg->fir1;
    ch_right_cfg.rxFifoTriggerLevel  = (uint8_t)(cfg->samples_per_intr - 1u);

    Cy_PDM_PCM_Channel_Init(PDM0, &ch_left_cfg, LEFT_CH_INDEX);
    Cy_PDM_PCM_Channel_Init(PDM0, &ch_right_cfg, RIGHT_CH_INDEX);

    /* Update the runtime frame parameters used by the ISR. */
    mic_dev.rate_cfg         = cfg;
    mic_dev.samples_per_intr = cfg->samples_per_intr;
    mic_dev.intr_per_frame   = cfg->intr_per_frame;
    mic_dev.frame_bytes      = (rt_uint16_t)(cfg->samples_per_ch_frame * MIC_MODE * 2u);
    mic_dev.is_running       = RT_FALSE;
    pdm_pcm_intr_cnt         = 0;

    /* Re-apply the user volume: Channel_Init() above restored the default
     * FIR1 scale, and Set_Fir1() also (re)writes the DECIM2 code. */
    set_pdm_pcm_gain(mic_dev.volume);

    NVIC_ClearPendingIRQ(PDM_IRQ_cfg.intrSrc);
    NVIC_EnableIRQ(PDM_IRQ_cfg.intrSrc);

    /* If capture was active before the rate change, resume it so the ISR keeps
     * delivering frames at the new rate. */
    if (was_running)
    {
        app_pdm_pcm_activate();
        mic_dev.is_running = RT_TRUE;
    }

    LOG_D("PDM set rate: %d Hz, clkDiv=%d, CIC=%d FIR0=%d FIR1=%d, frame=%d bytes",
          cfg->rate, cfg->clk_div, (int)cfg->cic, (int)cfg->fir0, (int)cfg->fir1,
          mic_dev.frame_bytes);

    return result;
}

static rt_err_t mic_getcaps(struct rt_audio_device *audio, struct rt_audio_caps *caps)
{
    rt_err_t result = RT_EOK;
    struct mic_device *mic_dev = (struct mic_device *)audio->parent.user_data;

    switch (caps->main_type)
    {
    case AUDIO_TYPE_QUERY:
        switch (caps->sub_type)
        {
        case AUDIO_TYPE_QUERY:
            caps->udata.mask = AUDIO_TYPE_INPUT | AUDIO_TYPE_MIXER;
            break;
        default:
            result = -RT_ERROR;
            break;
        }
        break;

    case AUDIO_TYPE_INPUT:
        switch (caps->sub_type)
        {
        case AUDIO_DSP_PARAM:
            caps->udata.config.samplerate = mic_dev->record_config.samplerate;
            caps->udata.config.channels   = mic_dev->record_config.channels;
            caps->udata.config.samplebits = mic_dev->record_config.samplebits;
            break;
        case AUDIO_DSP_SAMPLERATE:
            caps->udata.config.samplerate = mic_dev->record_config.samplerate;
            break;
        case AUDIO_DSP_CHANNELS:
            caps->udata.config.channels   = mic_dev->record_config.channels;
            break;
        case AUDIO_DSP_SAMPLEBITS:
            caps->udata.config.samplebits = mic_dev->record_config.samplebits;
            break;
        default:
            result = -RT_ERROR;
            break;
        }
        break;

    case AUDIO_TYPE_MIXER:
        switch (caps->sub_type)
        {
        case AUDIO_MIXER_VOLUME:
            caps->udata.value = mic_dev->volume;
            break;
        default:
            result = -RT_ERROR;
            break;
        }
        break;

    default:
        result = -RT_ERROR;
        break;
    }

    return result;
}

static rt_err_t mic_configure(struct rt_audio_device *audio, struct rt_audio_caps *caps)
{
    rt_err_t result = RT_EOK;
    struct mic_device *mic_dev = (struct mic_device *)audio->parent.user_data;
    switch (caps->main_type)
    {
    case AUDIO_TYPE_INPUT:
        switch (caps->sub_type)
        {
        case AUDIO_DSP_PARAM:
        {
            const pdm_rate_cfg_t *cfg;

            /* Sample rate: really reconfigure the hardware. Only the rates in
             * pdm_rate_table are supported; anything else keeps the current
             * applied rate and is reported back truthfully via getcaps. */
            cfg = pdm_rate_find(caps->udata.config.samplerate);
            if (cfg == RT_NULL)
            {
                LOG_W("unsupported samplerate %d Hz, keep %d Hz",
                      caps->udata.config.samplerate, mic_dev->record_config.samplerate);
                caps->udata.config.samplerate = mic_dev->record_config.samplerate;
            }
            else if (cfg != mic_dev->rate_cfg)
            {
                if (pdm_set_rate(cfg) != RT_EOK)
                {
                    LOG_E("apply samplerate %d Hz failed", cfg->rate);
                    caps->udata.config.samplerate = mic_dev->record_config.samplerate;
                }
                else
                {
                    mic_dev->record_config.samplerate = cfg->rate;
                }
            }

            /* Channels are fixed by the hardware layout (stereo when
             * ENABLE_STEREO_INPUT_FEED, mono otherwise) and cannot change. */
#ifdef ENABLE_STEREO_INPUT_FEED
            if (caps->udata.config.channels != 2)
            {
                LOG_W("PDM fixed at 2 channels, ignore requested %d channel(s)",
                      caps->udata.config.channels);
            }
            caps->udata.config.channels = 2;
#else
            if (caps->udata.config.channels != 1)
            {
                LOG_W("PDM fixed at 1 channel, ignore requested %d channel(s)",
                      caps->udata.config.channels);
            }
            caps->udata.config.channels = 1;
#endif /* ENABLE_STEREO_INPUT_FEED */

            if (caps->udata.config.samplebits != 16)
            {
                LOG_W("PDM fixed at 16 bits, ignore requested %d bits",
                      caps->udata.config.samplebits);
            }
            caps->udata.config.samplebits = 16;

            /* Save the real hardware configuration */
            mic_dev->record_config.channels   = caps->udata.config.channels;
            mic_dev->record_config.samplebits = caps->udata.config.samplebits;

            LOG_D("Set samplerate: %d, channels: %d, samplebits: %d",
                  mic_dev->record_config.samplerate, mic_dev->record_config.channels,
                  mic_dev->record_config.samplebits);
            break;
        }

        default:
            result = -RT_ERROR;
            break;
        }
        break;

    case AUDIO_TYPE_MIXER:
        switch (caps->sub_type)
        {
        case AUDIO_MIXER_VOLUME:
            mic_dev->volume = caps->udata.value;
            set_pdm_pcm_gain(mic_dev->volume);
            LOG_D("Set volume: %d\r\n", mic_dev->volume);
            break;
        default:
            result = -RT_ERROR;
            break;
        }
        break;

    default:
        result = -RT_ERROR;
        break;
    }

    return result;
}

static rt_err_t mic_init(struct rt_audio_device *audio)
{
    uint32_t i;
    const pdm_rate_cfg_t *default_cfg = RT_NULL;

    /* Keep runtime-editable copies of the BSP PDM configs. */
    pdm_runtime_cfg = CYBSP_PDM_config;
    ch_left_cfg     = channel_2_config;
    ch_right_cfg    = channel_3_config;

    /* Default to SAMPLE_RATE_HZ (16 kHz) and program the hardware. */
    for (i = 0; i < PDM_RATE_TABLE_SIZE; i++)
    {
        if (pdm_rate_table[i].rate == SAMPLE_RATE_HZ)
        {
            default_cfg = &pdm_rate_table[i];
            break;
        }
    }
    if (default_cfg == RT_NULL)
    {
        default_cfg = &pdm_rate_table[0];
    }
    pdm_set_rate(default_cfg);

    /* Enable PDM channels, they are activated for recording on mic_start. */
    Cy_PDM_PCM_Channel_Enable(PDM0, LEFT_CH_INDEX);
    Cy_PDM_PCM_Channel_Enable(PDM0, RIGHT_CH_INDEX);

    /* As registered for the right channel, clear and set the mask for it. */
    Cy_PDM_PCM_Channel_ClearInterrupt(PDM0, RIGHT_CH_INDEX, CY_PDM_PCM_INTR_MASK);
    Cy_PDM_PCM_Channel_SetInterruptMask(PDM0, RIGHT_CH_INDEX, CY_PDM_PCM_INTR_MASK);

    /* Register the PDM/PCM hardware block IRQ handler */
    if (CY_SYSINT_SUCCESS != Cy_SysInt_Init(&PDM_IRQ_cfg, &pdm_interrupt_handler))
    {
        LOG_D("PDM/PCM Initialization has failed! \r\n");
        CY_ASSERT(0);
    }

    LOG_D("PDM/PCM Initialization Successful \r\n");
    NVIC_ClearPendingIRQ(PDM_IRQ_cfg.intrSrc);
    NVIC_EnableIRQ(PDM_IRQ_cfg.intrSrc);

    return RT_EOK;
}

static rt_err_t mic_start(struct rt_audio_device *audio, int stream)
{
    struct mic_device *mic_dev = (struct mic_device *)audio->parent.user_data;

    if (stream == AUDIO_STREAM_RECORD && !mic_dev->is_running)
    {
        app_pdm_pcm_activate();
        rt_audio_rx_done(&mic_dev->audio, mic_dev->rx_fifo, 0);
        mic_dev->is_running = RT_TRUE;
        LOG_D("Microphone started");
    }

    return RT_EOK;
}

static rt_err_t mic_stop(struct rt_audio_device *audio, int stream)
{
    struct mic_device *mic_dev = (struct mic_device *)audio->parent.user_data;

    if (stream == AUDIO_STREAM_RECORD && mic_dev->is_running)
    {
        app_pdm_pcm_deactivate();
        mic_dev->is_running = RT_FALSE;
        LOG_D("Microphone stopped");
    }

    return RT_EOK;
}

static struct rt_audio_ops mic_ops =
{
    .getcaps     = mic_getcaps,
    .configure   = mic_configure,
    .init        = mic_init,
    .start       = mic_start,
    .stop        = mic_stop,
    .transmit    = RT_NULL,
    .buffer_info = RT_NULL,
};

int rt_hw_pdm_init(void)
{
    rt_uint8_t *rx_fifo;

    rx_fifo = rt_malloc(PDM_MAX_RX_FIFO_SIZE);
    if (rx_fifo == RT_NULL)
    {
        LOG_E("Failed to allocate RX FIFO");
        return -RT_ENOMEM;
    }

    rt_memset(rx_fifo, 0, PDM_MAX_RX_FIFO_SIZE);

    mic_dev.rx_fifo = rx_fifo;

    /* Set default configuration */
    mic_dev.record_config.samplerate = SAMPLE_RATE_HZ;
#ifdef ENABLE_STEREO_INPUT_FEED
    mic_dev.record_config.channels   = 2;
#else
    mic_dev.record_config.channels   = 1;
#endif /* ENABLE_STEREO_INPUT_FEED */
    mic_dev.record_config.samplebits = 16;
    mic_dev.volume                   = 40; /* gain */
    mic_dev.is_running               = RT_FALSE;
    /* 16 kHz defaults; updated by mic_init()/mic_configure() before the ISR
     * can run. */
    mic_dev.rate_cfg         = RT_NULL;
    mic_dev.samples_per_intr = 32;
    mic_dev.intr_per_frame   = 5;
    mic_dev.frame_bytes      = 160u * MIC_MODE * 2u;

    mic_dev.audio.ops = &mic_ops;
    LOG_I("audio pdm registered.\n");
    LOG_I("!!!Note: pdm depends on i2s0, they share clock.\n");
    rt_audio_register(&mic_dev.audio, "mic0", RT_DEVICE_FLAG_RDONLY, &mic_dev);

    return RT_EOK;
}
INIT_DEVICE_EXPORT(rt_hw_pdm_init);
