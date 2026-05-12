#ifndef __MATRIX_KEYBOARD_H__
#define __MATRIX_KEYBOARD_H__

#include "zf_common_headfile.h"

//=================================================== 硬件定义 ===================================================
// Row 引脚定义 (输出，从上到下)
#define MATRIX_ROW_0    B7                     // p17 → Row 0
#define MATRIX_ROW_1    E0                     // p16 → Row 1
#define MATRIX_ROW_2    E1                     // p15 → Row 2
#define MATRIX_ROW_3    E2                     // p14 → Row 3

// Col 引脚定义 (输入，从左到右)
#define MATRIX_COL_0    E3                     // p13 → Col 0
#define MATRIX_COL_1    E4                     // p12 → Col 1
#define MATRIX_COL_2    E5                     // p11 → Col 2
#define MATRIX_COL_3    E6                     // p10 → Col 3

//=================================================== 参数配置 ===================================================
#define MATRIX_ROW_NUM              (4)
#define MATRIX_COL_NUM              (4)
#define MATRIX_KEY_NUM              (MATRIX_ROW_NUM * MATRIX_COL_NUM)   // 16

#define MATRIX_DEBOUNCE_TIME        (10)        // 消抖时间 (ms)
#define MATRIX_LONG_PRESS_TIME      (1000)      // 长按判定时间 (ms)
#define MATRIX_SCAN_PERIOD          (10)        // 默认扫描周期 (ms)

//=================================================== 状态机状态枚举 ===================================================
typedef enum
{
    MATRIX_IDLE = 0,                           // 空闲状态，无按键按下
    MATRIX_DEBOUNCE,                            // 消抖中
    MATRIX_PRESSED,                             // 按键确认按下
    MATRIX_LONG_PRESS,                          // 长按触发
    MATRIX_RELEASE                              // 按键释放
} matrix_key_state_enum;

//=================================================== 按键事件枚举 ===================================================
typedef enum
{
    MATRIX_EVENT_NONE = 0,
    MATRIX_EVENT_SHORT_PRESS,                   // 短按事件
    MATRIX_EVENT_LONG_PRESS,                     // 长按事件
    MATRIX_EVENT_RELEASE                        // 释放事件
} matrix_key_event_enum;

//=================================================== 按键信息结构体 ===================================================
typedef struct
{
    uint8 row;                                  // 按键所在行 (0-3)
    uint8 col;                                  // 按键所在列 (0-3)
    uint8 index;                                // 按键编号 (1-16)
    matrix_key_state_enum state;                // 当前状态
    uint32 press_time;                          // 按下持续时间 (ms)
    matrix_key_event_enum event;                // 触发的事件
} matrix_key_info_struct;

//=================================================== 事件回调函数类型 ===================================================
typedef void (*matrix_key_callback)(uint8 key_index, matrix_key_event_enum event, uint8 key_value);

//=================================================== API 函数 ===================================================
// 初始化矩阵键盘
void matrix_keyboard_init(uint32 scan_period_ms);

// 设置按键事件回调
void matrix_keyboard_set_callback(matrix_key_callback callback);

// 使能/禁止矩阵键盘
void matrix_keyboard_set_enabled(uint8_t enabled);
uint8_t matrix_keyboard_is_enabled(void);

// 扫描函数（需在主循环或定时器中调用）
void matrix_keyboard_scanner(void);

// 获取当前按下的按键值
// 返回值: 0 表示无按键, 1-16 表示短按, 'A'-'P' 表示长按
uint8 matrix_keyboard_get_value(void);

// 清除按键事件
void matrix_keyboard_clear_event(void);

// 在OLED上显示按键功能
void matrix_keyboard_display_info(uint8 key_value);

// 获取当前按键信息
matrix_key_info_struct matrix_keyboard_get_info(void);

#endif