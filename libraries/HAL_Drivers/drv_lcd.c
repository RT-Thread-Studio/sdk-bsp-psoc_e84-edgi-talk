/*
 * Copyright (c) 2006-2018, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2025-01-06     yuanjie      first version
 */

#include <rtthread.h>
#include <board.h>

#ifdef BSP_USING_LCD

#include <rtdevice.h>
#include "cy_graphics.h"
#include "cy_axidmac.h"
#include <display_tl043wvv02.h>
#include <board.h>
#include "vg_lite.h"
#include "vg_lite_platform.h"
#ifdef PKG_USING_CPU_USAGE
    #include <cpu_usage.h>
#endif
#ifdef BSP_USING_LVGL
    #include "lv_port_disp.h"
#endif

#define DRV_DEBUG
#define LOG_TAG             "drv.lcd"
#include <drv_log.h>

/* 10.1" display pin configuration */
#define DISPLAY_RESET_PORT                  GPIO_PRT20
#define DISPLAY_RESET_PIN                   (7U)

#define LCD_WIDTH           480
#define LCD_HEIGHT          800
#define LCD_STRIDE          512
#define LCD_LAYER_X_POS_PIXELS 0
#define LCD_SCANOUT_X_ADJUST_PIXELS 0
#define LCD_LOGICAL_X_ADJUST_PIXELS 0
#define LCD_WINDOW_X_PIXELS 0
#define LCD_WINDOW_Y_PIXELS 0
#define LCD_GFXSS_ROTATE_180 0
#ifndef BSP_LCD_ROTATION_DEGREES
#define BSP_LCD_ROTATION_DEGREES 0
#endif
#define LCD_ROTATION_DEGREES BSP_LCD_ROTATION_DEGREES
#if defined(BSP_LCD_ROTATION_BACKEND_VGLITE) || \
    (BSP_LCD_ROTATION_DEGREES == 90) || (BSP_LCD_ROTATION_DEGREES == 270)
#define LCD_ROTATION_BACKEND_VGLITE 1
#else
#define LCD_ROTATION_BACKEND_VGLITE 0
#endif
#define LCD_MIPI_HFP_PIXELS 86
#define LCD_MIPI_HBP_PIXELS 87
#define LCD_MIPI_HSYNC_WIDTH_PIXELS 2
#define LCD_MIPI_VFP_LINES 182
#define LCD_MIPI_VBP_LINES 8
#define LCD_MIPI_VSYNC_LINES 2
#define LCD_MIPI_PIXEL_CLOCK_KHZ 33984
#define LCD_MIPI_PER_LANE_MBPS 900
#define LCD_MIPI_MAX_PHY_CLK_HZ 2500000000UL
#define LCD_BITS_PER_PIXEL  16
#define LCD_VISIBLE_X_OFFSET_PIXELS 0
#define LCD_VISIBLE_X_OFFSET_BYTES (LCD_VISIBLE_X_OFFSET_PIXELS * LCD_BITS_PER_PIXEL / 8)
#define LCD_PIXEL_FORMAT    RTGRAPHIC_PIXEL_FORMAT_RGB565
#define LCD_DEVICE(dev)     (struct drv_lcd_device*)(dev)
#define LCD_BL_PWM_DEV_NAME      "pwm18"
#define LCD_BL_PWM_CHANNEL       0
#define LCD_BL_PWM_PERIOD_NS     200000
#define LCD_BL_DEFAULT_BRIGHTNESS 80
#define LCD_AXIDMAC_CHANNEL      0U
#define LCD_AXIDMAC_TIMEOUT_MS   20U
#define LCD_AXIDMAC_ERROR_MASK   (CY_AXIDMAC_INTR_SRC_BUS_ERROR | \
                                  CY_AXIDMAC_INTR_DST_BUS_ERROR | \
                                  CY_AXIDMAC_INTR_INVALID_DESCR_TYPE | \
                                  CY_AXIDMAC_INTR_CURR_PTR_NULL | \
                                  CY_AXIDMAC_INTR_ACTIVE_CH_DISABLED | \
                                  CY_AXIDMAC_INTR_DESCR_BUS_ERROR)

#ifndef LCD_AXIDMAC_AREA_COPY_MIN_BYTES
#ifdef BSP_LCD_AXIDMAC_AREA_COPY_MIN_BYTES
#define LCD_AXIDMAC_AREA_COPY_MIN_BYTES BSP_LCD_AXIDMAC_AREA_COPY_MIN_BYTES
#else
#define LCD_AXIDMAC_AREA_COPY_MIN_BYTES (8U * 1024U)
#endif
#endif

#ifndef LCD_USE_AXIDMAC_AREA_COPY
#ifdef BSP_LCD_USE_AXIDMAC_AREA_COPY
#define LCD_USE_AXIDMAC_AREA_COPY 1
#else
#define LCD_USE_AXIDMAC_AREA_COPY 0
#endif
#endif

#if ((LCD_ROTATION_DEGREES != 0) && (LCD_ROTATION_DEGREES != 90) && \
     (LCD_ROTATION_DEGREES != 180) && (LCD_ROTATION_DEGREES != 270))
#error "LCD_ROTATION_DEGREES must be 0, 90, 180, or 270"
#endif

#if ((LCD_ROTATION_BACKEND_VGLITE != 0) && \
     (LCD_ROTATION_DEGREES != 90) && (LCD_ROTATION_DEGREES != 270))
#error "LCD_ROTATION_BACKEND_VGLITE requires LCD_ROTATION_DEGREES to be 90 or 270"
#endif

#if ((LCD_ROTATION_DEGREES == 90) || (LCD_ROTATION_DEGREES == 270))
#define LCD_LOGICAL_WIDTH   LCD_HEIGHT
#define LCD_LOGICAL_HEIGHT  LCD_WIDTH
#define LCD_RENDER_STRIDE   LCD_LOGICAL_WIDTH
#else
#define LCD_LOGICAL_WIDTH   LCD_WIDTH
#define LCD_LOGICAL_HEIGHT  LCD_HEIGHT
#define LCD_RENDER_STRIDE   LCD_STRIDE
#endif

#define LCD_RENDER_BUF_SIZE  (LCD_RENDER_STRIDE * LCD_LOGICAL_HEIGHT * LCD_BITS_PER_PIXEL / 8)
#define LCD_SCANOUT_BUF_SIZE (LCD_STRIDE * LCD_HEIGHT * LCD_BITS_PER_PIXEL / 8)
#define LCD_BUF_SIZE         LCD_SCANOUT_BUF_SIZE

#if LCD_ROTATION_BACKEND_VGLITE
#define LCD_NEEDS_SCANOUT_BUFFER 1
#else
#define LCD_NEEDS_SCANOUT_BUFFER 0
#endif

#if (LCD_SCANOUT_X_ADJUST_PIXELS >= LCD_WIDTH)
#error "LCD_SCANOUT_X_ADJUST_PIXELS must be smaller than LCD_WIDTH"
#endif

#define GPU_TESSELLATION_BUFFER_SIZE        ((LCD_LOGICAL_WIDTH) * 128U)
#define APP_BUFFER_COUNT                    (2U)
#define DEFAULT_GPU_CMD_BUFFER_SIZE         ((64U) * (512))
#define VG_PARAMS_POS                       (0UL)
#define GPU_MEM_BASE                        (0x0U)
#define VGLITE_HEAP_SIZE        (((DEFAULT_GPU_CMD_BUFFER_SIZE) * \
                                 (APP_BUFFER_COUNT)) + \
                                 ((GPU_TESSELLATION_BUFFER_SIZE) * \
                                 (APP_BUFFER_COUNT)))
CY_SECTION(".cy_gpu_buf") CY_ALIGN(__SCB_DCACHE_LINE_SIZE) uint8_t contiguous_mem[VGLITE_HEAP_SIZE] = { 0xFF };
volatile void *vglite_heap_base = &contiguous_mem;

struct drv_lcd_device _lcd;

GFXSS_Type *gfxbase = (GFXSS_Type*) GFXSS;
static cy_stc_gfx_context_t lcd_gfx_context;

CY_SECTION(".cy_gpu_buf") CY_ALIGN(__SCB_DCACHE_LINE_SIZE) static uint8_t graphics_storage[LCD_RENDER_BUF_SIZE + LCD_VISIBLE_X_OFFSET_BYTES] = {0xFF};
static uint8_t *graphics_buffer = &graphics_storage[LCD_VISIBLE_X_OFFSET_BYTES];

#if LCD_NEEDS_SCANOUT_BUFFER
CY_SECTION(".cy_gpu_buf") CY_ALIGN(__SCB_DCACHE_LINE_SIZE) static uint8_t graphics_scanout_storage[LCD_SCANOUT_BUF_SIZE] = {0x00};
#endif

#if LCD_USE_AXIDMAC_AREA_COPY
CY_SECTION(".cy_socmem_data") CY_ALIGN(8) static cy_stc_axidmac_descriptor_t lcd_axidmac_descriptor;
static rt_mutex_t lcd_axidmac_lock;
#endif

static inline uint16_t *lcd_render_framebuffer(void)
{
    return (uint16_t *)graphics_buffer;
}

static inline uint32_t *lcd_scanout_framebuffer(void)
{
#if LCD_NEEDS_SCANOUT_BUFFER
    return (uint32_t *)graphics_scanout_storage;
#else
    return (uint32_t *)&graphics_storage[0];
#endif
}

static void lcd_apply_gfxss_rotation(void)
{
#if vivENABLE_LAYER_ROT
    static rt_bool_t rotation_log_done = RT_FALSE;
    gctUINT rotation_cap = 0U;
    vivSTATUS cap_status;
#if LCD_GFXSS_ROTATE_180
    vivSTATUS select_status;
    vivSTATUS rotation_status;
    vivSTATUS commit_status;
#endif

    cap_status = viv_layer_query_capability(GFX_LAYER_GRAPHICS, vivLAYER_CAP_ROTATION, &rotation_cap);

#if LCD_GFXSS_ROTATE_180
    if ((cap_status == vivSTATUS_OK) && (rotation_cap != 0U))
    {
        select_status = viv_dc_select_layer(GFX_LAYER_GRAPHICS);
        rotation_status = viv_layer_rotation(vivROTANGLE_180);
        commit_status = viv_set_commit(0x1);
    }
    else
    {
        select_status = vivSTATUS_NOT_SUPPORT;
        rotation_status = vivSTATUS_NOT_SUPPORT;
        commit_status = vivSTATUS_NOT_SUPPORT;
    }

    if (!rotation_log_done)
    {
        if ((select_status != vivSTATUS_OK) ||
            (rotation_status != vivSTATUS_OK) ||
            (commit_status != vivSTATUS_OK))
        {
            LOG_W("GFXSS layer rotation config failed: cap=%d/%u select=%d rotation=%d commit=%d",
                  cap_status, rotation_cap, select_status, rotation_status, commit_status);
        }
        else
        {
            LOG_I("GFXSS layer rotation: %d degrees, cap=%d/%u",
                  LCD_GFXSS_ROTATE_180 ? 180 : 0, cap_status, rotation_cap);
        }
        rotation_log_done = RT_TRUE;
    }
#else
    if (!rotation_log_done)
    {
        LOG_I("GFXSS layer rotation disabled, cap=%d/%u", cap_status, rotation_cap);
        rotation_log_done = RT_TRUE;
    }
#endif
#endif
}

static void lcd_apply_runtime_gfxss_config(void)
{
    GFXSS_graphics_layer.layer_type = GFX_LAYER_GRAPHICS;
    GFXSS_graphics_layer.input_format_type = vivRGB565;
    GFXSS_graphics_layer.tiling_type = vivLINEAR;
    GFXSS_graphics_layer.pos_x = LCD_LAYER_X_POS_PIXELS;
    GFXSS_graphics_layer.pos_y = 0;
    GFXSS_graphics_layer.width = LCD_WIDTH;
    GFXSS_graphics_layer.height = LCD_HEIGHT;
    GFXSS_graphics_layer.zorder = 0;
    GFXSS_graphics_layer.layer_enable = true;
    GFXSS_graphics_layer.visibility = true;

    GFXSS_overlay0_layer.layer_enable = false;
    GFXSS_overlay0_layer.visibility = false;
    GFXSS_overlay0_layer.width = 1;
    GFXSS_overlay0_layer.height = 1;
    GFXSS_overlay1_layer.layer_enable = false;
    GFXSS_overlay1_layer.visibility = false;
    GFXSS_overlay1_layer.width = 1;
    GFXSS_overlay1_layer.height = 1;

    GFXSS_dc_config.gfx_layer_config = &GFXSS_graphics_layer;
    GFXSS_dc_config.ovl0_layer_config = &GFXSS_overlay0_layer;
    GFXSS_dc_config.ovl1_layer_config = &GFXSS_overlay1_layer;
    GFXSS_dc_config.display_type = GFX_DISP_TYPE_DSI_DPI;
    /* Device Configurator does not expose CFG1, but this panel wiring needs it. */
    GFXSS_dc_config.display_format = vivD16CFG1;
    GFXSS_dc_config.display_size = vivDISPLAY_CUSTOMIZED;
    GFXSS_dc_config.display_width = LCD_WIDTH;
    GFXSS_dc_config.display_height = LCD_HEIGHT;

    GFXSS_mipidsi_display_params.pixel_clock = LCD_MIPI_PIXEL_CLOCK_KHZ;
    GFXSS_mipidsi_display_params.hdisplay = LCD_WIDTH;
    GFXSS_mipidsi_display_params.hsync_width = LCD_MIPI_HSYNC_WIDTH_PIXELS;
    GFXSS_mipidsi_display_params.hfp = LCD_MIPI_HFP_PIXELS;
    GFXSS_mipidsi_display_params.hbp = LCD_MIPI_HBP_PIXELS;
    GFXSS_mipidsi_display_params.vdisplay = LCD_HEIGHT;
    GFXSS_mipidsi_display_params.vsync_width = LCD_MIPI_VSYNC_LINES;
    GFXSS_mipidsi_display_params.vfp = LCD_MIPI_VFP_LINES;
    GFXSS_mipidsi_display_params.vbp = LCD_MIPI_VBP_LINES;
    GFXSS_mipidsi_display_params.polarity_flags = 0;

    GFXSS_mipi_dsi_config.virtual_ch = 0;
    GFXSS_mipi_dsi_config.num_of_lanes = MTB_DISPLAY_EK79007AD3_PANEL_NUM_LANES;
    GFXSS_mipi_dsi_config.per_lane_mbps = LCD_MIPI_PER_LANE_MBPS;
    GFXSS_mipi_dsi_config.dpi_fmt = CY_MIPIDSI_FMT_RGB565;
    GFXSS_mipi_dsi_config.dsi_mode = DSI_VIDEO_MODE;
    GFXSS_mipi_dsi_config.max_phy_clk = LCD_MIPI_MAX_PHY_CLK_HZ;
    GFXSS_mipi_dsi_config.mode_flags =
        VID_MODE_TYPE_NON_BURST_SYNC_PULSES | ENABLE_LOW_POWER_CMD | ENABLE_LOW_POWER;
    GFXSS_mipi_dsi_config.display_params = &GFXSS_mipidsi_display_params;

    GFXSS_config.dc_cfg = &GFXSS_dc_config;
    GFXSS_config.gpu_cfg = &GFXSS_gpu_config;
    GFXSS_config.mipi_dsi_cfg = &GFXSS_mipi_dsi_config;
}

#if LCD_NEEDS_SCANOUT_BUFFER
static void lcd_vglite_buffer_init(vg_lite_buffer_t *buffer, void *memory,
                                   uint32_t width, uint32_t height, uint32_t stride_pixels)
{
    memset(buffer, 0, sizeof(*buffer));
    buffer->width = width;
    buffer->height = height;
    buffer->stride = stride_pixels * (LCD_BITS_PER_PIXEL / 8);
    buffer->format = VG_LITE_RGB565;
    buffer->tiled = VG_LITE_LINEAR;
    buffer->image_mode = VG_LITE_NORMAL_IMAGE_MODE;
    buffer->transparency_mode = VG_LITE_IMAGE_OPAQUE;
    buffer->memory = memory;
    buffer->address = (uint32_t)(uintptr_t)memory;
}

static vg_lite_error_t lcd_vglite_build_rotation_matrix(vg_lite_matrix_t *matrix,
                                                        uint32_t src_width,
                                                        uint32_t src_height)
{
    vg_lite_float_point4_t src;
    vg_lite_float_point4_t dst;
    const vg_lite_float_t logical_x_adjust = (vg_lite_float_t)LCD_LOGICAL_X_ADJUST_PIXELS;

    src[0].x = 0.0f;
    src[0].y = 0.0f;
    src[1].x = (vg_lite_float_t)src_width;
    src[1].y = 0.0f;
    src[2].x = (vg_lite_float_t)src_width;
    src[2].y = (vg_lite_float_t)src_height;
    src[3].x = 0.0f;
    src[3].y = (vg_lite_float_t)src_height;

#if LCD_ROTATION_DEGREES == 90
    dst[0].x = (vg_lite_float_t)LCD_WIDTH;
    dst[0].y = logical_x_adjust;
    dst[1].x = (vg_lite_float_t)LCD_WIDTH;
    dst[1].y = (vg_lite_float_t)LCD_HEIGHT + logical_x_adjust;
    dst[2].x = 0.0f;
    dst[2].y = (vg_lite_float_t)LCD_HEIGHT + logical_x_adjust;
    dst[3].x = 0.0f;
    dst[3].y = logical_x_adjust;
#elif LCD_ROTATION_DEGREES == 180
    dst[0].x = (vg_lite_float_t)LCD_WIDTH;
    dst[0].y = (vg_lite_float_t)LCD_HEIGHT;
    dst[1].x = 0.0f;
    dst[1].y = (vg_lite_float_t)LCD_HEIGHT;
    dst[2].x = 0.0f;
    dst[2].y = 0.0f;
    dst[3].x = (vg_lite_float_t)LCD_WIDTH;
    dst[3].y = 0.0f;
#elif LCD_ROTATION_DEGREES == 270
    dst[0].x = 0.0f;
    dst[0].y = (vg_lite_float_t)LCD_HEIGHT - logical_x_adjust;
    dst[1].x = 0.0f;
    dst[1].y = -logical_x_adjust;
    dst[2].x = (vg_lite_float_t)LCD_WIDTH;
    dst[2].y = -logical_x_adjust;
    dst[3].x = (vg_lite_float_t)LCD_WIDTH;
    dst[3].y = (vg_lite_float_t)LCD_HEIGHT - logical_x_adjust;
#else
    dst[0].x = 0.0f;
    dst[0].y = 0.0f;
    dst[1].x = (vg_lite_float_t)LCD_WIDTH;
    dst[1].y = 0.0f;
    dst[2].x = (vg_lite_float_t)LCD_WIDTH;
    dst[2].y = (vg_lite_float_t)LCD_HEIGHT;
    dst[3].x = 0.0f;
    dst[3].y = (vg_lite_float_t)LCD_HEIGHT;
#endif

    vg_lite_identity(matrix);
    return vg_lite_get_transform_matrix(src, dst, matrix);
}

static rt_bool_t lcd_vglite_rotate_to_scanout(const void *source_pixels,
                                              uint32_t source_width,
                                              uint32_t source_height,
                                              uint32_t source_stride)
{
    static rt_bool_t error_logged = RT_FALSE;
    vg_lite_buffer_t src;
    vg_lite_buffer_t dst;
    vg_lite_matrix_t matrix;
    vg_lite_error_t vg_status;

    lcd_vglite_buffer_init(&src, (void *)source_pixels,
                           source_width, source_height, source_stride);
    lcd_vglite_buffer_init(&dst, (void *)lcd_scanout_framebuffer(),
                           LCD_WIDTH, LCD_HEIGHT, LCD_STRIDE);

    vg_status = lcd_vglite_build_rotation_matrix(&matrix, source_width, source_height);
    if (vg_status != VG_LITE_SUCCESS)
    {
        if (!error_logged)
        {
            LOG_W("VG-Lite rotate matrix failed: %d", vg_status);
            error_logged = RT_TRUE;
        }
        return RT_FALSE;
    }

#if ((LCD_SCANOUT_X_ADJUST_PIXELS != 0) || (LCD_LOGICAL_X_ADJUST_PIXELS != 0))
    vg_status = vg_lite_clear(&dst, NULL, 0x00000000U);
    if (vg_status != VG_LITE_SUCCESS)
    {
        if (!error_logged)
        {
            LOG_W("VG-Lite clear failed: %d", vg_status);
            error_logged = RT_TRUE;
        }
        return RT_FALSE;
    }
#endif

    vg_status = vg_lite_blit(&dst, &src, &matrix, VG_LITE_BLEND_NONE, 0U, VG_LITE_FILTER_POINT);
    if (vg_status == VG_LITE_SUCCESS)
    {
        vg_status = vg_lite_finish();
    }

    if (vg_status != VG_LITE_SUCCESS)
    {
        if (!error_logged)
        {
            LOG_W("VG-Lite rotate %d failed: %d", LCD_ROTATION_DEGREES, vg_status);
            error_logged = RT_TRUE;
        }
        return RT_FALSE;
    }

    return RT_TRUE;
}
#endif

static void lcd_dcache_clean_range(const void *addr, uint32_t size)
{
    uintptr_t start = (uintptr_t)addr & ~((uintptr_t)__SCB_DCACHE_LINE_SIZE - 1U);
    uintptr_t end = ((uintptr_t)addr + size + __SCB_DCACHE_LINE_SIZE - 1U) &
                    ~((uintptr_t)__SCB_DCACHE_LINE_SIZE - 1U);

    if (size != 0U)
    {
        SCB_CleanDCache_by_Addr((void *)start, (int32_t)(end - start));
    }
}

static void lcd_dcache_invalidate_range(void *addr, uint32_t size)
{
    uintptr_t start = (uintptr_t)addr & ~((uintptr_t)__SCB_DCACHE_LINE_SIZE - 1U);
    uintptr_t end = ((uintptr_t)addr + size + __SCB_DCACHE_LINE_SIZE - 1U) &
                    ~((uintptr_t)__SCB_DCACHE_LINE_SIZE - 1U);

    if (size != 0U)
    {
        SCB_InvalidateDCache_by_Addr((void *)start, (int32_t)(end - start));
    }
}

#if LCD_USE_AXIDMAC_AREA_COPY
static rt_bool_t lcd_axidmac_copy_rgb565_area(const uint16_t *src_pixels,
                                              uint32_t src_stride,
                                              uint32_t dst_x,
                                              uint32_t dst_y,
                                              uint32_t width,
                                              uint32_t height)
{
    const uint32_t row_bytes = width * sizeof(uint16_t);
    const uint32_t total_bytes = row_bytes * height;
    const int32_t src_x_incr = (int32_t)(src_stride * sizeof(uint16_t));
    const int32_t dst_x_incr = (int32_t)(LCD_RENDER_STRIDE * sizeof(uint16_t));
    uint16_t *dst_pixels = lcd_render_framebuffer() + (dst_y * LCD_RENDER_STRIDE) + dst_x;
    uint32_t deadline;
    uint32_t intr;

    if ((src_pixels == RT_NULL) || (width == 0U) || (height == 0U))
    {
        return RT_FALSE;
    }

    if ((total_bytes < LCD_AXIDMAC_AREA_COPY_MIN_BYTES) ||
        (row_bytes > CY_AXIDMAC_LOOP_COUNT_MAX) ||
        (height > CY_AXIDMAC_LOOP_COUNT_MAX) ||
        (src_x_incr > CY_AXIDMAC_LOOP_INCREMENT_MAX) ||
        (dst_x_incr > CY_AXIDMAC_LOOP_INCREMENT_MAX))
    {
        return RT_FALSE;
    }

    if ((lcd_axidmac_lock != RT_NULL) &&
        (rt_mutex_take(lcd_axidmac_lock,
                       rt_tick_from_millisecond(LCD_AXIDMAC_TIMEOUT_MS)) != RT_EOK))
    {
        return RT_FALSE;
    }

    {
        cy_stc_axidmac_descriptor_config_t descriptor_config =
        {
            .retrigger = CY_AXIDMAC_RETRIG_IM,
            .interruptType = CY_AXIDMAC_DESCR,
            .triggerOutType = CY_AXIDMAC_DESCR,
            .channelState = CY_AXIDMAC_CHANNEL_DISABLED,
            .triggerInType = CY_AXIDMAC_DESCR,
            .dataPrefetch = true,
            .descriptorType = CY_AXIDMAC_2D_MEMORY_COPY,
            .srcAddress = (void *)src_pixels,
            .dstAddress = (void *)dst_pixels,
            .mCount = row_bytes,
            .srcXincrement = (int16_t)src_x_incr,
            .dstXincrement = (int16_t)dst_x_incr,
            .xCount = height,
            .srcYincrement = 0,
            .dstYincrement = 0,
            .yCount = 1U,
            .nextDescriptor = RT_NULL,
        };
        cy_stc_axidmac_channel_config_t channel_config =
        {
            .descriptor = &lcd_axidmac_descriptor,
            .priority = 2U,
            .enable = false,
            .bufferable = true,
        };

        Cy_AXIDMAC_Channel_Disable(SAXI_DMAC, LCD_AXIDMAC_CHANNEL);
        Cy_AXIDMAC_Channel_ClearInterrupt(SAXI_DMAC, LCD_AXIDMAC_CHANNEL, CY_AXIDMAC_INTR_MASK);

        if ((Cy_AXIDMAC_Descriptor_Init(&lcd_axidmac_descriptor, &descriptor_config) != CY_AXIDMAC_SUCCESS) ||
            (Cy_AXIDMAC_Channel_Init(SAXI_DMAC, LCD_AXIDMAC_CHANNEL, &channel_config) != CY_AXIDMAC_SUCCESS))
        {
            if (lcd_axidmac_lock != RT_NULL)
            {
                rt_mutex_release(lcd_axidmac_lock);
            }
            return RT_FALSE;
        }
    }

    lcd_dcache_clean_range(src_pixels, ((height - 1U) * src_stride * sizeof(uint16_t)) + row_bytes);
    lcd_dcache_clean_range(dst_pixels, ((height - 1U) * LCD_RENDER_STRIDE * sizeof(uint16_t)) + row_bytes);
    lcd_dcache_clean_range(&lcd_axidmac_descriptor, sizeof(lcd_axidmac_descriptor));

    Cy_AXIDMAC_Enable(SAXI_DMAC);
    Cy_AXIDMAC_Channel_Enable(SAXI_DMAC, LCD_AXIDMAC_CHANNEL);
    Cy_AXIDMAC_Channel_SetSwTrigger(SAXI_DMAC, LCD_AXIDMAC_CHANNEL);

    deadline = rt_tick_get() + rt_tick_from_millisecond(LCD_AXIDMAC_TIMEOUT_MS);
    do
    {
        intr = Cy_AXIDMAC_Channel_GetInterruptStatus(SAXI_DMAC, LCD_AXIDMAC_CHANNEL);
        if ((intr & LCD_AXIDMAC_ERROR_MASK) != 0U)
        {
            Cy_AXIDMAC_Channel_ClearInterrupt(SAXI_DMAC, LCD_AXIDMAC_CHANNEL, intr);
            Cy_AXIDMAC_Channel_Disable(SAXI_DMAC, LCD_AXIDMAC_CHANNEL);
            if (lcd_axidmac_lock != RT_NULL)
            {
                rt_mutex_release(lcd_axidmac_lock);
            }
            return RT_FALSE;
        }

        if ((intr & CY_AXIDMAC_INTR_COMPLETION) != 0U)
        {
            Cy_AXIDMAC_Channel_ClearInterrupt(SAXI_DMAC, LCD_AXIDMAC_CHANNEL, intr);
            lcd_dcache_invalidate_range(dst_pixels,
                                        ((height - 1U) * LCD_RENDER_STRIDE * sizeof(uint16_t)) + row_bytes);
            if (lcd_axidmac_lock != RT_NULL)
            {
                rt_mutex_release(lcd_axidmac_lock);
            }
            return RT_TRUE;
        }
    } while ((int32_t)(deadline - rt_tick_get()) > 0);

    Cy_AXIDMAC_Channel_Disable(SAXI_DMAC, LCD_AXIDMAC_CHANNEL);
    if (lcd_axidmac_lock != RT_NULL)
    {
        rt_mutex_release(lcd_axidmac_lock);
    }
    return RT_FALSE;
}
#endif

static void lcd_wait_present_done(void);

static void lcd_present_framebuffer(void)
{
    volatile cy_en_gfx_status_t status;

    lcd_dcache_clean_range(lcd_render_framebuffer(), LCD_RENDER_BUF_SIZE);
#if LCD_ROTATION_BACKEND_VGLITE
    lcd_dcache_invalidate_range(lcd_scanout_framebuffer(), LCD_SCANOUT_BUF_SIZE);
    if (!lcd_vglite_rotate_to_scanout(lcd_render_framebuffer(),
                                      LCD_LOGICAL_WIDTH,
                                      LCD_LOGICAL_HEIGHT,
                                      LCD_RENDER_STRIDE))
    {
        return;
    }
    lcd_dcache_invalidate_range(lcd_scanout_framebuffer(), LCD_SCANOUT_BUF_SIZE);
#else
    lcd_dcache_clean_range(lcd_scanout_framebuffer(), LCD_SCANOUT_BUF_SIZE);
#endif
    gfxbase->GFXSS_DC.DCNANO.GCREGFRAMEBUFFERSTRIDE = LCD_STRIDE * (LCD_BITS_PER_PIXEL / 8);
    status = Cy_GFXSS_Set_FrameBuffer(gfxbase, lcd_scanout_framebuffer(), &lcd_gfx_context);
    if (CY_GFX_SUCCESS != status)
    {
        LOG_E("[%s: %d] Image rendering failed. Error type: %u\r\n", __func__, __LINE__, status);
        CY_ASSERT(0);
    }
    lcd_apply_gfxss_rotation();
    lcd_wait_present_done();
}

void lcd_flush_rgb565(const void *pixels, uint32_t width, uint32_t height)
{
    const uint16_t *src = (const uint16_t *)pixels;
    uint16_t *dst = lcd_render_framebuffer();
    const uint32_t dst_x0 = LCD_WINDOW_X_PIXELS;
    const uint32_t dst_y0 = LCD_WINDOW_Y_PIXELS;

    if ((src == RT_NULL) || (width == 0U) || (height == 0U))
    {
        return;
    }

    if ((dst_x0 >= LCD_LOGICAL_WIDTH) || (dst_y0 >= LCD_LOGICAL_HEIGHT))
    {
        return;
    }

    memset(dst, 0, LCD_RENDER_BUF_SIZE);

    {
        uint32_t copy_width = (width > (LCD_LOGICAL_WIDTH - dst_x0)) ?
                              (LCD_LOGICAL_WIDTH - dst_x0) : width;
        uint32_t copy_height = (height > (LCD_LOGICAL_HEIGHT - dst_y0)) ?
                               (LCD_LOGICAL_HEIGHT - dst_y0) : height;

        for (uint32_t row = 0; row < copy_height; row++)
        {
            uint16_t *dst_row = dst + ((dst_y0 + row) * LCD_RENDER_STRIDE);
            const uint16_t *src_row = src + (row * width);

            memcpy(dst_row + dst_x0, src_row, copy_width * sizeof(uint16_t));
        }
    }

    lcd_present_framebuffer();
}

void lcd_flush_rgb565_area(const void *pixels, uint32_t x, uint32_t y,
                           uint32_t width, uint32_t height, uint32_t src_stride,
                           rt_bool_t present)
{
    const uint16_t *src = (const uint16_t *)pixels;
    uint16_t *dst = lcd_render_framebuffer();
    uint32_t dst_x0 = LCD_WINDOW_X_PIXELS + x;
    uint32_t dst_y0 = LCD_WINDOW_Y_PIXELS + y;
    uint32_t copy_width = width;
    uint32_t copy_height = height;

    if ((src == RT_NULL) || (width == 0U) || (height == 0U) || (src_stride == 0U))
    {
        return;
    }

    if (src_stride < width)
    {
        return;
    }

    if ((dst_x0 >= LCD_LOGICAL_WIDTH) || (dst_y0 >= LCD_LOGICAL_HEIGHT))
    {
        return;
    }

    if ((dst_x0 + copy_width) > LCD_LOGICAL_WIDTH)
    {
        copy_width = LCD_LOGICAL_WIDTH - dst_x0;
    }

    if ((dst_y0 + copy_height) > LCD_LOGICAL_HEIGHT)
    {
        copy_height = LCD_LOGICAL_HEIGHT - dst_y0;
    }

#if LCD_USE_AXIDMAC_AREA_COPY
    if (!lcd_axidmac_copy_rgb565_area(src, src_stride, dst_x0, dst_y0, copy_width, copy_height))
#endif
    {
        for (uint32_t row = 0; row < copy_height; row++)
        {
            uint16_t *dst_row = dst + ((dst_y0 + row) * LCD_RENDER_STRIDE);
            const uint16_t *src_row = src + (row * src_stride);

            memcpy(dst_row + dst_x0, src_row, copy_width * sizeof(uint16_t));
        }
    }

    if (present)
    {
        lcd_present_framebuffer();
    }
}

struct drv_lcd_device
{
    struct rt_device parent;

    struct rt_device_graphic_info lcd_info;

    struct rt_semaphore lcd_lock;

    /* 0:front_buf is being used 1: back_buf is being used*/
    rt_uint8_t cur_buf;
    rt_uint8_t *front_buf;
    rt_uint8_t *back_buf;
};

static void lcd_wait_present_done(void)
{
    (void)rt_sem_take(&_lcd.lcd_lock, RT_TICK_PER_SECOND / 20);
}

rt_err_t lcd_wait_frame_done(uint32_t timeout_ms)
{
    return rt_sem_take(&_lcd.lcd_lock, rt_tick_from_millisecond(timeout_ms));
}

#ifdef RT_USING_PWM
static struct rt_device_pwm *g_lcd_bl_pwm = RT_NULL;
static rt_err_t lcd_backlight_set(rt_uint8_t percent)
{
    rt_uint32_t pulse;

    if (percent > 100U)
    {
        percent = 100U;
    }

    if (g_lcd_bl_pwm == RT_NULL)
    {
        g_lcd_bl_pwm = (struct rt_device_pwm *)rt_device_find(LCD_BL_PWM_DEV_NAME);
        if (g_lcd_bl_pwm == RT_NULL)
        {
            LOG_W("Cannot find %s device", LCD_BL_PWM_DEV_NAME);
            return -RT_ENOSYS;
        }

        rt_pwm_enable(g_lcd_bl_pwm, LCD_BL_PWM_CHANNEL);
    }

    pulse = (LCD_BL_PWM_PERIOD_NS * percent) / 100U;
    rt_pwm_set(g_lcd_bl_pwm, LCD_BL_PWM_CHANNEL, LCD_BL_PWM_PERIOD_NS, pulse);
    return RT_EOK;
}
#endif

static rt_err_t drv_lcd_init(struct rt_device *device)
{
    (void)device;
    return RT_EOK;
}

static rt_err_t drv_lcd_control(struct rt_device *device, int cmd, void *args)
{
    struct drv_lcd_device *lcd = LCD_DEVICE(device);
    volatile cy_en_gfx_status_t status;
    switch (cmd)
    {
    case RTGRAPHIC_CTRL_RECT_UPDATE:
    {
        struct rt_device_rect_info *info = (struct rt_device_rect_info *)args;
        uint32_t start_line = 0U;
        uint32_t end_line = lcd->lcd_info.height;
        rt_bool_t try_partial = RT_FALSE;

        if (info != RT_NULL)
        {
            if ((info->width == 0U) || (info->height == 0U))
            {
                return RT_EOK;
            }
            if ((info->x >= lcd->lcd_info.width) || (info->y >= lcd->lcd_info.height))
            {
                return RT_EOK;
            }
            if ((info->x + info->width) > lcd->lcd_info.width)
            {
                info->width = lcd->lcd_info.width - info->x;
            }
            if ((info->y + info->height) > lcd->lcd_info.height)
            {
                info->height = lcd->lcd_info.height - info->y;
            }

            start_line = info->y;
            end_line = info->y + info->height;
            if ((info->x == 0U) && (info->width == lcd->lcd_info.width))
            {
                try_partial = RT_TRUE;
            }
        }

        if (try_partial &&
            ((lcd_gfx_context.dc_context.display_type == GFX_DISP_TYPE_DBI_A) ||
             (lcd_gfx_context.dc_context.display_type == GFX_DISP_TYPE_DBI_B) ||
             (lcd_gfx_context.dc_context.display_type == GFX_DISP_TYPE_DBI_C) ||
             (lcd_gfx_context.dc_context.display_type == GFX_DISP_TYPE_DSI_DBI)))
        {
            status = Cy_GFXSS_TransferPartialFrame(gfxbase, start_line, end_line, &lcd_gfx_context);
            if (CY_GFX_SUCCESS != status)
            {
                LOG_E("[%s: %d] Partial frame transfer failed. Error type: %u\r\n", __func__, __LINE__, status);
                CY_ASSERT(0);
            }
            break;
        }

        lcd_present_framebuffer();
    }
    break;

    case RTGRAPHIC_CTRL_GET_INFO:
    {
        struct rt_device_graphic_info *info = (struct rt_device_graphic_info *)args;

        RT_ASSERT(info != RT_NULL);
        info->pixel_format  = lcd->lcd_info.pixel_format;
        info->bits_per_pixel = 16;
        info->pitch         = lcd->lcd_info.pitch;
        info->width         = lcd->lcd_info.width;
        info->height        = lcd->lcd_info.height;
        info->framebuffer   = lcd->lcd_info.framebuffer;
        info->smem_len      = lcd->lcd_info.smem_len;
    }
    break;
    }

    return RT_EOK;
}

cy_stc_sysint_t dc_irq_cfg =
{
    .intrSrc = GFXSS_DC_IRQ,
    .intrPriority = 3U //DC_INT_PRIORITY
};
cy_stc_sysint_t gpu_irq_cfg =
{
    .intrSrc      = GFXSS_GPU_IRQ,
    .intrPriority = 3U //GPU_INT_PRIORITY
};

#ifdef PKG_USING_CPU_USAGE
uint32_t calculate_idle_percentage(void)
{
    static rt_bool_t cpu_usage_ready = RT_FALSE;
    uint32_t load;

    if (!cpu_usage_ready)
    {
        cpu_usage_init();
        cpu_usage_ready = RT_TRUE;
    }

    load = cpu_load_average();

    return 100 - load;
}
#endif

static void dc_irq_handler(void)
{
    rt_interrupt_enter();
    Cy_GFXSS_Clear_DC_Interrupt(gfxbase, &lcd_gfx_context);
    rt_sem_release(&_lcd.lcd_lock);
    rt_interrupt_leave();
}

static void gpu_irq_handler(void)
{
    rt_interrupt_enter();
    Cy_GFXSS_Clear_GPU_Interrupt(gfxbase, &lcd_gfx_context);
    vg_lite_IRQHandler();
    rt_interrupt_leave();
}

#if defined(BSP_USING_LVGL) || LCD_ROTATION_BACKEND_VGLITE
static rt_err_t lcd_vglite_init_once(void)
{
    static rt_bool_t vglite_ready = RT_FALSE;
    vg_lite_error_t vglite_status;
    vg_module_parameters_t vg_params;

    if (vglite_ready)
    {
        return RT_EOK;
    }

    vg_params.register_mem_base = (uint32_t)GFXSS_GFXSS_GPU_GCNANO;
    vg_params.gpu_mem_base[VG_PARAMS_POS] = GPU_MEM_BASE;
    vg_params.contiguous_mem_base[VG_PARAMS_POS] = (volatile void *)vglite_heap_base;
    vg_params.contiguous_mem_size[VG_PARAMS_POS] = VGLITE_HEAP_SIZE;
    vg_lite_init_mem(&vg_params);

    vglite_status = vg_lite_init((LCD_LOGICAL_WIDTH) / 4,
                                 (LCD_LOGICAL_HEIGHT) / 4);
    if (vglite_status != VG_LITE_SUCCESS)
    {
        LOG_E("VG-Lite init failed: %d", vglite_status);
        return -RT_ERROR;
    }

    LOG_I("Allocate GPU memory success\n");
    vglite_ready = RT_TRUE;
    return RT_EOK;
}
#endif

rt_err_t psoc_lcd_init(struct drv_lcd_device *lcd)
{
    /* 10.1" display pin configuration */
    mtb_display_tl043wvv02_pin_config_t tl043wvv02_pin_cfg =
    {
        .reset_port = DISPLAY_RESET_PORT,
        .reset_pin  = DISPLAY_RESET_PIN,
    };

    static cy_en_gfx_status_t gfx_status;
    static cy_en_sysint_status_t status;
    volatile cy_en_mipidsi_status_t mipi_status;

    lcd_apply_runtime_gfxss_config();

    /* Graphics subsystem initialization failed. Stop program execution */
    gfx_status = Cy_GFXSS_Init(gfxbase, &GFXSS_config, &lcd_gfx_context);
    if (CY_GFX_SUCCESS != gfx_status)
    {
        LOG_E("[%s: %d] Gfxss initialization failed. Error type: %u\r\n", __func__, __LINE__, status);
        return -RT_ERROR;
    }

    gfxbase->GFXSS_DC.DCNANO.GCREGFRAMEBUFFERSTRIDE = LCD_STRIDE * (LCD_BITS_PER_PIXEL / 8);
    /* Initialize GFXXs DC interrupt */
    status = Cy_SysInt_Init(&dc_irq_cfg, dc_irq_handler);
    if (CY_SYSINT_SUCCESS != status)
    {
        LOG_E("Error in registering DC interrupt: %d\r\n", status);
        return -RT_ERROR;
    }

    /* Enable interrupt in NVIC. */
    NVIC_EnableIRQ(GFXSS_DC_IRQ);

    status = Cy_SysInt_Init(&gpu_irq_cfg, gpu_irq_handler);
    if (CY_SYSINT_SUCCESS != status)
    {
        LOG_E("Error in registering GPU interrupt: %d\r\n", status);
        CY_ASSERT(0);
    }

    /* Enable GPU interrupt */
    Cy_GFXSS_Enable_GPU_Interrupt(GFXSS);

    /* Enable GFX GPU interrupt in NVIC. */
    NVIC_EnableIRQ(GFXSS_GPU_IRQ);

    mipi_status = mtb_display_tl043wvv02_init_without_display_on(GFXSS_GFXSS_MIPIDSI,
                                                                 &tl043wvv02_pin_cfg);

    /* Display initialization failed. Stop program execution */
    if (CY_MIPIDSI_SUCCESS != mipi_status)
    {
        LOG_E("[%s: %d] Display initialization failed. Error type: %u\r\n", __func__, __LINE__, status);
        return -RT_ERROR;
    }

    memset(lcd_render_framebuffer(), 0x00, LCD_RENDER_BUF_SIZE);
#if LCD_NEEDS_SCANOUT_BUFFER
    memset(lcd_scanout_framebuffer(), 0x00, LCD_SCANOUT_BUF_SIZE);
#endif
    SCB_CleanDCache();
    gfxbase->GFXSS_DC.DCNANO.GCREGFRAMEBUFFERSTRIDE = LCD_STRIDE * (LCD_BITS_PER_PIXEL / 8);
    gfx_status = Cy_GFXSS_Set_FrameBuffer(gfxbase, lcd_scanout_framebuffer(), &lcd_gfx_context);
    if (CY_GFX_SUCCESS != gfx_status)
    {
        LOG_E("[%s: %d] Initial framebuffer bind failed. Error type: %u\r\n", __func__, __LINE__, gfx_status);
        return -RT_ERROR;
    }
    lcd_apply_gfxss_rotation();

    mipi_status = mtb_display_tl043wvv02_display_on(GFXSS_GFXSS_MIPIDSI);
    if (CY_MIPIDSI_SUCCESS != mipi_status)
    {
        LOG_E("[%s: %d] Display ON failed. Error type: %u\r\n", __func__, __LINE__, mipi_status);
        return -RT_ERROR;
    }

    LOG_I("init screen success");
    return RT_EOK;
}

#ifdef RT_USING_DEVICE_OPS
const static struct rt_device_ops lcd_ops =
{
    drv_lcd_init,
    RT_NULL,
    RT_NULL,
    RT_NULL,
    RT_NULL,
    drv_lcd_control
};
#endif

int drv_lcd_hw_init(void)
{
    rt_err_t result = RT_EOK;
    struct rt_device *device = &_lcd.parent;

    /* memset _lcd to zero */
    memset(&_lcd, 0x00, sizeof(_lcd));

    /* init lcd_lock semaphore */
    result = rt_sem_init(&_lcd.lcd_lock, "lcd_lock", 0, RT_IPC_FLAG_FIFO);
    if (result != RT_EOK)
    {
        LOG_E("init semaphore failed!\n");
        result = -RT_ENOMEM;
        goto __exit;
    }

#if LCD_USE_AXIDMAC_AREA_COPY
    lcd_axidmac_lock = rt_mutex_create("lcd_dma", RT_IPC_FLAG_FIFO);
    if (lcd_axidmac_lock == RT_NULL)
    {
        LOG_E("init axidmac mutex failed!\n");
        result = -RT_ENOMEM;
        goto __exit;
    }
#endif

    /* config LCD dev info */
    _lcd.lcd_info.height = LCD_LOGICAL_HEIGHT;
    _lcd.lcd_info.width = LCD_LOGICAL_WIDTH;
    _lcd.lcd_info.bits_per_pixel = LCD_BITS_PER_PIXEL;
    _lcd.lcd_info.pixel_format = LCD_PIXEL_FORMAT;
    _lcd.lcd_info.pitch = LCD_RENDER_STRIDE * (LCD_BITS_PER_PIXEL / 8);
    _lcd.lcd_info.smem_len = LCD_RENDER_BUF_SIZE;

    /* malloc memory for Triple Buffering */
    // _lcd.front_buf=_lcd.lcd_info.framebuffer = rt_malloc_align(LCD_BUF_SIZE, 32);
    _lcd.front_buf = _lcd.lcd_info.framebuffer = (rt_uint8_t *)lcd_render_framebuffer();


    if (_lcd.lcd_info.framebuffer == RT_NULL)
    {
        LOG_E("init frame buffer failed!\n");
        result = -RT_ENOMEM;
        goto __exit;
    }
    /* memset buff to 0xFF */
    memset(_lcd.lcd_info.framebuffer, 0x00, LCD_RENDER_BUF_SIZE);
    device->type    = RT_Device_Class_Graphic;
#ifdef RT_USING_DEVICE_OPS
    device->ops     = &lcd_ops;
#else
    device->init    = drv_lcd_init;
#ifndef ART_PI_TouchGFX_LIB
    device->control = drv_lcd_control;
#endif
#endif

    /* register lcd device */
    rt_device_register(device, "lcd", RT_DEVICE_FLAG_RDWR);
    /* init mipi display */
    if (psoc_lcd_init(&_lcd) != RT_EOK)
    {
        result = -RT_ERROR;
        goto __exit;
    }
#ifdef RT_USING_PWM
    lcd_backlight_set(LCD_BL_DEFAULT_BRIGHTNESS);
#endif
#if defined(BSP_USING_LVGL) || LCD_ROTATION_BACKEND_VGLITE
    if (lcd_vglite_init_once() != RT_EOK)
    {
        result = -RT_ERROR;
        goto __exit;
    }
#endif
__exit:
    if (result != RT_EOK)
    {
#if LCD_USE_AXIDMAC_AREA_COPY
        if (lcd_axidmac_lock != RT_NULL)
        {
            rt_mutex_delete(lcd_axidmac_lock);
            lcd_axidmac_lock = RT_NULL;
        }
#endif
        rt_sem_delete(&_lcd.lcd_lock);
    }
    if (result == RT_EOK)
    {
        _lcd.parent.control(&_lcd.parent, RTGRAPHIC_CTRL_RECT_UPDATE, RT_NULL);
    }

    return result;
}
INIT_APP_EXPORT(drv_lcd_hw_init);


#ifdef DRV_DEBUG
#ifdef FINSH_USING_MSH
int lcd_test(void)
{
    struct drv_lcd_device *lcd;
    lcd = (struct drv_lcd_device *)rt_device_find("lcd");

    //while (1)
    {
        if (lcd->lcd_info.pixel_format == RTGRAPHIC_PIXEL_FORMAT_RGB565)
        {
            /* red */
            for (int i = 0; i < lcd->lcd_info.smem_len / 2; i++)
            {
                lcd->lcd_info.framebuffer[2 * i] = 0x00;
                lcd->lcd_info.framebuffer[2 * i + 1] = 0xF8;
            }
            lcd->parent.control(&lcd->parent, RTGRAPHIC_CTRL_RECT_UPDATE, RT_NULL);
            rt_thread_mdelay(1000);
            /* green */
            for (int i = 0; i < lcd->lcd_info.smem_len / 2; i++)
            {
                lcd->lcd_info.framebuffer[2 * i] = 0xE0;
                lcd->lcd_info.framebuffer[2 * i + 1] = 0x07;
            }
            lcd->parent.control(&lcd->parent, RTGRAPHIC_CTRL_RECT_UPDATE, RT_NULL);
            rt_thread_mdelay(1000);
            /* blue */
            for (int i = 0; i < lcd->lcd_info.smem_len / 2; i++)
            {
                lcd->lcd_info.framebuffer[2 * i] = 0x1F;
                lcd->lcd_info.framebuffer[2 * i + 1] = 0x00;
            }
            rt_thread_mdelay(1000);
            for (int i = 0; i < lcd->lcd_info.smem_len / 2; i++)
            {
                lcd->lcd_info.framebuffer[2 * i] = 0xFF;
                lcd->lcd_info.framebuffer[2 * i + 1] = 0xFF;
            }
        }
        else if (lcd->lcd_info.pixel_format == RTGRAPHIC_PIXEL_FORMAT_RGB888)
        {
            /* red */
            for (int i = 0; i < lcd->lcd_info.smem_len / 3; i++)
            {
                lcd->lcd_info.framebuffer[3 * i] = 0x00;
                lcd->lcd_info.framebuffer[3 * i + 1] = 0x00;
                lcd->lcd_info.framebuffer[3 * i + 2] = 0xff;
            }
            lcd->parent.control(&lcd->parent, RTGRAPHIC_CTRL_RECT_UPDATE, RT_NULL);
            rt_thread_mdelay(1000);
            /* green */
            for (int i = 0; i < lcd->lcd_info.smem_len / 3; i++)
            {
                lcd->lcd_info.framebuffer[3 * i] = 0x00;
                lcd->lcd_info.framebuffer[3 * i + 1] = 0xff;
                lcd->lcd_info.framebuffer[3 * i + 2] = 0x00;
            }
            lcd->parent.control(&lcd->parent, RTGRAPHIC_CTRL_RECT_UPDATE, RT_NULL);
            rt_thread_mdelay(1000);
            /* blue */
            for (int i = 0; i < lcd->lcd_info.smem_len / 3; i++)
            {
                lcd->lcd_info.framebuffer[3 * i] = 0xff;
                lcd->lcd_info.framebuffer[3 * i + 1] = 0x00;
                lcd->lcd_info.framebuffer[3 * i + 2] = 0x00;
            }
        }
        rt_thread_mdelay(1000);
    }
    return 0;
}
MSH_CMD_EXPORT(lcd_test, lcd_test);
#endif /* FINSH_USING_MSH */
#endif /* DRV_DEBUG */
#endif
