static uint8_t r[RX_BUF]; static uint16_t i;

void ESP_Send(const char *s) { while (*s) { USART_SendData(USART3, *s++); while (!USART_FLAG_TXE); } }
uint8_t ESP_Wait(const char *k, uint32_t ms) { uint32_t t=0; while (t<ms) { if (strstr(r,k)) return 1; Delay_ms(10); t+=10; } return 0; }

void WiFi_Join(const char *id, const char *pw) {
    char b[128]; i=0; sprintf(b, "AT+CWJAP=\"%s\",\"%s\"\r\n", id, pw); ESP_Send(b); ESP_Wait("GOT IP", WIFI_TO);
}
void MQTT_Cfg(const char *cid, const char *u, const char *p) {
    char b[256]; i=0; sprintf(b, "AT+MQTTUSERCFG=0,1,\"%s\",\"%s\",\"%s\",0,0,\"\"\r\n", cid,u,p); ESP_Send(b); ESP_Wait("OK", MQ_TO);
}
void MQTT_Conn(const char *h, uint16_t pt) {
    char b[128]; i=0; sprintf(b, "AT+MQTTCONN=0,\"%s\",%d,0\r\n", h, pt); ESP_Send(b); ESP_Wait("+MQTTCONNECTED:", MQ_TO);
}
void MQTT_Pub(const char *tp, const char *d) {
    char b[512]; i=0; sprintf(b, "AT+MQTTPUB=0,\"%s\",\"%s\",0,0\r\n", tp, d); ESP_Send(b); ESP_Wait("OK", MQ_TO);
}
void USART3_IRQHandler(void) {
     if (USART_GetITStatus(USART3, USART_IT_RXNE)) {
         uint8_t c=USART_ReceiveData(USART3); 
         if (i<RX_BUF-1) { r[i++]=c; r[i]=0; } 
        } 
    }

