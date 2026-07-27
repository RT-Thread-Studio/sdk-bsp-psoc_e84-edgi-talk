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
#include "lx_platform_log.h"
#include <stdio.h>
#include <stdbool.h>
#include <vg_lite.h>

/*********************
*      DEFINES
*********************/
const uint8_t lx_log_level = LX_LOG_LEVEL_USER;

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
void lx_platform_trace(const char* buf)
{
	LX_PRINTF(buf);
}

void lx_platform_assert(const char* file, uint32_t line)
{
	//assert(0);
}

void lx_platform_check_vglite(uint32_t baseOnVer)
{
	if(baseOnVer != VGLITE_RELEASE_VERSION)
	{
		LX_PRINTF("Warning!!! vglite version mismatched: 0x%x != 0x%x\n", baseOnVer, VGLITE_RELEASE_VERSION);
	}
}

/**********************
*   STATIC FUNCTIONS
**********************/
