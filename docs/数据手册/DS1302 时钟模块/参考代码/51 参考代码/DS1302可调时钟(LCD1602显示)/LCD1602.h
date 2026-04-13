#ifndef __LCD1602_H__
#define __LCD1602_H__

void LCD1602_Init();
void LCD1602_WriteCmd(unsigned char Cmd);
void LCD1602_WriteData(unsigned char Data);
void LCD_ShowChar(unsigned char Rows, unsigned char Cols, unsigned char Char);
void LCD_ShowString(unsigned char Rows, unsigned char Cols, unsigned char String[]);
void LCD_ShowNub(unsigned char Rows, unsigned char Cols, unsigned int Nub, unsigned char Len);
void LCD_ShowSignedNub(unsigned char Rows, unsigned char Cols, int Nub, unsigned char Len);
void LCD_ShowHex(unsigned char Rows, unsigned char Cols, unsigned int Hex, unsigned char Len);
void LCD_ShowBin(unsigned char Rows, unsigned char Cols, unsigned int Bin, unsigned char Len);

#endif