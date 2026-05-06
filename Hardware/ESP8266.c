/**
 * ESP8266 WiFi模块驱动
 * 使用USART3通信 (PB10=TX, PB11=RX)
 * 波特率: 115200
 * 支持AT指令控制
 */

#include "ESP8266.h"
#include "Delay.h"
#include "GPS.h"
#include "MLX90614.h"
#include "OLED.h"
#include "Servo.h"
#include "Settings.h"
#include "USART1.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static ESP8266_TypeDef esp8266;

static uint8_t mqttConnected = 0;

void ESP8266_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    USART_InitTypeDef USART_InitStructure;
    NVIC_InitTypeDef NVIC_InitStructure;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART3, ENABLE);

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOB, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(GPIOB, &GPIO_InitStructure);

    USART_InitStructure.USART_BaudRate = 115200;
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;
    USART_InitStructure.USART_StopBits = USART_StopBits_1;
    USART_InitStructure.USART_Parity = USART_Parity_No;
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
    USART_Init(USART3, &USART_InitStructure);

    NVIC_InitStructure.NVIC_IRQChannel = USART3_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

    USART_ITConfig(USART3, USART_IT_RXNE, ENABLE);
    USART_Cmd(USART3, ENABLE);

    ESP8266_Clear();
    Info("ESP8266 Init OK, Baud:115200\r\n");
}

void ESP8266_Clear(void)
{
    esp8266.rxIndex = 0;
    esp8266.rxComplete = 0;
    memset(esp8266.buffer, 0, ESP8266_BUFFER_SIZE);
}

void ESP8266_SendCmd(const char *cmd)
{
    while (*cmd)
    {
        while (USART_GetFlagStatus(USART3, USART_FLAG_TXE) == RESET);
        USART_SendData(USART3, *cmd++);
    }
    while (USART_GetFlagStatus(USART3, USART_FLAG_TC) == RESET);
}

uint8_t ESP8266_WaitResponse(const char *response, uint32_t timeout)
{
    uint32_t startTime = 0;

    while (startTime < timeout)
    {
        if (strstr(esp8266.buffer, response) != NULL)
        {
            return 1;
        }
        if (strstr(esp8266.buffer, "ERROR") != NULL)
        {
            return 0;
        }
        if (strstr(esp8266.buffer, "FAIL") != NULL)
        {
            return 0;
        }
        Delay_ms(10);
        startTime += 10;
    }
    return 0;
}

void ESP8266_PrintBuffer(void)
{
    uint8_t i, len, nibble;

    if (esp8266.rxIndex > 0)
    {
        len = esp8266.rxIndex;
        if(len > 8) len = 8;

        OLED_ShowString(4, 1, "HEX:");

        for(i = 0; i < len; i++)
        {
            nibble = (esp8266.buffer[i] >> 4) & 0x0F;
            if(nibble < 10) OLED_ShowChar(4, 6 + i*3, nibble + '0');
            else OLED_ShowChar(4, 6 + i*3, nibble - 10 + 'A');

            nibble = esp8266.buffer[i] & 0x0F;
            if(nibble < 10) OLED_ShowChar(4, 7 + i*3, nibble + '0');
            else OLED_ShowChar(4, 7 + i*3, nibble - 10 + 'A');

            OLED_ShowChar(4, 8 + i*3, ' ');
        }
    }
}

uint8_t ESP8266_TestConnection(void)
{
    uint16_t i;
    uint8_t data;

    ESP8266_Clear();
    ESP8266_SendCmd("AT\r\n");

    OLED_ShowString(2, 1, "Polling RX...");
    Delay_ms(500);

    for(i = 0; i < 100; i++)
    {
        if(USART_GetFlagStatus(USART3, USART_FLAG_RXNE) != RESET)
        {
            data = USART_ReceiveData(USART3);
            if(esp8266.rxIndex < ESP8266_BUFFER_SIZE - 1)
            {
                esp8266.buffer[esp8266.rxIndex++] = data;
                esp8266.buffer[esp8266.rxIndex] = '\0';
            }
            OLED_ShowNum(3, 1, esp8266.rxIndex, 3);
        }
        Delay_ms(10);
    }

    OLED_ShowString(2, 1, "RX len:");
    OLED_ShowNum(2, 9, ESP8266_GetBufferLen(), 4);

    if(ESP8266_GetBufferLen() == 0)
    {
        OLED_ShowString(3, 1, "No RX data!   ");
        OLED_ShowString(4, 1, "Check wiring! ");
        return 0;
    }

    ESP8266_PrintBuffer();

    if(ESP8266_WaitResponse("OK", 1000))
    {
        Info("ESP8266 AT OK\r\n");
        return 1;
    }
    else
    {
        Error("ESP8266 AT FAIL\r\n");
        return 0;
    }
}

uint8_t ESP8266_Reset(void)
{
    ESP8266_Clear();
    ESP8266_SendCmd("AT+RST\r\n");
    Delay_ms(2000);
    if(ESP8266_WaitResponse("ready", 3000))
    {
        Info("ESP8266 Reset OK\r\n");
        return 1;
    }
    Error("ESP8266 Reset FAIL\r\n");
    return 0;
}

uint8_t ESP8266_SetMode(uint8_t mode)
{
    char cmd[32];
    ESP8266_Clear();
    sprintf(cmd, "AT+CWMODE=%d\r\n", mode);
    ESP8266_SendCmd(cmd);
    if(ESP8266_WaitResponse("OK", 1000))
    {
        Info("ESP8266 SetMode %d OK\r\n", mode);
        return 1;
    }
    Error("ESP8266 SetMode %d FAIL\r\n", mode);
    return 0;
}

uint8_t ESP8266_ConnectWiFi(const char *ssid, const char *password)
{
    char cmd[128];
    uint8_t result;

    ESP8266_Clear();
    sprintf(cmd, "AT+CWJAP=\"%s\",\"%s\"\r\n", ssid, password);
    ESP8266_SendCmd(cmd);

    result = ESP8266_WaitResponse("WIFI GOT IP", 20000);

    if (result == 0)
    {
        result = ESP8266_WaitResponse("OK", 5000);
    }

    if(result)
    {
        Info("WiFi Connected: %s\r\n", ssid);
    }
    else
    {
        Error("WiFi Connect FAIL: %s\r\n", ssid);
    }

    return result;
}

uint8_t ESP8266_GetIP(char *ip)
{
    char *start, *end;

    ESP8266_Clear();
    ESP8266_SendCmd("AT+CIFSR\r\n");
    if (!ESP8266_WaitResponse("OK", 200))
    {
        Debug("GetIP: No OK response, buffer=%s\r\n", esp8266.buffer);
        return 0;
    }

    Debug("GetIP: raw=%s\r\n", esp8266.buffer);
    start = strstr(esp8266.buffer, "STAIP,\"");
    if (start == NULL)
    {
        return 0;
    }

    start += 7;
    end = strchr(start, '"');
    if (end == NULL)
    {
        return 0;
    }

    strncpy(ip, start, end - start);
    ip[end - start] = '\0';

    Info("ESP8266 IP: %s\r\n", ip);

    return 1;
}

uint8_t ESP8266_Disconnect(void)
{
    ESP8266_Clear();
    ESP8266_SendCmd("AT+CWQAP\r\n");
    if(ESP8266_WaitResponse("OK", 2000))
    {
        Info("WiFi Disconnected\r\n");
        return 1;
    }
    Error("WiFi Disconnect FAIL\r\n");
    return 0;
}

uint8_t ESP8266_ConnectTCP(const char *host, uint16_t port)
{
    char cmd[128];
    uint8_t result = 0;

    ESP8266_Clear();
    ESP8266_SendCmd("AT+CIPMUX=0\r\n");
    if (!ESP8266_WaitResponse("OK", 3000))
    {
        return 0;
    }
    Delay_ms(500);

    ESP8266_Clear();
    ESP8266_SendCmd("AT+CIPCLOSE\r\n");
    Delay_ms(500);

    ESP8266_Clear();
    sprintf(cmd, "AT+CIPSTART=\"TCP\",\"%s\",%d\r\n", host, port);
    ESP8266_SendCmd(cmd);
    Delay_ms(100);

    result = ESP8266_WaitResponse("CONNECT", 10000);
    if (result)
    {
        return 1;
    }

    if (strstr(esp8266.buffer, "ALREADY CONNECTED") != NULL)
    {
        return 1;
    }

    return 0;
}

uint8_t ESP8266_CloseTCP(void)
{
    ESP8266_Clear();
    ESP8266_SendCmd("AT+CIPCLOSE\r\n");
    return ESP8266_WaitResponse("CLOSED", 5000);
}

char* ESP8266_GetBuffer(void)
{
    return esp8266.buffer;
}

uint16_t ESP8266_GetBufferLen(void)
{
    return esp8266.rxIndex;
}

static void ESP8266_LogBuffer(void)
{
    if(esp8266.rxIndex > 0)
    {
        Debug("RX[%d]: %s\r\n", esp8266.rxIndex, esp8266.buffer);
    }
}

void ESP8266_MQTT_Clean(void)
{
    Info("MQTTCLEAN...\r\n");
    ESP8266_Clear();
    ESP8266_SendCmd("AT+MQTTCLEAN=0\r\n");
    if(ESP8266_WaitResponse("OK", 2000))
    {
        Info("MQTTCLEAN OK\r\n");
    }
    else
    {
        Error("MQTTCLEAN FAIL\r\n");
        ESP8266_LogBuffer();
    }
    mqttConnected = 0;
}

uint8_t ESP8266_MQTT_UserCfg(const char *client_id, const char *username, const char *password)
{
    char cmd[512];

    Info("MQTTUSERCFG: client=%s, user=%s\r\n", client_id, username);
    ESP8266_Clear();
    sprintf(cmd, "AT+MQTTUSERCFG=0,1,\"%s\",\"%s\",\"%s\",0,0,\"\"\r\n",
            client_id, username, password);
    ESP8266_SendCmd(cmd);

    if(ESP8266_WaitResponse("OK", 3000))
    {
        Debug("MQTT UserCfg OK\r\n");
        return 1;
    }
    Error("MQTT UserCfg FAIL\r\n");
    ESP8266_LogBuffer();
    return 0;
}

uint8_t ESP8266_MQTT_Connect(const char *host, uint16_t port)
{
    char cmd[128];

    Info("MQTTCONN: %s:%d\r\n", host, port);
    ESP8266_Clear();
    sprintf(cmd, "AT+MQTTCONN=0,\"%s\",%d,0\r\n", host, port);
    ESP8266_SendCmd(cmd);

    if(ESP8266_WaitResponse("+MQTTCONNECTED:", 10000))
    {
        if(ESP8266_WaitResponse("OK", 1000))
        {
            mqttConnected = 1;
            Info("MQTT Connected: %s:%d\r\n", host, port);
            return 1;
        }
    }

    Error("MQTT Connect FAIL: %s:%d\r\n", host, port);
    ESP8266_LogBuffer();
    return 0;
}

uint8_t ESP8266_MQTT_Subscribe(const char *topic)
{
    char cmd[256];

    Info("MQTTSUB: %s\r\n", topic);
    ESP8266_Clear();
    sprintf(cmd, "AT+MQTTSUB=0,\"%s\",0\r\n", topic);
    ESP8266_SendCmd(cmd);

    if(ESP8266_WaitResponse("OK", 3000))
    {
        Debug("MQTT Subscribe: %s\r\n", topic);
        return 1;
    }
    Error("MQTT Subscribe FAIL: %s\r\n", topic);
    ESP8266_LogBuffer();
    return 0;
}

uint8_t ESP8266_MQTT_Publish(const char *topic, const char *data)
{
    char cmd[512];
    uint16_t dataLen;

    dataLen = strlen(data);
    if(dataLen > 400)
    {
        Error("MQTTPUB FAIL: data too long, len=%d\r\n", dataLen);
        return 0;
    }

    Info("MQTTPUB: topic=%s, len=%d\r\n", topic, dataLen);
    Debug("Payload: %s\r\n", data);

    ESP8266_Clear();
    sprintf(cmd, "AT+MQTTPUB=0,\"%s\",\"%s\",0,0\r\n", topic, data);
    Debug("CMD: %s\r\n", cmd);

    if(strlen(cmd) > 500)
    {
        Error("MQTTPUB FAIL: cmd too long\r\n");
        return 0;
    }

    ESP8266_SendCmd(cmd);

    if(ESP8266_WaitResponse("OK", 3000))
    {
        Debug("MQTT Publish OK, len=%d\r\n", dataLen);
        return 1;
    }

    Error("MQTTPUB FAIL, rxBuf: %s\r\n", esp8266.buffer);
    ESP8266_LogBuffer();
    return 0;
}

uint8_t ESP8266_MQTT_IsConnected(void)
{
    return mqttConnected;
}

uint8_t ESP8266_MQTT_PublishTemperature(float temperature)
{
    char topic[128];
    char payload[256];
    
    sprintf(topic, "$sys/%s/%s/thing/property/post", MQTT_PRODUCT_ID, MQTT_DEVICE_NAME);
    sprintf(payload, "{\\\"id\\\":\\\"123\\\"\\,\\\"version\\\":\\\"1.0\\\"\\,\\\"params\\\":{\\\"temperature\\\":{\\\"value\\\":%.2f}}}", temperature);
    
    Debug("Pub Temp: %.2fC\r\n", temperature);
    if(ESP8266_MQTT_Publish(topic, payload))
    {
        Info("Commit Temp: %.2fC OK\r\n", temperature);
        return 1;
    }
    Error("Commit Temp: %.2fC FAIL\r\n", temperature);
    return 0;
}

uint8_t ESP8266_MQTT_PublishAmbientTemp(float ambient_temp)
{
    char topic[128];
    char payload[256];
    
    sprintf(topic, "$sys/%s/%s/thing/property/post", MQTT_PRODUCT_ID, MQTT_DEVICE_NAME);
    sprintf(payload, "{\\\"id\\\":\\\"123\\\"\\,\\\"version\\\":\\\"1.0\\\"\\,\\\"params\\\":{\\\"ambient_temperature\\\":{\\\"value\\\":%.2f}}}", ambient_temp);
    
    Debug("Pub AmbTemp: %.2fC\r\n", ambient_temp);
    if(ESP8266_MQTT_Publish(topic, payload))
    {
        Info("Commit AmbTemp: %.2fC OK\r\n", ambient_temp);
        return 1;
    }
    Error("Commit AmbTemp: %.2fC FAIL\r\n", ambient_temp);
    return 0;
}

uint8_t ESP8266_MQTT_PublishGPS(float latitude, float longitude)
{
    char topic[128];
    char payload[256];
    
    sprintf(topic, "$sys/%s/%s/thing/property/post", MQTT_PRODUCT_ID, MQTT_DEVICE_NAME);
    sprintf(payload, "{\\\"id\\\":\\\"123\\\"\\,\\\"version\\\":\\\"1.0\\\"\\,\\\"params\\\":{\\\"latitude\\\":{\\\"value\\\":%.6f}\\,\\\"longitude\\\":{\\\"value\\\":%.6f}}}", latitude, longitude);
    
    Debug("Pub GPS: %.6f, %.6f\r\n", latitude, longitude);
    if(ESP8266_MQTT_Publish(topic, payload))
    {
        Info("Commit GPS: %.6f, %.6f OK\r\n", latitude, longitude);
        return 1;
    }
    Error("Commit GPS: %.6f, %.6f FAIL\r\n", latitude, longitude);
    return 0;
}

uint8_t ESP8266_MQTT_PublishPillboxStatus(const char *status)
{
    char topic[128];
    char payload[256];
    
    sprintf(topic, "$sys/%s/%s/thing/property/post", MQTT_PRODUCT_ID, MQTT_DEVICE_NAME);
    sprintf(payload, "{\\\"id\\\":\\\"123\\\"\\,\\\"version\\\":\\\"1.0\\\"\\,\\\"params\\\":{\\\"pillbox_status\\\":{\\\"value\\\":\\\"%s\\\"}}}", status);
    
    Debug("Pub Pillbox: %s\r\n", status);
    if(ESP8266_MQTT_Publish(topic, payload))
    {
        Info("Commit Pillbox: %s OK\r\n", status);
        return 1;
    }
    Error("Commit Pillbox: %s FAIL\r\n", status);
    return 0;
}

uint8_t ESP8266_MQTT_HasPendingMessage(void)
{
    return (strstr(esp8266.buffer, "+MQTTSUBRECV:") != NULL) ? 1 : 0;
}

uint16_t ESP8266_MQTT_ParseMessage(char *payloadBuf, uint16_t bufSize)
{
    char *recv;
    char *p;
    uint16_t len;

    recv = strstr(esp8266.buffer, "+MQTTSUBRECV:");
    if(!recv) return 0;

    p = strchr(recv, ',');
    if(!p) return 0;
    p = strchr(p + 1, ',');
    if(!p) return 0;
    p = strchr(p + 1, ',');
    if(!p) return 0;
    p++;

    len = strlen(p);
    while(len > 0 && (p[len-1] == '\r' || p[len-1] == '\n' || p[len-1] == ' '))
        len--;

    if(len >= bufSize) len = bufSize - 1;
    memcpy(payloadBuf, p, len);
    payloadBuf[len] = '\0';

    return len;
}

static void MQTT_SendFloatReply(const char *property, float value)
{
    char cmd[512];
    char data[120];

    snprintf(data, sizeof(data),
        "{\\\"id\\\":\\\"123\\\"\\,\\\"version\\\":\\\"1.0\\\"\\,\\\"params\\\":{\\\"%s\\\":{\\\"value\\\":%.2f}}}",
        property, value);
    Info("  Reply: %s = %.2f\r\n", property, value);

    snprintf(cmd, sizeof(cmd), "AT+MQTTPUB=0,\"$sys/%s/%s/thing/property/post\",\"%s\",0,0\r\n",
        MQTT_PRODUCT_ID, MQTT_DEVICE_NAME, data);
    ESP8266_SendCmd(cmd);
    ESP8266_WaitResponse("OK", 3000);
}

static void MQTT_SendStringReply(const char *property, const char *value)
{
    char cmd[512];
    char data[120];

    snprintf(data, sizeof(data),
        "{\\\"id\\\":\\\"123\\\"\\,\\\"version\\\":\\\"1.0\\\"\\,\\\"params\\\":{\\\"%s\\\":{\\\"value\\\":\\\"%s\\\"}}}",
        property, value);
    Info("  Reply: %s = %s\r\n", property, value);

    snprintf(cmd, sizeof(cmd), "AT+MQTTPUB=0,\"$sys/%s/%s/thing/property/post\",\"%s\",0,0\r\n",
        MQTT_PRODUCT_ID, MQTT_DEVICE_NAME, data);
    ESP8266_SendCmd(cmd);
    ESP8266_WaitResponse("OK", 3000);
}

static void MQTT_SendIntReply(const char *property, int32_t value)
{
    char cmd[512];
    char data[120];

    snprintf(data, sizeof(data),
        "{\\\"id\\\":\\\"123\\\"\\,\\\"version\\\":\\\"1.0\\\"\\,\\\"params\\\":{\\\"%s\\\":{\\\"value\\\":%ld}}}",
        property, (long)value);
    Info("  Reply: %s = %ld\r\n", property, (long)value);

    snprintf(cmd, sizeof(cmd), "AT+MQTTPUB=0,\"$sys/%s/%s/thing/property/post\",\"%s\",0,0\r\n",
        MQTT_PRODUCT_ID, MQTT_DEVICE_NAME, data);
    ESP8266_SendCmd(cmd);
    ESP8266_WaitResponse("OK", 3000);
}

static void MQTT_SendEnumReply(const char *property, int value)
{
    char cmd[512];
    char data[120];

    snprintf(data, sizeof(data),
        "{\\\"id\\\":\\\"123\\\"\\,\\\"version\\\":\\\"1.0\\\"\\,\\\"params\\\":{\\\"%s\\\":{\\\"value\\\":%d}}}",
        property, value);
    Info("  Reply: %s = %d\r\n", property, value);

    snprintf(cmd, sizeof(cmd), "AT+MQTTPUB=0,\"$sys/%s/%s/thing/property/post\",\"%s\",0,0\r\n",
        MQTT_PRODUCT_ID, MQTT_DEVICE_NAME, data);
    ESP8266_SendCmd(cmd);
    ESP8266_WaitResponse("OK", 3000);
}

static void MQTT_SendPropertySetReply(const char *id, uint8_t code, const char *msg)
{
    char cmd[256];
    char data[128];
    
    if(id == NULL) id = "123";
    if(msg == NULL) msg = "success";
    
    snprintf(data, sizeof(data),
        "{\\\"id\\\":\\\"%s\\\"\\,\\\"code\\\":%d\\,\\\"msg\\\":\\\"%s\\\"}",
        id, code, msg);
    Info("  Property Set Reply: %s\r\n", data);
    
    ESP8266_Clear();
    snprintf(cmd, sizeof(cmd), "AT+MQTTPUB=0,\"$sys/%s/%s/thing/property/set_reply\",\"%s\",0,0\r\n",
        MQTT_PRODUCT_ID, MQTT_DEVICE_NAME, data);
    Debug("  CMD: %s\r\n", cmd);
    ESP8266_SendCmd(cmd);
}

uint8_t ESP8266_MQTT_HandleDownlink(void)
{
    char payload[256];
    char propName[64];
    char msgId[32] = "123";
    char *params;
    char *obj;
    char *propStart;
    char *propEnd;
    char *idPtr;
    char *idStart;
    char *idEnd;
    uint16_t len;
    uint16_t propLen;
    uint16_t idLen;
    GPS_InfoTypeDef *gpsInfo;
    char cmd[512];
    char data[120];

    if(!ESP8266_MQTT_HasPendingMessage()) return 0;

    len = ESP8266_MQTT_ParseMessage(payload, sizeof(payload));
    if(len == 0)
    {
        ESP8266_Clear();
        return 0;
    }

    Info("[Downlink] Received: %s\r\n", payload);

    idPtr = strstr(payload, "\"id\"");
    if(idPtr)
    {
        idStart = strchr(idPtr, ':');
        if(idStart)
        {
            idStart++;
            while(*idStart == ' ' || *idStart == '"') idStart++;
            idEnd = strchr(idStart, '"');
            if(idEnd)
            {
                idLen = idEnd - idStart;
                if(idLen < sizeof(msgId))
                {
                    memcpy(msgId, idStart, idLen);
                    msgId[idLen] = '\0';
                    Info("  MsgID: %s\r\n", msgId);
                }
            }
        }
    }

    params = strstr(payload, "\"params\"");
    if(!params)
    {
        MQTT_SendPropertySetReply(msgId, 100, "params missing");
        ESP8266_Clear();
        return 1;
    }

    obj = strchr(params + 8, '{');
    if(!obj)
    {
        MQTT_SendPropertySetReply(msgId, 100, "invalid format");
        ESP8266_Clear();
        return 1;
    }

    while(*obj != '\0')
    {
        propStart = strchr(obj, '"');
        if(!propStart) break;
        propStart++;

        propEnd = strchr(propStart, '"');
        if(!propEnd) break;
        propLen = propEnd - propStart;
        if(propLen >= sizeof(propName)) propLen = sizeof(propName) - 1;
        memcpy(propName, propStart, propLen);
        propName[propLen] = '\0';

        obj = propEnd + 1;
        while(*obj == ':' || *obj == ' ' || *obj == '{' || *obj == '"' || *obj == '}' || *obj == ',')
            obj++;

        Info("  Property: %s\r\n", propName);

        if(strcmp(propName, "temperature") == 0)
        {
            float objTemp = MLX90614_ReadObjectTemp();
            MQTT_SendFloatReply("temperature", objTemp);
            MQTT_SendPropertySetReply(msgId, 200, "success");
            break;
        }
        else if(strcmp(propName, "ambient_temperature") == 0)
        {
            float ambTemp = MLX90614_ReadAmbientTemp();
            MQTT_SendFloatReply("ambient_temperature", ambTemp);
            MQTT_SendPropertySetReply(msgId, 200, "success");
            break;
        }
        else if(strcmp(propName, "latitude") == 0 || strcmp(propName, "longitude") == 0)
        {
            gpsInfo = GPS_GetInfo();
            if(gpsInfo && gpsInfo->isValid)
            {
                char *gpsVal = (strcmp(propName, "latitude") == 0) ? gpsInfo->latitude : gpsInfo->longitude;
                snprintf(data, sizeof(data),
                    "{\\\"id\\\":\\\"123\\\"\\,\\\"version\\\":\\\"1.0\\\"\\,\\\"params\\\":{\\\"%s\\\":{\\\"value\\\":\\\"%s\\\"}}}",
                    propName, gpsVal);
                snprintf(cmd, sizeof(cmd), "AT+MQTTPUB=0,\"$sys/%s/%s/thing/property/post\",\"%s\",0,0\r\n",
                    MQTT_PRODUCT_ID, MQTT_DEVICE_NAME, data);
                ESP8266_SendCmd(cmd);
                ESP8266_WaitResponse("OK", 3000);
            }
            MQTT_SendPropertySetReply(msgId, 200, "success");
            break;
        }
        else if(strcmp(propName, "CellNumber") == 0)
        {
            MQTT_SendIntReply("CellNumber", 6);
            MQTT_SendPropertySetReply(msgId, 200, "success");
            break;
        }
        else if(strcmp(propName, "PillRemain") == 0)
        {
            Info("  [Set] PillRemain\r\n");
            MQTT_SendPropertySetReply(msgId, 200, "success");
            break;
        }
        else if(strcmp(propName, "Open") == 0)
        {
            MQTT_SendPropertySetReply(msgId, 200, "success");

            if(strstr(payload, "\"Open\":true") || strstr(payload, "\"Open\": true"))
            {
                Info("  [Servo] Open -> 120 deg\r\n");
                Servo_RotateAngle(120);
            }
            else
            {
                Info("  [Servo] Open -> -120 deg (close)\r\n");
                Servo_RotateAngle(-120);
            }
            break;
        }
        else if(strcmp(propName, "BatteryStatus") == 0)
        {
            MQTT_SendEnumReply("BatteryStatus", 1);
            MQTT_SendPropertySetReply(msgId, 200, "success");
            break;
        }
        else if(strcmp(propName, "TakePillState") == 0)
        {
            MQTT_SendEnumReply("TakePillState", 0);
            MQTT_SendPropertySetReply(msgId, 200, "success");
            break;
        }
        else if(strcmp(propName, "pillbox_status") == 0)
        {
            MQTT_SendStringReply("pillbox_status", "normal");
            MQTT_SendPropertySetReply(msgId, 200, "success");
            break;
        }
        else
        {
            Info("  [WARN] Unknown property: %s\r\n", propName);
            MQTT_SendPropertySetReply(msgId, 100, "property not found");
            break;
        }
    }

    ESP8266_Clear();
    return 1;
}

void USART3_IRQHandler(void)
{
    uint8_t data;

    if (USART_GetITStatus(USART3, USART_IT_RXNE) != RESET)
    {
        data = USART_ReceiveData(USART3);

        if (esp8266.rxIndex < ESP8266_BUFFER_SIZE - 1)
        {
            esp8266.buffer[esp8266.rxIndex++] = data;
            esp8266.buffer[esp8266.rxIndex] = '\0';
        }
    }
}