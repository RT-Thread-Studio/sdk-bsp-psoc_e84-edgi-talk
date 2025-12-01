/*******************************************************************************
* File Name        : lv_port_disp.h
*
* Description      : This file provides constants and function prototypes
*                    for configuring low level display driver in LVGL.
*
* Related Document : See README.md
*
******************************************************************************/

#ifndef LV_PORT_DISP_H

#define LV_PORT_DISP_H

#ifdef __cplusplus
extern "C" {
#endif


/*******************************************************************************
* Header Files
*******************************************************************************/
#include "cybsp.h"
#include "cy_pdl.h"
#include "cycfg.h"

#include "lvgl.h"


/*******************************************************************************
* Macros
*******************************************************************************/
#define MY_DISP_HOR_RES     (512U)//(512U)//水平
#define MY_DISP_VER_RES     (800U) //(800U)//竖直

extern cy_stc_gfx_context_t gfx_context;
extern void *frame_buffer1;
extern void *frame_buffer2;
extern rt_sem_t flush_sem;

/*******************************************************************************
* Function Prototypes
*******************************************************************************/
/* Initialize low level display driver */
void lv_port_disp_init(void);


#ifdef __cplusplus
} /*extern "C"*/
#endif


#endif /*LV_PORT_DISP_H*/

/* [] END OF FILE */
