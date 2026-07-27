/**
 * @file lv_draw_sw_mve_custom.h
 *
 * Local Cortex-M55/MVE fast paths for LVGL's software renderer.
 */

#ifndef LV_DRAW_SW_MVE_CUSTOM_H
#define LV_DRAW_SW_MVE_CUSTOM_H

#include <stdint.h>

#if defined(__ARM_FEATURE_MVE) && __ARM_FEATURE_MVE

#include <arm_mve.h>

#include "draw/sw/blend/lv_draw_sw_blend_private.h"

#ifndef LV_DRAW_SW_RGB565_SWAP
#define LV_DRAW_SW_RGB565_SWAP(buf, buf_size_px) \
    lv_draw_sw_mve_rgb565_swap((buf), (buf_size_px))
#endif

#ifndef LV_DRAW_SW_COLOR_BLEND_TO_RGB565
#define LV_DRAW_SW_COLOR_BLEND_TO_RGB565(dsc) \
    lv_draw_sw_mve_color_blend_to_rgb565((dsc))
#endif

#ifndef LV_DRAW_SW_COLOR_BLEND_TO_RGB565_WITH_OPA
#define LV_DRAW_SW_COLOR_BLEND_TO_RGB565_WITH_OPA(dsc) \
    lv_draw_sw_mve_color_blend_to_rgb565_with_opa((dsc))
#endif

#ifndef LV_DRAW_SW_COLOR_BLEND_TO_RGB565_WITH_MASK
#define LV_DRAW_SW_COLOR_BLEND_TO_RGB565_WITH_MASK(dsc) \
    lv_draw_sw_mve_color_blend_to_rgb565_with_mask((dsc))
#endif

#ifndef LV_DRAW_SW_COLOR_BLEND_TO_RGB565_MIX_MASK_OPA
#define LV_DRAW_SW_COLOR_BLEND_TO_RGB565_MIX_MASK_OPA(dsc) \
    lv_draw_sw_mve_color_blend_to_rgb565_mix_mask_opa((dsc))
#endif

#ifndef LV_DRAW_SW_RGB565_BLEND_NORMAL_TO_RGB565
#define LV_DRAW_SW_RGB565_BLEND_NORMAL_TO_RGB565(dsc) \
    lv_draw_sw_mve_rgb565_blend_normal_to_rgb565((dsc))
#endif

#ifndef LV_DRAW_SW_RGB565_BLEND_NORMAL_TO_RGB565_WITH_OPA
#define LV_DRAW_SW_RGB565_BLEND_NORMAL_TO_RGB565_WITH_OPA(dsc) \
    lv_draw_sw_mve_rgb565_blend_normal_to_rgb565_with_opa((dsc))
#endif

#ifndef LV_DRAW_SW_RGB565_BLEND_NORMAL_TO_RGB565_WITH_MASK
#define LV_DRAW_SW_RGB565_BLEND_NORMAL_TO_RGB565_WITH_MASK(dsc) \
    lv_draw_sw_mve_rgb565_blend_normal_to_rgb565_with_mask((dsc))
#endif

#ifndef LV_DRAW_SW_RGB565_BLEND_NORMAL_TO_RGB565_MIX_MASK_OPA
#define LV_DRAW_SW_RGB565_BLEND_NORMAL_TO_RGB565_MIX_MASK_OPA(dsc) \
    lv_draw_sw_mve_rgb565_blend_normal_to_rgb565_mix_mask_opa((dsc))
#endif

#ifndef LV_DRAW_SW_RGB888_BLEND_NORMAL_TO_RGB565
#define LV_DRAW_SW_RGB888_BLEND_NORMAL_TO_RGB565(dsc, src_px_size) \
    lv_draw_sw_mve_rgb888_blend_normal_to_rgb565((dsc), (src_px_size))
#endif

#ifndef LV_DRAW_SW_RGB888_BLEND_NORMAL_TO_RGB565_WITH_OPA
#define LV_DRAW_SW_RGB888_BLEND_NORMAL_TO_RGB565_WITH_OPA(dsc, src_px_size) \
    lv_draw_sw_mve_rgb888_blend_normal_to_rgb565_with_opa((dsc), (src_px_size))
#endif

#ifndef LV_DRAW_SW_RGB888_BLEND_NORMAL_TO_RGB565_WITH_MASK
#define LV_DRAW_SW_RGB888_BLEND_NORMAL_TO_RGB565_WITH_MASK(dsc, src_px_size) \
    lv_draw_sw_mve_rgb888_blend_normal_to_rgb565_with_mask((dsc), (src_px_size))
#endif

#ifndef LV_DRAW_SW_RGB888_BLEND_NORMAL_TO_RGB565_MIX_MASK_OPA
#define LV_DRAW_SW_RGB888_BLEND_NORMAL_TO_RGB565_MIX_MASK_OPA(dsc, src_px_size) \
    lv_draw_sw_mve_rgb888_blend_normal_to_rgb565_mix_mask_opa((dsc), (src_px_size))
#endif

#ifndef LV_DRAW_SW_ARGB8888_BLEND_NORMAL_TO_RGB565
#define LV_DRAW_SW_ARGB8888_BLEND_NORMAL_TO_RGB565(dsc) \
    lv_draw_sw_mve_argb8888_blend_normal_to_rgb565((dsc))
#endif

#ifndef LV_DRAW_SW_ARGB8888_BLEND_NORMAL_TO_RGB565_WITH_OPA
#define LV_DRAW_SW_ARGB8888_BLEND_NORMAL_TO_RGB565_WITH_OPA(dsc) \
    lv_draw_sw_mve_argb8888_blend_normal_to_rgb565_with_opa((dsc))
#endif

#ifndef LV_DRAW_SW_ARGB8888_BLEND_NORMAL_TO_RGB565_WITH_MASK
#define LV_DRAW_SW_ARGB8888_BLEND_NORMAL_TO_RGB565_WITH_MASK(dsc) \
    lv_draw_sw_mve_argb8888_blend_normal_to_rgb565_with_mask((dsc))
#endif

#ifndef LV_DRAW_SW_ARGB8888_BLEND_NORMAL_TO_RGB565_MIX_MASK_OPA
#define LV_DRAW_SW_ARGB8888_BLEND_NORMAL_TO_RGB565_MIX_MASK_OPA(dsc) \
    lv_draw_sw_mve_argb8888_blend_normal_to_rgb565_mix_mask_opa((dsc))
#endif

static inline uint16_t lv_draw_sw_mve_rgb888_to_rgb565_scalar(const uint8_t * src)
{
    return (uint16_t)(((src[2] & 0xf8U) << 8) |
                      ((src[1] & 0xfcU) << 3) |
                      ((src[0] & 0xf8U) >> 3));
}

static inline uint16x8_t lv_draw_sw_mve_opa_mix2_q(uint16x8_t a, uint16x8_t b)
{
    return vshrq_n_u16(vmulq_u16(a, b), 8);
}

static inline uint16x8_t lv_draw_sw_mve_opa_mix3_q(uint16x8_t a, uint16x8_t b, uint8_t c)
{
    uint16x8_t ab = lv_draw_sw_mve_opa_mix2_q(a, b);
    return lv_draw_sw_mve_opa_mix2_q(ab, vdupq_n_u16(c));
}

static inline uint16x8_t lv_draw_sw_mve_rgb565_mix_q(uint16x8_t src, uint16x8_t dst, uint8_t opa)
{
    int16x8_t mix = vdupq_n_s16((int16_t)(((uint32_t)opa + 4U) >> 3));

    uint16x8_t mask_r_b = vdupq_n_u16(0x1f);
    uint16x8_t mask_g = vdupq_n_u16(0x3f);

    int16x8_t src_r = vreinterpretq_s16_u16(vandq_u16(vshrq_n_u16(src, 11), mask_r_b));
    int16x8_t src_g = vreinterpretq_s16_u16(vandq_u16(vshrq_n_u16(src, 5), mask_g));
    int16x8_t src_b = vreinterpretq_s16_u16(vandq_u16(src, mask_r_b));

    int16x8_t dst_r = vreinterpretq_s16_u16(vandq_u16(vshrq_n_u16(dst, 11), mask_r_b));
    int16x8_t dst_g = vreinterpretq_s16_u16(vandq_u16(vshrq_n_u16(dst, 5), mask_g));
    int16x8_t dst_b = vreinterpretq_s16_u16(vandq_u16(dst, mask_r_b));

    int16x8_t r = vaddq_s16(dst_r, vshrq_n_s16(vmulq_s16(vsubq_s16(src_r, dst_r), mix), 5));
    int16x8_t g = vaddq_s16(dst_g, vshrq_n_s16(vmulq_s16(vsubq_s16(src_g, dst_g), mix), 5));
    int16x8_t b = vaddq_s16(dst_b, vshrq_n_s16(vmulq_s16(vsubq_s16(src_b, dst_b), mix), 5));

    return vorrq_u16(vorrq_u16(vshlq_n_u16(vreinterpretq_u16_s16(r), 11),
                               vshlq_n_u16(vreinterpretq_u16_s16(g), 5)),
                     vreinterpretq_u16_s16(b));
}

static inline uint16x8_t lv_draw_sw_mve_rgb565_mix_opa_q(uint16x8_t src, uint16x8_t dst, uint16x8_t opa)
{
    int16x8_t mix = vreinterpretq_s16_u16(vshrq_n_u16(vaddq_u16(opa, vdupq_n_u16(4)), 3));

    uint16x8_t mask_r_b = vdupq_n_u16(0x1f);
    uint16x8_t mask_g = vdupq_n_u16(0x3f);

    int16x8_t src_r = vreinterpretq_s16_u16(vandq_u16(vshrq_n_u16(src, 11), mask_r_b));
    int16x8_t src_g = vreinterpretq_s16_u16(vandq_u16(vshrq_n_u16(src, 5), mask_g));
    int16x8_t src_b = vreinterpretq_s16_u16(vandq_u16(src, mask_r_b));

    int16x8_t dst_r = vreinterpretq_s16_u16(vandq_u16(vshrq_n_u16(dst, 11), mask_r_b));
    int16x8_t dst_g = vreinterpretq_s16_u16(vandq_u16(vshrq_n_u16(dst, 5), mask_g));
    int16x8_t dst_b = vreinterpretq_s16_u16(vandq_u16(dst, mask_r_b));

    int16x8_t r = vaddq_s16(dst_r, vshrq_n_s16(vmulq_s16(vsubq_s16(src_r, dst_r), mix), 5));
    int16x8_t g = vaddq_s16(dst_g, vshrq_n_s16(vmulq_s16(vsubq_s16(src_g, dst_g), mix), 5));
    int16x8_t b = vaddq_s16(dst_b, vshrq_n_s16(vmulq_s16(vsubq_s16(src_b, dst_b), mix), 5));

    return vorrq_u16(vorrq_u16(vshlq_n_u16(vreinterpretq_u16_s16(r), 11),
                               vshlq_n_u16(vreinterpretq_u16_s16(g), 5)),
                     vreinterpretq_u16_s16(b));
}

static inline uint16x8_t lv_draw_sw_mve_rgb888_to_rgb565_q(const uint8_t * src, uint32_t px_size)
{
    uint16x8_t offsets3_b = {0, 3, 6, 9, 12, 15, 18, 21};
    uint16x8_t offsets3_g = {1, 4, 7, 10, 13, 16, 19, 22};
    uint16x8_t offsets3_r = {2, 5, 8, 11, 14, 17, 20, 23};
    uint16x8_t offsets4_b = {0, 4, 8, 12, 16, 20, 24, 28};
    uint16x8_t offsets4_g = {1, 5, 9, 13, 17, 21, 25, 29};
    uint16x8_t offsets4_r = {2, 6, 10, 14, 18, 22, 26, 30};
    uint16x8_t mask_r_b = vdupq_n_u16(0xf8);
    uint16x8_t mask_g = vdupq_n_u16(0xfc);

    uint16x8_t b = vldrbq_gather_offset_u16(src, px_size == 4U ? offsets4_b : offsets3_b);
    uint16x8_t g = vldrbq_gather_offset_u16(src, px_size == 4U ? offsets4_g : offsets3_g);
    uint16x8_t r = vldrbq_gather_offset_u16(src, px_size == 4U ? offsets4_r : offsets3_r);

    return vorrq_u16(vorrq_u16(vshlq_n_u16(vandq_u16(r, mask_r_b), 8),
                               vshlq_n_u16(vandq_u16(g, mask_g), 3)),
                     vshrq_n_u16(vandq_u16(b, mask_r_b), 3));
}

static inline uint16x8_t lv_draw_sw_mve_argb8888_alpha_q(const uint8_t * src)
{
    uint16x8_t offsets_a = {3, 7, 11, 15, 19, 23, 27, 31};
    return vldrbq_gather_offset_u16(src, offsets_a);
}

static inline lv_result_t lv_draw_sw_mve_rgb565_swap(void * buf, uint32_t buf_size_px)
{
    uint8_t * buf8 = (uint8_t *)buf;
    uint32_t bytes = buf_size_px * 2U;

    while(bytes >= 16U) {
        uint8x16_t v = vld1q_u8(buf8);
        v = vrev16q_u8(v);
        vst1q_u8(buf8, v);
        buf8 += 16;
        bytes -= 16U;
    }

    while(bytes >= 2U) {
        uint8_t tmp = buf8[0];
        buf8[0] = buf8[1];
        buf8[1] = tmp;
        buf8 += 2;
        bytes -= 2U;
    }

    return LV_RESULT_OK;
}

static inline lv_result_t lv_draw_sw_mve_color_blend_to_rgb565_with_mask(lv_draw_sw_blend_fill_dsc_t * dsc)
{
    if(dsc->mask_buf == NULL || dsc->opa < LV_OPA_MAX) return LV_RESULT_INVALID;

    uint16x8_t color_q = vdupq_n_u16(lv_color_to_u16(dsc->color));
    uint8_t * row = (uint8_t *)dsc->dest_buf;
    const lv_opa_t * mask = dsc->mask_buf;

    for(int32_t y = 0; y < dsc->dest_h; y++) {
        uint16_t * dst = (uint16_t *)row;
        int32_t x = 0;

        for(; x <= dsc->dest_w - 8; x += 8) {
            uint16x8_t mask_q = vldrbq_u16(&mask[x]);
            uint16x8_t dst_q = vld1q_u16(&dst[x]);
            vst1q_u16(&dst[x], lv_draw_sw_mve_rgb565_mix_opa_q(color_q, dst_q, mask_q));
        }

        for(; x < dsc->dest_w; x++) {
            dst[x] = lv_color_16_16_mix(lv_color_to_u16(dsc->color), dst[x], mask[x]);
        }

        row += dsc->dest_stride;
        mask += dsc->mask_stride;
    }

    return LV_RESULT_OK;
}

static inline lv_result_t lv_draw_sw_mve_color_blend_to_rgb565_mix_mask_opa(lv_draw_sw_blend_fill_dsc_t * dsc)
{
    if(dsc->mask_buf == NULL || dsc->opa >= LV_OPA_MAX) return LV_RESULT_INVALID;

    uint16x8_t color_q = vdupq_n_u16(lv_color_to_u16(dsc->color));
    uint16x8_t opa_q = vdupq_n_u16(dsc->opa);
    uint8_t * row = (uint8_t *)dsc->dest_buf;
    const lv_opa_t * mask = dsc->mask_buf;

    for(int32_t y = 0; y < dsc->dest_h; y++) {
        uint16_t * dst = (uint16_t *)row;
        int32_t x = 0;

        for(; x <= dsc->dest_w - 8; x += 8) {
            uint16x8_t mix_q = lv_draw_sw_mve_opa_mix2_q(vldrbq_u16(&mask[x]), opa_q);
            uint16x8_t dst_q = vld1q_u16(&dst[x]);
            vst1q_u16(&dst[x], lv_draw_sw_mve_rgb565_mix_opa_q(color_q, dst_q, mix_q));
        }

        for(; x < dsc->dest_w; x++) {
            dst[x] = lv_color_16_16_mix(lv_color_to_u16(dsc->color), dst[x], LV_OPA_MIX2(mask[x], dsc->opa));
        }

        row += dsc->dest_stride;
        mask += dsc->mask_stride;
    }

    return LV_RESULT_OK;
}

static inline lv_result_t lv_draw_sw_mve_color_blend_to_rgb565(lv_draw_sw_blend_fill_dsc_t * dsc)
{
    if(dsc->mask_buf != NULL || dsc->opa < LV_OPA_MAX) return LV_RESULT_INVALID;

    uint16_t color16 = lv_color_to_u16(dsc->color);
    uint16x8_t color_q = vdupq_n_u16(color16);
    uint8_t * row = (uint8_t *)dsc->dest_buf;

    for(int32_t y = 0; y < dsc->dest_h; y++) {
        uint16_t * dst = (uint16_t *)row;
        int32_t x = 0;

        for(; x <= dsc->dest_w - 8; x += 8) {
            vst1q_u16(&dst[x], color_q);
        }

        for(; x < dsc->dest_w; x++) {
            dst[x] = color16;
        }

        row += dsc->dest_stride;
    }

    return LV_RESULT_OK;
}

static inline lv_result_t lv_draw_sw_mve_color_blend_to_rgb565_with_opa(lv_draw_sw_blend_fill_dsc_t * dsc)
{
    if(dsc->mask_buf != NULL || dsc->opa >= LV_OPA_MAX || dsc->opa == LV_OPA_MIN) return LV_RESULT_INVALID;

    uint16x8_t color_q = vdupq_n_u16(lv_color_to_u16(dsc->color));
    uint8_t * row = (uint8_t *)dsc->dest_buf;

    for(int32_t y = 0; y < dsc->dest_h; y++) {
        uint16_t * dst = (uint16_t *)row;
        int32_t x = 0;

        for(; x <= dsc->dest_w - 8; x += 8) {
            uint16x8_t dst_q = vld1q_u16(&dst[x]);
            vst1q_u16(&dst[x], lv_draw_sw_mve_rgb565_mix_q(color_q, dst_q, dsc->opa));
        }

        for(; x < dsc->dest_w; x++) {
            dst[x] = lv_color_16_16_mix(lv_color_to_u16(dsc->color), dst[x], dsc->opa);
        }

        row += dsc->dest_stride;
    }

    return LV_RESULT_OK;
}

static inline lv_result_t lv_draw_sw_mve_rgb565_blend_normal_to_rgb565_with_mask(lv_draw_sw_blend_image_dsc_t * dsc)
{
    if(dsc->mask_buf == NULL || dsc->opa < LV_OPA_MAX || dsc->blend_mode != LV_BLEND_MODE_NORMAL) {
        return LV_RESULT_INVALID;
    }

    uint8_t * dst_row = (uint8_t *)dsc->dest_buf;
    const uint8_t * src_row = (const uint8_t *)dsc->src_buf;
    const lv_opa_t * mask = dsc->mask_buf;

    for(int32_t y = 0; y < dsc->dest_h; y++) {
        uint16_t * dst = (uint16_t *)dst_row;
        const uint16_t * src = (const uint16_t *)src_row;
        int32_t x = 0;

        for(; x <= dsc->dest_w - 8; x += 8) {
            uint16x8_t src_q = vld1q_u16(&src[x]);
            uint16x8_t dst_q = vld1q_u16(&dst[x]);
            uint16x8_t mask_q = vldrbq_u16(&mask[x]);
            vst1q_u16(&dst[x], lv_draw_sw_mve_rgb565_mix_opa_q(src_q, dst_q, mask_q));
        }

        for(; x < dsc->dest_w; x++) {
            dst[x] = lv_color_16_16_mix(src[x], dst[x], mask[x]);
        }

        dst_row += dsc->dest_stride;
        src_row += dsc->src_stride;
        mask += dsc->mask_stride;
    }

    return LV_RESULT_OK;
}

static inline lv_result_t lv_draw_sw_mve_rgb565_blend_normal_to_rgb565_mix_mask_opa(lv_draw_sw_blend_image_dsc_t * dsc)
{
    if(dsc->mask_buf == NULL || dsc->opa >= LV_OPA_MAX || dsc->blend_mode != LV_BLEND_MODE_NORMAL) {
        return LV_RESULT_INVALID;
    }

    uint8_t * dst_row = (uint8_t *)dsc->dest_buf;
    const uint8_t * src_row = (const uint8_t *)dsc->src_buf;
    const lv_opa_t * mask = dsc->mask_buf;
    uint16x8_t opa_q = vdupq_n_u16(dsc->opa);

    for(int32_t y = 0; y < dsc->dest_h; y++) {
        uint16_t * dst = (uint16_t *)dst_row;
        const uint16_t * src = (const uint16_t *)src_row;
        int32_t x = 0;

        for(; x <= dsc->dest_w - 8; x += 8) {
            uint16x8_t src_q = vld1q_u16(&src[x]);
            uint16x8_t dst_q = vld1q_u16(&dst[x]);
            uint16x8_t mix_q = lv_draw_sw_mve_opa_mix2_q(vldrbq_u16(&mask[x]), opa_q);
            vst1q_u16(&dst[x], lv_draw_sw_mve_rgb565_mix_opa_q(src_q, dst_q, mix_q));
        }

        for(; x < dsc->dest_w; x++) {
            dst[x] = lv_color_16_16_mix(src[x], dst[x], LV_OPA_MIX2(mask[x], dsc->opa));
        }

        dst_row += dsc->dest_stride;
        src_row += dsc->src_stride;
        mask += dsc->mask_stride;
    }

    return LV_RESULT_OK;
}

static inline lv_result_t lv_draw_sw_mve_rgb565_blend_normal_to_rgb565(lv_draw_sw_blend_image_dsc_t * dsc)
{
    if(dsc->mask_buf != NULL || dsc->opa < LV_OPA_MAX || dsc->blend_mode != LV_BLEND_MODE_NORMAL) {
        return LV_RESULT_INVALID;
    }

    uint8_t * dst_row = (uint8_t *)dsc->dest_buf;
    const uint8_t * src_row = (const uint8_t *)dsc->src_buf;

    for(int32_t y = 0; y < dsc->dest_h; y++) {
        uint16_t * dst = (uint16_t *)dst_row;
        const uint16_t * src = (const uint16_t *)src_row;
        int32_t x = 0;

        for(; x <= dsc->dest_w - 8; x += 8) {
            vst1q_u16(&dst[x], vld1q_u16(&src[x]));
        }

        for(; x < dsc->dest_w; x++) {
            dst[x] = src[x];
        }

        dst_row += dsc->dest_stride;
        src_row += dsc->src_stride;
    }

    return LV_RESULT_OK;
}

static inline lv_result_t lv_draw_sw_mve_rgb565_blend_normal_to_rgb565_with_opa(lv_draw_sw_blend_image_dsc_t * dsc)
{
    if(dsc->mask_buf != NULL || dsc->opa >= LV_OPA_MAX || dsc->opa == LV_OPA_MIN ||
       dsc->blend_mode != LV_BLEND_MODE_NORMAL) {
        return LV_RESULT_INVALID;
    }

    uint8_t * dst_row = (uint8_t *)dsc->dest_buf;
    const uint8_t * src_row = (const uint8_t *)dsc->src_buf;

    for(int32_t y = 0; y < dsc->dest_h; y++) {
        uint16_t * dst = (uint16_t *)dst_row;
        const uint16_t * src = (const uint16_t *)src_row;
        int32_t x = 0;

        for(; x <= dsc->dest_w - 8; x += 8) {
            uint16x8_t src_q = vld1q_u16(&src[x]);
            uint16x8_t dst_q = vld1q_u16(&dst[x]);
            vst1q_u16(&dst[x], lv_draw_sw_mve_rgb565_mix_q(src_q, dst_q, dsc->opa));
        }

        for(; x < dsc->dest_w; x++) {
            dst[x] = lv_color_16_16_mix(src[x], dst[x], dsc->opa);
        }

        dst_row += dsc->dest_stride;
        src_row += dsc->src_stride;
    }

    return LV_RESULT_OK;
}

static inline lv_result_t lv_draw_sw_mve_rgb888_blend_normal_to_rgb565(lv_draw_sw_blend_image_dsc_t * dsc,
                                                                       uint32_t src_px_size)
{
    if(dsc->mask_buf != NULL || dsc->opa < LV_OPA_MAX || dsc->blend_mode != LV_BLEND_MODE_NORMAL ||
       (src_px_size != 3U && src_px_size != 4U)) {
        return LV_RESULT_INVALID;
    }

    uint8_t * dst_row = (uint8_t *)dsc->dest_buf;
    const uint8_t * src_row = (const uint8_t *)dsc->src_buf;

    for(int32_t y = 0; y < dsc->dest_h; y++) {
        uint16_t * dst = (uint16_t *)dst_row;
        const uint8_t * src = src_row;
        int32_t x = 0;

        for(; x <= dsc->dest_w - 8; x += 8) {
            vst1q_u16(&dst[x], lv_draw_sw_mve_rgb888_to_rgb565_q(src + x * src_px_size, src_px_size));
        }

        for(; x < dsc->dest_w; x++) {
            dst[x] = lv_draw_sw_mve_rgb888_to_rgb565_scalar(src + x * src_px_size);
        }

        dst_row += dsc->dest_stride;
        src_row += dsc->src_stride;
    }

    return LV_RESULT_OK;
}

static inline lv_result_t lv_draw_sw_mve_rgb888_blend_normal_to_rgb565_with_opa(lv_draw_sw_blend_image_dsc_t * dsc,
                                                                                uint32_t src_px_size)
{
    if(dsc->mask_buf != NULL || dsc->opa >= LV_OPA_MAX || dsc->blend_mode != LV_BLEND_MODE_NORMAL ||
       (src_px_size != 3U && src_px_size != 4U)) {
        return LV_RESULT_INVALID;
    }

    uint8_t * dst_row = (uint8_t *)dsc->dest_buf;
    const uint8_t * src_row = (const uint8_t *)dsc->src_buf;

    for(int32_t y = 0; y < dsc->dest_h; y++) {
        uint16_t * dst = (uint16_t *)dst_row;
        const uint8_t * src = src_row;
        int32_t x = 0;

        for(; x <= dsc->dest_w - 8; x += 8) {
            uint16x8_t src_q = lv_draw_sw_mve_rgb888_to_rgb565_q(src + x * src_px_size, src_px_size);
            uint16x8_t dst_q = vld1q_u16(&dst[x]);
            vst1q_u16(&dst[x], lv_draw_sw_mve_rgb565_mix_q(src_q, dst_q, dsc->opa));
        }

        for(; x < dsc->dest_w; x++) {
            uint16_t c = lv_draw_sw_mve_rgb888_to_rgb565_scalar(src + x * src_px_size);
            dst[x] = lv_color_16_16_mix(c, dst[x], dsc->opa);
        }

        dst_row += dsc->dest_stride;
        src_row += dsc->src_stride;
    }

    return LV_RESULT_OK;
}

static inline lv_result_t lv_draw_sw_mve_rgb888_blend_normal_to_rgb565_with_mask(lv_draw_sw_blend_image_dsc_t * dsc,
                                                                                 uint32_t src_px_size)
{
    if(dsc->mask_buf == NULL || dsc->opa < LV_OPA_MAX || dsc->blend_mode != LV_BLEND_MODE_NORMAL ||
       (src_px_size != 3U && src_px_size != 4U)) {
        return LV_RESULT_INVALID;
    }

    uint8_t * dst_row = (uint8_t *)dsc->dest_buf;
    const uint8_t * src_row = (const uint8_t *)dsc->src_buf;
    const lv_opa_t * mask = dsc->mask_buf;

    for(int32_t y = 0; y < dsc->dest_h; y++) {
        uint16_t * dst = (uint16_t *)dst_row;
        const uint8_t * src = src_row;
        int32_t x = 0;

        for(; x <= dsc->dest_w - 8; x += 8) {
            uint16x8_t src_q = lv_draw_sw_mve_rgb888_to_rgb565_q(src + x * src_px_size, src_px_size);
            uint16x8_t dst_q = vld1q_u16(&dst[x]);
            uint16x8_t mask_q = vldrbq_u16(&mask[x]);
            vst1q_u16(&dst[x], lv_draw_sw_mve_rgb565_mix_opa_q(src_q, dst_q, mask_q));
        }

        for(; x < dsc->dest_w; x++) {
            uint16_t c = lv_draw_sw_mve_rgb888_to_rgb565_scalar(src + x * src_px_size);
            dst[x] = lv_color_16_16_mix(c, dst[x], mask[x]);
        }

        dst_row += dsc->dest_stride;
        src_row += dsc->src_stride;
        mask += dsc->mask_stride;
    }

    return LV_RESULT_OK;
}

static inline lv_result_t lv_draw_sw_mve_rgb888_blend_normal_to_rgb565_mix_mask_opa(lv_draw_sw_blend_image_dsc_t * dsc,
                                                                                    uint32_t src_px_size)
{
    if(dsc->mask_buf == NULL || dsc->opa >= LV_OPA_MAX || dsc->blend_mode != LV_BLEND_MODE_NORMAL ||
       (src_px_size != 3U && src_px_size != 4U)) {
        return LV_RESULT_INVALID;
    }

    uint8_t * dst_row = (uint8_t *)dsc->dest_buf;
    const uint8_t * src_row = (const uint8_t *)dsc->src_buf;
    const lv_opa_t * mask = dsc->mask_buf;
    uint16x8_t opa_q = vdupq_n_u16(dsc->opa);

    for(int32_t y = 0; y < dsc->dest_h; y++) {
        uint16_t * dst = (uint16_t *)dst_row;
        const uint8_t * src = src_row;
        int32_t x = 0;

        for(; x <= dsc->dest_w - 8; x += 8) {
            uint16x8_t src_q = lv_draw_sw_mve_rgb888_to_rgb565_q(src + x * src_px_size, src_px_size);
            uint16x8_t dst_q = vld1q_u16(&dst[x]);
            uint16x8_t mix_q = lv_draw_sw_mve_opa_mix2_q(vldrbq_u16(&mask[x]), opa_q);
            vst1q_u16(&dst[x], lv_draw_sw_mve_rgb565_mix_opa_q(src_q, dst_q, mix_q));
        }

        for(; x < dsc->dest_w; x++) {
            uint16_t c = lv_draw_sw_mve_rgb888_to_rgb565_scalar(src + x * src_px_size);
            dst[x] = lv_color_16_16_mix(c, dst[x], LV_OPA_MIX2(mask[x], dsc->opa));
        }

        dst_row += dsc->dest_stride;
        src_row += dsc->src_stride;
        mask += dsc->mask_stride;
    }

    return LV_RESULT_OK;
}

static inline lv_result_t lv_draw_sw_mve_argb8888_blend_normal_to_rgb565(lv_draw_sw_blend_image_dsc_t * dsc)
{
    if(dsc->mask_buf != NULL || dsc->opa < LV_OPA_MAX || dsc->blend_mode != LV_BLEND_MODE_NORMAL) {
        return LV_RESULT_INVALID;
    }

    uint8_t * dst_row = (uint8_t *)dsc->dest_buf;
    const uint8_t * src_row = (const uint8_t *)dsc->src_buf;

    for(int32_t y = 0; y < dsc->dest_h; y++) {
        uint16_t * dst = (uint16_t *)dst_row;
        const uint8_t * src = src_row;
        int32_t x = 0;

        for(; x <= dsc->dest_w - 8; x += 8) {
            uint16x8_t src_q = lv_draw_sw_mve_rgb888_to_rgb565_q(src + x * 4U, 4U);
            uint16x8_t dst_q = vld1q_u16(&dst[x]);
            uint16x8_t alpha_q = lv_draw_sw_mve_argb8888_alpha_q(src + x * 4U);
            vst1q_u16(&dst[x], lv_draw_sw_mve_rgb565_mix_opa_q(src_q, dst_q, alpha_q));
        }

        for(; x < dsc->dest_w; x++) {
            const uint8_t * p = src + x * 4U;
            uint16_t c = lv_draw_sw_mve_rgb888_to_rgb565_scalar(p);
            dst[x] = lv_color_16_16_mix(c, dst[x], p[3]);
        }

        dst_row += dsc->dest_stride;
        src_row += dsc->src_stride;
    }

    return LV_RESULT_OK;
}

static inline lv_result_t lv_draw_sw_mve_argb8888_blend_normal_to_rgb565_with_opa(lv_draw_sw_blend_image_dsc_t * dsc)
{
    if(dsc->mask_buf != NULL || dsc->opa >= LV_OPA_MAX || dsc->blend_mode != LV_BLEND_MODE_NORMAL) {
        return LV_RESULT_INVALID;
    }

    uint8_t * dst_row = (uint8_t *)dsc->dest_buf;
    const uint8_t * src_row = (const uint8_t *)dsc->src_buf;
    uint16x8_t opa_q = vdupq_n_u16(dsc->opa);

    for(int32_t y = 0; y < dsc->dest_h; y++) {
        uint16_t * dst = (uint16_t *)dst_row;
        const uint8_t * src = src_row;
        int32_t x = 0;

        for(; x <= dsc->dest_w - 8; x += 8) {
            uint16x8_t src_q = lv_draw_sw_mve_rgb888_to_rgb565_q(src + x * 4U, 4U);
            uint16x8_t dst_q = vld1q_u16(&dst[x]);
            uint16x8_t mix_q = lv_draw_sw_mve_opa_mix2_q(lv_draw_sw_mve_argb8888_alpha_q(src + x * 4U), opa_q);
            vst1q_u16(&dst[x], lv_draw_sw_mve_rgb565_mix_opa_q(src_q, dst_q, mix_q));
        }

        for(; x < dsc->dest_w; x++) {
            const uint8_t * p = src + x * 4U;
            uint16_t c = lv_draw_sw_mve_rgb888_to_rgb565_scalar(p);
            dst[x] = lv_color_16_16_mix(c, dst[x], LV_OPA_MIX2(p[3], dsc->opa));
        }

        dst_row += dsc->dest_stride;
        src_row += dsc->src_stride;
    }

    return LV_RESULT_OK;
}

static inline lv_result_t lv_draw_sw_mve_argb8888_blend_normal_to_rgb565_with_mask(lv_draw_sw_blend_image_dsc_t * dsc)
{
    if(dsc->mask_buf == NULL || dsc->opa < LV_OPA_MAX || dsc->blend_mode != LV_BLEND_MODE_NORMAL) {
        return LV_RESULT_INVALID;
    }

    uint8_t * dst_row = (uint8_t *)dsc->dest_buf;
    const uint8_t * src_row = (const uint8_t *)dsc->src_buf;
    const lv_opa_t * mask = dsc->mask_buf;

    for(int32_t y = 0; y < dsc->dest_h; y++) {
        uint16_t * dst = (uint16_t *)dst_row;
        const uint8_t * src = src_row;
        int32_t x = 0;

        for(; x <= dsc->dest_w - 8; x += 8) {
            uint16x8_t src_q = lv_draw_sw_mve_rgb888_to_rgb565_q(src + x * 4U, 4U);
            uint16x8_t dst_q = vld1q_u16(&dst[x]);
            uint16x8_t mix_q = lv_draw_sw_mve_opa_mix2_q(lv_draw_sw_mve_argb8888_alpha_q(src + x * 4U),
                                                         vldrbq_u16(&mask[x]));
            vst1q_u16(&dst[x], lv_draw_sw_mve_rgb565_mix_opa_q(src_q, dst_q, mix_q));
        }

        for(; x < dsc->dest_w; x++) {
            const uint8_t * p = src + x * 4U;
            uint16_t c = lv_draw_sw_mve_rgb888_to_rgb565_scalar(p);
            dst[x] = lv_color_16_16_mix(c, dst[x], LV_OPA_MIX2(p[3], mask[x]));
        }

        dst_row += dsc->dest_stride;
        src_row += dsc->src_stride;
        mask += dsc->mask_stride;
    }

    return LV_RESULT_OK;
}

static inline lv_result_t lv_draw_sw_mve_argb8888_blend_normal_to_rgb565_mix_mask_opa(lv_draw_sw_blend_image_dsc_t * dsc)
{
    if(dsc->mask_buf == NULL || dsc->opa >= LV_OPA_MAX || dsc->blend_mode != LV_BLEND_MODE_NORMAL) {
        return LV_RESULT_INVALID;
    }

    uint8_t * dst_row = (uint8_t *)dsc->dest_buf;
    const uint8_t * src_row = (const uint8_t *)dsc->src_buf;
    const lv_opa_t * mask = dsc->mask_buf;

    for(int32_t y = 0; y < dsc->dest_h; y++) {
        uint16_t * dst = (uint16_t *)dst_row;
        const uint8_t * src = src_row;
        int32_t x = 0;

        for(; x <= dsc->dest_w - 8; x += 8) {
            uint16x8_t src_q = lv_draw_sw_mve_rgb888_to_rgb565_q(src + x * 4U, 4U);
            uint16x8_t dst_q = vld1q_u16(&dst[x]);
            uint16x8_t mix_q = lv_draw_sw_mve_opa_mix3_q(lv_draw_sw_mve_argb8888_alpha_q(src + x * 4U),
                                                         vldrbq_u16(&mask[x]), dsc->opa);
            vst1q_u16(&dst[x], lv_draw_sw_mve_rgb565_mix_opa_q(src_q, dst_q, mix_q));
        }

        for(; x < dsc->dest_w; x++) {
            const uint8_t * p = src + x * 4U;
            uint16_t c = lv_draw_sw_mve_rgb888_to_rgb565_scalar(p);
            dst[x] = lv_color_16_16_mix(c, dst[x], LV_OPA_MIX3(mask[x], dsc->opa, p[3]));
        }

        dst_row += dsc->dest_stride;
        src_row += dsc->src_stride;
        mask += dsc->mask_stride;
    }

    return LV_RESULT_OK;
}

#endif /* defined(__ARM_FEATURE_MVE) && __ARM_FEATURE_MVE */

#endif /* LV_DRAW_SW_MVE_CUSTOM_H */
