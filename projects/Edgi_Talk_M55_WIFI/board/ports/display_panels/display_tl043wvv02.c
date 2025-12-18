/*******************************************************************************
* \file mtb_display_tl043wvv02.c
* \version 0.5.0
*
* \brief
* Provides implementation of the EK79007AD3 TFT DSI display driver library.
*
********************************************************************************
* \copyright
* Copyright 2024, Cypress Semiconductor Corporation (an Infineon company)
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

/*******************************************************************************
* Header Files
*******************************************************************************/
#include "display_tl043wvv02.h"

/*******************************************************************************
* Macros
*******************************************************************************/
#define GPIO_LOW                                         (0U)
#define GPIO_HIGH                                        (1U)

#define ONE_MS_DELAY                                     (1U)
#define PIN_HIGH_DELAY_MS                                (5U)

/* MIPI DSI command packet size/length in bytes */
#define PACKET_LENGTH                                    (2U)

/* Display controller command commit mask */
#define DC_CMD_COMMIT_MASK                               (0x00000001U)

#define RESET_VAL                                        (0U)

#define MAX_BRIGHTNESS_PERCENT                           (100U)

/* Macro to convert brightness percentage into PWM counter value */
#define BRIGHTNESS_PERCENT_TO_PWM_COUNT(percentage, period) \
    ((uint8_t)(((percentage) * (period)) / 100))

enum
{
    GENERIC_DATA,       //Generic Long Write, DT = 0x29
    DCS_CMD_DATA,       //DCS Long Write/write_LUT Command Packet, DT = 0x39
} cmd_pld_type;

typedef struct
{
    unsigned char        size;
    unsigned char        cmd_pld_type;
    unsigned char        buffer[49];

} lcd_table_setting_t;

const lcd_table_setting_t g_lcd_init_focuslcd[] =
{
    // 800*480 冠显
    {4, GENERIC_DATA,    {0x99, 0x71, 0x02, 0xa2}},
    {4, GENERIC_DATA,    {0x99, 0x71, 0x02, 0xa3}},
    {4, GENERIC_DATA,    {0x99, 0x71, 0x02, 0xa4}},

    {2, DCS_CMD_DATA,    {0xA4, 0x31}},   // 2Lanes

    {8, DCS_CMD_DATA,    {0xB0, 0x22, 0x57, 0x1E, 0x61, 0x2F, 0x57, 0x61}},   // VGH_VGL  (14v)

    {3, DCS_CMD_DATA,    {0xB7, 0x64, 0x64}},  //Source  (5v)

    {3, DCS_CMD_DATA,    {0xBF, 0xB4, 0xB4}},  //VCOM  (-2v)

    // Gamma----(5V)
    {38, GENERIC_DATA,   {0xC8, 0x00, 0x00, 0x0F, 0x1C, 0x34, 0x00, 0x60, 0x03, 0xA0, 0x06, 0x10, 0xFE, 0x06, 0x74, 0x03, 0x21, 0xC4, 0x00, 0x08, 0x00, 0x22, 0x46, 0x0F, 0x8F, 0x0A, 0x32, 0xF2, 0x0C, 0x42, 0x0C, 0xF3, 0x80, 0x00, 0xAB, 0xC0, 0x03, 0xC4}},
    {38, GENERIC_DATA,   {0xC9, 0x00, 0x00, 0x0F, 0x1C, 0x34, 0x00, 0x60, 0x03, 0xA0, 0x06, 0x10, 0xFE, 0x06, 0x74, 0x03, 0x21, 0xC4, 0x00, 0x08, 0x00, 0x22, 0x46, 0x0F, 0x8F, 0x0A, 0x32, 0xF2, 0x0C, 0x42, 0x0C, 0xF3, 0x80, 0x00, 0xAB, 0xC0, 0x03, 0xC4}},
    // Gamma----(5.5V)
    {38, GENERIC_DATA,   {0xC8, 0x00, 0x00, 0x13, 0x24, 0x44, 0x00, 0x74, 0x03, 0xB8, 0x04, 0x11, 0x16, 0x08, 0x86, 0x04, 0x21, 0xD3, 0x02, 0x10, 0x0F, 0x22, 0x4D, 0x0E, 0x90, 0x09, 0x32, 0xF0, 0x0B, 0x40, 0x0E, 0xF3, 0x7D, 0x0E, 0xA9, 0xBF, 0x03, 0xC4}},
    {38, GENERIC_DATA,   {0xC9, 0x00, 0x00, 0x13, 0x24, 0x44, 0x00, 0x74, 0x03, 0xB8, 0x04, 0x11, 0x16, 0x08, 0x86, 0x04, 0x21, 0xD3, 0x02, 0x10, 0x0F, 0x22, 0x4D, 0x0E, 0x90, 0x09, 0x32, 0xF0, 0x0B, 0x40, 0x0E, 0xF3, 0x7D, 0x0E, 0xA9, 0xBF, 0x03, 0xC4}},
    // GIP
    {7, DCS_CMD_DATA,    {0xD7, 0x10, 0x0C, 0x36, 0x19, 0x90, 0x90}},  // original : OK for USE 1000Mhz
    // {7, DCS_CMD_DATA,    {0xD7, 0x10, 0x0C, 0x36, 0x19, 0x90, 0x90}},  // original : teired off
    // {7, DCS_CMD_DATA,    {0xD7, 0x10, 0x0C, 0xB4, 0x19, 0x90, 0x90}},  // original : OK for USE 1000Mhz，modified at 07.23
    // {7, DCS_CMD_DATA,    {0xD7, 0x10, 0x0C, 0xB4, 0x19, 0xA0, 0xA0}},  // test
    // {7, DCS_CMD_DATA,    {0xD7, 0x00, 0x10, 0xCD, 0x08, 0xF0, 0xF0}},  // modified : half area above
    // {7, DCS_CMD_DATA,    {0xD7, 0x10, 0x2A, 0x90, 0x19, 0x90, 0x90}},  // new : has a horizontical area above under 1000Mhz

    {33, DCS_CMD_DATA,     {0xA3, 0x51, 0x03, 0x80, 0xCF, 0x44, 0x00, 0x00, 0x00, 0x00, 0x04, 0x78, 0x78, 0x00, 0x1A, 0x00, 0x45, 0x05, 0x00, 0x00, 0x00, 0x00, 0x46, 0x00, 0x00, 0x02, 0x20, 0x52, 0x00, 0x05, 0x00, 0x00, 0xFF}},
    {45, DCS_CMD_DATA,     {0xA6, 0x02, 0x00, 0x24, 0x55, 0x35, 0x00, 0x38, 0x00, 0x78, 0x78, 0x00, 0x24, 0x55, 0x36, 0x00, 0x37, 0x00, 0x78, 0x78, 0x02, 0xAC, 0x51, 0x3A, 0x00, 0x00, 0x00, 0x78, 0x78, 0x03, 0xAC, 0x21, 0x00, 0x04, 0x00, 0x00, 0x78, 0x78, 0x3e, 0x00, 0x06, 0x00, 0x00, 0x00, 0x00}},
    {49, DCS_CMD_DATA,     {0xA7, 0x19, 0x19, 0x00, 0x64, 0x40, 0x07, 0x16, 0x40, 0x00, 0x04, 0x03, 0x78, 0x78, 0x00, 0x64, 0x40, 0x25, 0x34, 0x00, 0x00, 0x02, 0x01, 0x78, 0x78, 0x00, 0x64, 0x40, 0x4B, 0x5A, 0x00, 0x00, 0x02, 0x01, 0x78, 0x78, 0x00, 0x24, 0x40, 0x69, 0x78, 0x00, 0x00, 0x00, 0x00, 0x78, 0x78, 0x00, 0x44}},
    {38, DCS_CMD_DATA,     {0xAC, 0x08, 0x0A, 0x11, 0x00, 0x13, 0x03, 0x1B, 0x18, 0x06, 0x1A, 0x19, 0x1B, 0x1B, 0x1B, 0x18, 0x1B, 0x09, 0x0B, 0x10, 0x02, 0x12, 0x01, 0x1B, 0x18, 0x06, 0x1A, 0x19, 0x1B, 0x1B, 0x1B, 0x18, 0x1B, 0xFF, 0x67, 0xFF, 0x67, 0x00}},

    {8, DCS_CMD_DATA,    {0xAD, 0xCC, 0x40, 0x46, 0x11, 0x04, 0x78, 0x78}},
    {15, DCS_CMD_DATA,   {0xE8, 0x30, 0x07, 0x00, 0x94, 0x94, 0x9C, 0x00, 0xE2, 0x04, 0x00, 0x00, 0x00, 0x00, 0xEF}},

    {34, DCS_CMD_DATA,   {0xE7, 0x8B, 0x3C, 0x00, 0x0C, 0xF0, 0x5D, 0x00, 0x5D, 0x00, 0x5D, 0x00, 0x5D, 0x00, 0xFF, 0x00, 0x08, 0x7B, 0x00, 0x00, 0xC8, 0x6A, 0x5A, 0x08, 0x1A, 0x3C, 0x00, 0x81, 0x01, 0xCC, 0x01, 0x7F, 0xF0, 0x22}},

    {2, DCS_CMD_DATA,    {0x35, 0x00}},  //TE off
    // {2, DCS_CMD_DATA , {0xB5, 0x85}},  //Screen Test Mode

};

static mtb_display_tl043wvv02_pin_config_t *disp_pin_config = NULL;
static mtb_display_tl043wvv02_backlight_config_t *backlight_config = NULL;

/* Variable to store period value of TCPWM block */
static uint32_t pwm_period = RESET_VAL;

/*******************************************************************************
* Function Name: mtb_display_tl043wvv02_init
********************************************************************************
*
* Performs EK79007AD3 TFT driver initialization using MIPI DSI interface.
*
* \param mipi_dsi_base
* Pointer to the MIPI DSI register base address.
*
* \param disp_tl043wvv02_pin_config
* Pointer to the EK79007AD3 display pin configuration structure.
*
* \return cy_en_mipidsi_status_t
* Initialization status.
*
* \funcusage
* \snippet snippet/main.c snippet_mtb_display_tl043wvv02_init
*
*******************************************************************************/
cy_en_mipidsi_status_t mtb_display_tl043wvv02_init(GFXSS_MIPIDSI_Type* mipi_dsi_base,
        mtb_display_tl043wvv02_pin_config_t *disp_tl043wvv02_pin_config)
{
    uint32_t i;
    cy_en_mipidsi_status_t status = CY_MIPIDSI_BAD_PARAM;

    CY_ASSERT(NULL != mipi_dsi_base);
    CY_ASSERT(NULL != disp_tl043wvv02_pin_config);

    disp_pin_config = disp_tl043wvv02_pin_config;

    /* Display pin initialization sequence */
    /* Initialize display RESET GPIO pin with initial value as HIGH */
    Cy_GPIO_Pin_FastInit(disp_pin_config->reset_port, disp_pin_config->reset_pin,
                         CY_GPIO_DM_STRONG_IN_OFF, GPIO_HIGH, HSIOM_SEL_GPIO);

    /* Perform reset */
    /* Pull the display RESET GPIO pin to LOW */
    Cy_GPIO_Write(disp_pin_config->reset_port, disp_pin_config->reset_pin, GPIO_LOW);
    Cy_SysLib_Delay(ONE_MS_DELAY * 50);

    /* Pull the display RESET GPIO pin to HIGH */
    Cy_GPIO_Write(disp_pin_config->reset_port, disp_pin_config->reset_pin, GPIO_HIGH);
    Cy_SysLib_Delay(PIN_HIGH_DELAY_MS * 25);

    /* Set the LCM init settings */
    for (i = 0; i < sizeof(g_lcd_init_focuslcd) / sizeof(g_lcd_init_focuslcd[0]); i++)
    {
        if (g_lcd_init_focuslcd[i].cmd_pld_type == GENERIC_DATA)
        {
            status = Cy_MIPIDSI_GenericWritePacket(mipi_dsi_base, g_lcd_init_focuslcd[i].buffer, g_lcd_init_focuslcd[i].size); // sizeof(g_lcd_init_focuslcd[i].buffer));
            viv_set_commit(DC_CMD_COMMIT_MASK);
            Cy_SysLib_Delay(ONE_MS_DELAY * 2);
        }
        else
        {
            status = Cy_MIPIDSI_WritePacket(mipi_dsi_base, g_lcd_init_focuslcd[i].buffer, g_lcd_init_focuslcd[i].size);
            viv_set_commit(DC_CMD_COMMIT_MASK);
        }
    }

    if (CY_MIPIDSI_SUCCESS == status)
    {
        Cy_SysLib_Delay(ONE_MS_DELAY);

        status = Cy_MIPIDSI_ExitSleep(mipi_dsi_base);

        if (CY_MIPIDSI_SUCCESS == status)
        {
            Cy_SysLib_Delay(ONE_MS_DELAY * 120);
            status = Cy_MIPIDSI_DisplayON(mipi_dsi_base);

            if (CY_MIPIDSI_SUCCESS == status)
            {
                viv_set_commit(DC_CMD_COMMIT_MASK);
                Cy_SysLib_Delay(ONE_MS_DELAY);
            }
        }
    }

    return status;
}

/*******************************************************************************
* Function Name: mtb_display_tl043wvv02_backlight_init
********************************************************************************
*
* Configures display backlight GPIO pin and enables PWM output on it.
*
* \param disp_tl043wvv02_backlight_config
* Pointer to the display backlight configuration structure.
*
* \return cy_en_tcpwm_status_t
* Display backlight PWM initialization status.
*
* \funcusage
* \snippet snippet/main.c snippet_mtb_display_tl043wvv02_backlight_init
*
*******************************************************************************/
cy_en_tcpwm_status_t mtb_display_tl043wvv02_backlight_init(
    mtb_display_tl043wvv02_backlight_config_t *disp_tl043wvv02_backlight_config)
{
    cy_en_tcpwm_status_t status = CY_TCPWM_BAD_PARAM;

    CY_ASSERT(NULL != disp_tl043wvv02_backlight_config);

    backlight_config = disp_tl043wvv02_backlight_config;

    /* Display backlight GPIO pin initialization in PWM mode */
    Cy_GPIO_Pin_FastInit(backlight_config->bl_port, backlight_config->bl_pin,
                         CY_GPIO_DM_STRONG_IN_OFF, GPIO_LOW, HSIOM_SEL_ACT_2);

    /* Initialize the TCPWM block for backlight pin */
    status = Cy_TCPWM_PWM_Init(backlight_config->pwm_hw, backlight_config->pwm_num,
                               backlight_config->pwm_config);

    if (CY_TCPWM_SUCCESS == status)
    {
        /* Enable the TCPWM block for backlight pin */
        Cy_TCPWM_PWM_Enable(backlight_config->pwm_hw, backlight_config->pwm_num);

        /* Fetch the initial values of period register */
        pwm_period =
            Cy_TCPWM_PWM_GetPeriod0(backlight_config->pwm_hw, backlight_config->pwm_num);

        /* Trigger a software start on the selected TCPWM */
        Cy_TCPWM_TriggerStart_Single(backlight_config->pwm_hw, backlight_config->pwm_num);
    }

    return status;
}

/*******************************************************************************
* Function Name: mtb_display_tl043wvv02_set_brightness
********************************************************************************
*
* Sets the brightness of the display panel to the desired percentage.
*
* \param brightness_percent
* Brightness value in percentage (0 to 100).
*
* \return void
*
*******************************************************************************/
void mtb_display_tl043wvv02_set_brightness(uint8_t brightness_percent)
{
    uint32_t compare0_value = 0;
    uint32_t compare1_value = 0;
    uint32_t counter_val    = 0;

    CY_ASSERT(MAX_BRIGHTNESS_PERCENT >= brightness_percent);

    counter_val = BRIGHTNESS_PERCENT_TO_PWM_COUNT(brightness_percent, pwm_period);
    compare0_value = compare1_value = (counter_val > pwm_period) ?
                                      pwm_period : counter_val;

    /* Set new values for CC0/1 compare buffers */
    Cy_TCPWM_PWM_SetCompare0BufVal(backlight_config->pwm_hw, backlight_config->pwm_num,
                                   compare0_value);
    Cy_TCPWM_PWM_SetCompare1BufVal(backlight_config->pwm_hw, backlight_config->pwm_num,
                                   compare1_value);

    /* Trigger compare swap with its buffer values */
    Cy_TCPWM_TriggerCaptureOrSwap_Single(backlight_config->pwm_hw, backlight_config->pwm_num);
}


/*******************************************************************************
* Function Name: mtb_display_tl043wvv02_deinit
********************************************************************************
*
* Performs de-initialization of the EK79007AD3 TFT driver using MIPI DSI interface.
* It also de-initializes PWM output on display backlight pin.
*
* \param mipi_dsi_base
* Pointer to the MIPI DSI register base address.
*
* \return cy_en_mipidsi_status_t
* De-initialization status.
*
*******************************************************************************/
cy_en_mipidsi_status_t mtb_display_tl043wvv02_deinit(GFXSS_MIPIDSI_Type* mipi_dsi_base)
{
    cy_en_mipidsi_status_t status = CY_MIPIDSI_BAD_PARAM;

    CY_ASSERT(NULL != mipi_dsi_base);
    CY_ASSERT(NULL != disp_pin_config);
    CY_ASSERT(NULL != backlight_config);

    status = Cy_MIPIDSI_EnterSleep(mipi_dsi_base);
    if (CY_MIPIDSI_SUCCESS == status)
    {
        /* Set display RESET GPIO pin to LOW */
        Cy_GPIO_Write(disp_pin_config->reset_port, disp_pin_config->reset_pin, GPIO_LOW);

        /* Stop and de-initialize PWM on display backlight pin */
        Cy_TCPWM_TriggerStopOrKill_Single(backlight_config->pwm_hw, backlight_config->pwm_num);
        Cy_TCPWM_PWM_Disable(backlight_config->pwm_hw, backlight_config->pwm_num);
        Cy_TCPWM_PWM_DeInit(backlight_config->pwm_hw, backlight_config->pwm_num,
                            backlight_config->pwm_config);
    }

    return status;
}


/* [] END OF FILE */
