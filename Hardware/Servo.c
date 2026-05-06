/**
 * SG90 360度舵机驱动实现
 * PWM频率: 50Hz (周期20ms)
 * 信号线: PA6 (TIM3_CH1)
 *
 * 360度舵机控制角度 = 速度% × 运行时间
 * 满速(100%) 转 360° 约需 1800ms，需实际校准
 */

#include "Servo.h"
#include "Delay.h"
#include "USART1.h"

#define SERVO_FULL_ROTATION_MS  432

static volatile uint8_t servoRotating = 0;
static uint32_t servoStartTime = 0;
static uint32_t servoDurationMs = 0;

void Servo_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
    TIM_OCInitTypeDef TIM_OCInitStructure;

    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);

    GPIO_InitStructure.GPIO_Pin = SERVO_GPIO_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(SERVO_GPIO_PORT, &GPIO_InitStructure);

    TIM_TimeBaseStructure.TIM_Period = 200 - 1;
    TIM_TimeBaseStructure.TIM_Prescaler = 7200 - 1;
    TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;
    TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseInit(SERVO_TIM, &TIM_TimeBaseStructure);

    TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;
    TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
    TIM_OCInitStructure.TIM_Pulse = 0;
    TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;
    TIM_OC1Init(SERVO_TIM, &TIM_OCInitStructure);
    TIM_OC1PreloadConfig(SERVO_TIM, TIM_OCPreload_Enable);

    TIM_Cmd(SERVO_TIM, ENABLE);

    Info("Servo Init OK\r\n");
}

void Servo_360_SetSpeed(int8_t speed)
{
    uint16_t compare;
    int16_t clampedSpeed;

    if (speed < -100) clampedSpeed = -100;
    else if (speed > 100) clampedSpeed = 100;
    else clampedSpeed = speed;

    compare = 15 + clampedSpeed * 0.1f;
    TIM_SetCompare1(SERVO_TIM, compare);
    Debug("Servo Speed: %d\r\n", clampedSpeed);
}

void Servo_360_Stop(void)
{
    TIM_SetCompare1(SERVO_TIM, 15);
    Debug("Servo Stop\r\n");
}

void Servo_360_Forward(void)
{
    Servo_360_SetSpeed(100);
    Debug("Servo Forward\r\n");
}

void Servo_360_Backward(void)
{
    Servo_360_SetSpeed(-100);
    Debug("Servo Backward\r\n");
}

void Servo_RotateAngle(int16_t angle)
{
    uint32_t durationMs;

    if(angle == 0) return;
    if(servoRotating) return;

    if(angle > 0)
    {
        Servo_360_Forward();
    }
    else
    {
        Servo_360_Backward();
        angle = -angle;
    }

    if(angle > 360) angle = 360;

    durationMs = (uint32_t)angle * SERVO_FULL_ROTATION_MS / 360;
    if(durationMs < 50) durationMs = 50;

    servoRotating = 1;
    servoStartTime = Delay_GetTime();
    servoDurationMs = durationMs;

    Info("Servo Rotate %d deg, run %lu ms\r\n",
         angle, (unsigned long)durationMs);
}

uint8_t Servo_IsRotating(void)
{
    return servoRotating;
}

void Servo_Tick(void)
{
    if(!servoRotating) return;

    if((Delay_GetTime() - servoStartTime) >= servoDurationMs)
    {
        Servo_360_Stop();
        servoRotating = 0;
        Info("Servo Rotate Done\r\n");
    }
}