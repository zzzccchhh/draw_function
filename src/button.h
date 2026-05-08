#ifndef __BUTTON_H
#define __BUTTON_H

#include <stdint.h>

void button_init(void);
uint8_t button_is_pressed(void);
void button_clear_flag(void);

#endif
