#include "app_state.h"

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
        coef_ctx.coef[i] = 0.0f;
        coef_ctx.input_buffer[i][0] = '\0';
    }
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