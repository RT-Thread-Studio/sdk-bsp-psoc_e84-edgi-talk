#include "rtthread.h"
#include "rtdevice.h"
#include "os_support.h"

extern struct rt_memheap system_heap;

void *opus_heap_malloc(uint32_t size)
{
    return rt_memheap_alloc(&system_heap, size);
}

void opus_heap_free(void *p)
{
    rt_memheap_free(p);
}
