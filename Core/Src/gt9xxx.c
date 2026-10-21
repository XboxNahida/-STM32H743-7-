#include "gt9xxx.h"
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>   /* 添加stdlib.h用于abs()函数 */

/* 手势识别上下文 */
static struct {
    touch_result_t current;
    touch_result_t previous;
    gesture_result_t gesture;
    
    /* 双击检测 */
    uint32_t last_tap_time;
    int32_t last_tap_x;
    int32_t last_tap_y;
    bool tap_pending;
    
    /* 长按检测 */
    uint32_t press_start_time;
    int32_t press_start_x;
    int32_t press_start_y;
    bool long_press_detected;
    
    /* 手势状态 */
    bool gesture_active;
    gesture_type_t active_gesture;
} gesture_ctx = {0};

/* 向GT9xxx寄存器写数据 */
static uint8_t gt9xxx_write_reg(uint16_t reg, uint8_t *buf, uint16_t len)
{
    uint8_t i;
    
    touch_iic_start();
    touch_iic_send_byte(GT9XXX_CMD_WR);
    if(touch_iic_wait_ack())
    {
        touch_iic_stop();
        return 1;
    }
    
    touch_iic_send_byte(reg >> 8);   /* 高8位地址 */
    if(touch_iic_wait_ack())
    {
        touch_iic_stop();
        return 1;
    }
    
    touch_iic_send_byte(reg & 0xFF); /* 低8位地址 */
    if(touch_iic_wait_ack())
    {
        touch_iic_stop();
        return 1;
    }
    
    for(i = 0; i < len; i++)
    {
        touch_iic_send_byte(buf[i]);
        if(touch_iic_wait_ack())
        {
            touch_iic_stop();
            return 1;
        }
    }
    
    touch_iic_stop();
    return 0;
}

/* 从GT9xxx寄存器读数据 */
static uint8_t gt9xxx_read_reg(uint16_t reg, uint8_t *buf, uint16_t len)
{
    uint8_t i;
    
    touch_iic_start();
    touch_iic_send_byte(GT9XXX_CMD_WR);
    if(touch_iic_wait_ack())
    {
        touch_iic_stop();
        return 1;
    }
    
    touch_iic_send_byte(reg >> 8);
    if(touch_iic_wait_ack())
    {
        touch_iic_stop();
        return 1;
    }
    
    touch_iic_send_byte(reg & 0xFF);
    if(touch_iic_wait_ack())
    {
        touch_iic_stop();
        return 1;
    }
    
    touch_iic_start();
    touch_iic_send_byte(GT9XXX_CMD_RD);
    if(touch_iic_wait_ack())
    {
        touch_iic_stop();
        return 1;
    }
    
    for(i = 0; i < len; i++)
    {
        buf[i] = touch_iic_read_byte(i == (len - 1) ? 0 : 1);
    }
    
    touch_iic_stop();
    return 0;
}

uint8_t gt9xxx_probe_readonly(char product_id[5])
{
    if (product_id == NULL)
    {
        return 1U;
    }

    memset(product_id, 0, 5U);
    if (gt9xxx_read_reg(GT9XXX_REG_ID, (uint8_t *)product_id, 4U) != 0U)
    {
        return 1U;
    }
    product_id[4] = '\0';
    return 0U;
}

/* 初始化GT9xxx */
uint8_t gt9xxx_init(void)
{
    uint8_t temp[5];
    uint8_t retry = 3;
    
//    printf("GT9xxx Init Step1: Hardware Reset\r\n");
    
    /* 硬件复位 */
    HAL_Delay(100);          // 电源稳定
    TOUCH_RST_LOW();
    HAL_Delay(20);           // 保持复位至少 10ms，这里给 20ms
    TOUCH_RST_HIGH();
    HAL_Delay(110);          // 释放复位后等待 150ms（大于手册要求的 100ms）
    
    /* 读取产品ID */
    while(retry--)
    {
//        printf("  Attempt %d... \r\n", 3 - retry);
        
        if(gt9xxx_read_reg(GT9XXX_REG_ID, temp, 4) == 0)
        {
            temp[4] = 0;
            
            /* 检查是否是支持的型号 */
            if(strcmp((char*)temp, "911") == 0 || 
               strcmp((char*)temp, "9147") == 0 || 
               strcmp((char*)temp, "1158") == 0 || 
               strcmp((char*)temp, "9271") == 0)
            {
                
                /* 软复位 */
                temp[0] = 0x02;
                gt9xxx_write_reg(GT9XXX_REG_CMD, temp, 1);
                HAL_Delay(10);
                /* 结束复位 */
                temp[0] = 0x00;
                gt9xxx_write_reg(GT9XXX_REG_CMD, temp, 1);
                HAL_Delay(10);
                /* 初始化手势识别上下文 */
                memset(&gesture_ctx, 0, sizeof(gesture_ctx));

                /* Upload config with reduced touch-merge distance.
                 * Factory default is large which makes two fingers need to be
                 * far apart before GT9xxx reports two separate points. */
                {
                    uint8_t cfg[240];
                    uint8_t cfg_len, i, cksum;

                    if (gt9xxx_read_reg(GT9XXX_REG_CONFIG, cfg, sizeof(cfg)) != 0) {

                    } else {
                        /* Determine config length from version byte */
                        if (cfg[0] >= 0x41)
                            cfg_len = 184;  /* GT911 */
                        else
                            cfg_len = 128;  /* older series */


                        /* Touch merge distance at offset 0x1E.
                         * The unit is sensor-row spacing (not mm).
                         * Factory values typically 30-80 keep fingers apart.
                         * Set to 10 so fingers can be very close. */
                        cfg[0x1E] = 5;  /* merge distance: 5 sensor units */

                        /* Recompute checksum (XOR bytes 0..len-3, negate+1) */
                        cksum = 0;
                        for (i = 0; i < cfg_len - 2; i++)
                            cksum += cfg[i];
                        cksum = (uint8_t)((~cksum) + 1);
                        cfg[cfg_len - 2] = cksum;
                        cfg[cfg_len - 1] = 0x01;

                        if (gt9xxx_write_reg(GT9XXX_REG_CONFIG, cfg, cfg_len) != 0) {

                        } else {

                            HAL_Delay(10);

                            /* Soft reset so the new config takes effect */
                            cfg[0] = 0x02;
                            gt9xxx_write_reg(GT9XXX_REG_CMD, cfg, 1);
                            HAL_Delay(10);
                            cfg[0] = 0x00;
                            gt9xxx_write_reg(GT9XXX_REG_CMD, cfg, 1);
                            HAL_Delay(10);
                        }
                    }
                }

                return 0;
            }
            else
            {
                printf("[BOOT] input FAIL item=gt9xxx action=identify id=%s code=1\r\n",
                       temp);
                return 1;
            }
        }
        
        printf("[BOOT] input WARN item=gt9xxx action=identify_retry remaining=%u\r\n",
               (unsigned int)retry);
        // 失败后再次硬件复位，增加等待时间
        TOUCH_RST_LOW();
        HAL_Delay(20);
        TOUCH_RST_HIGH();
        HAL_Delay(200);      // 逐步延长
    }
    
    printf("[BOOT] input FAIL item=gt9xxx action=init code=2\r\n");
    return 1;
}

/* 扫描触摸状态 */
void gt9xxx_scan(touch_result_t *result)
{
    uint8_t status;
    uint8_t buf[4];
    uint8_t i, touch_count;
    uint16_t reg_table[5] = {GT9XXX_REG_P1, GT9XXX_REG_P2, GT9XXX_REG_P3, GT9XXX_REG_P4, GT9XXX_REG_P5};
    
    /* 读取触摸状态 */
    if(gt9xxx_read_reg(GT9XXX_REG_READ_STATUS, &status, 1) != 0)
    {
        result->num = 0;
        return;
    }
    
    /* 清状态标志 */
    if(status & 0x80)
    {
        uint8_t clear = 0;
        gt9xxx_write_reg(GT9XXX_REG_READ_STATUS, &clear, 1);
    }
    
    touch_count = status & 0x0F;
    
    if(touch_count == 0 || touch_count > 5)
    {
        result->num = 0;
        return;
    }
    
    result->num = touch_count;
    
    /* 读取各触摸点坐标 */
    for(i = 0; i < touch_count; i++)
    {
        if(gt9xxx_read_reg(reg_table[i], buf, 4) == 0)
        {
            /* GT9xxx坐标格式（根据屏幕方向调整后）：
						 X高8位 = buf[3], X低8位 = buf[2]
						 Y高8位 = buf[1], Y低8位 = buf[0] */
            
            result->x[i] = ((uint16_t)buf[3] << 8) | buf[2];
						result->y[i] = ((uint16_t)buf[1] << 8) | buf[0];
        }
        else
        {
            result->x[i] = 0;
            result->y[i] = 0;
        }
    }
}

/* 检测手势 */
gesture_result_t gt9xxx_detect_gesture(touch_result_t *touch_data)
{
    gesture_result_t gesture = {0};
    gesture.type = GESTURE_NONE;
    
    /* 保存当前触摸数据 */
    memcpy(&gesture_ctx.previous, &gesture_ctx.current, sizeof(touch_result_t));
    memcpy(&gesture_ctx.current, touch_data, sizeof(touch_result_t));
    
    uint8_t curr_num = gesture_ctx.current.num;
    uint8_t prev_num = gesture_ctx.previous.num;
    
    /* 检测触摸按下 */
    if(curr_num > 0 && prev_num == 0)
    {
        gesture_ctx.press_start_time = HAL_GetTick();
        gesture_ctx.press_start_x = gesture_ctx.current.x[0];
        gesture_ctx.press_start_y = gesture_ctx.current.y[0];
        gesture_ctx.long_press_detected = false;
        gesture_ctx.gesture_active = true;
        
        /* 双击检测 */
        if(gesture_ctx.tap_pending)
        {
            uint32_t time_diff = HAL_GetTick() - gesture_ctx.last_tap_time;
            int32_t dist_x = abs(gesture_ctx.current.x[0] - gesture_ctx.last_tap_x);
            int32_t dist_y = abs(gesture_ctx.current.y[0] - gesture_ctx.last_tap_y);
            
            if(time_diff < 500 && dist_x < 30 && dist_y < 30)
            {
                gesture.type = GESTURE_DOUBLE_CLICK;
                gesture.center_x = gesture_ctx.current.x[0];
                gesture.center_y = gesture_ctx.current.y[0];
                gesture_ctx.tap_pending = false;
                return gesture;
            }
            else
            {
                gesture_ctx.tap_pending = false;
            }
        }
    }
    
    /* 检测触摸释放 */
    if(curr_num == 0 && prev_num > 0)
    {
        if(gesture_ctx.gesture_active && !gesture_ctx.long_press_detected)
        {
            uint32_t press_duration = HAL_GetTick() - gesture_ctx.press_start_time;
            int32_t total_dx = gesture_ctx.previous.x[0] - gesture_ctx.press_start_x;
            int32_t total_dy = gesture_ctx.previous.y[0] - gesture_ctx.press_start_y;
            int32_t total_dist = abs(total_dx) + abs(total_dy);
            
            /* 单击检测（短按且移动距离小） */
            if(press_duration < 500 && total_dist < 20)
            {
                if(!gesture_ctx.tap_pending)
                {
                    gesture.type = GESTURE_SINGLE_CLICK;
                    gesture.center_x = gesture_ctx.previous.x[0];
                    gesture.center_y = gesture_ctx.previous.y[0];
                    
                    /* 记录这次点击用于双击检测 */
                    gesture_ctx.last_tap_time = HAL_GetTick();
                    gesture_ctx.last_tap_x = gesture_ctx.previous.x[0];
                    gesture_ctx.last_tap_y = gesture_ctx.previous.y[0];
                    gesture_ctx.tap_pending = true;
                    
                    gesture_ctx.gesture_active = false;
                    return gesture;
                }
            }
            
            /* 滑动检测 */
						if(total_dist > 30)
						{
								/* raw_dx 对应屏幕上下，raw_dy 对应屏幕左右 */
								if(abs(total_dx) > abs(total_dy))
								{
										/* 主运动在 raw_x 方向 → 屏幕上下滑动 */
										gesture.type = (total_dx > 0) ? GESTURE_SWIPE_DOWN : GESTURE_SWIPE_UP;
								}
								else
								{
										/* 主运动在 raw_y 方向 → 屏幕左右滑动 */
										gesture.type = (total_dy > 0) ? GESTURE_SWIPE_RIGHT : GESTURE_SWIPE_LEFT;
								}
								/* delta_x, delta_y 也要映射到屏幕坐标系 */
								gesture.delta_x = total_dy;   /* 屏幕X变化量 = raw_y变化量 */
								gesture.delta_y = total_dx;   /* 屏幕Y变化量 = raw_x变化量 */
								gesture.center_x = gesture_ctx.press_start_x;
								gesture.center_y = gesture_ctx.press_start_y;
								
								gesture_ctx.gesture_active = false;
								return gesture;
						}
        }
        
        gesture_ctx.gesture_active = false;
    }
    
    /* 双指手势检测 */
    if(curr_num >= 2 && prev_num >= 2 && gesture_ctx.gesture_active)
    {
        /* 计算双指中心点 */
        int32_t curr_center_x = (gesture_ctx.current.x[0] + gesture_ctx.current.x[1]) / 2;
        int32_t curr_center_y = (gesture_ctx.current.y[0] + gesture_ctx.current.y[1]) / 2;
        int32_t prev_center_x = (gesture_ctx.previous.x[0] + gesture_ctx.previous.x[1]) / 2;
        int32_t prev_center_y = (gesture_ctx.previous.y[0] + gesture_ctx.previous.y[1]) / 2;
        
        /* 计算双指距离 */
        float curr_dist = sqrt(pow(gesture_ctx.current.x[1] - gesture_ctx.current.x[0], 2) +
                              pow(gesture_ctx.current.y[1] - gesture_ctx.current.y[0], 2));
        float prev_dist = sqrt(pow(gesture_ctx.previous.x[1] - gesture_ctx.previous.x[0], 2) +
                              pow(gesture_ctx.previous.y[1] - gesture_ctx.previous.y[0], 2));
        
        if(prev_dist > 0)
        {
            gesture.scale = curr_dist / prev_dist;
            gesture.center_x = curr_center_x;
            gesture.center_y = curr_center_y;
            gesture.delta_x = curr_center_x - prev_center_x;
            gesture.delta_y = curr_center_y - prev_center_y;
            
            /* 缩放检测 */
            if(fabs(gesture.scale - 1.0) > 0.1)
            {
                gesture.type = (gesture.scale > 1.0) ? GESTURE_PINCH_OUT : GESTURE_PINCH_IN;
                return gesture;
            }
        }
    }
    
    /* 长按检测 */
    if(curr_num == 1 && gesture_ctx.gesture_active && !gesture_ctx.long_press_detected)
    {
        uint32_t press_duration = HAL_GetTick() - gesture_ctx.press_start_time;
        if(press_duration > 800)  /* 长按800ms */
        {
            gesture_ctx.long_press_detected = true;
            /* 长按手势可以在这里扩展 */
        }
    }
    
    return gesture;
}

/* 获取最后的手势 */
gesture_result_t gt9xxx_get_gesture(void)
{
    return gesture_ctx.gesture;
}

/* 检查是否有活跃手势 */
bool gt9xxx_is_gesture_active(void)
{
    return gesture_ctx.gesture_active;
}

