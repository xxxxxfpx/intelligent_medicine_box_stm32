/**
 * DS1302 RTC驱动
 * 引脚定义: PB12=RST, PB13=DAT, PB14=CLK
 * 严格按照DS1302数据手册时序编写
 */

#include "ds1302.h"
#include "Delay.h"

void DS1302_GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);

    GPIO_InitStructure.GPIO_Pin = DS1302_RST_PIN | DS1302_CLK_PIN | DS1302_DAT_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(DS1302_PORT, &GPIO_InitStructure);

    DS1302_RST_Clr();
    DS1302_CLK_Clr();
    DS1302_DAT_Clr();
}

void DS1302_Init(void)
{
    DS1302_GPIO_Init();
    Delay_ms(10);
}

void DS1302_WriteByte(uint8_t dat)
{
    uint8_t i;
    
    for(i = 0; i < 8; i++)
    {
        DS1302_CLK_Clr();
        Delay_us(2);
        if(dat & (0x01 << i))
        {
            DS1302_DAT_Set();
        }
        else
        {
            DS1302_DAT_Clr();
        }
        Delay_us(2);
        DS1302_CLK_Set();
        Delay_us(2);
    }
    DS1302_CLK_Clr();
}

uint8_t DS1302_ReadByte(void)
{
    uint8_t i, dat = 0;
    GPIO_InitTypeDef GPIO_InitStructure;

    GPIO_InitStructure.GPIO_Pin = DS1302_DAT_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(DS1302_PORT, &GPIO_InitStructure);
    Delay_us(2);

    for(i = 0; i < 8; i++)
    {
        DS1302_CLK_Set();
        Delay_us(2);
        DS1302_CLK_Clr();
        Delay_us(2);
        if(GPIO_ReadInputDataBit(DS1302_PORT, DS1302_DAT_PIN))
        {
            dat |= (0x01 << i);
        }
    }

    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_Init(DS1302_PORT, &GPIO_InitStructure);
    DS1302_DAT_Clr();
    
    return dat;
}

void DS1302_WriteReg(uint8_t reg, uint8_t dat)
{
    uint8_t i;
    
    DS1302_RST_Clr();
    DS1302_CLK_Clr();
    Delay_us(4);
    DS1302_RST_Set();
    Delay_us(4);
    
    for(i = 0; i < 8; i++)
    {
        DS1302_CLK_Clr();
        Delay_us(2);
        if(reg & (0x01 << i))
        {
            DS1302_DAT_Set();
        }
        else
        {
            DS1302_DAT_Clr();
        }
        Delay_us(2);
        DS1302_CLK_Set();
        Delay_us(2);
    }
    DS1302_CLK_Clr();
    Delay_us(4);
    
    for(i = 0; i < 8; i++)
    {
        DS1302_CLK_Clr();
        Delay_us(2);
        if(dat & (0x01 << i))
        {
            DS1302_DAT_Set();
        }
        else
        {
            DS1302_DAT_Clr();
        }
        Delay_us(2);
        DS1302_CLK_Set();
        Delay_us(2);
    }
    DS1302_CLK_Clr();
    
    DS1302_RST_Clr();
    Delay_us(4);
}

uint8_t DS1302_ReadReg(uint8_t reg)
{
    uint8_t i, dat = 0;
    GPIO_InitTypeDef GPIO_InitStructure;
    
    reg |= 0x01;
    
    DS1302_RST_Clr();
    DS1302_CLK_Clr();
    Delay_us(4);
    DS1302_RST_Set();
    Delay_us(4);
    
    for(i = 0; i < 8; i++)
    {
        DS1302_CLK_Clr();
        Delay_us(2);
        if(reg & (0x01 << i))
        {
            DS1302_DAT_Set();
        }
        else
        {
            DS1302_DAT_Clr();
        }
        Delay_us(2);
        DS1302_CLK_Set();
        Delay_us(2);
    }
    DS1302_CLK_Clr();
    Delay_us(4);
    
    GPIO_InitStructure.GPIO_Pin = DS1302_DAT_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(DS1302_PORT, &GPIO_InitStructure);
    Delay_us(2);

    for(i = 0; i < 8; i++)
    {
        DS1302_CLK_Set();
        Delay_us(2);
        DS1302_CLK_Clr();
        Delay_us(2);
        if(GPIO_ReadInputDataBit(DS1302_PORT, DS1302_DAT_PIN))
        {
            dat |= (0x01 << i);
        }
    }

    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_Init(DS1302_PORT, &GPIO_InitStructure);
    DS1302_DAT_Clr();
    
    DS1302_RST_Clr();
    DS1302_CLK_Clr();
    
    return dat;
}

uint8_t DS1302_ConvertFromBcd(uint8_t bcd)
{
    return (bcd / 16 * 10 + bcd % 16);
}

uint8_t DS1302_ConvertToBcd(uint8_t dat)
{
    return (dat / 10 * 16 + dat % 10);
}

void DS1302_SetTime(DS1302_TimeTypeDef* time)
{
    DS1302_WriteReg(0x8E, 0x00);
    Delay_ms(1);

    DS1302_WriteReg(0x80, DS1302_ConvertToBcd(time->second) & 0x7F);
    DS1302_WriteReg(0x82, DS1302_ConvertToBcd(time->minute));
    DS1302_WriteReg(0x84, DS1302_ConvertToBcd(time->hour));
    DS1302_WriteReg(0x86, DS1302_ConvertToBcd(time->day));
    DS1302_WriteReg(0x88, DS1302_ConvertToBcd(time->month));
    DS1302_WriteReg(0x8A, DS1302_ConvertToBcd(time->week));
    DS1302_WriteReg(0x8C, DS1302_ConvertToBcd(time->year));
    Delay_ms(1);

    DS1302_WriteReg(0x8E, 0x80);
}

void DS1302_GetTime(DS1302_TimeTypeDef* time)
{
    uint8_t second, minute, hour, day, month, week, year;

    second = DS1302_ReadReg(0x80);
    minute = DS1302_ReadReg(0x82);
    hour   = DS1302_ReadReg(0x84);
    day    = DS1302_ReadReg(0x86);
    month  = DS1302_ReadReg(0x88);
    week   = DS1302_ReadReg(0x8A);
    year   = DS1302_ReadReg(0x8C);

    time->second = DS1302_ConvertFromBcd(second & 0x7F);
    time->minute = DS1302_ConvertFromBcd(minute & 0x7F);
    time->hour   = DS1302_ConvertFromBcd(hour & 0x3F);
    time->day    = DS1302_ConvertFromBcd(day & 0x3F);
    time->month  = DS1302_ConvertFromBcd(month & 0x1F);
    time->week   = DS1302_ConvertFromBcd(week & 0x07);
    time->year   = DS1302_ConvertFromBcd(year);
}

void DS1302_SetProtect(uint8_t protect)
{
    if(protect)
        DS1302_WriteReg(0x8E, 0x80);
    else
        DS1302_WriteReg(0x8E, 0x00);
}

void DS1302_StartClock(void)
{
    uint8_t sec;
    DS1302_WriteReg(0x8E, 0x00);
    Delay_ms(1);
    sec = DS1302_ReadReg(0x81);
    DS1302_WriteReg(0x80, sec & 0x7F);
    Delay_ms(1);
    DS1302_WriteReg(0x8E, 0x80);
}
