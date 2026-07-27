/******************************************************************************
* Copyright (c) 2021 Shanghai QDay Technology Co., Ltd.
* All rights reserved.
*
* This file is part of the LiteGFX 0.0.1 distribution.
*
* This software is licensed under terms that can be found in the LICENSE file in
* the root directory of this software component.
* If no LICENSE file comes with this software, it is provided AS-IS.
*
* Author:LiteGFX Team
* Date:2021.12.05
*******************************************************************************/
#ifndef LX_PLATFORM_MEMORY_H
#define LX_PLATFORM_MEMORY_H

#if defined(__cplusplus)
extern "C" {
#endif


/*********************
*      INCLUDES
*********************/
#include <stdlib.h>
#include <stdint.h>

/*********************
*      DEFINES
*********************/
#define LX_ATTRIBUTE_FAST_MEM

/**********************
*      TYPEDEFS
**********************/


/**********************
* GLOBAL PROTOTYPES
**********************/
void* lx_platform_malloc(size_t size);

void* lx_platform_calloc(size_t num, size_t size);

void* lx_platform_realloc(void* ptr, size_t size);

void lx_platform_free(void* ptr);

void* lx_platform_int_malloc(size_t size);

void lx_platform_int_free(void* ptr);

uint8_t lx_platform_get_format_bpp(int32_t format);

#if defined(__cplusplus)
}
#endif

#endif // LX_PLATFORM_MEMORY_H
