#include "function_plotter.h"
#include "oled.h"
#include "uart1.h"
#include <math.h>

const function_preset_t presets[] = {
    {FUNC_QUADRATIC,   "y=x^2/50",          {0.02f,  0.0f,   0.0f,  0.0f}},
    {FUNC_QUADRATIC,   "y=-x^2/50+32",      {-0.02f, 0.0f,  32.0f,  0.0f}},
    {FUNC_QUADRATIC,   "y=x^2/30-16",       {0.033f, 0.0f, -16.0f,  0.0f}},
    {FUNC_CUBIC,       "y=x^3/2000",        {0.0005f, 0.0f,   0.0f,  0.0f}},
    {FUNC_CUBIC,       "y=x^3/8000-16",     {0.000125f, 0.0f, -16.0f, 0.0f}},
    {FUNC_EXPONENTIAL, "y=e^(x/15)-20",     {1.0f, 1.0f/15.0f, -20.0f, 0.0f}},
    {FUNC_EXPONENTIAL, "y=2^(x/12)-20",     {1.0f, 0.0578f, -20.0f, 0.0f}},
    {FUNC_LOGARITHM,   "y=12*ln(x+65)-32",  {12.0f, 1.0f, 65.0f, -32.0f}},
    {FUNC_LOGARITHM,   "y=8*ln(x+65)-16",   {8.0f,  1.0f, 65.0f, -16.0f}},
    {FUNC_SINE,        "y=32*sin(x*pi/32)",   {32.0f, 0.0982f, 0.0f, 0.0f}},
    {FUNC_COSINE,      "y=32*cos(x*pi/32)",   {32.0f, 0.0982f, 0.0f, 0.0f}},
    {FUNC_TANGENT,     "y=tan(x*pi/64)",     {1.0f, 0.0491f, 0.0f, 0.0f}},
};

uint8_t current_index = 0;
uint8_t preset_count = sizeof(presets) / sizeof(presets[0]);

static float eval_function(float x, const function_preset_t *p)
{
    switch(p->type) {
        case FUNC_QUADRATIC:
            return p->coef[0]*x*x + p->coef[1]*x + p->coef[2];
        case FUNC_CUBIC:
            return p->coef[0]*x*x*x + p->coef[1]*x*x + p->coef[2]*x + p->coef[3];
        case FUNC_EXPONENTIAL:
            return p->coef[0] * expf(p->coef[1] * x) + p->coef[2];
        case FUNC_LOGARITHM:
            return p->coef[0] * logf(p->coef[1] * x + p->coef[2]) + p->coef[3];
        case FUNC_SINE:
            return p->coef[0] * sinf(p->coef[1] * x + p->coef[2]) + p->coef[3];
        case FUNC_COSINE:
            return p->coef[0] * cosf(p->coef[1] * x + p->coef[2]) + p->coef[3];
        case FUNC_TANGENT:
            return p->coef[0] * tanf(p->coef[1] * x + p->coef[2]) + p->coef[3];
    }
    return 0;
}

static void draw_line(int8_t x0, int8_t y0, int8_t x1, int8_t y1)
{
    int8_t dx = (x0 < x1) ? (x1 - x0) : (x0 - x1);
    int8_t dy = (y0 < y1) ? (y1 - y0) : (y0 - y1);
    int8_t sx = (x0 < x1) ? 1 : -1;
    int8_t sy = (y0 < y1) ? 1 : -1;
    int8_t err = dx - dy;

    while(1) {
        if(x0 >= 0 && x0 < 128 && y0 >= 0 && y0 < 64) {
            // 如果是文本像素，先清除文字再绘制曲线
            if(text_mask[x0][y0]) {
                OLED_ClearPoint((uint8_t)x0, (uint8_t)y0);
            }
            OLED_DrawPoint((uint8_t)x0, (uint8_t)y0);
        }
        if(x0 == x1 && y0 == y1) break;
        int8_t e2 = err * 2;
        if(e2 > -dy) { err -= dy; x0 += sx; }
        if(e2 < dx) { err += dx; y0 += sy; }
    }
}

void function_plot(const function_preset_t *preset)
{
    OLED_Clear();

    // 1. 先绘制文本（建立文本像素掩码）
    OLED_ShowString(0, 0, (uint8_t*)preset->name, 12);

    // 2. 绘制坐标轴
    for(uint8_t x = 0; x < 128; x++) {
        OLED_DrawPoint(x, 32);
    }
    for(uint8_t y = 0; y < 64; y++) {
        OLED_DrawPoint(64, y);
    }

    // 3. 绘制函数曲线（曲线会覆盖文本像素）
    int8_t prev_py = -1;

    for(uint8_t px = 0; px < 128; px++) {
        float math_x = (int8_t)(px - 64);

        float y0 = eval_function(math_x - 0.25f, preset);
        float y1 = eval_function(math_x, preset);
        float y2 = eval_function(math_x + 0.25f, preset);

        float y = (y0 + y1 + y2) / 3.0f;

        int16_t screen_y = 32 - (int16_t)(y);

        // 如果超出屏幕范围，跳过该点（不绘制也不连线）
        if(screen_y < 0 || screen_y > 63) {
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

    OLED_Refresh();
}

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
