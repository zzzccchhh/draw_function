#ifndef __FUNCTION_PLOTTER_H
#define __FUNCTION_PLOTTER_H

#include <stdint.h>

typedef enum {
    FUNC_QUADRATIC,   // y = ax² + bx + c
    FUNC_CUBIC,       // y = ax³ + bx² + cx + d
    FUNC_EXPONENTIAL, // y = a * e^(bx)
    FUNC_LOGARITHM    // y = a * ln(bx + c)
} function_type_t;

typedef struct {
    function_type_t type;
    char name[28];
    float coef[4]; // [a, b, c, d]
} function_preset_t;

extern const function_preset_t presets[];
extern uint8_t current_index;
extern uint8_t preset_count;

void function_plot(const function_preset_t *preset);
void function_send_to_uart(const function_preset_t *preset);

#endif
