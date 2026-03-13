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
#include <string.h>
#include "cy_smif.h"

#define LOG_TAG                "drv.fal_flash"
#include <drv_log.h>

/* Objects for serial memory */
static cy_stc_smif_context_t smif_context;

#define smifMemConfigs smif0MemConfigs
#define MEM_SLOT_NUM                     (0U)
#ifndef FAL_USING_NOR_FLASH_DEV_NAME
#define FAL_USING_NOR_FLASH_DEV_NAME             "norflash0"
#endif

/* Flash device configuration */
#define SMIF_BASE_ADDRESS      0x60000000
#define FLASH_START_ADDRESS    0x60E00000
#define FLASH_SIZE             (2 * 1024 * 1024) /* 2MB */
#define FLASH_SECTOR_SIZE      0x10000 /* 64KB sectors */
#define FLASH_END_ADDRESS      (FLASH_START_ADDRESS + FLASH_SIZE)

static struct rt_mutex smif_lock;

static int init(void);
static int read(long offset, uint8_t *buf, size_t size);
static int write(long offset, const uint8_t *buf, size_t size);
static int erase(long offset, size_t size);

struct rt_device *flash_dev;
struct fal_flash_dev nor_flash0 =
{
    .name       = FAL_USING_NOR_FLASH_DEV_NAME,
    .addr       = FLASH_START_ADDRESS,
    .len        = FLASH_SIZE,
    .blk_size   = FLASH_SECTOR_SIZE,
    .ops        = {init, read, write, erase},
    .write_gran = 1
};

static int init(void)
{
    rt_size_t erase_size = FLASH_SECTOR_SIZE;

    if (smifMemConfigs[MEM_SLOT_NUM] && smifMemConfigs[MEM_SLOT_NUM]->deviceCfg)
    {
        erase_size = smifMemConfigs[MEM_SLOT_NUM]->deviceCfg->eraseSize;
    }

    if (erase_size == 0)
    {
        erase_size = FLASH_SECTOR_SIZE;
    }

    nor_flash0.blk_size = erase_size;
    rt_mutex_init(&smif_lock, "smif", RT_IPC_FLAG_PRIO);

    /* SMIF already initialized by cybsp_init() */
    LOG_I("FAL flash initialized, erase size=%u", (unsigned int)nor_flash0.blk_size);
    return 0;
}

static int read(long offset, uint8_t *buf, size_t size)
{
    if (offset + size > FLASH_SIZE)
    {
        LOG_E("read out of range! offset=%ld, size=%d", offset, size);
        return -RT_EINVAL;
    }

    LOG_D("FAL read: offset %#lx, size %d", offset, size);
    cy_en_smif_status_t result;
    rt_mutex_take(&smif_lock, RT_WAITING_FOREVER);
    result = Cy_SMIF_MemRead(SMIF0_CORE, smifMemConfigs[MEM_SLOT_NUM], offset + (FLASH_START_ADDRESS - SMIF_BASE_ADDRESS), buf, size, &smif_context);
    rt_mutex_release(&smif_lock);
    if (result != CY_SMIF_SUCCESS)
    {
        LOG_E("Cy_SMIF_MemRead failed: %d", result);
        return -RT_ERROR;
    }

    return size;
}

static int write(long offset, const uint8_t *buf, size_t size)
{
    if (!buf || size == 0)
    {
        LOG_E("Invalid input: buf=%p, size=%d", buf, size);
        return -RT_EINVAL;
    }

    if (offset + size > FLASH_SIZE)
    {
        LOG_E("write out of range! offset=%ld, size=%d", offset, size);
        return -RT_EINVAL;
    }

    LOG_D("FAL write: offset %#lx, size %d", offset, size);
    cy_en_smif_status_t result;
    rt_mutex_take(&smif_lock, RT_WAITING_FOREVER);
    result = Cy_SMIF_MemWrite(SMIF0_CORE, smifMemConfigs[MEM_SLOT_NUM], offset + (FLASH_START_ADDRESS - SMIF_BASE_ADDRESS), buf, size, &smif_context);
    rt_mutex_release(&smif_lock);
    if (result != CY_SMIF_SUCCESS)
    {
        LOG_E("Cy_SMIF_MemWrite failed: %d", result);
        return -RT_ERROR;
    }

    return size;
}

static int erase(long offset, size_t size)
{
    if ((offset % (long)nor_flash0.blk_size) != 0 || (size % nor_flash0.blk_size) != 0)
    {
        LOG_E("erase unaligned! offset=%ld, size=%u, blk=%u", offset, (unsigned int)size, (unsigned int)nor_flash0.blk_size);
        return -RT_EINVAL;
    }

    if (offset + size > FLASH_SIZE)
    {
        LOG_E("erase out of range! offset=%ld, size=%d", offset, size);
        return -RT_EINVAL;
    }

    LOG_D("FAL erase: offset %#lx, size %d", offset, size);
    cy_en_smif_status_t result;
    rt_mutex_take(&smif_lock, RT_WAITING_FOREVER);
    result = Cy_SMIF_MemEraseSector(SMIF0_CORE, smifMemConfigs[MEM_SLOT_NUM], offset + (FLASH_START_ADDRESS - SMIF_BASE_ADDRESS), size, &smif_context);
    rt_mutex_release(&smif_lock);
    if (result != CY_SMIF_SUCCESS)
    {
        LOG_E("Cy_SMIF_MemEraseSector failed: %d", result);
        return -RT_ERROR;
    }
    return size;
}
