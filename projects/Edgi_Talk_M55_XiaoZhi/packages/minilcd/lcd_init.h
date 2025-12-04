#ifndef __LCD_INIT_H
#define __LCD_INIT_H

//#include "sys.h"
#include <rtthread.h>
#include <rtdevice.h>
#include <board.h>
#define USE_HORIZONTAL 2


#if USE_HORIZONTAL==0||USE_HORIZONTAL==1
    #define LCD_W 80
    #define LCD_H 160

#else
    #define LCD_W 160
    #define LCD_H 80
#endif


#define LCD_SCLK_Clr() rt_pin_write(GET_PIN(15,0), 0)//SCL=SCLK
#define LCD_SCLK_Set() rt_pin_write(GET_PIN(15,0), 1)

#define LCD_MOSI_Clr() rt_pin_write(GET_PIN(15,1), 0)//SDA=MOSI
#define LCD_MOSI_Set() rt_pin_write(GET_PIN(15,1), 1)

#define LCD_RES_Clr()  rt_pin_write(GET_PIN(13,6), 0)//RESGPIO_Pin_4
#define LCD_RES_Set()  rt_pin_write(GET_PIN(13,6), 1)

#define LCD_DC_Clr()   rt_pin_write(GET_PIN(15,4), 0)//DCGPIO_Pin_6
#define LCD_DC_Set()   rt_pin_write(GET_PIN(15,4), 1)

#define LCD_CS_Clr()   rt_pin_write(GET_PIN(15,3), 0)//CS
#define LCD_CS_Set()   rt_pin_write(GET_PIN(15,3), 1)

#define LCD_BLK_Clr()  rt_pin_write(GET_PIN(11,2), 0)//BLK
#define LCD_BLK_Set()  rt_pin_write(GET_PIN(11,2), 1)




void LCD_GPIO_Init(void);
void LCD_Writ_Bus(rt_uint8_t dat);
void LCD_WR_DATA8(rt_uint8_t dat);
void LCD_WR_DATA(rt_uint16_t dat);
void LCD_WR_REG(rt_uint8_t dat);
void LCD_Address_Set(rt_uint16_t x1, rt_uint16_t y1, rt_uint16_t x2, rt_uint16_t y2);
void LCD_Init(void);
#endif




