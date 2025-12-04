#include "lcd.h"
#include "lcd_init.h"


void LCD_Fill(rt_uint16_t xsta, rt_uint16_t ysta, rt_uint16_t xend, rt_uint16_t yend, rt_uint16_t color)
{
    rt_uint16_t i, j;
    LCD_Address_Set(xsta, ysta, xend - 1, yend - 1);
    for (i = ysta; i < yend; i++)
    {
        for (j = xsta; j < xend; j++)
        {
            LCD_WR_DATA(color);
        }
    }
}

void LCD_DrawPoint(rt_uint16_t x, rt_uint16_t y, rt_uint16_t color)
{
    LCD_Address_Set(x, y, x, y);
    LCD_WR_DATA(color);
}

void LCD_DrawLine(rt_uint16_t x1, rt_uint16_t y1, rt_uint16_t x2, rt_uint16_t y2, rt_uint16_t color)
{
    rt_uint16_t t;
    int xerr = 0, yerr = 0, delta_x, delta_y, distance;
    int incx, incy, uRow, uCol;
    delta_x = x2 - x1;
    delta_y = y2 - y1;
    uRow = x1;
    uCol = y1;
    if (delta_x > 0)incx = 1;
    else if (delta_x == 0)incx = 0;
    else
    {
        incx = -1;
        delta_x = -delta_x;
    }
    if (delta_y > 0)incy = 1;
    else if (delta_y == 0)incy = 0;
    else
    {
        incy = -1;
        delta_y = -delta_y;
    }
    if (delta_x > delta_y)distance = delta_x;
    else distance = delta_y;
    for (t = 0; t < distance + 1; t++)
    {
        LCD_DrawPoint(uRow, uCol, color);
        xerr += delta_x;
        yerr += delta_y;
        if (xerr > distance)
        {
            xerr -= distance;
            uRow += incx;
        }
        if (yerr > distance)
        {
            yerr -= distance;
            uCol += incy;
        }
    }
}


void LCD_DrawRectangle(rt_uint16_t x1, rt_uint16_t y1, rt_uint16_t x2, rt_uint16_t y2, rt_uint16_t color)
{
    LCD_DrawLine(x1, y1, x2, y1, color);
    LCD_DrawLine(x1, y1, x1, y2, color);
    LCD_DrawLine(x1, y2, x2, y2, color);
    LCD_DrawLine(x2, y1, x2, y2, color);
}


void Draw_Circle(rt_uint16_t x0, rt_uint16_t y0, rt_uint8_t r, rt_uint16_t color)
{
    int a, b;
    a = 0;
    b = r;
    while (a <= b)
    {
        LCD_DrawPoint(x0 - b, y0 - a, color);       //3
        LCD_DrawPoint(x0 + b, y0 - a, color);       //0
        LCD_DrawPoint(x0 - a, y0 + b, color);       //1
        LCD_DrawPoint(x0 - a, y0 - b, color);       //2
        LCD_DrawPoint(x0 + b, y0 + a, color);       //4
        LCD_DrawPoint(x0 + a, y0 - b, color);       //5
        LCD_DrawPoint(x0 + a, y0 + b, color);       //6
        LCD_DrawPoint(x0 - b, y0 + a, color);       //7
        a++;
        if ((a * a + b * b) > (r * r))
        {
            b--;
        }
    }
}

void LCD_ShowPicture(rt_uint16_t x, rt_uint16_t y, rt_uint16_t length, rt_uint16_t width, const rt_uint8_t pic[])
{
    rt_uint16_t i, j;
    rt_uint32_t k = 0;
    LCD_Address_Set(x, y, x + length - 1, y + width - 1);
    for (i = 0; i < length; i++)
    {
        for (j = 0; j < width; j++)
        {
            LCD_WR_DATA8(pic[k * 2]);
            LCD_WR_DATA8(pic[k * 2 + 1]);
            k++;
        }
    }
}


