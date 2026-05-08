#ifndef __OLED_H
#define __OLED_H

#include "zf_common_headfile.h"
#include <stdio.h>
#include <stdlib.h>

// OLED ���Ŷ���
#define OLED_SCL_GPIO_PORT    GPIOA
#define OLED_SCL_GPIO_PIN     GPIO_Pin_5

#define OLED_SDA_GPIO_PORT    GPIOA
#define OLED_SDA_GPIO_PIN     GPIO_Pin_7

#define OLED_RST_GPIO_PORT    GPIOE
#define OLED_RST_GPIO_PIN     GPIO_Pin_3

#define OLED_DC_GPIO_PORT     GPIOE
#define OLED_DC_GPIO_PIN      GPIO_Pin_2

#define OLED_CS_GPIO_PORT     GPIOE
#define OLED_CS_GPIO_PIN      GPIO_Pin_1

// GPIO ���ƺ�
#define OLED_SCL_Clr()  GPIO_ResetBits(OLED_SCL_GPIO_PORT, OLED_SCL_GPIO_PIN)
#define OLED_SCL_Set()  GPIO_SetBits(OLED_SCL_GPIO_PORT, OLED_SCL_GPIO_PIN)

#define OLED_SDA_Clr()  GPIO_ResetBits(OLED_SDA_GPIO_PORT, OLED_SDA_GPIO_PIN)
#define OLED_SDA_Set()  GPIO_SetBits(OLED_SDA_GPIO_PORT, OLED_SDA_GPIO_PIN)

#define OLED_RST_Clr()  GPIO_ResetBits(OLED_RST_GPIO_PORT, OLED_RST_GPIO_PIN)
#define OLED_RST_Set()  GPIO_SetBits(OLED_RST_GPIO_PORT, OLED_RST_GPIO_PIN)

#define OLED_DC_Clr()   GPIO_ResetBits(OLED_DC_GPIO_PORT, OLED_DC_GPIO_PIN)
#define OLED_DC_Set()   GPIO_SetBits(OLED_DC_GPIO_PORT, OLED_DC_GPIO_PIN)

#define OLED_CS_Clr()   GPIO_ResetBits(OLED_CS_GPIO_PORT, OLED_CS_GPIO_PIN)
#define OLED_CS_Set()   GPIO_SetBits(OLED_CS_GPIO_PORT, OLED_CS_GPIO_PIN)

#define OLED_CMD     0
#define OLED_DATA    1

// OLED �Դ滺����
extern uint8_t OLED_GRAM[128][8];

// 文本像素掩码 - 标记文字绘制区域，曲线经过时清除文字
extern uint8_t text_mask[128][64];

// �ֿ�����
extern const unsigned char ikun[];

// SPI ��ʼ��
void SPI1_Init(void);

// ����д�뺯��
void OLED_WR_Byte(uint8_t dat, uint8_t cmd);

// ��ʾ����
void OLED_ColorTurn(uint8_t i);
void OLED_DisplayTurn(uint8_t i);
void OLED_DisPlay_On(void);
void OLED_DisPlay_Off(void);

// ˢ�������
void OLED_Refresh(void);
void OLED_Clear(void);

// �����
void OLED_DrawPoint(uint8_t x, uint8_t y);
void OLED_ClearPoint(uint8_t x, uint8_t y);
void OLED_DrawLine(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1);

// �ַ���ʾ
void OLED_ShowChar(uint8_t x, uint8_t y, uint8_t chr, uint8_t size1);
void OLED_ShowString(uint8_t x, uint8_t y, uint8_t *chr, uint8_t size1);

// ������ʾ
void OLED_ShowNum(uint8_t x, uint8_t y, uint32_t num, uint8_t len, uint8_t size1);

// ������ʾ
void OLED_ShowChinese(uint8_t x, uint8_t y, uint8_t no, uint8_t size1);

// ͼƬ��ʾ
void OLED_ShowPicture(uint8_t x, uint8_t y, uint8_t width, uint8_t height, const uint8_t *pic);

// ��ʼ��
void OLED_Init(void);

#endif