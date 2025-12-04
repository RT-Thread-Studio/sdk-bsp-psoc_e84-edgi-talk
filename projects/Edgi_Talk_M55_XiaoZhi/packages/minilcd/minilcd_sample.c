#include "lcd_init.h"
#include "lcd.h"
#include "pic.h"

#define BLK GET_PIN(13,7)

int minilcd_test(void)
{
    rt_pin_mode(BLK, PIN_MODE_OUTPUT);
    rt_pin_write(BLK, 0);

    LCD_Init();
    LCD_Fill(0, 0, LCD_W, LCD_H, WHITE);

    LCD_ShowPicture(100, 1, 31, 80, gImage_test);
    LCD_ShowPicture(40, 1, 35, 80, gImage_test1);
    return 0;
}
INIT_APP_EXPORT(minilcd_test);
