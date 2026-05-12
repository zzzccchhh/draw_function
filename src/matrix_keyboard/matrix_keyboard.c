#include "matrix_keyboard.h"
#include <stdio.h>

//=================================================== 宏定义 ===================================================
#define MATRIX_KEY_NONE   0xFF

//=================================================== 静态常量 ===================================================
// Row 引脚数组
static const gpio_pin_enum matrix_row_pin[MATRIX_ROW_NUM] = {
    MATRIX_ROW_0, MATRIX_ROW_1, MATRIX_ROW_2, MATRIX_ROW_3
};

// Col 引脚数组
static const gpio_pin_enum matrix_col_pin[MATRIX_COL_NUM] = {
    MATRIX_COL_0, MATRIX_COL_1, MATRIX_COL_2, MATRIX_COL_3
};

// 按键值映射表 (短按输出 1-16)
static const uint8 short_press_map[MATRIX_ROW_NUM][MATRIX_COL_NUM] = {
    {1, 2, 3,  4},
    {5, 6, 7,  8},
    {9, 10, 11, 12},
    {13, 14, 15, 16}
};

//=================================================== 静态变量 ===================================================
static uint32 scan_period = MATRIX_SCAN_PERIOD;
static matrix_key_state_enum current_state = MATRIX_IDLE;

static uint8 current_row = 0;
static uint8 current_col = 0;
static uint8 current_key_index = 0;
static uint32 press_time_counter = 0;

static uint8 key_value = 0;
static uint8 long_press_triggered = 0;
static uint8 debounce_count = 0;

static matrix_key_callback user_callback = NULL;
static uint8_t keyboard_enabled = 1;

static matrix_key_info_struct key_info = {0};

//=================================================== 内部函数 ===================================================

//-------------------------------------------------------------------------------------------------------------------
// 函数名称     读取所有列的状态，返回列掩码
//-------------------------------------------------------------------------------------------------------------------
static uint8 matrix_read_cols(void)
{
    uint8 col_mask = 0;
    uint8 i;
    for(i = 0; i < MATRIX_COL_NUM; i++)
    {
        if(gpio_get_level(matrix_col_pin[i]) == GPIO_LOW)
        {
            col_mask |= (1 << i);
        }
    }
    return col_mask;
}

//-------------------------------------------------------------------------------------------------------------------
// 函数名称     扫描单行，返回按下的列掩码
//-------------------------------------------------------------------------------------------------------------------
static uint8 matrix_scan_row(uint8 row)
{
    uint8 i;

    // 先设置所有行为高电平
    for(i = 0; i < MATRIX_ROW_NUM; i++)
    {
        gpio_set_level(matrix_row_pin[i], GPIO_HIGH);
    }

    // 再将当前行设为低电平
    gpio_set_level(matrix_row_pin[row], GPIO_LOW);

    // 等待信号稳定
    system_delay_us(50);

    // 读取列状态
    return matrix_read_cols();
}

//-------------------------------------------------------------------------------------------------------------------
// 函数名称     扫描整个矩阵，返回按下的键位置
// 返回值: MATRIX_KEY_NONE 表示无按键按下，否则返回 (row << 4) | col
//-------------------------------------------------------------------------------------------------------------------
static uint8 matrix_scan_all(void)
{
    uint8 row, col;
    uint8 col_mask;

    for(row = 0; row < MATRIX_ROW_NUM; row++)
    {
        col_mask = matrix_scan_row(row);
        if(col_mask != 0)
        {
            // 找到按下的键，只取最低位为1的列
            for(col = 0; col < MATRIX_COL_NUM; col++)
            {
                if(col_mask & (1 << col))
                {
                    return (row << 4) | col;
                }
            }
        }
    }

    return MATRIX_KEY_NONE;
}

//-------------------------------------------------------------------------------------------------------------------
// 函数名称     将 row/col 转换为按键值
//-------------------------------------------------------------------------------------------------------------------
static uint8 matrix_get_key_value(uint8 row, uint8 col, uint8 is_long_press)
{
    uint8 index = short_press_map[row][col];

    if(is_long_press)
    {
        return 'A' + (index >> 2);  // 4->'A', 8->'B', 12->'C', 16->'D'
    }

    return index;
}

//=================================================== 状态机实现 ===================================================

void matrix_keyboard_scanner(void)
{
    uint8 key_position;

    if(!keyboard_enabled) return;

    switch(current_state)
    {
        //-----------------------------------
        case MATRIX_IDLE:
        //-----------------------------------
            key_position = matrix_scan_all();
            if(key_position != MATRIX_KEY_NONE)
            {
                current_row = key_position >> 4;
                current_col = key_position & 0x0F;
                debounce_count = 1;
                current_state = MATRIX_DEBOUNCE;
            }
            break;

        //-----------------------------------
        case MATRIX_DEBOUNCE:
        //-----------------------------------
            key_position = matrix_scan_all();
            if(key_position != MATRIX_KEY_NONE &&
               (key_position >> 4) == current_row &&
               (key_position & 0x0F) == current_col)
            {
                debounce_count++;
                if(debounce_count >= (MATRIX_DEBOUNCE_TIME / scan_period))
                {
                    current_key_index = short_press_map[current_row][current_col];
                    press_time_counter = 0;
                    long_press_triggered = 0;
                    key_value = 0;
                    current_state = MATRIX_PRESSED;

                    if(user_callback)
                    {
                        user_callback(current_key_index, MATRIX_EVENT_SHORT_PRESS, current_key_index);
                    }
                }
            }
            else
            {
                current_state = MATRIX_IDLE;
                debounce_count = 0;
            }
            break;

        //-----------------------------------
        case MATRIX_PRESSED:
        //-----------------------------------
            key_position = matrix_scan_all();
            if(key_position != MATRIX_KEY_NONE &&
               (key_position >> 4) == current_row &&
               (key_position & 0x0F) == current_col)
            {
                press_time_counter++;

                if(!long_press_triggered &&
                   press_time_counter >= (MATRIX_LONG_PRESS_TIME / scan_period))
                {
                    long_press_triggered = 1;
                    key_value = matrix_get_key_value(current_row, current_col, 1);
                    current_state = MATRIX_LONG_PRESS;

                    if(user_callback)
                    {
                        user_callback(current_key_index, MATRIX_EVENT_LONG_PRESS, key_value);
                    }
                }
            }
            else
            {
                if(!long_press_triggered)
                {
                    key_value = current_key_index;
                }

                if(user_callback)
                {
                    user_callback(current_key_index, MATRIX_EVENT_RELEASE, key_value);
                }

                current_state = MATRIX_RELEASE;
            }
            break;

        //-----------------------------------
        case MATRIX_LONG_PRESS:
        //-----------------------------------
            key_position = matrix_scan_all();
            if(key_position != MATRIX_KEY_NONE &&
               (key_position >> 4) == current_row &&
               (key_position & 0x0F) == current_col)
            {
                // 保持长按状态
            }
            else
            {
                key_value = 0;
                long_press_triggered = 0;
                current_state = MATRIX_RELEASE;
            }
            break;

        //-----------------------------------
        case MATRIX_RELEASE:
        //-----------------------------------
            key_position = matrix_scan_all();
            if(key_position == MATRIX_KEY_NONE)
            {
                current_state = MATRIX_IDLE;
                key_value = 0;
                long_press_triggered = 0;
            }
            break;

        //-----------------------------------
        default:
        //-----------------------------------
            current_state = MATRIX_IDLE;
            break;
    }
}

//=================================================== 外部API实现 ===================================================

void matrix_keyboard_init(uint32 scan_period_ms)
{
    uint8 i;

    for(i = 0; i < MATRIX_ROW_NUM; i++)
    {
        gpio_init(matrix_row_pin[i], GPO, GPIO_HIGH, GPO_PUSH_PULL);
    }

    for(i = 0; i < MATRIX_COL_NUM; i++)
    {
        gpio_init(matrix_col_pin[i], GPI, GPIO_HIGH, GPI_PULL_UP);
    }

    scan_period = scan_period_ms;
    current_state = MATRIX_IDLE;
    key_value = 0;
    long_press_triggered = 0;
    debounce_count = 0;
    press_time_counter = 0;
    current_row = 0;
    current_col = 0;
    current_key_index = 0;

    key_info.state = MATRIX_IDLE;
    key_info.row = 0;
    key_info.col = 0;
    key_info.index = 0;
    key_info.press_time = 0;
    key_info.event = MATRIX_EVENT_NONE;
}

void matrix_keyboard_set_callback(matrix_key_callback callback)
{
    user_callback = callback;
}

void matrix_keyboard_set_enabled(uint8_t enabled)
{
    keyboard_enabled = enabled;
    if(!enabled) {
        current_state = MATRIX_IDLE;
    }
}

uint8_t matrix_keyboard_is_enabled(void)
{
    return keyboard_enabled;
}

uint8 matrix_keyboard_get_value(void)
{
    return key_value;
}

void matrix_keyboard_clear_event(void)
{
    key_value = 0;
}

matrix_key_info_struct matrix_keyboard_get_info(void)
{
    matrix_key_info_struct info;

    info.row = current_row;
    info.col = current_col;
    info.index = current_key_index;
    info.state = current_state;
    info.press_time = press_time_counter * scan_period;

    if(long_press_triggered)
    {
        info.event = MATRIX_EVENT_LONG_PRESS;
    }
    else if(current_state == MATRIX_PRESSED || current_state == MATRIX_RELEASE)
    {
        info.event = MATRIX_EVENT_SHORT_PRESS;
    }
    else
    {
        info.event = MATRIX_EVENT_NONE;
    }

    return info;
}

