#include <REGX52.H>
#include "Delay1ms.h"
sbit Key1 = P3^1;
sbit Key2 = P3^0;
sbit Key3 = P3^2;
sbit Key4 = P3^3;

unsigned char Key()
{
	unsigned char value = 0;
	if (Key1 == 0) {Delay1ms(20); while(Key1 == 0); Delay1ms(20); value = 1;}
	if (Key2 == 0) {Delay1ms(20); while(Key2 == 0); Delay1ms(20); value = 2;}
	if (Key3 == 0) {Delay1ms(20); while(Key3 == 0); Delay1ms(20); value = 3;}
	if (Key4 == 0) {Delay1ms(20); while(Key4 == 0); Delay1ms(20); value = 4;}
	return value;
}