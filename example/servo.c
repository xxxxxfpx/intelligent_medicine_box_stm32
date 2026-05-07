static uint8_t busy; static uint32_t t0, dur;
void Servo_Init(void) {
    GPIO_InitTypeDef p; TIM_TimeBaseInitTypeDef b; TIM_OCInitTypeDef o;
    RCC_APB1PeriphClockCmd(RCC_TIM3, ENABLE); RCC_APB2PeriphClockCmd(RCC_GPIOA, ENABLE);
    p.GPIO_Pin=SERVO_PIN; p.GPIO_Mode=GPIO_Mode_AF_PP; p.GPIO_Speed=SPEED; GPIO_Init(SERVO_PORT,&p);
    b.TIM_Period=PERIOD-1; b.TIM_Prescaler=PSC-1; b.TIM_CounterMode=TIM_CounterMode_Up;
    TIM_TimeBaseInit(SERVO_TIM,&b);
    o.TIM_OCMode=TIM_OCMode_PWM1; o.TIM_OutputState=TIM_OutputState_Enable;
    o.TIM_Pulse=NULL_PULSE; o.TIM_OCPolarity=TIM_OCPolarity_High;
    TIM_OC1Init(SERVO_TIM,&o); TIM_Cmd(SERVO_TIM,ENABLE);
}
void Servo_SetSpeed(int8_t v) {
    if(v>100)v=100; if(v<-100)v=-100; TIM_SetCompare1(SERVO_TIM,NEUTRAL+v*SLOPE);
}
void Servo_Stop(void)   { TIM_SetCompare1(SERVO_TIM,NEUTRAL); }
void Servo_Forward(void) { Servo_SetSpeed(100); }
void Servo_Backward(void){ Servo_SetSpeed(-100); }
void Servo_Rotate(int16_t deg) {
    if(busy||!deg) return;
    deg>0?Servo_Forward():Servo_Backward();
    if(deg<0) deg=-deg; if(deg>360) deg=360;
    dur=(uint32_t)deg*FULL_ROT_MS/360; if(dur<MIN_MS)dur=MIN_MS;
    busy=1; t0=GetMillis();
}
void Servo_Tick(void) { if(busy&&GetMillis()-t0>=dur) { Servo_Stop(); busy=0; } }

