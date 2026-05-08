#include "oled.h"
#include "oledfont.h"

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


uint8_t OLED_GRAM[128][8];

// SPI1��ʼ������
void SPI1_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct;
    SPI_InitTypeDef SPI_InitStruct;
    
    // ����ʱ��
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOE | RCC_APB2Periph_SPI1, ENABLE);
    
    // ����SPI1����
    // PA5: SCK, PA7: MOSI, PB7: CS, PB6: DC, PB5: RST
    GPIO_InitStruct.GPIO_Pin = OLED_SCL_GPIO_PIN | OLED_SDA_GPIO_PIN;  // SCK��MOSI
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_AF_PP;  // �����������
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStruct);
    
    // ����CS��DC��RST����Ϊ��ͨ�������
    GPIO_InitStruct.GPIO_Pin = OLED_CS_GPIO_PIN | OLED_DC_GPIO_PIN | OLED_RST_GPIO_PIN;
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_Init(GPIOE, &GPIO_InitStruct);
    
    // ����SPI1����
    SPI_InitStruct.SPI_Direction = SPI_Direction_1Line_Tx;  // ������
    SPI_InitStruct.SPI_Mode = SPI_Mode_Master;  // ����ģʽ
    SPI_InitStruct.SPI_DataSize = SPI_DataSize_8b;  // 8λ����
    SPI_InitStruct.SPI_CPOL = SPI_CPOL_Low;  // ʱ�ӿ��е͵�ƽ
    SPI_InitStruct.SPI_CPHA = SPI_CPHA_1Edge;  // ��һ�����ز���
    SPI_InitStruct.SPI_NSS = SPI_NSS_Soft;  // ��������NSS
    SPI_InitStruct.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_8;  // ������Ԥ��Ƶ
    SPI_InitStruct.SPI_FirstBit = SPI_FirstBit_MSB;  // ��λ�ȴ�
    SPI_InitStruct.SPI_CRCPolynomial = 7;
    
    SPI_Init(SPI1, &SPI_InitStruct);
    
    // ʹ��SPI1
    SPI_Cmd(SPI1, ENABLE);
}

// д����/���� - Ӳ��SPIʵ��
void OLED_WR_Byte(uint8_t dat, uint8_t cmd)
{
    // ����DC���ţ�����/����ѡ��
    if(cmd == OLED_CMD)
        OLED_DC_Clr();
    else
        OLED_DC_Set();
    
    // Ƭѡʹ��
    OLED_CS_Clr();
    
    // �ȴ�SPI���ͻ�������
    while(SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_TXE) == RESET);
    
    // ��������
    SPI_I2S_SendData(SPI1, dat);
    
    // �ȴ����ݷ������
    while(SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_BSY) == SET);
    
    // Ƭѡ����
    OLED_CS_Set();
    
    // �ָ�DC����
    OLED_DC_Set();
}

// ��ɫ��ʾ
void OLED_ColorTurn(uint8_t i)
{
    if(i == 0)
    {
        OLED_WR_Byte(0xA6, OLED_CMD);
    }
    if(i == 1)
    {
        OLED_WR_Byte(0xA7, OLED_CMD);
    }
}

// ��Ļ��ת
void OLED_DisplayTurn(uint8_t i)
{
    if(i == 0)
    {
        OLED_WR_Byte(0xC8, OLED_CMD);
        OLED_WR_Byte(0xA1, OLED_CMD);
    }
    if(i == 1)
    {
        OLED_WR_Byte(0xC0, OLED_CMD);
        OLED_WR_Byte(0xA0, OLED_CMD);
    }
}

// ������ʾ
void OLED_DisPlay_On(void)
{
    OLED_WR_Byte(0x8D, OLED_CMD);
    OLED_WR_Byte(0x14, OLED_CMD);
    OLED_WR_Byte(0xAF, OLED_CMD);
}

// �ر���ʾ
void OLED_DisPlay_Off(void)
{
    OLED_WR_Byte(0x8D, OLED_CMD);
    OLED_WR_Byte(0x10, OLED_CMD);
    OLED_WR_Byte(0xAE, OLED_CMD);
}

// ˢ���Դ浽��Ļ
void OLED_Refresh(void)
{
    uint8_t i, n;
    for(i = 0; i < 8; i++)
    {
        OLED_WR_Byte(0xb0 + i, OLED_CMD);
        OLED_WR_Byte(0x00, OLED_CMD);
        OLED_WR_Byte(0x10, OLED_CMD);
        for(n = 0; n < 128; n++)
            OLED_WR_Byte(OLED_GRAM[n][i], OLED_DATA);
    }
}

// ����
void OLED_Clear(void)
{
    uint8_t i, n;
    for(i = 0; i < 8; i++)
    {
        for(n = 0; n < 128; n++)
        {
            OLED_GRAM[n][i] = 0;
        }
    }
    OLED_Refresh();
}

// ����
void OLED_DrawPoint(uint8_t x, uint8_t y)
{
    uint8_t i, m, n;
    i = y / 8;
    m = y % 8;
    n = 1 << m;
    OLED_GRAM[x][i] |= n;
}

// �����
void OLED_ClearPoint(uint8_t x, uint8_t y)
{
    uint8_t i, m, n;
    i = y / 8;
    m = y % 8;
    n = 1 << m;
    OLED_GRAM[x][i] &= ~n;
}

// ����
void OLED_DrawLine(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1)
{
    int8_t dx = (int8_t)(x1 > x0 ? x1 - x0 : x0 - x1);
    int8_t dy = (int8_t)(y1 > y0 ? y1 - y0 : y0 - y1);
    int8_t sx = (x0 < x1) ? 1 : -1;
    int8_t sy = (y0 < y1) ? 1 : -1;
    int16_t err = (int16_t)dx + (int16_t)dy;
    int16_t e2;

    while(1) {
        if(x0 < 128 && y0 < 64)
            OLED_DrawPoint(x0, y0);
        if(x0 == x1 && y0 == y1) break;
        e2 = 2 * err;
        if(e2 >= dy) { err += dy; x0 = (uint8_t)((int8_t)x0 + sx); }
        if(e2 <= dx) { err += dx; y0 = (uint8_t)((int8_t)y0 + sy); }
    }
}

// ��ʾ�ַ�
void OLED_ShowChar(uint8_t x, uint8_t y, uint8_t chr, uint8_t size1)
{
    uint8_t i, m, temp, size2, chr1;
    uint8_t y0 = y;
    size2 = (size1 / 8 + ((size1 % 8) ? 1 : 0)) * (size1 / 2);
    chr1 = chr - ' ';

    for(i = 0; i < size2; i++)
    {
        if(size1 == 12)      { temp = asc2_1206[chr1][i]; }
        else if(size1 == 16) { temp = asc2_1608[chr1][i]; }
        else return;

        for(m = 0; m < 8; m++)
        {
            if(temp & 0x80)
                OLED_DrawPoint(x, y);
            else
                OLED_ClearPoint(x, y);
            temp <<= 1;
            y++;
            if((y - y0) == size1)
            {
                y = y0;
                x++;
                break;
            }
        }
    }
    OLED_Refresh();
}

// ��ʾ�ַ���
void OLED_ShowString(uint8_t x, uint8_t y, uint8_t *chr, uint8_t size1)
{
    while((*chr >= ' ') && (*chr <= '~'))
    {
        OLED_ShowChar(x, y, *chr, size1);
        x += size1 / 2;
        if(x > 128 - size1)
        {
            x = 0;
            y += 2;
        }
        chr++;
    }
    OLED_Refresh();
}

// ������
uint32_t OLED_Pow(uint8_t m, uint8_t n)
{
    uint32_t result = 1;
    while(n--)
    {
        result *= m;
    }
    return result;
}

// ��ʾ����
void OLED_ShowNum(uint8_t x,uint8_t y,uint32_t num,uint8_t len,uint8_t size1)
{
    uint8_t t, temp;
    for(t = 0; t < len; t++)
    {
        temp = (num / OLED_Pow(10, len - t - 1)) % 10;
        OLED_ShowChar(x + (size1 / 2)*t, y, temp + '0', size1);
    }
    OLED_Refresh();
}

// ��ʾ16*16����
void OLED_ShowChinese(uint8_t x, uint8_t y, uint8_t no, uint8_t size1)
{
    uint8_t i,j;
    if(size1 == 16)
    {
        for(i = 0; i < 16; i++)
        {
            for(j = 0; j < 8; j++)
            {
                if(Hzk1[no][i] & (0x80 >> j))
                    OLED_DrawPoint(x + j, y + i);
            }
            for(j = 0; j < 8; j++)
            {
                if(Hzk1[no][i + 16] & (0x80 >> j))
                    OLED_DrawPoint(x + j + 8, y + i);
            }
        }
    }
    OLED_Refresh();
}

// OLED��ʼ�� - Ӳ��SPI�汾
void OLED_Init(void)
{
    // ��ʼ��SPI1
    SPI1_Init();
    
    // ��λʱ��
    OLED_RST_Clr();
    system_delay_ms(200);
    OLED_RST_Set();
    system_delay_ms(200);

    // OLED ��ʼ��ָ��
    OLED_WR_Byte(0xAE, OLED_CMD);
    OLED_WR_Byte(0xD5, OLED_CMD);
    OLED_WR_Byte(0x50, OLED_CMD);
    OLED_WR_Byte(0xA8, OLED_CMD);
    OLED_WR_Byte(0x3F, OLED_CMD);
    OLED_WR_Byte(0xD3, OLED_CMD);
    OLED_WR_Byte(0x00, OLED_CMD);
    OLED_WR_Byte(0x40, OLED_CMD);
    OLED_WR_Byte(0x8D, OLED_CMD);
    OLED_WR_Byte(0x14, OLED_CMD);
    OLED_WR_Byte(0x20, OLED_CMD);
    OLED_WR_Byte(0x02, OLED_CMD);
    OLED_WR_Byte(0xA1, OLED_CMD);
    OLED_WR_Byte(0xC8, OLED_CMD);
    OLED_WR_Byte(0xDA, OLED_CMD);
    OLED_WR_Byte(0x12, OLED_CMD);
    OLED_WR_Byte(0x81, OLED_CMD);
    OLED_WR_Byte(0xCF, OLED_CMD);
    OLED_WR_Byte(0xD9, OLED_CMD);
    OLED_WR_Byte(0xF1, OLED_CMD);
    OLED_WR_Byte(0xDB, OLED_CMD);
    OLED_WR_Byte(0x40, OLED_CMD);
    OLED_WR_Byte(0xA4, OLED_CMD);
    OLED_WR_Byte(0xA6, OLED_CMD);
    OLED_WR_Byte(0xAF, OLED_CMD);

    OLED_Clear();
}

// ��ʾͼƬ
void OLED_ShowPicture(uint8_t x, uint8_t y, uint8_t width, uint8_t height, const uint8_t *pic)
{
    uint8_t i, j;
    uint8_t page = height / 8;

    for (j = 0; j < page; j++)
    {
        OLED_WR_Byte(0xb0 + j + y/8, OLED_CMD);
        OLED_WR_Byte(x & 0x0F, OLED_CMD);
        OLED_WR_Byte(0x10 | (x>>4), OLED_CMD);

        for (i = 0; i < width; i++)
        {
            OLED_WR_Byte(pic[j * width + i], OLED_DATA);
        }
    }
}