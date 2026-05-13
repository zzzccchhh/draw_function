#include "function_plotter.h"
#include "oled_ssd1306.h"
#include "oled_hal.h"
#include "uart1.h"
#include <math.h>
#include <stdio.h>

/**
 * 预设函数数组
 * 包含12个预定义的数学函数，用于在OLED屏幕上绘制函数图形
 * 每个元素包含: 函数类型、显示名称、系数(a,b,c,d)
 *
 * 支持的函数类型:
 *   二次函数: y = ax² + bx + c (需要3个系数: a,b,c)
 *   三次函数: y = ax³ + bx² + cx + d (需要4个系数)
 *   指数函数: y = a * e^(bx) + c (需要3个系数: a,b,c)
 *   对数函数: y = a * ln(bx + c) + d (需要4个系数)
 *   三角函数: y = a * sin(bx + c) + d 等 (需要4个系数)
 */
const function_preset_t presets[] = {
    // 二次函数示例 - 开口向上的抛物线
    {FUNC_QUADRATIC,   "y=x^2/50",          {0.02f,  0.0f,   0.0f,  0.0f}},
    // 二次函数示例 - 开口向下的抛物线，顶点在y=32
    {FUNC_QUADRATIC,   "y=-x^2/50+32",      {-0.02f, 0.0f,  32.0f,  0.0f}},
    // 二次函数示例 - 开口向上的抛物线，向下移动16单位
    {FUNC_QUADRATIC,   "y=x^2/30-16",       {0.033f, 0.0f, -16.0f,  0.0f}},
    // 三次函数示例 - 标准的立方函数
    {FUNC_CUBIC,       "y=x^3/2000",        {0.0005f, 0.0f,   0.0f,  0.0f}},
    // 三次函数示例 - 立方函数向下移动16单位
    {FUNC_CUBIC,       "y=x^3/8000-16",     {0.000125f, 0.0f, -16.0f, 0.0f}},
    // 指数函数示例 - 以e为底的指数函数
    {FUNC_EXPONENTIAL, "y=e^(x/15)-20",     {1.0f, 1.0f/15.0f, -20.0f, 0.0f}},
    // 指数函数示例 - 以2为底的指数函数
    {FUNC_EXPONENTIAL, "y=2^(x/12)-20",     {1.0f, 0.0578f, -20.0f, 0.0f}},
    // 对数函数示例 - 较大的对数函数曲线
    {FUNC_LOGARITHM,   "y=12*ln(x+65)-32",  {12.0f, 1.0f, 65.0f, -32.0f}},
    // 对数函数示例 - 较小的对数函数曲线
    {FUNC_LOGARITHM,   "y=8*ln(x+65)-16",   {8.0f,  1.0f, 65.0f, -16.0f}},
    // 正弦函数示例 - 标准正弦波，振幅32
    {FUNC_SINE,        "y=32*sin(x*pi/32)",   {32.0f, 0.0982f, 0.0f, 0.0f}},
    // 余弦函数示例 - 余弦波，与正弦波相位差90度
    {FUNC_COSINE,      "y=32*cos(x*pi/32)",   {32.0f, 0.0982f, 0.0f, 0.0f}},
    // 正切函数示例 - 正切波
    {FUNC_TANGENT,     "y=tan(x*pi/64)",     {1.0f, 0.0491f, 0.0f, 0.0f}},
};

/** 当前选中的预设函数索引，用于在多个预设函数间切换 */
uint8_t current_index = 0;

/** 预设函数总数，通过计算数组大小得出 */
uint8_t preset_count = sizeof(presets) / sizeof(presets[0]);

/**
 * 获取函数类型的英文名称
 * @param type 函数类型枚举值
 * @return 函数类型的英文名称字符串
 *
 * 示例: FUNC_SINE -> "Sine"
 */
const char* function_get_name(function_type_t type)
{
    switch(type) {
        case FUNC_QUADRATIC:   return "Quadratic";
        case FUNC_CUBIC:       return "Cubic";
        case FUNC_EXPONENTIAL: return "Exponential";
        case FUNC_LOGARITHM:   return "Logarithm";
        case FUNC_SINE:        return "Sine";
        case FUNC_COSINE:      return "Cosine";
        case FUNC_TANGENT:     return "Tangent";
        default:               return "Unknown";
    }
}

/**
 * 获取函数表达式的模板字符串
 * @param type 函数类型枚举值
 * @return 函数表达式的模板，如 "y=ax^2+bx+c"
 *
 * 注意: 这是通用模板，不是带具体系数的表达式
 */
const char* function_get_expression(function_type_t type)
{
    switch(type) {
        case FUNC_QUADRATIC:   return "y=ax^2+bx+c";
        case FUNC_CUBIC:       return "y=ax^3+bx^2+cx+d";
        case FUNC_EXPONENTIAL: return "y=a*e^(bx)+c";
        case FUNC_LOGARITHM:   return "y=a*ln(bx+c)+d";
        case FUNC_SINE:        return "y=a*sin(bx+c)+d";
        case FUNC_COSINE:      return "y=a*cos(bx+c)+d";
        case FUNC_TANGENT:     return "y=a*tan(bx+c)+d";
        default:               return "Unknown";
    }
}

/**
 * 获取指定函数类型需要的系数个数
 * @param type 函数类型枚举值
 * @return 系数数量 (二次/指数函数返回3，其他返回4)
 *
 * 说明:
 *   - 二次函数 y=ax²+bx+c 需要3个系数: a, b, c
 *   - 三次函数 y=ax³+bx²+cx+d 需要4个系数: a, b, c, d
 *   - 指数函数 y=a*e^(bx)+c 需要3个系数: a, b, c
 *   - 对数/三角函数需要4个系数: a, b, c, d
 */
uint8_t function_get_coef_count(function_type_t type)
{
    switch(type) {
        case FUNC_QUADRATIC:   return 3;
        case FUNC_CUBIC:       return 4;
        case FUNC_EXPONENTIAL: return 3;
        case FUNC_LOGARITHM:   return 4;
        case FUNC_SINE:        return 4;
        case FUNC_COSINE:      return 4;
        case FUNC_TANGENT:     return 4;
        default:               return 0;
    }
}

/**
 * 根据给定系数计算函数在指定x处的值（内部函数）
 * @param x 数学坐标系中的x值
 * @param type 函数类型
 * @param coef 系数数组指针
 * @return 函数计算结果，若数学上无定义则返回NAN
 *
 * 计算规则:
 *   二次函数: y = ax² + bx + c
 *   三次函数: y = ax³ + bx² + cx + d
 *   指数函数: y = a * e^(bx) + c
 *   对数函数: y = a * ln(bx + c) + d (若bx+c<=0则返回NAN)
 *   正弦函数: y = a * sin(bx + c) + d
 *   余弦函数: y = a * cos(bx + c) + d
 *   正切函数: y = a * tan(bx + c) + d (若正切值无穷大则返回NAN)
 */
static float eval_function_custom(float x, function_type_t type, const float *coef)
{
    switch(type) {
        case FUNC_QUADRATIC:
            return coef[0]*x*x + coef[1]*x + coef[2];
        case FUNC_CUBIC:
            return coef[0]*x*x*x + coef[1]*x*x + coef[2]*x + coef[3];
        case FUNC_EXPONENTIAL:
            return coef[0] * expf(coef[1] * x) + coef[2];
        case FUNC_LOGARITHM: {
            float arg = coef[1] * x + coef[2];
            if(arg <= 0) return NAN;
            return coef[0] * logf(arg) + coef[3];
        }
        case FUNC_SINE:
            return coef[0] * sinf(coef[1] * x + coef[2]) + coef[3];
        case FUNC_COSINE:
            return coef[0] * cosf(coef[1] * x + coef[2]) + coef[3];
        case FUNC_TANGENT: {
            float arg = coef[1] * x + coef[2];
            float tan_val = tanf(arg);
            if(isinf(tan_val)) return NAN;
            return coef[0] * tan_val + coef[3];
        }
    }
    return 0;
}

/**
 * 根据给定预设函数计算在指定x处的值（内部函数）
 * @param x 数学坐标系中的x值
 * @param p 预设函数指针
 * @return 函数计算结果
 *
 * 此函数是对eval_function_custom的封装，提供更简洁的接口
 */
static float eval_function(float x, const function_preset_t *p)
{
    return eval_function_custom(x, p->type, p->coef);
}

/**
 * 在OLED屏幕上绘制系数值（内部函数）
 * @param type 函数类型
 * @param coef 系数数组指针
 * @param display_page 显示页 (0:显示a,b  1:显示c,d)
 *
 * 功能说明:
 *   - 清屏OLED底部区域 (y: 56-63)
 *   - 根据display_page选择显示不同的系数
 *   - 系数格式: "a=xx.xx, b=xx.xx" 格式
 *   - 使用手动字符绘制实现数字显示（不依赖printf）
 */
static void draw_coefficients(function_type_t type, const float *coef, uint8_t display_page);

/**
 * 使用Bresenham算法在OLED屏幕上绘制直线
 * @param x0 起点x坐标
 * @param y0 起点y坐标
 * @param x1 终点x坐标
 * @param y1 终点y坐标
 *
 * 算法说明:
 *   Bresenham直线算法是一种高效的像素绘制算法
 *   通过整数运算确定最佳像素位置，避免浮点计算
 *   绘制范围限制在 x:0-127, y:8-55 (OLED有效显示区)
 */
static void draw_line(int8_t x0, int8_t y0, int8_t x1, int8_t y1)
{
    int8_t dx = (x0 < x1) ? (x1 - x0) : (x0 - x1);
    int8_t dy = (y0 < y1) ? (y1 - y0) : (y0 - y1);
    int8_t sx = (x0 < x1) ? 1 : -1;
    int8_t sy = (y0 < y1) ? 1 : -1;
    int8_t err = dx - dy;

    while(1) {
        if(x0 >= 0 && x0 < 128 && y0 >= 8 && y0 < 56) {
            ssd1306_drawPixel(x0, y0, WHITE);
        }
        if(x0 == x1 && y0 == y1) break;
        int8_t e2 = err * 2;
        if(e2 > -dy) { err -= dy; x0 += sx; }
        if(e2 < dx) { err += dx; y0 += sy; }
    }
}

/**
 * 绘制自定义函数图形到OLED屏幕
 * @param type 函数类型
 * @param coef 系数数组指针
 * @param display_page 系数显示页 (0:显示a,b  1:显示c,d)
 *
 * 绘制流程:
 *   1. 清屏
 *   2. 在屏幕顶部显示函数表达式模板
 *   3. 绘制坐标轴 (x轴在y=32, y轴在x=64)
 *   4. 对屏幕每个像素列计算对应的数学x坐标
 *   5. 使用三点平均法平滑函数曲线
 *   6. 使用draw_line连接相邻点形成平滑曲线
 *   7. 在屏幕底部显示当前页的系数值
 *   8. 刷新屏幕显示
 *
 * 坐标映射:
 *   屏幕x: 0-127 -> 数学x: -21.3 到 +21.3 (比例: 3像素=1单位)
 *   屏幕y: 32-8  -> 数学y: 0 到 +12
 *   屏幕y: 32-55 -> 数学y: 0 到 -11.5
 */
void function_plot_custom(function_type_t type, const float *coef, uint8_t display_page)
{
    ssd1306_clearScreen();

    oled_setTextSize(1);
    oled_drawText(0, 0, function_get_expression(type));

    for(uint8_t x = 0; x < 128; x++) {
        ssd1306_drawPixel(x, 32, WHITE);
    }
    for(uint8_t y = 8; y < 64; y++) {
        ssd1306_drawPixel(64, y, WHITE);
    }

    int8_t prev_py = -1;

    for(uint8_t px = 0; px < 128; px++) {
        float math_x = ((int8_t)(px - 64)) / 3.0f;

        float y0 = eval_function_custom(math_x - 0.25f, type, coef);
        float y1 = eval_function_custom(math_x, type, coef);
        float y2 = eval_function_custom(math_x + 0.25f, type, coef);

        float y = (y0 + y1 + y2) / 3.0f;

        int16_t screen_y = 32 - (int16_t)(y / 2.0f);

        if(isnan(y) || screen_y < 8 || screen_y >= 56) {
            prev_py = -1;
            continue;
        }

        if(prev_py != -1) {
            int8_t dy = screen_y - prev_py;
            if(dy < -32 || dy > 32) {
                prev_py = -1;
            } else {
                draw_line((int8_t)(px - 1), prev_py, (int8_t)px, screen_y);
            }
        }
        prev_py = screen_y;
    }

    ssd1306_updateScreen();
    draw_coefficients(type, coef, display_page);
    ssd1306_updateScreen();
}

/**
 * 绘制预设函数图形到OLED屏幕
 * @param preset 预设函数指针，包含函数类型和系数
 *
 * 绘制流程:
 *   1. 清屏
 *   2. 在屏幕顶部显示预设函数名称
 *   3. 绘制坐标轴 (x轴在y=32, y轴在x=64)
 *   4. 对屏幕每个像素列计算对应的数学x坐标
 *   5. 使用三点平均法平滑函数曲线
 *   6. 使用draw_line连接相邻点形成平滑曲线
 *   7. 刷新屏幕显示
 *
 * 坐标映射:
 *   屏幕x: 0-127 -> 数学x: -21.3 到 +21.3 (比例: 3像素=1单位)
 *   屏幕y: 32-8  -> 数学y: 0 到 +12
 *   屏幕y: 32-55 -> 数学y: 0 到 -11.5
 */
void function_plot(const function_preset_t *preset)
{
    ssd1306_clearScreen();

    oled_setTextSize(1);
    oled_drawText(0, 0, preset->name);

    for(uint8_t x = 0; x < 128; x++) {
        ssd1306_drawPixel(x, 32, WHITE);
    }
    for(uint8_t y = 8; y < 64; y++) {
        ssd1306_drawPixel(64, y, WHITE);
    }

    int8_t prev_py = -1;

    for(uint8_t px = 0; px < 128; px++) {
        float math_x = ((int8_t)(px - 64)) / 3.0f;

        float y0 = eval_function(math_x - 0.25f, preset);
        float y1 = eval_function(math_x, preset);
        float y2 = eval_function(math_x + 0.25f, preset);

        float y = (y0 + y1 + y2) / 3.0f;

        int16_t screen_y = 32 - (int16_t)(y / 2.0f);

        if(isnan(y) || screen_y < 8 || screen_y >= 56) {
            prev_py = -1;
            continue;
        }

        if(prev_py != -1) {
            int8_t dy = screen_y - prev_py;
            if(dy < -32 || dy > 32) {
                prev_py = -1;
            } else {
                draw_line((int8_t)(px - 1), prev_py, (int8_t)px, screen_y);
            }
        }
        prev_py = screen_y;
    }

    ssd1306_updateScreen();
}

/**
 * 在OLED屏幕底部区域绘制函数系数值
 * @param type 函数类型
 * @param coef 系数数组指针
 * @param display_page 显示页选择 (0:显示a,b  1:显示c,d)
 *
 * 显示格式: "a=xx.xx, b=xx.xx" 或 "c=xx.xx, d=xx.xx"
 * 数值格式: 保留两位小数，使用字符逐个绘制
 *
 * 处理逻辑:
 *   1. 先用黑色清除底部区域 (y: 56-63)
 *   2. 根据display_page选择显示的系数范围
 *   3. 依次绘制系数名、等于号、数值
 *   4. 数值手动分解为整数部分和小数部分绘制
 */
static void draw_coefficients(function_type_t type, const float *coef, uint8_t display_page)
{
    uint8_t count = function_get_coef_count(type);
    const char *names = "abcd";

    for(uint8_t x = 0; x < 128; x++) {
        for(uint8_t y = 56; y < 64; y++) {
            ssd1306_drawPixel(x, y, BLACK);
        }
    }
    oled_setTextSize(1);

    uint8_t x_pos = 0;

    if(display_page == 0) {
        uint8_t start = 0;
        uint8_t end = (count >= 2) ? 2 : count;
        for(uint8_t i = start; i < end && x_pos < 110; i++) {
            if(i > start) {
                oled_drawChar(x_pos, 56, ',', WHITE, BLACK, 1);
                x_pos += 6;
            }
            oled_drawChar(x_pos, 56, names[i], WHITE, BLACK, 1);
            x_pos += 6;
            oled_drawChar(x_pos, 56, '=', WHITE, BLACK, 1);
            x_pos += 6;

            float val = coef[i];
            float abs_val = (val < 0) ? -val : val;
            int32_t int_part = (int32_t)abs_val;
            int32_t frac_part = (int32_t)((abs_val - int_part) * 100);

            if(val < 0) {
                oled_drawChar(x_pos, 56, '-', WHITE, BLACK, 1);
                x_pos += 6;
            }

            if(int_part >= 10) {
                oled_drawChar(x_pos, 56, '0' + (int_part / 10), WHITE, BLACK, 1);
                x_pos += 6;
            }
            oled_drawChar(x_pos, 56, '0' + (int_part % 10), WHITE, BLACK, 1);
            x_pos += 6;
            oled_drawChar(x_pos, 56, '.', WHITE, BLACK, 1);
            x_pos += 6;
            oled_drawChar(x_pos, 56, '0' + (frac_part / 10), WHITE, BLACK, 1);
            x_pos += 6;
            oled_drawChar(x_pos, 56, '0' + (frac_part % 10), WHITE, BLACK, 1);
            x_pos += 6;
        }
    } else {
        uint8_t start = 2;
        uint8_t end = count;
        if(start < count) {
            for(uint8_t i = start; i < end && x_pos < 110; i++) {
                if(i > start) {
                    oled_drawChar(x_pos, 56, ',', WHITE, BLACK, 1);
                    x_pos += 6;
                }
                oled_drawChar(x_pos, 56, names[i], WHITE, BLACK, 1);
                x_pos += 6;
                oled_drawChar(x_pos, 56, '=', WHITE, BLACK, 1);
                x_pos += 6;

                float val = coef[i];
                float abs_val = (val < 0) ? -val : val;
                int32_t int_part = (int32_t)abs_val;
                int32_t frac_part = (int32_t)((abs_val - int_part) * 100);

                if(val < 0) {
                    oled_drawChar(x_pos, 56, '-', WHITE, BLACK, 1);
                    x_pos += 6;
                }

                if(int_part >= 10) {
                    oled_drawChar(x_pos, 56, '0' + (int_part / 10), WHITE, BLACK, 1);
                    x_pos += 6;
                }
                oled_drawChar(x_pos, 56, '0' + (int_part % 10), WHITE, BLACK, 1);
                x_pos += 6;
                oled_drawChar(x_pos, 56, '.', WHITE, BLACK, 1);
                x_pos += 6;
                oled_drawChar(x_pos, 56, '0' + (frac_part / 10), WHITE, BLACK, 1);
                x_pos += 6;
                oled_drawChar(x_pos, 56, '0' + (frac_part % 10), WHITE, BLACK, 1);
                x_pos += 6;
            }
        }
    }
}

/**
 * 将浮点数转换为字符串（内部函数，用于串口输出）
 * @param buf 输出字符串缓冲区
 * @param val 要转换的浮点数值
 *
 * 转换格式: "-x.xx" 或 "x.xx" (保留3位小数)
 * 不处理整数部分大于99的情况
 */
static void float_to_str(char *buf, float val)
{
    int32_t int_part, frac_part;
    int sign = 0;

    if(val < 0) {
        sign = 1;
        val = -val;
    }

    int_part = (int32_t)val;
    frac_part = (int32_t)((val - int_part) * 1000);

    if(sign) *buf++ = '-';
    if(int_part >= 10) {
        *buf++ = '0' + (int_part / 10);
    }
    *buf++ = '0' + (int_part % 10);
    *buf++ = '.';
    *buf++ = '0' + (frac_part / 100);
    *buf++ = '0' + ((frac_part / 10) % 10);
    *buf++ = '0' + (frac_part % 10);
    *buf = '\0';
}

/**
 * 将浮点数追加到字符串缓冲区（内部函数）
 * @param dest 目标字符串指针的地址（函数内会递增指针）
 * @param val 要格式化的浮点数值
 *
 * 功能: 将val以"xx.xx"格式追加到*dest指向的位置
 *       函数返回后*dest指向写入内容的下一个位置
 * 格式: 保留两位小数，负数带负号
 */
static void format_float(char **dest, float val)
{
    char *buf = *dest;
    int neg = 0;

    if(val < 0) {
        neg = 1;
        val = -val;
    }

    int int_part = (int)val;
    int frac_part = (int)((val - int_part) * 100);

    if(neg) *buf++ = '-';

    if(int_part >= 10) {
        *buf++ = '0' + (int_part / 10);
    }
    *buf++ = '0' + (int_part % 10);

    *buf++ = '.';
    *buf++ = '0' + (frac_part / 10);
    *buf++ = '0' + (frac_part % 10);

    *dest = buf;
}

/**
 * 格式化函数表达式字符串
 * @param buf 输出字符串缓冲区
 * @param type 函数类型
 * @param coef 系数数组
 *
 * 将函数类型和系数转换为可读的表达式字符串
 * 例如: 二次函数 a=1,b=-2,c=3 -> "y=1.00x^2+-2.00x+3.00"
 *
 * 输出格式说明:
 *   - 以"y="开头
 *   - 系数使用format_float格式化（保留两位小数）
 *   - 三角函数、对数函数使用括号，如 "y=1.00*sin(0.10x+0.00)+0.00"
 */
void function_format_expression(char *buf, function_type_t type, const float *coef)
{
    char *p = buf;

    *p++ = 'y';
    *p++ = '=';

    switch(type) {
        case FUNC_QUADRATIC:
            format_float(&p, coef[0]);
            *p++ = 'x'; *p++ = '^'; *p++ = '2'; *p++ = '+';
            format_float(&p, coef[1]);
            *p++ = 'x'; *p++ = '+';
            format_float(&p, coef[2]);
            break;
        case FUNC_CUBIC:
            format_float(&p, coef[0]);
            *p++ = 'x'; *p++ = '^'; *p++ = '3'; *p++ = '+';
            format_float(&p, coef[1]);
            *p++ = 'x'; *p++ = '^'; *p++ = '2'; *p++ = '+';
            format_float(&p, coef[2]);
            *p++ = 'x'; *p++ = '+';
            format_float(&p, coef[3]);
            break;
        case FUNC_EXPONENTIAL:
            format_float(&p, coef[0]);
            *p++ = '*'; *p++ = 'e'; *p++ = '^'; *p++ = '(';
            format_float(&p, coef[1]);
            *p++ = 'x'; *p++ = ')'; *p++ = '+';
            format_float(&p, coef[2]);
            break;
        case FUNC_LOGARITHM:
            format_float(&p, coef[0]);
            *p++ = '*'; *p++ = 'l'; *p++ = 'n'; *p++ = '(';
            format_float(&p, coef[1]);
            *p++ = 'x'; *p++ = '+';
            format_float(&p, coef[2]);
            *p++ = ')'; *p++ = '+';
            format_float(&p, coef[3]);
            break;
        case FUNC_SINE:
            format_float(&p, coef[0]);
            *p++ = '*'; *p++ = 's'; *p++ = 'i'; *p++ = 'n'; *p++ = '(';
            format_float(&p, coef[1]);
            *p++ = 'x'; *p++ = '+';
            format_float(&p, coef[2]);
            *p++ = ')'; *p++ = '+';
            format_float(&p, coef[3]);
            break;
        case FUNC_COSINE:
            format_float(&p, coef[0]);
            *p++ = '*'; *p++ = 'c'; *p++ = 'o'; *p++ = 's'; *p++ = '(';
            format_float(&p, coef[1]);
            *p++ = 'x'; *p++ = '+';
            format_float(&p, coef[2]);
            *p++ = ')'; *p++ = '+';
            format_float(&p, coef[3]);
            break;
        case FUNC_TANGENT:
            format_float(&p, coef[0]);
            *p++ = '*'; *p++ = 't'; *p++ = 'a'; *p++ = 'n'; *p++ = '(';
            format_float(&p, coef[1]);
            *p++ = 'x'; *p++ = '+';
            format_float(&p, coef[2]);
            *p++ = ')'; *p++ = '+';
            format_float(&p, coef[3]);
            break;
    }
    *p = '\0';
}

/**
 * 通过串口发送当前函数信息
 * @param preset 预设函数指针
 *
 * 发送内容包括:
 *   - 分隔线: "===== Function Plotter ====="
 *   - 当前函数名称
 *   - 函数类型 (英文)
 *   - 四个系数的值 (a,b,c,d)
 *   - 分隔线: "==========================="
 *
 * 数值格式: 使用float_to_str转换，保留3位小数
 * 通信协议: 使用USART1_Printf，消息以\r\n结尾
 */
void function_send_to_uart(const function_preset_t *preset)
{
    const char *type_str;
    char buf[16];

    switch(preset->type) {
        case FUNC_QUADRATIC:   type_str = "Quadratic"; break;
        case FUNC_CUBIC:       type_str = "Cubic"; break;
        case FUNC_EXPONENTIAL: type_str = "Exponential"; break;
        case FUNC_LOGARITHM:   type_str = "Logarithm"; break;
        case FUNC_SINE:        type_str = "Sine"; break;
        case FUNC_COSINE:      type_str = "Cosine"; break;
        case FUNC_TANGENT:     type_str = "Tangent"; break;
        default:               type_str = "Unknown"; break;
    }

    USART1_Printf("\r\n===== Function Plotter =====\r\n");
    USART1_Printf("Current: %s\r\n", preset->name);
    USART1_Printf("Type: %s\r\n", type_str);
    USART1_Printf("Coefficients:\r\n");

    float_to_str(buf, preset->coef[0]);
    USART1_Printf("  a=%s", buf);
    float_to_str(buf, preset->coef[1]);
    USART1_Printf(", b=%s\r\n", buf);
    float_to_str(buf, preset->coef[2]);
    USART1_Printf("  c=%s", buf);
    float_to_str(buf, preset->coef[3]);
    USART1_Printf(", d=%s\r\n", buf);

    USART1_Printf("===========================\r\n");
}