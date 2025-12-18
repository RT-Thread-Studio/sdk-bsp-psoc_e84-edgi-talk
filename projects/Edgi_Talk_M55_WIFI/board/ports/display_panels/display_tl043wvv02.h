/*******************************************************************************
* \file display_tl043wvv02.h
*
* \brief
* Provides constants, parameter values, and API prototypes for the EK79007AD3
* TFT Display DSI driver library.
*
********************************************************************************
* \copyright
* Copyright 2024-2025, Cypress Semiconductor Corporation (an Infineon company)
* SPDX-License-Identifier: Apache-2.0
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*     http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*******************************************************************************/

#ifndef DISPLAY_TL043WVV02_H
#define DISPLAY_TL043WVV02_H


#if defined(__cplusplus)
extern "C" {
#endif


/*******************************************************************************
* Header Files
*******************************************************************************/
#include "cy_graphics.h"
#include "cy_pdl.h"


/*******************************************************************************
* Macros
*******************************************************************************/
/* Panel register(s) */
#define MTB_DISPLAY_EK79007AD3_PANEL_CTRL_REG            (0xB2)

/* Enable/disable 2 lane MIPI interface bit position */
#define MTB_DISPLAY_EK79007AD3_EN_2LANE_BIT_POS          (4UL)

/* Enable/disable 2 lane MIPI interface bit mask */
#define MTB_DISPLAY_EK79007AD3_EN_2LANE_MASK             \
    (1 << MTB_DISPLAY_EK79007AD3_EN_2LANE_BIT_POS)

/* Device max DPHY clock in Hz unit */
#define MIPI_MAX_PHY_CLK_HZ                              (2500000000UL)

/* Display specific configuration parameters */
#define MTB_DISPLAY_EK79007AD3_PANEL_NUM_LANES           (2U)

/* Default display resolution in horizontal direction (in pixels) */
#define MTB_DISPLAY_EK79007AD3_DEFAULT_HOR_RES           (1024U)

/* Default display resolution in vertical direction (in pixels)  */
#define MTB_DISPLAY_EK79007AD3_DEFAULT_VER_RES           (600U)

/* Display panel timings as per EK79007AD3 datasheet for 2 lanes */
#define MTB_DISPLAY_EK79007AD3_PANEL_HSYNC_WIDTH         (100U)
#define MTB_DISPLAY_EK79007AD3_PANEL_HFP                 (100U)
#define MTB_DISPLAY_EK79007AD3_PANEL_HBP                 (100U)
#define MTB_DISPLAY_EK79007AD3_PANEL_VSYNC_WIDTH         (10U)
#define MTB_DISPLAY_EK79007AD3_PANEL_VFP                 (10U)
#define MTB_DISPLAY_EK79007AD3_PANEL_VBP                 (10U)

#define MTB_DISPLAY_EK79007AD3_PANEL_PER_LANE_MBPS       (500U)

/* Pixel clock in KHz */
/* Pixel clock = ((hsync_width + hfp + hbp + display_width) *
 * (vsync_width + vfp + vbp + display_height) * fps / 1000)
 * Here fps is considered as 45 to achieve bit rate <= 500 Mbps to
 * match the display specification.
 */
#define MTB_DISPLAY_EK79007AD3_PANEL_PIXEL_CLK           (41706U)

/* PWM parameters */
#define DEFAULT_PWM_PERIOD                               (32768U)

/* Display backlight PWM parameters */
#define MTB_DISPLAY_BACKLIGHT_PWM_INPUT_DISABLED         (0x7U)
#define MTB_DISPLAY_BACKLIGHT_PWM_PERIOD0                (200U)
#define MTB_DISPLAY_BACKLIGHT_PWM_COMPARE0               (10U)
#define MTB_DISPLAY_BACKLIGHT_PWM_COMPARE2               (10U)
#define MTB_DISPLAY_BACKLIGHT_ENABLED_TAPS               (45U)


/*******************************************************************************
* Data Structures
*******************************************************************************/
/* EK79007AD3 display pin configuration structure */
typedef struct
{
    GPIO_PRT_Type *reset_port;
    uint32_t reset_pin;
} mtb_display_tl043wvv02_pin_config_t;

/* Display backlight configuration structure */
typedef struct
{
    GPIO_PRT_Type *bl_port;
    uint32_t bl_pin;
    TCPWM_Type *pwm_hw;
    uint32_t pwm_num;
    cy_stc_tcpwm_pwm_config_t *pwm_config;
} mtb_display_tl043wvv02_backlight_config_t;


/*******************************************************************************
* Global Variables
*******************************************************************************/
/* Display specific MIPI DSI configurations */
extern cy_stc_mipidsi_display_params_t mtb_display_tl043wvv02_mipidsi_display_params;
extern cy_stc_mipidsi_config_t mtb_display_tl043wvv02_mipidsi_config;

/* Display backlight configuration */
extern cy_stc_tcpwm_pwm_config_t mtb_display_tl043wvv02_backlight_pwm_config;


/*******************************************************************************
* Function Prototypes
*******************************************************************************/
cy_en_mipidsi_status_t mtb_display_tl043wvv02_init(GFXSS_MIPIDSI_Type* mipi_dsi_base,
        mtb_display_tl043wvv02_pin_config_t *disp_tl043wvv02_pin_config);

cy_en_tcpwm_status_t mtb_display_tl043wvv02_backlight_init(
    mtb_display_tl043wvv02_backlight_config_t *disp_tl043wvv02_backlight_config);
void mtb_display_tl043wvv02_set_brightness(uint8_t brightness_percent);
cy_en_mipidsi_status_t mtb_display_tl043wvv02_deinit(GFXSS_MIPIDSI_Type* mipi_dsi_base);


#if defined(__cplusplus)
}
#endif /* __cplusplus */


#endif /* DISPLAY_TL043WVV02_H */


/* [] END OF FILE */
