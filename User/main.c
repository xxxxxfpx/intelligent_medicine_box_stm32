/**
 * 智能药盒项目 - 主程序
 * DS1302 RTC + OLED显示
 * DS1302引脚: PB12=RST, PB13=DAT, PB14=CLK
 * OLED引脚: PB8=SCL, PB9=SDA
 */

#include "stm32f10x.h"
#include "Delay.h"
#include "OLED.h"
#include "DS1302.h"

uint8_t DS1302_TestConnection(void)
{
    uint8_t sec1, sec2;
    
    sec1 = DS1302_ReadReg(0x81);
    Delay_ms(100);
    sec2 = DS1302_ReadReg(0x81);
    
    if(sec1 != sec2)
    {
        return 1;
    }
    
    if((sec1 & 0x7F) != 0xFF && (sec1 & 0x7F) != 0x00)
    {
        return 1;
    }
    
    return 0;
}

int main(void)
{
    DS1302_TimeTypeDef time;
    uint8_t rawSec, rawMin, rawHour;
    uint8_t testResult;
    uint8_t i;
    
    OLED_Init();
    OLED_Clear();
    
    OLED_ShowString(1, 1, "DS1302 Test");
    OLED_ShowString(2, 1, "PB12=RST PB13=DAT");
    OLED_ShowString(3, 1, "PB14=CLK");
    Delay_ms(1000);
    OLED_Clear();
    
    DS1302_Init();
    
    OLED_ShowString(1, 1, "Test Connect...");
    
    testResult = DS1302_TestConnection();

    if(testResult)
    {
        OLED_ShowString(1, 1, "Connect: OK!   ");
    }
    else
    {
        OLED_ShowString(1, 1, "Connect: FAIL!");
        OLED_ShowString(2, 1, "Check wiring!");
        OLED_ShowString(3, 1, "VCC GND CLK DAT RST");
        while(1)
        {
            Delay_ms(1000);
        }
    }
    
    Delay_ms(1000);
    OLED_Clear();
    
    OLED_ShowString(1, 1, "Write RAM Test");
    DS1302_WriteReg(0x8E, 0x00);
    for(i = 0; i < 5; i++)
    {
        DS1302_WriteReg(0xC0 + i*2, 0x10 + i);
    }
    DS1302_WriteReg(0x8E, 0x80);
    
    OLED_ShowString(2, 1, "Read RAM:");
    for(i = 0; i < 5; i++)
    {
        uint8_t val = DS1302_ReadReg(0xC1 + i*2);
        OLED_ShowHexNum(2, 10 + i*3, val, 2);
    }
    Delay_ms(2000);
    OLED_Clear();
    
    OLED_ShowString(1, 1, "Start Clock...");
    DS1302_StartClock();
    OLED_ShowString(1, 1, "Clock Started!");
    Delay_ms(500);
    
    OLED_ShowString(1, 1, "Set Time...");
    time.hour = 12;
    time.minute = 30;
    time.second = 0;
    time.year = 25;
    time.month = 4;
    time.day = 13;
    time.week = 7;
    DS1302_SetTime(&time);
    OLED_ShowString(1, 1, "Set Done!    ");
    Delay_ms(500);
    OLED_Clear();
    
    while(1)
    {
        rawSec = DS1302_ReadReg(0x80);
        rawMin = DS1302_ReadReg(0x82);
        rawHour = DS1302_ReadReg(0x84);
        
        OLED_ShowString(1, 1, "RAW:");
        OLED_ShowHexNum(1, 5, rawSec, 2);
        OLED_ShowChar(1, 7, ' ');
        OLED_ShowHexNum(1, 8, rawMin, 2);
        OLED_ShowChar(1, 10, ' ');
        OLED_ShowHexNum(1, 11, rawHour, 2);
        
        DS1302_GetTime(&time);
        
        OLED_ShowString(2, 1, "Time:");
        OLED_ShowNum(2, 6, time.hour, 2);
        OLED_ShowChar(2, 8, ':');
        OLED_ShowNum(2, 9, time.minute, 2);
        OLED_ShowChar(2, 11, ':');
        OLED_ShowNum(2, 12, time.second, 2);
        
        OLED_ShowString(3, 1, "Date:");
        OLED_ShowNum(3, 6, time.year, 2);
        OLED_ShowChar(3, 8, '-');
        OLED_ShowNum(3, 9, time.month, 2);
        OLED_ShowChar(3, 11, '-');
        OLED_ShowNum(3, 12, time.day, 2);
        
        Delay_ms(500);
    }
}
