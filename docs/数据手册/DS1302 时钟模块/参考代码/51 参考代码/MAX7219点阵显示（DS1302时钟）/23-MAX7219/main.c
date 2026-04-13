#include <REGX52.H>
#include "MAX7219.H"
#include "Delay1ms.h"
#include "Timer0.h"
#include "DS1302.h"
unsigned char Hour, Min, Sec, MODE, Bright = 1;
void main()
{
	unsigned char i;
	MAX7219_Init();
	Timer0_Init();
//	DS1302_WRITE(0x8E, 0x00); // 关闭写保护
//	DS1302_WRITE(0x80, 0x00); // 秒
//	DS1302_WRITE(0x82, 0x07); // 分
//	DS1302_WRITE(0x84, 0x09); // 时
//	DS1302_WRITE(0x86, 0x19); // 日
//	DS1302_WRITE(0x88, 0x07); // 月
//	DS1302_WRITE(0x8C, 0x24); // 年
	while(1)
	{
		 
		if(P2_6 == 0)  // 外部按键调节亮度
		{
			Delay1ms(20);
			if(P2_6 == 0)
			{
				Bright++; 
				Bright %= 3;
				switch (Bright)
				{
					case 0: BrightSet = 1; break;
					case 1: BrightSet = 8; break;
					case 2: BrightSet = 15; break;
				}
				MAX7219_Init();			
			}

		} 
		
		
		Sec = DS1302_READ(0x81); // 读取秒
		Min = DS1302_READ(0x83); // 读取分
		Hour = DS1302_READ(0x85);// 读取时
		
		if (MODE)
		{
			for (i = 0; i < 8; i++)
			{
				DisplayData[0][i] = MAX7219DATA[Hour / 10][i]; // 更新缓存数据
				DisplayData[1][i] = MAX7219DATA[Hour % 10][i];
				DisplayData[2][i] = MAX7219DATA[10][i];
				DisplayData[3][i] = MAX7219DATA[Min / 10][i];
				DisplayData[4][i] = MAX7219DATA[Min % 10][i];
			}			
		}
		
		else
		{
			for (i = 0; i < 8; i++)
			{
				DisplayData[0][i] = MAX7219DATA[Min / 10][i]; // 更新缓存数据
				DisplayData[1][i] = MAX7219DATA[Min % 10][i];
				DisplayData[2][i] = MAX7219DATA[10][i];
				DisplayData[3][i] = MAX7219DATA[Sec / 10][i];
				DisplayData[4][i] = MAX7219DATA[Sec % 10][i];
			}				
		}
		
		MAX7219_Display(); // 更新显示
		
	}
}


void Timer0_Rountine(void) interrupt 1
{
	static unsigned int Count;
	Count++;
	TL0 = 0x66;				// 设置定时初始值
	TH0 = 0xFC;				// 设置定时初始值
	
	if(Count >= 35000)      // 约35秒切换一次显示内容
	{
		Count = 0;
		MODE = !MODE;
	}
	
}