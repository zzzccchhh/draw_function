#include "app_state.h"
#include <stdlib.h>

static app_state_t current_state = STATE_IDLE;
static coef_input_context_t coef_ctx;

static const char* func_type_names[] = {
    "Quadratic",
    "Cubic",
    "Exponential",
    "Logarithm",
    "Sine",
    "Cosine",
    "Tangent"
};

static const uint8_t coef_counts[] = {
    3,  // FUNC_QUADRATIC
    4,  // FUNC_CUBIC
    3,  // FUNC_EXPONENTIAL
    4,  // FUNC_LOGARITHM
    4,  // FUNC_SINE
    4,  // FUNC_COSINE
    4   // FUNC_TANGENT
};

void app_state_init(void)
{
    current_state = STATE_IDLE;
    coef_ctx.func_type = FUNC_QUADRATIC;
    coef_ctx.current_coef_index = 0;
    coef_ctx.coef_count = 3;
    coef_ctx.cursor_pos = 0;
    coef_ctx.sign = 1;
    coef_ctx.has_decimal = 0;
    for(uint8_t i = 0; i < 4; i++) {
        coef_ctx.coef[i] = 0.0f;
        coef_ctx.input_buffer[i][0] = '\0';
    }
    // Default: y = x^2/50
    coef_ctx.coef[0] = 0.02f;  // a
    coef_ctx.coef[1] = 0.0f;   // b
    coef_ctx.coef[2] = 0.0f;   // c
}

app_state_t app_state_get_current(void)
{
    return current_state;
}

void app_state_set_next(app_state_t next)
{
    current_state = next;
}

const char* app_state_get_func_name(function_type_t type)
{
    if(type >= FUNC_QUADRATIC && type <= FUNC_TANGENT) {
        return func_type_names[type];
    }
    return "Unknown";
}

uint8_t app_state_get_coef_count(function_type_t type)
{
    if(type >= FUNC_QUADRATIC && type <= FUNC_TANGENT) {
        return coef_counts[type];
    }
    return 0;
}

void app_state_cycle_function(void)
{
    coef_ctx.func_type = (function_type_t)((coef_ctx.func_type + 1) % 7);
    coef_ctx.coef_count = app_state_get_coef_count(coef_ctx.func_type);
    coef_input_reset();
}

void app_state_set_function(function_type_t type)
{
    coef_ctx.func_type = type;
    coef_ctx.coef_count = app_state_get_coef_count(type);
    coef_input_reset();
}

void coef_input_reset(void)
{
    coef_ctx.current_coef_index = 0;
    coef_ctx.cursor_pos = 0;
    coef_ctx.sign = 1;
    coef_ctx.has_decimal = 0;
    for(uint8_t i = 0; i < 4; i++) {
        coef_ctx.input_buffer[i][0] = '\0';
    }
}

void coef_input_resume(void)
{
    coef_ctx.current_coef_index = 0;
    coef_ctx.cursor_pos = 0;
    coef_ctx.sign = 1;
    coef_ctx.has_decimal = 0;
    // 不清空 input_buffer，保留上次的输入值
}

void coef_input_set_coef(uint8_t index, float value)
{
    if(index < 4) {
        coef_ctx.coef[index] = value;
    }
}

float coef_input_get_coef(uint8_t index)
{
    if(index < 4) {
        return coef_ctx.coef[index];
    }
    return 0.0f;
}

coef_input_context_t* coef_input_get_context(void)
{
    return &coef_ctx;
}

void coef_input_add_char(char c)
{
    if(c == '-' || c == '+') {
        coef_ctx.sign = (c == '-') ? -1 : 1;
        return;
    }

    uint8_t idx = coef_ctx.current_coef_index;
    if(coef_ctx.cursor_pos < 11) {
        coef_ctx.input_buffer[idx][coef_ctx.cursor_pos++] = c;
        coef_ctx.input_buffer[idx][coef_ctx.cursor_pos] = '\0';
        USART1_Printf("\r\nadd_char: idx=%d, buffer=\"%s\"\r\n", idx, coef_ctx.input_buffer[idx]);
    }
}

void coef_input_toggle_sign(void)
{
    uint8_t idx = coef_ctx.current_coef_index;
    coef_ctx.sign *= -1;

    if(coef_ctx.input_buffer[idx][0] == '-') {
        for(int i = 0; coef_ctx.input_buffer[idx][i] != '\0'; i++) {
            coef_ctx.input_buffer[idx][i] = coef_ctx.input_buffer[idx][i+1];
        }
        if(coef_ctx.cursor_pos > 0) coef_ctx.cursor_pos--;
    } else if(coef_ctx.input_buffer[idx][0] != '\0') {
        for(int i = 10; i > 0; i--) {
            coef_ctx.input_buffer[idx][i] = coef_ctx.input_buffer[idx][i-1];
        }
        coef_ctx.input_buffer[idx][0] = '-';
        coef_ctx.input_buffer[idx][++coef_ctx.cursor_pos] = '\0';
    }

    USART1_Printf("\r\ntoggle_sign: idx=%d, sign=%d, buffer=\"%s\"\r\n",
                  idx, coef_ctx.sign, coef_ctx.input_buffer[idx]);
}

void coef_input_clear(void)
{
    uint8_t idx = coef_ctx.current_coef_index;
    coef_ctx.input_buffer[idx][0] = '\0';
    coef_ctx.cursor_pos = 0;
    coef_ctx.sign = 1;
    coef_ctx.has_decimal = 0;
}

void coef_input_prev(void)
{
    if(coef_ctx.current_coef_index > 0) {
        uint8_t idx = coef_ctx.current_coef_index;
        if(coef_ctx.input_buffer[idx][0] != '\0') {
            coef_ctx.coef[idx] = atof(coef_ctx.input_buffer[idx]) * coef_ctx.sign;
        } else {
            coef_ctx.coef[idx] = 0.0f;
        }
        coef_ctx.current_coef_index--;
        coef_ctx.sign = 1;
        coef_ctx.has_decimal = 0;
        coef_ctx.cursor_pos = 0;
        while(coef_ctx.input_buffer[coef_ctx.current_coef_index][coef_ctx.cursor_pos] != '\0') {
            coef_ctx.cursor_pos++;
        }
        USART1_Printf("\r\ncoef_input_prev: now at index %d, buffer=\"%s\"\r\n",
                      coef_ctx.current_coef_index, coef_ctx.input_buffer[coef_ctx.current_coef_index]);
    }
}

void coef_input_next(void)
{
    if(coef_ctx.current_coef_index < coef_ctx.coef_count - 1) {
        uint8_t idx = coef_ctx.current_coef_index;
        if(coef_ctx.input_buffer[idx][0] != '\0') {
            coef_ctx.coef[idx] = atof(coef_ctx.input_buffer[idx]) * coef_ctx.sign;
        } else {
            coef_ctx.coef[idx] = 0.0f;
        }
        coef_ctx.current_coef_index++;
        coef_ctx.sign = 1;
        coef_ctx.has_decimal = 0;
        coef_ctx.cursor_pos = 0;
        while(coef_ctx.input_buffer[coef_ctx.current_coef_index][coef_ctx.cursor_pos] != '\0') {
            coef_ctx.cursor_pos++;
        }
        USART1_Printf("\r\ncoef_input_next: now at index %d, buffer=\"%s\"\r\n",
                      coef_ctx.current_coef_index, coef_ctx.input_buffer[coef_ctx.current_coef_index]);
    }
}

void coef_input_confirm(void)
{
    uint8_t idx = coef_ctx.current_coef_index;

    if(coef_ctx.input_buffer[idx][0] != '\0') {
        coef_ctx.coef[idx] = atof(coef_ctx.input_buffer[idx]) * coef_ctx.sign;
    } else {
        coef_ctx.coef[idx] = 0.0f;
    }

    coef_ctx.sign = 1;
    coef_ctx.has_decimal = 0;
    coef_ctx.display_page = 0;

    app_state_set_next(STATE_DISPLAY);

    char expr_buf[64];
    function_format_expression(expr_buf, coef_ctx.func_type, coef_ctx.coef);
    USART1_Printf("\r\nExpression: %s\r\n", expr_buf);

    function_plot_custom(coef_ctx.func_type, coef_ctx.coef, coef_ctx.display_page);
    USART1_SendString("\r\nConfirmed coefficients - displaying graph\r\n");
}

void coef_input_add_decimal(void)
{
    if(!coef_ctx.has_decimal) {
        uint8_t idx = coef_ctx.current_coef_index;
        if(coef_ctx.cursor_pos < 11) {
            coef_ctx.input_buffer[idx][coef_ctx.cursor_pos++] = '.';
            coef_ctx.input_buffer[idx][coef_ctx.cursor_pos] = '\0';
            coef_ctx.has_decimal = 1;
        }
    }
}

void coef_input_toggle_display_page(void)
{
    coef_ctx.display_page = !coef_ctx.display_page;
}