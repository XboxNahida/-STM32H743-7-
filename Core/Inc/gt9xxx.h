#ifndef __GT9XXX_H
#define __GT9XXX_H

#include "main.h"
#include "touch_i2c.h"
#include <stdbool.h>   /* 添加bool类型支持 */

/* GT9xxx I2C地址 */
#define GT9XXX_ADDR             0x14        /* 7位地址 */

/* I2C读写命令字节 */
#define GT9XXX_CMD_WR           (GT9XXX_ADDR << 1)      /* 0x28 */
#define GT9XXX_CMD_RD           ((GT9XXX_ADDR << 1) | 1) /* 0x29 */

/* GT9xxx寄存器定义 */
#define GT9XXX_REG_CMD          0x8040      /* 命令寄存器 */
#define GT9XXX_REG_ESD_CHECK    0x8041      /* ESD检查 */
#define GT9XXX_REG_CONFIG       0x8047      /* 配置起始地址 */
#define GT9XXX_REG_ID           0x8140      /* 产品ID */
#define GT9XXX_REG_READ_STATUS  0x814E      /* 触摸状态 */
#define GT9XXX_REG_P1           0x8150      /* 触摸点1 */
#define GT9XXX_REG_P2           0x8158      /* 触摸点2 */
#define GT9XXX_REG_P3           0x8160      /* 触摸点3 */
#define GT9XXX_REG_P4           0x8168      /* 触摸点4 */
#define GT9XXX_REG_P5           0x8170      /* 触摸点5 */

/* 触摸数据结果结构体 */
typedef struct {
    uint8_t  num;       /* 当前触摸点数 */
    uint16_t x[5];      /* X坐标 */
    uint16_t y[5];      /* Y坐标 */
} touch_result_t;

/* 手势类型定义 */
typedef enum {
    GESTURE_NONE = 0,
    GESTURE_SINGLE_CLICK,
    GESTURE_DOUBLE_CLICK,
    GESTURE_SWIPE_LEFT,
    GESTURE_SWIPE_RIGHT,
    GESTURE_SWIPE_UP,
    GESTURE_SWIPE_DOWN,
    GESTURE_PINCH_IN,
    GESTURE_PINCH_OUT,
    GESTURE_ROTATE
} gesture_type_t;

/* 手势结果结构体 */
typedef struct {
    gesture_type_t type;
    float scale;           /* 缩放比例 (1.0 = 无缩放) */
    int16_t delta_x;       /* X轴移动距离 */
    int16_t delta_y;       /* Y轴移动距离 */
    int32_t center_x;      /* 手势中心点X */
    int32_t center_y;      /* 手势中心点Y */
    float rotation;        /* 旋转角度 */
} gesture_result_t;

/* 函数声明 */
uint8_t gt9xxx_probe_readonly(char product_id[5]);
uint8_t gt9xxx_init(void);
void gt9xxx_scan(touch_result_t *result);
gesture_result_t gt9xxx_detect_gesture(touch_result_t *touch_data);
gesture_result_t gt9xxx_get_gesture(void);
bool gt9xxx_is_gesture_active(void);

#endif

