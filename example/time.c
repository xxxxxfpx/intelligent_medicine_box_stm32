#define DS1302_RST PB12
#define DS1302_DAT PB13
#define DS1302_CLK PB14

void DS1302_Init(){GPIO_Init(DS1302_RST|DS1302_DAT|DS1302_CLK,OUT_PP,50MHz);RST=0;CLK=0;DAT=0;}

void DS1302_WriteReg(uint8_t addr,uint8_t dat){RST=1;addr&=0xFE;
    for(i=0;i<8;i++){DAT=addr&1;CLK=1;CLK=0;addr>>=1;}
    for(i=0;i<8;i++){DAT=dat&1;CLK=1;CLK=0;dat>>=1;}RST=0;}

uint8_t DS1302_ReadReg(uint8_t addr){uint8_t dat=0;RST=1;addr|=0x01;
    for(i=0;i<8;i++){DAT=addr&1;CLK=1;CLK=0;addr>>=1;}
    DAT_SetInput(IPU);
    for(i=0;i<8;i++){dat>>=1;if(DAT_Read)dat|=0x80;CLK=1;CLK=0;}DAT_SetOutput(PP);RST=0;return dat;}

uint8_t BcdToDec(uint8_t bcd){return bcd/16*10+bcd%16;}
uint8_t DecToBcd(uint8_t dec){return dec/10*16+dec%10;}

void DS1302_SetTime(uint8_t year,uint8_t month,uint8_t day,uint8_t hour,uint8_t minute,uint8_t second,uint8_t week){
    DS1302_WriteReg(0x8E,0x00);DS1302_WriteReg(0x80,0x80);
    DS1302_WriteReg(0x8C,DecToBcd(year));DS1302_WriteReg(0x88,DecToBcd(month));DS1302_WriteReg(0x86,DecToBcd(day));
    DS1302_WriteReg(0x84,DecToBcd(hour));DS1302_WriteReg(0x82,DecToBcd(minute));DS1302_WriteReg(0x80,DecToBcd(second));
    DS1302_WriteReg(0x8A,DecToBcd(week));DS1302_WriteReg(0x8E,0x80);}

void DS1302_GetTime(DS1302_TimeTypeDef* t){
    t->year=DS1302_ReadReg(0x8C);t->month=DS1302_ReadReg(0x88);t->day=DS1302_ReadReg(0x86);
    t->hour=DS1302_ReadReg(0x84);t->minute=DS1302_ReadReg(0x82);t->second=DS1302_ReadReg(0x80)&0x7F;
    t->week=DS1302_ReadReg(0x8A);
    t->year=BcdToDec(t->year);t->month=BcdToDec(t->month);t->day=BcdToDec(t->day);
    t->hour=BcdToDec(t->hour);t->minute=BcdToDec(t->minute);t->second=BcdToDec(t->second);
    t->week=BcdToDec(t->week);}
