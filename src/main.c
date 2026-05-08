/*
 * ch32v307 eide demo
 * version: v1.2
 * Copyright (c) 2022 Taoyukai
 * SPDX-License-Identifier: Apache-2.0
 */

#include "zf_common_headfile.h"
#include "oled.h"
#include "uart1.h"
#include "button.h"
#include "function_plotter.h"

int main(void)
{
    clock_init(SYSTEM_CLOCK_144M);

    USART1_Init();
    USART1_SendString("\r\n=== System Started ===\r\n");

    debug_init();

    OLED_Init();
    OLED_Clear();
    OLED_ShowString(0, 0, (uint8_t*)"Test", 16);
    OLED_Refresh();

    button_init();
    function_plot(&presets[0]);
    function_send_to_uart(&presets[0]);

    while(1) {
        if(button_is_pressed()) {
            current_index = (current_index + 1) % preset_count;
            function_plot(&presets[current_index]);
            function_send_to_uart(&presets[current_index]);
        }
    }
}
