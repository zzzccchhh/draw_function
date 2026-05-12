#ifndef __FUNCTION_PLOTTER_H
#define __FUNCTION_PLOTTER_H

#include <stdint.h>

typedef enum {
    FUNC_QUADRATIC,   // y = ax² + bx + c
    FUNC_CUBIC,       // y = ax³ + bx² + cx + d
    FUNC_EXPONENTIAL, // y = a * e^(bx)
    FUNC_LOGARITHM,   // y = a * ln(bx + c)
    FUNC_SINE,        // y = a * sin(bx + c) + d
    FUNC_COSINE,      // y = a * cos(bx + c) + d
    FUNC_TANGENT      // y = a * tan(bx + c) + d
} function_type_t;

typedef struct {
    function_type_t type;
    char name[28];
    float coef[4];
} function_preset_t;

extern const function_preset_t presets[];
extern uint8_t current_index;
extern uint8_t preset_count;

void function_plot(const function_preset_t *preset);
void function_plot_custom(function_type_t type, const float *coef);
void function_send_to_uart(const function_preset_t *preset);
const char* function_get_name(function_type_t type);
const char* function_get_expression(function_type_t type);
uint8_t function_get_coef_count(function_type_t type);

#endif