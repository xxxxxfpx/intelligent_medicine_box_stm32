static uint8_t r[GPS_BUF]; static uint16_t i; static GPS_Info g;

void GPS_Init(void) {
    GPIO_InitTypeDef p; USART_InitTypeDef u; NVIC_InitTypeDef n;
    RCC_APB2PeriphClockCmd(RCC_GPIOA, ENABLE); RCC_APB1PeriphClockCmd(RCC_USART2, ENABLE);
    p.GPIO_Pin=GPIO_Pin_2; p.GPIO_Mode=GPIO_Mode_AF_PP; p.GPIO_Speed=SPEED; GPIO_Init(GPIOA,&p);
    p.GPIO_Pin=GPIO_Pin_3; p.GPIO_Mode=GPIO_Mode_IPU; GPIO_Init(GPIOA,&p);
    u.USART_BaudRate=GPS_BAUD; u.USART_WordLength=USART_WordLength_8b;
    u.USART_StopBits=USART_StopBits_1; u.USART_Parity=USART_Parity_No;
    u.USART_Mode=USART_Mode_Rx; USART_Init(USART2,&u);
    n.NVIC_IRQChannel=USART2_IRQn; n.NVIC_IRQChannelPreemptionPriority=PRI;
    n.NVIC_IRQChannelCmd=ENABLE; NVIC_Init(&n);
    USART_ITConfig(USART2,USART_IT_RXNE,ENABLE); USART_Cmd(USART2,ENABLE);
}
void GPS_ParseRMC(void) {
    char *p=r; uint8_t f=0;
    while((p=strchr(p,','))&&f<10) { p++; f++;
        if(f==2) g.valid=*p=='A';
        if(f==3) strncpy(g.lat,p,strcspn(p,",")); 
        if(f==5) strncpy(g.lon,p,strcspn(p,","));
    } g.updated=1;
}
void USART2_IRQHandler(void) {
    if(USART_GetITStatus(USART2,USART_IT_RXNE)) {
        uint8_t c=USART_ReceiveData(USART2);
        if(c=='$') i=0;
        if(i<GPS_BUF-1) r[i++]=c;
        if(c=='\n'&&r[1]=='G'&&r[3]=='R'&&r[4]=='M'&&r[5]=='C') GPS_ParseRMC();
    }
}

