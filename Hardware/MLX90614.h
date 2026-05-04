/**
 * MLX90614红外温度传感器驱动
 * 使用软件I2C通信 (PB3=SCL, PB4=SDA)
 * 单设备系统使用地址0x00
 * 可测量环境温度和物体温度
 * 注意: PB3/PB4默认被JTAG占用，需要禁用JTAG
 */

#ifndef __MLX90614_H
#define __MLX90614_H

#include "stm32f10x.h"

#define MLX90614_ADDR       0x00

#define MLX90614_TA         0x06
#define MLX90614_TOBJ1      0x07

void MLX90614_Init(void);
float MLX90614_ReadAmbientTemp(void);
float MLX90614_ReadObjectTemp(void);
uint8_t MLX90614_TestConnection(void);

#endif