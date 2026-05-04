/**
 * 智能药盒项目 - 配置文件
 * 所有可配置的常量集中在此文件中
 */

#ifndef __SETTINGS_H
#define __SETTINGS_H

#include "stm32f10x.h"

/*======================== WiFi 配置 ========================*/
#define WIFI_USERNAME  "115369"
#define WIFI_PASSWORD  "12345677"

/*======================== MQTT 配置 ========================*/
#define MQTT_BROKER_HOST     "mqtts.heclouds.com"
#define MQTT_BROKER_PORT     1883
#define MQTT_CLIENT_ID       "BOX"
#define MQTT_USERNAME         "y8PjkMrwSb"
#define MQTT_PASSWORD         "version=2018-10-31&res=products%2Fy8PjkMrwSb%2Fdevices%2FBOX&et=1787378552&method=md5&sign=Uw4%2Fsd8ngftzYB1xYZzHtQ%3D%3D"
#define MQTT_KEEPALIVE       120
#define MQTT_QOS_LEVEL       0

/* MQTT Topic定义 - 使用常量拼接 */
#define MQTT_TOPIC_PREFIX      "$sys"
#define MQTT_PRODUCT_ID       "y8PjkMrwSb"
#define MQTT_DEVICE_NAME      "BOX"
#define MQTT_TOPIC_PROP_POST  MQTT_TOPIC_PREFIX "/" MQTT_PRODUCT_ID "/" MQTT_DEVICE_NAME "/thing/property/post"
#define MQTT_TOPIC_PROP_SET   MQTT_TOPIC_PREFIX "/" MQTT_PRODUCT_ID "/" MQTT_DEVICE_NAME "/thing/property/set"
#define MQTT_TOPIC_PROP_POST_REPLY MQTT_TOPIC_PREFIX "/" MQTT_PRODUCT_ID "/" MQTT_DEVICE_NAME "/thing/property/post/reply"

#define MQTT_PUBLISH_INTERVAL 5000  // 数据上传周期(ms)

/*======================== GPS 配置 ========================*/
#define GPS_UART_BAUD 9600

/*======================== OLED 配置 ========================*/
#define OLED_I2C_ADDR 0x78

/*======================== ESP8266 配置 ========================*/
#define ESP8266_UART_BAUD 115200
#define ESP8266_TIMEOUT   20000  // WiFi连接超时时间(ms)

/*======================== DS1302 配置 ========================*/
#define DS1302_DEFAULT_YEAR    26
#define DS1302_DEFAULT_MONTH   4
#define DS1302_DEFAULT_DAY     17
#define DS1302_DEFAULT_HOUR    12
#define DS1302_DEFAULT_MINUTE  0
#define DS1302_DEFAULT_SECOND  0
#define DS1302_DEFAULT_WEEK    5

/*======================== MLX90614 配置 ========================*/
#define MLX90614_I2C_ADDR 0x5A

/*======================== 系统配置 ========================*/
#define SYSTEM_DISPLAY_REFRESH_MS 500  // 显示屏刷新周期(ms)
#define SYSTEM_TEMP_DECIMAL_PLACES 1   // 温度显示小数位数

#endif