#include <REGX52.H>
#include "DS1302.h"
#include "LCD1602.h"
#include "Key.h"
#include "Timer0.h"
unsigned char Mode, Flag, KeyValue, DisplayFlag;

void main()
{
	LCD1602_Init();
	Timer0_Init();
	LCD_ShowString(1, 1, "  -  -  ");
	LCD_ShowString(2, 1, "  :  :  ");	
	LCD_ShowString(1, 12, "Day:");
	
	while(1)
	{
		TR0 = 0;
		KeyValue = Key();
		TR0 = 1;
		if (KeyValue == 1) 
		{
			Mode = !Mode;
			Flag = 0;
		}
		
		if (Mode)
		{
			if (KeyValue == 2)
			{
				Flag++;
				Flag %= 7;
			}
			if (KeyValue == 3)
			{
				TimeData[Flag]++;
				if (Flag == 0) {if (TimeData[Flag] > 99) TimeData[Flag] = 0;
					if(TimeData[Flag] % 4 == 0 && TimeData[1] == 2 && TimeData[2] > 29) TimeData[2] = 1;
					if(TimeData[Flag] % 4 != 0 && TimeData[1] == 2 && TimeData[2] > 28) TimeData[2] = 1;
				}
				if (Flag == 1) {if (TimeData[Flag] > 12) TimeData[Flag] = 1;
					if(TimeData[Flag] == 4 || TimeData[Flag] == 6 ||
					   TimeData[Flag] == 9 || TimeData[Flag] == 11)  
					{
						if(TimeData[2] > 30) TimeData[2] = 1;
					}
					
				}
				
				if (Flag == 2) {
					if (TimeData[1] == 1 || TimeData[1] == 3 || 
						TimeData[1] == 5 || TimeData[1] == 7 || 
						TimeData[1] == 8 || TimeData[1] == 10 || 
						TimeData[1] == 12)
					{
						if (TimeData[Flag] > 31) TimeData[Flag] = 1;
					}
					
					else if (TimeData[1] == 4 || TimeData[1] == 6 || 
						     TimeData[1] == 9 || TimeData[1] == 11)
					{
						if (TimeData[Flag] > 30) TimeData[Flag] = 1;
					}
					
					
					else if ( TimeData[1] == 2 && TimeData[0] % 4 != 0 && TimeData[Flag] > 28)
					{
						TimeData[Flag] = 1;
					}
					
					
					else if ( TimeData[1] == 2 && TimeData[0] % 4 == 0 && TimeData[Flag] > 29)
					{
						TimeData[Flag] = 1;
					}
					
				}
								
				
				if (Flag == 3) {if (TimeData[Flag] > 23) TimeData[Flag] = 0;}
				if (Flag == 4) {if (TimeData[Flag] > 59) TimeData[Flag] = 0;}
				if (Flag == 5) {if (TimeData[Flag] > 59) TimeData[Flag] = 0;}
				if (Flag == 6) {if (TimeData[Flag] > 7) TimeData[Flag] = 1;}
				
			}
			if (KeyValue == 4)
			{
				TimeData[Flag]--;
				if (Flag == 0) {if (TimeData[Flag] < 0) TimeData[Flag] = 99;
					if(TimeData[Flag] % 4 == 0 && TimeData[1] == 2 && TimeData[2] < 29) TimeData[2] = 1;
					if(TimeData[Flag] % 4 != 0 && TimeData[1] == 2 && TimeData[2] > 28) TimeData[2] = 1;
				}
				if (Flag == 1) {if (TimeData[Flag] < 1) TimeData[Flag] = 12;
					if(TimeData[Flag] == 4 || TimeData[Flag] == 6 ||
					   TimeData[Flag] == 9 || TimeData[Flag] == 11
					)  
					{
						if (TimeData[2] > 30) TimeData[2] = 1;
					}
					
				}
				
				if (Flag == 2) {
					if (TimeData[1] == 1 || TimeData[1] == 3 || 
						TimeData[1] == 5 || TimeData[1] == 7 || 
						TimeData[1] == 8 || TimeData[1] == 10 || 
						TimeData[1] == 12)
					{
						if (TimeData[Flag] < 1) TimeData[Flag] = 31;
						
					}
					
					else if (TimeData[1] == 4 || TimeData[1] == 6 || 
						     TimeData[1] == 9 || TimeData[1] == 11)
					{
						if (TimeData[Flag] < 1 ) TimeData[Flag] = 30;
						
					}
					
					
					else if ( TimeData[1] == 2 && TimeData[0] % 4 != 0 && TimeData[Flag] < 1)
					{
						TimeData[Flag] = 28;
					}
					
					
					else if ( TimeData[1] == 2 && TimeData[0] % 4 == 0 && TimeData[Flag] < 1)
					{
						TimeData[Flag] = 28;
					}
					
				}
				
				if (Flag == 3) {if (TimeData[Flag] < 0) TimeData[Flag] = 23;}
				if (Flag == 4) {if (TimeData[Flag] < 0) TimeData[Flag] = 59;}
				if (Flag == 5) {if (TimeData[Flag] < 0) TimeData[Flag] = 59;}
				if (Flag == 6) {if (TimeData[Flag] < 1) TimeData[Flag] = 7;}				
			}
			Set_DS1302Time();
			
			if (DisplayFlag)
			{
				LCD_ShowNub(1, 1,  TimeData[0], 2);	
				LCD_ShowNub(1, 4,  TimeData[1], 2);	
				LCD_ShowNub(1, 7,  TimeData[2], 2);
				LCD_ShowNub(2, 1,  TimeData[3], 2);
				LCD_ShowNub(2, 4,  TimeData[4], 2);
				LCD_ShowNub(2, 7,  TimeData[5], 2);
				LCD_ShowNub(1, 16, TimeData[6], 1);				
			}
			else
			{
				if (Flag == 0) LCD_ShowString(1, 1,  "  ");
				if (Flag == 1) LCD_ShowString(1, 4,  "  ");
				if (Flag == 2) LCD_ShowString(1, 7,  "  ");
				if (Flag == 3) LCD_ShowString(2, 1,  "  ");
				if (Flag == 4) LCD_ShowString(2, 4,  "  ");
				if (Flag == 5) LCD_ShowString(2, 7,  "  ");
				if (Flag == 6) LCD_ShowString(1, 16,  " ");			
				
			}
			
		}
		else
		{
			Read_DS1302Time();
			LCD_ShowNub(1, 1,  TimeData[0], 2);
			LCD_ShowNub(1, 4,  TimeData[1], 2);
			LCD_ShowNub(1, 7,  TimeData[2], 2);
			LCD_ShowNub(2, 1,  TimeData[3], 2);
			LCD_ShowNub(2, 4,  TimeData[4], 2);
			LCD_ShowNub(2, 7,  TimeData[5], 2);
			LCD_ShowNub(1, 16, TimeData[6], 1);				
		}
		
	}
}


void Timer0_Rountine(void) interrupt 1
{
	static unsigned int count;
	count++;
	TL0 = 0x66;				//设置定时初始值
	TH0 = 0xFC;				//设置定时初始值
	if(count >= 500)
	{
		count = 0;
		DisplayFlag = !DisplayFlag;
	}
}
