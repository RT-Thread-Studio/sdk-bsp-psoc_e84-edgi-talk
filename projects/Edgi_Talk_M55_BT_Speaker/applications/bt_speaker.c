/*
 * Edgi-Talk Bluetooth speaker (A2DP Sink + AVRCP Target).
 *
 * The CYW55512 Bluetooth core is connected to SCB4 ("uart4").  BTstack
 * downloads the CYW55500A1 patch RAM, switches the UART to 3 Mbps, decodes
 * incoming SBC audio and feeds interleaved stereo PCM to the ES8388 codec
 * through RT-Thread's sound0 device.
 */

#include <rtthread.h>
#include <rtdevice.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "board.h"
#include "cy_scb_uart.h"

#include "btstack.h"
#include "btstack_audio.h"
#include "btstack_resample.h"
#include "btstack_ring_buffer.h"
#include "hci_transport.h"
#include "btstack_chipset_bcm.h"
#include "btstack_chipset_bcm_download_firmware.h"
#include "classic/btstack_link_key_db_memory.h"

#include "btstack_run_loop_rtt.h"
#include "btstack_uart_block_rtt.h"
#include "btstack_audio_rtt.h"

extern const char brcm_patch_version[];
extern const int brcm_patch_ram_length;

static uint32_t last_download_percent;

static void bt_download_progress(uint32_t offset, uint32_t total)
{
    uint32_t percent = total ? (offset * 100u) / total : 0;

    if (percent >= last_download_percent + 10u || offset >= total)
    {
        last_download_percent = percent;
        rt_kprintf("[bt] firmware download %u%% (%u/%u)\n",
                   (unsigned)percent, (unsigned)offset, (unsigned)total);
    }
}

#define BT_SPEAKER_NAME          "Edgi-Talk BT Speaker"
#define BT_UART_DEVICE_NAME      "uart4"
#define BT_UART_BAUDRATE_INIT    115200
#define BT_UART_BAUDRATE_MAIN    3000000
#define BT_REG_ON_PIN            GET_PIN(CYBSP_BT_POWER_PORT_NUM, CYBSP_BT_POWER_PIN)
#define BT_REG_ON_DELAY_MS       1000

#define BT_THREAD_STACK_SIZE     12288
#define BT_THREAD_PRIORITY       8

#define NUM_CHANNELS             2
#define BYTES_PER_FRAME          (2 * NUM_CHANNELS)
/*
 * 16 blocks * 8 subbands * 2 channels * bitpool 53 gives a 428-byte SBC
 * frame.  Keep some headroom so a high-bitpool phone cannot overrun the
 * stack buffer used while draining the ring buffer.
 */
#define MAX_SBC_FRAME_SIZE       512

/*
 * PCM produced by the SBC decoder is queued here and drained by the audio
 * timer.  8192 stereo frames is about 170 ms at 48 kHz, enough to absorb
 * the audio writer's scheduling jitter without stalling the decoder.
 */
#define PCM_FIFO_FRAMES          8192

#define BT_SOURCE_SAMPLE_RATE    48000

typedef struct
{
    int reconfigure;
    int num_channels;
    int sampling_frequency;
    int channel_mode;
    int block_length;
    int subbands;
    int allocation_method;
    int min_bitpool_value;
    int max_bitpool_value;
    int frames_per_buffer;
} avdtp_media_codec_configuration_sbc_t;

static avdtp_media_codec_configuration_sbc_t sbc_configuration;

static btstack_sbc_decoder_state_t sbc_decoder_state;
static btstack_sbc_mode_t sbc_mode = SBC_MODE_STANDARD;

static uint8_t decoded_audio_storage[PCM_FIFO_FRAMES * BYTES_PER_FRAME];
static btstack_ring_buffer_t decoded_audio_ring_buffer;
static unsigned int sbc_frame_size;

static btstack_resample_t resample_instance;
static uint32_t nominal_resample_factor = 0x10000;

static int audio_stream_started;
static int media_initialized;

static uint8_t sdp_avdtp_sink_service_buffer[150];
static uint8_t sdp_avrcp_target_service_buffer[150];
static uint8_t sdp_avrcp_controller_service_buffer[200];

static uint16_t a2dp_cid;
static uint8_t a2dp_local_seid;
static uint16_t avrcp_cid;
static uint8_t avrcp_connected;
static uint8_t avrcp_subevent_value[100];
static int volume_percentage;

static bd_addr_t device_addr;
static btstack_packet_callback_registration_t hci_event_callback_registration;

static rt_thread_t bt_thread;
static volatile int bt_started;
static int bt_ever_started;
static int baud_command_sent;
static volatile int stop_requested;
static volatile int name_requested;
static char pending_name[32];

static uint8_t media_sbc_codec_capabilities[] = {
    /* 44.1 kHz + 48 kHz, stereo + joint stereo. */
    ((AVDTP_SBC_48000 | AVDTP_SBC_44100) << 4) | (AVDTP_SBC_STEREO | AVDTP_SBC_JOINT_STEREO),
    (AVDTP_SBC_BLOCK_LENGTH_16 << 4) | (AVDTP_SBC_SUBBANDS_8 << 2) | AVDTP_SBC_ALLOCATION_METHOD_LOUDNESS,
    2, 53
};

static uint8_t media_sbc_codec_configuration[] = {
    (AVDTP_SBC_44100 << 4) | AVDTP_SBC_STEREO,
    (AVDTP_SBC_BLOCK_LENGTH_16 << 4) | (AVDTP_SBC_SUBBANDS_8 << 2) | AVDTP_SBC_ALLOCATION_METHOD_LOUDNESS,
    2, 53
};

static void hci_packet_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size);
static void a2dp_sink_packet_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size);
static void handle_l2cap_media_data_packet(uint8_t seid, uint8_t *packet, uint16_t size);
static void avrcp_packet_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size);
static void avrcp_controller_packet_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size);
static void avrcp_target_packet_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size);

/* ------------------------------------------------------------------------- */
/* SBC decoding and PCM playback                                             */
/* ------------------------------------------------------------------------- */

static uint32_t pcm_overflow_count;

/*
 * Called by the audio timer.  Decoding already happened when the media
 * packet arrived, so this only drains the PCM FIFO (and inserts silence
 * on underrun).  A late audio writer therefore cannot stall SBC decoding.
 */
static void playback_handler(int16_t *buffer, uint16_t num_audio_frames)
{
    uint32_t request = (uint32_t)num_audio_frames * BYTES_PER_FRAME;
    uint32_t bytes_read;

    btstack_ring_buffer_read(&decoded_audio_ring_buffer, (uint8_t *)buffer,
                             request, &bytes_read);
    if (bytes_read < request)
    {
        rt_memset((uint8_t *)buffer + bytes_read, 0, request - bytes_read);
    }
}

static void handle_pcm_data(int16_t *data, int num_audio_frames, int num_channels,
                            int sample_rate, void *context)
{
    int16_t output_buffer[(256 + 64) * NUM_CHANNELS];
    uint32_t resampled_frames;
    int status;

    (void)sample_rate;
    (void)context;
    (void)num_channels; /* must be stereo */

    if (num_audio_frames > 256)
    {
        num_audio_frames = 256;
    }

    resampled_frames = btstack_resample_block(&resample_instance, data, num_audio_frames,
                                              output_buffer);

    status = btstack_ring_buffer_write(&decoded_audio_ring_buffer,
                                       (uint8_t *)output_buffer,
                                       resampled_frames * BYTES_PER_FRAME);
    if (status)
    {
        if (pcm_overflow_count++ == 0)
        {
            rt_kprintf("[bt] PCM FIFO overflow\n");
        }
    }
}

static int media_processing_init(avdtp_media_codec_configuration_sbc_t configuration)
{
    const btstack_audio_sink_t *audio;

    if (media_initialized)
    {
        return 0;
    }

    btstack_sbc_decoder_init(&sbc_decoder_state, sbc_mode, handle_pcm_data, NULL);
    btstack_ring_buffer_init(&decoded_audio_ring_buffer, decoded_audio_storage, sizeof(decoded_audio_storage));
    btstack_resample_init(&resample_instance, configuration.num_channels);

    if (configuration.sampling_frequency > 0)
    {
        /*
         * btstack_resample's factor is input-sample step per output frame:
         * 44100 / 48000 = 0.91875, i.e. 128 SBC samples produce ~139 frames.
         */
        nominal_resample_factor = ((uint32_t)configuration.sampling_frequency << 16) /
                                  (uint32_t)BT_SOURCE_SAMPLE_RATE;
    }
    else
    {
        nominal_resample_factor = 0x10000;
    }

    audio = btstack_audio_sink_get_instance();
    if (audio == NULL || audio->init(NUM_CHANNELS, BT_SOURCE_SAMPLE_RATE,
                                     &playback_handler) == 0)
    {
        rt_kprintf("[bt] audio sink init failed\n");
        return -1;
    }

    audio_stream_started = 0;
    media_initialized = 1;
    return 0;
}

static void media_processing_start(void)
{
    const btstack_audio_sink_t *audio;

    if (!media_initialized)
    {
        return;
    }

    audio = btstack_audio_sink_get_instance();
    if (audio != NULL)
    {
        audio->start_stream();
    }
    audio_stream_started = 1;
}

static void media_processing_pause(void)
{
    const btstack_audio_sink_t *audio;

    if (!media_initialized)
    {
        return;
    }

    audio_stream_started = 0;
    audio = btstack_audio_sink_get_instance();
    if (audio != NULL)
    {
        audio->stop_stream();
    }
}

static void media_processing_close(void)
{
    const btstack_audio_sink_t *audio;

    if (!media_initialized)
    {
        return;
    }

    media_initialized = 0;
    audio_stream_started = 0;
    sbc_frame_size = 0;

    audio = btstack_audio_sink_get_instance();
    if (audio != NULL)
    {
        audio->close();
    }
}

static uint32_t resampling_factor_with_drift(uint32_t nominal, int pcm_frames_in_buffer)
{
    const uint32_t compensation = 0x00100;

    if (pcm_frames_in_buffer < (PCM_FIFO_FRAMES / 4))
    {
        /* FIFO low: produce more PCM per SBC frame. */
        return nominal - compensation;
    }
    if (pcm_frames_in_buffer <= (PCM_FIFO_FRAMES / 2))
    {
        return nominal;
    }
    /* FIFO high: produce less PCM per SBC frame. */
    return nominal + compensation;
}

static int read_media_data_header(uint8_t *packet, int size, int *offset,
                                  avdtp_media_packet_header_t *media_header)
{
    int media_header_len = 12; /* without CRC */
    int pos = *offset;

    if (size - pos < media_header_len)
    {
        return 0;
    }

    media_header->version = packet[pos] & 0x03;
    media_header->padding = get_bit16(packet[pos], 2);
    media_header->extension = get_bit16(packet[pos], 3);
    media_header->csrc_count = (packet[pos] >> 4) & 0x0F;
    pos++;

    media_header->marker = get_bit16(packet[pos], 0);
    media_header->payload_type = (packet[pos] >> 1) & 0x7F;
    pos++;

    media_header->sequence_number = big_endian_read_16(packet, pos);
    pos += 2;

    media_header->timestamp = big_endian_read_32(packet, pos);
    pos += 4;

    media_header->synchronization_source = big_endian_read_32(packet, pos);
    pos += 4;

    *offset = pos;
    return 1;
}

static int read_sbc_header(uint8_t *packet, int size, int *offset,
                           avdtp_sbc_codec_header_t *sbc_header)
{
    int sbc_header_len = 12; /* without CRC */
    int pos = *offset;

    if (size - pos < sbc_header_len)
    {
        return 0;
    }

    sbc_header->fragmentation = get_bit16(packet[pos], 7);
    sbc_header->starting_packet = get_bit16(packet[pos], 6);
    sbc_header->last_packet = get_bit16(packet[pos], 5);
    sbc_header->num_frames = packet[pos] & 0x0f;
    pos++;
    *offset = pos;
    return 1;
}

static void handle_l2cap_media_data_packet(uint8_t seid, uint8_t *packet, uint16_t size)
{
    avdtp_media_packet_header_t media_header;
    avdtp_sbc_codec_header_t sbc_header;
    const btstack_audio_sink_t *audio;
    int pcm_frames_in_buffer;
    int pos = 0;
    int i;

    (void)seid;

    if (!read_media_data_header(packet, size, &pos, &media_header))
    {
        return;
    }
    if (!read_sbc_header(packet, size, &pos, &sbc_header))
    {
        return;
    }
    if (sbc_header.num_frames == 0)
    {
        return;
    }

    if (!media_initialized)
    {
        return;
    }

    audio = btstack_audio_sink_get_instance();
    if (audio == NULL)
    {
        return;
    }

    sbc_frame_size = (size - pos) / sbc_header.num_frames;
    if (sbc_frame_size == 0)
    {
        return;
    }
    if (sbc_frame_size > MAX_SBC_FRAME_SIZE)
    {
        rt_kprintf("[bt] SBC frame too large: %u\n", (unsigned)sbc_frame_size);
        return;
    }

    /*
     * Decode on the media path, not in the audio timer.  The decoded PCM
     * goes into a 170 ms FIFO, so a late audio writer cannot cause SBC
     * packet loss.
     */
    pcm_frames_in_buffer = btstack_ring_buffer_bytes_available(&decoded_audio_ring_buffer) /
                           BYTES_PER_FRAME;
    btstack_resample_set_factor(&resample_instance,
                                resampling_factor_with_drift(nominal_resample_factor,
                                                             pcm_frames_in_buffer));

    for (i = 0; i < sbc_header.num_frames; i++)
    {
        btstack_sbc_decoder_process_data(&sbc_decoder_state, 0,
                                         packet + pos + (uint32_t)i * sbc_frame_size,
                                         (uint16_t)sbc_frame_size);
    }

    if (!audio_stream_started && pcm_frames_in_buffer >= 1024)
    {
        audio_stream_started = 1;
        audio->start_stream();
    }
}

/* ------------------------------------------------------------------------- */
/* Bluetooth event handling                                                  */
/* ------------------------------------------------------------------------- */

static void hci_packet_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size)
{
    (void)channel;
    (void)size;

    if (packet_type != HCI_EVENT_PACKET)
    {
        return;
    }

    switch (hci_event_packet_get_type(packet))
    {
    case BTSTACK_EVENT_STATE:
        if (btstack_event_state_get_state(packet) == HCI_STATE_WORKING)
        {
            bd_addr_t local_addr;

            gap_local_bd_addr(local_addr);
            rt_kprintf("[bt] stack up, local address %s\n", bd_addr_to_str(local_addr));
            gap_discoverable_control(1);

            /*
             * BTstack only runs its automatic BCM baud switch for specific
             * manufacturer IDs.  Explicitly switch to 3 Mbps once the stack is
             * idle; the command completes at the old baud rate and the event
             * handler below then moves the host UART.
             */
            if (!baud_command_sent &&
                btstack_uart_block_rtt_get_baudrate() != BT_UART_BAUDRATE_MAIN)
            {
                uint8_t command[9] = {
                    0x18, 0xFC, 0x06, 0x00, 0x00,
                    (uint8_t)(BT_UART_BAUDRATE_MAIN & 0xFF),
                    (uint8_t)((BT_UART_BAUDRATE_MAIN >> 8) & 0xFF),
                    (uint8_t)((BT_UART_BAUDRATE_MAIN >> 16) & 0xFF),
                    (uint8_t)((BT_UART_BAUDRATE_MAIN >> 24) & 0xFF)
                };

                baud_command_sent = 1;
                hci_send_cmd_packet(command, sizeof(command));
            }
        }
        break;

    case HCI_EVENT_COMMAND_COMPLETE:
        if (hci_event_command_complete_get_command_opcode(packet) == 0xFC18 &&
            btstack_uart_block_rtt_get_baudrate() != BT_UART_BAUDRATE_MAIN)
        {
            btstack_uart_block_rtt_instance()->set_baudrate(BT_UART_BAUDRATE_MAIN);
            rt_kprintf("[bt] host UART switched to %d baud\n", BT_UART_BAUDRATE_MAIN);
        }
        break;

    case HCI_EVENT_PIN_CODE_REQUEST:
    {
        bd_addr_t address;

        rt_kprintf("[bt] PIN code request, replying 0000\n");
        hci_event_pin_code_request_get_bd_addr(packet, address);
        gap_pin_code_response(address, "0000");
        break;
    }

    default:
        break;
    }
}

static void a2dp_sink_packet_handler(uint8_t packet_type, uint16_t channel,
                                     uint8_t *packet, uint16_t size)
{
    bd_addr_t address;
    uint8_t status;

    (void)channel;
    (void)size;

    if (packet_type != HCI_EVENT_PACKET)
    {
        return;
    }
    if (hci_event_packet_get_type(packet) != HCI_EVENT_A2DP_META)
    {
        return;
    }

    switch (packet[2])
    {
    case A2DP_SUBEVENT_SIGNALING_MEDIA_CODEC_OTHER_CONFIGURATION:
        rt_kprintf("[bt] non-SBC codec is not supported\n");
        break;

    case A2DP_SUBEVENT_SIGNALING_MEDIA_CODEC_SBC_CONFIGURATION:
        sbc_configuration.reconfigure =
            a2dp_subevent_signaling_media_codec_sbc_configuration_get_reconfigure(packet);
        sbc_configuration.num_channels =
            a2dp_subevent_signaling_media_codec_sbc_configuration_get_num_channels(packet);
        sbc_configuration.sampling_frequency =
            a2dp_subevent_signaling_media_codec_sbc_configuration_get_sampling_frequency(packet);
        sbc_configuration.channel_mode =
            a2dp_subevent_signaling_media_codec_sbc_configuration_get_channel_mode(packet);
        sbc_configuration.block_length =
            a2dp_subevent_signaling_media_codec_sbc_configuration_get_block_length(packet);
        sbc_configuration.subbands =
            a2dp_subevent_signaling_media_codec_sbc_configuration_get_subbands(packet);
        sbc_configuration.allocation_method =
            a2dp_subevent_signaling_media_codec_sbc_configuration_get_allocation_method(packet);
        sbc_configuration.min_bitpool_value =
            a2dp_subevent_signaling_media_codec_sbc_configuration_get_min_bitpool_value(packet);
        sbc_configuration.max_bitpool_value =
            a2dp_subevent_signaling_media_codec_sbc_configuration_get_max_bitpool_value(packet);
        sbc_configuration.frames_per_buffer =
            sbc_configuration.subbands * sbc_configuration.block_length;

        rt_kprintf("[bt] SBC: %d Hz, %d ch, bitpool %d..%d\n",
                   sbc_configuration.sampling_frequency,
                   sbc_configuration.num_channels,
                   sbc_configuration.min_bitpool_value,
                   sbc_configuration.max_bitpool_value);

        if (sbc_configuration.reconfigure)
        {
            media_processing_close();
        }
        media_processing_init(sbc_configuration);
        break;

    case A2DP_SUBEVENT_STREAM_ESTABLISHED:
        a2dp_subevent_stream_established_get_bd_addr(packet, address);
        status = a2dp_subevent_stream_established_get_status(packet);
        if (status != 0)
        {
            rt_kprintf("[bt] A2DP stream failed, status 0x%02x\n", status);
            break;
        }
        a2dp_cid = a2dp_subevent_stream_established_get_a2dp_cid(packet);
        rt_memcpy(device_addr, address, sizeof(bd_addr_t));
        rt_kprintf("[bt] A2DP connected: %s, cid 0x%02x, seid %d\n",
                   bd_addr_to_str(address), a2dp_cid, a2dp_local_seid);
        break;

    case A2DP_SUBEVENT_STREAM_STARTED:
        rt_kprintf("[bt] stream started\n");
        media_processing_start();
        break;

    case A2DP_SUBEVENT_STREAM_SUSPENDED:
        rt_kprintf("[bt] stream suspended\n");
        media_processing_pause();
        break;

    case A2DP_SUBEVENT_STREAM_RELEASED:
        rt_kprintf("[bt] stream released\n");
        media_processing_close();
        break;

    case A2DP_SUBEVENT_SIGNALING_CONNECTION_RELEASED:
        rt_kprintf("[bt] signaling connection released\n");
        media_processing_close();
        break;

    default:
        break;
    }
}

static void avrcp_volume_changed(uint8_t volume)
{
    const btstack_audio_sink_t *audio = btstack_audio_sink_get_instance();

    if (audio != NULL)
    {
        audio->set_volume(volume);
    }
}

static void avrcp_packet_handler(uint8_t packet_type, uint16_t channel,
                                 uint8_t *packet, uint16_t size)
{
    bd_addr_t address;
    uint8_t status;

    (void)channel;
    (void)size;

    if (packet_type != HCI_EVENT_PACKET)
    {
        return;
    }
    if (hci_event_packet_get_type(packet) != HCI_EVENT_AVRCP_META)
    {
        return;
    }

    switch (packet[2])
    {
    case AVRCP_SUBEVENT_CONNECTION_ESTABLISHED:
        status = avrcp_subevent_connection_established_get_status(packet);
        if (status != ERROR_CODE_SUCCESS)
        {
            rt_kprintf("[bt] AVRCP connection failed, status 0x%02x\n", status);
            avrcp_cid = 0;
            avrcp_connected = 0;
            break;
        }
        avrcp_cid = avrcp_subevent_connection_established_get_avrcp_cid(packet);
        avrcp_connected = 1;
        avrcp_subevent_connection_established_get_bd_addr(packet, address);
        rt_kprintf("[bt] AVRCP connected: %s\n", bd_addr_to_str(address));

        avrcp_controller_enable_notification(avrcp_cid,
                                             AVRCP_NOTIFICATION_EVENT_VOLUME_CHANGED);
        avrcp_controller_enable_notification(avrcp_cid,
                                             AVRCP_NOTIFICATION_EVENT_PLAYBACK_STATUS_CHANGED);
        break;

    case AVRCP_SUBEVENT_CONNECTION_RELEASED:
        rt_kprintf("[bt] AVRCP released\n");
        avrcp_cid = 0;
        avrcp_connected = 0;
        break;

    default:
        break;
    }
}

static void avrcp_controller_packet_handler(uint8_t packet_type, uint16_t channel,
                                            uint8_t *packet, uint16_t size)
{
    (void)channel;
    (void)size;

    if (packet_type != HCI_EVENT_PACKET)
    {
        return;
    }
    if (hci_event_packet_get_type(packet) != HCI_EVENT_AVRCP_META)
    {
        return;
    }

    if (packet[2] == AVRCP_SUBEVENT_NOTIFICATION_VOLUME_CHANGED)
    {
        uint8_t volume = avrcp_subevent_notification_volume_changed_get_absolute_volume(packet);

        volume_percentage = volume * 100 / 127;
        rt_kprintf("[bt] volume %d%% (%d)\n", volume_percentage, volume);
        avrcp_volume_changed(volume);
    }
}

static void avrcp_target_packet_handler(uint8_t packet_type, uint16_t channel,
                                        uint8_t *packet, uint16_t size)
{
    static uint8_t events[] = {
        AVRCP_NOTIFICATION_EVENT_PLAYBACK_STATUS_CHANGED,
        AVRCP_NOTIFICATION_EVENT_TRACK_CHANGED,
        AVRCP_NOTIFICATION_EVENT_VOLUME_CHANGED
    };
    static uint8_t companies[] = {0x00, 0x19, 0x58};

    (void)channel;
    (void)size;

    if (packet_type != HCI_EVENT_PACKET)
    {
        return;
    }
    if (hci_event_packet_get_type(packet) != HCI_EVENT_AVRCP_META)
    {
        return;
    }

    switch (packet[2])
    {
    case AVRCP_SUBEVENT_NOTIFICATION_VOLUME_CHANGED:
    {
        uint8_t volume = avrcp_subevent_notification_volume_changed_get_absolute_volume(packet);

        volume_percentage = volume * 100 / 127;
        rt_kprintf("[bt] AVRCP volume %d%% (%d)\n", volume_percentage, volume);
        avrcp_volume_changed(volume);
        break;
    }

    case AVRCP_SUBEVENT_EVENT_IDS_QUERY:
        avrcp_target_supported_events(avrcp_cid, sizeof(events) / sizeof(events[0]),
                                      events, sizeof(events));
        break;

    case AVRCP_SUBEVENT_COMPANY_IDS_QUERY:
        avrcp_target_supported_companies(avrcp_cid, sizeof(companies) / 3,
                                         companies, sizeof(companies));
        break;

    case AVRCP_SUBEVENT_OPERATION:
    {
        avrcp_operation_id_t operation_id =
            avrcp_subevent_operation_get_operation_id(packet);

        rt_kprintf("[bt] AVRCP operation %s\n", avrcp_operation2str(operation_id));
        (void)avrcp_subevent_value;
        break;
    }

    default:
        break;
    }
}

/* ------------------------------------------------------------------------- */
/* BTstack startup                                                           */
/* ------------------------------------------------------------------------- */

static int a2dp_and_avrcp_setup(void)
{
    avdtp_stream_endpoint_t *local_stream_endpoint;
    uint16_t controller_supported_features;
    uint16_t target_supported_features;

    l2cap_init();

    a2dp_sink_init();
    a2dp_sink_register_packet_handler(&a2dp_sink_packet_handler);
    a2dp_sink_register_media_handler(&handle_l2cap_media_data_packet);

    local_stream_endpoint = a2dp_sink_create_stream_endpoint(
        AVDTP_AUDIO, AVDTP_CODEC_SBC,
        media_sbc_codec_capabilities, sizeof(media_sbc_codec_capabilities),
        media_sbc_codec_configuration, sizeof(media_sbc_codec_configuration));
    if (local_stream_endpoint == NULL)
    {
        rt_kprintf("[bt] cannot create A2DP stream endpoint\n");
        return -1;
    }
    a2dp_local_seid = avdtp_local_seid(local_stream_endpoint);

    avrcp_init();
    avrcp_register_packet_handler(&avrcp_packet_handler);

    avrcp_controller_init();
    avrcp_controller_register_packet_handler(&avrcp_controller_packet_handler);

    avrcp_target_init();
    avrcp_target_register_packet_handler(&avrcp_target_packet_handler);

    sdp_init();

    rt_memset(sdp_avdtp_sink_service_buffer, 0, sizeof(sdp_avdtp_sink_service_buffer));
    a2dp_sink_create_sdp_record(sdp_avdtp_sink_service_buffer, 0x10001,
                                AVDTP_SINK_FEATURE_MASK_SPEAKER, NULL, NULL);
    sdp_register_service(sdp_avdtp_sink_service_buffer);

    controller_supported_features = AVRCP_FEATURE_MASK_CATEGORY_PLAYER_OR_RECORDER;
    rt_memset(sdp_avrcp_controller_service_buffer, 0,
              sizeof(sdp_avrcp_controller_service_buffer));
    avrcp_controller_create_sdp_record(sdp_avrcp_controller_service_buffer, 0x10002,
                                       controller_supported_features, NULL, NULL);
    sdp_register_service(sdp_avrcp_controller_service_buffer);

    target_supported_features = AVRCP_FEATURE_MASK_CATEGORY_MONITOR_OR_AMPLIFIER;
    rt_memset(sdp_avrcp_target_service_buffer, 0, sizeof(sdp_avrcp_target_service_buffer));
    avrcp_target_create_sdp_record(sdp_avrcp_target_service_buffer, 0x10003,
                                   target_supported_features, NULL, NULL);
    sdp_register_service(sdp_avrcp_target_service_buffer);

    gap_set_local_name(BT_SPEAKER_NAME);
    gap_discoverable_control(1);
    gap_set_class_of_device(0x200408);

    /* No display/keypad: accept Just Works pairing automatically. */
    gap_ssp_set_io_capability(SSP_IO_CAPABILITY_NO_INPUT_NO_OUTPUT);
    gap_ssp_set_auto_accept(1);

    hci_event_callback_registration.callback = &hci_packet_handler;
    hci_add_event_handler(&hci_event_callback_registration);

    return 0;
}

static void bt_speaker_phase2(int status);

static btstack_timer_source_t download_timeout_timer;
static volatile int firmware_download_done;
static int download_retries;

static void download_timeout_handler(btstack_timer_source_t *timer)
{
    (void)timer;

    if (firmware_download_done)
    {
        return;
    }

    /*
     * If tx_done is false, the host never shifted the HCI Reset out (CTS
     * blocked).  If it is true but rx_fifo is zero, the controller did not
     * answer.
     */
    rt_kprintf("[bt] uart state: tx_fifo=%u tx_done=%d rx_fifo=%u cts=%d flow=0x%08x\n",
               (unsigned)Cy_SCB_UART_GetNumInTxFifo(CYBSP_BT_UART_HW),
               (int)Cy_SCB_UART_IsTxComplete(CYBSP_BT_UART_HW),
               (unsigned)Cy_SCB_UART_GetNumInRxFifo(CYBSP_BT_UART_HW),
               rt_pin_read(GET_PIN(10, 2)),
               (unsigned)CYBSP_BT_UART_HW->UART_FLOW_CTRL);

    if (download_retries < 2)
    {
        const btstack_uart_block_t *uart = btstack_uart_block_rtt_instance();

        download_retries++;
        rt_kprintf("[bt] firmware download timeout, retry %d at %u baud\n",
                   download_retries, (unsigned)BT_UART_BAUDRATE_INIT);

        /*
         * Re-run the CYW boot sequence: hold BT_REG_ON low, configure the
         * UART (which asserts RTS -> module CTS low), then release it so
         * the boot ROM auto-bauds again on the next HCI Reset.
         *
         * Stay at 115200: the boot ROM does not answer the 0xFC18 baud
         * command until the minidriver has been downloaded.
         */
        rt_pin_write(BT_REG_ON_PIN, PIN_LOW);
        rt_thread_mdelay(150);
        btstack_uart_block_rtt_flush();
        uart->set_baudrate(BT_UART_BAUDRATE_INIT);
        rt_pin_write(BT_REG_ON_PIN, PIN_HIGH);
        rt_thread_mdelay(200);

        last_download_percent = 0;
        btstack_chipset_bcm_instance()->init(RT_NULL);
        btstack_chipset_bcm_download_firmware(uart, BT_UART_BAUDRATE_INIT,
                                              &bt_speaker_phase2);

        btstack_run_loop_set_timer(&download_timeout_timer, 20000);
        btstack_run_loop_add_timer(&download_timeout_timer);
        return;
    }

    rt_kprintf("[bt] firmware download timeout\n");
}

static void bt_speaker_phase2(int status)
{
    /* A stale retry may also report completion; only bring the stack up once. */
    if (firmware_download_done)
    {
        return;
    }
    firmware_download_done = 1;
    btstack_run_loop_remove_timer(&download_timeout_timer);

    if (status != 0)
    {
        rt_kprintf("[bt] firmware download failed: %d\n", status);
        return;
    }

    rt_kprintf("[bt] firmware downloaded, starting HCI init\n");

    /* Give the controller time to reboot after Launch_RAM. */
    rt_thread_mdelay(100);

    /* Drop any stale bytes before the H4 state machine starts reading. */
    btstack_uart_block_rtt_flush();

    /*
     * Start the HCI state machine.  The 3 Mbps switch happens once the stack
     * reaches WORKING (see hci_packet_handler), so it cannot race with the
     * 115200 re-init performed by hci_power_control().
     */
    if (a2dp_and_avrcp_setup() == 0)
    {
        hci_power_control(HCI_POWER_ON);
    }
}

/* Commands from MSH are marshalled onto the BTstack thread. */
static void bt_speaker_poll_commands(void)
{
    if (name_requested)
    {
        name_requested = 0;
        gap_set_local_name(pending_name);
        rt_kprintf("[bt] local name set to %s\n", pending_name);
    }

    if (stop_requested)
    {
        stop_requested = 0;
        hci_power_control(HCI_POWER_OFF);
        btstack_run_loop_rtt_request_exit();
    }
}

static void bt_speaker_thread(void *parameter)
{
    static hci_transport_config_uart_t transport_config = {
        HCI_TRANSPORT_CONFIG_UART,
        BT_UART_BAUDRATE_INIT,
        BT_UART_BAUDRATE_MAIN,
        1,
        BT_UART_DEVICE_NAME,
    };
    static btstack_uart_config_t uart_config = {
        BT_UART_BAUDRATE_INIT,
        1,
        BT_UART_DEVICE_NAME,
    };
    const btstack_chipset_t *chipset;
    const btstack_uart_block_t *uart;
    const hci_transport_t *transport;

    (void)parameter;

    rt_kprintf("[bt] starting Bluetooth speaker\n");

    baud_command_sent = 0;
    btstack_memory_init();
    btstack_run_loop_init(btstack_run_loop_rtt_instance());
    btstack_run_loop_rtt_set_poll_callback(&bt_speaker_poll_commands);
    btstack_audio_rtt_init_instance();

    chipset = btstack_chipset_bcm_instance();
    chipset->init(&transport_config);

    /*
     * Power-up sequence required by the CYW555xx boot ROM:
     *   1. keep BT_REG_ON low while the HCI UART is brought up,
     *   2. have host RTS asserted (module CTS low),
     *   3. release BT_REG_ON low -> high,
     *   4. send the first HCI Reset within 3 seconds.
     * The chip then auto-bauds on that first HCI Reset.
     *
     * BT_REG_ON (P11.0) and WL_REG_ON (P11.6) are only weak pull-ups in
     * the generated pin configuration, so drive them as outputs.
     */
    rt_pin_mode(GET_PIN(16, 3), PIN_MODE_OUTPUT);
    rt_pin_write(GET_PIN(16, 3), PIN_HIGH);
    rt_pin_mode(GET_PIN(11, 6), PIN_MODE_OUTPUT);
    rt_pin_write(GET_PIN(11, 6), PIN_HIGH);
    rt_pin_mode(BT_REG_ON_PIN, PIN_MODE_OUTPUT);
    rt_pin_write(BT_REG_ON_PIN, PIN_LOW);
    rt_thread_mdelay(150);

    uart = btstack_uart_block_rtt_instance();
    if (uart->init(&uart_config) != 0)
    {
        rt_kprintf("[bt] UART init failed\n");
        bt_started = 0;
        return;
    }

    btstack_uart_block_rtt_flush();

    /*
     * The SCB asserts RTS while its RX FIFO is below the trigger level,
     * i.e. at this point host RTS should already be low (module CTS low).
     */
    rt_kprintf("[bt] releasing BT_REG_ON, CTS=%d\n",
               rt_pin_read(GET_PIN(10, 2)));
    rt_pin_write(BT_REG_ON_PIN, PIN_HIGH);
    rt_thread_mdelay(BT_REG_ON_DELAY_MS);

    /* Use ReadOut for output pins (their input buffers are disabled). */
    rt_kprintf("[bt] pins: WIFI_OE(out)=%d BT_REG_ON(out)=%d WL_REG_ON(out)=%d DEV_WAKE(out)=%d HOST_WAKE(in)=%d\n",
               (int)Cy_GPIO_ReadOut(GPIO_PRT16, 3),
               (int)Cy_GPIO_ReadOut(GPIO_PRT11, 0),
               (int)Cy_GPIO_ReadOut(GPIO_PRT11, 6),
               (int)Cy_GPIO_ReadOut(GPIO_PRT10, 6),
               rt_pin_read(GET_PIN(10, 4)));

    transport = hci_transport_h4_instance(uart);
    hci_init(transport, &transport_config);
    hci_set_chipset(chipset);
    hci_set_link_key_db(btstack_link_key_db_memory_instance());

    firmware_download_done = 0;
    download_retries = 0;
    btstack_run_loop_set_timer_handler(&download_timeout_timer, &download_timeout_handler);
    /*
     * The boot ROM answers HCI Reset at 115200 but not the 0xFC18 baud
     * command.  Download at 115200 (about 11 s for the 127 KB patch) and
     * switch to 3 Mbps later, after the patched firmware is running.
     */
    btstack_run_loop_set_timer(&download_timeout_timer, 20000);
    btstack_run_loop_add_timer(&download_timeout_timer);

    rt_kprintf("[bt] downloading %s (%d bytes) at 115200, please wait about 12 s...\n",
               brcm_patch_version, brcm_patch_ram_length);

    last_download_percent = 0;
    btstack_chipset_bcm_set_progress_callback(&bt_download_progress);

    btstack_chipset_bcm_download_firmware(uart, BT_UART_BAUDRATE_INIT,
                                          &bt_speaker_phase2);

    btstack_run_loop_execute();

    rt_kprintf("[bt] run loop stopped\n");
    bt_started = 0;
}

int bt_speaker_start(void)
{
    if (bt_started)
    {
        rt_kprintf("[bt] already started\n");
        return 0;
    }

    /*
     * btstack_run_loop_init() can only be called once per boot; after
     * bt_speaker stop the stack must not be started again.
     */
    if (bt_ever_started)
    {
        rt_kprintf("[bt] stack already stopped, reboot to restart\n");
        return -1;
    }

    bt_thread = rt_thread_create("bt_speaker", bt_speaker_thread, RT_NULL,
                                 BT_THREAD_STACK_SIZE, BT_THREAD_PRIORITY, 10);
    if (bt_thread == RT_NULL)
    {
        rt_kprintf("[bt] cannot create thread\n");
        return -1;
    }

    bt_ever_started = 1;
    bt_started = 1;
    rt_thread_startup(bt_thread);
    return 0;
}

static int bt_speaker_cmd(int argc, char **argv)
{
    if (argc == 1 || rt_strcmp(argv[1], "start") == 0)
    {
        return bt_speaker_start();
    }

    if (rt_strcmp(argv[1], "stop") == 0)
    {
        stop_requested = 1;
        btstack_run_loop_rtt_wakeup();
        rt_kprintf("[bt] stop requested\n");
        return 0;
    }

    if (rt_strcmp(argv[1], "status") == 0)
    {
        rt_kprintf("[bt] state: %s\n", bt_started ? "running" : "stopped");
        rt_kprintf("[bt] A2DP cid: 0x%02x, AVRCP cid: 0x%02x, stream: %s\n",
                   a2dp_cid, avrcp_cid,
                   audio_stream_started ? "playing" : "idle");
        return 0;
    }

    if (rt_strcmp(argv[1], "name") == 0)
    {
        if (argc < 3)
        {
            rt_kprintf("usage: bt_speaker name <name>\n");
            return -RT_ERROR;
        }
        rt_strncpy(pending_name, argv[2], sizeof(pending_name) - 1);
        pending_name[sizeof(pending_name) - 1] = '\0';
        name_requested = 1;
        btstack_run_loop_rtt_wakeup();
        rt_kprintf("[bt] name change queued: %s\n", pending_name);
        return 0;
    }

    rt_kprintf("usage: bt_speaker [start|stop|status|name <name>]\n");
    return -RT_ERROR;
}
MSH_CMD_EXPORT_ALIAS(bt_speaker_cmd, bt_speaker, Bluetooth A2DP speaker control);

int main(void)
{
    rt_kprintf("\nEdgi-Talk Bluetooth speaker example\n");
    rt_kprintf("Phone pairing name: %s\n", BT_SPEAKER_NAME);
    bt_speaker_start();
    return 0;
}
