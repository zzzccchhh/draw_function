#include "function_plotter.h"
#include "oled_ssd1306.h"
#include "oled_hal.h"
#include "uart1.h"
#include <math.h>
#include <stdio.h>

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

static float eval_function_custom(float x, function_type_t type, const float *coef)
{
    switch(type) {
        case FUNC_QUADRATIC:
            return coef[0]*x*x + coef[1]*x + coef[2];
        case FUNC_CUBIC:
            return coef[0]*x*x*x + coef[1]*x*x + coef[2]*x + coef[3];
        case FUNC_EXPONENTIAL:
            return coef[0] * expf(coef[1] * x) + coef[2];
        case FUNC_LOGARITHM:
            return coef[0] * logf(coef[1] * x + coef[2]) + coef[3];
        case FUNC_SINE:
            return coef[0] * sinf(coef[1] * x + coef[2]) + coef[3];
        case FUNC_COSINE:
            return coef[0] * cosf(coef[1] * x + coef[2]) + coef[3];
        case FUNC_TANGENT:
            return coef[0] * tanf(coef[1] * x + coef[2]) + coef[3];
    }
    return 0;
}

static float eval_function(float x, const function_preset_t *p)
{
    return eval_function_custom(x, p->type, p->coef);
}

static void draw_coefficients(function_type_t type, const float *coef);
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

void function_plot_custom(function_type_t type, const float *coef)
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
        float math_x = (int8_t)(px - 64);

        float y0 = eval_function_custom(math_x - 0.25f, type, coef);
        float y1 = eval_function_custom(math_x, type, coef);
        float y2 = eval_function_custom(math_x + 0.25f, type, coef);

        float y = (y0 + y1 + y2) / 3.0f;

        int16_t screen_y = 32 - (int16_t)(y);

        if(screen_y < 8 || screen_y >= 56) {
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
    draw_coefficients(type, coef);
    ssd1306_updateScreen();
}

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
        float math_x = (int8_t)(px - 64);

        float y0 = eval_function(math_x - 0.25f, preset);
        float y1 = eval_function(math_x, preset);
        float y2 = eval_function(math_x + 0.25f, preset);

        float y = (y0 + y1 + y2) / 3.0f;

        int16_t screen_y = 32 - (int16_t)(y);

        if(screen_y < 8 || screen_y >= 56) {
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

static void draw_coefficients(function_type_t type, const float *coef)
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
    for(uint8_t i = 0; i < count && x_pos < 110; i++) {
        if(i > 0) {
            oled_drawChar(x_pos, 56, ',', WHITE, BLACK, 1);
            x_pos += 6;
        }
        oled_drawChar(x_pos, 56, names[i], WHITE, BLACK, 1);
        x_pos += 6;
        oled_drawChar(x_pos, 56, '=', WHITE, BLACK, 1);
        x_pos += 6;

        float val = coef[i];
        int32_t int_part = (int32_t)val;
        int32_t frac_part = (int32_t)((val < 0 ? -val : val - int_part) * 100);

        if(val < 0) {
            oled_drawChar(x_pos, 56, '-', WHITE, BLACK, 1);
            x_pos += 6;
            int_part = -int_part;
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