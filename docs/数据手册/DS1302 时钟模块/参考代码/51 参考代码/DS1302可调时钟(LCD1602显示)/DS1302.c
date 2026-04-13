#include <REGX52.H>
#define   SecAdd     0x80
#define   MinAdd     0x82
#define   HourAdd    0x84
#define   DataAdd    0x86
#define   MonthAdd   0x88
#define   YearAdd    0x8C
#define   WeekAdd    0x8A
#define   WP         0x8E

sbit DS1302_CE = P1^2;
sbit DS1302_DATA = P1^1;
sbit DS1302_SCLK = P1^0;
char TimeData[] = {23, 07, 05, 11, 13, 7, 5};  // 年 月 日 时 分 秒 周

void DS1302_Write(unsigned char Cmd, Time)
{
	unsigned char i;
	DS1302_CE = 0;
	DS1302_SCLK = 0;
	
	DS1302_CE = 1;
	for (i = 0; i < 8; i++)
	{
		
		DS1302_DATA = Cmd & (0x01 << i);
		DS1302_SCLK = 1;
		DS1302_SCLK = 0;
		
	}
	
	for (i = 0; i < 8; i++)
	{
		
		DS1302_DATA = Time & (0x01 << i);
		DS1302_SCLK = 1;
		DS1302_SCLK = 0;
		
	}
	DS1302_CE = 0;
}

unsigned char DS1302_Read(unsigned char Adderss)
{
	unsigned char Data = 0x00, i;
	Adderss |= 0x01;
	DS1302_CE = 0;
	DS1302_SCLK = 0;
	
	DS1302_CE = 1;
	for (i = 0; i < 8; i++)
	{
		DS1302_SCLK = 0;
		DS1302_DATA = Adderss & (0x01 << i);
		DS1302_SCLK = 1;
	}
	
	for (i = 0; i < 8; i++)
	{
		DS1302_SCLK = 1;
		DS1302_SCLK = 0;
		if (DS1302_DATA)
		{
			Data |= (0x01 << i);
		}
	}
	
	DS1302_CE = 0;
	DS1302_DATA = 0;
	Data = ((Data / 16 * 10) + (Data % 16));
	return Data;
}

void Set_DS1302Time()
{
	DS1302_Write(WP, 0x00);
	DS1302_Write(YearAdd,  (TimeData[0] / 10 * 16) + (TimeData[0] % 10));
	DS1302_Write(MonthAdd, (TimeData[1] / 10 * 16) + (TimeData[1] % 10));
	DS1302_Write(DataAdd,  (TimeData[2] / 10 * 16) + (TimeData[2] % 10));
	DS1302_Write(HourAdd,  (TimeData[3] / 10 * 16) + (TimeData[3] % 10));
	DS1302_Write(MinAdd,   (TimeData[4] / 10 * 16) + (TimeData[4] % 10));
	DS1302_Write(SecAdd,   (TimeData[5] / 10 * 16) + (TimeData[5] % 10));
	DS1302_Write(WeekAdd,  (TimeData[6] / 10 * 16) + (TimeData[6] % 10));
	DS1302_Write(WP, 0x80);
}

void Read_DS1302Time()
{

	TimeData[0] = DS1302_Read(YearAdd);
	TimeData[1] = DS1302_Read(MonthAdd);
	TimeData[2] = DS1302_Read(DataAdd);
	TimeData[3] = DS1302_Read(HourAdd);
	TimeData[4] = DS1302_Read(MinAdd);
	TimeData[5] = DS1302_Read(SecAdd);
	TimeData[6] = DS1302_Read(WeekAdd);
}