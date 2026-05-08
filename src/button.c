#include "button.h"
#include "zf_driver_exti.h"

static volatile uint8_t button_pressed_flag = 0;

void button_init(void)
{
    exti_init(E8, EXTI_TRIGGER_FALLING);
}

uint8_t button_is_pressed(void)
{
    if(button_pressed_flag) {
        button_pressed_flag = 0;
        return 1;
    }
    return 0;
}

void button_clear_flag(void)
{
    button_pressed_flag = 1;
}
