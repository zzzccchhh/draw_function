#include "button.h"
#include "zf_driver_exti.h"
#include "zf_driver_gpio.h"

static volatile button_state_t button_state = BUTTON_STATE_IDLE;
static volatile uint8_t button_pressed_flag = 0;

void button_init(void)
{
    exti_init(E8, EXTI_TRIGGER_FALLING);
}

uint8_t button_is_pressed(void)
{
    if(button_state == BUTTON_STATE_IDLE) {
        if(button_pressed_flag) {
            button_pressed_flag = 0;
            button_state = BUTTON_STATE_DEBOUNCE;
        }
    }

    if(button_state == BUTTON_STATE_DEBOUNCE) {
        if(!gpio_get_level(E8)) {
            button_state = BUTTON_STATE_PRESSED;
            return 1;
        } else {
            button_state = BUTTON_STATE_IDLE;
        }
    }

    if(button_state == BUTTON_STATE_PRESSED) {
        if(gpio_get_level(E8)) {
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
    }
}
