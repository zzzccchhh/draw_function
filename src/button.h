#ifndef __BUTTON_H
#define __BUTTON_H

#include <stdint.h>

typedef enum {
    BUTTON_STATE_IDLE = 0,
    BUTTON_STATE_DEBOUNCE,
    BUTTON_STATE_PRESSED
} button_state_t;

void button_init(void);
uint8_t button_is_pressed(void);
void button_clear_flag(void);
uint8_t button_get_state(void);

#endif
