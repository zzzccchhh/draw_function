#include "function_plotter.h"
#include "oled.h"
#include "uart1.h"
#include <math.h>

const function_preset_t presets[] = {
    {FUNC_QUADRATIC,   "y=x^2",             {0.008f, 0.0f,  0.0f,  0.0f}},
    {FUNC_QUADRATIC,   "y=-x^2+32",        {-0.002f,0.0f,  32.0f, 0.0f}},
    {FUNC_QUADRATIC,   "y=0.1x^2-5",       {0.1f,   0.0f, -5.0f,  0.0f}},
    {FUNC_CUBIC,       "y=x^3/2000",       {0.0005f, 0.0f,  0.0f,  0.0f}},
    {FUNC_CUBIC,       "y=x^3-x",          {0.002f, -1.0f,  0.0f,  0.0f}},
    {FUNC_EXPONENTIAL, "y=e^x/80",         {1.0f/80.0f, 1.0f, 0.0f, 0.0f}},
    {FUNC_EXPONENTIAL, "y=2^x/100",        {1.0f/100.0f, 0.693f, 0.0f, 0.0f}},
    {FUNC_LOGARITHM,   "y=10*ln(x+1)",     {10.0f, 1.0f,  0.0f,  0.0f}},
    {FUNC_LOGARITHM,   "y=8*ln(x/5+1)",    {8.0f,  0.2f,  0.0f,  0.0f}},
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
            return p->coef[0] * expf(p->coef[1] * x);
        case FUNC_LOGARITHM:
            return p->coef[0] * logf(p->coef[1] * x + 1e-6f);
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
        if(x0 >= 0 && x0 < 128 && y0 >= 0 && y0 < 64)
            OLED_DrawPoint((uint8_t)x0, (uint8_t)y0);
        if(x0 == x1 && y0 == y1) break;
        int8_t e2 = err * 2;
        if(e2 > -dy) { err -= dy; x0 += sx; }
        if(e2 < dx) { err += dx; y0 += sy; }
    }
}

void function_plot(const function_preset_t *preset)
{
    OLED_Clear();

    int8_t prev_py = -1;

    for(uint8_t px = 0; px < 128; px++) {
        float x = (float)px;
        float y = eval_function(x, preset);

        if(y < 0) y = 0;
        if(y > 63) y = 63;

        uint8_t py = (uint8_t)(63 - y);

        if(prev_py != -1) {
            draw_line((int8_t)(px - 1), (int8_t)prev_py, (int8_t)px, (int8_t)py);
        }
        prev_py = (int8_t)py;
    }

    OLED_ShowString(0, 0, (uint8_t*)preset->name, 8);
    OLED_Refresh();
}

void function_send_to_uart(const function_preset_t *preset)
{
    const char *type_str;
    switch(preset->type) {
        case FUNC_QUADRATIC:   type_str = "Quadratic"; break;
        case FUNC_CUBIC:       type_str = "Cubic"; break;
        case FUNC_EXPONENTIAL: type_str = "Exponential"; break;
        case FUNC_LOGARITHM:   type_str = "Logarithm"; break;
        default:               type_str = "Unknown"; break;
    }

    USART1_Printf("\r\n===== Function Plotter =====\r\n");
    USART1_Printf("Current: %s\r\n", preset->name);
    USART1_Printf("Type: %s\r\n", type_str);
    USART1_Printf("Coefficients:\r\n");
    USART1_Printf("  a=%.3f, b=%.3f\r\n", preset->coef[0], preset->coef[1]);
    USART1_Printf("  c=%.3f, d=%.3f\r\n", preset->coef[2], preset->coef[3]);
    USART1_Printf("===========================\r\n");
}
