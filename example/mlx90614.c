void MLX_Init(void) {
    GPIO_InitTypeDef p;
    RCC_APB2PeriphClockCmd(RCC_GPIOB|RCC_AFIO, ENABLE);
    GPIO_PinRemapConfig(GPIO_Remap_SWJ_JTAGDisable, ENABLE);
    p.GPIO_Pin=GPIO_Pin_3|GPIO_Pin_4; p.GPIO_Mode=GPIO_Mode_Out_OD; p.GPIO_Speed=SPEED;
    GPIO_Init(GPIOB,&p); SCL=1; SDA=1;
}
static void I2C_Start(void) { SDA=1;SCL=1;Delay_us(DELAY);SDA=0;Delay_us(DELAY);SCL=0;}
static void I2C_Stop(void)  { SDA=0;SCL=1;Delay_us(DELAY);SDA=1;}
static void I2C_Wr(uint8_t d) {
    for(int t=8;t--;d<<=1) { SDA=(d&0x80)!=0; Delay_us(DELAY);SCL=1;Delay_us(DELAY);SCL=0; }
}
static uint8_t I2C_Rd(void) {
    uint8_t v=0; SDA=1;
    for(int t=8;t--;) { SCL=0;Delay_us(DELAY);SCL=1;v=(v<<1)|RDSDA;Delay_us(DELAY); }
    return v;
}
float MLX_ReadTemp(uint8_t reg) {
    uint16_t raw;
    I2C_Start(); I2C_Wr(MLX_ADDR<<1); I2C_Wr(reg);
    I2C_Start(); I2C_Wr((MLX_ADDR<<1)|1);
    raw=I2C_Rd(); raw<<=8; raw|=I2C_Rd(); I2C_Stop();
    return raw*K-273.15f;
}
float MLX_ReadObj(void)  { return MLX_ReadTemp(REG_OBJ); }
float MLX_ReadAmb(void)  { return MLX_ReadTemp(REG_AMB); }



