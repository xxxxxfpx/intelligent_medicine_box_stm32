/**
 * GPS模块驱动 - ATGM336H-5N
 * 使用USART2接收GPS数据 (PA3=RX, PA2=TX)
 * 波特率: 9600, 8N1
 * 解析NMEA $GPRMC/$GNRMC语句
 */

#ifndef __GPS_H
#define __GPS_H

#include "stm32f10x.h"

#define GPS_BUFFER_LENGTH   100
#define GPS_TIME_LENGTH     11
#define GPS_LAT_LENGTH      12
#define GPS_LON_LENGTH      13
#define GPS_NS_LENGTH       2
#define GPS_EW_LENGTH       2
#define GPS_DATE_LENGTH     7

typedef struct {
    char UTCTime[GPS_TIME_LENGTH];
    char date[GPS_DATE_LENGTH];
    char latitude[GPS_LAT_LENGTH];
    char longitude[GPS_LON_LENGTH];
    char N_S[GPS_NS_LENGTH];
    char E_W[GPS_EW_LENGTH];
    uint8_t isValid;
    uint8_t isUpdated;
    uint16_t rxCount;
    uint16_t frameCount;
} GPS_InfoTypeDef;

void GPS_Init(void);
void GPS_Parse(void);
GPS_InfoTypeDef* GPS_GetInfo(void);
void GPS_Clear(void);
uint16_t GPS_GetRxCount(void);

#endif
