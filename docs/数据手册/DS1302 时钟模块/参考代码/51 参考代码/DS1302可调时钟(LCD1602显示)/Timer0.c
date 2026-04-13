#include <REGX52.H>

void Timer0_Init(void)		//1毫秒@11.0592MHz
{
	TMOD &= 0xF0;			//设置定时器模式
	TMOD |= 0x01;			//设置定时器模式
	TL0 = 0x66;				//设置定时初始值
	TH0 = 0xFC;				//设置定时初始值
	TF0 = 0;				//清除TF0标志
	TR0 = 1;				//定时器0开始计时
	EA = 1;
	ET0 = 1;
}

//void Timer0_Rountine(void) interrupt 1
//{
//	static unsigned int count;
//	count++;
//	TL0 = 0x66;				//设置定时初始值
//	TH0 = 0xFC;				//设置定时初始值
//	if(count >= 1000)
//	{
//		count = 0;
//	}
//}