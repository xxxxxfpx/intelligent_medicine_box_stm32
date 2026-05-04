/**
 * MLX90614红外温度传感器驱动
 * 使用软件I2C通信 (PB3=SCL, PB4=SDA)
 * 单设备系统使用地址0x00
 * 可测量环境温度和物体温度
 * 注意: PB3/PB4默认被JTAG占用，需要禁用JTAG
 */

#include "MLX90614.h"
#include "Delay.h"
#include "USART1.h"

#define BITBAND(addr, bitnum) ((addr & 0xF0000000)+0x2000000+((addr &0xFFFFF)<<5)+(bitnum<<2)) 
#define MEM_ADDR(addr)  *((volatile unsigned long  *)(addr)) 
#define BIT_ADDR(addr, bitnum) MEM_ADDR(BITBAND(addr, bitnum)) 

#define GPIOB_ODR_Addr    (GPIOB_BASE+12)
#define GPIOB_IDR_Addr    (GPIOB_BASE+8)

#define PBout(n)   BIT_ADDR(GPIOB_ODR_Addr,n)
#define PBin(n)    BIT_ADDR(GPIOB_IDR_Addr,n)

#define MLX_IIC_SCL    PBout(3)
#define MLX_IIC_SDA    PBout(4)
#define MLX_READ_SDA   PBin(4)

static void MLX_IIC_Delay(void)
{
    Delay_us(2);
}

static void MLX_IIC_Start(void)
{
    MLX_IIC_SDA = 1;
    MLX_IIC_SCL = 1;
    MLX_IIC_Delay();
    MLX_IIC_SDA = 0;
    MLX_IIC_Delay();
    MLX_IIC_SCL = 0;
    MLX_IIC_Delay();
}

static void MLX_IIC_Stop(void)
{
    MLX_IIC_SDA = 0;
    MLX_IIC_SCL = 1;
    MLX_IIC_Delay();
    MLX_IIC_SDA = 1;
    MLX_IIC_Delay();
}

static uint8_t MLX_IIC_Wait_Ack(void)
{
    uint8_t ucErrTime = 0;
    
    MLX_IIC_SDA = 1;
    MLX_IIC_Delay();
    MLX_IIC_SCL = 1;
    MLX_IIC_Delay();
    
    while (MLX_READ_SDA)
    {
        ucErrTime++;
        if (ucErrTime > 250)
        {
            MLX_IIC_Stop();
            Error("MLX90614 I2C Ack Timeout\r\n");
            return 1;
        }
    }
    MLX_IIC_SCL = 0;
    MLX_IIC_Delay();
    return 0;
}

static void MLX_IIC_Ack(void)
{
    MLX_IIC_SCL = 0;
    MLX_IIC_SDA = 0;
    MLX_IIC_Delay();
    MLX_IIC_SCL = 1;
    MLX_IIC_Delay();
    MLX_IIC_SCL = 0;
    MLX_IIC_Delay();
    MLX_IIC_SDA = 1;
}

static void MLX_IIC_NAck(void)
{
    MLX_IIC_SCL = 0;
    MLX_IIC_SDA = 1;
    MLX_IIC_Delay();
    MLX_IIC_SCL = 1;
    MLX_IIC_Delay();
    MLX_IIC_SCL = 0;
    MLX_IIC_Delay();
}

static void MLX_IIC_Send_Byte(uint8_t txd)
{
    uint8_t t;
    MLX_IIC_SCL = 0;
    for (t = 0; t < 8; t++)
    {
        MLX_IIC_SDA = (txd & 0x80) >> 7;
        txd <<= 1;
        MLX_IIC_Delay();
        MLX_IIC_SCL = 1;
        MLX_IIC_Delay();
        MLX_IIC_SCL = 0;
        MLX_IIC_Delay();
    }
    MLX_IIC_SDA = 1;
}

static uint8_t MLX_IIC_Read_Byte(unsigned char ack)
{
    unsigned char i, receive = 0;
    MLX_IIC_SDA = 1;
    for (i = 0; i < 8; i++)
    {
        MLX_IIC_SCL = 0;
        MLX_IIC_Delay();
        MLX_IIC_SCL = 1;
        receive <<= 1;
        if (MLX_READ_SDA) receive++;
        MLX_IIC_Delay();
    }
    if (ack)
        MLX_IIC_Ack();
    else
        MLX_IIC_NAck();
    return receive;
}

void MLX90614_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB | RCC_APB2Periph_AFIO, ENABLE);

    GPIO_PinRemapConfig(GPIO_Remap_SWJ_JTAGDisable, ENABLE);

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3 | GPIO_Pin_4;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_OD;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOB, &GPIO_InitStructure);

    MLX_IIC_SCL = 1;
    MLX_IIC_SDA = 1;

    Info("MLX90614 Init OK\r\n");
}

static uint16_t MLX90614_Read16(uint8_t reg)
{
    uint16_t data;
    uint8_t dataLow, dataHigh, pec;

    MLX_IIC_Start();
    MLX_IIC_Send_Byte((MLX90614_ADDR << 1) | 0x00);
    if (MLX_IIC_Wait_Ack())
    {
        MLX_IIC_Stop();
        return 0xFFFF;
    }
    MLX_IIC_Send_Byte(reg);
    if (MLX_IIC_Wait_Ack())
    {
        MLX_IIC_Stop();
        return 0xFFFF;
    }
    
    MLX_IIC_Start();
    MLX_IIC_Send_Byte((MLX90614_ADDR << 1) | 0x01);
    if (MLX_IIC_Wait_Ack())
    {
        MLX_IIC_Stop();
        return 0xFFFF;
    }
    
    dataLow = MLX_IIC_Read_Byte(1);
    dataHigh = MLX_IIC_Read_Byte(1);
    pec = MLX_IIC_Read_Byte(0);
    MLX_IIC_Stop();

    data = (uint16_t)dataHigh << 8 | dataLow;

    return data;
}

float MLX90614_ReadTemp(uint8_t reg)
{
    uint16_t rawTemp;
    float temp;

    rawTemp = MLX90614_Read16(reg);
    if (rawTemp == 0xFFFF) 
    {
        Error("MLX90614 Read Reg 0x%02X Failed\r\n", reg);
        return -273.15f;
    }

    temp = (float)rawTemp * 0.02f - 273.15f;

    Debug("MLX90614 Reg 0x%02X: raw=0x%04X, temp=%.2fC\r\n", reg, rawTemp, temp);

    return temp;
}

float MLX90614_ReadAmbientTemp(void)
{
    return MLX90614_ReadTemp(MLX90614_TA);
}

float MLX90614_ReadObjectTemp(void)
{
    return MLX90614_ReadTemp(MLX90614_TOBJ1);
}

uint8_t MLX90614_TestConnection(void)
{
    uint8_t result;
    
    MLX_IIC_Start();
    MLX_IIC_Send_Byte((MLX90614_ADDR << 1) | 0x00);
    result = MLX_IIC_Wait_Ack();
    MLX_IIC_Stop();
    
    if(result == 0)
    {
        Info("MLX90614 Connection OK\r\n");
        return 1;
    }
    else
    {
        Error("MLX90614 Connection FAIL\r\n");
        return 0;
    }
}