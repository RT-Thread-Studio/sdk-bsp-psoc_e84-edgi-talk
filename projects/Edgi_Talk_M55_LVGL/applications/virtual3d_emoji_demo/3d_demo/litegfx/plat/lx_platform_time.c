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
#include "lx_platform_time.h"
#include <rtthread.h>

 /*********************
 *      DEFINES
 *********************/


 /**********************
 *      TYPEDEFS
 **********************/


 /**********************
 *  STATIC PROTOTYPES
 **********************/


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
void lx_platform_get_time(lx_platform_time_t* time_p)
{
    uint32_t now_ms = rt_tick_get_millisecond();
    uint32_t seconds = now_ms / 1000U;

    if(time_p == RT_NULL)
    {
        return;
    }

    time_p->year = 2026;
    time_p->month = 1;
    time_p->mday = 1;
    time_p->hour = (seconds / 3600U) % 24U;
    time_p->minute = (seconds / 60U) % 60U;
    time_p->second = seconds % 60U;
    time_p->weekday = 4;
    time_p->millisecond = now_ms % 1000U;
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

