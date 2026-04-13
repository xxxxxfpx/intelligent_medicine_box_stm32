#ifndef __MAX7219_H__
#define __MAX7219_H__

void MAX7219_Init();
void MAX7219_Display();
void MAX7219_Write(unsigned char Add, Data);
void MAX7219_ShowNub(unsigned char Position, Nub);
extern unsigned char DisplayData[4][8];
extern unsigned char code MAX7219DATA[][8];
extern unsigned char BrightSet;
#endif