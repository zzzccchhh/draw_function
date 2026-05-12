#ifndef __BUTTON_H__
#define __BUTTON_H__

#include <stdint.h>

extern volatile uint32_t system_ms;

typedef enum {
    BUTTON_STATE_IDLE = 0,
    BUTTON_STATE_DEBOUNCE,
    BUTTON_STATE_PRESSED,
    BUTTON_STATE_LONG_PRESS_DETECT
} button_state_t;

typedef enum {
    BUTTON_EVENT_NONE = 0,
    BUTTON_EVENT_SHORT_PRESS,
    BUTTON_EVENT_LONG_PRESS_START,
    BUTTON_EVENT_RELEASED
} button_event_t;

typedef void (*button_callback_t)(button_event_t event);

void button_init(void);
uint8_t button_is_pressed(void);
void button_clear_flag(void);
uint8_t button_get_state(void);
void button_set_callback(button_callback_t callback);
uint32_t button_get_press_duration_ms(void);

#endif