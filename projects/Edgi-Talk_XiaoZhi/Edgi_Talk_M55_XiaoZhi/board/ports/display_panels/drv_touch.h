#ifndef __DRV_TOUCH_H__
#define __DRV_TOUCH_H__

#include <rtthread.h>
#include <rtdevice.h>
#include <board.h>


/* IC register */
#define ST_MISC_INFO_SWU_FLAG 0x80
#define ST_MISC_INFO_PROX_FLAG 0x20
#define ST_MISC_INFO_COORD_CHKSUM_FLAG 0x10
enum
{
    FIRMWARE_VERSION = 0,
    STATUS_REG,
    DEVICE_CONTROL_REG,
    TIMEOUT_TO_IDLE_REG,
    X_RESOLUTION_HIGH = 0x05,
    X_RESOLUTION_LOW = 0x06,
    Y_RESOLUTION_HIGH = 0x07,
    Y_RESOLUTION_LOW = 0x08,
    MAX_NUM_TOUCHES = 0x09,
    SENSING_COUNTER_H = 0x0A,
    SENSING_COUNTER_L = 0x0B,
    DEVICE_CONTROL_REG2 = 0x09,
    FIRMWARE_REVISION_3 = 0x0C,
    FIRMWARE_REVISION_2,
    FIRMWARE_REVISION_1,
    FIRMWARE_REVISION_0,
    TOUCH_INFO = 0x10,
    GESTURES = 0x12,
    PROX_RAW_HEADER = 0x5A,
    MISC_INFO = 0xF0,
    MISC_CONTROL = 0xF1,
    SMART_WAKE_UP_REG = 0xF2,
    CHIP_ID = 0xF4,
    XY_CHS = 0xF5,
    CMDIO_CONTROL = 0xF8,
    PAGE_REG = 0xFF,
    CMDIO_PORT = 0x110,
    EX_DIFF_EN = 0x130,
    DATA_OUTPUT_BUFFER = 0x140,
};

#define ST7102_RST_PIN  GET_PIN(17, 3)
#define ST7102_IRQ_PIN  GET_PIN(17, 2)

#define ST7102_ADDR_LEN          2
#define ST7102_REGITER_LEN       2
#define ST7102_MAX_TOUCH         0x0A
#define ST7102_POINT_INFO_NUM   5

#define ST7102_ADDRESS          0x55
#define ST7102_Device_Control    0x02
// #define ST7102_ADDRESS_LOW       0x14
#define ST7102_MAX_X_Coord_High   0x05
#define ST7102_MAX_X_Coord_Low    0x06
#define ST7102_MAX_Y_Coord_High   0x07
#define ST7102_MAX_Y_Coord_Low    0x08
#define ST7102_MAX_Touches        0x09
#define ST7102_READ_STATUS        0x10 //0 = No touch deceted

#define ST7102_COMMAND_REG       0x8040
#define ST7102_CONFIG_REG        0x8047

#define ST7102_PRODUCT_ID        0x8140
#define ST7102_VENDOR_ID         0x814A

/* Touch point info */
/* Point_1 = Point_0 + ST7102_Ponint2Ponint_Offset * (1 - 0) */
#define ST7102_Ponint2Ponint_Offset         0x07
#define ST7102_POINT0_X_COORD_High          0x14
#define ST7102_POINT0_X_COORD_Low           0x15
#define ST7102_POINT0_Y_COORD_High          0x16
#define ST7102_POINT0_Y_COORD_Low           0x17
#define ST7102_POINT0_Touch_Area            0x18
#define ST7102_POINT0_REG_Touch_Intensity   0x19
#define ST7102_Read_Start_Position          0x10


#define ST7102_CHECK_SUM         0x80FF

int rt_hw_ST7102_init(const char *name, struct rt_touch_config *cfg);

#endif
