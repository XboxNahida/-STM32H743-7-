/**
 * @file lv_port_indev_template.c
 *
 */

/*Copy this file as "lv_port_indev.c" and set this value to "1" to enable content*/
#if 1

/*********************
 *      INCLUDES
 *********************/
#include "lv_port_indev.h"
#include "gt9xxx.h"      /* 包含触摸驱动头文件 */
#include "touch_coordinate_map.h"
#include <stdio.h>      /* 添加stdio用于printf */
#include "tim.h"         /* 背光PWM定时器 */
#include "../../lvgl_GUI/generated/gui_guider.h" /* UI切换 */

#if LV_USE_IME_PINYIN
extern lv_obj_t *g_kb_ime;
#endif

/*********************
 *      DEFINES
 *********************/
#define DEBUG_TOUCH  0  /* 调试开关，1=开启调试输出，0=关闭 */
#define DEBUG_TOUCH_GESTURE  0  /* 手势调试开关，1=开启调试输出，0=关闭 */
#define MAX_TOUCH_POINTS 5  /* 最多支持的触摸点数 */

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/

static void touchpad_init(void);
static void touchpad_read(lv_indev_t * indev, lv_indev_data_t * data);
static bool touchpad_is_pressed(void);
static void touchpad_get_xy(int32_t * x, int32_t * y);
static void screen_off_hide_input_overlays(void);

/* 多点触摸回调函数 */
static void touchpad0_read(lv_indev_t * indev, lv_indev_data_t * data);
static void touchpad1_read(lv_indev_t * indev, lv_indev_data_t * data);
static void touchpad2_read(lv_indev_t * indev, lv_indev_data_t * data);
static void touchpad3_read(lv_indev_t * indev, lv_indev_data_t * data);
static void touchpad4_read(lv_indev_t * indev, lv_indev_data_t * data);

//static void mouse_init(void);
//static void mouse_read(lv_indev_t * indev, lv_indev_data_t * data);
//static bool mouse_is_pressed(void);
//static void mouse_get_xy(int32_t * x, int32_t * y);

//static void keypad_init(void);
//static void keypad_read(lv_indev_t * indev, lv_indev_data_t * data);
//static uint32_t keypad_get_key(void);

static void encoder_init(void);
static void encoder_read(lv_indev_t * indev, lv_indev_data_t * data);
static void encoder_handler(void);

static void button_init(void);
static void button_read(lv_indev_t * indev, lv_indev_data_t * data);
static int8_t button_get_pressed_id(void);
static bool button_is_pressed(uint8_t id);

/**********************
 *  STATIC VARIABLES
 **********************/
lv_indev_t * indev_touchpad[MAX_TOUCH_POINTS];  /* 多点触摸输入设备数组 */
lv_indev_t * indev_mouse;
lv_indev_t * indev_keypad;
lv_indev_t * indev_encoder;
lv_indev_t * indev_button;

static int32_t encoder_diff;
static lv_indev_state_t encoder_state;
static uint32_t debug_count = 0;  /* 调试计数器 */

/* 共享的触摸数据 */
touch_result_t shared_touch_data;
static int32_t last_x[MAX_TOUCH_POINTS] = {0};
static int32_t last_y[MAX_TOUCH_POINTS] = {0};

/* 息屏相关: 0=正常 1=低亮 2=熄灭 */
static uint32_t last_touch_tick  = 0;       /* 最后一次触摸的时间戳 */
static uint8_t  screen_dim_state = 0;       /* 息屏状态 */
static uint32_t dim_start_tick   = 0;       /* 进入低亮时刻 */
static int32_t  saved_backlight  = 500;     /* 正常亮度PWM值(仅状态0时更新) */
static uint8_t g_lock_screen_request;
uint32_t g_screen_off_timeout_ms = 60000;   /* 息屏超时(ms)，可通过串口指令修改 */

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/
 
/* 息屏检测定时器回调: 超时 → 5秒低亮 → 熄灭 (类似手机) */
static void screen_off_timer_cb(lv_timer_t * timer)
{
    (void)timer;

    /* 非桌面/锁屏页面：持续更新时间戳，禁止息屏 */
    lv_obj_t *act_scr = lv_scr_act();
    if (act_scr != guider_ui.DeskTop && act_scr != guider_ui.lock_screen) {
        last_touch_tick = HAL_GetTick();
        return;
    }

    uint32_t now = HAL_GetTick();
    uint32_t idle = now - last_touch_tick;

    switch (screen_dim_state) {
    case 0: /* 正常亮度 → 超时则进入低亮 */
        if (idle > g_screen_off_timeout_ms) {
            saved_backlight = __HAL_TIM_GET_COMPARE(&htim3, TIM_CHANNEL_2);
            __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_2, 50); /* 低亮 ~5% */
            screen_dim_state = 1;
            dim_start_tick = now;
        }
        break;
    case 1: /* 低亮中 → 5秒后完全熄灭 */
        if (now - dim_start_tick > 5000) {
            __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_2, 0);
            screen_dim_state = 2;
            g_lock_screen_request = 1U;
        }
        break;
    case 2: /* 已熄灭，等待触摸唤醒 */
    default:
        break;
    }
}

static void screen_off_hide_input_overlays(void)
{
    lv_obj_t *keyboard = guider_ui.g_kb_top_layer;

#if LV_USE_IME_PINYIN
    if ((g_kb_ime != NULL) && lv_obj_is_valid(g_kb_ime)) {
        lv_obj_t *candidate = lv_ime_pinyin_get_cand_panel(g_kb_ime);
        lv_ime_pinyin_set_textarea(g_kb_ime, NULL);
        lv_ime_pinyin_reset(g_kb_ime);
        if ((candidate != NULL) && lv_obj_is_valid(candidate)) {
            lv_obj_add_flag(candidate, LV_OBJ_FLAG_HIDDEN);
        }
    }
#endif

    if ((keyboard != NULL) && lv_obj_is_valid(keyboard)) {
        lv_keyboard_set_textarea(keyboard, NULL);
        lv_obj_add_flag(keyboard, LV_OBJ_FLAG_HIDDEN);
    }
}

void lv_port_indev_process_power_request(void)
{
    lv_obj_t *active_screen;
    lv_display_t *display;

    if (g_lock_screen_request == 0U) return;

    display = lv_display_get_default();
    if ((display != NULL) && (lv_display_get_screen_loading(display) != NULL)) {
        return;
    }

    active_screen = lv_screen_active();
    if (active_screen == guider_ui.lock_screen) {
        g_lock_screen_request = 0U;
        return;
    }
    if (active_screen != guider_ui.DeskTop) {
        return;
    }

    g_lock_screen_request = 0U;
    screen_off_hide_input_overlays();
    lv_indev_reset(NULL, NULL);
    ui_load_scr_animation(&guider_ui, &guider_ui.lock_screen,
        guider_ui.lock_screen_del, &guider_ui.DeskTop_del,
        setup_scr_lock_screen, LV_SCR_LOAD_ANIM_NONE, 50U, 50U, true, true);
}

void lv_port_indev_init(void)
{
    /* 初始化触摸屏 */
    touchpad_init();

    /* 创建息屏检测定时器 (每秒检查一次) */
    lv_timer_create(screen_off_timer_cb, 1000, NULL);

    /* 创建主触摸设备（兼容单点触摸） */
    indev_touchpad[0] = lv_indev_create();
    lv_indev_set_type(indev_touchpad[0], LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(indev_touchpad[0], touchpad_read);
    
    /* 创建额外的触摸设备用于多点触摸 */
    indev_touchpad[1] = lv_indev_create();
    lv_indev_set_type(indev_touchpad[1], LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(indev_touchpad[1], touchpad1_read);
    
    indev_touchpad[2] = lv_indev_create();
    lv_indev_set_type(indev_touchpad[2], LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(indev_touchpad[2], touchpad2_read);
    
    indev_touchpad[3] = lv_indev_create();
    lv_indev_set_type(indev_touchpad[3], LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(indev_touchpad[3], touchpad3_read);
    
    indev_touchpad[4] = lv_indev_create();
    lv_indev_set_type(indev_touchpad[4], LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(indev_touchpad[4], touchpad4_read);
    
#if DEBUG_TOUCH_GESTURE
    printf("LVGL: Multi-touch (up to %d points) initialized\r\n", MAX_TOUCH_POINTS);
#endif
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

/*------------------
 * Touchpad
 * -----------------*/

/*Initialize your touchpad*/
static void touchpad_init(void)
{
    /*Your code comes here*/
		/* 触摸屏已经在 gt9xxx_init() 中初始化，这里不需要做什么 */
}

/* 坐标映射函数 */
static void map_coordinates(int32_t raw_x, int32_t raw_y, int32_t *mapped_x, int32_t *mapped_y)
{
    (void)touch_coordinate_map(raw_x, raw_y, mapped_x, mapped_y);
}

void lv_port_indev_get_touch_snapshot(lv_port_touch_snapshot_t *snapshot)
{
    uint8_t index;

    if (snapshot == NULL)
    {
        return;
    }
    snapshot->count = (shared_touch_data.num <= MAX_TOUCH_POINTS) ?
                      shared_touch_data.num : MAX_TOUCH_POINTS;
    for (index = 0U; index < snapshot->count; index++)
    {
        int32_t mapped_x;
        int32_t mapped_y;

        map_coordinates(shared_touch_data.x[index],
                        shared_touch_data.y[index],
                        &mapped_x, &mapped_y);
        snapshot->x[index] = (int16_t)mapped_x;
        snapshot->y[index] = (int16_t)mapped_y;
    }
    for (; index < MAX_TOUCH_POINTS; index++)
    {
        snapshot->x[index] = 0;
        snapshot->y[index] = 0;
    }
}

/* 通用的触摸点读取函数 */
static void read_touch_point(lv_indev_data_t * data, uint8_t point_index)
{
    if(point_index < shared_touch_data.num)
    {
        /* 有触摸点 */
        int32_t raw_x = shared_touch_data.x[point_index];
        int32_t raw_y = shared_touch_data.y[point_index];
        int32_t mapped_x, mapped_y;
        
        map_coordinates(raw_x, raw_y, &mapped_x, &mapped_y);
        
        /* 第一次检测到触摸时打印 */
        if(data->state != LV_INDEV_STATE_PRESSED)
        {
#if DEBUG_TOUCH
            printf("TOUCH%d: Pressed at RAW(%d,%d) -> MAPPED(%d,%d)\r\n", 
                   point_index, raw_x, raw_y, mapped_x, mapped_y);
#endif
        }
        
        last_x[point_index] = mapped_x;
        last_y[point_index] = mapped_y;
        data->state = LV_INDEV_STATE_PRESSED;
    }
    else
    {
        /* 触摸点释放 */
        if(data->state == LV_INDEV_STATE_PRESSED)
        {
#if DEBUG_TOUCH
            printf("TOUCH%d: Released at (%d,%d)\r\n", 
                   point_index, last_x[point_index], last_y[point_index]);
#endif
        }
        data->state = LV_INDEV_STATE_RELEASED;
    }
    
    data->point.x = last_x[point_index];
    data->point.y = last_y[point_index];
}

/*Will be called by the library to read the touchpad*/
static void touchpad_read(lv_indev_t * indev_drv, lv_indev_data_t * data)
{
    static uint8_t first_run = 1;
    
    /* 扫描触摸 */
    gt9xxx_scan(&shared_touch_data);
    
    /* 息屏唤醒逻辑 */
    if (shared_touch_data.num > 0) {
        last_touch_tick = HAL_GetTick();
        switch (screen_dim_state) {
        case 0: /* 正常使用中: 仅更新时间戳 */ break;
        case 1: /* 低亮中: 恢复亮度, 不切锁屏 */
            __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_2, saved_backlight);
            screen_dim_state = 0;
            break;
        case 2: /* 已熄灭: 只恢复亮度，锁屏已在主循环安全切换 */
            __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_2, saved_backlight);
            screen_dim_state = 0;
            break;
        }
    }
    
    /* 检测手势 */
    gesture_result_t gesture = gt9xxx_detect_gesture(&shared_touch_data);
    
    /* 每秒打印一次调试信息 */
    debug_count++;
    if(debug_count >= 200)
    {
        debug_count = 0;
#if DEBUG_TOUCH
        printf("TOUCH DEBUG: num=%d", shared_touch_data.num);
        for(int i = 0; i < shared_touch_data.num; i++)
        {
            printf(", P%d=(%d,%d)", i, shared_touch_data.x[i], shared_touch_data.y[i]);
        }
        printf("\r\n");
#endif
    }
    
    /* 手势调试输出 */
    if(gesture.type != GESTURE_NONE)
    {
#if DEBUG_TOUCH_GESTURE
        switch(gesture.type)
        {
            case GESTURE_SINGLE_CLICK:
                printf("GESTURE: Single Click at (%d,%d)\r\n", gesture.center_x, gesture.center_y);
                break;
            case GESTURE_DOUBLE_CLICK:
                printf("GESTURE: Double Click at (%d,%d)\r\n", gesture.center_x, gesture.center_y);
                break;
            case GESTURE_SWIPE_LEFT:
                printf("GESTURE: Swipe Left (dx=%d)\r\n", gesture.delta_x);
                break;
            case GESTURE_SWIPE_RIGHT:
                printf("GESTURE: Swipe Right (dx=%d)\r\n", gesture.delta_x);
                break;
            case GESTURE_SWIPE_UP:
                printf("GESTURE: Swipe Up (dy=%d)\r\n", gesture.delta_y);
                break;
            case GESTURE_SWIPE_DOWN:
                printf("GESTURE: Swipe Down (dy=%d)\r\n", gesture.delta_y);
                break;
            case GESTURE_PINCH_IN:
                printf("GESTURE: Pinch In (scale=%.2f)\r\n", gesture.scale);
                break;
            case GESTURE_PINCH_OUT:
                printf("GESTURE: Pinch Out (scale=%.2f)\r\n", gesture.scale);
                break;
            default:
                break;
        }
#endif
        
        /* 根据手势类型执行LVGL操作 */
        switch(gesture.type)
        {
            case GESTURE_SWIPE_LEFT:
                /* 页面左滑 - 可以通过组来切换焦点 */
                // {
                //     lv_group_t * group = lv_group_get_default();
                //     if(group) lv_group_focus_next(group);
                // }
                break;
                
            case GESTURE_SWIPE_RIGHT:
                /* 页面右滑 */
                // {
                //     lv_group_t * group = lv_group_get_default();
                //     if(group) lv_group_focus_prev(group);
                // }
                break;
                
            case GESTURE_PINCH_IN:
                /* 缩小 - 获取当前聚焦的对象并缩放 */
                // {
                //     lv_obj_t * obj = lv_indev_get_obj_act();
                //     if(obj) {
                //         lv_coord_t zoom = lv_obj_get_style_transform_zoom(obj, 0);
                //         zoom = zoom > 64 ? zoom - 32 : zoom;
                //         lv_obj_set_style_transform_zoom(obj, zoom, 0);
                //     }
                // }
                break;
                
            case GESTURE_PINCH_OUT:
                /* 放大 */
                // {
                //     lv_obj_t * obj = lv_indev_get_obj_act();
                //     if(obj) {
                //         lv_coord_t zoom = lv_obj_get_style_transform_zoom(obj, 0);
                //         zoom = zoom < 512 ? zoom + 32 : zoom;
                //         lv_obj_set_style_transform_zoom(obj, zoom, 0);
                //     }
                // }
                break;
                
            case GESTURE_DOUBLE_CLICK:
                /* 双击重置缩放 */
                // {
                //     lv_obj_t * obj = lv_indev_get_obj_act();
                //     if(obj) {
                //         lv_obj_set_style_transform_zoom(obj, LV_IMG_ZOOM_NONE, 0);
                //     }
                // }
                break;
                
            default:
                break;
        }
    }
    
    /* 读取第一个触摸点（主触摸设备） */
    read_touch_point(data, 0);
    
    if(first_run)
    {
#if DEBUG_TOUCH_GESTURE
        printf("LVGL: Multi-touch ready. First touch state=%d, point=(%d,%d)\r\n", 
               data->state, data->point.x, data->point.y);
#endif
        first_run = 0;
    }
}

/* 第二个触摸点回调 */
static void touchpad1_read(lv_indev_t * indev_drv, lv_indev_data_t * data)
{
    read_touch_point(data, 1);
}

/* 第三个触摸点回调 */
static void touchpad2_read(lv_indev_t * indev_drv, lv_indev_data_t * data)
{
    read_touch_point(data, 2);
}

/* 第四个触摸点回调 */
static void touchpad3_read(lv_indev_t * indev_drv, lv_indev_data_t * data)
{
    read_touch_point(data, 3);
}

/* 第五个触摸点回调 */
static void touchpad4_read(lv_indev_t * indev_drv, lv_indev_data_t * data)
{
    read_touch_point(data, 4);
}

/*Return true is the touchpad is pressed*/
static bool touchpad_is_pressed(void)
{
    /*Your code comes here*/

    return false;
}

/*Get the x and y coordinates if the touchpad is pressed*/
static void touchpad_get_xy(int32_t * x, int32_t * y)
{
    /*Your code comes here*/

    (*x) = 0;
    (*y) = 0;
}

///*------------------
// * Mouse
// * -----------------*/

///*Initialize your mouse*/
//static void mouse_init(void)
//{
//    /*Your code comes here*/
//}

///*Will be called by the library to read the mouse*/
//static void mouse_read(lv_indev_t * indev_drv, lv_indev_data_t * data)
//{
//    /*Get the current x and y coordinates*/
//    mouse_get_xy(&data->point.x, &data->point.y);

//    /*Get whether the mouse button is pressed or released*/
//    if(mouse_is_pressed()) {
//        data->state = LV_INDEV_STATE_PRESSED;
//    }
//    else {
//        data->state = LV_INDEV_STATE_RELEASED;
//    }
//}

///*Return true is the mouse button is pressed*/
//static bool mouse_is_pressed(void)
//{
//    /*Your code comes here*/

//    return false;
//}

///*Get the x and y coordinates if the mouse is pressed*/
//static void mouse_get_xy(int32_t * x, int32_t * y)
//{
//    /*Your code comes here*/

//    (*x) = 0;
//    (*y) = 0;
//}

///*------------------
// * Keypad
// * -----------------*/

///*Initialize your keypad*/
//static void keypad_init(void)
//{
//    /*Your code comes here*/
//}

///*Will be called by the library to read the mouse*/
//static void keypad_read(lv_indev_t * indev_drv, lv_indev_data_t * data)
//{
//    static uint32_t last_key = 0;

//    /*Get the current x and y coordinates*/
//    mouse_get_xy(&data->point.x, &data->point.y);

//    /*Get whether the a key is pressed and save the pressed key*/
//    uint32_t act_key = keypad_get_key();
//    if(act_key != 0) {
//        data->state = LV_INDEV_STATE_PRESSED;

//        /*Translate the keys to LVGL control characters according to your key definitions*/
//        switch(act_key) {
//            case 1:
//                act_key = LV_KEY_NEXT;
//                break;
//            case 2:
//                act_key = LV_KEY_PREV;
//                break;
//            case 3:
//                act_key = LV_KEY_LEFT;
//                break;
//            case 4:
//                act_key = LV_KEY_RIGHT;
//                break;
//            case 5:
//                act_key = LV_KEY_ENTER;
//                break;
//        }

//        last_key = act_key;
//    }
//    else {
//        data->state = LV_INDEV_STATE_RELEASED;
//    }

//    data->key = last_key;
//}

///*Get the currently being pressed key.  0 if no key is pressed*/
//static uint32_t keypad_get_key(void)
//{
//    /*Your code comes here*/

//    return 0;
//}

/*------------------
 * Encoder
 * -----------------*/

/*Initialize your encoder*/
static void encoder_init(void)
{
    /*Your code comes here*/
}

/*Will be called by the library to read the encoder*/
static void encoder_read(lv_indev_t * indev_drv, lv_indev_data_t * data)
{

    data->enc_diff = encoder_diff;
    data->state = encoder_state;
}

/*Call this function in an interrupt to process encoder events (turn, press)*/
static void encoder_handler(void)
{
    /*Your code comes here*/

    encoder_diff += 0;
    encoder_state = LV_INDEV_STATE_RELEASED;
}

/*------------------
 * Button
 * -----------------*/

/*Initialize your buttons*/
static void button_init(void)
{
    /*Your code comes here*/
}

/*Will be called by the library to read the button*/
static void button_read(lv_indev_t * indev_drv, lv_indev_data_t * data)
{

    static uint8_t last_btn = 0;

    /*Get the pressed button's ID*/
    int8_t btn_act = button_get_pressed_id();

    if(btn_act >= 0) {
        data->state = LV_INDEV_STATE_PRESSED;
        last_btn = btn_act;
    }
    else {
        data->state = LV_INDEV_STATE_RELEASED;
    }

    /*Save the last pressed button's ID*/
    data->btn_id = last_btn;
}

/*Get ID  (0, 1, 2 ..) of the pressed button*/
static int8_t button_get_pressed_id(void)
{
    uint8_t i;

    /*Check to buttons see which is being pressed (assume there are 2 buttons)*/
    for(i = 0; i < 2; i++) {
        /*Return the pressed button's ID*/
        if(button_is_pressed(i)) {
            return i;
        }
    }

    /*No button pressed*/
    return -1;
}

/*Test if `id` button is pressed or not*/
static bool button_is_pressed(uint8_t id)
{

    /*Your code comes here*/

    return false;
}

#else /*Enable this file at the top*/

/*This dummy typedef exists purely to silence -Wpedantic.*/
typedef int keep_pedantic_happy;
#endif

