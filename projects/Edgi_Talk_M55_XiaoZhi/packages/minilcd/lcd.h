#ifndef __LCD_H
#define __LCD_H

#include <rtthread.h>

void LCD_Fill(rt_uint16_t xsta, rt_uint16_t ysta, rt_uint16_t xend, rt_uint16_t yend, rt_uint16_t color);
void LCD_DrawPoint(rt_uint16_t x, rt_uint16_t y, rt_uint16_t color);
void LCD_DrawLine(rt_uint16_t x1, rt_uint16_t y1, rt_uint16_t x2, rt_uint16_t y2, rt_uint16_t color);
void LCD_DrawRectangle(rt_uint16_t x1, rt_uint16_t y1, rt_uint16_t x2, rt_uint16_t y2, rt_uint16_t color);
void Draw_Circle(rt_uint16_t x0, rt_uint16_t y0, rt_uint8_t r, rt_uint16_t color);

void LCD_ShowChinese(rt_uint16_t x, rt_uint16_t y, rt_uint8_t *s, rt_uint16_t fc, rt_uint16_t bc, rt_uint8_t sizey, rt_uint8_t mode);
void LCD_ShowChinese12x12(rt_uint16_t x, rt_uint16_t y, rt_uint8_t *s, rt_uint16_t fc, rt_uint16_t bc, rt_uint8_t sizey, rt_uint8_t mode);
void LCD_ShowChinese16x16(rt_uint16_t x, rt_uint16_t y, rt_uint8_t *s, rt_uint16_t fc, rt_uint16_t bc, rt_uint8_t sizey, rt_uint8_t mode);
void LCD_ShowChinese24x24(rt_uint16_t x, rt_uint16_t y, rt_uint8_t *s, rt_uint16_t fc, rt_uint16_t bc, rt_uint8_t sizey, rt_uint8_t mode);
void LCD_ShowChinese32x32(rt_uint16_t x, rt_uint16_t y, rt_uint8_t *s, rt_uint16_t fc, rt_uint16_t bc, rt_uint8_t sizey, rt_uint8_t mode);

void LCD_ShowChar(rt_uint16_t x, rt_uint16_t y, rt_uint8_t num, rt_uint16_t fc, rt_uint16_t bc, rt_uint8_t sizey, rt_uint8_t mode);
void LCD_ShowString(rt_uint16_t x, rt_uint16_t y, const rt_uint8_t *p, rt_uint16_t fc, rt_uint16_t bc, rt_uint8_t sizey, rt_uint8_t mode);
rt_uint32_t mypow(rt_uint8_t m, rt_uint8_t n); //����
void LCD_ShowIntNum(rt_uint16_t x, rt_uint16_t y, rt_uint16_t num, rt_uint8_t len, rt_uint16_t fc, rt_uint16_t bc, rt_uint8_t sizey);
void LCD_ShowFloatNum1(rt_uint16_t x, rt_uint16_t y, float num, rt_uint8_t len, rt_uint16_t fc, rt_uint16_t bc, rt_uint8_t sizey);

void LCD_ShowPicture(rt_uint16_t x, rt_uint16_t y, rt_uint16_t length, rt_uint16_t width, const rt_uint8_t pic[]);

#define WHITE            0xFFFF
#define BLACK            0x0000
#define BLUE             0x001F
#define BRED             0XF81F
#define GRED                   0XFFE0
#define GBLUE                  0X07FF
#define RED              0xF800
#define MAGENTA          0xF81F
#define GREEN            0x07E0
#define CYAN             0x7FFF
#define YELLOW           0xFFE0
#define BROWN                0XBC40
#define BRRED                0XFC07
#define GRAY                 0X8430
#define DARKBLUE         0X01CF
#define LIGHTBLUE        0X7D7C
#define GRAYBLUE         0X5458
#define LIGHTGREEN       0X841F
#define LGRAY                0XC618
#define LGRAYBLUE        0XA651
#define LBBLUE           0X2B12

#endif





