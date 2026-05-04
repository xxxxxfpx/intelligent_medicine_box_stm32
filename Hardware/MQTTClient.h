/**
 * MQTT客户端驱动
 * 基于ESP8266 + OneNET平台
 */

#ifndef __MQTT_CLIENT_H
#define __MQTT_CLIENT_H

#include "stm32f10x.h"
#include "ESP8266.h"

#define MQTT_BUFFER_SIZE  512
#define MQTT_CMD_TIMEOUT  10000  // 10秒超时

typedef enum {
    MQTT_STATE_DISCONNECTED = 0,
    MQTT_STATE_CONNECTING,
    MQTT_STATE_CONNECTED,
    MQTT_STATE_ERROR
} MQTT_StateTypeDef;

typedef struct {
    char buffer[MQTT_BUFFER_SIZE];
    uint16_t rxIndex;
    uint8_t connected;
} MQTT_ClientTypeDef;

void MQTT_Init(void);
uint8_t MQTT_Connect(const char *clientID, const char *username, const char *password);
uint8_t MQTT_Subscribe(const char *topic);
uint8_t MQTT_Publish(const char *topic, const char *payload);
uint8_t MQTT_ProcessLoop(void);
uint8_t MQTT_IsConnected(void);
void MQTT_Disconnect(void);

#endif