#ifndef __APP_STATE_H__
#define __APP_STATE_H__

#include <stdint.h>
#include "function_plotter.h"

typedef enum {
    STATE_IDLE,
    STATE_SELECT_FUNCTION,
    STATE_INPUT_COEFFICIENTS,
    STATE_DISPLAY
} app_state_t;

typedef struct {
    function_type_t func_type;
    float coef[4];
    uint8_t current_coef_index;
    uint8_t coef_count;
    char input_buffer[4][12];
    uint8_t cursor_pos;
    int8_t sign;
    uint8_t has_decimal;
    uint8_t display_page;
} coef_input_context_t;

void app_state_init(void);
app_state_t app_state_get_current(void);
void app_state_set_next(app_state_t next);
const char* app_state_get_func_name(function_type_t type);
uint8_t app_state_get_coef_count(function_type_t type);
void app_state_cycle_function(void);
void app_state_set_function(function_type_t type);
void coef_input_reset(void);
void coef_input_set_coef(uint8_t index, float value);
float coef_input_get_coef(uint8_t index);
coef_input_context_t* coef_input_get_context(void);
void coef_input_add_char(char c);
void coef_input_toggle_sign(void);
void coef_input_clear(void);
void coef_input_prev(void);
void coef_input_next(void);
void coef_input_confirm(void);
void coef_input_add_decimal(void);
void coef_input_toggle_display_page(void);

#endif