/*
 * Copyright (c) 2006-2023, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2018-11-15     SummerGift   first version
 */

/*
 * NOTE: DO NOT include this file on the header file.
 */

#ifndef LOG_TAG
#define DBG_TAG               "drv"
#else
#define DBG_TAG               LOG_TAG
#endif /* LOG_TAG */

#ifdef DRV_ERROR
#define DBG_LVL               DBG_ERROR
#elif defined(DRV_WARNING)
#define DBG_LVL               DBG_WARNING
#elif defined(DRV_INFO)
#define DBG_LVL               DBG_INFO
#elif defined(DRV_DEBUG)
#define DBG_LVL               DBG_LOG
#endif /* DRV_ERROR */

#include <rtdbg.h>
