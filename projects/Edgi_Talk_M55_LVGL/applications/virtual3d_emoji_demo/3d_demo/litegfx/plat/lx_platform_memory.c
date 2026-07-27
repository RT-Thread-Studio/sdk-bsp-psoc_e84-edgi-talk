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

/*********************
*      INCLUDES
*********************/
#include "lx_platform_memory.h"
#include "lx_platform_log.h"
#include "stdbool.h"
#include <string.h>
#include "vg_lite.h"
#include <rtthread.h>
#include <rtdbg.h>
/*********************
*      DEFINES
*********************/

/**********************
*      TYPEDEFS
**********************/


/**********************
*  STATIC PROTOTYPES
**********************/
static void *lx_int_malloc_align(size_t size, size_t align);
static void lx_int_free_align(void *ptr);

/**********************
*  STATIC VARIABLES
**********************/

/**********************
*  GLOBAL VARIABLES
**********************/


/**********************
*      MACROS
**********************/


/**********************
*   GLOBAL FUNCTIONS
**********************/
void* lx_platform_malloc(size_t size)
{
    char* ptr = NULL;

    if(size != 0)
    {
        ptr = malloc(size);

        if (ptr != NULL)
        {
            memset(ptr, 0, size);
        }
        else
        {
            LX_LOG_ERROR("lx_platform_malloc failed, size:%d", size);
        }
    }

    return (void*)ptr;
}

void* lx_platform_calloc(size_t num, size_t size)
{
    char* ptr = NULL;

    if(num !=0 && size != 0)
    {
        ptr = malloc(num * size);

        if (ptr != NULL)
        {
            memset(ptr, 0, num * size);
        }            
    }

    return (void*)ptr;
}

void* lx_platform_realloc(void* ptr, size_t size)
{
    if (NULL == ptr)
        return malloc(size);
    else
        return realloc(ptr, size);
}

void lx_platform_free(void* ptr)
{
    if(ptr)
    {
        free(ptr);
    }
}

void* lx_platform_int_malloc(size_t size)
{
    void* ptr = NULL;

    ptr = lx_int_malloc_align(size, 64);

    if(ptr == NULL)
    {
        LOG_E("lx_platform_int_malloc failed!!!, size:%d", size);
    }

    return (void*)ptr;
}

void lx_platform_int_free(void* ptr)
{
    lx_int_free_align(ptr);
}

uint8_t lx_platform_get_format_bpp(int32_t format)
{
    uint8_t bpp = 0;
    vg_lite_buffer_format_t fmt = (vg_lite_buffer_format_t)format;

    switch(fmt) {
        case VG_LITE_BGR565:
        case VG_LITE_RGB565:
        bpp = 16;
    break;

    case VG_LITE_BGRA5658:
    case VG_LITE_ABGR8565:
    case VG_LITE_ARGB8565:
    case VG_LITE_RGBA5658:
        bpp = 24;
    break;

    case VG_LITE_BGRA8888:
    case VG_LITE_RGBA8888:
        bpp = 32;
    break;

    case VG_LITE_RGBX8888:
    case VG_LITE_BGRX8888:
        bpp = 32;
    break;

    case VG_LITE_BGR888:
    case VG_LITE_RGB888:
        bpp = 24;
    break;

    case VG_LITE_BGRA5551:
        bpp = 16;
    break;

    case VG_LITE_A8:
        bpp = 8;
    break;

    case VG_LITE_INDEX_8:
        bpp = 8;
    break;

   case VG_LITE_RGBA8888_ETC2_EAC:
      bpp = 8;
    break;

   default:
     break;
  }

   return bpp;
}

/**********************
*   STATIC FUNCTIONS
**********************/
#if 0
#include "tlsf.h"
#include "cy_graphics.h"

#define LX_INT_MEM_SIZE	(1200 * 1024)

CY_SECTION(".cy_gpu_buf") uint8_t lx_int_mem_pool[LX_INT_MEM_SIZE] = {0xff};

static tlsf_t lx_int_mem_tlsf = NULL; 

static void *lx_int_malloc(size_t size)
{
	void* ptr = NULL;

	if(lx_int_mem_tlsf == NULL)
	{
		lx_int_mem_tlsf = tlsf_create_with_pool((void *)lx_int_mem_pool, LX_INT_MEM_SIZE);
	}

	if(lx_int_mem_tlsf)
	{
		ptr = tlsf_malloc(lx_int_mem_tlsf, size);
	}

	if (ptr != NULL)
	{
		memset(ptr, 0, size);
	}	
	else
	{
		printf("lx_int_mem_tlsf failed!!!, size:%d", size);
	}	
	
	return ptr;
}


static void *lx_int_free(void* ptr)
{
	if(lx_int_mem_tlsf)
	{
		tlsf_free(lx_int_mem_tlsf, ptr);
	}
}
#endif

/**
 * This function allocates a memory block, which address is aligned to the
 * specified alignment size.
 *
 * @param  size is the allocated memory block size.
 *
 * @param  align is the alignment size.
 *
 * @return The memory block address was returned successfully, otherwise it was
 *         returned empty RT_NULL.
 */
static void *lx_int_malloc_align(size_t size, size_t align)
{
    void *ptr;
    void *align_ptr;
    int uintptr_size;
    size_t align_size;
    /* sizeof pointer */
    uintptr_size = sizeof(void*);
    uintptr_size -= 1;
    /* align the alignment size to uintptr size byte */
    align = ((align + uintptr_size) & ~uintptr_size);
    /* get total aligned size */
    align_size = ((size + uintptr_size) & ~uintptr_size) + align;
    /* allocate memory block from heap */
    #if 0
    ptr = lx_int_malloc(align_size);
    #else
    ptr = malloc(align_size);
    #endif

    if (ptr != NULL)
    {
        /* the allocated memory block is aligned */
        if (((uintptr_t)ptr & (align - 1)) == 0)
        {
            align_ptr = (void *)((uintptr_t)ptr + align);
        }
        else
        {
            align_ptr = (void *)(((uintptr_t)ptr + (align - 1)) & ~(align - 1));
        }

        /* set the pointer before alignment pointer to the real pointer */
        *((uintptr_t *)((uintptr_t)align_ptr - sizeof(void *))) = (uintptr_t)ptr;
        ptr = align_ptr;
    }
    
    return ptr;
}

/**
 * This function release the memory block, which is allocated by
 * lx_int_mem_free_align function and address is aligned.
 *
 * @param ptr is the memory block pointer.
 */
static void lx_int_free_align(void *ptr)
{
    void *real_ptr;

    /* NULL check */
    if (ptr == NULL)
    {
        return;
    }

    real_ptr = (void *) * (uintptr_t *)((uintptr_t)ptr - sizeof(void *));

    #if 0
    lx_int_free(real_ptr);
    #else
    free(real_ptr);
    #endif
}
