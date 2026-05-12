/*
 * ch32v307 function plotter with matrix keyboard
 * Matrix keyboard for coefficient input, PE8 for function selection
 */

#include "zf_common_headfile.h"
#include "oled_ssd1306.h"
#include "oled_hal.h"
#include "uart1.h"
#include "button.h"
#include "function_plotter.h"
#include "matrix_keyboard.h"
#include "app_state.h"
#include <math.h>

#define COEF_INPUT_TIMEOUT_MS  30000

volatile uint32_t system_ms = 0;
static volatile uint32_t last_input_time = 0;
static volatile uint8_t cursor_blink_state = 0;
static volatile uint32_t cursor_blink_timer = 0;

void button_callback(button_event_t event);
void matrix_key_event_handler(uint8_t key_index, matrix_key_event_enum event, uint8_t key_value);
void update_display(void);
void display_function_select(void);
void display_coef_input(void);
void display_graph(void);
float parse_coefficient(const char *str, int8_t sign);
void handle_digit_input(uint8_t digit);
void handle_decimal_input(void);
void handle_backspace(void);
void handle_clear_current(void);
void handle_clear_all(void);
void handle_navigate_prev(void);
void handle_navigate_next(void);
void handle_sign_toggle(void);
void handle_confirm(void);
void handle_cancel(void);

int main(void)
{
    clock_init(SYSTEM_CLOCK_144M);

    USART1_Init();
    USART1_SendString("\r\n=== Function Plotter Started ===\r\n");

    debug_init();

    ssd1306_Init(SSD1306_SWITCHCAPVCC);
    ssd1306_clearScreen();
    oled_setTextSize(2);
    oled_drawText(0, 0, "Loading...");
    ssd1306_updateScreen();

    button_init();
    button_set_callback(button_callback);

    timer_init(TIM_6, TIMER_MS);
    timer_start(TIM_6);

    matrix_keyboard_init(10);
    matrix_keyboard_set_callback(matrix_key_event_handler);
    matrix_keyboard_set_enabled(0);

    app_state_init();
    display_function_select();

    last_input_time = 0;

    while(1) {
        system_ms = timer_get(TIM_6);
        matrix_keyboard_scanner();
        button_is_pressed();

        if(app_state_get_current() == STATE_INPUT_COEFFICIENTS) {
            cursor_blink_timer++;
            if(cursor_blink_timer >= 50) {
                cursor_blink_timer = 0;
                cursor_blink_state = !cursor_blink_state;
                update_display();
            }

            uint32_t current_time = last_input_time + 50;
            if(last_input_time > 0 && (current_time - last_input_time) > COEF_INPUT_TIMEOUT_MS) {
                app_state_set_next(STATE_DISPLAY);
                function_plot_custom(coef_input_get_context()->func_type, coef_input_get_context()->coef);
                USART1_SendString("\r\nTimeout - displaying graph\r\n");
            }
        }
    }
}

void button_callback(button_event_t event)
{
    app_state_t state = app_state_get_current();

    if(event == BUTTON_EVENT_SHORT_PRESS) {
        USART1_Printf("\r\n[Event] SHORT_PRESS, state=%d\r\n", state);

        if(state == STATE_IDLE || state == STATE_DISPLAY) {
            app_state_cycle_function();
            app_state_set_next(STATE_SELECT_FUNCTION);
            display_function_select();
        } else if(state == STATE_SELECT_FUNCTION) {
            app_state_cycle_function();
            display_function_select();
        } else if(state == STATE_INPUT_COEFFICIENTS) {
            handle_navigate_next();
        }
    }
    else if(event == BUTTON_EVENT_LONG_PRESS_START) {
        USART1_Printf("\r\n[Event] LONG_PRESS, state=%d\r\n", state);

        if(state == STATE_SELECT_FUNCTION) {
            coef_input_reset();
            app_state_set_next(STATE_INPUT_COEFFICIENTS);
            matrix_keyboard_set_enabled(1);
            last_input_time = 0;
            update_display();
            USART1_SendString("\r\nEntered coefficient input mode\r\n");
        } else if(state == STATE_INPUT_COEFFICIENTS) {
            handle_confirm();
        } else if(state == STATE_IDLE || state == STATE_DISPLAY) {
            coef_input_reset();
            coef_input_set_coef(0, 1.0f);
            app_state_set_next(STATE_INPUT_COEFFICIENTS);
            matrix_keyboard_set_enabled(1);
            last_input_time = 0;
            update_display();
        }
    }
}

void matrix_key_event_handler(uint8_t key_index, matrix_key_event_enum event, uint8_t key_value)
{
    app_state_t state = app_state_get_current();

    if(state != STATE_INPUT_COEFFICIENTS) return;
    if(event != MATRIX_EVENT_SHORT_PRESS && event != MATRIX_EVENT_LONG_PRESS) return;

    last_input_time = 0;

    if(event == MATRIX_EVENT_LONG_PRESS) {
        switch(key_value) {
            case 'A': handle_sign_toggle(); break;
            case 'C': handle_clear_all(); break;
            case 'D': handle_cancel(); break;
            case 'G': handle_confirm(); break;
            default: break;
        }
    }
    else if(event == MATRIX_EVENT_SHORT_PRESS) {
        switch(key_index) {
            case 1: case 2: case 3:
            case 4: case 5: case 6:
            case 7: case 8: case 9:
            case 13: handle_digit_input(key_index); break;
            case 10: handle_digit_input(0); break;
            case 11: handle_decimal_input(); break;
            case 12: handle_backspace(); break;
            case 14: handle_navigate_prev(); break;
            case 15: handle_navigate_next(); break;
            case 16: handle_clear_current(); break;
            default: break;
        }
    }
}

void update_display(void)
{
    app_state_t state = app_state_get_current();

    switch(state) {
        case STATE_SELECT_FUNCTION:
        case STATE_INPUT_COEFFICIENTS:
            display_coef_input();
            break;
        case STATE_DISPLAY:
        case STATE_IDLE:
        default:
            break;
    }
}

void display_function_select(void)
{
    coef_input_context_t *ctx = coef_input_get_context();

    ssd1306_clearScreen();

    oled_setTextSize(1);
    const char *func_name = app_state_get_func_name(ctx->func_type);
    oled_drawText(0, 0, func_name);

    const char *expr = function_get_expression(ctx->func_type);
    oled_drawText(0, 10, expr);

    oled_drawText(0, 56, "Hold PE8 to input coef");
    ssd1306_updateScreen();
}

void display_coef_input(void)
{
    coef_input_context_t *ctx = coef_input_get_context();
    uint8_t count = ctx->coef_count;
    const char *coef_names = "abcd";

    ssd1306_clearScreen();
    oled_setTextSize(1);

    // 第1行：显示函数表达式
    const char *expr = function_get_expression(ctx->func_type);
    oled_drawText(0, 0, expr);

    // 第2行起：每行一个系数
    uint8_t y_pos = 12;
    for(uint8_t i = 0; i < count; i++) {
        char line_buf[16];
        char val_buf[12];

        // 获取系数值
        if(ctx->input_buffer[i][0] != '\0') {
            snprintf(val_buf, sizeof(val_buf), "%s", ctx->input_buffer[i]);
        } else {
            snprintf(val_buf, sizeof(val_buf), "%.2f", ctx->coef[i]);
        }

        // 格式化系数行
        snprintf(line_buf, sizeof(line_buf), "%c=%s", coef_names[i], val_buf);

        // 当前系数前加 > 指示符
        if(i == ctx->current_coef_index) {
            oled_drawChar(0, y_pos, '>', WHITE, BLACK, 1);
            oled_drawText(8, y_pos, line_buf);
        } else {
            oled_drawText(0, y_pos, " ");
            oled_drawText(8, y_pos, line_buf);
        }

        y_pos += 10;
    }

    ssd1306_updateScreen();
}

void display_graph(void)
{
    coef_input_context_t *ctx = coef_input_get_context();
    function_plot_custom(ctx->func_type, ctx->coef);

    char buf[64];
    snprintf(buf, sizeof(buf), "%s: a=%.2f", app_state_get_func_name(ctx->func_type), ctx->coef[0]);
    USART1_Printf("\r\n%s\r\n", buf);
}

float parse_coefficient(const char *str, int8_t sign)
{
    if(str == NULL || str[0] == '\0') return 0.0f;

    float result = 0.0f;
    float decimal = 0.0f;
    uint8_t decimal_place = 0;
    uint8_t in_decimal = 0;
    int8_t str_sign = 1;

    if(str[0] == '-') {
        str_sign = -1;
        str++;
    }

    while(*str) {
        if(*str >= '0' && *str <= '9') {
            if(in_decimal) {
                decimal_place++;
                float digit = (*str - '0') / powf(10.0f, decimal_place);
                decimal += digit;
            } else {
                result = result * 10.0f + (*str - '0');
            }
        } else if(*str == '.' && !in_decimal) {
            in_decimal = 1;
        }
        str++;
    }

    result = result + decimal;
    return (float)sign * str_sign * result;
}

void handle_digit_input(uint8_t digit)
{
    coef_input_context_t *ctx = coef_input_get_context();
    uint8_t idx = ctx->current_coef_index;
    uint8_t pos = ctx->cursor_pos;

    if(pos < 10) {
        ctx->input_buffer[idx][pos] = '0' + digit;
        ctx->input_buffer[idx][pos + 1] = '\0';
        ctx->cursor_pos++;
        update_display();
    }
}

void handle_decimal_input(void)
{
    coef_input_context_t *ctx = coef_input_get_context();
    uint8_t idx = ctx->current_coef_index;

    if(!ctx->has_decimal && ctx->cursor_pos > 0 && ctx->cursor_pos < 9) {
        ctx->input_buffer[idx][ctx->cursor_pos] = '.';
        ctx->input_buffer[idx][ctx->cursor_pos + 1] = '\0';
        ctx->cursor_pos++;
        ctx->has_decimal = 1;
        update_display();
    }
}

void handle_backspace(void)
{
    coef_input_context_t *ctx = coef_input_get_context();
    uint8_t idx = ctx->current_coef_index;

    if(ctx->cursor_pos > 0) {
        ctx->cursor_pos--;
        if(ctx->input_buffer[idx][ctx->cursor_pos] == '.') {
            ctx->has_decimal = 0;
        }
        ctx->input_buffer[idx][ctx->cursor_pos] = '\0';
        update_display();
    }
}

void handle_clear_current(void)
{
    coef_input_context_t *ctx = coef_input_get_context();
    uint8_t idx = ctx->current_coef_index;

    ctx->input_buffer[idx][0] = '\0';
    ctx->cursor_pos = 0;
    ctx->has_decimal = 0;
    ctx->sign = 1;
    update_display();
}

void handle_clear_all(void)
{
    coef_input_reset();
    update_display();
}

void handle_navigate_prev(void)
{
    coef_input_context_t *ctx = coef_input_get_context();

    if(ctx->current_coef_index > 0) {
        ctx->current_coef_index--;
        ctx->cursor_pos = 0;
        ctx->has_decimal = 0;
        ctx->sign = 1;
        update_display();
    }
}

void handle_navigate_next(void)
{
    coef_input_context_t *ctx = coef_input_get_context();

    ctx->current_coef_index++;
    if(ctx->current_coef_index >= ctx->coef_count) {
        ctx->current_coef_index = 0;
    }
    ctx->cursor_pos = 0;
    ctx->has_decimal = 0;
    ctx->sign = 1;
    update_display();
}

void handle_sign_toggle(void)
{
    coef_input_context_t *ctx = coef_input_get_context();
    uint8_t idx = ctx->current_coef_index;

    if(ctx->cursor_pos > 0) {
        if(ctx->input_buffer[idx][0] == '-') {
            for(uint8_t i = 0; i < ctx->cursor_pos - 1; i++) {
                ctx->input_buffer[idx][i] = ctx->input_buffer[idx][i + 1];
            }
            ctx->input_buffer[idx][ctx->cursor_pos - 1] = '\0';
            ctx->cursor_pos--;
        } else {
            for(uint8_t i = ctx->cursor_pos; i > 0; i--) {
                ctx->input_buffer[idx][i] = ctx->input_buffer[idx][i - 1];
            }
            ctx->input_buffer[idx][0] = '-';
            ctx->input_buffer[idx][ctx->cursor_pos + 1] = '\0';
            ctx->cursor_pos++;
        }
        update_display();
    }
}

void handle_confirm(void)
{
    coef_input_context_t *ctx = coef_input_get_context();

    for(uint8_t i = 0; i < ctx->coef_count; i++) {
        if(ctx->input_buffer[i][0] != '\0') {
            ctx->coef[i] = parse_coefficient(ctx->input_buffer[i], 1);
        }
    }

    matrix_keyboard_set_enabled(0);
    app_state_set_next(STATE_DISPLAY);
    display_graph();
    USART1_Printf("\r\nConfirmed and displaying graph\r\n");
}

void handle_cancel(void)
{
    coef_input_reset();
    matrix_keyboard_set_enabled(0);
    app_state_set_next(STATE_IDLE);

    coef_input_context_t *ctx = coef_input_get_context();
    function_plot_custom(ctx->func_type, ctx->coef);
    USART1_SendString("\r\nCancelled - displaying graph\r\n");
}