#include "lcd_init.h"
//#include "delay.h"

void LCD_GPIO_Init(void)
{

    // rt_pin_mode(GET_PIN(G, 12),PIN_MODE_OUTPUT);
    // rt_pin_mode(GET_PIN(D, 1),PIN_MODE_OUTPUT);
    // rt_pin_mode(GET_PIN(D, 4),PIN_MODE_OUTPUT);
    // rt_pin_mode(GET_PIN(D, 5),PIN_MODE_OUTPUT);
    // rt_pin_mode(GET_PIN(D, 15),PIN_MODE_OUTPUT);
    // rt_pin_mode(GET_PIN(E, 8),PIN_MODE_OUTPUT);
    rt_pin_mode(GET_PIN(15, 0), PIN_MODE_OUTPUT);
    rt_pin_mode(GET_PIN(15, 1), PIN_MODE_OUTPUT);
    rt_pin_mode(GET_PIN(13, 6), PIN_MODE_OUTPUT);
    rt_pin_mode(GET_PIN(15, 4), PIN_MODE_OUTPUT);
    rt_pin_mode(GET_PIN(15, 3), PIN_MODE_OUTPUT);
    rt_pin_mode(GET_PIN(11, 2), PIN_MODE_OUTPUT);

}

void LCD_Writ_Bus(rt_uint8_t dat)
{
    rt_uint8_t i;
    LCD_CS_Clr();
    for (i = 0; i < 8; i++)
    {
        LCD_SCLK_Clr();
        if (dat & 0x80)
        {
            LCD_MOSI_Set();
        }
        else
        {
            LCD_MOSI_Clr();
        }
        LCD_SCLK_Set();
        dat <<= 1;
    }
    LCD_CS_Set();
}

void LCD_WR_DATA8(rt_uint8_t dat)
{
    LCD_Writ_Bus(dat);
}

void LCD_WR_DATA(rt_uint16_t dat)
{
    LCD_Writ_Bus(dat >> 8);
    LCD_Writ_Bus(dat);
}


void LCD_WR_REG(rt_uint8_t dat)
{
    LCD_DC_Clr();
    LCD_Writ_Bus(dat);
    LCD_DC_Set();
}

void LCD_Address_Set(rt_uint16_t x1, rt_uint16_t y1, rt_uint16_t x2, rt_uint16_t y2)
{
    if (USE_HORIZONTAL == 0)
    {
        LCD_WR_REG(0x2a);
        LCD_WR_DATA(x1 + 26);
        LCD_WR_DATA(x2 + 26);
        LCD_WR_REG(0x2b);
        LCD_WR_DATA(y1 + 1);
        LCD_WR_DATA(y2 + 1);
        LCD_WR_REG(0x2c);
    }
    else if (USE_HORIZONTAL == 1)
    {
        LCD_WR_REG(0x2a);
        LCD_WR_DATA(x1 + 26);
        LCD_WR_DATA(x2 + 26);
        LCD_WR_REG(0x2b);
        LCD_WR_DATA(y1 + 1);
        LCD_WR_DATA(y2 + 1);
        LCD_WR_REG(0x2c);
    }
    else if (USE_HORIZONTAL == 2)
    {
        LCD_WR_REG(0x2a);
        LCD_WR_DATA(x1 + 1);
        LCD_WR_DATA(x2 + 1);
        LCD_WR_REG(0x2b);
        LCD_WR_DATA(y1 + 26);
        LCD_WR_DATA(y2 + 26);
        LCD_WR_REG(0x2c);
    }
    else
    {
        LCD_WR_REG(0x2a);
        LCD_WR_DATA(x1 + 1);
        LCD_WR_DATA(x2 + 1);
        LCD_WR_REG(0x2b);
        LCD_WR_DATA(y1 + 26);
        LCD_WR_DATA(y2 + 26);
        LCD_WR_REG(0x2c);
    }
}

void LCD_Init(void)
{
    LCD_GPIO_Init();

    LCD_RES_Clr();
    rt_thread_mdelay(100);
    LCD_RES_Set();
    rt_thread_mdelay(100);

    LCD_BLK_Set();
    rt_thread_mdelay(100);

    LCD_WR_REG(0x11);
    rt_thread_mdelay(120);
    LCD_WR_REG(0xB1);
    LCD_WR_DATA8(0x05);
    LCD_WR_DATA8(0x3C);
    LCD_WR_DATA8(0x3C);
    LCD_WR_REG(0xB2);
    LCD_WR_DATA8(0x05);
    LCD_WR_DATA8(0x3C);
    LCD_WR_DATA8(0x3C);
    LCD_WR_REG(0xB3);
    LCD_WR_DATA8(0x05);
    LCD_WR_DATA8(0x3C);
    LCD_WR_DATA8(0x3C);
    LCD_WR_DATA8(0x05);
    LCD_WR_DATA8(0x3C);
    LCD_WR_DATA8(0x3C);
    LCD_WR_REG(0xB4);
    LCD_WR_DATA8(0x03);
    LCD_WR_REG(0xC0);
    LCD_WR_DATA8(0xAB);
    LCD_WR_DATA8(0x0B);
    LCD_WR_DATA8(0x04);
    LCD_WR_REG(0xC1);
    LCD_WR_DATA8(0xC5);
    LCD_WR_REG(0xC2);
    LCD_WR_DATA8(0x0D);
    LCD_WR_DATA8(0x00);
    LCD_WR_REG(0xC3);
    LCD_WR_DATA8(0x8D);
    LCD_WR_DATA8(0x6A);
    LCD_WR_REG(0xC4);
    LCD_WR_DATA8(0x8D);
    LCD_WR_DATA8(0xEE);
    LCD_WR_REG(0xC5);
    LCD_WR_DATA8(0x0F);
    LCD_WR_REG(0xE0);
    LCD_WR_DATA8(0x07);
    LCD_WR_DATA8(0x0E);
    LCD_WR_DATA8(0x08);
    LCD_WR_DATA8(0x07);
    LCD_WR_DATA8(0x10);
    LCD_WR_DATA8(0x07);
    LCD_WR_DATA8(0x02);
    LCD_WR_DATA8(0x07);
    LCD_WR_DATA8(0x09);
    LCD_WR_DATA8(0x0F);
    LCD_WR_DATA8(0x25);
    LCD_WR_DATA8(0x36);
    LCD_WR_DATA8(0x00);
    LCD_WR_DATA8(0x08);
    LCD_WR_DATA8(0x04);
    LCD_WR_DATA8(0x10);
    LCD_WR_REG(0xE1);
    LCD_WR_DATA8(0x0A);
    LCD_WR_DATA8(0x0D);
    LCD_WR_DATA8(0x08);
    LCD_WR_DATA8(0x07);
    LCD_WR_DATA8(0x0F);
    LCD_WR_DATA8(0x07);
    LCD_WR_DATA8(0x02);
    LCD_WR_DATA8(0x07);
    LCD_WR_DATA8(0x09);
    LCD_WR_DATA8(0x0F);
    LCD_WR_DATA8(0x25);
    LCD_WR_DATA8(0x35);
    LCD_WR_DATA8(0x00);
    LCD_WR_DATA8(0x09);
    LCD_WR_DATA8(0x04);
    LCD_WR_DATA8(0x10);

    LCD_WR_REG(0xFC);
    LCD_WR_DATA8(0x80);

    LCD_WR_REG(0x3A);
    LCD_WR_DATA8(0x05);
    LCD_WR_REG(0x36);
    if (USE_HORIZONTAL == 0)LCD_WR_DATA8(0x08);
    else if (USE_HORIZONTAL == 1)LCD_WR_DATA8(0xC8);
    else if (USE_HORIZONTAL == 2)LCD_WR_DATA8(0x78);
    else LCD_WR_DATA8(0xA8);
    LCD_WR_REG(0x21);
    LCD_WR_REG(0x29);
    LCD_WR_REG(0x2A);
    LCD_WR_DATA8(0x00);
    LCD_WR_DATA8(0x1A);
    LCD_WR_DATA8(0x00);
    LCD_WR_DATA8(0x69);
    LCD_WR_REG(0x2B);
    LCD_WR_DATA8(0x00);
    LCD_WR_DATA8(0x01);
    LCD_WR_DATA8(0x00);
    LCD_WR_DATA8(0xA0);
    LCD_WR_REG(0x2C);
}
