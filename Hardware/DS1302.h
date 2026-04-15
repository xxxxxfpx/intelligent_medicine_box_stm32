/**
 * DS1302 RTC驱动头文件
 * 引脚定义: PB12=RST, PB13=DAT, PB14=CLK
 */

#ifndef __DS1302_H
#define __DS1302_H

#include "stm32f10x.h"

#define DS1302_RST_PIN    GPIO_Pin_12  // PB12 - 复位/片选
#define DS1302_CLK_PIN    GPIO_Pin_14  // PB14 - 时钟
#define DS1302_DAT_PIN    GPIO_Pin_13  // PB13 - 数据

#define DS1302_PORT       GPIOB

#define DS1302_RST_Clr()  GPIO_ResetBits(DS1302_PORT, DS1302_RST_PIN)
#define DS1302_RST_Set()  GPIO_SetBits(DS1302_PORT, DS1302_RST_PIN)

#define DS1302_CLK_Clr()  GPIO_ResetBits(DS1302_PORT, DS1302_CLK_PIN)
#define DS1302_CLK_Set()  GPIO_SetBits(DS1302_PORT, DS1302_CLK_PIN)

#define DS1302_DAT_Clr()  GPIO_ResetBits(DS1302_PORT, DS1302_DAT_PIN)
#define DS1302_DAT_Set()  GPIO_SetBits(DS1302_PORT, DS1302_DAT_PIN)

typedef struct {
    uint8_t second;
    uint8_t minute;
    uint8_t hour;
    uint8_t day;
    uint8_t month;
    uint8_t week;
    uint8_t year;
} DS1302_TimeTypeDef;

void DS1302_Init(void);
void DS1302_WriteReg(uint8_t reg, uint8_t dat);
uint8_t DS1302_ReadReg(uint8_t reg);
void DS1302_SetTime(DS1302_TimeTypeDef* time);
void DS1302_GetTime(DS1302_TimeTypeDef* time);
void DS1302_SetProtect(uint8_t protect);
void DS1302_StartClock(void);

#endif
