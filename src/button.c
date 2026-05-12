#include "button.h"
#include "zf_driver_exti.h"
#include "zf_driver_gpio.h"
#include "zf_common_headfile.h"

static volatile button_state_t button_state = BUTTON_STATE_IDLE;
static volatile uint8_t button_pressed_flag = 0;
static volatile uint8_t long_press_triggered = 0;
static volatile uint8_t debounce_counter = 0;
static volatile uint32_t press_start_tick = 0;
static button_callback_t user_callback = 0;

#define BUTTON_LONG_PRESS_MS     1000
#define BUTTON_DEBOUNCE_COUNT    5

void button_init(void)
{
    exti_init(E8, EXTI_TRIGGER_FALLING);
}

void button_set_callback(button_callback_t callback)
{
    user_callback = callback;
}

uint32_t button_get_press_duration_ms(void)
{
    if(button_state == BUTTON_STATE_PRESSED || button_state == BUTTON_STATE_LONG_PRESS_DETECT) {
        return system_ms - press_start_tick;
    }
    return 0;
}

uint8_t button_is_pressed(void)
{
    uint8_t current_level = gpio_get_level(E8);

    if(button_state == BUTTON_STATE_IDLE) {
        if(button_pressed_flag) {
            button_pressed_flag = 0;
            debounce_counter = 0;
            button_state = BUTTON_STATE_DEBOUNCE;
        }
    }

    if(button_state == BUTTON_STATE_DEBOUNCE) {
        if(current_level == 0) {
            debounce_counter++;
            if(debounce_counter >= BUTTON_DEBOUNCE_COUNT) {
                button_state = BUTTON_STATE_PRESSED;
                press_start_tick = system_ms;
                long_press_triggered = 0;
            }
        } else {
            button_state = BUTTON_STATE_IDLE;
        }
    }

    if(button_state == BUTTON_STATE_PRESSED) {
        uint32_t press_duration = system_ms - press_start_tick;

        if(current_level) {
            if(long_press_triggered) {
                long_press_triggered = 0;
            } else {
                if(user_callback) {
                    user_callback(BUTTON_EVENT_SHORT_PRESS);
                }
            }
            button_state = BUTTON_STATE_IDLE;
        } else {
            if(press_duration >= BUTTON_LONG_PRESS_MS && !long_press_triggered) {
                long_press_triggered = 1;
                button_state = BUTTON_STATE_LONG_PRESS_DETECT;
                if(user_callback) {
                    user_callback(BUTTON_EVENT_LONG_PRESS_START);
                }
            }
        }
    }

    if(button_state == BUTTON_STATE_LONG_PRESS_DETECT) {
        if(current_level) {
            button_state = BUTTON_STATE_IDLE;
        }
    }

    return 0;
}

uint8_t button_get_state(void)
{
    return (uint8_t)button_state;
}

void button_clear_flag(void)
{
    if(button_state == BUTTON_STATE_IDLE) {
        button_pressed_flag = 1;
        debounce_counter = 0;
    }
}
