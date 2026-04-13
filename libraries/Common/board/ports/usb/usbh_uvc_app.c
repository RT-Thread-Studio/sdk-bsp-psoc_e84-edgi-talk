/*
 * CherryUSB UVC Host Application Layer
 * FPS monitoring, msh commands, video device callbacks, frame processing.
 */

#include <rtthread.h>
#include <rtdevice.h>
#include "usbh_uvc_stream.h"

#undef  USB_DBG_TAG
#define USB_DBG_TAG "uvc_app"
#include "usb_log.h"

int uvc_display_init(void);
void uvc_display_frame(struct usbh_videoframe *frame, uint16_t src_w, uint16_t src_h);

/* ---------- frame buffers ---------- */

/*
 * Use one raw-sized backing store for both YUYV and MJPEG.
 * This keeps MJPEG safe even when the camera uses a larger-than-expected
 * compressed frame, while preserving the existing YUYV path.
 */
#define UVC_FRAME_BUF_SIZE  (640 * 480 * 2)
#define UVC_FRAME_BUF_COUNT 4

static USB_MEM_ALIGNX uint8_t frame_buffer[UVC_FRAME_BUF_COUNT][UVC_FRAME_BUF_SIZE]
    __attribute__((section(".m33_m55_shared_hyperram")));
static struct usbh_videoframe frame_pool[UVC_FRAME_BUF_COUNT];

static volatile uint8_t uvc_app_running;
static volatile uint32_t g_uvc_display_fps;
static volatile uint32_t g_uvc_display_drop_count;

/* ---------- CherryUSB video class callbacks ---------- */

void usbh_video_run(struct usbh_video *video_class)
{
    USB_LOG_INFO("UVC device connected\r\n");
    usbh_video_list_info(video_class);
}

void usbh_video_stop(struct usbh_video *video_class)
{
    USB_LOG_INFO("UVC device disconnected\r\n");
    usbh_video_stream_stop();
    uvc_app_running = 0;
}

/* ---------- FPS print thread ---------- */

extern volatile uint32_t g_uvc_fps;
extern volatile uint32_t uvc_transfer_count;
extern volatile uint32_t video_complete_count;
extern volatile uint32_t uvc_dbg_eof_count;
extern volatile uint32_t uvc_dbg_fid_toggle;
extern volatile uint32_t uvc_dbg_empty_pkt;
extern volatile uint32_t uvc_dbg_data_pkt;
extern volatile uint32_t uvc_dbg_last_eof_offset;
extern volatile uint32_t uvc_dbg_frame_done;
extern volatile uint32_t uvc_dbg_err_bit;
extern volatile uint32_t uvc_dbg_payload_bytes;
extern volatile uint32_t uvc_dbg_hdronly_pkt;

static void uvc_display_fps_record(void)
{
    static uint32_t frame_count;
    static rt_tick_t tick_last;
    rt_tick_t tick_now;
    rt_tick_t tick_delta;

    frame_count++;
    if ((frame_count % 10U) != 0U) {
        return;
    }

    tick_now = rt_tick_get();
    if (tick_last == 0U) {
        tick_last = tick_now;
        return;
    }

    tick_delta = tick_now - tick_last;
    if (tick_delta > 0U) {
        g_uvc_display_fps = (10U * RT_TICK_PER_SECOND) / tick_delta;
    }
    tick_last = tick_now;
}

static void uvc_frame_keep_latest(struct usbh_videoframe **frame)
{
    struct usbh_videoframe *latest = *frame;
    struct usbh_videoframe *queued;

    while (usbh_video_stream_dequeue(&queued, 0) == 0) {
        usbh_video_stream_enqueue(latest);
        latest = queued;
        g_uvc_display_drop_count++;
    }

    *frame = latest;
}

static void uvc_fps_thread_entry(void *arg)
{
    while (uvc_app_running) {
        USB_LOG_INFO("UVC fps:%d disp:%d drop:%u urb:%d eof:%d fid:%d err:%d empty:%d data:%d hdronly:%d payload:%d done:%d last_off:%u\r\n",
                     (int)g_uvc_fps, (int)g_uvc_display_fps,
                     (unsigned)g_uvc_display_drop_count,
                     (int)video_complete_count,
                     (int)uvc_dbg_eof_count, (int)uvc_dbg_fid_toggle,
                     (int)uvc_dbg_err_bit, (int)uvc_dbg_empty_pkt,
                     (int)uvc_dbg_data_pkt,
                     (int)uvc_dbg_hdronly_pkt, (int)uvc_dbg_payload_bytes,
                     (int)uvc_dbg_frame_done,
                     (unsigned)uvc_dbg_last_eof_offset);
        rt_thread_mdelay(3000);
    }
}

/* ---------- frame consumer thread ---------- */

static uint16_t uvc_cam_w, uvc_cam_h;

static void uvc_frame_thread_entry(void *arg)
{
    struct usbh_videoframe *frame;
    uint32_t frame_count = 0;
    int ret;
    uint16_t stream_w;
    uint16_t stream_h;

    /* initialise display output */
    uvc_display_init();

    while (uvc_app_running) {
        ret = usbh_video_stream_dequeue(&frame, RT_TICK_PER_SECOND);
        if (ret < 0)
            continue;

        uvc_frame_keep_latest(&frame);

        frame_count++;
        if ((frame_count % 30) == 1) {
            USB_LOG_INFO("frame #%d  size=%u fmt=%s\r\n",
                         (int)frame_count, (unsigned)frame->frame_size,
                         frame->frame_format == USBH_VIDEO_FORMAT_MJPEG ?
                             "mjpeg" : "yuyv");
        }

        usbh_video_stream_get_info(&stream_w, &stream_h, RT_NULL);

        /* render frame to LCD */
        uvc_display_frame(frame,
                          stream_w ? stream_w : uvc_cam_w,
                          stream_h ? stream_h : uvc_cam_h);
        uvc_display_fps_record();

        usbh_video_stream_enqueue(frame);
    }
}

/* ---------- msh commands ---------- */

static int cmd_usbh_uvc_start(int argc, char **argv)
{
    uint8_t fmt = USBH_VIDEO_FORMAT_UNCOMPRESSED;  /* default: YUYV */
    uint16_t w = 640, h = 480;
    int ret;

    if (uvc_app_running) {
        USB_LOG_WRN("UVC already running, stop first\r\n");
        return -1;
    }

    if (argc >= 2) {
        fmt = atoi(argv[1]);  /* 0 = YUYV, 1 = MJPEG */
    }
    if (argc >= 4) {
        w = atoi(argv[2]);
        h = atoi(argv[3]);
    }

    USB_LOG_INFO("UVC start: %ux%u format=%s\r\n", w, h,
                 fmt == USBH_VIDEO_FORMAT_MJPEG ? "mjpeg" : "yuyv");

    /* initialise frame pool */
    {
        for (uint32_t i = 0; i < UVC_FRAME_BUF_COUNT; i++) {
            frame_pool[i].frame_buf = frame_buffer[i];
            frame_pool[i].frame_bufsize = UVC_FRAME_BUF_SIZE;
        }
    }

    uvc_cam_w = w;
    uvc_cam_h = h;
    g_uvc_display_fps = 0;
    g_uvc_display_drop_count = 0;

    ret = usbh_video_stream_create(frame_pool, UVC_FRAME_BUF_COUNT);
    if (ret < 0) {
        USB_LOG_ERR("UVC frame pool create failed: %d\r\n", ret);
        return ret;
    }

    uvc_app_running = 1;

    /* spawn helper threads */
    rt_thread_t t;

    t = rt_thread_create("uvc_fps", uvc_fps_thread_entry, NULL,
                         1024, 20, 10);
    if (t) rt_thread_startup(t);

    t = rt_thread_create("uvc_frm", uvc_frame_thread_entry, NULL,
                         8192, 22, 10);
    if (t) rt_thread_startup(t);

    /* start the video stream (frames are queued inside stream_start) */
    usbh_video_stream_start(w, h, fmt);

    return 0;
}
MSH_CMD_EXPORT_ALIAS(cmd_usbh_uvc_start, usbh_uvc_start,
                     Start UVC stream: usbh_uvc_start [fmt] [w] [h]);

static int cmd_usbh_uvc_stop(int argc, char **argv)
{
    uvc_app_running = 0;
    rt_thread_mdelay(100);  /* let helper threads notice the flag and exit */
    usbh_video_stream_stop();
    USB_LOG_INFO("UVC stopped\r\n");
    return 0;
}
MSH_CMD_EXPORT_ALIAS(cmd_usbh_uvc_stop, usbh_uvc_stop, Stop UVC stream);
