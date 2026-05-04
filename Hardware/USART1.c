/**
 * USART1 串口驱动实现
 * 引脚: PA9=TX, PA10=RX
 * 波特率: 115200
 * 支持日志级别: ERROR, INFO, DEBUG
 */

#include "USART1.h"
#include <string.h>
#include <stdio.h>

/* 当前日志级别 */
static LogLevel currentLevel = LOG_LEVEL_DEBUG;

/* 初始化标志 */
static uint8_t isInitialized = 0;

/* 日志级别前缀 */
static const char *levelPrefix[] = {
    "",
    "[ERROR]",
    "[INFO]",
    "[DEBUG]"
};

/**
 * @brief 初始化USART1
 * @param None
 * @retval None
 */
void USART1_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    USART_InitTypeDef USART_InitStructure;
    
    /* 使能GPIOA和USART1时钟 */
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_USART1, ENABLE);
    
    /* 配置PA9为复用推挽输出 (TX) */
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);
    
    /* 配置PA10为浮空输入 (RX) */
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(GPIOA, &GPIO_InitStructure);
    
    /* 配置USART1参数 */
    USART_InitStructure.USART_BaudRate = 115200;
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;
    USART_InitStructure.USART_StopBits = USART_StopBits_1;
    USART_InitStructure.USART_Parity = USART_Parity_No;
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
    USART_Init(USART1, &USART_InitStructure);
    
    /* 使能USART1 */
    USART_Cmd(USART1, ENABLE);

    isInitialized = 1;
}

/**
 * @brief 发送单个字符
 * @param ch: 要发送的字符
 * @retval None
 */
void USART1_SendChar(uint8_t ch)
{
    if (!isInitialized) return;
    while (USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET);
    USART_SendData(USART1, ch);
    while (USART_GetFlagStatus(USART1, USART_FLAG_TC) == RESET);
}

/**
 * @brief 发送字符串
 * @param str: 要发送的字符串
 * @retval None
 */
void USART1_SendString(const char *str)
{
    while (*str)
    {
        USART1_SendChar(*str++);
    }
}

/**
 * @brief 格式化输出
 * @param fmt: 格式字符串
 * @param ...: 可变参数
 * @retval None
 */
void USART1_Printf(const char *fmt, ...)
{
    char buffer[256];
    va_list args;
    
    va_start(args, fmt);
    vsprintf(buffer, fmt, args);
    va_end(args);
    
    USART1_SendString(buffer);
}

/**
 * @brief 设置日志级别
 * @param level: 日志级别
 * @retval None
 */
void Set_Level(LogLevel level)
{
    if (level >= LOG_LEVEL_NONE && level <= LOG_LEVEL_DEBUG)
    {
        currentLevel = level;
    }
}

/**
 * @brief 输出INFO级别日志
 * @param fmt: 格式字符串
 * @param ...: 可变参数
 * @retval None
 */
void Info(const char *fmt, ...)
{
    if (currentLevel >= LOG_LEVEL_INFO)
    {
        char buffer[256];
        va_list args;
        
        USART1_SendString(levelPrefix[LOG_LEVEL_INFO]);
        USART1_SendString(" ");
        
        va_start(args, fmt);
        vsprintf(buffer, fmt, args);
        va_end(args);
        
        USART1_SendString(buffer);
    }
}

/**
 * @brief 输出DEBUG级别日志
 * @param fmt: 格式字符串
 * @param ...: 可变参数
 * @retval None
 */
void Debug(const char *fmt, ...)
{
    if (currentLevel >= LOG_LEVEL_DEBUG)
    {
        char buffer[256];
        va_list args;
        
        USART1_SendString(levelPrefix[LOG_LEVEL_DEBUG]);
        USART1_SendString(" ");
        
        va_start(args, fmt);
        vsprintf(buffer, fmt, args);
        va_end(args);
        
        USART1_SendString(buffer);
    }
}

/**
 * @brief 输出ERROR级别日志
 * @param fmt: 格式字符串
 * @param ...: 可变参数
 * @retval None
 */
void Error(const char *fmt, ...)
{
    if (currentLevel >= LOG_LEVEL_ERROR)
    {
        char buffer[256];
        va_list args;
        
        USART1_SendString(levelPrefix[LOG_LEVEL_ERROR]);
        USART1_SendString(" ");
        
        va_start(args, fmt);
        vsprintf(buffer, fmt, args);
        va_end(args);
        
        USART1_SendString(buffer);
    }
}
