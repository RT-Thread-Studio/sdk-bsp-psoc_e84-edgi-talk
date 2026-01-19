#include "lx_resource.h"

#include "cy_utils.h"
#include "lz4/lz4.h"

#include"rtthread.h"
#include"rtdevice.h"
#include"os_support.h"

#define DRV_INFO
#define LOG_TAG         "resource"
#include <drv_log.h>

extern struct rt_memheap system_heap;

#define LX_USE_PSRAM (1)

#if (LX_USE_PSRAM)

#if 0
#include "tlsf/tlsf.h"

#define LX_RES_MEM_SIZE	(2 * 1024 * 1024)

CY_SECTION(".cy_xip") __attribute__((used)) uint8_t lx_res_mem_pool[LX_RES_MEM_SIZE] = {0xff};

static tlsf_t lx_res_mem_tlsf = NULL; 

void *lx_res_malloc(size_t size)
{
	void* ptr = NULL;

	if(lx_res_mem_tlsf == NULL)
	{
		lx_res_mem_tlsf = tlsf_create_with_pool((void *)lx_res_mem_pool, LX_RES_MEM_SIZE);
	}

	if(lx_res_mem_tlsf)
	{
		ptr = tlsf_malloc(lx_res_mem_tlsf, size);
	}

	if (ptr != NULL)
	{
		memset(ptr, 0, size);
	}	
	else
	{
		LOG_E("lx_res_malloc failed!!!, size:%d", size);
	}	
	
	return ptr;
}


void *lx_res_free(void* ptr)
{
	if(lx_res_mem_tlsf)
	{
		tlsf_free(lx_res_mem_tlsf, ptr);
	}
}

#else

void *lx_res_malloc(size_t size)
{
    void* ptr =  rt_memheap_alloc(&system_heap, size);


    if (ptr != NULL)
    {
        memset(ptr, 0, size);
        LOG_D("lx_res_malloc success!!!, size:%d \n", size);
    }
    else
    {
        LOG_E("lx_res_malloc failed!!!, size:%d  \n", size);
    }

    return ptr;
}


void *lx_res_free(void* ptr)
{
    if(ptr != NULL)
    {
        rt_memheap_free(ptr);
        LOG_E("lx_res_free =>rt_memheap_free!!! \n");
    }
}


#endif
#endif




/**********************
*  STATIC PROTOTYPES
**********************/
static void *lx_res_malloc_align(size_t size, size_t align);
static void lx_res_free_align(void *ptr);
void lx_lz4_model_unload(void *ptr)
{
	lx_res_free_align(ptr);
}  



void*  lx_lz4_model_load(uint32_t decomp_size,void* lz4_data, uint32_t lz4_data_size)
{  
	LOG_D("lx_lz4_model_load ->aaa decomp_size:%d", decomp_size);
	LOG_D("lx_lz4_model_load ->aaa lz4_data_size:%d", lz4_data_size);
	void* decomp_buffer =  lx_res_malloc_align(decomp_size, 64);

	LOG_D("lx_lz4_model_load ->bbb decomp_buffer:%p", decomp_buffer);
    if(decomp_buffer)
    {
        int32_t ret = LZ4_decompress_safe(lz4_data, decomp_buffer, lz4_data_size, decomp_size);
        LOG_D("lx_lz4_model_load ->LZ4_decompress_safe return A, ret:%d ", ret);
        if(ret == decomp_size)
        {
            // decomp_buffer;
			LOG_D("lx_lz4_model_load ->LZ4_decompress_safe success, ret:%d ", ret);
			LOG_D("lx_lz4_model_load ->LZ4_decompress_safe success, decomp_buffer:%p ", decomp_buffer);
        }
        else
        {
            LOG_D("lx_lz4_model_load ->LZ4_decompress_safe failed, ret:%d ", ret);

            lx_res_free_align(decomp_buffer);
            decomp_buffer = NULL;
        }                
    }
    else
    {
        LOG_D("lx_lz4_model_load->lx_res_malloc_align failed, size:%d ", decomp_size);
    }

    return decomp_buffer;
}

lv_image_dsc_t* lx_lz4_image_load(lv_image_dsc_t* image_dsc, void* lz4_data, uint32_t lz4_data_size)
{
    lx_lz4_image_unload(image_dsc);   

    uint32_t decomp_size = image_dsc->data_size;
    void* decomp_buffer = lx_res_malloc_align(decomp_size, 64);

    if(decomp_buffer)
    {
        int32_t ret = LZ4_decompress_safe(lz4_data, decomp_buffer, lz4_data_size, decomp_size);

        if(ret == decomp_size)
        {
            image_dsc->data = decomp_buffer;
			LOG_D("lx_lz4_image_load  ************* image_dsc->data=%p ",image_dsc->data);
        }
        else
        {
            LOG_E("LZ4_decompress_safe failed, ret:%d", ret);

            lx_res_free_align(decomp_buffer);
            decomp_buffer = NULL;
        }                
    }
    else
    {
        LOG_E("lx_res_malloc_align failed, size:%d", decomp_size);
    }

    return image_dsc;
}

void lx_lz4_image_unload(lv_image_dsc_t* image_dsc)
{
    if(image_dsc->data)
    {
        lx_res_free_align(image_dsc->data);
        image_dsc->data = NULL;
    }
}

/**********************
*   STATIC FUNCTIONS
**********************/
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
static void *lx_res_malloc_align(size_t size, size_t align)
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
	#if defined(LX_USE_PSRAM)
	ptr = lx_res_malloc(align_size);
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
static void lx_res_free_align(void *ptr)
{
    void *real_ptr;
    /* NULL check */
    if (ptr == NULL)
    {
        return;
    }
    real_ptr = (void *) * (uintptr_t *)((uintptr_t)ptr - sizeof(void *));

	#if defined(LX_USE_PSRAM)
	lx_res_free(real_ptr);
	#else
    free(real_ptr);
	#endif	
}