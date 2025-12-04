/*******************************************************************************
* File Name        : lv_port_indev.h
*
* Description      : This file provides constants and function prototypes
*                    used for configuring low level input device drivers
*                    (touchpad, mousepad, keypad etc.) in LVGL.
*
* Related Document : See README.md
*
*******************************************************************************/

#ifndef LV_PORT_INDEV_H

#define LV_PORT_INDEV_H

#ifdef __cplusplus
extern "C" {
#endif


/*******************************************************************************
* Header Files
*******************************************************************************/
#include "lvgl.h"
#include "cy_scb_i2c.h"


/*******************************************************************************
* Variables
*******************************************************************************/
extern cy_stc_scb_i2c_context_t disp_touch_i2c_controller_context;


/*******************************************************************************
* Function Prototypes
*******************************************************************************/
void lv_port_indev_init(void);


#ifdef __cplusplus
} /*extern "C"*/
#endif


#endif /* LV_PORT_INDEV_H */

/* [] END OF FILE */
