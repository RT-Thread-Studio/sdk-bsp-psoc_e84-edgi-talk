#include "cy_pdl.h"
#include "cybsp.h"
#include "mtb_hal.h"
#include "cycfg_qspi_memslot.h"
#include "rtthread.h"

#define DRV_DEBUG
#define LOG_TAG         "drv_hyperam"
#include <drv_log.h>

#define PSRAM_MAP_BASE                (0x64000000UL)
#define PSRAM_MAP_SIZE                (0x1000000UL)
#define PSRAM_ADDRESS                 (0x64400000UL) /* PSRAM test address */
#define PSRAM_HEAP_MAX_SIZE           (PSRAM_MAP_BASE + PSRAM_MAP_SIZE - PSRAM_ADDRESS)

#if defined(BSP_USING_HYPERAM_SIZE) && (BSP_USING_HYPERAM_SIZE > PSRAM_HEAP_MAX_SIZE)
#error "BSP_USING_HYPERAM_SIZE exceeds the mapped HyperRAM window"
#endif

#if defined(BSP_USING_HYPERAM) && defined(RT_USING_MEMHEAP_AS_HEAP)
struct rt_memheap system_heap;
#endif

struct rt_memheap *drv_hyperam_get_memheap(void)
{
#if defined(BSP_USING_HYPERAM) && defined(RT_USING_MEMHEAP_AS_HEAP)
    return &system_heap;
#else
    return RT_NULL;
#endif
}

#if defined(BSP_USING_HYPERAM) && defined(RT_USING_MEMHEAP_AS_HEAP)
static int hyperam_init(void)
{
    rt_memheap_init(&system_heap, "hyperam", (void *)PSRAM_ADDRESS, BSP_USING_HYPERAM_SIZE);
    LOG_D("hyperam init success, mapped at 0x%X, size is %d bytes, data width is %d", PSRAM_ADDRESS, BSP_USING_HYPERAM_SIZE, 16);

    return RT_EOK;
}
INIT_BOARD_EXPORT(hyperam_init);
#endif
