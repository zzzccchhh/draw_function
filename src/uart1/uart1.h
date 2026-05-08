#ifndef UART1_H
#define UART1_H

#include <stdint.h>

void USART1_Init(void);
void USART1_SendByte(uint8_t data);
void USART1_SendString(char *str);
void USART1_Printf(const char *format, ...);

#endif