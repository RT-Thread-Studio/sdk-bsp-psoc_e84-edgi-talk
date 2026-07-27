/*
 * MIT License
 *
 * Copyright (c) 2024 Evlers
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * Change Logs:
 * Date         Author      Notes
 * 2024-01-27   Evlers      first implementation
 */

#include <stdint.h>
#include <string.h>

#include "rtthread.h"

#include "wiced_resource.h"
#include "whd_resource_api.h"

#if defined(WHD_RESOURCES_IN_MEMORY)
    #ifdef WLAN_MFG_FIRMWARE
        extern const resource_hnd_t wifi_mfg_firmware_image;
        extern const resource_hnd_t wifi_mfg_firmware_clm_blob;
    #else
        extern const resource_hnd_t wifi_firmware_image;
        extern const resource_hnd_t wifi_firmware_clm_blob;
    #endif
    extern const char wifi_nvram_image[];
    extern const uint32_t wifi_nvram_image_size;
#endif

#if defined(WHD_RESOURCES_IN_SDCARD)
    #ifndef WHD_RESOURCES_FIRMWARE_PATH_NAME
        #define WHD_RESOURCES_FIRMWARE_PATH_NAME "/sdcard/55500A1.trxcse"
    #endif
    #ifndef WHD_RESOURCES_CLM_PATH_NAME
        #define WHD_RESOURCES_CLM_PATH_NAME "/sdcard/55500A1.clm_blob"
    #endif
    #ifndef WHD_RESOURCES_NVRAM_PATH_NAME
        #define WHD_RESOURCES_NVRAM_PATH_NAME "/sdcard/cyw55513modpse84som_rev3.txt"
    #endif
#endif

#if defined(WHD_RESOURCES_IN_EXTERNAL_STORAGE_FS) || defined(WHD_RESOURCES_IN_SDCARD)
    #include <dfs_file.h>
    #include <sys/stat.h>
    #include <unistd.h>
    #include <fcntl.h>

    #ifndef WHD_RESOURCES_FS_WAIT_TIMEOUT_MS
        #define WHD_RESOURCES_FS_WAIT_TIMEOUT_MS 30000
    #endif
#endif

#define DBG_TAG           "whd.resources"
#define DBG_LVL           DBG_INFO
#include "rtdbg.h"


#if defined(WHD_RESOURCES_IN_SDCARD) || defined(WHD_RESOURCES_IN_EXTERNAL_STORAGE_FS)
static const char *name_list[] =
{
    [WHD_RESOURCE_WLAN_FIRMWARE] = WHD_RESOURCES_FIRMWARE_PATH_NAME,
    [WHD_RESOURCE_WLAN_CLM] = WHD_RESOURCES_CLM_PATH_NAME,
    [WHD_RESOURCE_WLAN_NVRAM] = WHD_RESOURCES_NVRAM_PATH_NAME
};
#elif defined(WHD_RESOURCES_IN_EXTERNAL_STORAGE_FAL)
static const char *name_list[] =
{
    [WHD_RESOURCE_WLAN_FIRMWARE] = WHD_RESOURCES_FIRMWARE_PART_NAME,
    [WHD_RESOURCE_WLAN_CLM] = WHD_RESOURCES_CLM_PART_NAME,
    [WHD_RESOURCE_WLAN_NVRAM] = WHD_RESOURCES_NVRAM_PART_NAME
};
#endif

uint8_t r_buffer[WHD_RESOURCES_BLOCK_SIZE];

#if defined(WHD_RESOURCES_IN_EXTERNAL_STORAGE_FAL) || defined(WHD_RESOURCES_IN_EXTERNAL_STORAGE_FS) || defined(WHD_RESOURCES_IN_SDCARD)
static void nvram_convert_line_endings(char *data, uint32_t size)
{
    /* convert the newline to null-terminator */
    for (uint32_t i = 0; i < size; i ++)
    {
        if (data[i] == 0x0A)
        {
            data[i] = 0x00;
        }
    }
}
#endif

#ifdef WHD_RESOURCES_IN_MEMORY
static const resource_hnd_t *host_memory_resource_handle(whd_resource_type_t type)
{
    if (type == WHD_RESOURCE_WLAN_FIRMWARE)
    {
#ifdef WLAN_MFG_FIRMWARE
        return &wifi_mfg_firmware_image;
#else
        return &wifi_firmware_image;
#endif
    }

    if (type == WHD_RESOURCE_WLAN_CLM)
    {
#ifdef WLAN_MFG_FIRMWARE
        return &wifi_mfg_firmware_clm_blob;
#else
        return &wifi_firmware_clm_blob;
#endif
    }

    return RT_NULL;
}

static uint32_t host_memory_resource_read(whd_resource_type_t type, uint32_t offset, uint32_t size,
                                          uint32_t *size_out, void *buffer)
{
    const resource_hnd_t *resource;
    const uint8_t *resource_data;
    uint32_t resource_size;

    if (type == WHD_RESOURCE_WLAN_NVRAM)
    {
        resource_data = (const uint8_t *)wifi_nvram_image;
        resource_size = wifi_nvram_image_size;
    }
    else
    {
        resource = host_memory_resource_handle(type);
        if (resource == RT_NULL || resource->location != RESOURCE_IN_MEMORY)
        {
            return RESOURCE_UNSUPPORTED;
        }

        resource_data = (const uint8_t *)resource->val.mem.data;
        resource_size = (uint32_t)resource_get_size(resource);
    }

    if (offset > resource_size)
    {
        return RESOURCE_OFFSET_TOO_BIG;
    }

    *size_out = MIN(size, resource_size - offset);
    memcpy(buffer, &resource_data[offset], *size_out);

    return RESOURCE_SUCCESS;
}

static uint32_t host_platform_resource_size (whd_driver_t whd_drv, whd_resource_type_t type, uint32_t *size_out)
{
    const resource_hnd_t *resource;

    if (type == WHD_RESOURCE_WLAN_NVRAM)
    {
        *size_out = wifi_nvram_image_size;
        return WHD_SUCCESS;
    }

    resource = host_memory_resource_handle(type);
    if (resource == RT_NULL || resource->location != RESOURCE_IN_MEMORY)
    {
        return RESOURCE_UNSUPPORTED;
    }

    *size_out = (uint32_t)resource_get_size(resource);

    return WHD_SUCCESS;
}

static uint32_t host_get_resource_block_size (whd_driver_t whd_drv, whd_resource_type_t type, uint32_t *size_out)
{
    *size_out = WHD_RESOURCES_BLOCK_SIZE;
    return RESOURCE_SUCCESS;
}

static uint32_t host_get_resource_no_of_blocks (whd_driver_t whd_drv, whd_resource_type_t type, uint32_t *block_count)
{
    uint32_t resource_size = 0;
    uint32_t block_size;

    host_platform_resource_size(whd_drv, type, &resource_size);
    host_get_resource_block_size(whd_drv, type, &block_size);
    *block_count = resource_size / block_size;
    if (resource_size % block_size)
        *block_count += 1;

    return RESOURCE_SUCCESS;
}

static uint32_t host_get_resource_block (whd_driver_t whd_drv, whd_resource_type_t type, uint32_t blockno, const uint8_t **data, uint32_t *size_out)
{
    uint32_t resource_size;
    uint32_t block_size;
    uint32_t block_count;
    uint32_t read_pos;
    uint32_t result;

    host_platform_resource_size(whd_drv, type, &resource_size);
    host_get_resource_block_size(whd_drv, type, &block_size);
    host_get_resource_no_of_blocks(whd_drv, type, &block_count);
    memset(r_buffer, 0, block_size);
    read_pos = blockno * block_size;

    if (blockno >= block_count)
    {
        return WHD_BADARG;
    }

    result = host_memory_resource_read(type, read_pos, block_size, size_out, r_buffer);
    if (result != RESOURCE_SUCCESS)
    {
        return result;
    }

    *data = (uint8_t *)&r_buffer;

    return RESOURCE_SUCCESS;
}

static uint32_t host_resource_read (whd_driver_t whd_drv, whd_resource_type_t type, uint32_t offset, uint32_t size, uint32_t *size_out, void *buffer)
{
    return host_memory_resource_read(type, offset, size, size_out, buffer);
}
#endif

#ifdef WHD_RESOURCES_IN_EXTERNAL_STORAGE_FAL
#include "fal.h"

static uint32_t host_platform_resource_size (whd_driver_t whd_drv, whd_resource_type_t type, uint32_t *size_out)
{
    resource_hnd_t resource;
    const struct fal_partition *part;
    const char *part_name = name_list[type];

    if ((part = fal_partition_find(part_name)) == NULL)
    {
        LOG_E("No %s partition found", part_name);
        return RESOURCE_FILE_OPEN_FAIL;
    }

    if (fal_partition_read(part, 0, (uint8_t *)&resource, sizeof(resource)) != sizeof(resource))
    {
        LOG_E("Read resource size failed for partition[%s]", part_name);
        return RESOURCE_FILE_OPEN_FAIL;
    }

    if (resource.location != RESOURCE_IN_EXTERNAL_STORAGE ||
            resource.size > (part->len - sizeof(resource_hnd_t)))
    {
        LOG_E("Read resource head error for partition[%s]", part_name);
        return RESOURCE_FILE_OPEN_FAIL;
    }

    *size_out = (uint32_t)resource_get_size(((const resource_hnd_t *)&resource));

    return WHD_SUCCESS;
}

static uint32_t host_get_resource_block_size (whd_driver_t whd_drv, whd_resource_type_t type, uint32_t *size_out)
{
    *size_out = WHD_RESOURCES_BLOCK_SIZE;
    return RESOURCE_SUCCESS;
}

static uint32_t host_get_resource_no_of_blocks (whd_driver_t whd_drv, whd_resource_type_t type, uint32_t *block_count)
{
    uint32_t resource_size = 0;
    uint32_t block_size;

    host_platform_resource_size(whd_drv, type, &resource_size);
    host_get_resource_block_size(whd_drv, type, &block_size);
    *block_count = resource_size / block_size;
    if (resource_size % block_size)
        *block_count += 1;

    return RESOURCE_SUCCESS;
}

static uint32_t host_get_resource_block (whd_driver_t whd_drv, whd_resource_type_t type, uint32_t blockno, const uint8_t **data, uint32_t *size_out)
{
    uint32_t resource_size;
    uint32_t block_size;
    uint32_t block_count;
    uint32_t read_pos;
    resource_hnd_t resource;
    const struct fal_partition *part;
    const char *part_name = name_list[type];

    host_platform_resource_size(whd_drv, type, &resource_size);
    host_get_resource_block_size(whd_drv, type, &block_size);
    host_get_resource_no_of_blocks(whd_drv, type, &block_count);
    memset(r_buffer, 0, block_size);
    read_pos = blockno * block_size;

    if (blockno >= block_count)
    {
        return WHD_BADARG;
    }

    if ((part = fal_partition_find(part_name)) == NULL)
    {
        LOG_E("No %s partition found", part_name);
        return RESOURCE_FILE_OPEN_FAIL;
    }

    if (fal_partition_read(part, 0, (uint8_t *)&resource, sizeof(resource)) != sizeof(resource))
    {
        LOG_E("Read block failed for partition[%s]", part_name);
        return RESOURCE_FILE_OPEN_FAIL;
    }

    if (resource.location != RESOURCE_IN_EXTERNAL_STORAGE ||
            resource.size > (part->len - sizeof(resource_hnd_t)))
    {
        LOG_E("Read resource head error for partition[%s]", part_name);
        return RESOURCE_FILE_OPEN_FAIL;
    }

    if (read_pos > resource.size)
    {
        LOG_E("Read position beyond resource size for partition[%s]", part_name);
        return RESOURCE_OFFSET_TOO_BIG;
    }

    *size_out = fal_partition_read(part, read_pos + sizeof(resource_hnd_t), r_buffer, MIN(block_size, resource.size - read_pos));
    *data = (uint8_t *)&r_buffer;

    if (type == WHD_RESOURCE_WLAN_NVRAM)
    {
        nvram_convert_line_endings((char *)*data, *size_out);
    }

    return RESOURCE_SUCCESS;
}

static uint32_t host_resource_read (whd_driver_t whd_drv, whd_resource_type_t type, uint32_t offset, uint32_t size, uint32_t *size_out, void *buffer)
{
    resource_hnd_t resource;
    const struct fal_partition *part;
    const char *part_name = name_list[type];

    if ((part = fal_partition_find(part_name)) == NULL)
    {
        LOG_E("No %s partition found", part_name);
        return RESOURCE_FILE_OPEN_FAIL;
    }

    if (fal_partition_read(part, 0, (uint8_t *)&resource, sizeof(resource)) != sizeof(resource))
    {
        LOG_E("Read failed for partition[%s]", part_name);
        return RESOURCE_FILE_OPEN_FAIL;
    }

    if (resource.location != RESOURCE_IN_EXTERNAL_STORAGE ||
            resource.size > (part->len - sizeof(resource_hnd_t)))
    {
        LOG_E("Read resource head error for partition[%s]", part_name);
        return RESOURCE_FILE_OPEN_FAIL;
    }

    if (offset > resource.size)
    {
        LOG_E("Read offset beyond resource size for partition[%s]", part_name);
        return RESOURCE_OFFSET_TOO_BIG;
    }

    *size_out = fal_partition_read(part, offset + sizeof(resource_hnd_t), buffer, MIN(size, resource.size - offset));

    if (type == WHD_RESOURCE_WLAN_NVRAM)
    {
        nvram_convert_line_endings((char *)buffer, *size_out);
    }

    return RESOURCE_SUCCESS;
}
#endif

#if defined(WHD_RESOURCES_IN_EXTERNAL_STORAGE_FS) || defined(WHD_RESOURCES_IN_SDCARD)

static rt_bool_t whd_resource_file_ready(const char *path)
{
    struct stat stat_buf;

    if (stat(path, &stat_buf) != 0)
    {
        return RT_FALSE;
    }

    return (stat_buf.st_size > 0) ? RT_TRUE : RT_FALSE;
}

void whd_wait_fs_mount(void)
{
    rt_tick_t start_tick = rt_tick_get();

    while (1)
    {
        if (whd_resource_file_ready(WHD_RESOURCES_FIRMWARE_PATH_NAME) &&
                whd_resource_file_ready(WHD_RESOURCES_CLM_PATH_NAME) &&
                whd_resource_file_ready(WHD_RESOURCES_NVRAM_PATH_NAME))
        {
            LOG_I("WHD resource files are ready");
            return;
        }

#if WHD_RESOURCES_FS_WAIT_TIMEOUT_MS > 0
        if ((rt_tick_get() - start_tick) >= rt_tick_from_millisecond(WHD_RESOURCES_FS_WAIT_TIMEOUT_MS))
        {
            LOG_E("Wait WHD resource files timeout: %s, %s, %s",
                  WHD_RESOURCES_FIRMWARE_PATH_NAME,
                  WHD_RESOURCES_CLM_PATH_NAME,
                  WHD_RESOURCES_NVRAM_PATH_NAME);
            return;
        }
#endif

        rt_thread_mdelay(200);
    }
}

static uint32_t host_platform_resource_size (whd_driver_t whd_drv, whd_resource_type_t type, uint32_t *size_out)
{
    int fd;
    struct stat stat;
    const char *path_name = name_list[type];

    if ((fd = open(path_name, O_RDONLY)) < 0)
    {
        LOG_E("No %s file found", path_name);
        return RESOURCE_FILE_OPEN_FAIL;
    }

    if (fstat(fd, &stat) < 0)
    {
        close(fd);
        LOG_E("Read failed for file[%s]", path_name);
        return RESOURCE_FILE_OPEN_FAIL;
    }

    *size_out = stat.st_size;

    close(fd);

    return WHD_SUCCESS;
}

static uint32_t host_get_resource_block_size (whd_driver_t whd_drv, whd_resource_type_t type, uint32_t *size_out)
{
    *size_out = WHD_RESOURCES_BLOCK_SIZE;
    return RESOURCE_SUCCESS;
}

static uint32_t host_get_resource_no_of_blocks (whd_driver_t whd_drv, whd_resource_type_t type, uint32_t *block_count)
{
    uint32_t resource_size = 0;
    uint32_t block_size;

    host_platform_resource_size(whd_drv, type, &resource_size);
    host_get_resource_block_size(whd_drv, type, &block_size);
    *block_count = resource_size / block_size;
    if (resource_size % block_size)
        *block_count += 1;

    return RESOURCE_SUCCESS;
}

static uint32_t host_get_resource_block (whd_driver_t whd_drv, whd_resource_type_t type, uint32_t blockno, const uint8_t **data, uint32_t *size_out)
{
    int fd;
    uint32_t resource_size = 0;
    uint32_t block_size;
    uint32_t block_count;
    uint32_t read_pos;
    const char *path_name = name_list[type];

    host_platform_resource_size(whd_drv, type, &resource_size);
    host_get_resource_block_size(whd_drv, type, &block_size);
    host_get_resource_no_of_blocks(whd_drv, type, &block_count);
    memset(r_buffer, 0, block_size);
    read_pos = blockno * block_size;

    if (blockno >= block_count)
    {
        return WHD_BADARG;
    }

    if ((fd = open(path_name, O_RDONLY)) < 0)
    {
        LOG_E("No %s file found", path_name);
        return RESOURCE_FILE_OPEN_FAIL;
    }

    if (read_pos > resource_size)
    {
        close(fd);
        return RESOURCE_OFFSET_TOO_BIG;
    }

    lseek(fd, read_pos, SEEK_SET);
    *size_out = read(fd, r_buffer, MIN(block_size, resource_size - read_pos));
    *data = (uint8_t *)&r_buffer;

    if (type == WHD_RESOURCE_WLAN_NVRAM)
    {
        nvram_convert_line_endings((char *)*data, *size_out);
    }

    close(fd);

    return RESOURCE_SUCCESS;
}

static uint32_t host_resource_read (whd_driver_t whd_drv, whd_resource_type_t type, uint32_t offset, uint32_t size, uint32_t *size_out, void *buffer)
{
    int fd;
    uint32_t resource_size = 0;
    const char *path_name = name_list[type];

    host_platform_resource_size(whd_drv, type, &resource_size);

    if ((fd = open(path_name, O_RDONLY)) < 0)
    {
        LOG_E("No %s file found", path_name);
        return RESOURCE_FILE_OPEN_FAIL;
    }

    if (offset > resource_size)
    {
        close(fd);
        return RESOURCE_OFFSET_TOO_BIG;
    }

    lseek(fd, offset, SEEK_SET);
    /* read directly into the provided buffer */
    *size_out = read(fd, buffer, MIN(size, resource_size - offset));

    if (type == WHD_RESOURCE_WLAN_NVRAM)
    {
        nvram_convert_line_endings((char *)buffer, *size_out);
    }

    close(fd);

    return RESOURCE_SUCCESS;
}
#endif

whd_resource_source_t resource_ops =
{
    .whd_resource_size = host_platform_resource_size,
    .whd_get_resource_block_size = host_get_resource_block_size,
    .whd_get_resource_no_of_blocks = host_get_resource_no_of_blocks,
    .whd_get_resource_block = host_get_resource_block,
    .whd_resource_read = host_resource_read
};
