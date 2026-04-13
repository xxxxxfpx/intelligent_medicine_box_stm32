#include <REGX52.H>
sbit DS1302_CE = P2^5;
sbit DS1302_SCLK = P2^3;
sbit DS1302_DATA = P2^4;

void DS1302_WRITE(unsigned char Cmd, unsigned char Data)
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
		DS1302_DATA = Data & (0x01 << i);
		DS1302_SCLK = 1;
		DS1302_SCLK = 0;
	}
	
	DS1302_CE = 0;
	
}

unsigned char DS1302_READ(unsigned char ADD)
{
	unsigned char Data = 0, i;
	DS1302_CE = 0;
	DS1302_SCLK = 0;
	
	DS1302_CE = 1;
	for (i = 0; i < 8; i++)
	{
		DS1302_SCLK = 0;
		DS1302_DATA = ADD & (0x01 << i);	
		DS1302_SCLK = 1;
	}
	
	for(i = 0; i < 8; i++)
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
	Data = (Data / 16 * 10) + (Data % 16);
	return Data;
}

