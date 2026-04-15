/**
 * DS1302 RTC驱动
 * 引脚定义: PB12=RST, PB13=DAT, PB14=CLK
 * 严格参考51单片机DS1302驱动时序编写
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

void DS1302_WriteReg(uint8_t addr, uint8_t dat)
{
    uint8_t i;

    DS1302_RST_Clr();
    DS1302_CLK_Clr();
    DS1302_RST_Set();

    addr = addr & 0xFE;
    for(i = 0; i < 8; i++)
    {
        if(addr & 0x01)
        {
            DS1302_DAT_Set();
        }
        else
        {
            DS1302_DAT_Clr();
        }
        DS1302_CLK_Set();
        DS1302_CLK_Clr();
        addr = addr >> 1;
    }

    for(i = 0; i < 8; i++)
    {
        if(dat & 0x01)
        {
            DS1302_DAT_Set();
        }
        else
        {
            DS1302_DAT_Clr();
        }
        DS1302_CLK_Set();
        DS1302_CLK_Clr();
        dat = dat >> 1;
    }

    DS1302_RST_Clr();
}

uint8_t DS1302_ReadReg(uint8_t addr)
{
    uint8_t i;
    uint8_t dat = 0;
    GPIO_InitTypeDef GPIO_InitStructure;

    DS1302_RST_Clr();
    DS1302_CLK_Clr();
    DS1302_RST_Set();

    addr = addr | 0x01;
    for(i = 0; i < 8; i++)
    {
        if(addr & 0x01)
        {
            DS1302_DAT_Set();
        }
        else
        {
            DS1302_DAT_Clr();
        }
        DS1302_CLK_Set();
        DS1302_CLK_Clr();
        addr = addr >> 1;
    }

    GPIO_InitStructure.GPIO_Pin = DS1302_DAT_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(DS1302_PORT, &GPIO_InitStructure);

    for(i = 0; i < 8; i++)
    {
        dat = dat >> 1;
        if(GPIO_ReadInputDataBit(DS1302_PORT, DS1302_DAT_PIN))
        {
            dat |= 0x80;
        }
        DS1302_CLK_Set();
        DS1302_CLK_Clr();
    }

    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_Init(DS1302_PORT, &GPIO_InitStructure);
    DS1302_DAT_Clr();

    DS1302_RST_Clr();

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
    DS1302_WriteReg(0x80, 0x80);
    DS1302_WriteReg(0x8C, DS1302_ConvertToBcd(time->year));
    DS1302_WriteReg(0x88, DS1302_ConvertToBcd(time->month));
    DS1302_WriteReg(0x86, DS1302_ConvertToBcd(time->day));
    DS1302_WriteReg(0x84, DS1302_ConvertToBcd(time->hour));
    DS1302_WriteReg(0x82, DS1302_ConvertToBcd(time->minute));
    DS1302_WriteReg(0x80, DS1302_ConvertToBcd(time->second));
    DS1302_WriteReg(0x8A, DS1302_ConvertToBcd(time->week));
    DS1302_WriteReg(0x8E, 0x80);
}

void DS1302_GetTime(DS1302_TimeTypeDef* time)
{
    uint8_t second, minute, hour, day, month, week, year;

    year   = DS1302_ReadReg(0x8C);
    month  = DS1302_ReadReg(0x88);
    day    = DS1302_ReadReg(0x86);
    hour   = DS1302_ReadReg(0x84);
    minute = DS1302_ReadReg(0x82);
    second = DS1302_ReadReg(0x80) & 0x7F;
    week   = DS1302_ReadReg(0x8A);

    time->year   = DS1302_ConvertFromBcd(year);
    time->month  = DS1302_ConvertFromBcd(month);
    time->day    = DS1302_ConvertFromBcd(day);
    time->hour   = DS1302_ConvertFromBcd(hour);
    time->minute = DS1302_ConvertFromBcd(minute);
    time->second = DS1302_ConvertFromBcd(second);
    time->week   = DS1302_ConvertFromBcd(week);
}

void DS1302_StartClock(void)
{
    DS1302_WriteReg(0x8E, 0x00);
    DS1302_WriteReg(0x80, 0x00);
    DS1302_WriteReg(0x8E, 0x80);
}
