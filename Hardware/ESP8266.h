/**
 * ESP8266 WiFi模块驱动
 * 使用USART3通信 (PB10=TX, PB11=RX)
 * 波特率: 115200
 * 支持AT指令控制
 */

#ifndef __ESP8266_H
#define __ESP8266_H

#include "stm32f10x.h"

#define ESP8266_BUFFER_SIZE  1024

typedef struct {
    char buffer[ESP8266_BUFFER_SIZE];
    uint16_t rxIndex;
    uint8_t rxComplete;
} ESP8266_TypeDef;

void ESP8266_Init(void);
void ESP8266_Clear(void);
void ESP8266_SendCmd(const char *cmd);
uint8_t ESP8266_WaitResponse(const char *response, uint32_t timeout);
uint8_t ESP8266_TestConnection(void);
uint8_t ESP8266_Reset(void);
uint8_t ESP8266_SetMode(uint8_t mode);
uint8_t ESP8266_ConnectWiFi(const char *ssid, const char *password);
uint8_t ESP8266_GetIP(char *ip);
uint8_t ESP8266_Disconnect(void);
uint8_t ESP8266_ConnectTCP(const char *host, uint16_t port);
uint8_t ESP8266_CloseTCP(void);
char* ESP8266_GetBuffer(void);
uint16_t ESP8266_GetBufferLen(void);

void ESP8266_MQTT_Clean(void);
uint8_t ESP8266_MQTT_UserCfg(const char *client_id, const char *username, const char *password);
uint8_t ESP8266_MQTT_Connect(const char *host, uint16_t port);
uint8_t ESP8266_MQTT_Subscribe(const char *topic);
uint8_t ESP8266_MQTT_Publish(const char *topic, const char *data);
uint8_t ESP8266_MQTT_IsConnected(void);
uint8_t ESP8266_MQTT_PublishTemperature(float temperature);
uint8_t ESP8266_MQTT_PublishAmbientTemp(float ambient_temp);
uint8_t ESP8266_MQTT_PublishGPS(float latitude, float longitude);
uint8_t ESP8266_MQTT_PublishPillboxStatus(const char *status);

/**
 * Check if there is a new MQTT subscription message from the broker
 * Scans esp8266.buffer for "+MQTTSUBRECV:" prefix
 * Returns: 1 if message detected, 0 otherwise
 */
uint8_t ESP8266_MQTT_HasPendingMessage(void);

/**
 * Parse the last received MQTT subscription message from esp8266.buffer
 * Extracts payload content (JSON string after the 3rd comma)
 * payloadBuf: output buffer to store the parsed payload
 * bufSize: size of payloadBuf
 * Returns: length of parsed payload, 0 if parse failed
 */
uint16_t ESP8266_MQTT_ParseMessage(char *payloadBuf, uint16_t bufSize);

/**
 * Handle MQTT downlink messages from OneNET (property set commands)
 * Must be called in main loop. Detects +MQTTSUBRECV:, parses JSON,
 * responds to set commands and reports current values.
 * Returns: 1 if a downlink message was handled, 0 if none pending
 */
uint8_t ESP8266_MQTT_HandleDownlink(void);

#endif