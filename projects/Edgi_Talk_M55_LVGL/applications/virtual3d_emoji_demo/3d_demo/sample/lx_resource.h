
#include "lvgl.h"

#ifndef LX_RESOURCE_H
#define LX_RESOURCE_H

void lx_main_menu_image_preload_all();

void lx_main_menu_image_unload_all();

#define LX_IMG_DECLARE(var_name) \
extern lv_image_dsc_t var_name;   \
extern const uint32_t var_name##_LZ4_SIZE;   \
extern const void* var_name##_LZ4_DATA;

lv_image_dsc_t* lx_lz4_image_load(lv_image_dsc_t* image_dsc, void* lz4_data, uint32_t lz4_data_size);

void lx_lz4_image_unload(lv_image_dsc_t* image_dsc);

#define LX_LOAD_IMAGE(a)    lx_lz4_image_load(&a, (void *)a##_LZ4_DATA, a##_LZ4_SIZE)
#define LX_UNLOAD_IMAGE(a)  lx_lz4_image_unload(&a)
#else
#define LX_IMG_DECLARE(var_name) extern const lv_image_dsc_t var_name;

#define LX_LOAD_IMAGE(a)    &a
#define LX_UNLOAD_IMAGE(a)
#endif

extern void lx_lz4_model_unload(void *ptr);
#define LX_UNLOAD_MODEL(a)  lx_lz4_model_unload(a);
#define LX_MOD_DECLARE(var_name) \
extern const uint8_t var_name[];   \
extern const uint32_t var_name##_ORIGINAL_SIZE;   \
extern const uint32_t var_name##_COMPRESSED_SIZE;

void*  lx_lz4_model_load(uint32_t decomp_size,void* lz4_data, uint32_t lz4_data_size);

#define LX_LOAD_MODEL(a)    lx_lz4_model_load(a##_ORIGINAL_SIZE, a, a##_COMPRESSED_SIZE)

#define LX_GET_MODEL_COMPRESSED_SIZE(name) name##_COMPRESSED_SIZE 
#define LX_GET_MODEL_ORIGINAL_SIZE(name) name##_ORIGINAL_SIZE

