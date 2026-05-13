#ifndef __FUNCTION_PLOTTER_H
#define __FUNCTION_PLOTTER_H

#include <stdint.h>

/**
 * 函数类型枚举
 * 支持7种数学函数类型:
 *   FUNC_QUADRATIC   - 二次函数: y = ax² + bx + c
 *   FUNC_CUBIC       - 三次函数: y = ax³ + bx² + cx + d
 *   FUNC_EXPONENTIAL - 指数函数: y = a * e^(bx) + c
 *   FUNC_LOGARITHM   - 对数函数: y = a * ln(bx + c) + d
 *   FUNC_SINE        - 正弦函数: y = a * sin(bx + c) + d
 *   FUNC_COSINE      - 余弦函数: y = a * cos(bx + c) + d
 *   FUNC_TANGENT     - 正切函数: y = a * tan(bx + c) + d
 */
typedef enum {
    FUNC_QUADRATIC,   // y = ax² + bx + c
    FUNC_CUBIC,       // y = ax³ + bx² + cx + d
    FUNC_EXPONENTIAL, // y = a * e^(bx)
    FUNC_LOGARITHM,   // y = a * ln(bx + c)
    FUNC_SINE,        // y = a * sin(bx + c) + d
    FUNC_COSINE,      // y = a * cos(bx + c) + d
    FUNC_TANGENT      // y = a * tan(bx + c) + d
} function_type_t;

/**
 * 预设函数结构体
 * 用于存储预设函数的所有信息:
 *   type   - 函数类型
 *   name   - 函数名称(显示用)
 *   coef   - 系数数组(a, b, c, d)
 */
typedef struct {
    function_type_t type;
    char name[28];
    float coef[4];
} function_preset_t;

/** 预设函数数组 - 包含12个预定义的函数图形 */
extern const function_preset_t presets[];

/** 当前选中的预设函数索引 */
extern uint8_t current_index;

/** 预设函数总数 */
extern uint8_t preset_count;

/**
 * @brief 绘制预设函数图形
 * @param preset 预设函数指针,包含函数类型和系数
 */
void function_plot(const function_preset_t *preset);

/**
 * @brief 绘制自定义函数图形
 * @param type 函数类型
 * @param coef 系数数组指针
 * @param display_page 系数显示页(0:显示a,b; 1:显示c,d)
 */
void function_plot_custom(function_type_t type, const float *coef, uint8_t display_page);

/**
 * @brief 通过串口发送函数信息
 * @param preset 预设函数指针
 */
void function_send_to_uart(const function_preset_t *preset);

/**
 * @brief 获取函数类型名称
 * @param type 函数类型
 * @return 函数类型的中文/英文名称
 */
const char* function_get_name(function_type_t type);

/**
 * @brief 获取函数表达式模板
 * @param type 函数类型
 * @return 函数表达式字符串(如"y=ax^2+bx+c")
 */
const char* function_get_expression(function_type_t type);

/**
 * @brief 获取指定函数类型的系数个数
 * @param type 函数类型
 * @return 系数数量(二次函数3个,其他4个)
 */
uint8_t function_get_coef_count(function_type_t type);

/**
 * @brief 格式化函数表达式字符串
 * @param buf 输出缓冲区
 * @param type 函数类型
 * @param coef 系数数组
 */
void function_format_expression(char *buf, function_type_t type, const float *coef);

#endif