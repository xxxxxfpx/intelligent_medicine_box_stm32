#define SERVO_TIM TIM3
#define SERVO_GPIO_PORT GPIOA
#define SERVO_GPIO_PIN GPIO_Pin_6

void Servo_Init(void)
{
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
    
    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Pin = SERVO_GPIO_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(SERVO_GPIO_PORT, &GPIO_InitStructure);
    
    TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
    TIM_TimeBaseStructure.TIM_Period = 200 - 1;
    TIM_TimeBaseStructure.TIM_Prescaler = 7200 - 1;
    TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseInit(SERVO_TIM, &TIM_TimeBaseStructure);
    
    TIM_OCInitTypeDef TIM_OCInitStructure;
    TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;
    TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
    TIM_OCInitStructure.TIM_Pulse = 15;
    TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;
    TIM_OC1Init(SERVO_TIM, &TIM_OCInitStructure);
    
    TIM_Cmd(SERVO_TIM, ENABLE);
}

void Servo_360_SetSpeed(int8_t speed)
{
    uint16_t compare = 15 + speed * 0.1f;
    TIM_SetCompare1(SERVO_TIM, compare);
}

void Servo_360_Stop(void)
{
    TIM_SetCompare1(SERVO_TIM, 15);
}

void Servo_360_Forward(void)
{
    Servo_360_SetSpeed(100);
}

void Servo_360_Backward(void)
{
    Servo_360_SetSpeed(-100);
}
