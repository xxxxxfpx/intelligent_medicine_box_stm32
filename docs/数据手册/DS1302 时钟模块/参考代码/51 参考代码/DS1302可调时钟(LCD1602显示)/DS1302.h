#ifndef __DS1302_H__
#define __DS1302_H__

void DS1302_Write(unsigned char Cmd, Time);
unsigned char DS1302_Read(unsigned char Adderss);
extern char TimeData[];
void Set_DS1302Time();
void Read_DS1302Time();

#endif