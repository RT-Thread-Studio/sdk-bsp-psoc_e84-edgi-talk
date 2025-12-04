/*
 * Copyright (c) 2006-2018, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2018-01-26     armink       the first version
 */

#include <fal.h>
#include "cycfg_qspi_memslot.h"

#define smifMemConfigs smif0MemConfigs
#define MEM_SLOT_NUM                     (0U)
#ifndef FAL_USING_NOR_FLASH_DEV_NAME
    #define FAL_USING_NOR_FLASH_DEV_NAME             "norflash0"
#endif
static int init(void);
static int read(long offset, uint8_t *buf, size_t size);
static int write(long offset, const uint8_t *buf, size_t size);
static int erase(long offset, size_t size);

struct rt_device *flash_dev;
struct fal_flash_dev nor_flash0 =
{
    .name       = FAL_USING_NOR_FLASH_DEV_NAME,
    .addr       = 0,
    .len        = 16 * 1024 * 1024,
    .blk_size   = 4096,
    .ops        = {init, read, write, erase},
    .write_gran = 1
};

static int init(void)
{
    cy_rslt_t result;
    result = cy_serial_flash_qspi_attach(smifMemConfigs[MEM_SLOT_NUM],
                                         BSP_USING_FLASH_D0_PIN, BSP_USING_FLASH_D1_PIN, BSP_USING_FLASH_D2_PIN, BSP_USING_FLASH_D3_PIN, BSP_USING_FLASH_D4_PIN, BSP_USING_FLASH_D5_PIN,
                                         BSP_USING_FLASH_D6_PIN, BSP_USING_FLASH_D7_PIN, BSP_USING_FLASH_FTAM_SSEL_PIN);

    if (result != 0)
    {
        rt_kprintf("cy_serial_flash_qspi_attach failed");
    }

    return 0;
}

static int read(long offset, uint8_t *buf, size_t size)
{
    cy_rslt_t result;
    result = cy_serial_flash_qspi_read(nor_flash0.addr + offset, size, buf);
    if (result != 0)
    {
        rt_kprintf("cy_serial_flash_qspi_read failed");
    }
    return size;
}

static int write(long offset, const uint8_t *buf, size_t size)
{
    cy_rslt_t result;
    result = cy_serial_flash_qspi_write(nor_flash0.addr + offset, size, buf);
    if (result != 0)
    {
        rt_kprintf("cy_serial_flash_qspi_write failed");
    }
    return size;
}

static int erase(long offset, size_t size)
{
    cy_rslt_t result;
    result = cy_serial_flash_qspi_erase(nor_flash0.addr + offset, size);
    if (result != 0)
    {
        rt_kprintf("cy_serial_flash_qspi_erase failed");
    }
    return size;
}

static int rt_flash_init(void)
{
    fal_init();
    struct rt_device *flash_dev = fal_blk_device_create("flash");
    return RT_EOK;
}
INIT_ENV_EXPORT(rt_flash_init);
