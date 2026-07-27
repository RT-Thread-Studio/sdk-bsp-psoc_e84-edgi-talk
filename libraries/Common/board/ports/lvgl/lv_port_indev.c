/*******************************************************************************
* File Name        : lv_port_indev.c
*
* Description      : This file provides implementation of low level input device
*                    driver for LVGL.
*
* Related Document : See README.md
*
******************************************************************************/

/*******************************************************************************
* Header Files
*******************************************************************************/
#include "lv_port_indev.h"
#include "lv_port_disp.h"
#include "cy_utils.h"
#include "drv_touch.h"
#include "cybsp.h"

/*******************************************************************************
* Global Variables
*******************************************************************************/
lv_indev_t *indev_touchpad;

#define TOUCH_PHYS_HOR_RES ((int32_t)BSP_LCD_PHYSICAL_HOR_RES)
#define TOUCH_PHYS_VER_RES ((int32_t)BSP_LCD_PHYSICAL_VER_RES)

static int32_t touch_clamp(int32_t value, int32_t min, int32_t max)
{
    if (value < min)
    {
        return min;
    }
    if (value > max)
    {
        return max;
    }
    return value;
}

static void touchpad_transform_point(rt_int16_t raw_x, rt_int16_t raw_y, lv_point_t *point)
{
    int32_t x;
    int32_t y;

#if defined(BSP_LCD_ROTATION_DEGREES) && (BSP_LCD_ROTATION_DEGREES == 90)
    x = raw_y;
    y = (TOUCH_PHYS_HOR_RES - 1) - raw_x;
#elif defined(BSP_LCD_ROTATION_DEGREES) && (BSP_LCD_ROTATION_DEGREES == 180)
    x = (TOUCH_PHYS_HOR_RES - 1) - raw_x;
    y = (TOUCH_PHYS_VER_RES - 1) - raw_y;
#elif defined(BSP_LCD_ROTATION_DEGREES) && (BSP_LCD_ROTATION_DEGREES == 270)
    x = (TOUCH_PHYS_VER_RES - 1) - raw_y;
    y = raw_x;
#else
    x = raw_x;
    y = raw_y;
#endif

    point->x = touch_clamp(x, 0, MY_DISP_HOR_RES - 1);
    point->y = touch_clamp(y, 0, MY_DISP_VER_RES - 1);
}

/*******************************************************************************
* Function Name: touchpad_init
********************************************************************************
* Summary:
*  Initialization function for touchpad supported by LittelvGL.
*
* Parameters:
*  void
*
* Return:
*  void
*
*******************************************************************************/
static void touchpad_init(void)
{
    cy_rslt_t result = CY_RSLT_SUCCESS;

    if (rt_hw_ST7102_port() != result)
    {
        CY_ASSERT(0);
    }
}


/*******************************************************************************
* Function Name: touchpad_read
********************************************************************************
* Summary:
*  Touchpad read function called by the LVGL library.
*  Here you will find example implementation of input devices supported by
*  LittelvGL:
*   - Touchpad
*   - Mouse (with cursor support)
*   - Keypad (supports GUI usage only with key)
*   - Encoder (supports GUI usage only with: left, right, push)
*   - Button (external buttons to press points on the screen)
*
*   The `..._read()` function are only examples.
*   You should shape them according to your hardware.
*
*
* Parameters:
*  *indev_drv: Pointer to the input driver structure to be registered by HAL.
*  *data: Pointer to the data buffer holding touch coordinates.
*
* Return:
*  void
*
*******************************************************************************/
static void touchpad_read(lv_indev_t *indev_drv, lv_indev_data_t *data)
{
    static lv_point_t last_point = {0, 0};
    rt_int16_t touch_x = 0;
    rt_int16_t touch_y = 0;
    cy_rslt_t result = CY_RSLT_SUCCESS;

    (void)indev_drv;

    data->state = LV_INDEV_STATE_REL;
    result = ST7102_get_single_touch(&touch_x, &touch_y);
    if (CY_RSLT_SUCCESS == result)
    {
        touchpad_transform_point(touch_x, touch_y, &last_point);
        data->state = LV_INDEV_STATE_PR;
    }
    /* Set the last pressed coordinates */
    data->point = last_point;
}


/*******************************************************************************
* Function Name: lv_port_indev_init
********************************************************************************
* Summary:
*  Initialization function for input devices supported by LittelvGL.
*
* Parameters:
*  void
*
* Return:
*  void
*
*******************************************************************************/
void lv_port_indev_init(void)
{
    /* Initialize your touchpad if you have. */
    touchpad_init();

    /* Register a touchpad input device */
    indev_touchpad = lv_indev_create();
    lv_indev_set_type(indev_touchpad, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(indev_touchpad, touchpad_read);
}


/* [] END OF FILE */
