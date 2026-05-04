/**
 * USART1 串口驱动 - 带日志级别功能
 * 引脚: PA9=TX, PA10=RX
 * 波特率: 115200
 */

#ifndef __USART1_H
#define __USART1_H

#include "stm32f10x.h"
#include <stdarg.h>

/* 日志级别定义 */
typedef enum {
    LOG_LEVEL_NONE = 0,
    LOG_LEVEL_ERROR,
    LOG_LEVEL_INFO,
    LOG_LEVEL_DEBUG
} LogLevel;

/* 函数声明 */
void USART1_Init(void);
void USART1_SendChar(uint8_t ch);
void USART1_SendString(const char *str);
void USART1_Printf(const char *fmt, ...);

/* 日志级别设置 */
void Set_Level(LogLevel level);

/* 带级别的日志输出 */
void Info(const char *fmt, ...);
void Debug(const char *fmt, ...);
void Error(const char *fmt, ...);

#endif
