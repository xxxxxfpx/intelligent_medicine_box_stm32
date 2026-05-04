/**
 * MQTT客户端驱动实现
 * 支持连接、订阅、发布功能
 */

#include "MQTTClient.h"
#include "ESP8266.h"
#include "Delay.h"
#include "Settings.h"
#include "USART1.h"
#include <string.h>
#include <stdio.h>

static MQTT_ClientTypeDef mqttClient;

static void MQTT_Clear(void)
{
    mqttClient.rxIndex = 0;
    memset(mqttClient.buffer, 0, MQTT_BUFFER_SIZE);
}

static void MQTT_SendPacket(uint8_t *data, uint16_t len)
{
    while (len--) {
        while (USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET);
        USART_SendData(USART1, *data++);
    }
    while (USART_GetFlagStatus(USART1, USART_FLAG_TC) == RESET);
}

static uint8_t MQTT_WaitResponse(const char *response, uint32_t timeout)
{
    uint32_t startTime = 0;

    while (startTime < timeout) {
        if (strstr(mqttClient.buffer, response) != NULL) {
            return 1;
        }
        if (strstr(mqttClient.buffer, "ERROR") != NULL) {
            return 0;
        }
        Delay_ms(10);
        startTime += 10;
    }
    return 0;
}

void MQTT_Init(void)
{
    memset(&mqttClient, 0, sizeof(MQTT_ClientTypeDef));
    Info("MQTT Client Init OK\r\n");
}

uint8_t MQTT_Connect(const char *clientID, const char *username, const char *password)
{
    uint16_t len = 0;
    uint16_t payloadLen = 0;
    uint16_t totalLen = 0;
    uint8_t packet[256];
    char cmd[32];
    uint8_t retry = 0;

    MQTT_Clear();

    payloadLen = 2 + strlen(clientID) + 2 + strlen(username) + 2 + strlen(password);
    totalLen = 10 + payloadLen;

    packet[len++] = 0x10;
    if (totalLen > 127) {
        packet[len++] = (totalLen & 0x7F) | 0x80;
        packet[len++] = (totalLen >> 7) & 0x7F;
    } else {
        packet[len++] = totalLen;
    }

    packet[len++] = 0x00;
    packet[len++] = 0x04;
    packet[len++] = 'M';
    packet[len++] = 'Q';
    packet[len++] = 'T';
    packet[len++] = 'T';
    packet[len++] = 0x04;
    packet[len++] = 0xC2;
    packet[len++] = (MQTT_KEEPALIVE >> 8) & 0xFF;
    packet[len++] = MQTT_KEEPALIVE & 0xFF;

    packet[len++] = (strlen(clientID) >> 8) & 0xFF;
    packet[len++] = strlen(clientID) & 0xFF;
    memcpy(&packet[len], clientID, strlen(clientID));
    len += strlen(clientID);

    packet[len++] = (strlen(username) >> 8) & 0xFF;
    packet[len++] = strlen(username) & 0xFF;
    memcpy(&packet[len], username, strlen(username));
    len += strlen(username);

    packet[len++] = (strlen(password) >> 8) & 0xFF;
    packet[len++] = strlen(password) & 0xFF;
    memcpy(&packet[len], password, strlen(password));
    len += strlen(password);

    sprintf(cmd, "AT+CIPSEND=%d\r\n", len);
    ESP8266_SendCmd(cmd);
    Delay_ms(200);

    if (strstr(mqttClient.buffer, ">") != NULL) {
    } else {
        Error("MQTT No Prompt\r\n");
        return 0;
    }

    MQTT_SendPacket(packet, len);

    retry = 0;
    while (retry < 50) {
        if (mqttClient.buffer[0] == 0x20 && mqttClient.buffer[2] == 0x00 && mqttClient.buffer[3] == 0x00) {
            mqttClient.connected = 1;
            Info("MQTT Connected: %s\r\n", clientID);
            return 1;
        }
        if (strstr(mqttClient.buffer, "ERROR") != NULL) {
            Error("MQTT Connect ERROR\r\n");
            return 0;
        }
        Delay_ms(100);
        retry++;
    }

    Error("MQTT Connect Timeout\r\n");
    return 0;
}

uint8_t MQTT_Subscribe(const char *topic)
{
    uint16_t len = 0;
    uint16_t topicLen = strlen(topic);
    uint16_t packetLen = 5 + topicLen + 2;
    uint8_t packet[128];
    char cmd[32];

    if (!mqttClient.connected) {
        Error("MQTT Subscribe FAIL: Not connected\r\n");
        return 0;
    }

    MQTT_Clear();

    packet[len++] = 0x82;
    packet[len++] = packetLen;
    packet[len++] = 0x00;
    packet[len++] = 0x01;

    packet[len++] = (topicLen >> 8) & 0xFF;
    packet[len++] = topicLen & 0xFF;
    memcpy(&packet[len], topic, topicLen);
    len += topicLen;

    packet[len++] = MQTT_QOS_LEVEL;

    sprintf(cmd, "AT+CIPSEND=%d\r\n", len);
    ESP8266_SendCmd(cmd);
    Delay_ms(200);

    if (strstr(mqttClient.buffer, ">") == NULL) {
        Error("MQTT Subscribe No Prompt\r\n");
        return 0;
    }

    Delay_ms(50);
    MQTT_SendPacket(packet, len);
    Delay_ms(500);

    if(strstr(mqttClient.buffer, "\x90\x03\x00\x01") != NULL)
    {
        Debug("MQTT Subscribe: %s\r\n", topic);
        return 1;
    }
    Error("MQTT Subscribe FAIL: %s\r\n", topic);
    return 0;
}

uint8_t MQTT_Publish(const char *topic, const char *payload)
{
    uint16_t topicLen = strlen(topic);
    uint16_t payloadLen = strlen(payload);
    uint16_t totalLen = 2 + topicLen + payloadLen;
    uint16_t idx = 0;
    uint8_t packet[512];
    char cmd[32];

    if (!mqttClient.connected) {
        Error("MQTT Publish FAIL: Not connected\r\n");
        return 0;
    }

    MQTT_Clear();

    packet[idx++] = 0x30;
    if (totalLen > 127) {
        packet[idx++] = (totalLen & 0x7F) | 0x80;
        packet[idx++] = (totalLen >> 7) & 0x7F;
    } else {
        packet[idx++] = totalLen;
    }

    packet[idx++] = (topicLen >> 8) & 0xFF;
    packet[idx++] = topicLen & 0xFF;
    memcpy(&packet[idx], topic, topicLen);
    idx += topicLen;

    memcpy(&packet[idx], payload, payloadLen);
    idx += payloadLen;

    sprintf(cmd, "AT+CIPSEND=%d\r\n", idx);
    ESP8266_SendCmd(cmd);
    Delay_ms(200);

    if (strstr(mqttClient.buffer, ">") == NULL) {
        Error("MQTT Publish No Prompt\r\n");
        return 0;
    }

    Delay_ms(50);
    MQTT_SendPacket(packet, idx);
    Delay_ms(300);

    Debug("MQTT Publish OK, Topic: %s\r\n", topic);
    return 1;
}

uint8_t MQTT_ProcessLoop(void)
{
    return mqttClient.connected;
}

uint8_t MQTT_IsConnected(void)
{
    return mqttClient.connected;
}

void MQTT_Disconnect(void)
{
    if (mqttClient.connected) {
        uint8_t packet[2] = {0xE0, 0x00};
        MQTT_SendPacket(packet, 2);
        mqttClient.connected = 0;
        Info("MQTT Disconnected\r\n");
    }
}
