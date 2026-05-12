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
void update_display(void);
void display_function_select(void);
void display_coef_input(void);
void display_graph(void);

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

    app_state_init();
    display_function_select();

    last_input_time = 0;

    while(1) {
        system_ms = timer_get(TIM_6);
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
        }
    }
    else if(event == BUTTON_EVENT_LONG_PRESS_START) {
        USART1_Printf("\r\n[Event] LONG_PRESS, state=%d\r\n", state);

        if(state == STATE_SELECT_FUNCTION) {
            coef_input_reset();
            app_state_set_next(STATE_INPUT_COEFFICIENTS);
            last_input_time = 0;
            update_display();
            USART1_SendString("\r\nEntered coefficient input mode\r\n");
        } else if(state == STATE_INPUT_COEFFICIENTS) {
            // Long press to confirm - for future use
        } else if(state == STATE_IDLE || state == STATE_DISPLAY) {
            coef_input_reset();
            coef_input_set_coef(0, 1.0f);
            app_state_set_next(STATE_INPUT_COEFFICIENTS);
            last_input_time = 0;
            update_display();
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