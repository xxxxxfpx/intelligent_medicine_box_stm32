/**
 * SG90 360度舵机驱动
 * 使用TIM3 PWM控制
 * 接线：信号线(橙色) -> PA6
 */

#ifndef __SERVO_H
#define __SERVO_H

#include <stm32f10x.h>

#define SERVO_GPIO_PORT  GPIOA
#define SERVO_GPIO_PIN   GPIO_Pin_6
#define SERVO_TIM       TIM3
#define SERVO_TIM_CHANNEL TIM_Channel_1

void Servo_Init(void);
void Servo_360_SetSpeed(int8_t speed);
void Servo_360_Stop(void);
void Servo_360_Forward(void);
void Servo_360_Backward(void);
void Servo_RotateAngle(int16_t angle);
uint8_t Servo_IsRotating(void);
void Servo_Tick(void);

#endif