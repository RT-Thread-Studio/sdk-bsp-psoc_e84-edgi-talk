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
#ifndef BSP_LCD_PHYSICAL_HOR_RES
#define BSP_LCD_PHYSICAL_HOR_RES     (480U)
#endif

#ifndef BSP_LCD_PHYSICAL_VER_RES
#define BSP_LCD_PHYSICAL_VER_RES     (800U)
#endif

#ifndef BSP_LCD_ROTATION_DEGREES
#define BSP_LCD_ROTATION_DEGREES     0
#endif

#if ((BSP_LCD_ROTATION_DEGREES != 0) && (BSP_LCD_ROTATION_DEGREES != 90) && \
     (BSP_LCD_ROTATION_DEGREES != 180) && (BSP_LCD_ROTATION_DEGREES != 270))
#error "BSP_LCD_ROTATION_DEGREES must be 0, 90, 180, or 270"
#endif

#if ((BSP_LCD_ROTATION_DEGREES == 90) || (BSP_LCD_ROTATION_DEGREES == 270))
#define MY_DISP_HOR_RES     BSP_LCD_PHYSICAL_VER_RES
#define MY_DISP_VER_RES     BSP_LCD_PHYSICAL_HOR_RES
#else
#define MY_DISP_HOR_RES     BSP_LCD_PHYSICAL_HOR_RES
#define MY_DISP_VER_RES     BSP_LCD_PHYSICAL_VER_RES
#endif

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
