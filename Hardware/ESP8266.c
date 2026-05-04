/**
 * ESP8266 WiFi模块驱动
 * 使用USART3通信 (PB10=TX, PB11=RX)
 * 波特率: 115200
 * 支持AT指令控制
 */

#include "ESP8266.h"
#include "Delay.h"
#include "OLED.h"
#include "Settings.h"
#include "USART1.h"
#include <string.h>

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
    uint16_t lastLen = 0;

    while (startTime < timeout)
    {
        if (esp8266.rxIndex > lastLen)
        {
            lastLen = esp8266.rxIndex;
        }
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
    if (!ESP8266_WaitResponse("OK", 2000))
    {
        return 0;
    }

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