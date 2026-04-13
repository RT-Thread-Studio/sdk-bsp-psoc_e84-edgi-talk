#include "deepcraft_lsm6ds3.h"

#include <stdint.h>
#include <string.h>

#include <rtthread.h>

#include "protocol/pb_encode.h"
#include "drivers/lsm6ds3tr/lsm6ds3tr-c_port.h"

#ifdef BSP_USING_LSM6DS3

#define IMU_OPTION_KEY_RATE        101
#define IMU_OPTION_KEY_ACCEL_RANGE 102
#define IMU_OPTION_KEY_GYRO_RANGE  103
#define IMU_OPTION_KEY_STREAM_MODE 104

typedef enum
{
    LSM6DS3_MODE_COMBINED = 0,
    LSM6DS3_MODE_SPLIT = 1,
    LSM6DS3_MODE_ONLY_ACCEL = 2,
    LSM6DS3_MODE_ONLY_GYRO = 3,
} deepcraft_lsm6ds3_mode_t;

typedef struct
{
    rt_bool_t running;
    int stream0_id;
    int stream1_id;
    int sample_rate_hz;
    int accel_range_g;
    int gyro_range_dps;
    deepcraft_lsm6ds3_mode_t stream_mode;
    rt_tick_t last_tick;
    float combined_data[6];
    float accel_data[3];
    float gyro_data[3];
} deepcraft_lsm6ds3_ctx_t;

static deepcraft_lsm6ds3_ctx_t g_lsm6ds3;

static const char *g_rate_labels[] = { "50 Hz", "100 Hz", "200 Hz", "400 Hz" };
static const char *g_accel_range_labels[] = { "2 G", "4 G", "8 G", "16 G" };
static const char *g_gyro_range_labels[] = { "125 dps", "250 dps", "500 dps", "1000 dps", "2000 dps" };
static const char *g_mode_labels[] = { "Combined", "Split", "Only Accel", "Only Gyro" };

static const char *g_axis_labels[] = { "X", "Y", "Z" };
static const char *g_sensor_labels[] = { "Accel", "Gyro" };

static bool deepcraft_lsm6ds3_decode_rate(int index, int *rate_hz)
{
    if (rate_hz == RT_NULL)
    {
        return false;
    }

    switch (index)
    {
    case 0:
        *rate_hz = 50;
        return true;
    case 1:
        *rate_hz = 100;
        return true;
    case 2:
        *rate_hz = 200;
        return true;
    case 3:
        *rate_hz = 400;
        return true;
    default:
        return false;
    }
}

static bool deepcraft_lsm6ds3_decode_accel_range(int index, int *accel_range_g)
{
    if (accel_range_g == RT_NULL)
    {
        return false;
    }

    switch (index)
    {
    case 0:
        *accel_range_g = 2;
        return true;
    case 1:
        *accel_range_g = 4;
        return true;
    case 2:
        *accel_range_g = 8;
        return true;
    case 3:
        *accel_range_g = 16;
        return true;
    default:
        return false;
    }
}

static bool deepcraft_lsm6ds3_decode_gyro_range(int index, int *gyro_range_dps)
{
    if (gyro_range_dps == RT_NULL)
    {
        return false;
    }

    switch (index)
    {
    case 0:
        *gyro_range_dps = 125;
        return true;
    case 1:
        *gyro_range_dps = 250;
        return true;
    case 2:
        *gyro_range_dps = 500;
        return true;
    case 3:
        *gyro_range_dps = 1000;
        return true;
    case 4:
        *gyro_range_dps = 2000;
        return true;
    default:
        return false;
    }
}

static bool deepcraft_lsm6ds3_decode_mode(int index, deepcraft_lsm6ds3_mode_t *mode)
{
    if (mode == RT_NULL)
    {
        return false;
    }

    switch (index)
    {
    case 0:
        *mode = LSM6DS3_MODE_COMBINED;
        return true;
    case 1:
        *mode = LSM6DS3_MODE_SPLIT;
        return true;
    case 2:
        *mode = LSM6DS3_MODE_ONLY_ACCEL;
        return true;
    case 3:
        *mode = LSM6DS3_MODE_ONLY_GYRO;
        return true;
    default:
        return false;
    }
}

static bool deepcraft_lsm6ds3_write_payload(
    protocol_t *protocol,
    int device_id,
    int stream_id,
    int frame_count,
    int total_bytes,
    pb_ostream_t *ostream,
    void *arg)
{
    deepcraft_lsm6ds3_ctx_t *ctx = (deepcraft_lsm6ds3_ctx_t *)arg;
    const pb_byte_t *payload = RT_NULL;
    int payload_bytes = 0;

    (void)protocol;
    (void)device_id;
    (void)frame_count;

    if (ctx == RT_NULL)
    {
        return false;
    }

    if (ctx->stream_mode == LSM6DS3_MODE_COMBINED)
    {
        if (stream_id != ctx->stream0_id)
        {
            return false;
        }

        payload = (const pb_byte_t *)ctx->combined_data;
        payload_bytes = (int)sizeof(ctx->combined_data);
    }
    else if (ctx->stream_mode == LSM6DS3_MODE_SPLIT)
    {
        if (stream_id == ctx->stream0_id)
        {
            payload = (const pb_byte_t *)ctx->accel_data;
            payload_bytes = (int)sizeof(ctx->accel_data);
        }
        else if (stream_id == ctx->stream1_id)
        {
            payload = (const pb_byte_t *)ctx->gyro_data;
            payload_bytes = (int)sizeof(ctx->gyro_data);
        }
        else
        {
            return false;
        }
    }
    else if (ctx->stream_mode == LSM6DS3_MODE_ONLY_ACCEL)
    {
        if (stream_id != ctx->stream0_id)
        {
            return false;
        }

        payload = (const pb_byte_t *)ctx->accel_data;
        payload_bytes = (int)sizeof(ctx->accel_data);
    }
    else if (ctx->stream_mode == LSM6DS3_MODE_ONLY_GYRO)
    {
        if (stream_id != ctx->stream0_id)
        {
            return false;
        }

        payload = (const pb_byte_t *)ctx->gyro_data;
        payload_bytes = (int)sizeof(ctx->gyro_data);
    }

    if (payload == RT_NULL || payload_bytes <= 0 || total_bytes != payload_bytes)
    {
        return false;
    }

    return pb_write(ostream, payload, (size_t)payload_bytes);
}

static bool deepcraft_lsm6ds3_load_options(protocol_t *protocol, int device, deepcraft_lsm6ds3_ctx_t *ctx)
{
    int rate_index;
    int accel_range_index;
    int gyro_range_index;
    int mode_index;

    if (protocol_get_option_oneof(protocol, device, IMU_OPTION_KEY_RATE, &rate_index) != PROTOCOL_STATUS_SUCCESS ||
        !deepcraft_lsm6ds3_decode_rate(rate_index, &ctx->sample_rate_hz))
    {
        protocol_set_device_status(protocol, device, protocol_DeviceStatus_DEVICE_STATUS_ERROR, "Failed to decode sample rate option.");
        return true;
    }

    if (protocol_get_option_oneof(protocol, device, IMU_OPTION_KEY_ACCEL_RANGE, &accel_range_index) != PROTOCOL_STATUS_SUCCESS ||
        !deepcraft_lsm6ds3_decode_accel_range(accel_range_index, &ctx->accel_range_g))
    {
        protocol_set_device_status(protocol, device, protocol_DeviceStatus_DEVICE_STATUS_ERROR, "Failed to decode accel range option.");
        return true;
    }

    if (protocol_get_option_oneof(protocol, device, IMU_OPTION_KEY_GYRO_RANGE, &gyro_range_index) != PROTOCOL_STATUS_SUCCESS ||
        !deepcraft_lsm6ds3_decode_gyro_range(gyro_range_index, &ctx->gyro_range_dps))
    {
        protocol_set_device_status(protocol, device, protocol_DeviceStatus_DEVICE_STATUS_ERROR, "Failed to decode gyro range option.");
        return true;
    }

    if (protocol_get_option_oneof(protocol, device, IMU_OPTION_KEY_STREAM_MODE, &mode_index) != PROTOCOL_STATUS_SUCCESS ||
        !deepcraft_lsm6ds3_decode_mode(mode_index, &ctx->stream_mode))
    {
        protocol_set_device_status(protocol, device, protocol_DeviceStatus_DEVICE_STATUS_ERROR, "Failed to decode stream mode option.");
        return true;
    }

    return true;
}

static bool deepcraft_lsm6ds3_configure_streams(protocol_t *protocol, int device, void *arg)
{
    deepcraft_lsm6ds3_ctx_t *ctx = (deepcraft_lsm6ds3_ctx_t *)arg;
    int stream0;

    ctx->stream0_id = -1;
    ctx->stream1_id = -1;

    if (!deepcraft_lsm6ds3_load_options(protocol, device, ctx))
    {
        return true;
    }

    if (protocol_clear_streams(protocol, device) != PROTOCOL_STATUS_SUCCESS)
    {
        protocol_set_device_status(protocol, device, protocol_DeviceStatus_DEVICE_STATUS_ERROR, "Failed to clear streams.");
        return true;
    }

    if (ctx->stream_mode == LSM6DS3_MODE_COMBINED)
    {
        stream0 = protocol_add_stream(
            protocol,
            device,
            "Combined",
            protocol_StreamDirection_STREAM_DIRECTION_OUTPUT,
            protocol_DataType_DATA_TYPE_F32,
            (float)ctx->sample_rate_hz,
            1,
            "mg, mdps");

        if (stream0 < 0)
        {
            protocol_set_device_status(protocol, device, protocol_DeviceStatus_DEVICE_STATUS_ERROR, "Failed to add combined stream.");
            return true;
        }

        ctx->stream0_id = stream0;

        if (protocol_add_stream_rank(protocol, device, stream0, "Sensor", 2, g_sensor_labels) != PROTOCOL_STATUS_SUCCESS ||
            protocol_add_stream_rank(protocol, device, stream0, "Axis", 3, g_axis_labels) != PROTOCOL_STATUS_SUCCESS)
        {
            protocol_set_device_status(protocol, device, protocol_DeviceStatus_DEVICE_STATUS_ERROR, "Failed to configure combined stream ranks.");
            return true;
        }
    }
    else
    {
        const char *stream0_name = (ctx->stream_mode == LSM6DS3_MODE_ONLY_GYRO) ? "Gyro" : "Accel";
        const char *stream0_unit = (ctx->stream_mode == LSM6DS3_MODE_ONLY_GYRO) ? "mdps" : "mg";

        stream0 = protocol_add_stream(
            protocol,
            device,
            stream0_name,
            protocol_StreamDirection_STREAM_DIRECTION_OUTPUT,
            protocol_DataType_DATA_TYPE_F32,
            (float)ctx->sample_rate_hz,
            1,
            stream0_unit);

        if (stream0 < 0)
        {
            protocol_set_device_status(protocol, device, protocol_DeviceStatus_DEVICE_STATUS_ERROR, "Failed to add stream.");
            return true;
        }

        ctx->stream0_id = stream0;

        if (protocol_add_stream_rank(protocol, device, stream0, "Axis", 3, g_axis_labels) != PROTOCOL_STATUS_SUCCESS)
        {
            protocol_set_device_status(protocol, device, protocol_DeviceStatus_DEVICE_STATUS_ERROR, "Failed to configure stream rank.");
            return true;
        }

        if (ctx->stream_mode == LSM6DS3_MODE_SPLIT)
        {
            int stream1 = protocol_add_stream(
                protocol,
                device,
                "Gyro",
                protocol_StreamDirection_STREAM_DIRECTION_OUTPUT,
                protocol_DataType_DATA_TYPE_F32,
                (float)ctx->sample_rate_hz,
                1,
                "mdps");

            if (stream1 < 0)
            {
                protocol_set_device_status(protocol, device, protocol_DeviceStatus_DEVICE_STATUS_ERROR, "Failed to add gyro stream.");
                return true;
            }

            ctx->stream1_id = stream1;

            if (protocol_add_stream_rank(protocol, device, stream1, "Axis", 3, g_axis_labels) != PROTOCOL_STATUS_SUCCESS)
            {
                protocol_set_device_status(protocol, device, protocol_DeviceStatus_DEVICE_STATUS_ERROR, "Failed to configure gyro stream rank.");
                return true;
            }
        }
    }

    protocol_set_device_status(protocol, device, protocol_DeviceStatus_DEVICE_STATUS_READY, "LSM6DS3 is ready.");
    return true;
}

static void deepcraft_lsm6ds3_start(protocol_t *protocol, int device, pb_ostream_t *ostream, void *arg)
{
    deepcraft_lsm6ds3_ctx_t *ctx = (deepcraft_lsm6ds3_ctx_t *)arg;
    lsm6ds3tr_port_config_t config;

    (void)ostream;

    if (ctx->running)
    {
        return;
    }

    if (ctx->stream0_id < 0)
    {
        protocol_set_device_status(protocol, device, protocol_DeviceStatus_DEVICE_STATUS_ERROR, "Invalid IMU stream configuration.");
        return;
    }

    config.sample_rate_hz = ctx->sample_rate_hz;
    config.accel_range_g = ctx->accel_range_g;
    config.gyro_range_dps = ctx->gyro_range_dps;

    if (lsm6ds3tr_port_configure(&config) != RT_EOK)
    {
        protocol_set_device_status(protocol, device, protocol_DeviceStatus_DEVICE_STATUS_ERROR, "Failed to configure LSM6DS3 hardware.");
        return;
    }

    ctx->running = RT_TRUE;
    ctx->last_tick = 0;

    protocol_set_device_status(protocol, device, protocol_DeviceStatus_DEVICE_STATUS_ACTIVE, "LSM6DS3 streaming.");
}

static void deepcraft_lsm6ds3_stop(protocol_t *protocol, int device, pb_ostream_t *ostream, void *arg)
{
    deepcraft_lsm6ds3_ctx_t *ctx = (deepcraft_lsm6ds3_ctx_t *)arg;

    (void)ostream;

    if (!ctx->running)
    {
        return;
    }

    ctx->running = RT_FALSE;

    protocol_set_device_status(protocol, device, protocol_DeviceStatus_DEVICE_STATUS_READY, "LSM6DS3 stopped.");
}

static void deepcraft_lsm6ds3_poll(protocol_t *protocol, int device, pb_ostream_t *ostream, void *arg)
{
    deepcraft_lsm6ds3_ctx_t *ctx = (deepcraft_lsm6ds3_ctx_t *)arg;
    lsm6ds3tr_sample_t sample;

    if (!ctx->running)
    {
        return;
    }

    rt_tick_t now = rt_tick_get();
    rt_tick_t interval_ticks = (rt_tick_t)((RT_TICK_PER_SECOND + ctx->sample_rate_hz - 1) / ctx->sample_rate_hz);
    if (interval_ticks == 0)
    {
        interval_ticks = 1;
    }

    if (ctx->last_tick != 0 && (now - ctx->last_tick) < interval_ticks)
    {
        return;
    }

    if (lsm6ds3tr_port_read_sample(&sample) != RT_EOK)
    {
        return;
    }

    if ((sample.valid_mask & (LSM6DS3TR_SAMPLE_VALID_ACC | LSM6DS3TR_SAMPLE_VALID_GYRO)) == 0)
    {
        return;
    }

    ctx->accel_data[0] = sample.acc_mg[0];
    ctx->accel_data[1] = sample.acc_mg[1];
    ctx->accel_data[2] = sample.acc_mg[2];

    ctx->gyro_data[0] = sample.gyro_mdps[0];
    ctx->gyro_data[1] = sample.gyro_mdps[1];
    ctx->gyro_data[2] = sample.gyro_mdps[2];

    ctx->combined_data[0] = ctx->accel_data[0];
    ctx->combined_data[1] = ctx->accel_data[1];
    ctx->combined_data[2] = ctx->accel_data[2];
    ctx->combined_data[3] = ctx->gyro_data[0];
    ctx->combined_data[4] = ctx->gyro_data[1];
    ctx->combined_data[5] = ctx->gyro_data[2];

    ctx->last_tick = now;

    protocol_send_data_chunk(
        protocol,
        device,
        ctx->stream0_id,
        1,
        0,
        ostream,
        deepcraft_lsm6ds3_write_payload);

    if (ctx->stream_mode == LSM6DS3_MODE_SPLIT && ctx->stream1_id >= 0)
    {
        protocol_send_data_chunk(
            protocol,
            device,
            ctx->stream1_id,
            1,
            0,
            ostream,
            deepcraft_lsm6ds3_write_payload);
    }
}

bool deepcraft_lsm6ds3_register(protocol_t *protocol)
{
    int device;
    device_manager_t manager;

    if (protocol == RT_NULL)
    {
        return false;
    }

    rt_memset(&g_lsm6ds3, 0, sizeof(g_lsm6ds3));
    g_lsm6ds3.stream0_id = -1;
    g_lsm6ds3.stream1_id = -1;

    if (lsm6ds3tr_port_init() != RT_EOK)
    {
        rt_kprintf("deepcraft: lsm6ds3 init failed.\r\n");
        return false;
    }

    manager.arg = &g_lsm6ds3;
    manager.busy = RT_NULL;
    manager.configure_streams = deepcraft_lsm6ds3_configure_streams;
    manager.start = deepcraft_lsm6ds3_start;
    manager.stop = deepcraft_lsm6ds3_stop;
    manager.poll = deepcraft_lsm6ds3_poll;
    manager.data_received = RT_NULL;

    device = protocol_add_device(
        protocol,
        protocol_DeviceType_DEVICE_TYPE_SENSOR,
        "LSM6DS3TR",
        "Accelerometer and Gyroscope (LSM6DS3TR)",
        manager);

    if (device < 0)
    {
        return false;
    }

    if (protocol_add_option_oneof(
            protocol,
            device,
            IMU_OPTION_KEY_RATE,
            "Frequency",
            "Sample frequency (Hz)",
            0,
            g_rate_labels,
            sizeof(g_rate_labels) / sizeof(g_rate_labels[0])) != PROTOCOL_STATUS_SUCCESS)
    {
        return false;
    }

    if (protocol_add_option_oneof(
            protocol,
            device,
            IMU_OPTION_KEY_ACCEL_RANGE,
            "Accel Range",
            "Min/Max gravity range in G",
            0,
            g_accel_range_labels,
            sizeof(g_accel_range_labels) / sizeof(g_accel_range_labels[0])) != PROTOCOL_STATUS_SUCCESS)
    {
        return false;
    }

    if (protocol_add_option_oneof(
            protocol,
            device,
            IMU_OPTION_KEY_GYRO_RANGE,
            "Gyro Range",
            "Angular rate measurement range",
            4,
            g_gyro_range_labels,
            sizeof(g_gyro_range_labels) / sizeof(g_gyro_range_labels[0])) != PROTOCOL_STATUS_SUCCESS)
    {
        return false;
    }

    if (protocol_add_option_oneof(
            protocol,
            device,
            IMU_OPTION_KEY_STREAM_MODE,
            "Mode",
            "Stream configuration",
            0,
            g_mode_labels,
            sizeof(g_mode_labels) / sizeof(g_mode_labels[0])) != PROTOCOL_STATUS_SUCCESS)
    {
        return false;
    }

    return deepcraft_lsm6ds3_configure_streams(protocol, device, &g_lsm6ds3);
}

#else

bool deepcraft_lsm6ds3_register(protocol_t *protocol)
{
    (void)protocol;
    rt_kprintf("deepcraft: BSP_USING_LSM6DS3 disabled, lsm6ds3 unavailable.\r\n");
    return false;
}

#endif
