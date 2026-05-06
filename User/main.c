/**
 * 智能药盒项目 - 主程序
 * DS1302 RTC + OLED显示 + GPS定位 + 红外测温 + WiFi
 * DS1302引脚: PB12=RST, PB13=DAT, PB14=CLK
 * OLED引脚: PB8=SCL, PB9=SDA
 * GPS引脚: PA2=TX, PA3=RX (USART2)
 * MLX90614引脚: PB3=SCL, PB4=SDA
 * ESP8266引脚: PB10=TX, PB11=RX (USART3)
 */

#include "stm32f10x.h"
#include "Delay.h"
#include "OLED.h"
#include "chinese_font.h"
#include "DS1302.h"
#include "GPS.h"
#include "MLX90614.h"
#include "ESP8266.h"
#include "Settings.h"
#include "USART1.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static float NMEAToDecimal(const char *nmeaStr, const char *dir)
{
    int degrees;
    float minutes;
    float decimal;
    int len;
    int dotPos = 0;
    int degLen;
    char degStr[4] = {0};
    char minStr[16] = {0};

    if(nmeaStr == NULL || strlen(nmeaStr) == 0) return 0.0f;

    len = strlen(nmeaStr);

    while(dotPos < len && nmeaStr[dotPos] != '.') dotPos++;

    degLen = dotPos - 2;
    if(degLen < 1) degLen = 1;

    strncpy(degStr, nmeaStr, degLen);
    degrees = atoi(degStr);

    strncpy(minStr, nmeaStr + degLen, len - degLen);
    minutes = (float)atof(minStr);

    decimal = degrees + minutes / 60.0f;

    if(dir != NULL && (dir[0] == 'S' || dir[0] == 'W'))
    {
        decimal = -decimal;
    }

    return decimal;
}

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

static uint8_t GPS_SyncDS1302(GPS_InfoTypeDef *gpsInfo)
{
    DS1302_TimeTypeDef dsTime;
    DS1302_TimeTypeDef currentTime;
    uint8_t hour, minute, second;
    uint8_t day, month, year;
    char buf[3];

    if(!gpsInfo->isValid) return 0;
    if(strlen(gpsInfo->UTCTime) < 6) return 0;
    if(strlen(gpsInfo->date) < 6) return 0;

    buf[0] = gpsInfo->UTCTime[0];
    buf[1] = gpsInfo->UTCTime[1];
    buf[2] = '\0';
    hour = atoi(buf);

    buf[0] = gpsInfo->UTCTime[2];
    buf[1] = gpsInfo->UTCTime[3];
    buf[2] = '\0';
    minute = atoi(buf);

    buf[0] = gpsInfo->UTCTime[4];
    buf[1] = gpsInfo->UTCTime[5];
    buf[2] = '\0';
    second = atoi(buf);

    hour += 8;
    if(hour >= 24) hour -= 24;

    buf[0] = gpsInfo->date[0];
    buf[1] = gpsInfo->date[1];
    buf[2] = '\0';
    day = atoi(buf);

    buf[0] = gpsInfo->date[2];
    buf[1] = gpsInfo->date[3];
    buf[2] = '\0';
    month = atoi(buf);

    buf[0] = gpsInfo->date[4];
    buf[1] = gpsInfo->date[5];
    buf[2] = '\0';
    year = atoi(buf);

    dsTime.hour = hour;
    dsTime.minute = minute;
    dsTime.second = second;
    dsTime.day = day;
    dsTime.month = month;
    dsTime.year = year;

    DS1302_GetTime(&currentTime);
    if(currentTime.year == year && currentTime.month == month &&
       currentTime.day == day && currentTime.hour == hour &&
       currentTime.minute == minute)
    {
        return 1;
    }

    DS1302_WriteReg(0x8E, 0x00);
    DS1302_SetTime(&dsTime);
    DS1302_WriteReg(0x8E, 0x80);

    Info("DS1302 Synced from GPS: 20%02d/%02d/%02d %02d:%02d:%02d\r\n",
         year, month, day, hour, minute, second);
    return 1;
}

int main_ok(void)
{
    uint8_t testResult;
    char ipStr[20];
    static uint32_t lastPublishTime = 0;
    float ambientTemp, objectTemp;
    GPS_InfoTypeDef *gpsInfo;
    float latDecimal = 0.0f, lonDecimal = 0.0f;
    uint8_t hasGpsFix = 0;

    Delay_Reset();

    OLED_Init();
    OLED_Clear();

    USART1_Init();
    Set_Level(LOG_LEVEL_DEBUG);
    
    Info("USART1 Ready\r\n");
    
    MLX90614_Init();
    Delay_ms(100);
    
    DS1302_Init();
    DS1302_StartClock();
    
    GPS_Init();
    
    ESP8266_Init();
    
    while(!ESP8266_GetIP(ipStr))
    {
        OLED_ShowString(3, 1, "WiFi:No IP     ");
        Error("WiFi No IP, retrying...\r\n");
        Delay_ms(WIFI_RETRY_INTERVAL_MS);
    }
    OLED_ShowString(3, 1, "WiFi:Connected ");
    OLED_ShowString(4, 1, "IP:            ");
    OLED_ShowString(4, 5, ipStr);
    Info("WiFi Connected, IP: %s\r\n", ipStr);
    Delay_ms(1000);
    
    OLED_ShowString(1, 1, "MQTT Config... ");
    ESP8266_MQTT_Clean();
    Delay_ms(500);
    
    testResult = ESP8266_MQTT_UserCfg(MQTT_CLIENT_ID, MQTT_USERNAME, MQTT_PASSWORD);
    if(!testResult)
    {
        OLED_ShowString(1, 1, "MQTT CFG:FAIL!");
        Error("MQTT Config Fail\r\n");
        while(1) { Delay_ms(1000); }
    }
    else
    {
        OLED_ShowString(2, 1, "Connecting...  ");
        testResult = ESP8266_MQTT_Connect(MQTT_BROKER_HOST, MQTT_BROKER_PORT);
        if(!testResult)
        {
            OLED_ShowString(2, 1, "MQTT Conn:FAIL!");
            Error("MQTT Connect Fail\r\n");
            while(1) { Delay_ms(1000); }
        }
        else
        {
            OLED_ShowString(2, 1, "MQTT:Connected!");
            Info("MQTT Connected, start publishing\r\n");
        }
    }
    
    Delay_ms(1000);
    OLED_ShowString(4, 1, "Start Publish..");
    
    while(1)
    {
        if(ESP8266_MQTT_IsConnected())
        {
            if(lastPublishTime == 0 || (Delay_GetTime() - lastPublishTime) > 5000)
            {
                lastPublishTime = Delay_GetTime();
                
                objectTemp = MLX90614_ReadObjectTemp();
                ambientTemp = MLX90614_ReadAmbientTemp();
                
                gpsInfo = GPS_GetInfo();
                if(gpsInfo->isValid)
                {
                    latDecimal = NMEAToDecimal(gpsInfo->latitude, gpsInfo->N_S);
                    lonDecimal = NMEAToDecimal(gpsInfo->longitude, gpsInfo->E_W);
                    hasGpsFix = 1;
                    Info("GPS Fix: %.6f, %.6f\r\n", latDecimal, lonDecimal);
                    GPS_SyncDS1302(gpsInfo);
                }
                else if(!hasGpsFix)
                {
                    Info("GPS No Fix, using default coords\r\n");
                }
                
                if(objectTemp > -200)
                {
                    Info("Read: Obj=%.1fC, Amb=%.1fC\r\n", objectTemp, ambientTemp);
                    
                    ESP8266_MQTT_PublishTemperature(objectTemp);
                    Delay_ms(200);
                    ESP8266_MQTT_PublishAmbientTemp(ambientTemp);
                    Delay_ms(200);
                    ESP8266_MQTT_PublishGPS(latDecimal, lonDecimal);
                    Delay_ms(200);
                    ESP8266_MQTT_PublishPillboxStatus("normal");
                }
                else
                {
                    Error("MLX90614 Read Error\r\n");
                }
            }
        }
        else
        {
            Error("MQTT Disconnected!\r\n");
            Delay_ms(1000);
        }
        
        Delay_ms(100);
    }
}

int main(void)
{
    uint8_t testResult;
    char ipStr[20];
    static uint32_t lastPublishTime = 0;
    static uint32_t lastPageSwitchTime = 0;
    static uint32_t lastTimeUpdate = 0;
    static uint8_t currentPage = 0;
    static uint8_t displayedPage = 0;
    float ambientTemp, objectTemp;
    GPS_InfoTypeDef *gpsInfo;
    float latDecimal = 0.0f, lonDecimal = 0.0f;
    uint8_t hasGpsFix = 0;
    static uint8_t wifiState = 0;
    DS1302_TimeTypeDef currentTime;

    Delay_Reset();

    OLED_Init();
    OLED_Clear();

    USART1_Init();
    Set_Level(LOG_LEVEL_DEBUG);

    Info("USART1 Ready\r\n");

    MLX90614_Init();
    Delay_ms(100);

    DS1302_Init();
    DS1302_StartClock();

    GPS_Init();

    ESP8266_Init();

    while(!ESP8266_GetIP(ipStr))
    {
        OLED_ShowString(3, 1, "WiFi:No IP     ");
        Error("WiFi No IP, retrying...\r\n");
        Delay_ms(WIFI_RETRY_INTERVAL_MS);
    }
    OLED_ShowString(3, 1, "WiFi:Connected ");
    OLED_ShowString(4, 1, "IP:            ");
    OLED_ShowString(4, 5, ipStr);
    Info("WiFi Connected, IP: %s\r\n", ipStr);
    Delay_ms(1000);

    OLED_ShowString(1, 1, "MQTT Config... ");
    ESP8266_MQTT_Clean();
    Delay_ms(500);

    testResult = ESP8266_MQTT_UserCfg(MQTT_CLIENT_ID, MQTT_USERNAME, MQTT_PASSWORD);
    if(!testResult)
    {
        OLED_ShowString(1, 1, "MQTT CFG:FAIL!");
        Error("MQTT Config Fail\r\n");
        while(1) { Delay_ms(1000); }
    }
    else
    {
        OLED_ShowString(2, 1, "Connecting...  ");
        testResult = ESP8266_MQTT_Connect(MQTT_BROKER_HOST, MQTT_BROKER_PORT);
        if(!testResult)
        {
            OLED_ShowString(2, 1, "MQTT Conn:FAIL!");
            Error("MQTT Connect Fail\r\n");
            while(1) { Delay_ms(1000); }
        }
        else
        {
            OLED_ShowString(2, 1, "MQTT:Connected!");
            Info("MQTT Connected, start publishing\r\n");
            wifiState = 1;
        }
    }

    Delay_ms(1000);
    OLED_ShowString(4, 1, "Start Publish..");

    Info("MQTT Connected, start subscribing downlink topic\r\n");
    if(ESP8266_MQTT_Subscribe(MQTT_TOPIC_PROP_SET))
    {
        Info("Downlink Subscribe OK\r\n");
    }
    else
    {
        Error("Downlink Subscribe FAIL\r\n");
    }

    while(1)
    {
        ESP8266_MQTT_HandleDownlink();

        if(ESP8266_MQTT_IsConnected())
        {
            if(lastPublishTime == 0 || (Delay_GetTime() - lastPublishTime) > 5000)
            {
                lastPublishTime = Delay_GetTime();

                objectTemp = MLX90614_ReadObjectTemp();
                ambientTemp = MLX90614_ReadAmbientTemp();

                gpsInfo = GPS_GetInfo();
                if(gpsInfo->isValid)
                {
                    latDecimal = NMEAToDecimal(gpsInfo->latitude, gpsInfo->N_S);
                    lonDecimal = NMEAToDecimal(gpsInfo->longitude, gpsInfo->E_W);
                    hasGpsFix = 1;
                    Info("GPS Fix: %.6f, %.6f\r\n", latDecimal, lonDecimal);
                    GPS_SyncDS1302(gpsInfo);
                }
                else if(!hasGpsFix)
                {
                    Info("GPS No Fix, using default coords\r\n");
                }

                if(objectTemp > -200)
                {
                    Info("Read: Obj=%.1fC, Amb=%.1fC\r\n", objectTemp, ambientTemp);

                    ESP8266_MQTT_PublishTemperature(objectTemp);
                    Delay_ms(200);
                    ESP8266_MQTT_PublishAmbientTemp(ambientTemp);
                    Delay_ms(200);
                    /* GPS有效时才提交位置 */
                    if(hasGpsFix)
                    {
                        ESP8266_MQTT_PublishGPS(latDecimal, lonDecimal);
                        Delay_ms(200);
                    }
                    ESP8266_MQTT_PublishPillboxStatus("normal");
                }
                else
                {
                    Error("MLX90614 Read Error\r\n");
                }
            }
        }
        else
        {
            Error("MQTT Disconnected!\r\n");
            wifiState = 2;
            Delay_ms(1000);
        }

        /* 3页面自动轮换显示 */
        if(lastPageSwitchTime == 0 || (Delay_GetTime() - lastPageSwitchTime) > 4000)
        {
            lastPageSwitchTime = Delay_GetTime();
            DS1302_GetTime(&currentTime);

            /* 将objectTemp和ambientTemp更新为最新值 */
            objectTemp = MLX90614_ReadObjectTemp();
            ambientTemp = MLX90614_ReadAmbientTemp();

            OLED_Clear();

            switch(currentPage)
            {
                case 0:  /* 页面1: 年月+时间+温度 */
                    /* 第1行: "年月:YY/MM/DD" */
                    OLED_ShowChinese16x16(1, 1, Chinese_Nian);
                    OLED_ShowChinese16x16(1, 3, Chinese_Yue);
                    OLED_ShowChar(1, 7, ':');
                    OLED_ShowNum(1, 8, currentTime.year, 2);
                    OLED_ShowChar(1, 10, '/');
                    OLED_ShowNum(1, 11, currentTime.month, 2);
                    OLED_ShowChar(1, 13, '/');
                    OLED_ShowNum(1, 14, currentTime.day, 2);

                    /* 第2行: "时间:HH:MM:SS" */
                    OLED_ShowChinese16x16(2, 1, Chinese_Shi);
                    OLED_ShowChinese16x16(2, 3, Chinese_Jian);
                    OLED_ShowChar(2, 7, ':');
                    OLED_ShowNum(2, 8, currentTime.hour, 2);
                    OLED_ShowChar(2, 10, ':');
                    OLED_ShowNum(2, 11, currentTime.minute, 2);
                    OLED_ShowChar(2, 13, ':');
                    OLED_ShowNum(2, 14, currentTime.second, 2);

                    /* 第3行: "物温:XX.XC" */
                    OLED_ShowChinese16x16(3, 1, Chinese_Wu);
                    OLED_ShowChinese16x16(3, 3, Chinese_Wen);
                    OLED_ShowChar(3, 7, ':');
                    if(objectTemp >= 0)
                    {
                        OLED_ShowNum(3, 8, (int32_t)objectTemp, 2);
                    }
                    else
                    {
                        OLED_ShowChar(3, 8, '-');
                        OLED_ShowNum(3, 9, (int32_t)(-objectTemp), 2);
                    }
                    OLED_ShowChar(3, 12, '.');
                    OLED_ShowNum(3, 13, (int32_t)(objectTemp * 10) % 10, 1);
                    OLED_ShowChar(3, 14, 'C');

                    /* 第4行: "环境:XX.XC" */
                    OLED_ShowChinese16x16(4, 1, Chinese_Huan);
                    OLED_ShowChinese16x16(4, 3, Chinese_Jing);
                    OLED_ShowChar(4, 7, ':');
                    if(ambientTemp >= 0)
                    {
                        OLED_ShowNum(4, 8, (int32_t)ambientTemp, 2);
                    }
                    else
                    {
                        OLED_ShowChar(4, 8, '-');
                        OLED_ShowNum(4, 9, (int32_t)(-ambientTemp), 2);
                    }
                    OLED_ShowChar(4, 12, '.');
                    OLED_ShowNum(4, 13, (int32_t)(ambientTemp * 10) % 10, 1);
                    OLED_ShowChar(4, 14, 'C');
                    break;

                case 1:  /* 页面2: 网络+定位+经纬度 */
                    /* 第1行: "网络:{status}" */
                    OLED_ShowChinese16x16(1, 1, Chinese_Wang);
                    OLED_ShowChinese16x16(1, 3, Chinese_Luo2);
                    OLED_ShowChar(1, 7, ':');
                    if(wifiState == 1)
                    {
                        OLED_ShowChinese16x16(1, 5, Chinese_Zai);
                        OLED_ShowChinese16x16(1, 6, Chinese_Xian);
                    }
                    else if(wifiState == 2)
                    {
                        OLED_ShowChinese16x16(1, 5, Chinese_Li);
                        OLED_ShowChinese16x16(1, 6, Chinese_Xian);
                    }
                    else
                    {
                        OLED_ShowChinese16x16(1, 5, Chinese_Lian);
                        OLED_ShowChinese16x16(1, 6, Chinese_Jie);
                        OLED_ShowChinese16x16(1, 7, Chinese_Zhong);
                    }

                    /* 第2行: "定位:{status}" */
                    OLED_ShowChinese16x16(2, 1, Chinese_Ding);
                    OLED_ShowChinese16x16(2, 3, Chinese_Wei);
                    OLED_ShowChar(2, 7, ':');
                    gpsInfo = GPS_GetInfo();
                    if(gpsInfo->isValid)
                    {
                        OLED_ShowChinese16x16(2, 5, Chinese_You);
                        OLED_ShowChinese16x16(2, 6, Chinese_Xiao);
                    }
                    else
                    {
                        OLED_ShowChinese16x16(2, 5, Chinese_Sou);
                        OLED_ShowChinese16x16(2, 6, Chinese_Suo);
                        OLED_ShowChinese16x16(2, 7, Chinese_Zhong);
                    }

                    /* 第3行: 经度 (GPS有效时显示) */
                    if(gpsInfo->isValid)
                    {
                        OLED_ShowChinese16x16(3, 1, Chinese_Jing2);
                        OLED_ShowChinese16x16(3, 3, Chinese_Du);
                        OLED_ShowChar(3, 7, ':');
                        OLED_ShowString(3, 8, gpsInfo->longitude);
                    }
                    else
                    {
                        OLED_ShowString(3, 1, "                ");
                    }

                    /* 第4行: 纬度 (GPS有效时显示) */
                    if(gpsInfo->isValid)
                    {
                        OLED_ShowChinese16x16(4, 1, Chinese_Wei2);
                        OLED_ShowChinese16x16(4, 3, Chinese_Du);
                        OLED_ShowChar(4, 7, ':');
                        OLED_ShowString(4, 8, gpsInfo->latitude);
                    }
                    else
                    {
                        OLED_ShowString(4, 1, "                ");
                    }
                    break;
            }

            displayedPage = currentPage;
            currentPage = (currentPage + 1) % 2;  /* 循环切换2页面 */
        }

        /* 页面0实时刷新时间（每1秒更新） */
        if(displayedPage == 0 && (Delay_GetTime() - lastTimeUpdate) >= 1000)
        {
            lastTimeUpdate = Delay_GetTime();
            DS1302_GetTime(&currentTime);

            OLED_ShowNum(2, 8, currentTime.hour, 2);
            OLED_ShowChar(2, 10, ':');
            OLED_ShowNum(2, 11, currentTime.minute, 2);
            OLED_ShowChar(2, 13, ':');
            OLED_ShowNum(2, 14, currentTime.second, 2);
        }

        Delay_ms(100);
    }
}

int main1(void)
{
    DS1302_TimeTypeDef time;
    GPS_InfoTypeDef *gpsInfo;
    float objTemp, ambTemp;
    uint8_t testResult;
    char ipStr[20];
    char mqttPayload[256];
    static uint32_t lastPublishTime = 0;
    static uint8_t mqttConnected = 0;
    
    Delay_Reset();

    OLED_Init();
    OLED_Clear();

    /* OLED_ShowString(1, 1, "Smart Pillbox"); */
    /* OLED_ShowString(2, 1, "Init..."); */
    /* Delay_ms(500); */

    DS1302_Init();
    testResult = DS1302_TestConnection();

    if(testResult)
    {
        Info("DS1302 Test OK\r\n");
    }
    else
    {
        Error("DS1302 Test FAIL\r\n");
        while(1) { Delay_ms(1000); }
    }
    
    GPS_Init();
    
    MLX90614_Init();
    testResult = MLX90614_TestConnection();
    if(testResult)
    {
        Info("MLX90614 Test OK\r\n");
    }
    else
    {
        Error("MLX90614 Test FAIL\r\n");
    }

    ESP8266_Init();
    
    testResult = ESP8266_TestConnection();
    if(!testResult)
    {
        Error("ESP8266 No Response\r\n");
    }
    
    OLED_ShowString(1, 1, "WiFi Check...  ");
    Delay_ms(500);
    
    if(ESP8266_GetIP(ipStr))
    {
        Info("WiFi Connected, IP: %s\r\n", ipStr);
        OLED_ShowString(1, 1, "WiFi: Connected");
        OLED_ShowString(2, 1, "IP:            ");
        OLED_ShowString(2, 5, ipStr);
        OLED_ShowString(3, 1, "Skip Connect   ");
    }
    else
    {
        Error("WiFi No IP\r\n");
        OLED_ShowString(1, 1, "WiFi: No IP    ");
        OLED_ShowString(3, 1, "Need Config    ");
    }
    Delay_ms(1000);
    
    OLED_ShowString(1, 1, "MQTT Init...   ");
    Info("MQTT Init...\r\n");
    
    ESP8266_MQTT_Clean();
    Delay_ms(500);
    
    OLED_ShowString(3, 1, "User Config... ");
    testResult = ESP8266_MQTT_UserCfg(MQTT_CLIENT_ID, MQTT_USERNAME, MQTT_PASSWORD);
    
    if(!testResult)
    {
        Error("MQTT UserCfg FAIL\r\n");
        OLED_ShowString(1, 1, "MQTT CFG: FAIL!");
        Delay_ms(3000);
    }
    else
    {
        OLED_ShowString(3, 1, "Connecting...  ");
        testResult = ESP8266_MQTT_Connect(MQTT_BROKER_HOST, MQTT_BROKER_PORT);
        
        if(testResult)
        {
            OLED_ShowString(1, 1, "MQTT: Connected!");
            OLED_ShowString(3, 1, "Subscribing... ");
            Delay_ms(500);
            
            if(ESP8266_MQTT_Subscribe(MQTT_TOPIC_PROP_SET))
            {
                OLED_ShowString(3, 1, "Sub: OK!       ");
            }
            else
            {
                Error("MQTT Subscribe FAIL\r\n");
                OLED_ShowString(3, 1, "Sub: FAIL!     ");
            }
        }
        else
        {
            Error("MQTT Connect FAIL\r\n");
            OLED_ShowString(1, 1, "MQTT: FAIL!    ");
            OLED_ShowString(3, 1, "Check broker! ");
        }
    }
    
    Delay_ms(2000);
    
    DS1302_StartClock();
    
    time.hour = DS1302_DEFAULT_HOUR;
    time.minute = DS1302_DEFAULT_MINUTE;
    time.second = DS1302_DEFAULT_SECOND;
    time.year = DS1302_DEFAULT_YEAR;
    time.month = DS1302_DEFAULT_MONTH;
    time.day = DS1302_DEFAULT_DAY;
    time.week = DS1302_DEFAULT_WEEK;
    DS1302_SetTime(&time);
    OLED_Clear();
    Info("Main Loop Start\r\n");
    
    while(1)
    {
        DS1302_GetTime(&time);
        
        objTemp = MLX90614_ReadObjectTemp();
        ambTemp = MLX90614_ReadAmbientTemp();
        
        if(objTemp > -200)
        {
            Debug("Obj: %.1fC, Amb: %.1fC\r\n", objTemp, ambTemp);
        }
        
        gpsInfo = GPS_GetInfo();
        
        /* if(gpsInfo->isValid) */
        /* { */
        /*     OLED_ShowString(4, 1, "GPS: Valid     "); */
        /* } */
        /* else if(gpsInfo->frameCount > 0) */
        /* { */
        /*     OLED_ShowString(4, 1, "GPS: No Fix    "); */
        /* } */
        /* else if(gpsInfo->rxCount > 0) */
        /* { */
        /*     OLED_ShowString(4, 1, "GPS: Data RX   "); */
        /* } */
        /* else */
        /* { */
        /*     OLED_ShowString(4, 1, "GPS: No Data   "); */
        /* } */
        
        if(ESP8266_MQTT_IsConnected())
        {
            if(lastPublishTime == 0 || (Delay_GetTime() - lastPublishTime) > MQTT_PUBLISH_INTERVAL)
            {
                lastPublishTime = Delay_GetTime();
                
                sprintf(mqttPayload, "{\\\"id\\\":\\\"123\\\"\\,\\\"version\\\":\\\"1.0\\\"\\,\\\"params\\\":{\\\"temperature\\\":{\\\"value\\\":%.1f}}}",
                        objTemp);
                
                if(ESP8266_MQTT_Publish(MQTT_TOPIC_PROP_POST, mqttPayload))
                {
                    Info("MQTT Published: Temp=%.1fC\r\n", objTemp);
                    OLED_ShowString(4, 1, "MQTT: Sent!   ");
                }
                else
                {
                    Error("MQTT Publish FAIL\r\n");
                    OLED_ShowString(4, 1, "MQTT: FAIL!    ");
                }
            }
        }

        /* Servo_360_Forward(); */
        /* Delay_ms(1000); */
        /* Servo_360_Stop(); */
        /* Delay_ms(500); */
        /* Servo_360_Backward(); */
        /* Delay_ms(1000); */
        /* Servo_360_Stop(); */
        /* Delay_ms(500); */

        Delay_ms(500);
    }
}
