#define MLX90614_ADDR 0x00
#define MLX90614_TA 0x06
#define MLX90614_TOBJ1 0x07

void MLX90614_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB | RCC_APB2Periph_AFIO, ENABLE);
    GPIO_PinRemapConfig(GPIO_Remap_SWJ_JTAGDisable, ENABLE);
    
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3 | GPIO_Pin_4;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_OD;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOB, &GPIO_InitStructure);
}

void MLX_IIC_Start(void)
{
    MLX_IIC_SDA = 1;
    MLX_IIC_SCL = 1;
    Delay_us(2);
    MLX_IIC_SDA = 0;
    Delay_us(2);
    MLX_IIC_SCL = 0;
}

void MLX_IIC_Stop(void)
{
    MLX_IIC_SDA = 0;
    MLX_IIC_SCL = 1;
    Delay_us(2);
    MLX_IIC_SDA = 1;
}

void MLX_IIC_Send_Byte(uint8_t txd)
{
    for (int t = 0; t < 8; t++)
    {
        MLX_IIC_SDA = (txd & 0x80) >> 7;
        txd <<= 1;
        Delay_us(2);
        MLX_IIC_SCL = 1;
        Delay_us(2);
        MLX_IIC_SCL = 0;
    }
}

uint8_t MLX90614_ReadByte(void)
{
    uint8_t receive = 0;
    MLX_IIC_SDA = 1;
    for (int i = 0; i < 8; i++)
    {
        MLX_IIC_SCL = 0;
        Delay_us(2);
        MLX_IIC_SCL = 1;
        receive <<= 1;
        if (MLX_READ_SDA) receive++;
        Delay_us(2);
    }
    return receive;
}

float MLX90614_ReadTemp(uint8_t reg)
{
    uint16_t rawTemp;
    
    MLX_IIC_Start();
    MLX_IIC_Send_Byte((MLX90614_ADDR << 1) | 0x00);
    MLX_IIC_Send_Byte(reg);
    MLX_IIC_Start();
    MLX_IIC_Send_Byte((MLX90614_ADDR << 1) | 0x01);
    
    uint8_t dataLow = MLX90614_ReadByte();
    uint8_t dataHigh = MLX90614_ReadByte();
    MLX_IIC_Stop();
    
    rawTemp = (uint16_t)dataHigh << 8 | dataLow;
    return (float)rawTemp * 0.02f - 273.15f;
}

float MLX90614_ReadObjectTemp(void)
{
    return MLX90614_ReadTemp(MLX90614_TOBJ1);
}
