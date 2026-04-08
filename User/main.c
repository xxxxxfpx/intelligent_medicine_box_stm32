#include "stm32f10x.h"   // Device header

int main(void){

	OLED_Init();
	OLED_ShowChar(3, 2, 'R');
	while(1){
		Delay_s(1);
		OLED_ShowChar(3, 2, 'R');
		Delay_s(1);
		OLED_ShowChar(3, 2, 'M');
	}
}
