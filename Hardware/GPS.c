/**
 * GPS模块驱动 - ATGM336H-5N
 * 使用USART2接收GPS数据 (PA3=RX, PA2=TX)
 * 波特率: 9600, 8N1
 * 解析NMEA $GPRMC/$GNRMC语句
 */

#include "GPS.h"
#include "USART1.h"
#include <string.h>

static char gpsRxBuffer[GPS_BUFFER_LENGTH];
static uint8_t gpsRxIndex = 0;
static GPS_InfoTypeDef gpsInfo;
static uint16_t lastLogFrame = 0;

void GPS_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    USART_InitTypeDef USART_InitStructure;
    NVIC_InitTypeDef NVIC_InitStructure;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    USART_InitStructure.USART_BaudRate = 9600;
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;
    USART_InitStructure.USART_StopBits = USART_StopBits_1;
    USART_InitStructure.USART_Parity = USART_Parity_No;
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStructure.USART_Mode = USART_Mode_Rx;
    USART_Init(USART2, &USART_InitStructure);

    NVIC_InitStructure.NVIC_IRQChannel = USART2_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 2;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 2;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

    USART_ITConfig(USART2, USART_IT_RXNE, ENABLE);
    USART_Cmd(USART2, ENABLE);

    GPS_Clear();

    Info("GPS Init OK, Baud:9600\r\n");
}

void GPS_Clear(void)
{
    memset(&gpsInfo, 0, sizeof(gpsInfo));
    memset(gpsRxBuffer, 0, GPS_BUFFER_LENGTH);
    gpsRxIndex = 0;
    lastLogFrame = 0;
}

static uint8_t GPS_IsRMCFrame(void)
{
    if (gpsRxBuffer[0] != '$') return 0;
    if (gpsRxBuffer[1] != 'G') return 0;
    if (gpsRxBuffer[2] != 'P' && gpsRxBuffer[2] != 'N') return 0;
    if (gpsRxBuffer[3] != 'R') return 0;
    if (gpsRxBuffer[4] != 'M') return 0;
    if (gpsRxBuffer[5] != 'C') return 0;
    return 1;
}

static void GPS_ParseRMC(void)
{
    char *p = gpsRxBuffer;
    char *next;
    uint8_t fieldIndex = 0;
    char tempBuf[16];

    while ((next = strchr(p, ',')) != NULL && fieldIndex < 10)
    {
        uint8_t len = next - p;
        if (len > 0 && len < sizeof(tempBuf))
        {
            memcpy(tempBuf, p, len);
            tempBuf[len] = '\0';

            switch (fieldIndex)
            {
                case 1:
                    if (len < GPS_TIME_LENGTH)
                        strcpy(gpsInfo.UTCTime, tempBuf);
                    break;
                case 2:
                    gpsInfo.isValid = (tempBuf[0] == 'A') ? 1 : 0;
                    break;
                case 3:
                    if (len < GPS_LAT_LENGTH)
                        strcpy(gpsInfo.latitude, tempBuf);
                    break;
                case 4:
                    if (len < GPS_NS_LENGTH)
                        strcpy(gpsInfo.N_S, tempBuf);
                    break;
                case 5:
                    if (len < GPS_LON_LENGTH)
                        strcpy(gpsInfo.longitude, tempBuf);
                    break;
                case 6:
                    if (len < GPS_EW_LENGTH)
                        strcpy(gpsInfo.E_W, tempBuf);
                    break;
                case 9:
                    if (len < GPS_DATE_LENGTH)
                        strcpy(gpsInfo.date, tempBuf);
                    break;
            }
        }
        p = next + 1;
        fieldIndex++;
    }

    gpsInfo.isUpdated = 1;

    if(gpsInfo.frameCount - lastLogFrame >= 10)
        {
            lastLogFrame = gpsInfo.frameCount;
            if(gpsInfo.isValid)
            {
                Info("GPS RMC: Date=%s, Time=%s, Lat=%s%s, Lon=%s%s\r\n",
                     gpsInfo.date, gpsInfo.UTCTime, gpsInfo.latitude, gpsInfo.N_S, gpsInfo.longitude, gpsInfo.E_W);
            }
            else
            {
                Info("GPS RMC: Invalid\r\n");
            }
        }
}

void GPS_Parse(void)
{
    if (gpsInfo.isUpdated)
    {
        gpsInfo.isUpdated = 0;
    }
}

GPS_InfoTypeDef* GPS_GetInfo(void)
{
    return &gpsInfo;
}

uint16_t GPS_GetRxCount(void)
{
    return gpsInfo.rxCount;
}

void USART2_IRQHandler(void)
{
    uint8_t data;
    static uint8_t firstData = 1;

    if (USART_GetITStatus(USART2, USART_IT_RXNE) != RESET)
    {
        data = USART_ReceiveData(USART2);
        gpsInfo.rxCount++;

        if(firstData)
        {
            firstData = 0;
            Info("GPS Data RX Started\r\n");
        }

        if (data == '$')
        {
            gpsRxIndex = 0;
            memset(gpsRxBuffer, 0, GPS_BUFFER_LENGTH);
        }

        if (gpsRxIndex < GPS_BUFFER_LENGTH - 1)
        {
            gpsRxBuffer[gpsRxIndex++] = data;
        }

        if (data == '\n' && GPS_IsRMCFrame())
        {
            GPS_ParseRMC();
            gpsInfo.frameCount++;
            gpsRxIndex = 0;
        }
    }
}
