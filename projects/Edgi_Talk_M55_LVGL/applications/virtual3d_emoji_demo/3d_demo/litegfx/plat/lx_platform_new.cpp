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

#if 1
#include "lx_platform_memory.h"
#include "lx_platform_log.h"

//#define LITEGFX_MEMORY_DEBUG (1)

extern "C" void LxMemPrint();
extern "C" {
void *__dso_handle __attribute__((weak)) = (void *)&__dso_handle;
}

#if defined(LITEGFX_MEMORY_DEBUG)
__attribute__((section("psram_data")))
static void* memVector[1024] = { 0 };
static uint32_t memVectorSize = 0;

static void memVectorAdd(void* ptr)
{
    if(ptr)
    {
        memVector[memVectorSize] = ptr;
        memVectorSize++;
    }
}

static void memVectorRemove(void* ptr)
{
    if(ptr)
    {
        for(int i = 0; i < memVectorSize; i++)
        {
            if(memVector[i] == ptr)
            {
                for(int j = i; j < memVectorSize - 1; j++)
                {
                    memVector[j] = memVector[j + 1];
                }

                memVector[memVectorSize - 1] = 0;
                memVectorSize--;
                break;
            }
        }
    }
}
#endif


__attribute__((weak)) void *operator new(size_t size)
{
    void* ptr = lx_platform_malloc(size);

    #if defined(LITEGFX_MEMORY_DEBUG)
    memVectorAdd(ptr);
    #endif

    return ptr;
}

__attribute__((weak)) void *operator new[](size_t size)
{
    void* ptr = lx_platform_malloc(size);

    #if defined(LITEGFX_MEMORY_DEBUG)
    memVectorAdd(ptr);
    #endif

    return ptr;
}

__attribute__((weak)) void operator delete(void* ptr)
{
    #if defined(LITEGFX_MEMORY_DEBUG)
    memVectorRemove(ptr);
    #endif

    lx_platform_free(ptr);
}

__attribute__((weak)) void operator delete[](void* ptr)
{
    #if defined(LITEGFX_MEMORY_DEBUG)
    memVectorRemove(ptr);
    #endif

    lx_platform_free(ptr);
}

__attribute__((weak)) void operator delete(void* ptr, size_t size)
{
    (void)size;
    operator delete(ptr);
}

__attribute__((weak)) void operator delete[](void* ptr, size_t size)
{
    (void)size;
    operator delete[](ptr);
}

void LxMemPrint()
{
    #if defined(LITEGFX_MEMORY_DEBUG)
    LX_LOG_USER("LxMemPrint begin:\n");
    for(int i = 0; i < memVectorSize; i++)
    {
        LX_LOG_USER("Litegfx memVector[%d]:0x%x\r\n", i, memVector[i]);
    }
    LX_LOG_USER("LxMemPrint end.\n");
    #else
        LX_LOG_WARN("Please enable LITEGFX_MEMORY_DEBUG");
    #endif
}

#endif
